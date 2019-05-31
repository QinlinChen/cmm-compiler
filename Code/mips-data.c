#include "mips-data.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------ *
 *            var info list             *
 * ------------------------------------ */

typedef struct vilistnode {
    varinfo_t *varinfo;
    struct vilistnode *prev;
    struct vilistnode *next;
} vilistnode_t;

static struct {
    int size;
    vilistnode_t *front;
    vilistnode_t *back;
} varinfolist;

varinfo_t *create_varinfo(operand_t *var, int reg, int offset)
{
    varinfo_t *new_varinfo = malloc(sizeof(varinfo_t));
    if (new_varinfo) {
        new_varinfo->var = *var;
        new_varinfo->reg = reg;
        new_varinfo->offset = offset;
    }
    return new_varinfo;
}

void print_varinfo(varinfo_t *vi)
{
    fprint_operand(stdout, &vi->var);
    printf(" reg: %s, offset %d", get_regalias(vi->reg), vi->offset);
}

vilistnode_t *create_vilistnode(varinfo_t *vi)
{
    vilistnode_t *newnode = malloc(sizeof(vilistnode_t));
    if (newnode) {
        newnode->varinfo = vi;
        newnode->next = newnode->prev = NULL;
    }
    return newnode;
}

varinfo_t *destroy_vilistnode(vilistnode_t *node)
{
    varinfo_t *ret = node->varinfo;
    free(node);
    return ret;
}

void init_varinfolist()
{
    varinfolist.size = 0;
    varinfolist.front = varinfolist.back = NULL;
}

void varinfolist_clear()
{
    vilistnode_t *cur = varinfolist.front;
    while (cur) {
        vilistnode_t *save = cur->next;
        free(cur);
        cur = save;
    }
    init_varinfolist();
}

void varinfolist_push_back(varinfo_t *vi)
{
    assert(vi);
    vilistnode_t *newnode = create_vilistnode(vi);
    assert(newnode);

    if (varinfolist.size == 0)
        varinfolist.front = newnode;
    else
        varinfolist.back->next = newnode;
    newnode->prev = varinfolist.back;
    varinfolist.back = newnode;
    varinfolist.size++;
}

varinfo_t *varinfolist_find(operand_t *var)
{
    for (vilistnode_t *cur = varinfolist.front; cur != NULL; cur = cur->next) {
        if (operand_is_equal(&cur->varinfo->var, var))
            return cur->varinfo;
    }
    return NULL;
}

void print_varinfolist()
{
    for (vilistnode_t *cur = varinfolist.front; cur != NULL; cur = cur->next) {
        print_varinfo(cur->varinfo);
        printf("\n");
    }
}

int varinfolist_try_add_var(operand_t *var, int size, int offset);
void collect_varinfo_param(ic_param_t *ic, int n_param);
int collect_varinfo_dec(ic_dec_t *ic, int offset);
int collect_varinfo_assign(ic_assign_t *ic, int offset);
int collect_varinfo_arithbop(ic_arithbop_t *ic, int offset);
int collect_varinfo_ref(ic_ref_t *ic, int offset);
int collect_varinfo_dref(ic_dref_t *ic, int offset);
int collect_varinfo_drefassign(ic_drefassign_t *ic, int offset);
int collect_varinfo_condgoto(ic_condgoto_t *ic, int offset);
int collect_varinfo_return(ic_return_t *ic, int offset);
int collect_varinfo_arg(ic_arg_t *ic, int offset);
int collect_varinfo_call(ic_call_t *ic, int offset);
int collect_varinfo_read(ic_read_t *ic, int offset);
int collect_varinfo_write(ic_write_t *ic, int offset);

int collect_varinfo(iclistnode_t *funcdefnode)
{
    assert(funcdefnode->ic->kind == IC_FUNCDEF);
    int offset = 0;
    int n_param = 1;

    for (iclistnode_t *cur = funcdefnode->next; cur != NULL; cur = cur->next) {
        intercode_t *ic = cur->ic;
        if (ic->kind == IC_FUNCDEF)
            break;
        switch (ic->kind) {
        case IC_PARAM:
            collect_varinfo_param((ic_param_t *)ic, n_param++); break;
        case IC_DEC:
            offset = collect_varinfo_dec((ic_dec_t *)ic, offset); break;
        case IC_ASSIGN:
            offset = collect_varinfo_assign((ic_assign_t *)ic, offset); break;
        case IC_ARITHBOP:
            offset = collect_varinfo_arithbop((ic_arithbop_t *)ic, offset); break;
        case IC_REF:
            offset = collect_varinfo_ref((ic_ref_t *)ic, offset); break;
        case IC_DREF:
            offset = collect_varinfo_dref((ic_dref_t *)ic, offset); break;
        case IC_DREFASSIGN:
            offset = collect_varinfo_drefassign((ic_drefassign_t *)ic, offset); break;
        case IC_CONDGOTO:
            offset = collect_varinfo_condgoto((ic_condgoto_t *)ic, offset); break;
        case IC_RETURN:
            offset = collect_varinfo_return((ic_return_t *)ic, offset); break;
        case IC_ARG:
            offset = collect_varinfo_arg((ic_arg_t *)ic, offset); break;
        case IC_CALL:
            offset = collect_varinfo_call((ic_call_t *)ic, offset); break;
        case IC_READ:
            offset = collect_varinfo_read((ic_read_t *)ic, offset); break;
        case IC_WRITE:
            offset = collect_varinfo_write((ic_write_t *)ic, offset); break;
        case IC_FUNCDEF:
            assert(0); break;
        default:
            break;
        }
    }

    return offset; /* the total offset that should be added to $SP. */
}

