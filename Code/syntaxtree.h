#ifndef _SYNTAX_TREE_H
#define _SYNTAX_TREE_H

typedef struct treenode {
    const char *name;
    int lineno;
    int is_term;
    int token;              /* used when is_term == 1 */
    union {
        char *id;           /* name of token ID */
        int ival;           /* value of token INT */
        float fval;         /* value of token FLOAT */
        const char *type;   /* type name of token TYPE */
    };
    struct treenode *child;
    struct treenode *next;
} treenode_t;

treenode_t *create_treenode(const char *name, int lineno, int is_term);
treenode_t *create_nontermnode(const char *name, int lineno);
treenode_t *create_termnode(const char *name, int lineno, int token);
treenode_t *create_idnode(int lineno, const char *id);
treenode_t *create_intnode(int lineno, int ival);
treenode_t *create_floatnode(int lineno, int fval);
treenode_t *create_typenode(int lineno, const char *type);
void destroy_treenode(treenode_t *node);

#endif