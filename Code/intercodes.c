#include "intercodes.h"
#include "intercode.h"
#include "semantics.h"
#include "syntax.tab.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------ *
 *             intercodes               *
 * ------------------------------------ */

static iclist_t intercodes;

void init_intercodes()
{
    init_iclist(&intercodes);
}

void intercodes_push_back(intercode_t *ic)
{
    iclist_push_back(&intercodes, ic);
}

void fprint_intercodes(FILE *fp)
{
    fprint_iclist(fp, &intercodes);
}

/* ------------------------------------ *
 *           translate errors           *
 * ------------------------------------ */

void translate_error(int lineno, const char *msg, ...)
{
    printf("Line %d: ", lineno);
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    printf("\n");
}

/* ------------------------------------ *
 *              translate               *
 * ------------------------------------ */

void add_builtin_func();
void intercodes_translate_r(treenode_t *node);
void translate_ext_def(treenode_t *ext_def);
void translate_ext_dec_list(treenode_t *ext_dec_list, type_t *spec);

void translate_comp_st(treenode_t *comp_st);
void translate_def_list(treenode_t *def_list);
void translate_stmt_list(treenode_t *stmt_list);

/* Translate an expression and return the result in operand_t. */
operand_t translate_exp(treenode_t *exp);
operand_t translate_literal(treenode_t *literal);
operand_t translate_var(treenode_t *id);
operand_t translate_unary_minus(treenode_t *exp);
operand_t translate_arithbop(treenode_t *lexp, treenode_t *rexp, int icop);
operand_t translate_assign(treenode_t *lexp, treenode_t *rexp);

/* Translate an expression as a condition. */
void translate_cond(treenode_t *exp, int truelabel, int falselabel);
void translate_cond_not(treenode_t *exp, int truelabel, int falselabel);
void translate_cond_relop(treenode_t *lexp, treenode_t *rexp,
                          int truelabel, int falselabel, int icop);
void translate_cond_and(treenode_t *lexp, treenode_t *rexp,
                        int truelabel, int falselabel);
void translate_cond_or(treenode_t *lexp, treenode_t *rexp,
                       int truelabel, int falselabel);
void translate_cond_otherwise(treenode_t *exp, int truelabel, int falselabel);

void intercodes_translate(treenode_t *root)
{
    init_structdef_table();
    init_symbol_table();
    init_intercodes();
    add_builtin_func();

    intercodes_translate_r(root);

    fprint_intercodes(stdout);
}

void add_builtin_func()
{
    symbol_t readfunc, writefunc;
    typelist_t typelist;

    /* add 'int read()' */
    type_t *inttype = (type_t *)create_type_basic(TYPE_INT);
    type_t *readfunctype = (type_t *)create_type_func(inttype, NULL);
    init_symbol(&readfunc, readfunctype, "read", 0, 1);
    symbol_table_add(&readfunc);

    /* add 'int write(int)' */
    init_typelist(&typelist);
    typelist_push_back(&typelist, inttype);
    type_t *writefunctype = (type_t *)create_type_func(inttype, &typelist);
    init_symbol(&writefunc, writefunctype, "write", 0, 1);
    symbol_table_add(&writefunc);
}

void intercodes_translate_r(treenode_t *node)
{
    if (!node)
        return;

    if (!strcmp(node->name, "ExtDef")) {
        translate_ext_def(node);
        return;
    }

    for (treenode_t *child = node->child; child != NULL; child = child->next)
        intercodes_translate_r(child);
}

void translate_ext_def(treenode_t *ext_def)
{
    assert(ext_def);
    assert(!strcmp(ext_def->name, "ExtDef"));
    treenode_t *specifer = ext_def->child;
    assert(specifer);
    type_t *spec = analyse_specifier(specifer);
    if (!spec)
        return;

    treenode_t *child2 = specifer->next;
    assert(child2);
    if (!strcmp(child2->name, "ExtDecList")) {
        translate_ext_dec_list(child2, spec);
        return;
    }
    if (!strcmp(child2->name, "FunDec")) {
        symbol_t func;
        fieldlist_t paramlist;
        init_fieldlist(&paramlist);
        analyse_fun_dec(child2, spec, &func, &paramlist);
        assert(child2->next);
        int is_def = !strcmp(child2->next->name, "SEMI") ? 0 : 1;

        if (checked_symbol_table_add_func(&func, is_def) != 0)
            return;
        if (is_def) {
            symbol_table_pushenv();
            // TODO: gen func and params
            translate_comp_st(child2->next);
            symbol_table_popenv();
        }
        return;
    }
    assert(!strcmp(child2->name, "SEMI"));
}

void translate_ext_dec_list(treenode_t *ext_dec_list, type_t *spec)
{
    translate_error(ext_dec_list->lineno, "Assumption 4 is violated. "
                                          "Global variables are not allowed.");
}

