#ifndef _TYPE_SYSTEM_H
#define _TYPE_SYSTEM_H

/* abstract type: other types inherit from it. */
enum {
    TYPE_BASIC, TYPE_ARRAY, TYPE_STRUCT, TYPE_FUNC 
};

typedef struct type {
    int kind;
} type_t;

/* basic type: T := int | float */
enum {
    TYPE_INT, TYPE_FLOAT
};

int typename_to_id(const char *type_name);
const char *typeid_to_name(int id);

typedef struct type_basic {
    int kind;
    int type_id;
} type_basic_t;

type_basic_t *create_type_basic(int type_id);

/* array type: T := T[] */
typedef struct type_array {
    int kind;
    int size;
    type_t *extend_from;
} type_array_t;

/* fieldlist */
typedef struct fieldlistnode {
    char *fieldname;
    type_t *type;
    struct filedlistnode *next;
} fieldlistnode_t;

typedef struct fieldlist {
    int size;
    fieldlistnode_t *front;
    fieldlistnode_t *back;
} fieldlist_t;

void init_fieldlist(fieldlist_t *fieldlist);
void fieldlist_push_back(fieldlist_t *fieldlist,
                         const char *fieldname, type_t *type);
void fieldlist_concat(fieldlist_t *lhs, fieldlist_t *rhs);

/* struct type: T := struct_name { T fieldname; ...; } */
typedef struct type_struct {
    int kind;
    char *structname;
    fieldlist_t fields;
} type_struct_t;

type_struct_t *create_type_struct(const char *structname, fieldlist_t *fields);

/* typelist */
typedef struct typelistnode {
    type_t *type;
    struct typelistnode *next;
} typelistnode_t;

typedef struct typelist {
    int size;
    typelistnode_t *front;
    typelistnode_t *back;
} typelist_t;

void init_typelist(typelist_t *typelist);
void typelist_push_back(typelist_t *typelist, type_t *type);
type_t *typelist_find_type_struct_by_name(typelist_t *typelist, const char *name);

/* func type: T := T (T, T, ..., T) */
typedef struct type_func {
    int kind;
    type_t *ret_type;
    typelist_t types;
} type_func_t;

/* used to store all types in one structure. */
typedef struct type_storage {
    union {
        type_t t;
        type_basic_t tb;
        type_array_t ta;
        type_struct_t ts;
        type_func_t tf;
    };
} type_storage_t;

#endif