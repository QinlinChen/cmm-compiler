#include "lex.yy.c"

int main(int argc, char **argv)
{
    if (argc > 1) {
        if (!(yyin = fopen(argv[1], "r"))) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
    }
    while (yylex() != 0)
        continue;
    return EXIT_SUCCESS;
}

