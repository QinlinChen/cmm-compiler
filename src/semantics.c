#include "semantics.h"
#include "type-system.h"
#include "syntax.tab.h"
#include "intercode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

/* ------------------------------------ *
 *           semantic errors            *
 * ------------------------------------ */

static int _has_semantic_error = 0;

void semantic_error(int errtype, int lineno, const char *msg, ...)
{
    _has_semantic_error = 1;
    printf("Error type %d at Line %d: ", errtype, lineno);
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    printf("\n");
}

int has_semantic_error()
{
    return _has_semantic_error;
}

/* ------------------------------------ *
 *           semantic analyse           *
 * ------------------------------------ */

/* Recursively find ExtDef and begin our analysis. */
void semantic_analyse_r(treenode_t *node);

/* Analyse ExtDef and add symbols(variables/functions) to the symbol table. */
void analyse_ext_def(treenode_t *ext_def);

/* Analyse StructSpecifier and return the type infomation. */
type_t *analyse_struct_specifier(treenode_t *struct_specifier);

/* enum for 'context', the argument of analyse_def_list */
enum { CONTEXT_STRUCT_DEF, CONTEXT_VAR_DEF };

/* Analyse DefList. If it is called during the context of struct field definition,
 * we will append the field to the 'fieldlist'. Otherwise, it must be analysing
 * local definitions and we will add them to the symbol table. In this case,
 * 'fieldlist' can be NULL. */
void analyse_def_list(treenode_t *def_list, fieldlist_t *ret, int context);

/* Analysis functions used by analyse_def_list */
void analyse_def(treenode_t *def, fieldlist_t *fieldlist, int context);
void analyse_dec_list(treenode_t *dec_list, type_t *spec,
                      fieldlist_t *fieldlist, int context);
void analyse_dec(treenode_t *dec, type_t *spec,
                 fieldlist_t *fieldlist, int context);

/* Analyse ExtDecList and add global variables to the symbol table. */
void analyse_ext_dec_list(treenode_t *ext_dec_list, type_t *spec);

/* Analysis functions used by analyse_func_dec */
void analyse_var_list(treenode_t *var_list, fieldlist_t *fieldlist);
void analyse_param_dec(treenode_t *param_dec, fieldlist_t *fieldlist);

/* Analyse CompSt. If 'params' is not NULL, it will add them to the
 * symbol table on entering the new scope. */
void analyse_comp_st(treenode_t *comp_st, type_t *ret_spec, fieldlist_t *params);
void analyse_stmt_list(treenode_t *stmt_list, type_t *ret_spec);
void analyse_stmt(treenode_t *stmt, type_t *ret_spec);

/* Typecheck. */
int analyse_args(treenode_t *args, typelist_t *ret_args);

type_t *typecheck_exp(treenode_t *exp, int *is_lval);
type_t *typecheck_literal(treenode_t *literal, int *is_lval);
type_t *typecheck_var(treenode_t *id, int *is_lval);
type_t *typecheck_struct_access(treenode_t *exp, treenode_t *dot,
                                treenode_t *id, int *is_lval);
type_t *typecheck_array_access(treenode_t *exp, treenode_t *idxexp, int *is_lval);
type_t *typecheck_func_call(treenode_t *id, treenode_t *args, int *is_lval);

enum {
    OP_BINARY_ARITH, OP_BINARY_BOOL, OP_REL,
    OP_UNARY_ARITH, OP_UNARY_BOOL,
};

type_t *typecheck_binary_op(treenode_t *lexp, treenode_t *rexp,
                            int op, int *is_lval);
type_t *typecheck_unary_op(treenode_t *exp, int op, int *is_lval);
type_t *typecheck_assign(treenode_t *lexp, treenode_t *rexp, int *is_lval);

void semantic_analyse(treenode_t *root)
{
    init_varid();
    init_structdef_table();
    init_symbol_table();

    add_builtin_func();

    semantic_analyse_r(root);

    symbol_table_check_undefined_symbol();

    // print_structdef_table();
    // print_symbol_table();
}

