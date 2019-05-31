#include "mips.h"
#include "mips-data.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------ *
 *          generate mips asm           *
 * ------------------------------------ */

void gen_mips_framework(FILE *fp);
iclistnode_t *gen_mips_dispatch(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_label(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_funcdef(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_assign(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_goto(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_dec(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_param(FILE *fp, iclistnode_t *cur);

/* utils */
void gen_mips_tag(FILE *fp, const char *tag, ...);
void gen_mips_jmp_tag(FILE *fp, const char *jmpcmd, const char *tag, ...);
void gen_mips_add_sp(FILE *fp, int offset);

void gen_mips(FILE *fp)
{
    // gen_mips_framework(fp);

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
    case IC_ARITHBOP: break;
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

    /* Remember to clear the information used by the last function. */
    varinfolist_clear();
    reginfo_table_clear();

    /* Collect variable information in this function and allocate memory for them. */
    int offset = collect_varinfo(cur);
    gen_mips_add_sp(fp, offset);

    // print_reginfo_table();
    print_varinfolist();
    
    return cur->next;
}

iclistnode_t *gen_mips_assign(FILE *fp, iclistnode_t *cur)
{

    return cur->next;
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

/* utils */

void gen_mips_tag(FILE *fp, const char *tag, ...)
{
    va_list ap;
    va_start(ap, tag);
    vfprintf(fp, tag, ap);
    va_end(ap);
    fprintf(fp, ":\n");
}

void gen_mips_add_sp(FILE *fp, int offset)
{
    fprintf(fp, "addi $sp, $sp, %d\n", offset);
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