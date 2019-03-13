%option yylineno

%{
#include "syntax.tab.h"
#include <assert.h>
#include <stdio.h>

int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

void handle_line_comment();
void handle_block_comment();

%}

ws              [ \t]+

digit           [0-9]
nonzerodigit    [1-9]
octdigit        [0-7]
hexdigit        [0-9a-fA-F]

letter          [a-zA-Z_]
id              {letter}({letter}|{digit})*

integer         {decinteger}|{octinteger}|{hexinteger}
decinteger      0|{nonzerodigit}{digit}*
octinteger      0{octdigit}+
hexinteger      0[xX]{hexdigit}+

float           {pointfloat}|{exponentfloat}
pointfloat      {digit}*"."{digit}+|{digit}+"."
exponentfloat   {pointfloat}[eE]("+"|"-")?{digit}+

%%

{ws} { /* Do nothing. */ }
"\n" { yycolumn = 1; }
"//" { handle_line_comment(); }
"/*" { handle_block_comment(); }
"struct" { return STRUCT; }
"return" { return RETURN; }
"if" { return IF; }
"else" { return ELSE; }
"while" { return WHILE; }
"int" { return TYPE; }
"float" { return TYPE; }
";" { return SEMI; }
"," { return COMMA; }
"=" { return ASSIGNOP; }
"==" { return RELOP; }
">=" { return RELOP; }
"<=" { return RELOP; }
"!=" { return RELOP; }
">" { return RELOP; }
"<" { return RELOP; }
"+" { return PLUS; }
"-" { return MINUS; }
"*" { return STAR; }
"/" { return DIV; }
"&&" { return AND; }
"||" { return OR; }
"." { return DOT; }
"!" { return NOT; }
"(" { return LP; }
")" { return RP; }
"[" { return LB; }
"]" { return RB; }
"{" { return LC; }
"}" { return RC; }
{id} { printf("ID (%s)\n", yytext); return ID; }
{decinteger} { int d; sscanf(yytext, "%d", &d); printf("decINT (%d)\n", d); return INT; }
{octinteger} { int o; sscanf(yytext, "%o", &o); printf("octINT (%d)\n", o); return INT; }
{hexinteger} { int x; sscanf(yytext, "%x", &x); printf("hexINT (%d)\n", x); return INT; }
{float} { float f; sscanf(yytext, "%f", &f); printf("FLOAT (%f)\n", f); return FLOAT; }
. { printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); }

%%

void handle_line_comment()
{
    char c;
    while ((c = input()) != EOF) {
        if (c == '\n')
            break;
    }
}

void handle_block_comment()
{
    char c;
    int status = 0;
    
    while ((c = input()) != EOF) {
        // DFA for "*/"
        switch (status) {
        case 0:
            if (c == '*')
                status = 1;
            break;
        case 1:
            if (c == '/')
                return;     // match successfuly
            else
                status = 0;
            break;
        default:
            assert(0);      // Should not reach here
            break;
        }
    }
    printf("Error type A at Line %d: No matched \'*/\'", yylineno);
}