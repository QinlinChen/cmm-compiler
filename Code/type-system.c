#include "type-system.h"

#include <string.h>

static const char *typename_table[] ={
    [TYPE_INT] = "int",
    [TYPE_FLOAT] = "float"
};

#define TYPENAME_TABLE_SIZE sizeof(typename_table) / sizeof(typename_table[0])

int typename_to_id(const char *type_name)
{
    for (int i = 0; i < TYPENAME_TABLE_SIZE; ++i)
        if (typename_table[i] && !strcmp(typename_table[i], type_name))
            return i;
    return -1;
}

const char *typeid_to_name(int id)
{
    return typename_table[id];
}
