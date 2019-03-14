%{
#include "syntaxtree.h"

#define YYERROR_VERBOSE 1
#define YYSTYPE treenode_t *

int has_error = 0;

#define yyerror(msg) \
    do {\
        has_error = 1; \
        fprintf(stderr, "Error type B at Line %d: %s\n", yylineno, msg); \
        fflush(stderr); \
    } while (0)


#include "lex.yy.c"
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
Program: ExtDefList {
        $$ = create_nontermnode("Program", @$.first_line);
        add_child($$, $1);
        if (!has_error)
            print_tree($$);
    }
    ;
ExtDefList: ExtDef ExtDefList {
        $$ = create_nontermnode("ExtDefList", @$.first_line);
        add_child2($$, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
ExtDef: Specifier ExtDecList SEMI {
        $$ = create_nontermnode("ExtDef", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | Specifier SEMI {
        $$ = create_nontermnode("ExtDef", @$.first_line);
        add_child2($$, $1, $2);
    }
    | Specifier FunDec CompSt {
        $$ = create_nontermnode("ExtDef", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    ;
ExtDecList: VarDec {
        $$ = create_nontermnode("ExtDecList", @$.first_line);
        add_child($$, $1);
    }
    | VarDec COMMA ExtDecList {
        $$ = create_nontermnode("ExtDecList", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    ;

/* Specifiers */
Specifier: TYPE {
        $$ = create_nontermnode("Specifier", @$.first_line);
        add_child($$, $1);
    }
    | StructSpecifier {
        $$ = create_nontermnode("Specifier", @$.first_line);
        add_child($$, $1);
    }
    ;
StructSpecifier: STRUCT OptTag LC DefList RC {
        $$ = create_nontermnode("StructSpecifier", @$.first_line);
        add_child5($$, $1, $2, $3, $4, $5);
    }
    | STRUCT Tag {
        $$ = create_nontermnode("StructSpecifier", @$.first_line);
        add_child2($$, $1, $2);
    }
    ;
OptTag: ID {
        $$ = create_nontermnode("OptTag", @$.first_line);
        add_child($$, $1);
    }
    | /* empty */ { $$ = NULL; }
    ;
Tag: ID {
        $$ = create_nontermnode("Tag", @$.first_line);
        add_child($$, $1);
    }
    ;

/* Declarators */
VarDec: ID {
        $$ = create_nontermnode("VarDec", @$.first_line);
        add_child($$, $1);
    }
    | VarDec LB INT RB { 
        $$ = create_nontermnode("VarDec", @$.first_line);
        add_child4($$, $1, $2, $3, $4);
    }
    ;
FunDec: ID LP VarList RP { 
        $$ = create_nontermnode("FunDec", @$.first_line);
        add_child4($$, $1, $2, $3, $4);
    }
    | ID LP RP { 
        $$ = create_nontermnode("FunDec", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    ;
VarList: ParamDec COMMA VarList { 
        $$ = create_nontermnode("VarList", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | ParamDec { 
        $$ = create_nontermnode("VarList", @$.first_line);
        add_child($$, $1);
    }
    ;
ParamDec: Specifier VarDec { 
        $$ = create_nontermnode("ParamDec", @$.first_line);
        add_child2($$, $1, $2);
    }
    ;

/* Statements */
CompSt: LC DefList StmtList RC { 
        $$ = create_nontermnode("CompSt", @$.first_line);
        add_child4($$, $1, $2, $3, $4);
    }
    ;
StmtList: Stmt StmtList { 
        $$ = create_nontermnode("StmtList", @$.first_line);
        add_child2($$, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
Stmt: Exp SEMI  { 
        $$ = create_nontermnode("Stmt", @$.first_line);
        add_child2($$, $1, $2);
    }
    | CompSt {
        $$ = create_nontermnode("Stmt", @$.first_line);
        add_child($$, $1);
    }
    | RETURN Exp SEMI {
        $$ = create_nontermnode("Stmt", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
        $$ = create_nontermnode("Stmt", @$.first_line);
        add_child5($$, $1, $2, $3, $4, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt {
        $$ = create_nontermnode("Stmt", @$.first_line);
        treenode_t *children[] = { $1, $2, $3, $4, $5, $6, $7};
        add_children($$, children, 7);
    }
    | WHILE LP Exp RP Stmt {
        $$ = create_nontermnode("Stmt", @$.first_line);
        add_child5($$, $1, $2, $3, $4, $5);
    }
    | error SEMI { printf("Stat: \';\'\n"); }
    ;

/* Local Definitions */
DefList: Def DefList {
        $$ = create_nontermnode("DefList", @$.first_line);
        add_child2($$, $1, $2);
    }
    | /* empty */ { $$ = NULL; }
    ;
Def: Specifier DecList SEMI {
        $$ = create_nontermnode("Def", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    ;
DecList: Dec {
        $$ = create_nontermnode("DecList", @$.first_line);
        add_child($$, $1);
    }
    | Dec COMMA DecList {
        $$ = create_nontermnode("DecList", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
    ;
Dec: VarDec {
        $$ = create_nontermnode("Dec", @$.first_line);
        add_child($$, $1);
    }
    | VarDec ASSIGNOP Exp {
        $$ = create_nontermnode("Dec", @$.first_line);
        add_child3($$, $1, $2, $3);
    }
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
    | ID {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child($$, $1);
    }
    | INT {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child($$, $1);
    }
    | FLOAT {
        $$ = create_nontermnode("Exp", @$.first_line);
        add_child($$, $1);
    }
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
