%{
#include "syntaxtree.h"
#define YYSTYPE treenode_t *

#include "lex.yy.c"

void yyerror(char* msg);
%}

%token INT FLOAT
%token ID
%token SEMI COMMA
%token ASSIGNOP RELOP
%token PLUS MINUS STAR DIV
%token AND OR DOT NOT
%token TYPE
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT UMINUS
%left DOT LP RP LB RB

%%

/* High-level Definitions */
Program: ExtDefList
    ;
ExtDefList: ExtDef ExtDefList
    | /* empty */
    ;
ExtDef: Specifier ExtDecList SEMI
    | Specifier SEMI
    | Specifier FunDec CompSt
    ;
ExtDecList: VarDec
    | VarDec COMMA ExtDecList
    ;

/* Specifiers */
Specifier: TYPE
    | StructSpecifier
    ;
StructSpecifier: STRUCT OptTag LC DefList RC
    | STRUCT Tag
    ;
OptTag: ID
    | /* empty */
    ;
Tag: ID
    ;

/* Declarators */
VarDec: ID
    | VarDec LB INT RB
    ;
FunDec: ID LP VarList RP
    | ID LP RP
    ;
VarList: ParamDec COMMA VarList
    | ParamDec
    ;
ParamDec: Specifier VarDec
    ;

/* Statements */
CompSt: LC DefList StmtList RC
    ;
StmtList: Stmt StmtList
    | /* empty */
    ;
Stmt: Exp SEMI  { print_tree($1); }
    | CompSt
    | RETURN Exp SEMI
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE
    | IF LP Exp RP Stmt ELSE Stmt
    | WHILE LP Exp RP Stmt
    ;

/* Local Definitions */
DefList: Def DefList
    | /* empty */
    ;
Def: Specifier DecList SEMI
    ;
DecList: Dec
    | Dec COMMA DecList
    ;
Dec: VarDec
    | VarDec ASSIGNOP Exp
    ;

/* Expressions */
Exp: Exp ASSIGNOP Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp AND Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp OR Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp RELOP Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp PLUS Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp MINUS Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp STAR Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp DIV Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | LP Exp RP {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | MINUS Exp %prec UMINUS {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child2($$, $1, $2);
    }
    | NOT Exp {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child2($$, $1, $2);
    }
    | ID LP Args RP {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child4($$, $1, $2, $3, $4);
    }
    | ID LP RP {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp LB Exp RB {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child4($$, $1, $2, $3, $4);
    }
    | Exp DOT ID {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | ID
    | INT
    | FLOAT
    ;
Args: Exp COMMA Args {
        $$ = create_nontermnode("Args", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Exp {
        $$ = create_nontermnode("Args", @$.first_line);
        add_child($$, $1);
    }
    ;

%%

void yyerror(char* msg)
{
    fprintf(stderr, "error: %s\n", msg);
}
