#define _POSIX_C_SOURCE 200809L

#include "type-system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------ *
 *             basic type               *
 * ------------------------------------ */

static const char *typename_table[] ={
    [TYPE_INT] = "int",
    [TYPE_FLOAT] = "float"
};

#define TYPENAME_TABLE_SIZE sizeof(typename_table) / sizeof(typename_table[0])

int typename_to_id(const char *type_name)
{
    for (int i = 0; i < TYPENAME_TABLE_SIZE; ++i)
        if (typename_table[i] && !strcmp(typename_table[i], type_name))
            return i;
    return -1;
}

const char *typeid_to_name(int id)
{
    return typename_table[id];
}

type_basic_t *create_type_basic(int type_id)
{
    type_basic_t *tb = malloc(sizeof(type_basic_t));
    assert(tb);
    tb->kind = TYPE_BASIC;
    tb->type_id = type_id;
    return tb;
}

/* ------------------------------------ *
 *              fieldlist               *
 * ------------------------------------ */

void init_fieldlist(fieldlist_t *fieldlist)
{
    fieldlist->front = NULL;
}

void fieldlist_push_back(fieldlist_t *fieldlist,
                         const char *fieldname, type_t *type)
{
    assert(fieldlist);
    fieldlistnode_t *newnode = malloc(sizeof(fieldlistnode_t));
    assert(newnode);
    newnode->fieldname = strdup(fieldname);
    newnode->type = type;
    newnode->next = NULL;

    if (fieldlist->size == 0)
        fieldlist->front = newnode;
    else
        fieldlist->back->next = newnode;
    fieldlist->back = newnode;
    fieldlist->size++;
}

void fieldlist_concat(fieldlist_t *lhs, fieldlist_t *rhs)
{
    assert(lhs);
    assert(rhs);
    if (rhs->size == 0)
        return;

    if (lhs->size == 0) {
        *lhs = *rhs;
    }
    else {
        lhs->back->next = rhs->front;
        lhs->back = rhs->back;
    }
}

/* ------------------------------------ *
 *             struct type              *
 * ------------------------------------ */

type_struct_t *create_type_struct(const char *structname, fieldlist_t *fields)
{
    type_struct_t *ts = malloc(sizeof(type_struct_t));
    assert(ts);
    ts->kind = TYPE_STRUCT;
    ts->structname = NULL;
    init_fieldlist(&ts->fields);
    if (structname)
        ts->structname = strdup(structname);
    if (fields)
        ts->fields = *fields;
    return ts;
}

/* ------------------------------------ *
 *              typelist                *
 * ------------------------------------ */

void init_typelist(typelist_t *typelist)
{
    assert(typelist);
    typelist->size = 0;
    typelist->front = typelist->back = NULL;
}

void typelist_push_back(typelist_t *typelist, type_t *type)
{
    assert(type);
    typelistnode_t *newnode = malloc(sizeof(typelistnode_t));
    assert(newnode);
    newnode->type = type;
    newnode->next = NULL;

    if (typelist->size == 0)
        typelist->front = newnode;
    else
        typelist->back->next = newnode;
    typelist->back = newnode;
    typelist->size++;
}

type_t *typelist_find_type_struct_by_name(typelist_t *typelist, const char *name)
{
    assert(typelist);
    if (!name)
        return NULL;

    for (typelistnode_t *cur = typelist->front;
         cur != typelist->back; cur = cur->next) {
        assert(cur->type);
        if (cur->type->kind == TYPE_STRUCT) {
            type_struct_t *ts = cur->type;
            if (ts->structname && !strcmp(ts->structname, name))
                return cur->type;
        }
    }
    return NULL;
}