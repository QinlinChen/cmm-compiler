%option yylineno

%{
#include <assert.h>

int chars = 0;
int words = 0;
int lines = 0;

void handle_line_comment();
void handle_block_comment();

%}

delim [ \t\n]
ws {delim}+

digit [0-9]
nonzerodigit [1-9]
octdigit [0-7]
hexdigit [0-9a-fA-F]

letter [a-zA-Z_]
id {letter}({letter}|{digit})*

decinteger 0|{nonzerodigit}{digit}*
octinteger 0{octdigit}+
hexinteger 0[xX]{hexdigit}+
integer {decinteger}|{octinteger}|{hexinteger}

pointfloat {digit}*"."{digit}+|{digit}+"."
exponentfloat {pointfloat}[eE]("+"|"-")?{digit}+
float {pointfloat}|{exponentfloat}

%%

{ws} { /* Do nothing. */ }
"//" { handle_line_comment(); }
"/*" { handle_block_comment(); }
"struct" { printf("STRUCT\n"); }
"return" { printf("RETURN\n"); }
"if" { printf("IF\n"); }
"else" { printf("ELSE\n"); }
"while" { printf("WHILE\n"); }
"int" { printf("TYPE (int)\n"); }
"float" { printf("TYPE (float)\n"); }
";" { printf("SEMI\n"); }
"," { printf("COMMA\n"); }
"=" { printf("ASSIGNOP\n"); }
"==" { printf("RELOP (==)\n"); }
">=" { printf("RELOP (>=)\n"); }
"<=" { printf("RELOP (<=)\n"); }
"!=" { printf("RELOP (!=)\n"); }
">" { printf("RELOP (>)\n"); }
"<" { printf("RELOP (<)\n"); }
"+" { printf("ADD\n"); }
"-" { printf("MINUS\n"); }
"*" { printf("STAR\n"); }
"/" { printf("DIV\n"); }
"&&" { printf("AND\n"); }
"||" { printf("OR\n"); }
"." { printf("DOT\n"); }
"!" { printf("NOT\n"); }
"(" { printf("LP\n"); }
")" { printf("RP\n"); }
"[" { printf("LB\n"); }
"]" { printf("RB\n"); }
"{" { printf("LC\n"); }
"}" { printf("RC\n"); }
{id} { printf("ID (%s)\n", yytext); }
{decinteger} { int i; sscanf(yytext, "%d", &i); printf("octINT (%d)\n", i); }
{octinteger} { int i; sscanf(yytext, "%o", &i); printf("octINT (%d)\n", i); }
{hexinteger} { int i; sscanf(yytext, "%x", &i); printf("hexINT (%d)\n", i); }
{float} { float f; sscanf(yytext, "%f", &f); printf("FLOAT (%f)\n", f); }
. { printf("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext); }

%%

void handle_line_comment()
{
    char c;
    while ((c = input()) != '\n')
        continue;
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
                return;     // match succeed
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