void translate_comp_st(treenode_t *comp_st)
{
    assert(comp_st);
    assert(!strcmp(comp_st->name, "CompSt"));
    assert(comp_st->child);

    treenode_t *child2 = comp_st->child->next;
    assert(child2);

    if (!strcmp(child2->name, "DefList")) {
        translate_def_list(child2);
        treenode_t *child3 = child2->next;
        if (!strcmp(child3->name, "StmtList"))
            translate_stmt_list(child3);
        else
            assert(!strcmp(child3->name, "RC"));
    }
    else if (!strcmp(child2->name, "StmtList")) {
        translate_stmt_list(child2);
    }
    else {
        assert(!strcmp(child2->name, "RC"));
    }
}

void translate_def_list(treenode_t *def_list)
{
    // TODO: gen code for array and struct
    analyse_def_list(def_list, NULL, CONTEXT_VAR_DEF);
}

void translate_stmt_list(treenode_t *stmt_list)
{
    // FIXME:
    if (!stmt_list)
        return;

    if (!strcmp(stmt_list->name, "Exp")) {
        translate_cond(stmt_list, 1, 2);
        printf("\n");
        return;
    }

    for (treenode_t *child = stmt_list->child; child != NULL; child = child->next)
        translate_stmt_list(child);
}

operand_t translate_exp(treenode_t *exp)
{
    assert(exp);
    assert(!strcmp(exp->name, "Exp"));
    treenode_t *child = exp->child;
    assert(child);

    if (!strcmp(child->name, "INT") || !strcmp(child->name, "FLOAT"))
        return translate_literal(child);
    if (!strcmp(child->name, "ID")) {
        if (!child->next) {
            return translate_var(child);
        }
        // TODO: gen code for funccall

        // assert(!strcmp(child->next->name, "LP"));
        // treenode_t *child3 = child->next->next;
        // assert(child3);
        // if (!strcmp(child3->name, "Args"))
        //     return typecheck_func_call(child, child3, is_lval);
        // assert(!strcmp(child3->name, "RP"));
        // return typecheck_func_call(child, NULL, is_lval);
    }
    if (!strcmp(child->name, "LP"))
        return translate_exp(child->next);
    if (!strcmp(child->name, "MINUS"))
        return translate_unary_minus(child->next);
    if (!strcmp(child->name, "NOT")) {
        assert(0);  // TODO:
        // return typecheck_unary_op(child->next, OP_UNARY_BOOL, is_lval);
    }
    assert(!strcmp(child->name, "Exp"));
    treenode_t *child2 = child->next;
    assert(child2);
    treenode_t *child3 = child2->next;
    assert(child3);
    if (!strcmp(child2->name, "PLUS"))
        return translate_arithbop(child, child3, ICOP_ADD);
    if (!strcmp(child2->name, "MINUS"))
        return translate_arithbop(child, child3, ICOP_SUB);
    if (!strcmp(child2->name, "STAR"))
        return translate_arithbop(child, child3, ICOP_MUL);
    if (!strcmp(child2->name, "DIV"))
        return translate_arithbop(child, child3, ICOP_DIV);
    if (!strcmp(child2->name, "ASSIGNOP"))
        return translate_assign(child, child3);
    if (!strcmp(child2->name, "DOT"))
        assert(0);
        // return typecheck_struct_access(child, child2, child3, is_lval);
    if (!strcmp(child2->name, "LB"))
        assert(0);
        // return typecheck_array_access(child, child3, is_lval);

    // if (!strcmp(child2->name, "RELOP"))
    //     return typecheck_binary_op(child, child3, OP_REL, is_lval);
    // if (!strcmp(child2->name, "AND") || !strcmp(child2->name, "OR"))
    //     return typecheck_binary_op(child, child3, OP_BINARY_BOOL, is_lval);

    assert(0);  /* Should not reach here! */

}

operand_t translate_literal(treenode_t *literal)
{
    assert(literal);
    operand_t op;

    if (literal->token != INT) {
        translate_error(literal->lineno, "Assumption 1 is violated. "
                                         "Floats are not allowed.");
        init_const_operand(&op, 0);
        return op;
    }

    init_const_operand(&op, literal->ival);
    return op;
}

operand_t translate_var(treenode_t *id)
{
    assert(id);
    assert(id->token == ID);
    symbol_t *symbol;
    operand_t op;

    if (symbol_table_find_by_name(id->id, &symbol) != 0) {
        assert(0);
        init_const_operand(&op, 0);
        return op;
    }

    init_var_operand(&op, symbol->id);
    return op;
}

operand_t translate_unary_minus(treenode_t *exp)
{
    assert(exp);

    operand_t target, zero;
    operand_t subexp = translate_exp(exp);

    if (is_const_operand(&subexp)) {
        init_const_operand(&target, -subexp.val);
        return target;
    }
        
    init_temp_var(&target);
    init_const_operand(&zero, 0);
    intercodes_push_back(create_ic_arithbop(ICOP_SUB, &target, &zero, &subexp));
    return target;
}

