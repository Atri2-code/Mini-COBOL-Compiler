#ifndef SEMANTIC_H
#define SEMANTIC_H
#include "ast.h"

typedef struct SymEntry {
    char           name[256];
    char           pic_type;
    int            pic_len;
    struct SymEntry *next;
} SymEntry;

typedef struct {
    SymEntry *head;
    int       errors;
} SymTable;

void symtable_init(SymTable *st);
void symtable_add(SymTable *st, const char *name, char pic_type, int pic_len);
SymEntry *symtable_lookup(SymTable *st, const char *name);
void symtable_free(SymTable *st);

int semantic_check(ASTNode *prog, SymTable *st);

#endif
