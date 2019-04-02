#ifndef _SEMANTICS_H
#define _SEMANTICS_H

#include "syntaxtree.h"
#include "type-system.h"

void semantic_analyse(treenode_t *root);
int has_semantic_error();

#endif