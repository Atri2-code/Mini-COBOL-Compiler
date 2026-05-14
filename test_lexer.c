#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/lexer.h"

static int passed = 0, failed = 0;

#define ASSERT_EQ(a, b, msg) \
    do { if ((a)==(b)) { passed++; } \
    else { fprintf(stderr, "FAIL [%s]: expected %d got %d\n", msg, (int)(b), (int)(a)); failed++; } } while(0)

#define ASSERT_STR(a, b, msg) \
    do { if (strcmp((a),(b))==0) { passed++; } \
    else { fprintf(stderr, "FAIL [%s]: expected '%s' got '%s'\n", msg, (b), (a)); failed++; } } while(0)

void test_keywords(void) {
    const char *src = "IDENTIFICATION DIVISION MOVE TO COMPUTE DISPLAY IF ELSE END-IF STOP RUN";
    Lexer l; lexer_init(&l, src);
    TokenType expected[] = {
        TOK_IDENTIFICATION, TOK_DIVISION, TOK_MOVE, TOK_TO,
        TOK_COMPUTE, TOK_DISPLAY, TOK_IF, TOK_ELSE, TOK_END_IF,
        TOK_STOP, TOK_RUN, TOK_EOF
    };
    for (int i = 0; expected[i] != TOK_EOF; i++) {
        Token t = lexer_next(&l);
        ASSERT_EQ(t.type, expected[i], "keyword token type");
    }
}

void test_integers(void) {
    Lexer l; lexer_init(&l, "42 1234 0 99999");
    int vals[] = {42, 1234, 0, 99999};
    for (int i = 0; i < 4; i++) {
        Token t = lexer_next(&l);
        ASSERT_EQ(t.type, TOK_INTEGER, "integer token type");
        ASSERT_EQ(atoi(t.text), vals[i], "integer value");
    }
}

void test_identifiers(void) {
    Lexer l; lexer_init(&l, "WS-NUM1 GROSS-PAY MY-VAR");
    const char *names[] = {"WS-NUM1", "GROSS-PAY", "MY-VAR"};
    for (int i = 0; i < 3; i++) {
        Token t = lexer_next(&l);
        ASSERT_EQ(t.type, TOK_IDENT, "ident token type");
        ASSERT_STR(t.text, names[i], "ident text");
    }
}

void test_symbols(void) {
    Lexer l1; lexer_init(&l1, "."); ASSERT_EQ(lexer_next(&l1).type, TOK_DOT,   "dot");
    Lexer l2; lexer_init(&l2, "="); ASSERT_EQ(lexer_next(&l2).type, TOK_EQUALS,"equals");
    Lexer l3; lexer_init(&l3, "+"); ASSERT_EQ(lexer_next(&l3).type, TOK_PLUS,  "plus");
    Lexer l4; lexer_init(&l4, "5-3");
    lexer_next(&l4); /* skip 5 */
    ASSERT_EQ(lexer_next(&l4).type, TOK_MINUS, "minus");
    Lexer l5; lexer_init(&l5, "2*3");
    lexer_next(&l5); /* skip 2 */
    ASSERT_EQ(lexer_next(&l5).type, TOK_STAR,  "star");
    Lexer l6; lexer_init(&l6, "/"); ASSERT_EQ(lexer_next(&l6).type, TOK_SLASH, "slash");
}

void test_string_literal(void) {
    Lexer l; lexer_init(&l, "\"HELLO WORLD\"");
    Token t = lexer_next(&l);
    ASSERT_EQ(t.type, TOK_STRING, "string token type");
    ASSERT_STR(t.text, "HELLO WORLD", "string value");
}

void test_line_numbers(void) {
    Lexer l; lexer_init(&l, "MOVE\nTO\nCOMPUTE");
    Token t;
    t = lexer_next(&l); ASSERT_EQ(t.line, 1, "line 1");
    t = lexer_next(&l); ASSERT_EQ(t.line, 2, "line 2");
    t = lexer_next(&l); ASSERT_EQ(t.line, 3, "line 3");
}

int main(void) {
    printf("=== Lexer Tests ===\n");
    test_keywords();
    test_integers();
    test_identifiers();
    test_symbols();
    test_string_literal();
    test_line_numbers();
    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
