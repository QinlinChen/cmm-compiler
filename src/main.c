#include <stdio.h>

extern int yydebug;
void yyrestart(FILE *);
int yyparse(void);
FILE *fout;

int main(int argc, char **argv)
{
    FILE *fin;
    
    if (argc <= 1)
        return 1;
    if (!(fin = fopen(argv[1], "r"))) {
        perror(argv[1]);
        return 1;
    }

    fout = stdout;
    if (argc >= 3 && !(fout = fopen(argv[2], "w"))) {
        perror(argv[2]);
        return 1;
    }

    yydebug = 0;
    yyrestart(fin);
    yyparse();

    return 0;
}
