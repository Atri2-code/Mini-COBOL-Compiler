#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

ASTNode *ast_node(NodeType type) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    if (!n) { fprintf(stderr, "Out of memory\n"); exit(1); }
    n->type = type;
    return n;
}

static const char *node_type_name(NodeType t) {
    switch (t) {
        case NODE_PROGRAM:  return "PROGRAM";
        case NODE_DATA_ITEM:return "DATA_ITEM";
        case NODE_MOVE:     return "MOVE";
        case NODE_COMPUTE:  return "COMPUTE";
        case NODE_DISPLAY:  return "DISPLAY";
        case NODE_PERFORM:  return "PERFORM";
        case NODE_IF:       return "IF";
        case NODE_STOP:     return "STOP";
        case NODE_BINOP:    return "BINOP";
        case NODE_IDENT:    return "IDENT";
        case NODE_INTEGER:  return "INTEGER";
        case NODE_STRING:   return "STRING";
        default:            return "?";
    }
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("[%s]", node_type_name(node->type));
    if (node->sval[0]) printf(" \"%s\"", node->sval);
    if (node->type == NODE_INTEGER) printf(" %d", node->ival);
    if (node->type == NODE_DATA_ITEM)
        printf(" PIC %c(%d)", node->pic_type, node->pic_len);
    printf("\n");
    if (node->left)        ast_print(node->left,        indent + 1);
    if (node->right)       ast_print(node->right,       indent + 1);
    if (node->cond)        ast_print(node->cond,        indent + 1);
    if (node->then_branch) ast_print(node->then_branch, indent + 1);
    if (node->else_branch) ast_print(node->else_branch, indent + 1);
    if (node->next)        ast_print(node->next,        indent);
}

void ast_free(ASTNode *node) {
    if (!node) return;
    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->cond);
    ast_free(node->then_branch);
    ast_free(node->else_branch);
    ast_free(node->next);
    free(node);
}
