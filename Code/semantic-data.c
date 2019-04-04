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
    struct stnode *prev;
    struct stnode *next;
    struct stnode *sibling;
} stnode_t;

stnode_t *create_stnode(symbol_t *symbol);
symbol_t *destroy_stnode(stnode_t *stnode);

/* environment stack */
typedef struct envnode {
    stnode_t *symbol_head;
    struct envnode *before;
} envnode_t;

envnode_t *create_envnode(stnode_t *symbol_head);
stnode_t *destroy_envnode(envnode_t *envnode);

typedef struct envstack {
    envnode_t *top;
} envstack_t;

void init_envstack(envstack_t *envstack);
void envstack_pushenv(envstack_t *envstack);
stnode_t *envstack_popenv(envstack_t *envstack);
void envstack_add(envstack_t *envstack, stnode_t *stnode);

/* hash table */
#define HT_SIZE 2333

typedef struct hashtable {
    stnode_t *slots[HT_SIZE];
} hashtable_t;

void init_hashtable(hashtable_t *hashtable);
void hashtable_add(hashtable_t *hashtable, stnode_t *stnode);
unsigned int hash_str(const char *str);
unsigned int hash_symbol(symbol_t *symbol);

/* symbol table */
struct {
    envstack_t envstack;
    hashtable_t hashtable;
} symbol_table;

/* implementations */
stnode_t *create_stnode(symbol_t *symbol)
{
    stnode_t *newnode = malloc(sizeof(stnode_t));
    if (newnode)
        newnode->symbol = symbol;
    return newnode;
}

symbol_t *destroy_stnode(stnode_t *stnode)
{
    symbol_t *ret = NULL;
    if (stnode)
        ret = stnode->symbol;
    free(stnode);
    return ret;
}

envnode_t *create_envnode(stnode_t *symbol_head)
{
    envnode_t *newnode = malloc(sizeof(envnode_t));
    if (newnode) {
        newnode->symbol_head = symbol_head;
        newnode->before = NULL;
    }
    return newnode;
}

stnode_t *destroy_envnode(envnode_t *envnode)
{
    stnode_t *ret = NULL;
    if (envnode)
        ret = envnode->symbol_head;
    free(envnode);
    return ret;
}

void init_envstack(envstack_t *envstack)
{
    assert(envstack);
    envnode_t *envnode = create_envnode(NULL);
    assert(envnode);
    envstack->top = envnode;
}

void envstack_pushenv(envstack_t *envstack)
{
    assert(envstack);
    envnode_t *envnode = create_envnode(NULL);
    assert(envnode);

    envnode->before = envstack->top;
    envstack->top = envnode;
}

stnode_t *envstack_popenv(envstack_t *envstack)
{
    assert(envstack);
    assert(envstack->top);
    envnode_t *topenv = envstack->top;
    envstack->top = topenv->before;

    return destroy_envnode(topenv);
}

void envstack_add(envstack_t *envstack, stnode_t *stnode)
{
    assert(envstack);
    assert(envstack->top);
    envnode_t *top = envstack->top;
    stnode->sibling = top->symbol_head;
    top->symbol_head = stnode;
}

void init_hashtable(hashtable_t *hashtable)
{
    memset(hashtable->slots, 0, sizeof(hashtable->slots));
}

void hashtable_add(hashtable_t *hashtable, stnode_t *stnode)
{
    unsigned int index = hash_symbol(stnode->symbol) % HT_SIZE;
    stnode_t *head = hashtable->slots[index];
    stnode->next = head;
    if (head)
        head->prev = stnode;
    hashtable->slots[index] = stnode;
    stnode->prev = NULL;
}

void hashtable_delete(hashtable_t *hashtable, stnode_t *stnode)
{
    unsigned int index = hash_symbol(stnode->symbol) % HT_SIZE;
    if (stnode->prev)
        stnode->prev->next = stnode->next;
    else
        hashtable->slots[index] = stnode->next;
    if (stnode->next)
        stnode->next->prev = stnode->prev;
}

unsigned int hash_str(const char *str)
{
    unsigned int val = 0, i;
    for (char *p = str; *p; ++p) {
        val = (val << 2) + *p;
        if (i = val & ~0x3fff)
            val = (val ^ (i >> 12)) & 0x3fff;
    }
    return val;
}

unsigned int hash_symbol(symbol_t *symbol)
{
    return hash_str(symbol->name);
}

void init_symbol_table()
{
    init_envstack(&symbol_table.envstack);
    init_hashtable(&symbol_table.hashtable);
}

void symbol_table_add(symbol_t *symbol)
{
    stnode_t *stnode = create_stnode(symbol);
    envstack_add(&symbol_table.envstack, stnode);
    hashtable_add(&symbol_table.hashtable, stnode);
}

void symbol_table_pushenv()
{
    envstack_pushenv(&symbol_table.envstack);
}

void symbol_table_popenv()
{
    stnode_t *head = envstack_popenv(&symbol_table.envstack);

    while (head) {
        stnode_t *next = head->next;
        hashtable_delete(&symbol_table.hashtable, head);
        symbol_t *symbol = destroy_stnode(head); // FIXME: memory leak.
        head = next;
    }
    // TODO:
}

void print_symbol_table()
{
    // TODO:
}
