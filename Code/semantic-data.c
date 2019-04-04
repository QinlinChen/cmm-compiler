#include "semantic-data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

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
    return typelist_find_type_struct_by_name(&structdef_table, structname);
}

void print_structdef_table()
{
    print_typelist(&structdef_table);
    printf("\n");
}

/* ------------------------------------ *
 *             symbol table             *
 * ------------------------------------ */

/* symbol table node */
typedef struct stnode {
    symbol_t *symbol;
    struct stnode *before;
    struct stnode *next;
    struct stnode *sibling;
} stnode_t;

stnode_t *create_stnode(symbol_t *symbol);

/* environment stack */
typedef struct envnode {
    stnode_t *symbol_head;
    struct envnode *before;
} envnode_t;

envnode_t *create_envnode();

typedef struct envstack {
    envnode_t *top;
} envstack_t;

void init_envstack(envstack_t *envstack);
void envstack_pushenv(envstack_t *envstack);
void envstack_popenv(envstack_t *envstack);
void envstack_add(envstack_t *envstack, stnode_t *stnode);

/* hash table */
#define HT_BUCKET_SIZE 100

typedef struct hashtable {
    stnode_t *buckets[HT_BUCKET_SIZE];
} hashtable_t;

void init_hashtable(hashtable_t *hashtable);
void hashtable_add(hashtable_t *hashtable, stnode_t *stnode);

/* symbol table */
struct {
    envstack_t envstack;
    hashtable_t hashtable;
} symbol_table;

stnode_t *create_stnode(symbol_t *symbol)
{
    stnode_t *newnode = malloc(sizeof(stnode_t));
    if (newnode)
        newnode->symbol = symbol;
    return newnode;
}

envnode_t *create_envnode()
{
    envnode_t *newnode = malloc(sizeof(envnode_t));
    if (newnode) {
        newnode->symbol_head = NULL;
        newnode->before = NULL;
    }
    return newnode;
}

void init_envstack(envstack_t *envstack)
{
    assert(envstack);
    envnode_t *envnode = create_envnode();
    assert(envnode);
    envstack->top = envnode;
}

void envstack_pushenv(envstack_t *envstack)
{
    // TODO:
}

void envstack_popenv(envstack_t *envstack)
{
    // TODO:
}

void init_hashtable(hashtable_t *hashtable)
{
    memset(hashtable->buckets, 0, sizeof(hashtable->buckets));
}

void hashtable_add(hashtable_t *hashtable, stnode_t *stnode)
{
    // TODO:
}

void init_symbol_table()
{
    init_envstack(&symbol_table.envstack);
    init_hashtable(&symbol_table.hashtable);
}

void symbol_table_add(symbol_t *symbol)
{
    // TODO:
}
