#include "intercodes.h"
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

iclist_t *get_intercodes()
{
    return &intercodes;
}

/* ------------------------------------ *
 *           translate errors           *
 * ------------------------------------ */

static int _has_translate_error = 0;

void translate_error(int lineno, const char *msg, ...)
{
    _has_translate_error = 1;
    printf("Line %d: ", lineno);
    va_list ap;
    va_start(ap, msg);
    vprintf(msg, ap);
    va_end(ap);
    printf("\n");
}

int has_translate_error()
{
    return _has_translate_error;
}

/* ------------------------------------ *
 *              translate               *
 * ------------------------------------ */

void intercodes_translate_r(treenode_t *node);
void translate_ext_def(treenode_t *ext_def);
void translate_ext_dec_list(treenode_t *ext_dec_list, type_t *spec);

/* Translate local definitions. */
void translate_def_list(treenode_t *def_list);
void translate_def(treenode_t *def);
void translate_dec_list(treenode_t *dec_list, type_t *spec);
void translate_dec(treenode_t *dec, type_t *spec);

/* Translate statements */
void gen_funcdef(const char *fname, fieldlist_t *params);
void translate_comp_st(treenode_t *comp_st);
void translate_def_list(treenode_t *def_list);
void translate_stmt_list(treenode_t *stmt_list);
void translate_stmt(treenode_t *stmt);
void translate_stmt_if(treenode_t *exp, treenode_t *stmt);
void translate_stmt_if_else(treenode_t *exp, treenode_t *stmt1, treenode_t *stmt2);
void translate_stmt_while(treenode_t *exp, treenode_t *stmt);

/* Translate an expression and store the result in 'target' with type 'operand_t'.
 * If 'target' is NULL, it will allocate a temporary operand and return it.
 * In fact, 'target' is provided only in the 'optim_translate_assign' function.
 * The 'translate_exp' always sets it as NULL. */
operand_t translate_exp(treenode_t *exp);
operand_t translate_literal(treenode_t *literal);
operand_t translate_var(treenode_t *id);
operand_t translate_func_call(treenode_t *id, treenode_t *args, operand_t *target);
operand_t translate_assign(treenode_t *lexp, treenode_t *rexp);
operand_t translate_unary_minus(treenode_t *exp, operand_t *target);
operand_t translate_arithbop(treenode_t *lexp, treenode_t *rexp, int icop,
                             operand_t *target);
operand_t translate_boolexp(treenode_t *exp, operand_t *target);
operand_t translate_accessexp(treenode_t *exp);

void translate_args(treenode_t *args);
operand_t get_first_arg(treenode_t *args);

/* Try to make use of the 'target' argument to eliminate redundent variables
 * generated by assignment expression. If success, return 0. */
int optim_translate_assign(operand_t *target, treenode_t *rexp);

/* Translate an expression as a condition. */
void translate_cond(treenode_t *exp, int labeltrue, int labelfalse);
void translate_cond_not(treenode_t *exp, int labeltrue, int labelfalse);
void translate_cond_and(treenode_t *lexp, treenode_t *rexp,
                        int labeltrue, int labelfalse);
void translate_cond_or(treenode_t *lexp, treenode_t *rexp,
                       int labeltrue, int labelfalse);
void translate_cond_relop(treenode_t *lexp, treenode_t *rexp,
                          int labeltrue, int labelfalse, int icop);
void translate_cond_otherwise(treenode_t *exp, int labeltrue, int labelfalse);

/* Translate a memeory access expression and return its address. */
operand_t translate_access(treenode_t *exp, type_t **ret);
operand_t translate_access_var(treenode_t *id, type_t **ret);
operand_t translate_access_array(treenode_t *exp, treenode_t *idxexp, type_t **ret);
operand_t translate_access_struct(treenode_t *exp, treenode_t *id, type_t **ret);

/* Dereference the address generated by translate_access */
operand_t try_deref(operand_t *addr);

