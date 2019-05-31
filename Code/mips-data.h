#ifndef _MIPS_DATA_H
#define _MIPS_DATA_H

#include "intercodes.h"

/* ------------------------------------ *
 *            var info list             *
 * ------------------------------------ */

typedef struct varinfo
{
    operand_t var;
    /* This field indicates which reg the var is stored.
     * If reg is R_SP or R_FP, offset is used to indicate
     * its relative position in the stack. */
    int reg;
    int offset;
} varinfo_t;

varinfo_t *create_varinfo(operand_t *var, int reg, int offset);
void print_varinfo(varinfo_t *vi);

void init_varinfolist();
void varinfolist_clear();
void varinfolist_push_back(varinfo_t *vi);
varinfo_t *varinfolist_find(operand_t *var);
void print_varinfolist();

int collect_varinfo(iclistnode_t *funcdefnode);


/* ------------------------------------ *
 *            reg info table            *
 * ------------------------------------ */

typedef struct reginfo
{
    int is_empty;
    int is_locked;
    operand_t var_loaded;
} reginfo_t;

#define REG_SIZE    32

enum {
    R_NONE = -1, R_ZERO = 0, R_AT,
    R_V0, R_V1, R_A0, R_A1, R_A2, R_A3,
    R_T0, R_T1, R_T2, R_T3, R_T4, R_T5, R_T6, R_T7,
    R_S0, R_S1, R_S2, R_S3, R_S4, R_S5, R_S6, R_S7,
    R_T8, R_T9, R_K0, R_K1,
    R_GP, R_SP, R_FP, R_RA
};

const char *get_regalias(int reg);

void init_reginfo_table();
void reginfo_table_clear();
void print_reginfo_table();

void reginfo_table_lock(int reg);
void reginfo_table_unlock(int reg);
int reginfo_table_is_empty(int reg);
int reginfo_table_find_var(operand_t *var);
int reginfo_table_find_empty();
int reginfo_table_find_expellable();
operand_t reginfo_table_get_var(int reg);
void reginfo_table_alloc_reg(int reg, operand_t *var);
operand_t reginfo_table_free_reg(int reg);


#endif