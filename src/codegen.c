#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include <stdint.h>
#include "utils.h"

// simple codegen that assumes parameters are in SysV registers
// maps locals to stack slots; collects assigned locals

typedef struct VarList { char *name; int index; struct VarList *next; } VarList;

static int findVarIndex(VarList *v, const char *name) {
    for (VarList *p=v; p; p=p->next) if (strcmp(p->name, name)==0) return p->index;
    return -1;
}

static void addVar(VarList **v, char *name, int idx) {
    VarList *n = malloc(sizeof(VarList)); n->name = name; n->index = idx; n->next = *v; *v = n;
}

// generate code for expr leaving value in %rax
static void genExpr(FILE *out, Expr *e, VarList *locals);

static void genBinOp(FILE *out, Expr *e, VarList *locals) {
    // evaluate left -> rax, push, right -> rax, pop rbx, rax = rbx op rax
    genExpr(out, e->binop.left, locals);
    fprintf(out, "    pushq %%rax\n");
    genExpr(out, e->binop.right, locals);
    fprintf(out, "    popq %%rbx\n");
    switch (e->binop.op) {
        case BIN_ADD: fprintf(out, "    addq %%rbx, %%rax\n    movq %%rax, %%rax\n"); break;
        case BIN_SUB: fprintf(out, "    subq %%rax, %%rbx\n    movq %%rbx, %%rax\n"); break;
        case BIN_MUL: fprintf(out, "    imulq %%rbx, %%rax\n"); break;
        case BIN_DIV:
            fprintf(out, "    movq %%rax, %%rdi\n    movq %%rbx, %%rax\n    cqto\n    idivq %%rdi\n"); // rax = rbx / rdi
            break;
        case BIN_MOD:
            fprintf(out, "    movq %%rax, %%rdi\n    movq %%rbx, %%rax\n    cqto\n    idivq %%rdi\n    movq %%rdx, %%rax\n");
            break;
        case BIN_BAND:
            fprintf(out, "    andq %%rbx, %%rax\n");
            break;
        case BIN_BOR:
            fprintf(out, "    orq %%rbx, %%rax\n");
            break;
        case BIN_BXOR:
            fprintf(out, "    xorq %%rbx, %%rax\n");
            break;
        case BIN_SHL:
            // rax = left (rbx) shl right (rax)
            fprintf(out, "    movq %%rax, %%rcx\n");
            fprintf(out, "    movq %%rbx, %%rax\n");
            fprintf(out, "    shlq %%cl, %%rax\n");
            break;
        case BIN_SHR:
            // logical right shift (zero-fill)
            fprintf(out, "    movq %%rax, %%rcx\n");
            fprintf(out, "    movq %%rbx, %%rax\n");
            fprintf(out, "    shrq %%cl, %%rax\n");
            break;
        default:
            fprintf(out, "    movq $0, %%rax\n");
            break;
    }
}

static void genExpr(FILE *out, Expr *e, VarList *locals) {
    if (!e) return;
    switch (e->kind) {
        case EX_INT:
            fprintf(out, "    movq $%lld, %%rax\n", (long long)e->intValue); break;
        case EX_VAR: {
            int idx = findVarIndex(locals, e->varName);
            if (idx>=0) {
                int offset = 8*(idx+1);
                fprintf(out, "    movq -%d(%%rbp), %%rax\n", offset);
            } else {
                // could be global mem symbol
                fprintf(out, "    // unknown var %s\n", e->varName);
                fprintf(out, "    movq $0, %%rax\n");
            }
            break;
        }
        case EX_ADDR:
            fprintf(out, "    leaq %s(%%rip), %%rax\n", e->addrName); break;
        case EX_INDEX: {
            // only support mem[...] where arr is var 'mem'
            if (e->index.arr->kind==EX_VAR && strcmp(e->index.arr->varName,"mem")==0) {
                // evaluate index into rax
                genExpr(out, e->index.index, locals);
                // rax = index
                fprintf(out, "    movq %%rax, %%rsi\n"); // index in rsi
                fprintf(out, "    movq mem(%%rip), %%rbx\n");
                fprintf(out, "    leaq (%%rbx,%%rsi,8), %%rcx\n");
                fprintf(out, "    movq (%%rcx), %%rax\n");
            } else {
                fprintf(out, "    movq $0, %%rax\n");
            }
            break;
        }
        case EX_CALL: {
            // special builtin: __mem_store(index, value)
            if (e->call.fn->kind==EX_VAR && strcmp(e->call.fn->varName,"__mem_store")==0) {
                // args[0]=index, args[1]=value
                genExpr(out, e->call.args[0], locals); // index -> rax
                fprintf(out, "    movq %%rax, %%rsi\n"); // index in rsi
                genExpr(out, e->call.args[1], locals); // value -> rax
                fprintf(out, "    movq %%rax, %%rdx\n"); // value in rdx
                fprintf(out, "    movq mem(%%rip), %%rbx\n");
                fprintf(out, "    leaq (%%rbx,%%rsi,8), %%rcx\n");
                fprintf(out, "    movq %%rdx, (%%rcx)\n");
                return;
            }
            // evaluate args into registers RDI, RSI, RDX, RCX, R8, R9
            const char *regs[] = {"%rdi","%rsi","%rdx","%rcx","%r8","%r9"};
            for (int i=0;i<e->call.argCount && i<6;i++) {
                genExpr(out, e->call.args[i], locals);
                fprintf(out, "    movq %%rax, %s\n", regs[i]);
            }
            // if function expression is var (direct call)
            if (e->call.fn->kind==EX_VAR) {
                fprintf(out, "    call %s\n", e->call.fn->varName);
            } else {
                // indirect: need to preserve argument registers while evaluating function pointer
                int n = e->call.argCount < 6 ? e->call.argCount : 6;
                // push regs in reverse so we can pop into correct order
                for (int i = n-1; i>=0; --i) {
                    fprintf(out, "    pushq %s\n", regs[i]);
                }
                genExpr(out, e->call.fn, locals); // result in rax (fn ptr)
                // restore args into registers
                for (int i=0;i<n;i++) {
                    fprintf(out, "    popq %s\n", regs[i]);
                }
                fprintf(out, "    call *%%rax\n");
            }
            break;
        }
        case EX_BINOP:
            genBinOp(out, e, locals); break;
        default:
            fprintf(out, "    movq $0, %%rax\n"); break;
    }
}

