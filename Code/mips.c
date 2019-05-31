#include "mips.h"
#include "mips-data.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

/* core */
void gen_mips_framework(FILE *fp);
iclistnode_t *gen_mips_dispatch(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_label(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_funcdef(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_assign(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_arithbop(FILE *fp, iclistnode_t *cur);
void gen_mips_arithbop_add(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs);
void gen_mips_arithbop_sub(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs);
void gen_mips_arithbop_mul(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs);
void gen_mips_arithbop_div(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs);
iclistnode_t *gen_mips_goto(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_dec(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_param(FILE *fp, iclistnode_t *cur);

/* generate basic mips instruction */
void gen_mips_tag(FILE *fp, const char *tag, ...);
void gen_mips_jmp_tag(FILE *fp, const char *jmpcmd, const char *tag, ...);
void gen_mips_addi(FILE *fp, int rt, int rs, int i);
void gen_mips_add(FILE *fp, int rd, int rs, int rt);
void gen_mips_sub(FILE *fp, int rd, int rs, int rt);
void gen_mips_mul(FILE *fp, int rd, int rs, int rt);
void gen_mips_div(FILE *fp, int rt, int rs);
void gen_mips_mflo(FILE *fp, int rs);
void gen_mips_lw(FILE *fp, int rt, int rs, int offset);
void gen_mips_sw(FILE *fp, int rt, int rs, int offset);
void gen_mips_li(FILE *fp, int rd, int i);
void gen_mips_move(FILE *fp, int rd, int rs);

/* utils */
void gen_mips_add_sp(FILE *fp, int offset);
void gen_mips_load_var(FILE *fp, int reg, operand_t *var);
void gen_mips_store_var(FILE *fp, int reg, operand_t *var);
void gen_mips_prologue(FILE *fp);
void gen_mips_epilogue(FILE *fp);
int gen_mips_get_reg(FILE *fp, operand_t *var, int is_lval);

/* ------------------------------------ *
 *          generate mips asm           *
 * ------------------------------------ */

void gen_mips(FILE *fp)
{
    gen_mips_framework(fp);

    iclistnode_t *cur = get_intercodes()->front;
    while (cur) {
        cur = gen_mips_dispatch(fp, cur);
    }
}

void gen_mips_framework(FILE *fp)
{
    fprintf(fp,
            ".data\n"
            "_prompt: .asciiz \"Enter an integer:\"\n"
            "_ret: .asciiz \"\\n\"\n"
            ".globl main\n"
            ".text\n"
            "read:\n"
            "li $v0, 4\n"
            "la $a0, _prompt\n"
            "syscall\n"
            "li $v0, 5\n"
            "syscall\n"
            "jr $ra\n"
            "write:\n"
            "li $v0, 1\n"
            "syscall\n"
            "li $v0, 4\n"
            "la $a0, _ret\n"
            "syscall\n"
            "move $v0, $0\n"
            "jr $ra\n");
}

iclistnode_t *gen_mips_dispatch(FILE *fp, iclistnode_t *cur)
{
    switch (cur->ic->kind) {
    case IC_LABEL: return gen_mips_label(fp, cur);
    case IC_FUNCDEF: return gen_mips_funcdef(fp, cur);
    case IC_ASSIGN: return gen_mips_assign(fp, cur);
    case IC_ARITHBOP:  return gen_mips_arithbop(fp, cur);
    case IC_REF: break;
    case IC_DREF: break;
    case IC_DREFASSIGN: break;
    case IC_GOTO: return gen_mips_goto(fp, cur);
    case IC_CONDGOTO: break;
    case IC_RETURN: break;
    case IC_DEC: return gen_mips_dec(fp, cur);
    case IC_ARG: break;
    case IC_CALL: break;
    case IC_PARAM: return gen_mips_param(fp, cur);
    case IC_READ: break;
    case IC_WRITE: break;
    default:
        assert(0); break;
    }
    return cur->next;
}

iclistnode_t *gen_mips_label(FILE *fp, iclistnode_t *cur)
{
    ic_label_t *ic = (ic_label_t *)cur->ic;
    gen_mips_tag(fp, "L%d", ic->labelid);
    return cur->next;
}

iclistnode_t *gen_mips_funcdef(FILE *fp, iclistnode_t *cur)
{
    ic_funcdef_t *ic = (ic_funcdef_t *)cur->ic;
    gen_mips_tag(fp, ic->fname);
    gen_mips_prologue(fp);

    /* Remember to clear the information used by the last function. */
    varinfolist_clear();
    reginfo_table_clear();

    /* Collect variable information in this function and allocate memory for them. */
    int offset = collect_varinfo(cur);
    gen_mips_add_sp(fp, offset);

    print_varinfolist();  // FIXME: delete this line

    return cur->next;
}

iclistnode_t *gen_mips_assign(FILE *fp, iclistnode_t *cur)
{
    ic_assign_t *ic = (ic_assign_t *)cur->ic;
    assert(!is_const_operand(&ic->lhs));

    if (is_const_operand(&ic->rhs)) {
        int reg = gen_mips_get_reg(fp, &ic->lhs, 1);
        gen_mips_li(fp, reg, ic->rhs.val);
    }
    else {
        int rs = gen_mips_get_reg(fp, &ic->rhs, 0);
        reginfo_table_lock(rs);
        int rt = gen_mips_get_reg(fp, &ic->lhs, 1);
        reginfo_table_unlock(rs);
        gen_mips_move(fp, rt, rs);
    }

    // print_reginfo_table();
    return cur->next;
}

iclistnode_t *gen_mips_arithbop(FILE *fp, iclistnode_t *cur)
{
    ic_arithbop_t *ic = (ic_arithbop_t *)cur->ic;

    if (ic->op == ICOP_ADD)
        gen_mips_arithbop_add(fp, &ic->target, &ic->lhs, &ic->rhs);
    else if (ic->op == ICOP_SUB)
        gen_mips_arithbop_sub(fp, &ic->target, &ic->lhs, &ic->rhs);
    else if (ic->op == ICOP_MUL)
        gen_mips_arithbop_mul(fp, &ic->target, &ic->lhs, &ic->rhs);
    else if (ic->op == ICOP_DIV)
        gen_mips_arithbop_div(fp, &ic->target, &ic->lhs, &ic->rhs);
    else
        assert(0); /* Should not reach here */

    return cur->next;
}

void gen_mips_arithbop_add(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs)
{
    if (is_const_operand(lhs) && is_const_operand(rhs)) {
        int reg = gen_mips_get_reg(fp, target, 1);
        gen_mips_li(fp, reg, lhs->val + rhs->val);
    }
    else if (is_const_operand(lhs) || is_const_operand(rhs)) {
        if (is_const_operand(lhs))
        {
            operand_t *temp = rhs;
            rhs = lhs;
            lhs = temp;
        }
        assert(!is_const_operand(lhs));
        assert(is_const_operand(rhs));
        int rs = gen_mips_get_reg(fp, lhs, 0);
        reginfo_table_lock(rs);
        int rt = gen_mips_get_reg(fp, target, 1);
        reginfo_table_unlock(rs);
        gen_mips_addi(fp, rt, rs, rhs->val);
    }
    else {
        int rs = gen_mips_get_reg(fp, lhs, 0);
        reginfo_table_lock(rs);
        int rt = gen_mips_get_reg(fp, rhs, 0);
        reginfo_table_lock(rt);
        int rd = gen_mips_get_reg(fp, target, 1);
        reginfo_table_unlock(rs);
        reginfo_table_unlock(rt);
        gen_mips_add(fp, rd, rs, rt);
    }
}

void gen_mips_arithbop_sub(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs)
{
    if (is_const_operand(lhs) && is_const_operand(rhs)) {
        int reg = gen_mips_get_reg(fp, target, 1);
        gen_mips_li(fp, reg, lhs->val - rhs->val);
    }
    else if (is_const_operand(rhs)) {
        int rs = gen_mips_get_reg(fp, lhs, 0);
        reginfo_table_lock(rs);
        int rt = gen_mips_get_reg(fp, target, 1);
        reginfo_table_unlock(rs);
        gen_mips_addi(fp, rt, rs, -rhs->val);
    }
    else {
        int rs = gen_mips_get_reg(fp, lhs, 0);
        reginfo_table_lock(rs);
        int rt = gen_mips_get_reg(fp, rhs, 0);
        reginfo_table_lock(rt);
        int rd = gen_mips_get_reg(fp, target, 1);
        reginfo_table_unlock(rs);
        reginfo_table_unlock(rt);
        gen_mips_sub(fp, rd, rs, rt);
    }
}

void gen_mips_arithbop_mul(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs)
{
    if (is_const_operand(lhs) && is_const_operand(rhs)) {
        int reg = gen_mips_get_reg(fp, target, 1);
        gen_mips_li(fp, reg, lhs->val * rhs->val);
    }
    else {
        int rs = gen_mips_get_reg(fp, lhs, 0);
        reginfo_table_lock(rs);
        int rt = gen_mips_get_reg(fp, rhs, 0);
        reginfo_table_lock(rt);
        int rd = gen_mips_get_reg(fp, target, 1);
        reginfo_table_unlock(rs);
        reginfo_table_unlock(rt);
        gen_mips_mul(fp, rd, rs, rt);
    }
}

void gen_mips_arithbop_div(FILE *fp, operand_t *target,
                           operand_t *lhs, operand_t *rhs)
{
    if (is_const_operand(lhs) && is_const_operand(rhs)) {
        int reg = gen_mips_get_reg(fp, target, 1);
        gen_mips_li(fp, reg, lhs->val / rhs->val);
    }
    else {
        int rs = gen_mips_get_reg(fp, lhs, 0);
        reginfo_table_lock(rs);
        int rt = gen_mips_get_reg(fp, rhs, 0);
        reginfo_table_unlock(rs);
        gen_mips_div(fp, rs, rt);
        int rd = gen_mips_get_reg(fp, target, 1);
        gen_mips_mflo(fp, rd);
    }
}

iclistnode_t *gen_mips_goto(FILE *fp, iclistnode_t *cur)
{
    ic_goto_t *ic = (ic_goto_t *)cur->ic;
    gen_mips_jmp_tag(fp, "j", "L%d", ic->labelid);
    return cur->next;
}

iclistnode_t *gen_mips_dec(FILE *fp, iclistnode_t *cur)
{
    return cur->next; /* Do nothing. */
}

iclistnode_t *gen_mips_param(FILE *fp, iclistnode_t *cur)
{
    return cur->next; /* Do nothing. */
}

/* ------------------------------------ *
 *    generate basic mips instruction   *
 * ------------------------------------ */

void gen_mips_tag(FILE *fp, const char *tag, ...)
{
    va_list ap;
    va_start(ap, tag);
    vfprintf(fp, tag, ap);
    va_end(ap);
    fprintf(fp, ":\n");
}

void gen_mips_jmp_tag(FILE *fp, const char *jmpcmd, const char *tag, ...)
{
    fprintf(fp, "%s ", jmpcmd);
    va_list ap;
    va_start(ap, tag);
    vfprintf(fp, tag, ap);
    va_end(ap);
    fprintf(fp, "\n");
}

void gen_mips_addi(FILE *fp, int rt, int rs, int i)
{
    fprintf(fp, "addi $%s, $%s, %d\n", get_regalias(rt),
            get_regalias(rs), i);
}

void gen_mips_add(FILE *fp, int rd, int rs, int rt)
{
    fprintf(fp, "add $%s, $%s, $%s\n", get_regalias(rd),
            get_regalias(rs), get_regalias(rt));
}

void gen_mips_sub(FILE *fp, int rd, int rs, int rt)
{
    fprintf(fp, "sub $%s, $%s, $%s\n", get_regalias(rd),
            get_regalias(rs), get_regalias(rt));
}

void gen_mips_mul(FILE *fp, int rd, int rs, int rt)
{
    fprintf(fp, "mul $%s, $%s, $%s\n", get_regalias(rd),
            get_regalias(rs), get_regalias(rt));
}

void gen_mips_div(FILE *fp, int rs, int rt)
{
    fprintf(fp, "div $%s, $%s\n", get_regalias(rs), get_regalias(rt));
}

void gen_mips_mflo(FILE *fp, int rs)
{
    fprintf(fp, "mflo $%s\n", get_regalias(rs));
}

void gen_mips_lw(FILE *fp, int rt, int rs, int offset)
{
    fprintf(fp, "lw $%s, %d($%s)\n", get_regalias(rt),
            offset, get_regalias(rs));
}

void gen_mips_sw(FILE *fp, int rt, int rs, int offset)
{
    fprintf(fp, "sw $%s, %d($%s)\n", get_regalias(rt),
            offset, get_regalias(rs));
}

void gen_mips_li(FILE *fp, int rd, int i)
{
    fprintf(fp, "li $%s, %d\n", get_regalias(rd), i);
}

void gen_mips_move(FILE *fp, int rd, int rs)
{
    fprintf(fp, "move $%s, $%s\n", get_regalias(rd), get_regalias(rs));
}

/* ------------------------------------ *
 *                utils                 *
 * ------------------------------------ */

void gen_mips_add_sp(FILE *fp, int offset)
{
    gen_mips_addi(fp, R_SP, R_SP, offset);
}

void gen_mips_prologue(FILE *fp)
{
    gen_mips_add_sp(fp, -4);
    gen_mips_sw(fp, R_FP, R_SP, 0);
    gen_mips_move(fp, R_FP, R_SP);
}

void gen_mips_epilogue(FILE *fp)
{
    gen_mips_move(fp, R_SP, R_FP);
    gen_mips_lw(fp, R_FP, R_SP, 0);
    gen_mips_add_sp(fp, 4);
}

void gen_mips_load_var(FILE *fp, int reg, operand_t *var)
{
    if (is_const_operand(var)) {
        gen_mips_li(fp, reg, var->val);
    }
    else {
        varinfo_t *varinfo = varinfolist_find(var);
        assert(varinfo);
        assert(varinfo->reg == R_FP || varinfo->reg == R_SP);
        gen_mips_lw(fp, reg, varinfo->reg, varinfo->offset);
    }
}

void gen_mips_store_var(FILE *fp, int reg, operand_t *var)
{
    if (is_const_operand(var))
        return;

    varinfo_t *varinfo = varinfolist_find(var);
    assert(varinfo);
    assert(varinfo->reg == R_FP || varinfo->reg == R_SP);
    gen_mips_sw(fp, reg, varinfo->reg, varinfo->offset);
}

int gen_mips_get_reg(FILE *fp, operand_t *var, int is_lval)
{
    assert(var);
    assert(var->kind != OPERAND_NONE);

    int reg;

    if ((reg = reginfo_table_find_var(var)) != R_NONE)
        return reg;

    if ((reg = reginfo_table_find_empty()) != R_NONE) {
        if (!is_lval)
            gen_mips_load_var(fp, reg, var);
        reginfo_table_alloc_reg(reg, var);
        return reg;
    }

    if ((reg = reginfo_table_find_expellable()) != R_NONE) {
        operand_t oldvar = reginfo_table_free_reg(reg);
        gen_mips_store_var(fp, reg, &oldvar);
        if (!is_lval)
            gen_mips_load_var(fp, reg, var);
        reginfo_table_alloc_reg(reg, var);
        return reg;
    }

    fprintf(stderr, "Fail to allocate any registers. "
            "What an algorithm are you writing?\n");
    exit(0);
    return R_NONE;
}
