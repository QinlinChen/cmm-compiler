#include "intercodes.h"
#include "intercode.h"
#include "semantic-data.h"
#include "semantics.h"

#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------ *
 *             intercodes               *
 * ------------------------------------ */

static iclist_t intercodes;

void init_intercodes()
{
    init_iclist(&intercodes);
}

void intercodes_push_back(intercode_t *ic)
{
    iclist_push_back(&intercodes, ic);
}

void fprint_intercodes(FILE *fp)
{
    fprint_iclist(fp, &intercodes);
}

/* ------------------------------------ *
 *              translate               *
 * ------------------------------------ */

void intercodes_translate(treenode_t *root)
{
    init_structdef_table();
    init_symbol_table();
    init_intercodes();

}