void intercodes_translate(treenode_t *root)
{
    init_varid();
    init_labelid();
    init_structdef_table();
    init_symbol_table();
    init_intercodes();

    add_builtin_func();

    intercodes_translate_r(root);
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
            symbol_table_add_params(&paramlist);
            gen_funcdef(func.name, &paramlist);
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

void gen_funcdef(const char *fname, fieldlist_t *params)
{
    intercodes_push_back(create_ic_funcdef(fname));

    symbol_t *symbol;
    for (fieldlistnode_t *param = params->front; param != NULL; param = param->next) {
        if (symbol_table_find_by_name(param->fieldname, &symbol) != 0) {
            assert(0);
            return;
        }
        operand_t var;
        init_var_operand(&var, symbol->id);
        intercodes_push_back(create_ic_param(&var));
    }
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
    assert(def_list);
    assert(!strcmp(def_list->name, "DefList"));
    treenode_t *def = def_list->child;
    assert(def);

    translate_def(def);
    if (def->next)
        translate_def_list(def->next);
}

void translate_def(treenode_t *def)
{
    assert(def);
    assert(!strcmp(def->name, "Def"));
    treenode_t *specifier = def->child;
    assert(specifier);

    type_t *spec = analyse_specifier(specifier);
    if (!spec)
        return;
    translate_dec_list(specifier->next, spec);
}

void translate_dec_list(treenode_t *dec_list, type_t *spec)
{
    assert(dec_list);
    assert(!strcmp(dec_list->name, "DecList"));
    treenode_t *dec = dec_list->child;
    assert(dec);

    translate_dec(dec, spec);
    if (dec->next)
        translate_dec_list(dec->next->next, spec);
}

void translate_dec(treenode_t *dec, type_t *spec)
{
    assert(dec);
    assert(!strcmp(dec->name, "Dec"));
    treenode_t *var_dec = dec->child;
    assert(var_dec);

    symbol_t symbol;
    analyse_var_dec(var_dec, spec, &symbol);
    checked_symbol_table_add_var(&symbol);
    if (symbol.type->kind != TYPE_BASIC) {
        assert(symbol.type->kind != TYPE_FUNC);
        operand_t var;
        init_var_operand(&var, symbol.id);
        intercodes_push_back(create_ic_dec(&var, symbol.type->width));
    }

    treenode_t *assignop = var_dec->next;
    if (assignop) {
        assert(!strcmp(assignop->name, "ASSIGNOP"));
        /* Create a temporory tree to fit the interface of 'translate_assign' */
        treenode_t *temp_exp = create_nontermnode("Exp", symbol.lineno);
        treenode_t *temp_id = create_idnode(symbol.lineno, symbol.name);
        add_child(temp_exp, temp_id);
        translate_assign(temp_exp, assignop->next);
        destroy_treenode(temp_id);
        destroy_treenode(temp_exp);
    }
}

void translate_stmt_list(treenode_t *stmt_list)
{
    assert(stmt_list);
    assert(!strcmp(stmt_list->name, "StmtList"));
    treenode_t *stmt = stmt_list->child;
    assert(stmt);

    translate_stmt(stmt);
    if (stmt->next)
        translate_stmt_list(stmt->next);
}

void translate_stmt(treenode_t *stmt)
{
    assert(stmt);
    assert(!strcmp(stmt->name, "Stmt"));
    treenode_t *child = stmt->child;
    assert(child);

    if (!strcmp(child->name, "Exp")) {
        translate_exp(child);
    }
    else if (!strcmp(child->name, "CompSt")) {
        translate_comp_st(child);
    }
    else if (!strcmp(child->name, "RETURN")) {
        assert(child->next);
        operand_t ret = translate_exp(child->next);
        ret = try_deref(&ret);
        intercodes_push_back(create_ic_return(&ret));
    }
    else if (!strcmp(child->name, "IF")) {
        treenode_t *exp = child->next->next;
        treenode_t *stmt = exp->next->next;
        if (stmt->next)
            translate_stmt_if_else(exp, stmt, stmt->next->next);
        else
            translate_stmt_if(exp, stmt);
    }
    else {
        assert(!strcmp(child->name, "WHILE"));
        treenode_t *exp = child->next->next;
        treenode_t *stmt = exp->next->next;
        translate_stmt_while(exp, stmt);
    }
}

void translate_stmt_if(treenode_t *exp, treenode_t *stmt)
{
    int labelfalse = alloc_labelid();

    translate_cond(exp, LABEL_FALL, labelfalse);
    translate_stmt(stmt);
    intercodes_push_back(create_ic_label(labelfalse));
}

void translate_stmt_if_else(treenode_t *exp, treenode_t *stmt1, treenode_t *stmt2)
{
    int labelfalse = alloc_labelid();
    int labelexit = alloc_labelid();

    translate_cond(exp, LABEL_FALL, labelfalse);
    translate_stmt(stmt1);
    intercodes_push_back(create_ic_goto(labelexit));
    intercodes_push_back(create_ic_label(labelfalse));
    translate_stmt(stmt2);
    intercodes_push_back(create_ic_label(labelexit));
}

void translate_stmt_while(treenode_t *exp, treenode_t *stmt)
{
    int labelbegin = alloc_labelid();
    int labelexit = alloc_labelid();

    intercodes_push_back(create_ic_label(labelbegin));
    translate_cond(exp, LABEL_FALL, labelexit);
    translate_stmt(stmt);
    intercodes_push_back(create_ic_goto(labelbegin));
    intercodes_push_back(create_ic_label(labelexit));
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
        assert(!strcmp(child->next->name, "LP"));
        treenode_t *child3 = child->next->next;
        assert(child3);
        if (!strcmp(child3->name, "Args"))
            return translate_func_call(child, child3, NULL);
        assert(!strcmp(child3->name, "RP"));
        return translate_func_call(child, NULL, NULL);
    }
    if (!strcmp(child->name, "LP"))
        return translate_exp(child->next);
    if (!strcmp(child->name, "MINUS"))
        return translate_unary_minus(child->next, NULL);
    if (!strcmp(child->name, "NOT"))
        return translate_boolexp(exp, NULL);

    assert(!strcmp(child->name, "Exp"));
    treenode_t *child2 = child->next;
    assert(child2);
    treenode_t *child3 = child2->next;
    assert(child3);
    if (!strcmp(child2->name, "PLUS"))
        return translate_arithbop(child, child3, ICOP_ADD, NULL);
    if (!strcmp(child2->name, "MINUS"))
        return translate_arithbop(child, child3, ICOP_SUB, NULL);
    if (!strcmp(child2->name, "STAR"))
        return translate_arithbop(child, child3, ICOP_MUL, NULL);
    if (!strcmp(child2->name, "DIV"))
        return translate_arithbop(child, child3, ICOP_DIV, NULL);
    if (!strcmp(child2->name, "ASSIGNOP"))
        return translate_assign(child, child3);
    if (!strcmp(child2->name, "AND") || !strcmp(child2->name, "OR") ||
        !strcmp(child2->name, "RELOP"))
        return translate_boolexp(exp, NULL);
    if (!strcmp(child2->name, "DOT") || !strcmp(child2->name, "LB"))
        return translate_accessexp(exp);
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
    operand_t var;

    if (symbol_table_find_by_name(id->id, &symbol) != 0) {
        assert(0);
        init_const_operand(&var, 0);
        return var;
    }

    if (symbol->type->kind == TYPE_BASIC) {
        init_var_operand(&var, symbol->id);
        return var;
    }

    assert(symbol->type->kind == TYPE_ARRAY || symbol->type->kind == TYPE_STRUCT);
    if (symbol->is_param) {
        init_addr_operand(&var, symbol->id);
        return var;
    }

    operand_t addr;
    init_temp_addr(&addr);
    init_var_operand(&var, symbol->id);
    intercodes_push_back(create_ic_ref(&addr, &var));
    return addr;
}

