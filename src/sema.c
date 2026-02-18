#include "sema.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 #include <stdint.h>
#include "utils.h"

typedef struct Def { char *name; struct Def *next; } Def;

static int isDefined(Def *d, const char *name) {
    for (Def *p=d;p;p=p->next) if (strcmp(p->name,name)==0) return 1;
    return 0;
}

static void addDef(Def **d, const char *name) {
    Def *n = malloc(sizeof(Def)); n->name = strDup(name); n->next = *d; *d = n;
}

static int checkExpr(Expr *e, Def *defs, Def *funcs) {
    if (!e) return 1;
    switch (e->kind) {
        case EX_INT: return 1;
        case EX_VAR:
            if (strcmp(e->varName,"mem")==0) return 1;
            if (!isDefined(defs, e->varName) && !isDefined(funcs, e->varName)) {
                fprintf(stderr,"semantic error: use of undefined variable '%s'\n", e->varName);
                return 0;
            }
            return 1;
        case EX_ADDR:
            return 1;
        case EX_INDEX:
            return checkExpr(e->index.arr, defs, funcs) && checkExpr(e->index.index, defs, funcs);
        case EX_CALL:
            if (!checkExpr(e->call.fn, defs, funcs)) return 0;
            for (int i=0;i<e->call.argCount;i++) if (!checkExpr(e->call.args[i], defs, funcs)) return 0;
            return 1;
        case EX_BINOP:
            return checkExpr(e->binop.left, defs, funcs) && checkExpr(e->binop.right, defs, funcs);
    }
    return 1;
}

int semaCheck(Program *p) {
    // collect global function names
    Def *funcs = NULL;
    for (Function *ff = p->functions; ff; ff = ff->next) addDef(&funcs, ff->name);
    // add builtin 'print' to funcs
    addDef(&funcs, "print");

    for (Function *f = p->functions; f; f=f->next) {
        Def *defs = NULL;
        // params are defined
        for (int i=0;i<f->paramCount;i++) addDef(&defs, f->params[i]);
        // scan statements
        for (Stmt *s=f->body;s;s=s->next) {
            if (s->kind==NODE_STMT_ASSIGN) {
                // check rhs
                if (!checkExpr(s->assign.rhs, defs, funcs)) return 0;
                // define lhs
                addDef(&defs, s->assign.lhs);
            } else if (s->kind==NODE_STMT_RETURN) {
                if (!checkExpr(s->retExpr, defs, funcs)) return 0;
            } else if (s->kind==NODE_STMT_EXPR) {
                if (s->exprStmt->kind==EX_CALL && s->exprStmt->call.fn->kind==EX_VAR &&
                    strcmp(s->exprStmt->call.fn->varName,"__mem_store")==0) {
                    // mem store builtin: args are expressions
                    if (!checkExpr(s->exprStmt->call.args[0], defs, funcs)) return 0;
                    if (!checkExpr(s->exprStmt->call.args[1], defs, funcs)) return 0;
                } else {
                    if (!checkExpr(s->exprStmt, defs, funcs)) return 0;
                }
            }
        }
    }
    return 1;
}

