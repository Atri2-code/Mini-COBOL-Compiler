#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Cannot open '%s'\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

static void usage(const char *argv0) {
    fprintf(stderr, "Usage: %s <input.cbl> [-o <output.asm>] [--ast] [--tokens]\n", argv0);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 2) usage(argv[0]);

    const char *input   = NULL;
    const char *output  = "out.asm";
    int show_ast    = 0;
    int show_tokens = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i+1 < argc) { output = argv[++i]; }
        else if (strcmp(argv[i], "--ast") == 0)        { show_ast = 1; }
        else if (strcmp(argv[i], "--tokens") == 0)     { show_tokens = 1; }
        else                                            { input = argv[i]; }
    }
    if (!input) usage(argv[0]);

    char *src = read_file(input);

    /* ── Token dump mode ───────────────────────────────────────────────── */
    if (show_tokens) {
        Lexer l; lexer_init(&l, src);
        Token t;
        while ((t = lexer_next(&l)).type != TOK_EOF)
            printf("L%-4d %-20s '%s'\n", t.line, token_type_name(t.type), t.text);
        free(src); return 0;
    }

    /* ── Parse ─────────────────────────────────────────────────────────── */
    Parser p;
    parser_init(&p, src);
    ASTNode *prog = parser_parse(&p);

    if (p.errors) {
        fprintf(stderr, "%d parse error(s). Aborting.\n", p.errors);
        ast_free(prog); free(src); return 1;
    }

    if (show_ast) { ast_print(prog, 0); }

    /* ── Semantic analysis ─────────────────────────────────────────────── */
    SymTable st;
    symtable_init(&st);
    semantic_check(prog, &st);

    if (st.errors) {
        fprintf(stderr, "%d semantic error(s). Aborting.\n", st.errors);
        ast_free(prog); symtable_free(&st); free(src); return 1;
    }

    /* ── Code generation ───────────────────────────────────────────────── */
    FILE *out = fopen(output, "w");
    if (!out) { fprintf(stderr, "Cannot write '%s'\n", output); return 1; }
    codegen(prog, &st, out);
    fclose(out);

    printf("Compiled '%s' -> '%s'\n", input, output);
    printf("Assemble with: nasm -f elf64 %s -o out.o && ld out.o -o out\n", output);

    ast_free(prog);
    symtable_free(&st);
    free(src);
    return 0;
}