operand_t translate_func_call(treenode_t *id, treenode_t *args, operand_t *target)
{
    assert(id);
    assert(id->id);
    operand_t ret;

    if (!strcmp(id->id, "read")) {
        assert(!args);
        if (target) {
            intercodes_push_back(create_ic_read(target));
            return *target;
        }
        init_temp_var(&ret);
        intercodes_push_back(create_ic_read(&ret));
        return ret;
    }

    if (!strcmp(id->id, "write")) {
        assert(args);
        operand_t arg = get_first_arg(args);
        arg = try_deref(&arg);
        intercodes_push_back(create_ic_write(&arg));
        init_const_operand(&ret, 0);
        if (target) {
            intercodes_push_back(create_ic_assign(target, &ret));
            return *target;
        }
        return ret;
    }

    if (args)
        translate_args(args);
    if (target) {
        intercodes_push_back(create_ic_call(id->id, target));
        return *target;
    }
    init_temp_var(&ret);
    intercodes_push_back(create_ic_call(id->id, &ret));
    return ret;
}

void translate_args(treenode_t *args)
{
    assert(args);
    assert(!strcmp(args->name, "Args"));
    treenode_t *arg = args->child;
    assert(arg);

    operand_t result = translate_exp(arg);
    if (arg->next)
        translate_args(arg->next->next);
    intercodes_push_back(create_ic_arg(&result));
}

