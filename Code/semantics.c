#include "semantics.h"
#include "syntax.tab.h"
#include "semantic-data.h"

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

void semantic_analyse_r(treenode_t *node);

/* Analyse ExtDef and add symbols to the symbol table. */
void analyse_ext_def(treenode_t *ext_def);
/* Analyse Specifier and return the type infomation. */
type_t *analyse_specifier(treenode_t *specifier);
/* Analyse StructSpecifier and return the type infomation. */
type_t *analyse_struct_specifier(treenode_t *struct_specifier);
/* Analyse DefList, Def, DecList, Dec, VarDec.
 * If it is called during the struct field definition, we will
 * append the field to the 'fieldlist'. Otherwise, it must be
 * analysing local variable definitions and we will add them to
 * the symbol table. In this case, 'fieldlist' can be NULL. */
void analyse_def_list(treenode_t *def_list, fieldlist_t *fieldlist,
                      int is_during_structdef);
void analyse_def(treenode_t *def, fieldlist_t *fieldlist, int is_during_structdef);
void analyse_dec_list(treenode_t *dec_list, type_t *spec, fieldlist_t *fieldlist,
                      int is_during_structdef);
void analyse_dec(treenode_t *dec, type_t *spec, fieldlist_t *fieldlist,
                 int is_during_structdef);
void analyse_var_dec(treenode_t *var_dec, type_t *spec, fieldlist_t *fieldlist,
                     int is_during_structdef);
/* Analyse ExtDecList and add global variable to the symbol table. */
void analyse_ext_dec_list(treenode_t *ext_dec_list, type_t *spec);
/* Analyse FunDec and add function to the symbol table.
 * It will return the arguments of the function in 'fieldlist' for
 * analysing CompSt use. */
void analyse_fun_dec(treenode_t *fun_dec, fieldlist_t *fieldlist, int is_def);
/* Analyse CompSt. If 'args' is not NULL, it will add them to the
 * symbol table on entering the new scope. */
void analyse_comp_st(treenode_t *comp_st, fieldlist_t *args);

void semantic_analyse(treenode_t *root)
{
    init_structdef_table();
    init_symbol_table();
    semantic_analyse_r(root);
    print_structdef_table();
    print_symbol_table();
}

/* Recursively find ExtDef and begin our analysis. */
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
    printf("get specifier: ");
    print_type(spec);
    printf("\n");

    treenode_t *child2 = specifer->next;
    assert(child2);
    if (!strcmp(child2->name, "ExtDecList")) {
        analyse_ext_dec_list(child2, spec);
        return;
    }
    if (!strcmp(child2->name, "FunDec")) {
        assert(child2->next);
        int is_def = !strcmp(child2->next->name, "SEMI") ? 0 : 1;
        fieldlist_t fieldlist;
        init_fieldlist(&fieldlist);
        analyse_fun_dec(child2, &fieldlist, is_def);
        if (is_def)
            analyse_comp_st(child2->next, &fieldlist);
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
            analyse_def_list(child2->next->next, &fieldlist,
                             /* is_during_structdef */ 1);
        structdef = create_type_struct(id->id, &fieldlist);
        if (structdef_table_find_by_name(id->id)) {
            semantic_error(16, id->lineno, "Duplicated name \"%s\"", id->id);
            return NULL;
        }
        structdef_table_add(structdef);
        return (type_t *)structdef;
    }
    if (!strcmp(child2->name, "Tag")) {
        /* In this case, we search an already defined struct type
         * from the structdef_table */
        treenode_t *id = child2->child;
        assert(id->token == ID);
        assert(id->id);
        if (!(structdef = structdef_table_find_by_name(id->id)))
            semantic_error(17, child2->lineno, "Undefined structure \"%s\"",
                           id->id);
        return (type_t *)structdef;     /* can be NULL */
    }
    assert(!strcmp(child2->name, "LC"));
    /* In this case, we create an anonymous struct type
     * and add it to structdef_table. */
    fieldlist_t fieldlist;
    init_fieldlist(&fieldlist);
    if (strcmp(child2->next->name, "RC"))
        analyse_def_list(child2->next, &fieldlist,
                         /* is_during_structdef */ 1);
    structdef = create_type_struct(NULL, &fieldlist);
    structdef_table_add(structdef);
    return (type_t *)structdef;
}

