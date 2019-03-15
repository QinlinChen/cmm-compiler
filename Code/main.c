#include <stdio.h>

extern int yydebug;
void yyrestart(FILE *);
int yyparse(void);


int main(int argc, char **argv)
{
    FILE *f;
    
    if (argc <= 1)
        return 1;
    if (!(f = fopen(argv[1], "r"))) {
        perror(argv[1]);
        return 1;
    }

    yydebug = 0;
    yyrestart(f);
    yyparse();

    return 0;
}