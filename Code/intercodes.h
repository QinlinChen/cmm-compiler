#ifndef _INTERCODES_H
#define _INTERCODES_H

#include "syntaxtree.h"
#include <stdio.h>

void intercodes_translate(treenode_t *root);
void fprint_intercodes(FILE *fp);

#endif