void analyse_def_list(treenode_t *def_list, fieldlist_t *fieldlist,
                      int is_during_structdef)
{
    assert(def_list);
    assert(!strcmp(def_list->name, "DefList"));
    treenode_t *def = def_list->child;
    assert(def);

    analyse_def(def, fieldlist, is_during_structdef);
    if (def->next)
        analyse_def_list(def->next, fieldlist, is_during_structdef);
}

void analyse_def(treenode_t *def, fieldlist_t *fieldlist,
                 int is_during_structdef)
{
    assert(def);
    assert(!strcmp(def->name, "Def"));
    treenode_t *specifier = def->child;
    assert(specifier);

    type_t *spec = analyse_specifier(specifier);
    if (!spec)
        return;
    analyse_dec_list(specifier->next, spec, fieldlist, is_during_structdef);
}

void analyse_dec_list(treenode_t *dec_list, type_t *spec, fieldlist_t *fieldlist,
                      int is_during_structdef)
{
    assert(dec_list);
    assert(!strcmp(dec_list->name, "DecList"));
    treenode_t *dec = dec_list->child;
    assert(dec);

    analyse_dec(dec, spec, fieldlist, is_during_structdef);
    if (dec->next)
        analyse_dec_list(dec->next->next, spec, fieldlist, is_during_structdef);
}

void analyse_dec(treenode_t *dec, type_t *spec, fieldlist_t *fieldlist,
                 int is_during_structdef)
{
    assert(dec);
    assert(!strcmp(dec->name, "Dec"));
    treenode_t *var_dec = dec->child;
    assert(var_dec);
    analyse_var_dec(var_dec, spec, fieldlist, is_during_structdef);

    treenode_t *assignop = var_dec->next;
    if (assignop) {
        assert(!strcmp(assignop->name, "ASSIGNOP"));
        if (is_during_structdef) {
            semantic_error(15, assignop->lineno,
                           "Field assigned during definition");
        }
        else {
            /* TODO: Do something in the future. */
        }
    }
}

void analyse_var_dec(treenode_t *var_dec, type_t *spec, fieldlist_t *fieldlist,
                     int is_during_structdef)
{
    assert(var_dec);
    assert(!strcmp(var_dec->name, "VarDec"));
    treenode_t *child = var_dec->child;
    assert(child);

    if (!strcmp(child->name, "ID")) {
        assert(child->id);
        if (is_during_structdef) {
            if (fieldlist_find_type_by_fieldname(fieldlist, child->id))
                semantic_error(15, child->lineno, "Redefined field \"%s\"",
                               child->id);
            else
                fieldlist_push_back(fieldlist, child->id, spec);
        }
        else {
            if (symbol_table_find_by_name(child->id, NULL) == 0
                || structdef_table_find_by_name(child->id))
                semantic_error(3, child->lineno, "Redefined variable \"%s\"",
                               child->id);
            else {
                symbol_t symbol;
                init_symbol(&symbol, spec, child->id, child->lineno, 1);
                symbol_table_add(&symbol);
            }
        }
        return;
    }
    assert(child->next);
    treenode_t *intnode = child->next->next;
    assert(intnode);
    assert(intnode->token == INT);
    type_array_t *type_array = create_type_array(intnode->ival, spec);
    analyse_var_dec(child, (type_t *)type_array, fieldlist, is_during_structdef);
}

void analyse_ext_dec_list(treenode_t *ext_dec_list, type_t *spec)
{
    assert(ext_dec_list);
    assert(!strcmp(ext_dec_list->name, "ExtDecList"));
    treenode_t *var_dec = ext_dec_list->child;
    assert(var_dec);

    analyse_var_dec(var_dec, spec, NULL, /* is_during_structdef */ 0);
    if (var_dec->next)
        analyse_ext_dec_list(var_dec->next->next, spec);
}

void analyse_fun_dec(treenode_t *fun_dec, fieldlist_t *fieldlist, int is_def)
{
    assert(fun_dec);
    assert(!strcmp(fun_dec->name, "FunDec"));
}

void analyse_comp_st(treenode_t *comp_st, fieldlist_t *args)
{
    // TODO:
}