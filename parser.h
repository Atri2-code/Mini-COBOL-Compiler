#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer  lexer;
    Token  current;
    int    errors;
} Parser;

void     parser_init(Parser *p, const char *src);
ASTNode *parser_parse(Parser *p);

#endif
