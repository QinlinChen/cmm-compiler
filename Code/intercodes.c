#include "intercodes.h"
#include "intercode.h"

#include <stdio.h>
#include <stdlib.h>

void intercodes_translate(treenode_t *root)
{
    iclist_t iclist;
    init_iclist(&iclist);


    fprint_iclist(stdout, &iclist);
}