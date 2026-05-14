#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

static void advance(Parser *p) { p->current = lexer_next(&p->lexer); }

static int check(Parser *p, TokenType t) { return p->current.type == t; }

static int expect(Parser *p, TokenType t) {
    if (check(p, t)) { advance(p); return 1; }
    fprintf(stderr, "Line %d: expected %s, got '%s'\n",
            p->current.line, token_type_name(t), p->current.text);
    p->errors++;
    return 0;
}

/* Forward declarations */
static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_expr(Parser *p);

/* ── Expression ─────────────────────────────────────────────────────────── */

static ASTNode *parse_primary(Parser *p) {
    ASTNode *n;
    if (check(p, TOK_INTEGER)) {
        n = ast_node(NODE_INTEGER);
        n->ival = atoi(p->current.text);
        advance(p);
        return n;
    }
    if (check(p, TOK_STRING)) {
        n = ast_node(NODE_STRING);
        strncpy(n->sval, p->current.text, sizeof(n->sval)-1);
        advance(p);
        return n;
    }
    if (check(p, TOK_IDENT)) {
        n = ast_node(NODE_IDENT);
        strncpy(n->sval, p->current.text, sizeof(n->sval)-1);
        advance(p);
        return n;
    }
    if (check(p, TOK_LPAREN)) {
        advance(p);
        n = parse_expr(p);
        expect(p, TOK_RPAREN);
        return n;
    }
    fprintf(stderr, "Line %d: unexpected token '%s' in expression\n",
            p->current.line, p->current.text);
    p->errors++;
    advance(p);
    return ast_node(NODE_INTEGER); /* error recovery */
}

static ASTNode *parse_term(Parser *p) {
    ASTNode *left = parse_primary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH)) {
        ASTNode *op = ast_node(NODE_BINOP);
        op->sval[0] = check(p, TOK_STAR) ? '*' : '/';
        op->sval[1] = '\0';
        advance(p);
        op->left  = left;
        op->right = parse_primary(p);
        left = op;
    }
    return left;
}

static ASTNode *parse_expr(Parser *p) {
    ASTNode *left = parse_term(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        ASTNode *op = ast_node(NODE_BINOP);
        op->sval[0] = check(p, TOK_PLUS) ? '+' : '-';
        op->sval[1] = '\0';
        advance(p);
        op->left  = left;
        op->right = parse_term(p);
        left = op;
    }
    return left;
}

/* ── Statements ──────────────────────────────────────────────────────────── */

static ASTNode *parse_move(Parser *p) {
    advance(p); /* consume MOVE */
    ASTNode *n = ast_node(NODE_MOVE);
    n->left = parse_expr(p);
    expect(p, TOK_TO);
    n->right = ast_node(NODE_IDENT);
    strncpy(n->right->sval, p->current.text, sizeof(n->right->sval)-1);
    expect(p, TOK_IDENT);
    return n;
}

static ASTNode *parse_compute(Parser *p) {
    advance(p); /* consume COMPUTE */
    ASTNode *n  = ast_node(NODE_COMPUTE);
    n->left = ast_node(NODE_IDENT);
    strncpy(n->left->sval, p->current.text, sizeof(n->left->sval)-1);
    expect(p, TOK_IDENT);
    expect(p, TOK_EQUALS);
    n->right = parse_expr(p);
    return n;
}

static ASTNode *parse_display(Parser *p) {
    advance(p); /* consume DISPLAY */
    ASTNode *n = ast_node(NODE_DISPLAY);
    n->left = parse_expr(p);
    return n;
}

static ASTNode *parse_perform(Parser *p) {
    advance(p); /* consume PERFORM */
    ASTNode *n = ast_node(NODE_PERFORM);
    strncpy(n->sval, p->current.text, sizeof(n->sval)-1);
    expect(p, TOK_IDENT);
    return n;
}

static ASTNode *parse_if(Parser *p) {
    advance(p); /* consume IF */
    ASTNode *n = ast_node(NODE_IF);
    n->cond = parse_expr(p);
    /* then block */
    ASTNode *head = NULL, *tail = NULL;
    while (!check(p, TOK_ELSE) && !check(p, TOK_END_IF) && !check(p, TOK_EOF)) {
        ASTNode *s = parse_statement(p);
        if (!s) break;
        if (!head) head = tail = s;
        else { tail->next = s; tail = s; }
    }
    n->then_branch = head;
    if (check(p, TOK_ELSE)) {
        advance(p);
        head = tail = NULL;
        while (!check(p, TOK_END_IF) && !check(p, TOK_EOF)) {
            ASTNode *s = parse_statement(p);
            if (!s) break;
            if (!head) head = tail = s;
            else { tail->next = s; tail = s; }
        }
        n->else_branch = head;
    }
    expect(p, TOK_END_IF);
    return n;
}

