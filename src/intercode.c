#include "intercode.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------ *
 *              operator                *
 * ------------------------------------ */

static int free_varid = 1;
static int free_labelid = 1;

void init_varid()
{
    free_varid = 1;
}

void init_labelid()
{
    free_labelid = 1;
}

int alloc_varid()
{
    return free_varid++;
}

int alloc_labelid()
{
    return free_labelid++;
}

void init_var_operand(operand_t *op, int varid)
{
    assert(op);
    op->kind = OPERAND_VAR;
    op->varid = varid;
    op->is_temp = 0;
}

void init_addr_operand(operand_t *op, int varid)
{
    assert(op);
    op->kind = OPERAND_ADDR;
    op->varid = varid;
    op->is_temp = 0;
}

void init_const_operand(operand_t *op, int val)
{
    assert(op);
    op->kind = OPERAND_CONST;
    op->val = val;
    op->is_temp = 0;
}

void init_temp_var(operand_t *op)
{
    assert(op);
    op->kind = OPERAND_VAR;
    op->varid = alloc_varid();
    op->is_temp = 1;
}

void init_temp_addr(operand_t *op)
{
    assert(op);
    op->kind = OPERAND_ADDR;
    op->varid = alloc_varid();
    op->is_temp = 1;
}

int is_const_operand(operand_t *op)
{
    return op->kind == OPERAND_CONST;
}

int operand_is_equal(operand_t *lhs, operand_t *rhs)
{
    if (lhs->kind == OPERAND_VAR || lhs->kind == OPERAND_ADDR) {
        if (rhs->kind == OPERAND_VAR || rhs->kind == OPERAND_ADDR)
            return lhs->varid == rhs->varid;
        return 0;
    }
    if (lhs->kind == OPERAND_CONST) {
        if (rhs->kind == OPERAND_CONST)
            return lhs->val == rhs->val;
        return 0;
    }
    return 0;
}

void fprint_operand(FILE *fp, operand_t *op)
{
    switch (op->kind) {
    case OPERAND_VAR: /* fall through */
    case OPERAND_ADDR:
        fprintf(fp, "%c%d", (op->is_temp ? 't' : 'v'), op->varid); break;
    case OPERAND_CONST:
        fprintf(fp, "#%d", op->val); break;
    default:
        assert(0); break;
    }
}

/* ------------------------------------ *
 *              intercode               *
 * ------------------------------------ */

const char *icop_to_str(int icop)
{
    switch (icop) {
    case ICOP_ADD: return "+";
    case ICOP_SUB: return "-";
    case ICOP_MUL: return "*";
    case ICOP_DIV: return "/";
    case ICOP_EQ: return "==";
    case ICOP_NEQ: return "!=";
    case ICOP_L: return "<";
    case ICOP_LE: return "<=";
    case ICOP_G: return ">";
    case ICOP_GE: return ">=";
    default: assert(0); break;
    }
    return NULL;
}

int str_to_icop(const char *str)
{
    assert(str);
    if (!strcmp(str, "+"))
        return ICOP_ADD;
    if (!strcmp(str, "-"))
        return ICOP_SUB;
    if (!strcmp(str, "*"))
        return ICOP_MUL;
    if (!strcmp(str, "/"))
        return ICOP_DIV;
    if (!strcmp(str, "=="))
        return ICOP_EQ;
    if (!strcmp(str, "!="))
        return ICOP_NEQ;
    if (!strcmp(str, "<"))
        return ICOP_L;
    if (!strcmp(str, "<="))
        return ICOP_LE;
    if (!strcmp(str, ">"))
        return ICOP_G;
    if (!strcmp(str, ">="))
        return ICOP_GE;
    assert(0);
    return 0;
}

int complement_rel_icop(int icop)
{
    switch (icop) {
    case ICOP_EQ:  return ICOP_NEQ;
    case ICOP_NEQ: return ICOP_EQ;
    case ICOP_L:   return ICOP_GE;
    case ICOP_LE:  return ICOP_G;
    case ICOP_G:   return ICOP_LE;
    case ICOP_GE:  return ICOP_L;
    default: assert(0); break;
    }
    return ICOP_EQ;
}