void semantic_analyse_r(treenode_t *node)
{
    if (!node)
        return;

    if (!strcmp(node->name, "ExtDef")) {
        analyse_ext_def(node);
        return;
    }

    for (treenode_t *child = node->child; child != NULL; child = child->next)
        semantic_analyse_r(child);
}

void analyse_ext_def(treenode_t *ext_def)
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
        analyse_ext_dec_list(child2, spec);
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
        if (is_def)
            analyse_comp_st(child2->next, spec, &paramlist);
        return;
    }
    assert(!strcmp(child2->name, "SEMI"));
}

type_t *analyse_specifier(treenode_t *specifier)
{
    assert(specifier);
    assert(!strcmp(specifier->name, "Specifier"));
    treenode_t *child = specifier->child;
    assert(child);

    if (!strcmp(child->name, "TYPE"))
        return (type_t *)create_type_basic(child->type_id);
    if (!strcmp(child->name, "StructSpecifier"))
        return analyse_struct_specifier(child);

    assert(0);  /* Should not reach here! */
    return NULL;
}

type_t *analyse_struct_specifier(treenode_t *struct_specifier)
{
    assert(struct_specifier);
    assert(!strcmp(struct_specifier->name, "StructSpecifier"));
    assert(struct_specifier->child);
    treenode_t *child2 = struct_specifier->child->next;
    assert(child2);
    type_struct_t *structdef = NULL;

    if (!strcmp(child2->name, "OptTag")) {
        /* In this case, we create a named struct type
         * and add it to structdef_table. */
        treenode_t *id = child2->child;
        assert(id->token == ID);
        assert(id->id);
        fieldlist_t fieldlist;
        init_fieldlist(&fieldlist);
        if (strcmp(child2->next->next->name, "RC"))
            analyse_def_list(child2->next->next, &fieldlist, CONTEXT_STRUCT_DEF);
        structdef = create_type_struct(id->id, &fieldlist);
        if (checked_structdef_table_add(structdef, id->lineno) != 0)
            return NULL;
        return (type_t *)structdef;
    }
    if (!strcmp(child2->name, "Tag")) {
        /* In this case, we search an already defined struct type
         * from the structdef_table */
        treenode_t *id = child2->child;
        assert(id->token == ID);
        assert(id->id);
        if (!(structdef = structdef_table_find_by_name(id->id)))
            semantic_error(17, child2->lineno, "Undefined structure \"%s\".",
                           id->id);
        return (type_t *)structdef;     /* can be NULL */
    }
    assert(!strcmp(child2->name, "LC"));
    /* In this case, we create an anonymous struct type
     * and add it to structdef_table. */
    fieldlist_t fieldlist;
    init_fieldlist(&fieldlist);
    if (strcmp(child2->next->name, "RC"))
        analyse_def_list(child2->next, &fieldlist, CONTEXT_STRUCT_DEF);
    structdef = create_type_struct(NULL, &fieldlist);
    structdef_table_add(structdef);
    return (type_t *)structdef;
}

void analyse_def_list(treenode_t *def_list, fieldlist_t *ret, int context)
{
    assert(def_list);
    assert(!strcmp(def_list->name, "DefList"));
    treenode_t *def = def_list->child;
    assert(def);

    analyse_def(def, ret, context);
    if (def->next)
        analyse_def_list(def->next, ret, context);
}

void analyse_def(treenode_t *def, fieldlist_t *fieldlist, int context)
{
    assert(def);
    assert(!strcmp(def->name, "Def"));
    treenode_t *specifier = def->child;
    assert(specifier);

    type_t *spec = analyse_specifier(specifier);
    if (!spec)
        return;
    analyse_dec_list(specifier->next, spec, fieldlist, context);
}

void analyse_dec_list(treenode_t *dec_list, type_t *spec,
                      fieldlist_t *fieldlist, int context)
{
    assert(dec_list);
    assert(!strcmp(dec_list->name, "DecList"));
    treenode_t *dec = dec_list->child;
    assert(dec);

    analyse_dec(dec, spec, fieldlist, context);
    if (dec->next)
        analyse_dec_list(dec->next->next, spec, fieldlist, context);
}

