#include "syntaxtree.h"
#include "syntax.tab.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

treenode_t *create_treenode(const char *name, int lineno, int is_term)
{
    treenode_t *newnode = malloc(sizeof(treenode_t));
    newnode->name = name;
    newnode->lineno = lineno;
    newnode->is_term = is_term;
    newnode->child = newnode->next = NULL;
    return newnode;
}

treenode_t *create_nontermnode(const char *name, int lineno)
{
    treenode_t *newnode = create_treenode(name, lineno, 0);
    newnode->token = 0;
    return newnode;
}

treenode_t *create_termnode(const char *name, int lineno, int token)
{
    treenode_t *newnode = create_treenode(name, lineno, 1);
    assert(newnode->is_term == 1);
    newnode->token = token;
    return newnode;
}

treenode_t *create_idnode(int lineno, const char *id)
{
    treenode_t *newnode = create_termnode("ID", lineno, ID);
    assert(id != NULL);
    size_t idlen = strlen(id);
    newnode->id = malloc(idlen + 1);
    strcpy(newnode->id, id);
    assert(newnode->id[idlen] == '\0');
    return newnode;
}

treenode_t *create_intnode(int lineno, int ival)
{
    treenode_t *newnode = create_termnode("INT", lineno, INT);
    newnode->ival = ival;
    return newnode;
}

treenode_t *create_floatnode(int lineno, int fval)
{
    treenode_t *newnode = create_termnode("FLOAT", lineno, FLOAT);
    newnode->fval = fval;
    return newnode;
}

treenode_t *create_typenode(int lineno, const char *type)
{
    treenode_t *newnode = create_termnode("TYPE", lineno, TYPE);
    newnode->type = type;
    return newnode;
}

void destroy_treenode(treenode_t *node)
{
    if (!node)
        return;
    if (node->is_term && node->token == ID)
        free(node->id);
    free(node);
}