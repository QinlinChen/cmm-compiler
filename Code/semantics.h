#ifndef _SEMANTICS_H
#define _SEMANTICS_H

#include "syntaxtree.h"
#include "semantic-data.h"

int has_semantic_error();

/* Entrance of semantic analysis */
void semantic_analyse(treenode_t *root);

/* Analyse Specifier and return the type infomation. */
type_t *analyse_specifier(treenode_t *specifier);

/* enum for 'context', the argument of analyse_def_list */
enum { CONTEXT_STRUCT_DEF, CONTEXT_VAR_DEF };

/* Analyse DefList. If it is called during the context of struct field definition,
 * we will append the field to the 'fieldlist'. Otherwise, it must be analysing
 * local definitions and we will add them to the symbol table. In this case,
 * 'fieldlist' can be NULL. */
void analyse_def_list(treenode_t *def_list, fieldlist_t *ret, int context);

/* Analyse FunDec and return a symbol of func type. It will also store
 * the parameters of the function in 'fieldlist' for analysing CompSt use. */
void analyse_fun_dec(treenode_t *fun_dec, type_t *spec,
                     symbol_t *ret_symbol, fieldlist_t *ret_params);

/* Wrapper functions that check semantic errors. */
int checked_structdef_table_add(type_struct_t *structdef, int lineno);
int checked_fieldlist_push_back(fieldlist_t *fieldlist, symbol_t *symbol);
int checked_paramlist_push_back(fieldlist_t *paramlist, symbol_t *symbol);
int checked_symbol_table_add_var(symbol_t *symbol);
int checked_symbol_table_add_func(symbol_t *func, int is_def);

#endif