void analyse_dec(treenode_t *dec, type_t *spec,
                 fieldlist_t *fieldlist, int context)
{
    assert(dec);
    assert(!strcmp(dec->name, "Dec"));
    treenode_t *var_dec = dec->child;
    assert(var_dec);

    symbol_t symbol;
    analyse_var_dec(var_dec, spec, &symbol);
    if (context == CONTEXT_STRUCT_DEF)
        checked_fieldlist_push_back(fieldlist, &symbol);
    else if (context == CONTEXT_VAR_DEF)
        checked_symbol_table_add_var(&symbol);
    else
        assert(0); /* Should not reach here! */

    treenode_t *assignop = var_dec->next;
    if (assignop) {
        assert(!strcmp(assignop->name, "ASSIGNOP"));
        if (context == CONTEXT_STRUCT_DEF) {
            semantic_error(15, assignop->lineno,
                           "Field assigned during definition.");
        }
        else if (context == CONTEXT_VAR_DEF) {
            /* Create a temporory tree to fit the interface of 'typecheck_assign' */
            treenode_t *temp_exp = create_nontermnode("Exp", symbol.lineno);
            treenode_t *temp_id = create_idnode(symbol.lineno, symbol.name);
            add_child(temp_exp, temp_id);
            typecheck_assign(temp_exp, assignop->next, NULL);
            destroy_treenode(temp_id);
            destroy_treenode(temp_exp);
        }
        else {
            assert(0); /* Should not reach here! */
        }
    }
}

void analyse_var_dec(treenode_t *var_dec, type_t *spec, symbol_t *ret)
{
    assert(var_dec);
    assert(!strcmp(var_dec->name, "VarDec"));
    treenode_t *child = var_dec->child;
    assert(child);

    if (!strcmp(child->name, "ID")) {
        assert(child->id);
        init_symbol(ret, spec, child->id, child->lineno, 0);
        return;
    }
    assert(child->next);
    treenode_t *intnode = child->next->next;
    assert(intnode);
    assert(intnode->token == INT);
    type_array_t *type_array = create_type_array(intnode->ival, spec);
    analyse_var_dec(child, (type_t *)type_array, ret);
}

void analyse_ext_dec_list(treenode_t *ext_dec_list, type_t *spec)
{
    assert(ext_dec_list);
    assert(!strcmp(ext_dec_list->name, "ExtDecList"));
    treenode_t *var_dec = ext_dec_list->child;
    assert(var_dec);

    symbol_t symbol;
    analyse_var_dec(var_dec, spec, &symbol);
    checked_symbol_table_add_var(&symbol);

    if (var_dec->next)
        analyse_ext_dec_list(var_dec->next->next, spec);
}

void analyse_fun_dec(treenode_t *fun_dec, type_t *spec,
                     symbol_t *ret_symbol, fieldlist_t *ret_params)
{
    assert(fun_dec);
    assert(!strcmp(fun_dec->name, "FunDec"));
    treenode_t *id = fun_dec->child;
    assert(id);
    assert(id->token = ID);
    assert(id->next);
    treenode_t *child3 = id->next->next;
    assert(child3);

    if (!strcmp(child3->name, "VarList"))
        analyse_var_list(child3, ret_params);
    else
        assert(!strcmp(child3->name, "RP"));

    type_func_t *type_func = create_type_func(spec, NULL);
    type_func_add_params_from_fieldlist(type_func, ret_params);
    init_symbol(ret_symbol, (type_t *)type_func, id->id, id->lineno, 0);
}

void analyse_var_list(treenode_t *var_list, fieldlist_t *paramlist)
{
    assert(var_list);
    assert(!strcmp(var_list->name, "VarList"));
    treenode_t *param_dec = var_list->child;
    assert(param_dec);

    analyse_param_dec(param_dec, paramlist);
    if (param_dec->next)
        analyse_var_list(param_dec->next->next, paramlist);
}

