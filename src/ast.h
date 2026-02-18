#ifndef AST_H
#define AST_H

#include <stdint.h>

typedef enum {
    NODE_FUNC,
    NODE_STMT_ASSIGN,
    NODE_STMT_RETURN,
    NODE_STMT_EXPR,
    NODE_STMT_BLOCK,
    NODE_STMT_IF,
    NODE_STMT_WHILE
} NodeKind;

typedef enum { EX_INT, EX_VAR, EX_BINOP, EX_CALL, EX_ADDR, EX_INDEX } ExprKind;

typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_MOD,
    BIN_EQ,
    BIN_NEQ,
    BIN_LT,
    BIN_GT,
    BIN_LE,
    BIN_GE
} BinOpKind;

typedef struct Expr {
    ExprKind kind;
    union {
        int64_t intValue;
        char *varName;
        struct { BinOpKind op; struct Expr *left; struct Expr *right; } binop;
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
        struct { struct Expr *cond; struct Stmt *thenBranch; struct Stmt *elseBranch; } ifStmt;
        struct { struct Expr *cond; struct Stmt *body; } whileStmt;
        struct Stmt *blockBody;
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
Expr *newBinOpExpr(BinOpKind op, Expr *l, Expr *r);
Expr *newCallExpr(Expr *fn, Expr **args, int argCount);
Expr *newAddrExpr(char *name);
Expr *newIndexExpr(Expr *arr, Expr *index);

Stmt *newAssignStmt(char *lhs, Expr *rhs);
Stmt *newReturnStmt(Expr *e);
Stmt *newExprStmt(Expr *e);
Stmt *newBlockStmt(Stmt *body);
Stmt *newIfStmt(Expr *cond, Stmt *thenBranch, Stmt *elseBranch);
Stmt *newWhileStmt(Expr *cond, Stmt *body);

Function *newFunction(char *name, char **params, int paramCount, Stmt *body);
Program *newProgram(void);

void freeProgram(Program *p);

#endif

