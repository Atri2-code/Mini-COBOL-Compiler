#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

void symtable_init(SymTable *st) { st->head = NULL; st->errors = 0; }

void symtable_add(SymTable *st, const char *name, char pic_type, int pic_len) {
    SymEntry *e = calloc(1, sizeof(SymEntry));
    strncpy(e->name, name, sizeof(e->name)-1);
    e->pic_type = pic_type;
    e->pic_len  = pic_len;
    e->next     = st->head;
    st->head    = e;
}

SymEntry *symtable_lookup(SymTable *st, const char *name) {
    for (SymEntry *e = st->head; e; e = e->next)
        if (strcmp(e->name, name) == 0) return e;
    return NULL;
}

void symtable_free(SymTable *st) {
    SymEntry *e = st->head;
    while (e) { SymEntry *nx = e->next; free(e); e = nx; }
}

static int check_expr(ASTNode *n, SymTable *st) {
    if (!n) return 1;
    if (n->type == NODE_IDENT) {
        if (!symtable_lookup(st, n->sval)) {
            fprintf(stderr, "Semantic error: undeclared variable '%s'\n", n->sval);
            st->errors++;
            return 0;
        }
        return 1;
    }
    return check_expr(n->left, st) & check_expr(n->right, st);
}

static int check_stmt(ASTNode *n, SymTable *st) {
    if (!n) return 1;
    int ok = 1;
    switch (n->type) {
        case NODE_MOVE:
            ok &= check_expr(n->left, st);
            if (!symtable_lookup(st, n->right->sval)) {
                fprintf(stderr, "Semantic error: undeclared target '%s' in MOVE\n",
                        n->right->sval);
                st->errors++; ok = 0;
            }
            break;
        case NODE_COMPUTE:
            if (!symtable_lookup(st, n->left->sval)) {
                fprintf(stderr, "Semantic error: undeclared target '%s' in COMPUTE\n",
                        n->left->sval);
                st->errors++; ok = 0;
            }
            ok &= check_expr(n->right, st);
            break;
        case NODE_DISPLAY:
            ok &= check_expr(n->left, st);
            break;
        case NODE_IF:
            ok &= check_expr(n->cond, st);
            ok &= check_stmt(n->then_branch, st);
            ok &= check_stmt(n->else_branch, st);
            break;
        default: break;
    }
    return ok & check_stmt(n->next, st);
}

int semantic_check(ASTNode *prog, SymTable *st) {
    /* Register data items */
    for (ASTNode *d = prog->left; d; d = d->next)
        if (d->type == NODE_DATA_ITEM)
            symtable_add(st, d->sval, d->pic_type, d->pic_len);
    /* Check procedure */
    return check_stmt(prog->right, st);
}
