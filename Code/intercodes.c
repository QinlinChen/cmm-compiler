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

void translate_exp(treenode_t *exp, operand_t *ret);
void translate_literal(treenode_t *literal, operand_t *ret);
void translate_var(treenode_t *id, operand_t *ret);
void translate_unary_minus(treenode_t *exp, operand_t *ret);
void translate_arithbop(treenode_t *lexp, treenode_t *rexp,
                        int icop, operand_t *ret);

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
    type_t *inttype = (type_t *)create_type_basic(TYPE_INT);
    typelist_t typelist;
    init_typelist(&typelist);
    typelist_push_back(&typelist, inttype);

    type_t *readfunctype = (type_t *)create_type_func(inttype, NULL);
    type_t *writefunctype = (type_t *)create_type_func(inttype, &typelist);

    symbol_t readfunc, writefunc;
    init_symbol(&readfunc, readfunctype, "read", 0, 1);
    init_symbol(&writefunc, writefunctype, "write", 0, 1);
    symbol_table_add(&readfunc);
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
        operand_t op;
        translate_exp(stmt_list, &op);
        fprint_operand(stdout, &op);
        printf("\n");
        return;
    }

    for (treenode_t *child = stmt_list->child; child != NULL; child = child->next)
        translate_stmt_list(child);
}

void translate_exp(treenode_t *exp, operand_t *ret)
{
    assert(exp);
    assert(!strcmp(exp->name, "Exp"));
    treenode_t *child = exp->child;
    assert(child);

    if (!strcmp(child->name, "INT") || !strcmp(child->name, "FLOAT")) {
        translate_literal(child, ret);
        return;
    }
    if (!strcmp(child->name, "ID")) {
        if (!child->next) {
            translate_var(child, ret);
            return;
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
    if (!strcmp(child->name, "LP")) {
        translate_exp(child->next, ret);
        return;
    }
    if (!strcmp(child->name, "MINUS")) {
        translate_unary_minus(child->next, ret);
        return;
    }
    if (!strcmp(child->name, "NOT")) {
        assert(0);  // TODO:
        // return typecheck_unary_op(child->next, OP_UNARY_BOOL, is_lval);
        return;
    }
    assert(!strcmp(child->name, "Exp"));
    treenode_t *child2 = child->next;
    assert(child2);
    treenode_t *child3 = child2->next;
    assert(child3);
    // if (!strcmp(child2->name, "DOT"))
    //     return typecheck_struct_access(child, child2, child3, is_lval);
    // if (!strcmp(child2->name, "LB"))
    //     return typecheck_array_access(child, child3, is_lval);
    // if (!strcmp(child2->name, "ASSIGNOP"))
    //     return typecheck_assign(child, child3, is_lval);
    // if (!strcmp(child2->name, "RELOP"))
    //     return typecheck_binary_op(child, child3, OP_REL, is_lval);
    // if (!strcmp(child2->name, "AND") || !strcmp(child2->name, "OR"))
    //     return typecheck_binary_op(child, child3, OP_BINARY_BOOL, is_lval);
    if (!strcmp(child2->name, "PLUS")) {
        translate_arithbop(child, child3, ICOP_ADD, ret);
        return;
    }
    if (!strcmp(child2->name, "MINUS")) {
        translate_arithbop(child, child3, ICOP_SUB, ret);
        return;
    }
    if (!strcmp(child2->name, "STAR")) {
        translate_arithbop(child, child3, ICOP_MUL, ret);
        return;
    }
    if (!strcmp(child2->name, "DIV")) {
        translate_arithbop(child, child3, ICOP_DIV, ret);
        return;
    }
    assert(0);  /* Should not reach here! */
}

void translate_literal(treenode_t *literal, operand_t *ret)
{
    assert(literal);

    if (literal->token != INT) {
        translate_error(literal->lineno, "Assumption 1 is violated. "
                                         "Floats are not allowed.");
        return;
    }
    if (ret)
        init_const_operand(ret, literal->ival);
}

void translate_var(treenode_t *id, operand_t *ret)
{
    assert(id);
    assert(id->token == ID);

    symbol_t *symbol;
    if (symbol_table_find_by_name(id->id, &symbol) != 0) {
        assert(0);
        return;
    }
    if (ret)
        init_var_operand(ret, symbol->id);
}

void translate_unary_minus(treenode_t *exp, operand_t *ret)
{
    assert(exp);

    operand_t subexp;
    translate_exp(exp, &subexp);

    if (subexp.kind == OPRAND_CONST) {
        init_const_operand(ret, -subexp.val);
        return;
    }
        
    init_temp_var(ret);
    operand_t zero;
    init_const_operand(&zero, 0);
    intercodes_push_back(create_ic_arithbop(ICOP_SUB, ret, &zero, &subexp));
}

void translate_arithbop(treenode_t *lexp, treenode_t *rexp,
                        int icop, operand_t *ret)
{
    assert(lexp);
    assert(rexp);
    
    // TODO:
}