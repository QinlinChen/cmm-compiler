%option yylineno

%{
#include "syntax.tab.h"
#include "syntaxtree.h"

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
void handle_decinteger();
void handle_octinteger();
void handle_hexinteger();
void handle_float();
void handle_undefined_char();
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
"struct" { yylval = create_termnode("STRUCT", yylineno, STRUCT); return STRUCT; }
"return" { yylval = create_termnode("return", yylineno, RETURN); return RETURN; }
"if"     { yylval = create_termnode("IF", yylineno, IF); return IF; }
"else"   { yylval = create_termnode("ELSE", yylineno, ELSE); return ELSE; }
"while"  { yylval = create_termnode("WHILE", yylineno, WHILE); return WHILE; }
"int"    { yylval = create_typenode(yylineno, "int"); return TYPE; }
"float"  { yylval = create_typenode(yylineno, "float"); return TYPE; }
";"  { yylval = create_termnode("SEMI", yylineno, SEMI); return SEMI; }
","  { yylval = create_termnode("COMMA", yylineno, COMMA); return COMMA; }
"="  { yylval = create_termnode("ASSIGNOP", yylineno, ASSIGNOP); return ASSIGNOP; }
"==" { yylval = create_termnode("RELOP", yylineno, RELOP); return RELOP; }
">=" { yylval = create_termnode("RELOP", yylineno, RELOP); return RELOP; }
"<=" { yylval = create_termnode("RELOP", yylineno, RELOP); return RELOP; }
"!=" { yylval = create_termnode("RELOP", yylineno, RELOP); return RELOP; }
">"  { yylval = create_termnode("RELOP", yylineno, RELOP); return RELOP; }
"<"  { yylval = create_termnode("RELOP", yylineno, RELOP); return RELOP; }
"+"  { yylval = create_termnode("PLUS", yylineno, PLUS); return PLUS; }
"-"  { yylval = create_termnode("MINUS", yylineno, MINUS); return MINUS; }
"*"  { yylval = create_termnode("STAR", yylineno, STAR); return STAR; }
"/"  { yylval = create_termnode("DIV", yylineno, DIV); return DIV; }
"&&" { yylval = create_termnode("AND", yylineno, AND); return AND; }
"||" { yylval = create_termnode("OR", yylineno, OR); return OR; }
"."  { yylval = create_termnode("DOT", yylineno, DOT); return DOT; }
"!"  { yylval = create_termnode("NOT", yylineno, NOT); return NOT; }
"("  { yylval = create_termnode("LP", yylineno, LP); return LP; }
")"  { yylval = create_termnode("RP", yylineno, RP); return RP; }
"["  { yylval = create_termnode("LB", yylineno, LB); return LB; }
"]"  { yylval = create_termnode("RB", yylineno, RB); return RB; }
"{"  { yylval = create_termnode("LC", yylineno, LC); return LC; }
"}"  { yylval = create_termnode("RC", yylineno, RC); return RC; }
{id}         { yylval = create_idnode(yylineno, yytext); return ID; }
{decinteger} { handle_decinteger(); return INT; }
{octinteger} { handle_octinteger(); return INT; }
{hexinteger} { handle_hexinteger(); return INT; }
{float}      { handle_float(); return FLOAT; }
. { handle_undefined_char(); }

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

void handle_decinteger()
{
    int d;
    sscanf(yytext, "%d", &d);
    yylval = create_intnode(yylineno, d);
}

void handle_octinteger()
{
    int o;
    sscanf(yytext, "%o", &o);
    yylval = create_intnode(yylineno, o);
}

void handle_hexinteger()
{
    int x;
    sscanf(yytext, "%x", &x);
    yylval = create_intnode(yylineno, x);
}

void handle_float()
{
    float f;
    sscanf(yytext, "%f", &f);
    yylval = create_floatnode(yylineno, f);
}

void handle_undefined_char()
{
    printf("Error type A at Line %d: Mysterious characters \'%s\'\n",
           yylineno, yytext);
}