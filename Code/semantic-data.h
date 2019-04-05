#ifndef _SEMANTIC_DATA_H
#define _SEMANTIC_DATA_H

#include "type-system.h"

/* the table of structure definitions */
void init_structdef_table();
void structdef_table_add(type_struct_t *structdef);
type_struct_t *structdef_table_find_by_name(const char *structname);
void print_structdef_table();

/* symbol table */
typedef struct symbol {
    type_t *type;
    const char *name;
    int lineno;
    int is_defined;
} symbol_t;

void init_symbol(symbol_t *symbol, type_t *type, const char *name,
                 int lineno, int is_defined);
void symbol_set_defined(symbol_t *symbol, int is_defined);
void print_symbol(symbol_t *symbol);

void init_symbol_table();
void symbol_table_add(symbol_t *symbol);
void symbol_table_pushenv();
void symbol_table_popenv();
int symbol_table_find_by_name(const char *name, symbol_t **ret);
void symbol_table_check_undefined_symbol();
void print_symbol_table();

#endif