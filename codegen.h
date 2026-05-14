#ifndef CODEGEN_H
#define CODEGEN_H
#include <stdio.h>
#include "ast.h"
#include "semantic.h"

void codegen(ASTNode *prog, SymTable *st, FILE *out);

#endif