intercode_t *create_ic_label(int labelid)
{
    ic_label_t *ic = malloc(sizeof(ic_label_t));
    assert(ic);
    ic->kind = IC_LABEL;
    ic->labelid = labelid;
    return (intercode_t *)ic;
}

intercode_t *create_ic_funcdef(const char *fname)
{
    assert(fname);
    ic_funcdef_t *ic = malloc(sizeof(ic_funcdef_t));
    assert(ic);
    ic->kind = IC_FUNCDEF;
    ic->fname = fname;
    return (intercode_t *)ic;
}

intercode_t *create_ic_assign(operand_t *lhs, operand_t *rhs)
{
    assert(lhs);
    assert(rhs);
    ic_assign_t *ic = malloc(sizeof(ic_assign_t));
    assert(ic);
    ic->kind = IC_ASSIGN;
    ic->lhs = *lhs;
    ic->rhs = *rhs;
    return (intercode_t *)ic;
}

intercode_t *create_ic_arithbop(int op, operand_t *target,
                                operand_t *lhs, operand_t *rhs)
{
    assert(target);
    assert(lhs);
    assert(rhs);
    ic_arithbop_t *ic = malloc(sizeof(ic_arithbop_t));
    assert(ic);
    ic->kind = IC_ARITHBOP;
    ic->op = op;
    ic->target = *target;
    ic->lhs = *lhs;
    ic->rhs = *rhs;
    return (intercode_t *)ic;
}

intercode_t *create_ic_ref(operand_t *lhs, operand_t *rhs)
{
    assert(lhs);
    assert(rhs);
    ic_ref_t *ic = malloc(sizeof(ic_ref_t));
    assert(ic);
    ic->kind = IC_REF;
    ic->lhs = *lhs;
    ic->rhs = *rhs;
    return (intercode_t *)ic;
}

intercode_t *create_ic_dref(operand_t *lhs, operand_t *rhs)
{
    assert(lhs);
    assert(rhs);
    ic_dref_t *ic = malloc(sizeof(ic_dref_t));
    assert(ic);
    ic->kind = IC_DREF;
    ic->lhs = *lhs;
    ic->rhs = *rhs;
    return (intercode_t *)ic;
}

intercode_t *create_ic_drefassign(operand_t *lhs, operand_t *rhs)
{
    assert(lhs);
    assert(rhs);
    ic_drefassign_t *ic = malloc(sizeof(ic_drefassign_t));
    assert(ic);
    ic->kind = IC_DREFASSIGN;
    ic->lhs = *lhs;
    ic->rhs = *rhs;
    return (intercode_t *)ic;
}

intercode_t *create_ic_goto(int labelid)
{
    ic_goto_t *ic = malloc(sizeof(ic_goto_t));
    assert(ic);
    ic->kind = IC_GOTO;
    ic->labelid = labelid;
    return (intercode_t *)ic;
}

intercode_t *create_ic_condgoto(int relop, operand_t *lhs, operand_t *rhs,
                                int labelid)
{
    assert(lhs);
    assert(rhs);
    ic_condgoto_t *ic = malloc(sizeof(ic_condgoto_t));
    assert(ic);
    ic->kind = IC_CONDGOTO;
    ic->relop = relop;
    ic->lhs = *lhs;
    ic->rhs = *rhs;
    ic->labelid = labelid;
    return (intercode_t *)ic;
}

intercode_t *create_ic_return(operand_t *ret)
{
    assert(ret);
    ic_return_t *ic = malloc(sizeof(ic_return_t));
    assert(ic);
    ic->kind = IC_RETURN;
    ic->ret = *ret;
    return (intercode_t *)ic;
}

