#include "syntax.tab.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    // FILE *f;

    // if (argc <= 1)
    //     return 1;
    // if (!(f = fopen(argv[1], "r"))) {
    //     perror(argv[1]);
    //     return 1;
    // }

    // yyrestart(f);
    yyparse();
    return 0;
}