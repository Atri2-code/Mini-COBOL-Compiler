#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* Each numeric variable lives at a qword on the stack.
   We track offset from rbp. */
typedef struct VarOffset {
    char name[256];
    int  offset; /* negative, from rbp */
    struct VarOffset *next;
} VarOffset;

typedef struct {
    FILE      *out;
    SymTable  *st;
    VarOffset *vars;
    int        next_offset; /* grows downward, starts at -8 */
    int        label_count;
} CG;

static void cg_init(CG *cg, FILE *out, SymTable *st) {
    cg->out         = out;
    cg->st          = st;
    cg->vars        = NULL;
    cg->next_offset = 0;
    cg->label_count = 0;
}

static int var_offset(CG *cg, const char *name) {
    for (VarOffset *v = cg->vars; v; v = v->next)
        if (strcmp(v->name, name) == 0) return v->offset;
    /* allocate new slot */
    cg->next_offset -= 8;
    VarOffset *v = calloc(1, sizeof(VarOffset));
    strncpy(v->name, name, sizeof(v->name)-1);
    v->offset = cg->next_offset;
    v->next   = cg->vars;
    cg->vars  = v;
    return v->offset;
}

static int new_label(CG *cg) { return cg->label_count++; }

/* Emit expression — result ends up in rax */
static void emit_expr(CG *cg, ASTNode *n) {
    if (!n) return;
    switch (n->type) {
        case NODE_INTEGER:
            fprintf(cg->out, "    mov rax, %d\n", n->ival);
            break;
        case NODE_IDENT: {
            int off = var_offset(cg, n->sval);
            fprintf(cg->out, "    mov rax, [rbp%+d]\n", off);
            break;
        }
        case NODE_BINOP:
            emit_expr(cg, n->left);
            fprintf(cg->out, "    push rax\n");
            emit_expr(cg, n->right);
            fprintf(cg->out, "    mov rbx, rax\n");
            fprintf(cg->out, "    pop rax\n");
            switch (n->sval[0]) {
                case '+': fprintf(cg->out, "    add rax, rbx\n"); break;
                case '-': fprintf(cg->out, "    sub rax, rbx\n"); break;
                case '*': fprintf(cg->out, "    imul rax, rbx\n"); break;
                case '/':
                    fprintf(cg->out, "    cqo\n");
                    fprintf(cg->out, "    idiv rbx\n");
                    break;
            }
            break;
        default: break;
    }
}

/* Emit a print_int helper call (rdi = value) */
static void emit_print_int(CG *cg) {
    /* Use Linux syscall write via a small itoa routine inlined */
    fprintf(cg->out,
        "    ; --- DISPLAY (integer) ---\n"
        "    mov rdi, rax\n"
        "    call _print_int\n");
}

static void emit_stmt(CG *cg, ASTNode *n);

static void emit_stmt_list(CG *cg, ASTNode *n) {
    for (; n; n = n->next) emit_stmt(cg, n);
}

static void emit_stmt(CG *cg, ASTNode *n) {
    if (!n) return;
    switch (n->type) {
        case NODE_MOVE: {
            emit_expr(cg, n->left);
            int off = var_offset(cg, n->right->sval);
            fprintf(cg->out, "    mov [rbp%+d], rax\n", off);
            break;
        }
        case NODE_COMPUTE: {
            emit_expr(cg, n->right);
            int off = var_offset(cg, n->left->sval);
            fprintf(cg->out, "    mov [rbp%+d], rax\n", off);
            break;
        }
        case NODE_DISPLAY:
            emit_expr(cg, n->left);
            emit_print_int(cg);
            break;
        case NODE_IF: {
            int lbl_else = new_label(cg);
            int lbl_end  = new_label(cg);
            emit_expr(cg, n->cond);
            fprintf(cg->out, "    cmp rax, 0\n");
            fprintf(cg->out, "    je .L%d\n", lbl_else);
            emit_stmt_list(cg, n->then_branch);
            fprintf(cg->out, "    jmp .L%d\n", lbl_end);
            fprintf(cg->out, ".L%d:\n", lbl_else);
            emit_stmt_list(cg, n->else_branch);
            fprintf(cg->out, ".L%d:\n", lbl_end);
            break;
        }
        case NODE_STOP:
            fprintf(cg->out,
                "    ; STOP RUN\n"
                "    mov rax, 60\n"
                "    xor rdi, rdi\n"
                "    syscall\n");
            break;
        default: break;
    }
}

/* Initialise declared variables to their VALUE, or 0 */
static void emit_data_init(CG *cg, ASTNode *data) {
    for (ASTNode *d = data; d; d = d->next) {
        if (d->type != NODE_DATA_ITEM) continue;
        int off = var_offset(cg, d->sval);
        if (d->left && d->left->type == NODE_INTEGER) {
            fprintf(cg->out, "    mov qword [rbp%+d], %d\n", off, d->left->ival);
        } else {
            fprintf(cg->out, "    mov qword [rbp%+d], 0\n", off);
        }
    }
}

void codegen(ASTNode *prog, SymTable *st, FILE *out) {
    CG cg;
    cg_init(&cg, out, st);

    /* Count variables to reserve stack space */
    int var_count = 0;
    for (ASTNode *d = prog->left; d; d = d->next)
        if (d->type == NODE_DATA_ITEM) var_count++;
    int stack_space = (var_count + 4) * 8; /* round up generously */
    if (stack_space % 16) stack_space += 8; /* 16-byte align */

    fprintf(out,
        "bits 64\n"
        "global _start\n\n"
        "section .text\n\n"
        "; _print_int: prints integer in rdi followed by newline\n"
        "_print_int:\n"
        "    push rbp\n"
        "    mov rbp, rsp\n"
        "    sub rsp, 32\n"
        "    mov rax, rdi\n"
        "    lea rsi, [rbp-20]\n"
        "    mov byte [rsi], 0x0a\n"  /* newline at end */
        "    dec rsi\n"
        "    mov rcx, 10\n"
        "    test rax, rax\n"
        "    jns .pos\n"
        "    neg rax\n"
        ".pos:\n"
        "    xor rdx, rdx\n"
        "    div rcx\n"
        "    add dl, '0'\n"
        "    mov [rsi], dl\n"
        "    dec rsi\n"
        "    test rax, rax\n"
        "    jnz .pos\n"
        "    inc rsi\n"
        "    lea rdx, [rbp-19]\n"
        "    sub rdx, rsi\n"
        "    inc rdx\n"
        "    mov rax, 1\n"
        "    mov rdi, 1\n"
        "    syscall\n"
        "    leave\n"
        "    ret\n\n"
        "_start:\n"
        "    push rbp\n"
        "    mov rbp, rsp\n"
        "    sub rsp, %d\n\n", stack_space);

    emit_data_init(&cg, prog->left);
    fprintf(out, "\n");
    emit_stmt_list(&cg, prog->right);

    /* Default exit if no STOP RUN */
    fprintf(out,
        "    mov rax, 60\n"
        "    xor rdi, rdi\n"
        "    syscall\n");
}