operand_t get_first_arg(treenode_t *args)
{
    assert(args);
    assert(!strcmp(args->name, "Args"));
    return translate_exp(args->child);
}

operand_t translate_assign(treenode_t *lexp, treenode_t *rexp)
{
    assert(lexp);
    assert(rexp);

    operand_t lhs = translate_exp(lexp);

    if (lhs.kind == OPERAND_VAR && optim_translate_assign(&lhs, rexp) == 0)
        return lhs;

    operand_t rhs = translate_exp(rexp);
    rhs = try_deref(&rhs);

    assert(!is_const_operand(&lhs));
    if (lhs.kind == OPERAND_ADDR)
        intercodes_push_back(create_ic_drefassign(&lhs, &rhs));
    else
        intercodes_push_back(create_ic_assign(&lhs, &rhs));
    return lhs;
}

int optim_translate_assign(operand_t *target, treenode_t *rexp)
{
    assert(rexp);
    assert(!strcmp(rexp->name, "Exp"));
    treenode_t *child = rexp->child;
    assert(child);

    if (!strcmp(child->name, "ID") && child->next) {
        assert(!strcmp(child->next->name, "LP"));
        treenode_t *child3 = child->next->next;
        assert(child3);
        if (!strcmp(child3->name, "Args")) {
            translate_func_call(child, child3, target);
        }
        else {
            assert(!strcmp(child3->name, "RP"));
            translate_func_call(child, NULL, target);
        }
        return 0;
    }
    if (!strcmp(child->name, "LP")) {
        optim_translate_assign(target, child->next);
        return 0;
    }
    if (!strcmp(child->name, "MINUS")) {
        translate_unary_minus(child->next, target);
        return 0;
    }
    if (!strcmp(child->name, "NOT")) {
        translate_boolexp(rexp, target);
        return 0;
    }
    if (strcmp(child->name, "Exp"))
        return -1;

    treenode_t *child2 = child->next;
    assert(child2);
    treenode_t *child3 = child2->next;
    assert(child3);
    if (!strcmp(child2->name, "PLUS")) {
        translate_arithbop(child, child3, ICOP_ADD, target);
        return 0;
    }
    if (!strcmp(child2->name, "MINUS")) {
        translate_arithbop(child, child3, ICOP_SUB, target);
        return 0;
    }
    if (!strcmp(child2->name, "STAR")) {
        translate_arithbop(child, child3, ICOP_MUL, target);
        return 0;
    }
    if (!strcmp(child2->name, "DIV")) {
        translate_arithbop(child, child3, ICOP_DIV, target);
        return 0;
    }
    if (!strcmp(child2->name, "AND") || !strcmp(child2->name, "OR") ||
        !strcmp(child2->name, "RELOP")) {
        translate_boolexp(rexp, target);
        return 0;
    }
    return -1;
}

