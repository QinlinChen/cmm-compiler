#include "mips.h"
#include "mips-data.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------ *
 *          generate mips asm           *
 * ------------------------------------ */

void gen_mips_framework(FILE *fp);
iclistnode_t *gen_mips_dispatch(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_label(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_funcdef(FILE *fp, iclistnode_t *cur);
iclistnode_t *gen_mips_gotof(FILE *fp, iclistnode_t *cur);

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
    case IC_LABEL:
        return gen_mips_label(fp, cur);
    case IC_FUNCDEF:
        return gen_mips_funcdef(fp, cur);
    case IC_ASSIGN:
        break;
    case IC_ARITHBOP:
        break;
    case IC_REF:
        break;
    case IC_DREF:
        break;
    case IC_DREFASSIGN:
        break;
    case IC_GOTO:
        return gen_mips_gotof(fp, cur);
    case IC_CONDGOTO:
        break;
    case IC_RETURN:
        break;
    case IC_DEC:
        break;
    case IC_ARG:
        break;
    case IC_CALL:
        break;
    case IC_PARAM:
        break;
    case IC_READ:
        break;
    case IC_WRITE:
        break;
    default:
        assert(0); break;
    }
    return cur->next;
}

iclistnode_t *gen_mips_label(FILE *fp, iclistnode_t *cur)
{
    ic_label_t *ic = (ic_label_t *)cur->ic;
    fprintf(fp, "L%d:\n", ic->labelid);
    return cur->next;
}

iclistnode_t *gen_mips_funcdef(FILE *fp, iclistnode_t *cur)
{
    reginfo_table_clear();
    varinfolist_clear();
    int size = collect_varinfo(cur);
    printf("total size: %d\n", size);

    // print_reginfo_table();
    print_varinfolist();
    
    ic_funcdef_t *ic = (ic_funcdef_t *)cur->ic;
    fprintf(fp, "%s:\n", ic->fname);
    return cur->next;
}

iclistnode_t *gen_mips_gotof(FILE *fp, iclistnode_t *cur)
{
    ic_goto_t *ic = (ic_goto_t *)cur->ic;
    fprintf(fp, "j L%d\n", ic->labelid);
    return cur->next;
}