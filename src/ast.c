#include "ast.h"
#include <stdlib.h>
#include <string.h>

Expr *newIntExpr(int64_t v) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EX_INT; e->intValue = v; return e;
}
Expr *newVarExpr(char *name) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EX_VAR; e->varName = name; return e;
}
Expr *newBinOpExpr(char op, Expr *l, Expr *r) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EX_BINOP; e->binop.op = op; e->binop.left = l; e->binop.right = r; return e;
}
Expr *newCallExpr(Expr *fn, Expr **args, int argCount) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EX_CALL; e->call.fn = fn; e->call.args = args; e->call.argCount = argCount; return e;
}
Expr *newAddrExpr(char *name) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EX_ADDR; e->addrName = name; return e;
}
Expr *newIndexExpr(Expr *arr, Expr *index) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EX_INDEX; e->index.arr = arr; e->index.index = index; return e;
}

Stmt *newAssignStmt(char *lhs, Expr *rhs) {
    Stmt *s = calloc(1, sizeof(Stmt)); s->kind = NODE_STMT_ASSIGN; s->assign.lhs = lhs; s->assign.rhs = rhs; return s;
}
Stmt *newReturnStmt(Expr *e) {
    Stmt *s = calloc(1, sizeof(Stmt)); s->kind = NODE_STMT_RETURN; s->retExpr = e; return s;
}
Stmt *newExprStmt(Expr *e) {
    Stmt *s = calloc(1, sizeof(Stmt)); s->kind = NODE_STMT_EXPR; s->exprStmt = e; return s;
}

Function *newFunction(char *name, char **params, int paramCount, Stmt *body) {
    Function *f = calloc(1, sizeof(Function));
    f->name = name; f->params = params; f->paramCount = paramCount; f->body = body; f->next = NULL; return f;
}

Program *newProgram(void) {
    Program *p = calloc(1, sizeof(Program)); p->functions = NULL; return p;
}

void freeProgram(Program *p) {
    // minimal free - omit for brevity
    (void)p;
}