operand_t translate_unary_minus(treenode_t *exp, operand_t *target)
{
    assert(exp);

    operand_t var, zero;
    operand_t subexp = translate_exp(exp);
    subexp = try_deref(&subexp);

    if (is_const_operand(&subexp)) {
        init_const_operand(&var, -subexp.val);
        if (target) {
            intercodes_push_back(create_ic_assign(target, &var));
            return *target;
        }
        return var;
    }

    init_const_operand(&zero, 0);
    if (target) {
        intercodes_push_back(create_ic_arithbop(ICOP_SUB, target, &zero, &subexp));
        return *target;
    }
    init_temp_var(&var);
    intercodes_push_back(create_ic_arithbop(ICOP_SUB, &var, &zero, &subexp));
    return var;
}

void check_zero_divisor(int lineno, operand_t *divisor)
{
    assert(divisor);
    assert(is_const_operand(divisor));
    if (divisor->val == 0) {
        translate_error(lineno, "divide zero error.");
        exit(0);
    }
}

operand_t translate_arithbop(treenode_t *lexp, treenode_t *rexp, int icop,
                             operand_t *target)
{
    assert(lexp);
    assert(rexp);

    operand_t var;
    operand_t lhs = translate_exp(lexp);
    lhs = try_deref(&lhs);
    operand_t rhs = translate_exp(rexp);
    rhs = try_deref(&rhs);

    if (is_const_operand(&lhs) && is_const_operand(&rhs)) {
        int val;
        switch (icop) {
        case ICOP_ADD:
            val = lhs.val + rhs.val; break;
        case ICOP_SUB:
            val = lhs.val - rhs.val; break;
        case ICOP_MUL:
            val = lhs.val * rhs.val; break;
        case ICOP_DIV:
            check_zero_divisor(rexp->lineno, &rhs);
            val = lhs.val / rhs.val; break;
        default:
            assert(0); break;
        }
        init_const_operand(&var, val);
        if (target) {
            intercodes_push_back(create_ic_assign(target, &var));
            return *target;
        }
        return var;
    }

    if (target) {
        intercodes_push_back(create_ic_arithbop(icop, target, &lhs, &rhs));
        return *target;
    }
    init_temp_var(&var);
    intercodes_push_back(create_ic_arithbop(icop, &var, &lhs, &rhs));
    return var;
}

operand_t translate_boolexp(treenode_t *exp, operand_t *target)
{
    int labelfalse = alloc_labelid();
    operand_t zero, one, op;
    init_const_operand(&zero, 0);
    init_const_operand(&one, 1);

    if (target)
        op = *target;
    else
        init_temp_var(&op);

    intercodes_push_back(create_ic_assign(&op, &zero));
    translate_cond(exp, LABEL_FALL, labelfalse);
    intercodes_push_back(create_ic_assign(&op, &one));
    intercodes_push_back(create_ic_label(labelfalse));
    return op;
}

operand_t translate_accessexp(treenode_t *exp)
{
    return translate_access(exp, NULL);
}

void translate_cond(treenode_t *exp, int labeltrue, int labelfalse)
{
    assert(exp);
    assert(!strcmp(exp->name, "Exp"));
    treenode_t *child = exp->child;
    assert(child);

    if (!strcmp(child->name, "LP")) {
        translate_cond(child->next, labeltrue, labelfalse);
        return;
    }
    if (!strcmp(child->name, "NOT")) {
        translate_cond_not(child->next, labeltrue, labelfalse);
        return;
    }

    treenode_t *child2, *child3;
    if ((child2 = child->next) && (child3 = child2->next)) {
        if (!strcmp(child2->name, "AND")) {
            translate_cond_and(child, child3, labeltrue, labelfalse);
            return;
        }
        if (!strcmp(child2->name, "OR")) {
            translate_cond_or(child, child3, labeltrue, labelfalse);
            return;
        }
        if (!strcmp(child2->name, "RELOP")) {
            translate_cond_relop(child, child3, labeltrue, labelfalse,
                                 str_to_icop(child2->relop));
            return;
        }
    }
    translate_cond_otherwise(exp, labeltrue, labelfalse);
}

