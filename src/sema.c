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
            if (!isDefined(defs, e->addrName) && !isDefined(funcs, e->addrName)) {
                fprintf(stderr,"semantic error: address-of undefined name '%s'\n", e->addrName);
                return 0;
            }
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

static int checkStmt(Stmt *s, Def **defs, Def *funcs);

static int checkStmtList(Stmt *s, Def **defs, Def *funcs) {
    for (Stmt *p = s; p; p = p[0].next) {
        if (!checkStmt(p, defs, funcs)) return 0;
    }
    return 1;
}

static int checkStmt(Stmt *s, Def **defs, Def *funcs) {
    if (!s) return 1;
    if (s[0].kind==NODE_STMT_ASSIGN) {
        if (!checkExpr(s[0].assign.rhs, defs[0], funcs)) return 0;
        addDef(defs, s[0].assign.lhs);
        return 1;
    }
    if (s[0].kind==NODE_STMT_RETURN) {
        return checkExpr(s[0].retExpr, defs[0], funcs);
    }
    if (s[0].kind==NODE_STMT_EXPR) {
        if (s[0].exprStmt->kind==EX_CALL && s[0].exprStmt->call.fn->kind==EX_VAR &&
            strcmp(s[0].exprStmt->call.fn->varName,"__mem_store")==0) {
            if (!checkExpr(s[0].exprStmt->call.args[0], defs[0], funcs)) return 0;
            if (!checkExpr(s[0].exprStmt->call.args[1], defs[0], funcs)) return 0;
            return 1;
        }
        if (s[0].exprStmt->kind==EX_CALL && s[0].exprStmt->call.fn->kind==EX_VAR &&
            strcmp(s[0].exprStmt->call.fn->varName,"__index_store")==0) {
            if (!checkExpr(s[0].exprStmt->call.args[0], defs[0], funcs)) return 0;
            if (!checkExpr(s[0].exprStmt->call.args[1], defs[0], funcs)) return 0;
            if (!checkExpr(s[0].exprStmt->call.args[2], defs[0], funcs)) return 0;
            return 1;
        }
        return checkExpr(s[0].exprStmt, defs[0], funcs);
    }
    if (s[0].kind==NODE_STMT_BLOCK) {
        return checkStmtList(s[0].blockBody, defs, funcs);
    }
    if (s[0].kind==NODE_STMT_IF) {
        if (!checkExpr(s[0].ifStmt.cond, defs[0], funcs)) return 0;
        if (!checkStmt(s[0].ifStmt.thenBranch, defs, funcs)) return 0;
        if (s[0].ifStmt.elseBranch) {
            if (!checkStmt(s[0].ifStmt.elseBranch, defs, funcs)) return 0;
        }
        return 1;
    }
    if (s[0].kind==NODE_STMT_WHILE) {
        if (!checkExpr(s[0].whileStmt.cond, defs[0], funcs)) return 0;
        return checkStmt(s[0].whileStmt.body, defs, funcs);
    }
    return 1;
}

int semaCheck(Program *p) {
    // collect global function names
    Def *funcs = NULL;
    for (Function *ff = p->functions; ff; ff = ff->next) addDef(&funcs, ff->name);
    // runtime-provided helpers callable from stdlib
    addDef(&funcs, "rt_put_int");
    addDef(&funcs, "rt_get_int");
    addDef(&funcs, "rt_exit");
    // internal compiler-lowered helpers
    addDef(&funcs, "__mem_store");
    addDef(&funcs, "__index_store");

    for (Function *f = p->functions; f; f=f->next) {
        Def *defs = NULL;
        // params are defined
        for (int i=0;i<f->paramCount;i++) addDef(&defs, f->params[i]);
        if (!checkStmt(f->body, &defs, funcs)) return 0;
    }
    return 1;
}

