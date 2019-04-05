#define _POSIX_C_SOURCE 200809L

#include "type-system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------ *
 *            abstract type             *
 * ------------------------------------ */

int type_is_equal(type_t *lhs, type_t *rhs)
{
    assert(lhs);
    assert(rhs);
    if (lhs->kind != rhs->kind)
        return 0;
    switch (lhs->kind) {
    case TYPE_BASIC:
        return type_basic_is_equal((type_basic_t *)lhs, (type_basic_t *)rhs);
    case TYPE_ARRAY:
        return type_array_is_equal((type_array_t *)lhs, (type_array_t *)rhs);
    case TYPE_STRUCT:
        return type_struct_is_equal((type_struct_t *)lhs, (type_struct_t *)rhs);
    case TYPE_FUNC:
        return type_func_is_equal((type_func_t *)lhs, (type_func_t *)rhs);
    default:
        assert(0);
    }
    return 0;
}

void print_type(type_t *type)
{
    assert(type);
    switch (type->kind) {
    case TYPE_BASIC: print_type_basic((type_basic_t *)type); break;
    case TYPE_ARRAY: print_type_array((type_array_t *)type); break;
    case TYPE_STRUCT: print_type_struct((type_struct_t *)type); break;
    case TYPE_FUNC: print_type_func((type_func_t *)type); break;
    default: assert(0); break;
    }
}

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

int type_basic_is_equal(type_basic_t *lhs, type_basic_t *rhs)
{
    assert(lhs);
    assert(rhs);
    return lhs->type_id == rhs->type_id;
}

void print_type_basic(type_basic_t *tb)
{
    assert(tb);
    switch (tb->type_id) {
        case TYPE_INT: printf("int"); break;
        case TYPE_FLOAT: printf("float"); break;
        default: assert(0); break;
    }
}

/* ------------------------------------ *
 *             array type               *
 * ------------------------------------ */

type_array_t *create_type_array(int size, type_t *extend_from)
{
    type_array_t *ta = malloc(sizeof(type_array_t));
    assert(ta);
    ta->kind = TYPE_ARRAY;
    ta->size = size;
    ta->extend_from = extend_from;
    return ta;
}

int type_array_is_equal(type_array_t *lhs, type_array_t *rhs)
{
    assert(lhs);
    assert(rhs);
    if (lhs->size != rhs->size)
        return 0;
    return type_is_equal(lhs->extend_from, rhs->extend_from);
}

void print_type_array(type_array_t *ta)
{
    assert(ta);
    printf("[%d]", ta->size);
    print_type(ta->extend_from);
}

/* ------------------------------------ *
 *              fieldlist               *
 * ------------------------------------ */

void init_fieldlist(fieldlist_t *fieldlist)
{
    fieldlist->size = 0;
    fieldlist->front = fieldlist->back = NULL;
}

fieldlistnode_t *create_fieldlistnode(const char *fieldname, type_t *type)
{
    fieldlistnode_t *newnode = malloc(sizeof(fieldlistnode_t));
    if (newnode){
        newnode->fieldname = fieldname;
        newnode->type = type;
        newnode->next = NULL;
    }
    return newnode;
}

void fieldlist_push_back(fieldlist_t *fieldlist,
                         const char *fieldname, type_t *type)
{
    assert(fieldlist);
    fieldlistnode_t *newnode = create_fieldlistnode(fieldname, type);
    assert(newnode);

    if (fieldlist->size == 0)
        fieldlist->front = newnode;
    else
        fieldlist->back->next = newnode;
    fieldlist->back = newnode;
    fieldlist->size++;
}

void fieldlist_push_front(fieldlist_t *fieldlist,
                          const char *fieldname, type_t *type)
{
    assert(fieldlist);
    fieldlistnode_t *newnode = create_fieldlistnode(fieldname, type);
    assert(newnode);

    if (fieldlist->size == 0)
        fieldlist->back = newnode;
    else
        newnode->next = fieldlist->front;
    fieldlist->front = newnode;
    fieldlist->size++;
}

type_t *fieldlist_find_type_by_fieldname(fieldlist_t *fieldlist,
                                         const char *fieldname)
{
    for (fieldlistnode_t *cur = fieldlist->front; cur != NULL; cur = cur->next)
        if (!strcmp(cur->fieldname, fieldname))
            return cur->type;
    return NULL;
}