void translate_cond_not(treenode_t *exp, int labeltrue, int labelfalse)
{
    translate_cond(exp, labelfalse, labeltrue);
}

void translate_cond_and(treenode_t *lexp, treenode_t *rexp,
                        int labeltrue, int labelfalse)
{
    int labelid = (labelfalse == LABEL_FALL ? alloc_labelid() : labelfalse);
    translate_cond(lexp, LABEL_FALL, labelid);
    translate_cond(rexp, labeltrue, labelfalse);
    if (labelfalse == LABEL_FALL)
        intercodes_push_back(create_ic_label(labelid));
}

void translate_cond_or(treenode_t *lexp, treenode_t *rexp,
                       int labeltrue, int labelfalse)
{
    int labelid = (labeltrue == LABEL_FALL ? alloc_labelid() : labeltrue);
    translate_cond(lexp, labelid, LABEL_FALL);
    translate_cond(rexp, labeltrue, labelfalse);
    if (labeltrue == LABEL_FALL)
        intercodes_push_back(create_ic_label(labelid));
}

void translate_cond_relop(treenode_t *lexp, treenode_t *rexp,
                          int labeltrue, int labelfalse, int icop)
{
    operand_t lhs = translate_exp(lexp);
    lhs = try_deref(&lhs);
    operand_t rhs = translate_exp(rexp);
    rhs = try_deref(&rhs);

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
        if (labeltrue != LABEL_FALL && labelfalse != LABEL_FALL)
            intercodes_push_back(create_ic_goto((cond? labeltrue : labelfalse)));
        else if (labeltrue != LABEL_FALL && cond)
            intercodes_push_back(create_ic_goto(labeltrue));
        else if (labelfalse != LABEL_FALL && !cond)
            intercodes_push_back(create_ic_goto(labelfalse));
        return;
    }

    if (labeltrue != LABEL_FALL && labelfalse != LABEL_FALL) {
        intercodes_push_back(create_ic_condgoto(icop, &lhs, &rhs, labeltrue));
        intercodes_push_back(create_ic_goto(labelfalse));
    }
    else if (labeltrue != LABEL_FALL) {
        intercodes_push_back(create_ic_condgoto(icop, &lhs, &rhs, labeltrue));
    }
    else if (labelfalse != LABEL_FALL) {
        icop = complement_rel_icop(icop);
        intercodes_push_back(create_ic_condgoto(icop, &lhs, &rhs, labelfalse));
    }
}

void translate_cond_otherwise(treenode_t *exp, int labeltrue, int labelfalse)
{
    operand_t op = translate_exp(exp);
    op = try_deref(&op);

    if (is_const_operand(&op)) {
        if (labeltrue != LABEL_FALL && labelfalse != LABEL_FALL)
            intercodes_push_back(create_ic_goto((op.val ? labeltrue : labelfalse)));
        else if (labeltrue != LABEL_FALL && op.val)
            intercodes_push_back(create_ic_goto(labeltrue));
        else if (labelfalse != LABEL_FALL && !op.val)
            intercodes_push_back(create_ic_goto(labelfalse));
        return;
    }

    operand_t zero;
    init_const_operand(&zero, 0);
    if (labeltrue != LABEL_FALL && labelfalse != LABEL_FALL) {
        intercodes_push_back(create_ic_condgoto(ICOP_NEQ, &op, &zero, labeltrue));
        intercodes_push_back(create_ic_goto(labelfalse));
    }
    else if (labeltrue != LABEL_FALL) {
        intercodes_push_back(create_ic_condgoto(ICOP_NEQ, &op, &zero, labeltrue));
    }
    else if (labelfalse != LABEL_FALL) {
        intercodes_push_back(create_ic_condgoto(ICOP_EQ, &op, &zero, labelfalse));
    }
}