void analyse_param_dec(treenode_t *param_dec, fieldlist_t *paramlist)
{
    assert(param_dec);
    assert(!strcmp(param_dec->name, "ParamDec"));
    treenode_t *specifier = param_dec->child;
    assert(specifier);

    type_t *spec = analyse_specifier(specifier);
    if (!spec)
        return;

    treenode_t *var_dec = specifier->next;
    symbol_t symbol;
    analyse_var_dec(var_dec, spec, &symbol);
    checked_paramlist_push_back(paramlist, &symbol);
}

void analyse_comp_st(treenode_t *comp_st, type_t *ret_spec, fieldlist_t *params)
{
    assert(comp_st);
    assert(!strcmp(comp_st->name, "CompSt"));
    assert(comp_st->child);
    symbol_table_pushenv();
    if (params)
        symbol_table_add_params(params);

    treenode_t *child2 = comp_st->child->next;
    assert(child2);

    if (!strcmp(child2->name, "DefList")) {
        analyse_def_list(child2, NULL, CONTEXT_VAR_DEF);
        treenode_t *child3 = child2->next;
        if (!strcmp(child3->name, "StmtList"))
            analyse_stmt_list(child3, ret_spec);
        else
            assert(!strcmp(child3->name, "RC"));
    }
    else if (!strcmp(child2->name, "StmtList")) {
        analyse_stmt_list(child2, ret_spec);
    }
    else {
        assert(!strcmp(child2->name, "RC"));
    }

    symbol_table_popenv();  /* Remember to pop environment! */
}

void analyse_stmt_list(treenode_t *stmt_list, type_t *ret_spec)
{
    assert(stmt_list);
    assert(!strcmp(stmt_list->name, "StmtList"));
    treenode_t *stmt = stmt_list->child;
    assert(stmt);

    analyse_stmt(stmt, ret_spec);
    if (stmt->next)
        analyse_stmt_list(stmt->next, ret_spec);
}

void analyse_stmt(treenode_t *stmt, type_t *ret_spec)
{
    assert(stmt);
    assert(!strcmp(stmt->name, "Stmt"));
    treenode_t *child = stmt->child;
    assert(child);

    if (!strcmp(child->name, "Exp")) {
        typecheck_exp(child, NULL);
    }
    else if (!strcmp(child->name, "CompSt")) {
        analyse_comp_st(child, ret_spec, NULL);
    }
    else if (!strcmp(child->name, "RETURN")) {
        assert(child->next);
        type_t *ret_type = typecheck_exp(child->next, NULL);
        if (ret_type && !type_is_equal(ret_spec, ret_type)) {
            semantic_error(8, child->next->lineno,
                           "Type mismatched for return.");
            return;
        }
    }
    else {
        assert(!strcmp(child->name, "IF") || !strcmp(child->name, "WHILE"));
        treenode_t *exp = child->next->next;
        type_t *exptype = typecheck_exp(exp, NULL);
        if (exptype && !type_is_int(exptype))
            semantic_error(0, exp->lineno, "Expression conflicts assumption 2.");
        treenode_t *stmt = exp->next->next;
        analyse_stmt(stmt, ret_spec);
        if (child->token == IF && stmt->next)
            analyse_stmt(stmt->next->next, ret_spec);
    }
}