// generate function: map params+locals to slots
void genFunction(FILE *out, Function *f) {
    // collect locals: params first
    VarList *locals = NULL;
    for (int i=0;i<f->paramCount;i++) addVar(&locals, strDup(f->params[i]), i);
    // scan body for assigned vars
    Stmt *s;
    int localCount = f->paramCount;
    for (s=f->body;s;s=s->next) {
        if (s->kind==NODE_STMT_ASSIGN) {
            if (findVarIndex(locals, s->assign.lhs)<0) {
                addVar(&locals, strDup(s->assign.lhs), localCount++);
            }
        }
    }
    int stackSize = localCount*8;

    // function label: if name is main -> lang_main
    const char *fname = strcmp(f->name,"main")==0 ? "lang_main" : f->name;
    fprintf(out, "    .globl %s\n", fname);
    fprintf(out, "    .type %s, @function\n", fname);
    fprintf(out, "%s:\n", fname);
    fprintf(out, "    pushq %%rbp\n    movq %%rsp, %%rbp\n");
    if (stackSize>0) fprintf(out, "    subq $%d, %%rsp\n", stackSize);
    // move params from registers into locals
    const char *pRegs[] = {"%rdi","%rsi","%rdx","%rcx","%r8","%r9"};
    for (int i=0;i<f->paramCount;i++) {
        int offset = 8*(i+1);
        fprintf(out, "    movq %s, -%d(%%rbp)\n", pRegs[i], offset);
    }
    // zero uninitialized locals
    for (int i=f->paramCount;i<localCount;i++) {
        int offset = 8*(i+1);
        fprintf(out, "    movq $0, -%d(%%rbp)\n", offset);
    }

    // emit statements
    for (s=f->body;s;s=s->next) {
        if (s->kind==NODE_STMT_ASSIGN) {
            // generate rhs -> rax
            genExpr(out, s->assign.rhs, locals);
            // store into local or mem
            if (strcmp(s->assign.lhs,"mem")==0) {
                // not supported direct; expect mem[index] usage instead
            } else {
                int idx = findVarIndex(locals, s->assign.lhs);
                if (idx>=0) {
                    int offset = 8*(idx+1);
                    fprintf(out, "    movq %%rax, -%d(%%rbp)\n", offset);
                } else {
                    // unknown lhs
                }
            }
        } else if (s->kind==NODE_STMT_RETURN) {
            genExpr(out, s->retExpr, locals);
            // epilogue
            if (stackSize>0) fprintf(out, "    addq $%d, %%rsp\n", stackSize);
            fprintf(out, "    popq %%rbp\n    ret\n");
        } else if (s->kind==NODE_STMT_EXPR) {
            // expression statement (e.g., mem[0] = &add handled as assign generally)
            // if it's a call, evaluate
            if (s->exprStmt->kind==EX_CALL) {
                genExpr(out, s->exprStmt, locals);
            }
        }
    }
    // default return 0
    fprintf(out, "    movq $0, %%rax\n");
    if (stackSize>0) fprintf(out, "    addq $%d, %%rsp\n", stackSize);
    fprintf(out, "    popq %%rbp\n    ret\n");
}

void genProgram(const char *outPath, Program *prog) {
    FILE *out = fopen(outPath,"w");
    if (!out) { perror("fopen"); exit(1); }
    fprintf(out, ".text\n");
    for (Function *f = prog->functions; f; f=f->next) {
        genFunction(out, f);
    }
    // do not emit mem symbol here; runtime provides mem and memArray
    fclose(out);
}