static ASTNode *parse_statement(Parser *p) {
    switch (p->current.type) {
        case TOK_MOVE:    return parse_move(p);
        case TOK_COMPUTE: return parse_compute(p);
        case TOK_DISPLAY: return parse_display(p);
        case TOK_PERFORM: return parse_perform(p);
        case TOK_IF:      return parse_if(p);
        case TOK_STOP: {
            advance(p); expect(p, TOK_RUN);
            return ast_node(NODE_STOP);
        }
        default: return NULL;
    }
}

/* ── PIC clause ──────────────────────────────────────────────────────────── */

static void parse_pic_clause(Parser *p, ASTNode *item) {
    /* After consuming PIC token, current is the type character.
       Tokeniser splits "9(4)" as: INTEGER('9') LPAREN INTEGER('4') RPAREN
       or IDENT('X') LPAREN INTEGER('20') RPAREN */
    char type_char = '\0';
    if (check(p, TOK_INTEGER) && p->current.text[0] == '9') {
        type_char = '9'; advance(p);
    } else if (check(p, TOK_IDENT) &&
               (p->current.text[0] == 'X' || p->current.text[0] == 'x')) {
        type_char = 'X'; advance(p);
    } else {
        return; /* unrecognised PIC type */
    }
    item->pic_type = type_char;
    if (check(p, TOK_LPAREN)) {
        advance(p);
        item->pic_len = atoi(p->current.text);
        if (check(p, TOK_INTEGER)) advance(p);
        if (check(p, TOK_RPAREN))  advance(p);
    } else {
        item->pic_len = 1;
    }
    snprintf(item->pic, sizeof(item->pic), "%c(%d)", type_char, item->pic_len);
}

/* ── Data Division ───────────────────────────────────────────────────────── */

static ASTNode *parse_data_item(Parser *p) {
    ASTNode *item = ast_node(NODE_DATA_ITEM);
    /* level number */
    advance(p); /* skip 01 etc. */
    strncpy(item->sval, p->current.text, sizeof(item->sval)-1);
    expect(p, TOK_IDENT);
    if (check(p, TOK_PIC)) {
        advance(p);
        parse_pic_clause(p, item);
    }
    if (check(p, TOK_VALUE)) {
        advance(p);
        item->left = parse_primary(p);
    }
    /* consume trailing dot if present */
    if (check(p, TOK_DOT)) advance(p);
    return item;
}

/* ── Top-level parse ─────────────────────────────────────────────────────── */

void parser_init(Parser *p, const char *src) {
    lexer_init(&p->lexer, src);
    p->errors = 0;
    advance(p);
}

ASTNode *parser_parse(Parser *p) {
    ASTNode *prog = ast_node(NODE_PROGRAM);

    /* Skip everything until DATA or PROCEDURE DIVISION */
    while (!check(p, TOK_EOF) &&
           !check(p, TOK_DATA) &&
           !check(p, TOK_PROCEDURE)) {
        advance(p);
    }

    /* DATA DIVISION */
    ASTNode *data_head = NULL, *data_tail = NULL;
    if (check(p, TOK_DATA)) {
        advance(p); expect(p, TOK_DIVISION); expect(p, TOK_DOT);
        if (check(p, TOK_WORKING_STORAGE)) {
            advance(p); expect(p, TOK_SECTION); expect(p, TOK_DOT);
            while (check(p, TOK_INTEGER)) { /* level numbers are integers */
                ASTNode *item = parse_data_item(p);
                if (!data_head) data_head = data_tail = item;
                else { data_tail->next = item; data_tail = item; }
            }
        }
    }
    prog->left = data_head;

    /* PROCEDURE DIVISION */
    ASTNode *stmt_head = NULL, *stmt_tail = NULL;
    if (check(p, TOK_PROCEDURE)) {
        advance(p); expect(p, TOK_DIVISION); expect(p, TOK_DOT);
        ASTNode *s;
        while ((s = parse_statement(p)) != NULL) {
            if (!stmt_head) stmt_head = stmt_tail = s;
            else { stmt_tail->next = s; stmt_tail = s; }
        }
    }
    prog->right = stmt_head;

    return prog;
}
