#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/parser.h"

static int passed = 0, failed = 0;
#define ASSERT(cond, msg) \
    do { if (cond) { passed++; } \
    else { fprintf(stderr, "FAIL: %s\n", msg); failed++; } } while(0)

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f); buf[sz] = '\0'; fclose(f);
    return buf;
}

void test_parse_hello(void) {
    char *src = read_file("samples/hello.cbl");
    Parser p; parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);
    ASSERT(p.errors == 0,       "hello: no parse errors");
    ASSERT(prog != NULL,        "hello: program node exists");
    ASSERT(prog->left  != NULL, "hello: has data items");
    ASSERT(prog->right != NULL, "hello: has statements");
    free(src); ast_free(prog);
}

void test_parse_payroll(void) {
    char *src = read_file("samples/payroll.cbl");
    Parser p; parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);
    ASSERT(p.errors == 0,       "payroll: no parse errors");
    ASSERT(prog->left  != NULL, "payroll: has data items");
    ASSERT(prog->right != NULL, "payroll: has statements");
    int count = 0;
    for (ASTNode *d = prog->left; d; d = d->next) count++;
    ASSERT(count >= 4, "payroll: has 4+ data items");
    free(src); ast_free(prog);
}

void test_data_item_found(void) {
    char *src = read_file("samples/hello.cbl");
    Parser p; parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);
    int found = 0;
    for (ASTNode *d = prog->left; d; d = d->next)
        if (strcmp(d->sval, "WS-NUM1") == 0) { found = 1; break; }
    ASSERT(found, "WS-NUM1 data item found");
    free(src); ast_free(prog);
}

void test_first_stmt_move(void) {
    char *src = read_file("samples/hello.cbl");
    Parser p; parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);
    ASSERT(prog->right != NULL, "has first statement");
    if (prog->right)
        ASSERT(prog->right->type == NODE_MOVE, "first stmt is MOVE");
    free(src); ast_free(prog);
}

void test_compute_stmt(void) {
    char *src = read_file("samples/hello.cbl");
    Parser p; parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);
    /* Walk to 3rd statement (COMPUTE) */
    ASTNode *s = prog->right;
    int i = 0;
    while (s && i < 2) { s = s->next; i++; }
    ASSERT(s != NULL, "3rd statement exists");
    if (s) ASSERT(s->type == NODE_COMPUTE, "3rd stmt is COMPUTE");
    free(src); ast_free(prog);
}

int main(void) {
    printf("=== Parser Tests ===\n");
    test_parse_hello();
    test_parse_payroll();
    test_data_item_found();
    test_first_stmt_move();
    test_compute_stmt();
    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