operand_t translate_arithbop(treenode_t *lexp, treenode_t *rexp, int icop)
{
    assert(lexp);
    assert(rexp);
    
    operand_t lhs = translate_exp(lexp);
    operand_t rhs = translate_exp(rexp);
    operand_t target;

    if (is_const_operand(&lhs) && is_const_operand(&rhs)) {
        int target_val;
        switch (icop) {
        case ICOP_ADD: target_val = lhs.val + rhs.val; break;
        case ICOP_SUB: target_val = lhs.val - rhs.val; break;
        case ICOP_MUL: target_val = lhs.val * rhs.val; break;
        case ICOP_DIV: target_val = lhs.val / rhs.val; break;
        default: assert(0); break;
        }
        init_const_operand(&target, target_val);
        return target;
    }

    init_temp_var(&target);
    intercodes_push_back(create_ic_arithbop(icop, &target, &lhs, &rhs));
    return target;
}

operand_t translate_assign(treenode_t *lexp, treenode_t *rexp)
{
    assert(lexp);
    assert(rexp);

    operand_t lhs = translate_exp(lexp);
    operand_t rhs = translate_exp(rexp);

    assert(!is_const_operand(&lhs));
    intercodes_push_back(create_ic_assign(&lhs, &rhs));
    return lhs;
}

void translate_cond(treenode_t *exp, int truelabel, int falselabel)
{
    assert(exp);
    assert(!strcmp(exp->name, "Exp"));
    treenode_t *child = exp->child;
    assert(child);

    if (!strcmp(child->name, "NOT")) {
        translate_cond_not(exp->next, truelabel, falselabel);
        return;
    }

    treenode_t *child2, *child3;
    if ((child2 = child->next) && (child3 = child2->next)) {
        if (!strcmp(child2->name, "RELOP")) {
            translate_cond_relop(child, child3, truelabel, falselabel,
                                 str_to_icop(child2->relop));
            return;
        }   
        if (!strcmp(child2->name, "AND")) {
            translate_cond_and(child, child3, truelabel, falselabel);
            return;
        }
        if (!strcmp(child2->name, "OR")) {
            translate_cond_or(child, child3, truelabel, falselabel);
            return;
        }
    }
    translate_cond_otherwise(exp, truelabel, falselabel);
}

void translate_cond_not(treenode_t *exp, int truelabel, int falselabel)
{
    translate_cond(exp, falselabel, truelabel);
}

void translate_cond_relop(treenode_t *lexp, treenode_t *rexp,
                          int truelabel, int falselabel, int icop)
{
    operand_t lhs = translate_exp(lexp);
    operand_t rhs = translate_exp(rexp);

    if (is_const_operand(&lhs) && is_const_operand(&rhs)) {
        int cond;
        switch (icop) {
        case ICOP_EQ:  cond = (lhs.val == rhs.val); break;
        case ICOP_NEQ: cond = (lhs.val != rhs.val); break;
        case ICOP_L:   cond = (lhs.val < rhs.val);  break;
        case ICOP_LE:  cond = (lhs.val <= rhs.val); break;
        case ICOP_G:   cond = (lhs.val > rhs.val);  break;
        case ICOP_GE:  cond = (lhs.val >= rhs.val); break;
        default: assert(0); break;
        }
        intercodes_push_back(create_ic_goto((cond ? truelabel : falselabel)));
        return;
    }

    intercodes_push_back(create_ic_condgoto(icop, &lhs, &rhs, truelabel));
    intercodes_push_back(create_ic_goto(falselabel));
}

void translate_cond_and(treenode_t *lexp, treenode_t *rexp,
                        int truelabel, int falselabel)
{
    int labelid = alloc_labelid();
    translate_cond(lexp, labelid, falselabel);
    intercodes_push_back(create_ic_label(labelid));
    translate_cond(rexp, truelabel, falselabel);
}

void translate_cond_or(treenode_t *lexp, treenode_t *rexp,
                       int truelabel, int falselabel)
{
    int labelid = alloc_labelid();
    translate_cond(lexp, truelabel, labelid);
    intercodes_push_back(create_ic_label(labelid));
    translate_cond(rexp, truelabel, falselabel);
}

void translate_cond_otherwise(treenode_t *exp, int truelabel, int falselabel)
{
    operand_t op = translate_exp(exp);

    if (is_const_operand(&op)) {
        intercodes_push_back(create_ic_goto((op.val ? truelabel : falselabel)));
        return;
    }

    operand_t zero;
    init_const_operand(&zero, 0);
    intercodes_push_back(create_ic_condgoto(ICOP_NEQ, &op, &zero, truelabel));
    intercodes_push_back(create_ic_goto(falselabel));
}