operand_t translate_access(treenode_t *exp, type_t **ret)
{
    assert(exp);
    assert(!strcmp(exp->name, "Exp"));
    treenode_t *child = exp->child;
    assert(child);

    if (!strcmp(child->name, "ID")) {
        assert(!child->next);
        return translate_access_var(child, ret);
    }
    if (!strcmp(child->name, "LP"))
        return translate_access(child->next, ret);

    assert(!strcmp(child->name, "Exp"));
    treenode_t *child2 = child->next;
    assert(child2);
    treenode_t *child3 = child2->next;
    assert(child3);

    if (!strcmp(child2->name, "DOT"))
        return translate_access_struct(child, child3, ret);
    if (!strcmp(child2->name, "LB"))
        return translate_access_array(child, child3, ret);

    assert(0);  /* Should not reach here! */
}

operand_t translate_access_var(treenode_t *id, type_t **ret)
{
    assert(id);
    assert(id->token == ID);
    symbol_t *symbol;
    operand_t var;

    if (symbol_table_find_by_name(id->id, &symbol) != 0) {
        assert(0);
        init_const_operand(&var, 0);
        return var;
    }

    assert(symbol->type->kind == TYPE_ARRAY || symbol->type->kind == TYPE_STRUCT);
    if (ret)
        *ret = symbol->type;

    if (symbol->is_param) {
        init_addr_operand(&var, symbol->id);
        return var;
    }

    operand_t addr;
    init_temp_addr(&addr);
    init_var_operand(&var, symbol->id);
    intercodes_push_back(create_ic_ref(&addr, &var));
    return addr;
}

operand_t translate_access_array(treenode_t *exp, treenode_t *idxexp, type_t **ret)
{
    assert(exp);
    assert(idxexp);

    type_t *subtype, *elemtype;
    operand_t addr = translate_access(exp, &subtype);
    assert(subtype->kind == TYPE_ARRAY);
    elemtype = type_array_access((type_array_t *)subtype);
    assert(elemtype);
    if (ret)
        *ret = elemtype;
    operand_t idx = translate_exp(idxexp);
    idx = try_deref(&idx);

    operand_t offset, elemwidth;
    if (is_const_operand(&idx)) {
        if (idx.val == 0)
            return addr;
        init_const_operand(&offset, elemtype->width * idx.val);
    }
    else {
        init_temp_var(&offset);
        init_const_operand(&elemwidth, elemtype->width);
        intercodes_push_back(create_ic_arithbop(ICOP_MUL, &offset, &idx, &elemwidth));
    }
    operand_t newaddr;
    init_temp_addr(&newaddr);
    intercodes_push_back(create_ic_arithbop(ICOP_ADD, &newaddr, &addr, &offset));
    return newaddr;
}

operand_t translate_access_struct(treenode_t *exp, treenode_t *id, type_t **ret)
{
    assert(exp);
    assert(id);

    type_t *subtype, *fieldtype;
    int offset;

    operand_t addr = translate_access(exp, &subtype);
    assert(subtype->kind == TYPE_STRUCT);
    fieldtype = type_struct_access((type_struct_t *)subtype, id->id, &offset);
    assert(fieldtype);
    if (ret)
        *ret = fieldtype;

    if (offset == 0)
        return addr;
    operand_t offsetop, newaddr;
    init_const_operand(&offsetop, offset);
    init_temp_addr(&newaddr);
    intercodes_push_back(create_ic_arithbop(ICOP_ADD, &newaddr, &addr, &offsetop));
    return newaddr;
}

operand_t try_deref(operand_t *addr)
{
    assert(addr);
    if (addr->kind != OPERAND_ADDR)
        return *addr;

    operand_t var;
    init_temp_var(&var);
    intercodes_push_back(create_ic_dref(&var, addr));
    return var;
}
