#ifndef _SYNTAX_TREE_H
#define _SYNTAX_TREE_H

#include "type-system.h"

typedef struct treenode
{
    const char *name;
    int lineno;
    int is_term;
    int token; /* used when is_term == 1 */
    union {
        char *id;       /* name of ID */
        int ival;       /* value of INT */
        float fval;     /* value of FLOAT */
        int typeid;     /* typeid of TYPE */
    };
    struct treenode *child;
    struct treenode *next;
} treenode_t;

treenode_t *create_treenode(const char *name, int lineno, int is_term);
treenode_t *create_nontermnode(const char *name, int lineno);
treenode_t *create_termnode(const char *name, int lineno, int token);
treenode_t *create_idnode(int lineno, const char *id);
treenode_t *create_intnode(int lineno, int ival);
treenode_t *create_floatnode(int lineno, float fval);
treenode_t *create_typenode(int lineno, const char *type_name);
void destroy_treenode(treenode_t *node);

void add_child(treenode_t *parent, treenode_t *child);
void add_sibling(treenode_t *lhs, treenode_t *rhs);
void add_children(treenode_t *parent, treenode_t *children[], int nchild);
void add_child2(treenode_t *parent, treenode_t *c1, treenode_t *c2);
void add_child3(treenode_t *parent, treenode_t *c1, treenode_t *c2,
                treenode_t *c3);
void add_child4(treenode_t *parent, treenode_t *c1, treenode_t *c2,
                treenode_t *c3, treenode_t *c4);
void add_child5(treenode_t *parent, treenode_t *c1, treenode_t *c2,
                treenode_t *c3, treenode_t *c4, treenode_t *c5);

void print_tree(treenode_t *root);
void print_tree_r(treenode_t *root, int depth);

#endif