intercode_t *create_ic_dec(operand_t *var, int size)
{
    ic_dec_t *ic = malloc(sizeof(ic_dec_t));
    assert(ic);
    ic->kind = IC_DEC;
    ic->var = *var;
    ic->size = size;
    return (intercode_t *)ic;
}

intercode_t *create_ic_arg(operand_t *arg)
{
    assert(arg);
    ic_arg_t *ic = malloc(sizeof(ic_arg_t));
    assert(ic);
    ic->kind = IC_ARG;
    ic->arg = *arg;
    return (intercode_t *)ic;
}

intercode_t *create_ic_call(const char *fname, operand_t *ret)
{
    assert(fname);
    ic_call_t *ic = malloc(sizeof(ic_call_t));
    assert(ic);
    ic->kind = IC_CALL;
    ic->fname = fname;
    ic->ret = *ret;
    return (intercode_t *)ic;
}

intercode_t *create_ic_param(operand_t *var)
{
    ic_param_t *ic = malloc(sizeof(ic_param_t));
    assert(ic);
    ic->kind = IC_PARAM;
    ic->var = *var;
    return (intercode_t *)ic;
}

intercode_t *create_ic_read(operand_t *var)
{
    ic_read_t *ic = malloc(sizeof(ic_read_t));
    assert(ic);
    ic->kind = IC_READ;
    ic->var = *var;
    return (intercode_t *)ic;
}

intercode_t *create_ic_write(operand_t *var)
{
    ic_write_t *ic = malloc(sizeof(ic_write_t));
    assert(ic);
    ic->kind = IC_WRITE;
    ic->var = *var;
    return (intercode_t *)ic;
}

void fprint_ic_label(FILE *fp, ic_label_t *ic)
{
    fprintf(fp, "LABEL L%d :", ic->labelid);
}

void fprint_ic_funcdef(FILE *fp, ic_funcdef_t *ic)
{
    fprintf(fp, "FUNCTION %s :", ic->fname);
}

void fprint_ic_assign(FILE *fp, ic_assign_t *ic)
{
    fprint_operand(fp, &ic->lhs);
    fprintf(fp, " := ");
    fprint_operand(fp, &ic->rhs);
}

void fprint_ic_arithbop(FILE *fp, ic_arithbop_t *ic)
{
    fprint_operand(fp, &ic->target);
    fprintf(fp, " := ");
    fprint_operand(fp, &ic->lhs);
    fprintf(fp, " %s ", icop_to_str(ic->op));
    fprint_operand(fp, &ic->rhs);
}

void fprint_ic_ref(FILE *fp, ic_ref_t *ic)
{
    fprint_operand(fp, &ic->lhs);
    fprintf(fp, " := &");
    fprint_operand(fp, &ic->rhs);
}

void fprint_ic_dref(FILE *fp, ic_dref_t *ic)
{
    fprint_operand(fp, &ic->lhs);
    fprintf(fp, " := *");
    fprint_operand(fp, &ic->rhs);
}

void fprint_ic_drefassign(FILE *fp, ic_drefassign_t *ic)
{
    fprintf(fp, "*");
    fprint_operand(fp, &ic->lhs);
    fprintf(fp, " := ");
    fprint_operand(fp, &ic->rhs);
}

void fprint_ic_goto(FILE *fp, ic_goto_t *ic)
{
    fprintf(fp, "GOTO L%d", ic->labelid);
}

void fprint_ic_condgoto(FILE *fp, ic_condgoto_t *ic)
{
    fprintf(fp, "IF ");
    fprint_operand(fp, &ic->lhs);
    fprintf(fp, " %s ", icop_to_str(ic->relop));
    fprint_operand(fp, &ic->rhs);
    fprintf(fp, " GOTO L%d", ic->labelid);
}

void fprint_ic_return(FILE *fp, ic_return_t *ic)
{
    fprintf(fp, "RETURN ");
    fprint_operand(fp, &ic->ret);
}

void fprint_ic_dec(FILE *fp, ic_dec_t *ic)
{
    fprintf(fp, "DEC ");
    fprint_operand(fp, &ic->var);
    fprintf(fp, " %d", ic->size);
}

