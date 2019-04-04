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
    char *name;
} symbol_t;

void init_symbol_table();
void symbol_table_add(symbol_t *symbol);
void symbol_table_pushenv();
void symbol_table_popenv();
void print_symbol_table();

#endif