type_t *typecheck_exp(treenode_t *exp, int *is_lval)
{
    assert(exp);
    assert(!strcmp(exp->name, "Exp"));
    treenode_t *child = exp->child;
    assert(child);

    if (!strcmp(child->name, "INT") || !strcmp(child->name, "FLOAT"))
        return typecheck_literal(child, is_lval);
    if (!strcmp(child->name, "ID")) {
        if (!child->next)
            return typecheck_var(child, is_lval);
        assert(!strcmp(child->next->name, "LP"));
        treenode_t *child3 = child->next->next;
        assert(child3);
        if (!strcmp(child3->name, "Args"))
            return typecheck_func_call(child, child3, is_lval);
        assert(!strcmp(child3->name, "RP"));
        return typecheck_func_call(child, NULL, is_lval);
    }
    if (!strcmp(child->name, "LP"))
        return typecheck_exp(child->next, is_lval);
    if (!strcmp(child->name, "MINUS"))
        return typecheck_unary_op(child->next, OP_UNARY_ARITH, is_lval);
    if (!strcmp(child->name, "NOT"))
        return typecheck_unary_op(child->next, OP_UNARY_BOOL, is_lval);
    assert(!strcmp(child->name, "Exp"));
    treenode_t *child2 = child->next;
    assert(child2);
    treenode_t *child3 = child2->next;
    assert(child3);
    if (!strcmp(child2->name, "DOT"))
        return typecheck_struct_access(child, child2, child3, is_lval);
    if (!strcmp(child2->name, "LB"))
        return typecheck_array_access(child, child3, is_lval);
    if (!strcmp(child2->name, "ASSIGNOP"))
        return typecheck_assign(child, child3, is_lval);
    if (!strcmp(child2->name, "RELOP"))
        return typecheck_binary_op(child, child3, OP_REL, is_lval);
    if (!strcmp(child2->name, "AND") || !strcmp(child2->name, "OR"))
        return typecheck_binary_op(child, child3, OP_BINARY_BOOL, is_lval);
    if (!strcmp(child2->name, "PLUS") || !strcmp(child2->name, "MINUS") ||
        !strcmp(child2->name, "STAR") || !strcmp(child2->name, "DIV"))
        return typecheck_binary_op(child, child3, OP_BINARY_ARITH, is_lval);
    assert(0);  /* Should not reach here! */
    return NULL;
}

type_t *typecheck_literal(treenode_t *literal, int *is_lval)
{
    assert(literal);
    assert(literal->is_term);
    if (is_lval)
        *is_lval = 0;

    switch (literal->token) {
    case INT: return (type_t *)create_type_basic(TYPE_INT);
    case FLOAT: return (type_t *)create_type_basic(TYPE_FLOAT);
    default: break;
    }
    assert(0);  /* Should not reach here! */
    return TYPE_INT;
}

type_t *typecheck_var(treenode_t *id, int *is_lval)
{
    assert(id);
    assert(id->token == ID);
    if (is_lval)
        *is_lval = 1;

    symbol_t *symbol;
    if (symbol_table_find_by_name(id->id, &symbol) != 0) {
        semantic_error(1, id->lineno, "Undefined variable \"%s\".", id->id);
        return NULL;
    }
    return symbol->type;
}

type_t *typecheck_struct_access(treenode_t *exp, treenode_t *dot,
                                treenode_t *id, int *is_lval)
{
    assert(exp);
    assert(dot);
    assert(id);
    assert(id->token == ID);
    if (is_lval)
        *is_lval = 1;

    type_t *exptype = typecheck_exp(exp, NULL);
    if (!exptype)
        return NULL;
    if (exptype->kind != TYPE_STRUCT) {
        semantic_error(13, dot->lineno, "Illegal use of \".\".");
        return NULL;
    }
    type_t *ret_type = type_struct_access((type_struct_t *)exptype, id->id, NULL);
    if (!ret_type) {
        semantic_error(14, id->lineno, "Non-existent field \"%s\".", id->id);
        return NULL;
    }
    return ret_type;
}

type_t *typecheck_array_access(treenode_t *exp, treenode_t *idxexp, int *is_lval)
{
    assert(exp);
    assert(idxexp);
    if (is_lval)
        *is_lval = 1;

    int exptype_error = 0, idxexptype_error = 0;
    type_t *exptype = typecheck_exp(exp, NULL);
    type_t *idxexptype = typecheck_exp(idxexp, NULL);
    /* Beacause we want to report as many errors as possible,
     * we check exptype errors and idxexptype errors seperately
     * without early return. */
    if (!exptype)
        exptype_error = 1;
    else if (exptype->kind != TYPE_ARRAY) {
        semantic_error(10, exp->lineno, "\"%s\" is not an array.",
                       treenode_repr(exp));
        exptype_error = 1;
    }
    if (!idxexptype)
        idxexptype_error = 1;
    else if (!type_is_int(idxexptype)) {
        semantic_error(12, idxexp->lineno, "\"%s\" is not an integer.",
                       treenode_repr(idxexp));
        idxexptype_error = 1;
    }
    if (exptype_error || idxexptype_error) {
        if (!exptype_error) /* Try repairing. */
            return type_array_access((type_array_t *)exptype);
        return NULL;
    }

    return type_array_access((type_array_t *)exptype);
}

