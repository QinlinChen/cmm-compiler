#include "semantic-data.h"
#include "intercode.h"

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
    printf("structdef table:\n");
    print_typelist(&structdef_table);
    printf("\n");
}

/* ------------------------------------ *
 *             symbol table             *
 * ------------------------------------ */

/* symbol table node */
typedef struct stnode {
    symbol_t symbol;
    struct stnode *prev;
    struct stnode *next;
    struct stnode *sibling;
} stnode_t;

stnode_t *create_stnode(symbol_t *symbol);
void destroy_stnode(stnode_t *stnode, symbol_t *ret);

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
stnode_t *envstack_find_by_name_in_top(envstack_t *envstack, const char *name);
void envstack_traverse(envstack_t *envstack, void (*handle)(stnode_t *node));
void print_envstack(envstack_t *envstack);

/* hash table */
#define HT_SIZE 2333

typedef struct hashtable {
    stnode_t *slots[HT_SIZE];
} hashtable_t;

unsigned int hash_str(const char *str);
unsigned int hash_symbol(symbol_t *symbol);
void init_hashtable(hashtable_t *hashtable);
stnode_t **hashtable_get_slot(hashtable_t *hashtable, const char *name);
void hashtable_add(hashtable_t *hashtable, stnode_t *stnode);
void hashtable_delete(hashtable_t *hashtable, stnode_t *stnode);
stnode_t *hashtable_find_by_name(hashtable_t *hashtable, const char *name);
void print_hashtable(hashtable_t *hashtable);

/* symbol table */
struct {
    envstack_t envstack;
    hashtable_t hashtable;
} symbol_table;

stnode_t *create_stnode(symbol_t *symbol)
{
    stnode_t *newnode = calloc(1, sizeof(stnode_t));
    if (newnode)
        newnode->symbol = *symbol;
    return newnode;
}

