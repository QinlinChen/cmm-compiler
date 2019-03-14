#include <stdio.h>

void yyrestart(FILE *);
int yyparse(void);
extern int yydebug;

int main(int argc, char **argv)
{
    FILE *f;

    if (argc <= 1)
        return 1;
    if (!(f = fopen(argv[1], "r"))) {
        perror(argv[1]);
        return 1;
    }

    yydebug = 1;
    yyrestart(f);
    yyparse();
    return 0;
}