type_t *typecheck_func_call(treenode_t *id, treenode_t *args, int *is_lval)
{
    assert(id);
    if (is_lval)
        *is_lval = 0;

    symbol_t *symbol;
    if (symbol_table_find_by_name(id->id, &symbol) != 0) {
        semantic_error(2, id->lineno, "Undefined function \"%s\".", id->id);
        return NULL;
    }
    assert(symbol->type);
    if (symbol->type->kind != TYPE_FUNC) {
        semantic_error(11, id->lineno, "\"%s\" is not a function.", id->id);
        return NULL;
    }
    type_func_t *funcinfo = (type_func_t *)symbol->type;

    typelist_t arglist;
    init_typelist(&arglist);
    if (args && analyse_args(args, &arglist) != 0)
        return funcinfo->ret_type;  /* Try repairing. */

    if (!typelist_is_equal(&funcinfo->types, &arglist)) {
        /* sematic_error function is not strong enough to print
         * all error infomation as we want. So, here we work around it. */
        printf("Error type 9 at Line %d: Function \"%s(", id->lineno, id->id);
        print_typelist(&funcinfo->types);
        printf(")\" is not applicable for arguments \"(");
        print_typelist(&arglist);
        printf(")\".\n");
        return funcinfo->ret_type;  /* Try repairing. */
    }
    return funcinfo->ret_type;
}

int analyse_args(treenode_t *args, typelist_t *ret_args)
{
    assert(args);
    assert(!strcmp(args->name, "Args"));
    treenode_t *arg = args->child;
    assert(arg);

    type_t *arg_type = typecheck_exp(arg, NULL);
    if (!arg_type)
        return -1; /* Failure */
    typelist_push_back(ret_args, arg_type);
    if (arg->next) {
        assert(arg->next->next);
        return analyse_args(arg->next->next, ret_args);
    }
    return 0; /* Success */
}

type_t *typecheck_binary_op(treenode_t *lexp, treenode_t *rexp,
                            int op, int *is_lval)
{
    assert(lexp);
    assert(rexp);
    if (is_lval)
        *is_lval = 0;

    type_t *ltype = typecheck_exp(lexp, NULL);
    type_t *rtype = typecheck_exp(rexp, NULL);
    if (!ltype || !rtype)
        return NULL;

    if (!type_is_equal(ltype, rtype)) {
        semantic_error(7, lexp->lineno, "Type mismatched for operands.");
        return NULL;
    }

    if (op == OP_BINARY_ARITH) {
        if (ltype->kind != TYPE_BASIC) {
            semantic_error(7, lexp->lineno,
                           "Type mismatched for the operator and operands. "
                           "\"int\" or \"float\" is expected.");
            return NULL;
        }
        return ltype;
    }
    if (op == OP_BINARY_BOOL) {
        if (!type_is_int(ltype)) {
            semantic_error(7, lexp->lineno,
                           "Type mismatched for the operator and operands. "
                           "\"int\" is expected.");
            return NULL;
        }
        return ltype;
    }
    if (op == OP_REL) {
        if (ltype->kind != TYPE_BASIC) {
            semantic_error(7, lexp->lineno,
                           "Type mismatched for the operator and operands. "
                           "\"int\" or \"float\" is expected.");
            return NULL;
        }
        return (type_t *)create_type_basic(TYPE_INT);
    }
    assert(0);  /* Should not reach here! */
    return NULL;
}

