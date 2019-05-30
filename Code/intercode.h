#ifndef _INTERCODE_H
#define _INTERCODE_H

#include <stdio.h>

/* ------------------------------------ *
 *               operand                *
 * ------------------------------------ */

enum {
    OPERAND_VAR, OPERAND_ADDR, OPERAND_CONST
};

#define LABEL_FALL  -1

typedef struct operand {
    int kind;
    union {
        int varid;
        int val;
    };
    int is_temp;
} operand_t;

void init_varid();
void init_labelid();
int alloc_varid();
int alloc_labelid();

void init_var_operand(operand_t *op, int varid);
void init_addr_operand(operand_t *op, int varid);
void init_const_operand(operand_t *op, int val);
void init_temp_var(operand_t *op);
void init_temp_addr(operand_t *op);

int is_const_operand(operand_t *op);
void fprint_operand(FILE *fp, operand_t *op);

/* ------------------------------------ *
 *              intercode               *
 * ------------------------------------ */

enum {
    ICOP_ADD, ICOP_SUB, ICOP_MUL, ICOP_DIV,
    ICOP_EQ, ICOP_NEQ, ICOP_L, ICOP_LE,
    ICOP_G, ICOP_GE
};

const char *icop_to_str(int icop);
int str_to_icop(const char *str);
int complement_rel_icop(int icop);

enum {
    IC_LABEL, IC_FUNCDEF, IC_ASSIGN, IC_ARITHBOP,
    IC_REF, IC_DREF, IC_DREFASSIGN, IC_GOTO, IC_CONDGOTO,
    IC_RETURN, IC_DEC, IC_ARG, IC_CALL, IC_PARAM,
    IC_READ, IC_WRITE
};

typedef struct intercode {
    int kind;
} intercode_t;

typedef struct ic_label {
    int kind;
    int labelid;
} ic_label_t;

typedef struct ic_funcdef {
    int kind;
    const char *fname;
} ic_funcdef_t;

typedef struct ic_assign {
    int kind;
    operand_t lhs;
    operand_t rhs;
} ic_assign_t;

typedef struct ic_arithbop {
    int kind;
    int op;
    operand_t target;
    operand_t lhs;
    operand_t rhs;
} ic_arithbop_t;

typedef struct ic_ref {
    int kind;
    operand_t lhs;
    operand_t rhs;
} ic_ref_t;

typedef struct ic_dref {
    int kind;
    operand_t lhs;
    operand_t rhs;
} ic_dref_t;

typedef struct ic_drefassign {
    int kind;
    operand_t lhs;
    operand_t rhs;
} ic_drefassign_t;

typedef struct ic_goto {
    int kind;
    int labelid;
} ic_goto_t;

typedef struct ic_condgoto {
    int kind;
    int relop;
    operand_t lhs;
    operand_t rhs;
    int labelid;
} ic_condgoto_t;

typedef struct ic_return {
    int kind;
    operand_t ret;
} ic_return_t;

typedef struct ic_dec {
    int kind;
    operand_t var;
    int size;
} ic_dec_t;

typedef struct ic_arg {
    int kind;
    operand_t arg;
} ic_arg_t;

typedef struct ic_call {
    int kind;
    const char *fname;
    operand_t ret;
} ic_call_t;

typedef struct ic_param {
    int kind;
    operand_t var;
} ic_param_t;

typedef struct ic_read {
    int kind;
    operand_t var;
} ic_read_t;

typedef struct ic_write {
    int kind;
    operand_t var;
} ic_write_t;

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
intercode_t *create_ic_dec(operand_t *var, int size);
intercode_t *create_ic_arg(operand_t *arg);
intercode_t *create_ic_call(const char *fname, operand_t *ret);
intercode_t *create_ic_param(operand_t *var);
intercode_t *create_ic_read(operand_t *var);
intercode_t *create_ic_write(operand_t *var);

void fprint_intercode(FILE *fp, intercode_t *ic);

/* ------------------------------------ *
 *           intercodelist              *
 * ------------------------------------ */

typedef struct iclistnode {
    intercode_t *ic;
    struct iclistnode *prev;
    struct iclistnode *next;
} iclistnode_t;

typedef struct lclist {
    int size;
    iclistnode_t *front;
    iclistnode_t *back;
} iclist_t;

void init_iclist(iclist_t *iclist);
void iclist_push_back(iclist_t *iclist, intercode_t *ic);
void fprint_iclist(FILE *fp, iclist_t *iclist);

#endif