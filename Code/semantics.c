#include "semantics.h"
#include "syntax.tab.h"

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
 *  the table of structure definitions  *
 * ------------------------------------ */

static typelist_t structdef_table;

void init_structdef_table()
{
    init_typelist(&structdef_table);
}

void structdef_table_add(type_struct_t *structdef)
{
    typelist_push_back(&structdef_table, (type_t *)structdef);
}

type_struct_t *structdef_table_find_by_name(const char *structname)
{
    return (type_struct_t *)typelist_find_type_struct_by_name(
        &structdef_table, structname);
}

/* ------------------------------------ *
 *             symbol table             *
 * ------------------------------------ */


/* ------------------------------------ *
 *           semantic_analyse           *
 * ------------------------------------ */

void semantic_analyse_r(treenode_t *node, treenode_t *parent);
type_t *analyse_specifier(treenode_t *specifier);
type_t *analyse_struct_specifier(treenode_t *struct_specifier);
void analyse_def_list(treenode_t *def_list, fieldlist_t *fieldlist,
                      int is_during_structdef);
void analyse_def(treenode_t *def, fieldlist_t *fieldlist,
                 int is_during_structdef);
void analyse_dec_list(treenode_t *dec_list, type_t *spec, fieldlist_t *fieldlist,
                      int is_during_structdef);
void analyse_dec(treenode_t *dec, type_t *spec, fieldlist_t *fieldlist,
                 int is_during_structdef);
void analyse_var_dec(treenode_t *var_dec, type_t *spec, fieldlist_t *fieldlist,
                     int is_during_structdef);

void semantic_analyse(treenode_t *root)
{
    semantic_analyse_r(root, NULL);
}

void semantic_analyse_r(treenode_t *node, treenode_t *parent)
{
    if (!node)
        return;

    if (!strcmp(node->name, "Specifier")) {
        type_t *ret = analyse_specifier(node);
        return;
    }
    // TODO: handle other TOKEN.

    for (treenode_t *child = node->child; child != NULL; child = child->next) {
        semantic_analyse_r(child, node);
    }
}

type_t *analyse_specifier(treenode_t *specifier)
{
    printf("analyse_specifier\n");
    assert(specifier);
    assert(!strcmp(specifier->name, "Specifier"));
    treenode_t *child = specifier->child;
    assert(child);

    if (!strcmp(child->name, "TYPE")) {
        printf("get TYPE\n");
        return (type_t *)create_type_basic(child->type_id);
    }
    if (!strcmp(child->name, "StructSpecifier")) {
        printf("get StructSpecifier\n");
        return analyse_struct_specifier(child);
    }

    assert(0);  /* Should not reach here! */
    return NULL;
}

type_t *analyse_struct_specifier(treenode_t *struct_specifier)
{
    printf("analyse_struct_specifier\n");
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
        if (structdef_table_find_by_name(id->id)) {
            semantic_error(16, id->lineno, "Duplicated name \"%s\"", id->id);
            return NULL;
        }
        if (!strcmp(child2->next->next->name, "RC")) {
            structdef = create_type_struct(id->id, NULL);
        }
        else {
            fieldlist_t fieldlist;
            init_fieldlist(&fieldlist);
            analyse_def_list(child2->next->next, &fieldlist,
                             /* is_during_structdef */ 1);
            structdef = create_type_struct(id->id, &fieldlist);
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
    if (!strcmp(child2->next->name, "RC")) {
        structdef = create_type_struct(NULL, NULL);
    }
    else {
        fieldlist_t fieldlist;
        init_fieldlist(&fieldlist);
        analyse_def_list(child2->next, &fieldlist, /* is_during_structdef */ 1);
        structdef = create_type_struct(NULL, &fieldlist);
    }
    structdef_table_add(structdef);
    return (type_t *)structdef;
}

void analyse_def_list(treenode_t *def_list, fieldlist_t *fieldlist,
                      int is_during_structdef)
{
    printf("analyse_def_list\n");
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
    printf("analyse_def\n");
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
    printf("analyse_dec_list\n");
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
    printf("analyse_dec\n");
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
            /* Do something in future. */
        }
    }
}

void analyse_var_dec(treenode_t *var_dec, type_t *spec, fieldlist_t *fieldlist,
                     int is_during_structdef)
{
    printf("analyse_var_dec\n");
    assert(var_dec);
    assert(!strcmp(var_dec->name, "VarDec"));
    treenode_t *child = var_dec->child;
    assert(child);

    if (!strcmp(child->name, "ID")) {
        fieldlist_push_back(fieldlist, child->id, spec);
        return;
    }
    assert(child->next);
    treenode_t *intnode = child->next->next;
    assert(intnode);
    assert(intnode->token == INT);
    type_array_t *type_array = create_type_array(intnode->ival, spec);
    analyse_var_dec(child, (type_t *)type_array, fieldlist, is_during_structdef);
}