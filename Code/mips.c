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
iclistnode_t *gen_mips_ref(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_dref(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_drefassign(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_goto(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_condgoto(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_return(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_args(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_call(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_dec(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_param(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_read(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_write(FILE *fp, iclistnode_t *cur);

/* generate basic mips instruction */
void gen_mips_tag(FILE *fp, const char *tag, ...);
void gen_mips_jmp_tag(FILE *fp, const char *jmpcmd, const char *tag, ...);
void gen_mips_jr(FILE *fp, int reg);
void gen_mips_b_tag(FILE *fp, const char *bcmd, int rs, int rt,
                    const char *tag, ...);
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
void gen_mips_push(FILE *fp, int reg);
void gen_mips_pop(FILE *fp, int reg);
void gen_mips_load_var(FILE *fp, int reg, operand_t *var);
void gen_mips_store_var(FILE *fp, int reg, operand_t *var);
int gen_mips_get_reg(FILE *fp, operand_t *var, int is_lval);

void gen_mips_prologue(FILE *fp);
void gen_mips_epilogue(FILE *fp);
void gen_mips_before_call(FILE *fp);
void gen_mips_after_call(FILE *fp);

void gen_mips_writeback(FILE *fp, int reg);
void gen_mips_writeback_args(FILE *fp);
void gen_mips_writeback_vars(FILE *fp);

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
    case IC_ARITHBOP: return gen_mips_arithbop(fp, cur);
    case IC_REF: return gen_mips_ref(fp, cur);
    case IC_DREF: return gen_mips_dref(fp, cur);
    case IC_DREFASSIGN: return gen_mips_drefassign(fp, cur);
    case IC_GOTO: return gen_mips_goto(fp, cur);
    case IC_CONDGOTO: return gen_mips_condgoto(fp, cur);
    case IC_RETURN: return gen_mips_return(fp, cur);
    case IC_DEC: return gen_mips_dec(fp, cur);
    case IC_ARG: return gen_mips_args(fp, cur);
    case IC_CALL: return gen_mips_call(fp, cur);
    case IC_PARAM: return gen_mips_param(fp, cur);
    case IC_READ: return gen_mips_read(fp, cur);
    case IC_WRITE: return gen_mips_write(fp, cur);
    default: assert(0); break;  /* Should not reach here */
    }
    return cur->next;
}

iclistnode_t *gen_mips_label(FILE *fp, iclistnode_t *cur)
{
    ic_label_t *ic = (ic_label_t *)cur->ic;

    gen_mips_writeback_vars(fp);    /* Write back at the end of the basic block. */
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

    print_varinfolist();
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

iclistnode_t *gen_mips_ref(FILE *fp, iclistnode_t *cur)
{
    ic_ref_t *ic = (ic_ref_t *)cur->ic;
    varinfo_t *varinfo = varinfolist_find(&ic->rhs);
    int reg = gen_mips_get_reg(fp, &ic->lhs, 1);
    gen_mips_addi(fp, reg, varinfo->reg, varinfo->offset);

    return cur->next;
}

iclistnode_t *gen_mips_dref(FILE *fp, iclistnode_t *cur)
{
    ic_dref_t *ic = (ic_dref_t *)cur->ic;
    int rs = gen_mips_get_reg(fp, &ic->rhs, 0);
    reginfo_table_lock(rs);
    int rt = gen_mips_get_reg(fp, &ic->lhs, 1);
    reginfo_table_unlock(rs);
    gen_mips_lw(fp, rt, rs, 0);

    return cur->next;
}

iclistnode_t *gen_mips_drefassign(FILE *fp, iclistnode_t *cur)
{
    ic_drefassign_t *ic = (ic_drefassign_t *)cur->ic;
    int rs = gen_mips_get_reg(fp, &ic->rhs, 0);
    reginfo_table_lock(rs);
    int rt = gen_mips_get_reg(fp, &ic->lhs, 0);
    reginfo_table_unlock(rs);
    gen_mips_sw(fp, rs, rt, 0);

    return cur->next;
}

iclistnode_t *gen_mips_goto(FILE *fp, iclistnode_t *cur)
{
    ic_goto_t *ic = (ic_goto_t *)cur->ic;

    gen_mips_writeback_vars(fp);    /* Write back at the end of the basic block. */
    gen_mips_jmp_tag(fp, "j", "L%d", ic->labelid);

    return cur->next;
}

iclistnode_t *gen_mips_condgoto(FILE *fp, iclistnode_t *cur)
{
    ic_condgoto_t *ic = (ic_condgoto_t *)cur->ic;
    int rs = gen_mips_get_reg(fp, &ic->lhs, 0);
    reginfo_table_lock(rs);
    int rt = gen_mips_get_reg(fp, &ic->rhs, 0);
    reginfo_table_unlock(rs);

    gen_mips_writeback_vars(fp);    /* Write back at the end of the basic block. */

    switch (ic->relop) {
    case ICOP_EQ:
        gen_mips_b_tag(fp, "beq", rs, rt, "L%d", ic->labelid); break;
    case ICOP_NEQ:
        gen_mips_b_tag(fp, "bne", rs, rt, "L%d", ic->labelid); break;
    case ICOP_G:
        gen_mips_b_tag(fp, "bgt", rs, rt, "L%d", ic->labelid); break;
    case ICOP_GE:
        gen_mips_b_tag(fp, "bge", rs, rt, "L%d", ic->labelid); break;
    case ICOP_L:
        gen_mips_b_tag(fp, "blt", rs, rt, "L%d", ic->labelid); break;
    case ICOP_LE:
        gen_mips_b_tag(fp, "ble", rs, rt, "L%d", ic->labelid); break;
    default:
        assert(0); break; /* Should not reach here. */
    } 
    return cur->next;
}

iclistnode_t *gen_mips_return(FILE *fp, iclistnode_t *cur)
{
    ic_return_t *ic = (ic_return_t *)cur->ic;
    operand_t *ret = &ic->ret;

    if (is_const_operand(ret)) {
        gen_mips_li(fp, R_V0, ret->val);
    }
    else {
        int reg = gen_mips_get_reg(fp, &ic->ret, 0);
        gen_mips_move(fp, R_V0, reg);
    }

    gen_mips_epilogue(fp);
    gen_mips_jr(fp, R_RA);
    
    return cur->next;
}

iclistnode_t *gen_mips_args(FILE *fp, iclistnode_t *cur)
{
    gen_mips_writeback_args(fp);

    iclistnode_t *end;
    for (end = cur->next; end != NULL; end = end->next)
        if (end->ic->kind != IC_ARG)
            break;
    
    int i = 1;
    for (iclistnode_t *iter = end->prev; iter != cur->prev; iter = iter->prev) {
        ic_arg_t *ic = (ic_arg_t *)iter->ic;

        if (i <= 4) {
            int rd = R_A0 + i - 1;
            int rs;
            assert(reginfo_table_is_empty(rd));
            if (is_const_operand(&ic->arg)) {
                gen_mips_li(fp, rd, ic->arg.val);
            }
            else {
                if ((rs = reginfo_table_find_var(&ic->arg)) != R_NONE)
                    gen_mips_move(fp, rd, rs);
                else
                    gen_mips_load_var(fp, rd, &ic->arg);
            }
        }
        else {
            int reg = gen_mips_get_reg(fp, &ic->arg, 0);
            gen_mips_push(fp, reg);
        }
        i++;
    }

    return end;
}

iclistnode_t *gen_mips_call(FILE *fp, iclistnode_t *cur)
{
    ic_call_t *ic = (ic_call_t *)cur->ic;

    gen_mips_writeback_vars(fp);

    gen_mips_before_call(fp);
    gen_mips_jmp_tag(fp, "jal", ic->fname);
    gen_mips_after_call(fp);

    int reg = gen_mips_get_reg(fp, &ic->ret, 1);
    gen_mips_move(fp, reg, R_V0);

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

iclistnode_t *gen_mips_read(FILE *fp, iclistnode_t *cur)
{
    ic_read_t *ic = (ic_read_t *)cur->ic;

    gen_mips_writeback(fp, R_A0);

    gen_mips_before_call(fp);
    gen_mips_jmp_tag(fp, "jal", "read");
    gen_mips_after_call(fp);

    int reg = gen_mips_get_reg(fp, &ic->var, 1);
    gen_mips_move(fp, reg, R_V0);

    return cur->next;
}

iclistnode_t *gen_mips_write(FILE *fp, iclistnode_t *cur)
{
    ic_write_t *ic = (ic_write_t *)cur->ic;
    
    gen_mips_writeback(fp, R_A0);
    int reg = gen_mips_get_reg(fp, &ic->var, 0);
    gen_mips_move(fp, R_A0, reg);

    gen_mips_before_call(fp);
    gen_mips_jmp_tag(fp, "jal", "write");
    gen_mips_after_call(fp);

    return cur->next;
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

void gen_mips_jr(FILE *fp, int reg)
{
    fprintf(fp, "jr $%s\n", get_regalias(reg));
}

void gen_mips_b_tag(FILE *fp, const char *bcmd, int rs, int rt,
                    const char *tag, ...)
{
    fprintf(fp, "%s $%s, $%s, ", bcmd, get_regalias(rs), get_regalias(rt));
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

void gen_mips_push(FILE *fp, int reg)
{
    gen_mips_add_sp(fp, -4);
    gen_mips_sw(fp, reg, R_SP, 0);
}

void gen_mips_pop(FILE *fp, int reg)
{
    gen_mips_lw(fp, reg, R_SP, 0);
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
    if (!varinfo) {
        fprint_operand(stdout, var);
    }
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
        gen_mips_writeback(fp, reg);
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

void gen_mips_prologue(FILE *fp)
{
    gen_mips_push(fp, R_FP);
    gen_mips_move(fp, R_FP, R_SP);
}

void gen_mips_epilogue(FILE *fp)
{
    gen_mips_move(fp, R_SP, R_FP);
    gen_mips_pop(fp, R_FP);
}

void gen_mips_before_call(FILE *fp)
{
    gen_mips_push(fp, R_RA);
}

void gen_mips_after_call(FILE *fp)
{
    gen_mips_pop(fp, R_RA);
}

void gen_mips_writeback(FILE *fp, int reg)
{
    if (!reginfo_table_is_empty(reg)) {
        operand_t var = reginfo_table_free_reg(reg);
        gen_mips_store_var(fp, reg, &var);
    }
    assert(reginfo_table_is_empty(reg));
}

void gen_mips_writeback_args(FILE *fp)
{
    for (int reg = R_A0; reg <= R_A3; ++reg)
        gen_mips_writeback(fp, reg);
}

void gen_mips_writeback_vars(FILE *fp)
{
    for (int reg = R_A0; reg <= R_T9; ++reg)
        gen_mips_writeback(fp, reg);
}
