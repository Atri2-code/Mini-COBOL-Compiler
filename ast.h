#ifndef AST_H
#define AST_H

typedef enum {
    NODE_PROGRAM,
    NODE_DATA_ITEM,
    NODE_MOVE,
    NODE_COMPUTE,
    NODE_DISPLAY,
    NODE_PERFORM,
    NODE_IF,
    NODE_STOP,
    NODE_BINOP,
    NODE_IDENT,
    NODE_INTEGER,
    NODE_STRING,
} NodeType;

typedef struct ASTNode {
    NodeType type;
    char     sval[256];   /* identifier name, string literal, operator */
    int      ival;        /* integer literal value */
    /* pic clause */
    char     pic[64];
    int      pic_len;
    char     pic_type;    /* '9' numeric, 'X' alphanumeric */
    /* children */
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *cond;
    struct ASTNode *then_branch;
    struct ASTNode *else_branch;
    struct ASTNode *next;  /* sibling in statement list */
} ASTNode;

ASTNode *ast_node(NodeType type);
void     ast_print(ASTNode *node, int indent);
void     ast_free(ASTNode *node);

#endif
