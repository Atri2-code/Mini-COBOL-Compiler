#include <stdio.h>
#include "../src/parser.h"
#include "../src/semantic.h"

static int passed = 0, failed = 0;
#define ASSERT(cond, msg) \
    do { if (cond) { passed++; } \
    else { fprintf(stderr, "FAIL: %s\n", msg); failed++; } } while(0)

void test_undeclared_variable(void) {
    const char *src =
        "IDENTIFICATION DIVISION.\n"
        "PROGRAM-ID. TEST.\n"
        "DATA DIVISION.\n"
        "WORKING-STORAGE SECTION.\n"
        "01 WS-A PIC 9(4) VALUE 0.\n"
        "PROCEDURE DIVISION.\n"
        "    MOVE 1 TO WS-UNDECLARED\n"  /* undeclared */
        "    STOP RUN.\n";
    Parser p; parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);
    SymTable st; symtable_init(&st);
    semantic_check(prog, &st);
    ASSERT(st.errors > 0, "undeclared variable detected");
    ast_free(prog); symtable_free(&st);
}

void test_declared_variables_ok(void) {
    const char *src =
        "IDENTIFICATION DIVISION.\n"
        "PROGRAM-ID. TEST.\n"
        "DATA DIVISION.\n"
        "WORKING-STORAGE SECTION.\n"
        "01 WS-A PIC 9(4) VALUE 0.\n"
        "01 WS-B PIC 9(4) VALUE 0.\n"
        "01 WS-C PIC 9(8) VALUE 0.\n"
        "PROCEDURE DIVISION.\n"
        "    MOVE 10 TO WS-A\n"
        "    MOVE 20 TO WS-B\n"
        "    COMPUTE WS-C = WS-A + WS-B\n"
        "    DISPLAY WS-C\n"
        "    STOP RUN.\n";
    Parser p; parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);
    SymTable st; symtable_init(&st);
    semantic_check(prog, &st);
    ASSERT(st.errors == 0, "all declared variables pass semantic check");
    ast_free(prog); symtable_free(&st);
}

int main(void) {
    printf("=== Semantic Tests ===\n");
    test_undeclared_variable();
    test_declared_variables_ok();
    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