void fprint_ic_arg(FILE *fp, ic_arg_t *ic)
{
    fprintf(fp, "ARG ");
    fprint_operand(fp, &ic->arg);
}

void fprint_ic_call(FILE *fp, ic_call_t *ic)
{
    fprint_operand(fp, &ic->ret);
    fprintf(fp, " := CALL %s", ic->fname);
}

void fprint_ic_param(FILE *fp, ic_param_t *ic)
{
    fprintf(fp, "PARAM ");
    fprint_operand(fp, &ic->var);
}

void fprint_ic_read(FILE *fp, ic_read_t *ic)
{
    fprintf(fp, "READ ");
    fprint_operand(fp, &ic->var);
}

void fprint_ic_write(FILE *fp, ic_write_t *ic)
{
    fprintf(fp, "WRITE ");
    fprint_operand(fp, &ic->var);
}

void fprint_intercode(FILE *fp, intercode_t *ic)
{
    assert(ic);
    assert(fp);
    switch (ic->kind) {
    case IC_LABEL:
        fprint_ic_label(fp, (ic_label_t *)ic); break;
    case IC_FUNCDEF:
        fprint_ic_funcdef(fp, (ic_funcdef_t *)ic); break;
    case IC_ASSIGN:
        fprint_ic_assign(fp, (ic_assign_t *)ic); break;
    case IC_ARITHBOP:
        fprint_ic_arithbop(fp, (ic_arithbop_t *)ic); break;
    case IC_REF:
        fprint_ic_ref(fp, (ic_ref_t *)ic); break;
    case IC_DREF:
        fprint_ic_dref(fp, (ic_dref_t *)ic); break;
    case IC_DREFASSIGN:
        fprint_ic_drefassign(fp, (ic_drefassign_t *)ic); break;
    case IC_GOTO:
        fprint_ic_goto(fp, (ic_goto_t *)ic); break;
    case IC_CONDGOTO:
        fprint_ic_condgoto(fp, (ic_condgoto_t *)ic); break;
    case IC_RETURN:
        fprint_ic_return(fp, (ic_return_t *)ic); break;
    case IC_DEC:
        fprint_ic_dec(fp, (ic_dec_t *)ic); break;
    case IC_ARG:
        fprint_ic_arg(fp, (ic_arg_t *)ic); break;
    case IC_CALL:
        fprint_ic_call(fp, (ic_call_t *)ic); break;
    case IC_PARAM:
        fprint_ic_param(fp, (ic_param_t *)ic); break;
    case IC_READ:
        fprint_ic_read(fp, (ic_read_t *)ic); break;
    case IC_WRITE:
        fprint_ic_write(fp, (ic_write_t *)ic); break;
    default:
        assert(0); break;
    }
}

/* ------------------------------------ *
 *           intercodelist              *
 * ------------------------------------ */

iclistnode_t *create_iclistnode(intercode_t *ic)
{
    iclistnode_t *newnode = malloc(sizeof(iclistnode_t));
    if (newnode) {
        newnode->ic = ic;
        newnode->next = newnode->prev = NULL;
    }
    return newnode;
}

void init_iclist(iclist_t *iclist)
{
    assert(iclist);
    iclist->size = 0;
    iclist->front = iclist->back = NULL;
}

void iclist_push_back(iclist_t *iclist, intercode_t *ic)
{
    assert(iclist);
    assert(ic);
    iclistnode_t *newnode = create_iclistnode(ic);
    assert(newnode);

    if (iclist->size == 0)
        iclist->front = newnode;
    else
        iclist->back->next = newnode;
    newnode->prev = iclist->back;
    iclist->back = newnode;
    iclist->size++;
}

void fprint_iclist(FILE *fp, iclist_t *iclist)
{
    for (iclistnode_t *cur = iclist->front; cur != NULL; cur = cur->next) {
        fprint_intercode(fp, cur->ic);
        fprintf(fp, "\n");
    }
}
