#ifndef AST_H
#define AST_H

#include <stdint.h>

typedef enum { NODE_FUNC, NODE_STMT_ASSIGN, NODE_STMT_RETURN, NODE_STMT_EXPR } NodeKind;

typedef enum { EX_INT, EX_VAR, EX_BINOP, EX_CALL, EX_ADDR, EX_INDEX } ExprKind;

typedef struct Expr {
    ExprKind kind;
    union {
        int64_t intValue;
        char *varName;
        struct { char op; struct Expr *left; struct Expr *right; } binop;
        struct { struct Expr *fn; struct Expr **args; int argCount; } call;
        char *addrName; // for &func
        struct { struct Expr *arr; struct Expr *index; } index;
    };
} Expr;

typedef struct Stmt {
    NodeKind kind;
    union {
        struct { char *lhs; Expr *rhs; } assign;
        Expr *retExpr;
        Expr *exprStmt;
    };
    struct Stmt *next;
} Stmt;

typedef struct Function {
    char *name;
    char **params;
    int paramCount;
    Stmt *body;
    struct Function *next;
} Function;

typedef struct Program {
    Function *functions;
} Program;

// helpers
Expr *newIntExpr(int64_t v);
Expr *newVarExpr(char *name);
Expr *newBinOpExpr(char op, Expr *l, Expr *r);
Expr *newCallExpr(Expr *fn, Expr **args, int argCount);
Expr *newAddrExpr(char *name);
Expr *newIndexExpr(Expr *arr, Expr *index);

Stmt *newAssignStmt(char *lhs, Expr *rhs);
Stmt *newReturnStmt(Expr *e);
Stmt *newExprStmt(Expr *e);

Function *newFunction(char *name, char **params, int paramCount, Stmt *body);
Program *newProgram(void);

void freeProgram(Program *p);

#endif

