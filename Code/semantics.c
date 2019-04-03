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
 *           semantic_analyse           *
 * ------------------------------------ */

void semantic_analyse_r(treenode_t *node, treenode_t *parent);
type_t *analyse_specifier(treenode_t *specifier);
type_t *analyse_struct_specifier(treenode_t *struct_specifier);
fieldlist_t analyse_def_list(treenode_t *def_list);
void analyse_def(treenode_t *def, fieldlist_t *fieldlist);

void semantic_analyse(treenode_t *root)
{
    semantic_analyse_r(root, NULL);
}

void semantic_analyse_r(treenode_t *node, treenode_t *parent)
{
    if (!node)
        return;

    if (!strcmp(node->name, "Specifier")) {
        printf("analyse_specifier\n");
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
    assert(specifier);
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
    assert(struct_specifier);
    assert(struct_specifier->child);
    treenode_t *child2 = struct_specifier->child->next;
    assert(child2);
    type_struct_t *structdef = NULL;

    if (!strcmp(child2->name, "OptTag")) {
        /* In this case, we create a named struct type
         * and add it to structdef_table. */
        treenode_t *id = child2->child;
        treenode_t *def_list = child2->next->next;
        assert(id->token == ID);
        assert(id->id);
        fieldlist_t fieldlist = analyse_def_list(def_list);
        structdef = create_type_struct(id->id, &fieldlist);
        if (structdef_table_find_by_name(structdef->structname))
            semantic_error(16, id->lineno, "Duplicated name \"%s\"",
                           structdef->structname);
        structdef_table_add(structdef);
        return (type_t *)structdef;
    }
    if (!strcmp(child2->name, "Tag")) {
        /* In this case, we search an already defined struct type
         * from the structdef_table */
        treenode_t *id = child2->child;
        assert(id->token == ID);
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
        fieldlist_t fieldlist = analyse_def_list(child2->next);
        structdef = create_type_struct(NULL, &fieldlist);
    }
    structdef_table_add(structdef);
    return (type_t *)structdef;
}

fieldlist_t analyse_def_list(treenode_t *def_list)
{

}

void analyse_def(treenode_t *def, fieldlist_t *fieldlist)
{

}

void analyse_dec_list(treenode_t *dec_list, type_t *spec, fieldlist_t *fieldlist)
{

}

void analyse_dec(treenode_t *dec, type_t *spec, fieldlist_t *fieldlist)
{

}

type_t *analyse_var_dec(treenode_t *var_dec, type_t *spec)
{

}