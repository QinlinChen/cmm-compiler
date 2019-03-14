#include "syntaxtree.h"
#include "syntax.tab.h"

#include <stdio.h>
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

void add_child(treenode_t *parent, treenode_t *child)
{
    assert(parent != NULL);
    parent->child = child;
}

void add_sibling(treenode_t *lhs, treenode_t *rhs)
{
    assert(lhs != NULL);
    lhs->next = rhs;
}

void add_children(treenode_t *parent, treenode_t *children[], int nchild)
{
    assert(parent != NULL);
    if (nchild <= 0)
        return;
    parent->child = children[0];
    treenode_t *cur = children[0];    
    for (int i = 1; i < nchild; ++i) {
        if (!children[i])
            break;
        cur->next = children[i];
        cur = children[i];
    }
}

void add_child2(treenode_t *parent, treenode_t *c1, treenode_t *c2)
{
    treenode_t *children[] = { c1, c2 };
    add_children(parent, children, 2);
}

void add_child3(treenode_t *parent, treenode_t *c1, treenode_t *c2,
                treenode_t *c3)
{
    treenode_t *children[] = { c1, c2, c3 };
    add_children(parent, children, 3);
}

void add_child4(treenode_t *parent, treenode_t *c1, treenode_t *c2,
                treenode_t *c3, treenode_t *c4)
{
    treenode_t *children[] = { c1, c2, c3, c4 };
    add_children(parent, children, 4);
}

void add_child5(treenode_t *parent, treenode_t *c1, treenode_t *c2,
                treenode_t *c3, treenode_t *c4, treenode_t *c5)
{
    treenode_t *children[] = { c1, c2, c3, c4, c5 };
    add_children(parent, children, 5);
}

void print_tree(treenode_t *root)
{
    print_tree_r(root, 0);
}

void print_tree_r(treenode_t *root, int depth)
{
    if (!root)
        return;
    for (int i = 0; i < depth; ++i)
        printf("  ");

    printf("%s", root->name);
    if (root->is_term) {
        switch (root->token) {
        case ID:    printf(": %s", root->id); break;
        case INT:   printf(": %d", root->ival); break;
        case FLOAT: printf(": %f", root->fval); break;
        case TYPE:  printf(": %s", root->type); break;
        default:    break;
        }
    }
    else {
        printf(" (%d)", root->lineno);
    }
    printf("\n");

    for (treenode_t *child = root->child; child != NULL; child = child->next) {
        print_tree_r(child, depth + 1);
    }
}