int varinfolist_try_add_var(operand_t *var, int size, int offset)
{
    if (is_const_operand(var))
        return offset;

    varinfo_t *varinfo = varinfolist_find(var);
    if (varinfo)
        return offset;
    
    offset -= size;
    varinfolist_push_back(create_varinfo(var, R_FP, offset));
    return offset;
}

void collect_varinfo_param(ic_param_t *ic, int n_param)
{
    varinfo_t *varinfo;

    if (n_param <= 4)
        varinfo = create_varinfo(&ic->var, R_A0 + n_param - 1, 0);
    else
        varinfo = create_varinfo(&ic->var, R_FP, 8 + 4 * (n_param - 5));
    varinfolist_push_back(varinfo);
}

int collect_varinfo_dec(ic_dec_t *ic, int offset)
{
    return varinfolist_try_add_var(&ic->var, ic->size, offset);
}

int collect_varinfo_assign(ic_assign_t *ic, int offset)
{
    offset = varinfolist_try_add_var(&ic->lhs, 4, offset);
    offset = varinfolist_try_add_var(&ic->rhs, 4, offset);
    return offset;
}

int collect_varinfo_arithbop(ic_arithbop_t *ic, int offset)
{
    offset = varinfolist_try_add_var(&ic->lhs, 4, offset);
    offset = varinfolist_try_add_var(&ic->rhs, 4, offset);
    offset = varinfolist_try_add_var(&ic->target, 4, offset);
    return offset;
}

int collect_varinfo_ref(ic_ref_t *ic, int offset)
{
    offset = varinfolist_try_add_var(&ic->lhs, 4, offset);
    offset = varinfolist_try_add_var(&ic->rhs, 4, offset);
    return offset;
}

int collect_varinfo_dref(ic_dref_t *ic, int offset)
{
    offset = varinfolist_try_add_var(&ic->lhs, 4, offset);
    offset = varinfolist_try_add_var(&ic->rhs, 4, offset);
    return offset;
}

int collect_varinfo_drefassign(ic_drefassign_t *ic, int offset)
{
    offset = varinfolist_try_add_var(&ic->lhs, 4, offset);
    offset = varinfolist_try_add_var(&ic->rhs, 4, offset);
    return offset;
}

int collect_varinfo_condgoto(ic_condgoto_t *ic, int offset)
{
    offset = varinfolist_try_add_var(&ic->lhs, 4, offset);
    offset = varinfolist_try_add_var(&ic->rhs, 4, offset);
    return offset;
}

int collect_varinfo_return(ic_return_t *ic, int offset)
{
    return varinfolist_try_add_var(&ic->ret, 4, offset);
}

int collect_varinfo_arg(ic_arg_t *ic, int offset)
{
    return varinfolist_try_add_var(&ic->arg, 4, offset);
}

int collect_varinfo_call(ic_call_t *ic, int offset)
{
    return varinfolist_try_add_var(&ic->ret, 4, offset);
}

int collect_varinfo_read(ic_read_t *ic, int offset)
{
    return varinfolist_try_add_var(&ic->var, 4, offset);
}

int collect_varinfo_write(ic_write_t *ic, int offset)
{
    return varinfolist_try_add_var(&ic->var, 4, offset);
}

/* ------------------------------------ *
 *            reg info table            *
 * ------------------------------------ */

static reginfo_t reginfo_table[REG_SIZE];

const char *get_regalias(int reg)
{
    static const char *regalias[REG_SIZE] = {
        "zero", "at",
        "v0", "v1", "a0", "a1", "a2", "a3",
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8", "t9", "k0", "k1",
        "gp", "sp", "fp", "ra"
    };

    return regalias[reg];
}

void init_reginfo_table()
{
    memset(&reginfo_table, 0, sizeof(reginfo_table));
    for (int i = R_T0; i <= R_T9; ++i) {
        reginfo_table[i].is_empty = 1;
    }
}

void reginfo_table_clear()
{
    init_reginfo_table();
}

void print_reginfo_table()
{
    for (int i = R_T0; i <= R_T9; ++i) {
        printf("%s: ", get_regalias(i));
        if (!reginfo_table[i].is_empty)
            fprint_operand(stdout, &reginfo_table[i].var_loaded);
        printf("\n");
    }
}
