#ifndef _INTERCODE_H
#define _INTERCODE_H

#include <stdio.h>

enum {
    OPRAND_VAR, OPRAND_ADDR, OPRAND_CONST
};

typedef struct operand {
    int kind;
    union {
        int varid;
        int val;
    };
} operand_t;

enum {
    IC_LABEL, IC_FUNCDEF, IC_ASSIGN, IC_ARITHBOP,
    IC_REF, IC_DREF, IC_DREFASSIGN, IC_GOTO, IC_CONDGOTO,
    IC_RETURN, IC_DEC, IC_ARG, IC_CALL, IC_PARAM,
    IC_READ, IC_WRITE
};

enum {
    ICOP_ADD, ICOP_SUB, ICOP_MUL, ICOP_DIV,
    ICOP_EQ, ICOP_NEQ, ICOP_L, ICOP_LE,
    ICOP_G, ICOP_GE
};

typedef struct intercode {
    int kind;
} intercode_t;

int alloc_varid();
int alloc_labelid();

void init_var_operand(operand_t *op, int varid);
void init_addr_operand(operand_t *op, int varid);
void init_const_operand(operand_t *op, int val);
void init_temp_var(operand_t *op);
void init_temp_addr(operand_t *op);

intercode_t *create_ic_label(int labelid);
intercode_t *create_ic_funcdef(const char *fname);
intercode_t *create_ic_assign(operand_t *lhs, operand_t *rhs);
intercode_t *create_ic_arithbop(int op, operand_t *target,
                                operand_t *lhs, operand_t *rhs);
intercode_t *create_ic_ref(operand_t *lhs, operand_t *rhs);
intercode_t *create_ic_dref(operand_t *lhs, operand_t *rhs);
intercode_t *create_ic_drefassign(operand_t *lhs, operand_t *rhs);
intercode_t *create_ic_goto(int labelid);
intercode_t *create_ic_condgoto(int relop, operand_t *lhs, operand_t *rhs,
                                int labelid);
intercode_t *create_ic_return(operand_t *ret);
intercode_t *create_ic_dec(int varid, int size);
intercode_t *create_ic_arg(operand_t *arg);
intercode_t *create_ic_call(const char *fname, int retvarid);
intercode_t *create_ic_param(int varid);
intercode_t *create_ic_read(int varid);
intercode_t *create_ic_write(int varid);

void fprint_intercode(FILE *fp, intercode_t *ic);

#endif