type_t *typecheck_unary_op(treenode_t *exp, int op, int *is_lval)
{
    assert(exp);
    if (is_lval)
        *is_lval = 0;

    type_t *exptype = typecheck_exp(exp, NULL);
    if (!exptype)
        return NULL;

    if (op == OP_UNARY_ARITH) {
        if (exptype->kind != TYPE_BASIC) {
            semantic_error(7, exp->lineno,
                           "Type mismatched for the operator and the operand. "
                           "\"int\" or \"float\" is expected.");
            return NULL;
        }
        return exptype;
    }
    if (op == OP_UNARY_BOOL) {
        if (!type_is_int(exptype)) {
            semantic_error(7, exp->lineno,
                           "Type mismatched for the operator and the operand. "
                           "\"int\" is expected.");
            return NULL;
        }
        return exptype;
    }
    assert(0);  /* Should not reach here! */
    return NULL;
}

type_t *typecheck_assign(treenode_t *lexp, treenode_t *rexp, int *is_lval)
{
    assert(lexp);
    assert(rexp);
    if (is_lval)
        *is_lval = 0;

    int ltype_is_lval;
    type_t *ltype = typecheck_exp(lexp, &ltype_is_lval);
    type_t *rtype = typecheck_exp(rexp, NULL);
    if (ltype && !ltype_is_lval) {
        semantic_error(6, lexp->lineno, "The left-hand side of an assignment "
                       "must be a left value.");
        return NULL;
    }
    if (!ltype || !rtype)
        return NULL;

    if (!type_is_equal(ltype, rtype)) {
        semantic_error(5, lexp->lineno, "Type mismatched for assignment.");
        return NULL;
    }
    if (ltype->kind == TYPE_FUNC) {
        semantic_error(7, lexp->lineno, "Functions should not exist at "
                       "any side of an assignment.");
        return NULL;
    }
    return ltype;
}

int checked_structdef_table_add(type_struct_t *structdef, int lineno)
{
    if (structdef_table_find_by_name(structdef->structname)) {
        semantic_error(16, lineno, "Duplicated name \"%s\".",
                       structdef->structname);
        return -1;
    }
    structdef_table_add(structdef);
    return 0;
}

int checked_fieldlist_push_back(fieldlist_t *fieldlist, symbol_t *symbol)
{
    if (fieldlist_find_type_by_fieldname(fieldlist, symbol->name)) {
        semantic_error(15, symbol->lineno, "Redefined field \"%s\".",
                       symbol->name);
        return -1;
    }
    fieldlist_push_back(fieldlist, symbol->name, symbol->type);
    return 0;
}

int checked_paramlist_push_back(fieldlist_t *paramlist, symbol_t *symbol)
{
    if (fieldlist_find_type_by_fieldname(paramlist, symbol->name)) {
        semantic_error(15, symbol->lineno, "Redefined parameter \"%s\".",
                       symbol->name);
        return -1;
    }
    fieldlist_push_back(paramlist, symbol->name, symbol->type);
    return 0;
}

int checked_symbol_table_add_var(symbol_t *symbol)
{
    assert(symbol->type->kind != TYPE_FUNC);
    if (symbol_table_find_by_name_in_curenv(symbol->name, NULL) == 0 ||
        structdef_table_find_by_name(symbol->name)) {
        semantic_error(3, symbol->lineno, "Redefined variable \"%s\".",
                       symbol->name);
        return -1;
    }
    symbol_set_defined(symbol, 1);
    symbol_table_add(symbol);
    return 0;
}

int checked_symbol_table_add_func(symbol_t *func, int is_def)
{
    assert(func->type->kind == TYPE_FUNC);
    if (structdef_table_find_by_name(func->name)) {
        semantic_error(3, func->lineno,
                       "Redefined name \"%s\".", func->name);
        return -1;
    }
    symbol_t *pfind;
    if (symbol_table_find_by_name_in_curenv(func->name, &pfind) == 0) {
        if (pfind->is_defined && is_def) {
            semantic_error(4, func->lineno,
                           "Redefined function \"%s\".", func->name);
            return -1;
        }
        if (!type_is_equal(pfind->type, func->type)) {
            semantic_error(19, func->lineno,
                           "Inconsistent declaration of function \"%s\".",
                           func->name);
            return -1;
        }
        if (is_def)
            symbol_set_defined(pfind, 1);
    }
    else {
        symbol_set_defined(func, is_def);
        symbol_table_add(func);
    }
    return 0;
}