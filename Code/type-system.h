#ifndef _TYPE_SYSTEM_H
#define _TYPE_SYSTEM_H

/* abstract type: other types inherit from it. */
enum {
    TYPE_BASIC, TYPE_ARRAY, TYPE_STRUCT, TYPE_FUNC 
};

struct type {
    int kind;
};

/* basic type: T := int | float */
enum {
    TYPE_INT, TYPE_FLOAT
};

struct type_basic {
    int kind;
    int type_id;
};

int typename_to_id(const char *type_name);
const char *typeid_to_name(int id);

/* array type: T := T[] */
struct type_array {
    int kind;
    int size;
    struct type *extend_from;
};

/* struct type: T := struct_name { T fieldname; ...; } */
struct fieldlistnode {
    char *fieldname;
    struct type *type;
    struct filedlistnode *next;
};

struct fieldlist {
    struct fieldlistnode *front;
};

struct type_struct {
    int kind;
    struct fieldlist fields;
};

/* func type: T := T (T, T, ..., T) */
struct typelistnode {
    struct type *type;
    struct typelistnode *next;
};

struct typelist {
    struct typelistnode *front;
};

struct type_func {
    int kind;
    struct type *ret_type;
    struct typelist types;
};

/* used to store all types in one structure. */
struct type_storage {
    union {
        struct type t;
        struct type_basic tb;
        struct type_array ta;
        struct type_struct ts;
        struct type_func tf;
    };
};



#endif