int fieldlist_is_equal(fieldlist_t *lhs, fieldlist_t *rhs)
{
    if (lhs->size != rhs->size)
        return 0;
    fieldlistnode_t *lcur = lhs->front, *rcur = rhs->front;
    while (lcur) {
        assert(rcur);
        if (!type_is_equal(lcur->type, rcur->type))
            return 0;
        lcur = lcur->next;
        rcur = rcur->next;
    }
    return 1;
}

void print_fieldlist(fieldlist_t *fieldlist)
{
    for (fieldlistnode_t *cur = fieldlist->front; cur != NULL; cur = cur->next) {
        printf("%s: ", cur->fieldname);
        print_type(cur->type);
        printf(";");
        if (cur->next)
            printf(" ");
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
        ts->structname = structname;
    if (fields)
        ts->fields = *fields;
    return ts;
}

int type_struct_is_equal(type_struct_t *lhs, type_struct_t *rhs)
{
    return fieldlist_is_equal(&lhs->fields, &rhs->fields);
}

void print_type_struct(type_struct_t *ts)
{
    assert(ts);
    printf("struct");
    if (ts->structname)
        printf(" %s", ts->structname);
    printf(" { ");
    print_fieldlist(&ts->fields);
    printf(" }");
}

/* ------------------------------------ *
 *              typelist                *
 * ------------------------------------ */

typelistnode_t *create_typelistnode(type_t *type)
{
    typelistnode_t *newnode = malloc(sizeof(typelistnode_t));
    if (newnode) {
        newnode->type = type;
        newnode->next = NULL;
    }
    return newnode;
}

void init_typelist(typelist_t *typelist)
{
    assert(typelist);
    typelist->size = 0;
    typelist->front = typelist->back = NULL;
}

void typelist_push_back(typelist_t *typelist, type_t *type)
{
    assert(type);
    typelistnode_t *newnode = create_typelistnode(type);
    assert(newnode);

    if (typelist->size == 0)
        typelist->front = newnode;
    else
        typelist->back->next = newnode;
    typelist->back = newnode;
    typelist->size++;
}

type_struct_t *typelist_find_type_struct_by_name(typelist_t *typelist,
                                                 const char *name)
{
    assert(typelist);
    if (!name)
        return NULL;

    for (typelistnode_t *cur = typelist->front; cur != NULL; cur = cur->next) {
        assert(cur->type);
        if (cur->type->kind == TYPE_STRUCT) {
            type_struct_t *ts = (type_struct_t *)cur->type;
            if (ts->structname && !strcmp(ts->structname, name))
                return ts;
        }
    }
    return NULL;
}

int typelist_is_equal(typelist_t *lhs, typelist_t *rhs)
{
    if (lhs->size != rhs->size)
        return 0;
    typelistnode_t *lcur = lhs->front, *rcur = rhs->front;
    while (lcur) {
        assert(rcur);
        if (!type_is_equal(lcur->type, rcur->type))
            return 0;
        lcur = lcur->next;
        rcur = rcur->next;
    }
    return 1;
}

void print_typelist(typelist_t *typelist)
{
    for (typelistnode_t *cur = typelist->front; cur != NULL; cur = cur->next) {
        print_type(cur->type);
        if (cur->next)
            printf(", ");
    }
}

/* ------------------------------------ *
 *              func type               *
 * ------------------------------------ */

type_func_t *create_type_func(type_t *ret_type, typelist_t *types)
{
    type_func_t *tf = malloc(sizeof(type_func_t));
    assert(tf);
    tf->kind = TYPE_FUNC;
    tf->ret_type = ret_type;
    init_typelist(&tf->types);
    if (types)
        tf->types = *types;
    return tf;
}

void type_func_add_params_from_fieldlist(type_func_t *type_func,
                                         fieldlist_t *fieldlist)
{
    for (fieldlistnode_t *cur = fieldlist->front; cur != NULL; cur = cur->next) {
        assert(cur->type);
        typelist_push_back(&type_func->types, cur->type);
    }
}

int type_func_is_equal(type_func_t *lhs, type_func_t *rhs)
{
    return type_is_equal(lhs->ret_type, rhs->ret_type)
        && typelist_is_equal(&lhs->types, &rhs->types);
}

void print_type_func(type_func_t *tf)
{
    print_type(tf->ret_type);
    printf(" (");
    print_typelist(&tf->types);
    printf(")");
}