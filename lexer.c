#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

typedef struct { const char *word; TokenType type; } Keyword;

static Keyword KEYWORDS[] = {
    {"IDENTIFICATION", TOK_IDENTIFICATION}, {"DIVISION",        TOK_DIVISION},
    {"PROGRAM-ID",     TOK_PROGRAM_ID},     {"ENVIRONMENT",     TOK_ENVIRONMENT},
    {"DATA",           TOK_DATA},           {"WORKING-STORAGE", TOK_WORKING_STORAGE},
    {"SECTION",        TOK_SECTION},        {"PROCEDURE",       TOK_PROCEDURE},
    {"MOVE",           TOK_MOVE},           {"TO",              TOK_TO},
    {"COMPUTE",        TOK_COMPUTE},        {"DISPLAY",         TOK_DISPLAY},
    {"PERFORM",        TOK_PERFORM},        {"IF",              TOK_IF},
    {"ELSE",           TOK_ELSE},           {"END-IF",          TOK_END_IF},
    {"STOP",           TOK_STOP},           {"RUN",             TOK_RUN},
    {"PIC",            TOK_PIC},            {"VALUE",           TOK_VALUE},
    {NULL, TOK_UNKNOWN}
};

void lexer_init(Lexer *l, const char *src) {
    l->src  = src;
    l->pos  = 0;
    l->line = 1;
}

static void skip_whitespace_and_comments(Lexer *l) {
    while (l->src[l->pos]) {
        /* skip whitespace */
        if (isspace((unsigned char)l->src[l->pos])) {
            if (l->src[l->pos] == '\n') l->line++;
            l->pos++;
        /* COBOL line comment: * in column 7 (pos % 80 == 6) or modern * anywhere after space */
        } else if (l->src[l->pos] == '*' &&
                   (l->pos == 0 || l->src[l->pos-1] == '\n')) {
            while (l->src[l->pos] && l->src[l->pos] != '\n') l->pos++;
        } else {
            break;
        }
    }
}

static Token make_token(TokenType type, const char *text, int line) {
    Token t;
    t.type = type;
    t.line = line;
    strncpy(t.text, text, sizeof(t.text) - 1);
    t.text[sizeof(t.text) - 1] = '\0';
    return t;
}

Token lexer_next(Lexer *l) {
    skip_whitespace_and_comments(l);

    if (!l->src[l->pos]) return make_token(TOK_EOF, "", l->line);

    int line = l->line;
    char c   = l->src[l->pos];

    /* String literal */
    if (c == '"' || c == '\'') {
        char delim = c;
        l->pos++;
        char buf[256]; int bi = 0;
        while (l->src[l->pos] && l->src[l->pos] != delim)
            buf[bi++] = l->src[l->pos++];
        if (l->src[l->pos]) l->pos++; /* closing quote */
        buf[bi] = '\0';
        return make_token(TOK_STRING, buf, line);
    }

    /* Integer literal */
    if (isdigit((unsigned char)c)) {
        char buf[64]; int bi = 0;
        while (isdigit((unsigned char)l->src[l->pos]))
            buf[bi++] = l->src[l->pos++];
        buf[bi] = '\0';
        return make_token(TOK_INTEGER, buf, line);
    }

    /* Identifier or keyword (may contain hyphen, but hyphen must be between alpha/digits) */
    if (isalpha((unsigned char)c)) {
        char buf[256]; int bi = 0;
        while (1) {
            char ch = l->src[l->pos];
            if (isalnum((unsigned char)ch)) {
                buf[bi++] = (char)toupper((unsigned char)ch);
                l->pos++;
            } else if (ch == '-' && isalnum((unsigned char)l->src[l->pos+1])) {
                /* hyphen is part of identifier only if followed by alnum */
                buf[bi++] = '-';
                l->pos++;
            } else {
                break;
            }
        }
        buf[bi] = '\0';
        for (int i = 0; KEYWORDS[i].word; i++)
            if (strcmp(buf, KEYWORDS[i].word) == 0)
                return make_token(KEYWORDS[i].type, buf, line);
        return make_token(TOK_IDENT, buf, line);
    }

    /* Single-char symbols */
    l->pos++;
    switch (c) {
        case '.': return make_token(TOK_DOT,    ".", line);
        case '=': return make_token(TOK_EQUALS, "=", line);
        case '+': return make_token(TOK_PLUS,   "+", line);
        case '-': return make_token(TOK_MINUS,  "-", line);
        case '*': return make_token(TOK_STAR,   "*", line);
        case '/': return make_token(TOK_SLASH,  "/", line);
        case '(': return make_token(TOK_LPAREN, "(", line);
        case ')': return make_token(TOK_RPAREN, ")", line);
    }
    char unk[2] = {c, '\0'};
    return make_token(TOK_UNKNOWN, unk, line);
}

Token lexer_peek(Lexer *l) {
    int saved_pos  = l->pos;
    int saved_line = l->line;
    Token t = lexer_next(l);
    l->pos  = saved_pos;
    l->line = saved_line;
    return t;
}

const char *token_type_name(TokenType t) {
    static const char *names[] = {
        "IDENTIFICATION","DIVISION","PROGRAM-ID","ENVIRONMENT","DATA",
        "WORKING-STORAGE","SECTION","PROCEDURE","MOVE","TO","COMPUTE",
        "DISPLAY","PERFORM","IF","ELSE","END-IF","STOP","RUN","PIC","VALUE",
        "INTEGER","STRING","IDENT","DOT","EQUALS","PLUS","MINUS","STAR",
        "SLASH","LPAREN","RPAREN","EOF","UNKNOWN"
    };
    return names[t];
}