void destroy_stnode(stnode_t *stnode, symbol_t *ret)
{
    if (stnode && ret)
        *ret = stnode->symbol;
    free(stnode);
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
    envstack->top = NULL;
    envstack_pushenv(envstack);
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

stnode_t *envstack_find_by_name_in_top(envstack_t *envstack, const char *name)
{
    assert(envstack);
    for (stnode_t *node = envstack->top->symbol_head;
         node != NULL; node = node->sibling)
        if (!strcmp(node->symbol.name, name))
            return node;
    return NULL;
}

void envstack_traverse(envstack_t *envstack, void (*handle)(stnode_t *node))
{
    assert(envstack);
    for (envnode_t *env = envstack->top; env != NULL; env = env->before)
        for (stnode_t *node = env->symbol_head; node != NULL; node = node->sibling)
            handle(node);
}

void print_envstack(envstack_t *envstack)
{
    assert(envstack);
    for (envnode_t *env = envstack->top; env != NULL; env = env->before) {
        for (stnode_t *node = env->symbol_head; node != NULL; node = node->sibling) {
            print_symbol(&node->symbol);
            printf(";");
            if (node->sibling)
                printf(" ");
        }
        printf("\n");
    }
}

unsigned int hash_str(const char *str)
{
    unsigned int val = 0, i;
    for (const char *p = str; *p; ++p) {
        val = (val << 2) + *p;
        if ((i = val & ~0x3fff))
            val = (val ^ (i >> 12)) & 0x3fff;
    }
    return val;
}

void init_hashtable(hashtable_t *hashtable)
{
    assert(hashtable);
    memset(hashtable->slots, 0, sizeof(hashtable->slots));
}

stnode_t **hashtable_get_slot(hashtable_t *hashtable, const char *name)
{
    assert(hashtable);
    unsigned int index = hash_str(name) % HT_SIZE;
    return &(hashtable->slots[index]);
}

void hashtable_add(hashtable_t *hashtable, stnode_t *stnode)
{
    assert(hashtable);
    assert(stnode);
    stnode_t **phead = hashtable_get_slot(hashtable, stnode->symbol.name);
    stnode->next = *phead;
    if (*phead)
        (*phead)->prev = stnode;
    *phead = stnode;
    stnode->prev = NULL;
}

void hashtable_delete(hashtable_t *hashtable, stnode_t *stnode)
{
    assert(hashtable);
    assert(stnode);
    stnode_t **phead = hashtable_get_slot(hashtable, stnode->symbol.name);
    if (stnode->prev)
        stnode->prev->next = stnode->next;
    else
        *phead = stnode->next;
    if (stnode->next)
        stnode->next->prev = stnode->prev;
}

stnode_t *hashtable_find_by_name(hashtable_t *hashtable, const char *name)
{
    stnode_t **phead = hashtable_get_slot(hashtable, name);
    for (stnode_t *cur = *phead; cur != NULL; cur = cur->next)
        if (!strcmp(cur->symbol.name, name))
            return cur;
    return NULL;
}

void print_hashtable(hashtable_t *hashtable)
{
    assert(hashtable);
    for (int i = 0; i < HT_SIZE; ++i) {
        for (stnode_t *node = hashtable->slots[i]; node != NULL; node = node->next) {
            print_symbol(&node->symbol);
            printf(";");
            if (node->next)
                printf(" ");
            else
                printf("\n");
        }
    }
}

void init_symbol(symbol_t *symbol, type_t *type, const char *name,
                 int lineno, int is_defined)
{
    symbol->type = type;
    symbol->name = name;
    symbol->lineno = lineno;
    symbol->is_defined = is_defined;
}

void symbol_set_defined(symbol_t *symbol, int is_defined)
{
    assert(symbol);
    symbol->is_defined = is_defined;
}

void print_symbol(symbol_t *symbol)
{
    printf("%s: ", symbol->name);
    print_type(symbol->type);
}

void init_symbol_table()
{
    init_envstack(&symbol_table.envstack);
    init_hashtable(&symbol_table.hashtable);
}

void symbol_table_add(symbol_t *symbol)
{
    symbol->id = alloc_varid();
    stnode_t *stnode = create_stnode(symbol);
    envstack_add(&symbol_table.envstack, stnode);
    hashtable_add(&symbol_table.hashtable, stnode);
}

void symbol_table_add_from_fieldlist(fieldlist_t *fieldlist)
{
    for (fieldlistnode_t *cur = fieldlist->front; cur != NULL; cur = cur->next) {
        symbol_t symbol;
        init_symbol(&symbol, cur->type, cur->fieldname, -1, 1);
        symbol_table_add(&symbol);
    }
}

void symbol_table_pushenv()
{
    envstack_pushenv(&symbol_table.envstack);
}

void symbol_table_popenv()
{
    stnode_t *head = envstack_popenv(&symbol_table.envstack);

    while (head) {
        stnode_t *sibling = head->sibling;
        hashtable_delete(&symbol_table.hashtable, head);
        destroy_stnode(head, NULL);
        head = sibling;
    }
}

int symbol_table_find_by_name(const char *name, symbol_t **ret)
{
    stnode_t *result = hashtable_find_by_name(&symbol_table.hashtable, name);
    if (!result)
        return -1;  /* Not found. */
    if (ret)
        *ret = &result->symbol;
    return 0;   /* success */
}

int symbol_table_find_by_name_in_curenv(const char *name, symbol_t **ret)
{
    stnode_t *result = envstack_find_by_name_in_top(&symbol_table.envstack, name);
    if (!result)
        return -1;  /* Not found. */
    if (ret)
        *ret = &result->symbol;
    return 0;
}

extern void semantic_error(int errtype, int lineno, const char *msg, ...);

void check_undefined_symbol(stnode_t *node)
{
    symbol_t *symbol = &node->symbol;
    if (!symbol->is_defined)
        semantic_error(18, symbol->lineno, "Undefined function \"%s\".",
                       symbol->name);
}

void symbol_table_check_undefined_symbol()
{
    envstack_traverse(&symbol_table.envstack, check_undefined_symbol);
}

void print_symbol_table()
{
    printf("environment stack:\n");
    print_envstack(&symbol_table.envstack);
    printf("hashtable:\n");
    print_hashtable(&symbol_table.hashtable);
}

void add_builtin_func()
{
    symbol_t readfunc, writefunc;
    typelist_t typelist;

    /* add 'int read()' */
    type_t *inttype = (type_t *)create_type_basic(TYPE_INT);
    type_t *readfunctype = (type_t *)create_type_func(inttype, NULL);
    init_symbol(&readfunc, readfunctype, "read", 0, 1);
    symbol_table_add(&readfunc);

    /* add 'int write(int)' */
    init_typelist(&typelist);
    typelist_push_back(&typelist, inttype);
    type_t *writefunctype = (type_t *)create_type_func(inttype, &typelist);
    init_symbol(&writefunc, writefunctype, "write", 0, 1);
    symbol_table_add(&writefunc);
}
