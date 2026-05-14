#ifndef LEXER_H
#define LEXER_H

typedef enum {
    // Keywords
    TOK_IDENTIFICATION, TOK_DIVISION, TOK_PROGRAM_ID,
    TOK_ENVIRONMENT, TOK_DATA, TOK_WORKING_STORAGE, TOK_SECTION,
    TOK_PROCEDURE, TOK_MOVE, TOK_TO, TOK_COMPUTE, TOK_DISPLAY,
    TOK_PERFORM, TOK_IF, TOK_ELSE, TOK_END_IF, TOK_STOP, TOK_RUN,
    TOK_PIC, TOK_VALUE,
    // Literals & identifiers
    TOK_INTEGER, TOK_STRING, TOK_IDENT,
    // Symbols
    TOK_DOT, TOK_EQUALS, TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH,
    TOK_LPAREN, TOK_RPAREN,
    // Special
    TOK_EOF, TOK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char      text[256];
    int       line;
} Token;

typedef struct {
    const char *src;
    int         pos;
    int         line;
} Lexer;

void  lexer_init(Lexer *l, const char *src);
Token lexer_next(Lexer *l);
Token lexer_peek(Lexer *l);
const char *token_type_name(TokenType t);

#endif
