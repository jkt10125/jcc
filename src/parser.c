#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

static void next(Parser *p) { p->cur = lexerNext(&p->lx); }
static int accept(Parser *p, TokenKind k) {
    if (p->cur.kind==k) { next(p); return 1; } return 0;
}
static void expect(Parser *p, TokenKind k) {
    if (p->cur.kind!=k) {
        fprintf(stderr, "parse error: expected token %d but got %d text='%s' at pos %d\n", k, p->cur.kind, p->cur.text?p->cur.text:"", p->lx.pos);
        exit(1);
    }
    next(p);
}

void parserInit(Parser *p, const char *src) {
    lexerInit(&p->lx, src);
    next(p);
}

// forward
static Expr *parseExpr(Parser *p);
static Stmt *parseStmt(Parser *p);

static char *copyIdent(const char *s) { return s ? strDup(s) : NULL; }

static Stmt *parseBlock(Parser *p) {
    expect(p, TOK_LBRACE);
    Stmt *head = NULL, *tail = NULL;
    while (p->cur.kind != TOK_RBRACE && p->cur.kind != TOK_EOF) {
        Stmt *s = parseStmt(p);
        if (!head) head = tail = s; else { tail->next = s; tail = s; }
    }
    expect(p, TOK_RBRACE);
    return newBlockStmt(head);
}

static Expr *parsePrimary(Parser *p) {
    if (p->cur.kind==TOK_NUMBER) {
        Expr *e = newIntExpr(p->cur.num);
        next(p); return e;
    }
    if (p->cur.kind==TOK_IDENT) {
        char *name = copyIdent(p->cur.text);
        next(p);
        // function call?
        if (p->cur.kind==TOK_LPAREN) {
            next(p); // consume '('
            Expr **args = NULL; int ac = 0;
            if (p->cur.kind!=TOK_RPAREN) {
                while (1) {
                    Expr *arg = parseExpr(p);
                    args = realloc(args, sizeof(Expr*)*(ac+1));
                    args[ac++] = arg;
                    if (p->cur.kind==TOK_COMMA) { next(p); continue; }
                    break;
                }
            }
            expect(p, TOK_RPAREN);
            Expr *fn = newVarExpr(name);
            Expr *call = newCallExpr(fn, args, ac);
            return call;
        }
        return newVarExpr(name);
    }
    if (p->cur.kind==TOK_AMP) {
        next(p);
        if (p->cur.kind!=TOK_IDENT) { fprintf(stderr,"& must be followed by ident\n"); exit(1); }
        char *n = strDup(p->cur.text);
        next(p);
        return newAddrExpr(n);
    }
    if (p->cur.kind==TOK_LPAREN) {
        next(p);
        Expr *e = parseExpr(p);
        expect(p, TOK_RPAREN);
        return e;
    }
    fprintf(stderr,"unexpected token in primary\n"); exit(1);
}

// handle index: primary [ expr ]
static Expr *parsePostfix(Parser *p) {
    Expr *e = parsePrimary(p);
    while (p->cur.kind==TOK_LBRACK || p->cur.kind==TOK_LPAREN) {
        if (p->cur.kind==TOK_LBRACK) {
            next(p);
            Expr *idx = parseExpr(p);
            expect(p, TOK_RBRACK);
            e = newIndexExpr(e, idx);
        } else {
            // call postfix: e(args)
            next(p); // consume '('
            Expr **args = NULL; int ac = 0;
            if (p->cur.kind!=TOK_RPAREN) {
                while (1) {
                    Expr *arg = parseExpr(p);
                    args = realloc(args, sizeof(Expr*)*(ac+1));
                    args[ac++] = arg;
                    if (p->cur.kind==TOK_COMMA) { next(p); continue; }
                    break;
                }
            }
            expect(p, TOK_RPAREN);
            e = newCallExpr(e, args, ac);
        }
    }
    return e;
}

static Expr *parseMulDiv(Parser *p) {
    Expr *e = parsePostfix(p);
    while (p->cur.kind==TOK_STAR || p->cur.kind==TOK_SLASH || p->cur.kind==TOK_PERCENT) {
        BinOpKind op = (p->cur.kind==TOK_STAR)?BIN_MUL:(p->cur.kind==TOK_SLASH)?BIN_DIV:BIN_MOD;
        next(p);
        Expr *r = parsePostfix(p);
        e = newBinOpExpr(op,e,r);
    }
    return e;
}

static Expr *parseAddSub(Parser *p) {
    Expr *e = parseMulDiv(p);
    while (p->cur.kind==TOK_PLUS || p->cur.kind==TOK_MINUS) {
        BinOpKind op = (p->cur.kind==TOK_PLUS)?BIN_ADD:BIN_SUB;
        next(p);
        Expr *r = parseMulDiv(p);
        e = newBinOpExpr(op,e,r);
    }
    return e;
}

static Expr *parseShift(Parser *p) {
    Expr *e = parseAddSub(p);
    while (p->cur.kind==TOK_SHL || p->cur.kind==TOK_SHR) {
        BinOpKind op = (p->cur.kind==TOK_SHL)?BIN_SHL:BIN_SHR;
        next(p);
        Expr *r = parseAddSub(p);
        e = newBinOpExpr(op, e, r);
    }
    return e;
}

static Expr *parseBitAnd(Parser *p) {
    Expr *e = parseShift(p);
    while (p->cur.kind==TOK_AMP) {
        next(p);
        Expr *r = parseShift(p);
        e = newBinOpExpr(BIN_BAND, e, r);
    }
    return e;
}

static Expr *parseBitXor(Parser *p) {
    Expr *e = parseBitAnd(p);
    while (p->cur.kind==TOK_CARET) {
        next(p);
        Expr *r = parseBitAnd(p);
        e = newBinOpExpr(BIN_BXOR, e, r);
    }
    return e;
}

static Expr *parseBitOr(Parser *p) {
    Expr *e = parseBitXor(p);
    while (p->cur.kind==TOK_PIPE) {
        next(p);
        Expr *r = parseBitXor(p);
        e = newBinOpExpr(BIN_BOR, e, r);
    }
    return e;
}

static Expr *parseCompare(Parser *p) {
    Expr *e = parseBitOr(p);
    while (p->cur.kind==TOK_EQ || p->cur.kind==TOK_NEQ ||
           p->cur.kind==TOK_LT || p->cur.kind==TOK_GT ||
           p->cur.kind==TOK_LE || p->cur.kind==TOK_GE) {
        BinOpKind op = BIN_EQ;
        if (p->cur.kind==TOK_EQ) op = BIN_EQ;
        else if (p->cur.kind==TOK_NEQ) op = BIN_NEQ;
        else if (p->cur.kind==TOK_LT) op = BIN_LT;
        else if (p->cur.kind==TOK_GT) op = BIN_GT;
        else if (p->cur.kind==TOK_LE) op = BIN_LE;
        else if (p->cur.kind==TOK_GE) op = BIN_GE;
        next(p);
        Expr *r = parseBitOr(p);
        e = newBinOpExpr(op, e, r);
    }
    return e;
}

static Expr *parseExpr(Parser *p) {
    return parseCompare(p);
}

static Stmt *parseStmt(Parser *p) {
    if (p->cur.kind==TOK_LBRACE) {
        return parseBlock(p);
    }
    if (p->cur.kind==TOK_IF) {
        next(p);
        expect(p, TOK_LPAREN);
        Expr *cond = parseExpr(p);
        expect(p, TOK_RPAREN);
        if (p->cur.kind != TOK_LBRACE) {
            fprintf(stderr, "parse error: if-body must be a block { ... }\n");
            exit(1);
        }
        Stmt *thenBranch = parseBlock(p);
        Stmt *elseBranch = NULL;
        if (p->cur.kind==TOK_ELSE) {
            next(p);
            if (p->cur.kind != TOK_LBRACE) {
                fprintf(stderr, "parse error: else-body must be a block { ... }\n");
                exit(1);
            }
            elseBranch = parseBlock(p);
        }
        return newIfStmt(cond, thenBranch, elseBranch);
    }
    if (p->cur.kind==TOK_WHILE) {
        next(p);
        expect(p, TOK_LPAREN);
        Expr *cond = parseExpr(p);
        expect(p, TOK_RPAREN);
        if (p->cur.kind != TOK_LBRACE) {
            fprintf(stderr, "parse error: while-body must be a block { ... }\n");
            exit(1);
        }
        Stmt *body = parseBlock(p);
        return newWhileStmt(cond, body);
    }
    if (p->cur.kind==TOK_RETURN) {
        next(p);
        Expr *e = parseExpr(p);
        expect(p, TOK_SEMI);
        return newReturnStmt(e);
    }
    // assignment or expr
    if (p->cur.kind==TOK_IDENT) {
            char *name = strDup(p->cur.text);
        next(p);
        // handle postfix: index or call
        if (p->cur.kind==TOK_LBRACK) {
            // parse index expression LHS like mem[expr]
            next(p);
            Expr *idx = parseExpr(p);
            expect(p, TOK_RBRACK);
            // assignment?
            if (p->cur.kind==TOK_ASSIGN) {
                next(p);
                Expr *rhs = parseExpr(p);
                expect(p, TOK_SEMI);
                if (strcmp(name, "mem") == 0) {
                    Expr **args = malloc(sizeof(Expr*)*2);
                    args[0] = idx; args[1] = rhs;
                    Expr *storeCall = newCallExpr(newVarExpr(strDup("__mem_store")), args, 2);
                    return newExprStmt(storeCall);
                } else {
                    // generic pointer/array store: base[index] = value
                    Expr **args = malloc(sizeof(Expr*)*3);
                    args[0] = newVarExpr(name); // base address expression (local)
                    args[1] = idx;
                    args[2] = rhs;
                    Expr *storeCall = newCallExpr(newVarExpr(strDup("__index_store")), args, 3);
                    return newExprStmt(storeCall);
                }
            } else {
                // expression stmt of index access
                Expr *arr = newVarExpr(name);
                Expr *idxExpr = newIndexExpr(arr, idx);
                expect(p, TOK_SEMI);
                return newExprStmt(idxExpr);
            }
        }
        // assignment?
        if (p->cur.kind==TOK_ASSIGN) {
            next(p);
            Expr *rhs = parseExpr(p);
            expect(p, TOK_SEMI);
            return newAssignStmt(name, rhs);
        } else if (p->cur.kind==TOK_LPAREN) {
            // function call starting with ident
            next(p);
            Expr **args = NULL; int ac = 0;
            if (p->cur.kind!=TOK_RPAREN) {
                while (1) {
                    Expr *arg = parseExpr(p);
                    args = realloc(args, sizeof(Expr*)*(ac+1));
                    args[ac++] = arg;
                    if (p->cur.kind==TOK_COMMA) { next(p); continue; }
                    break;
                }
            }
            expect(p, TOK_RPAREN);
            Expr *fn = newVarExpr(name);
            Expr *call = newCallExpr(fn, args, ac);
            expect(p, TOK_SEMI);
            return newExprStmt(call);
        } else {
            // expr stmt like `x;`
            Expr *ve = newVarExpr(name);
            expect(p, TOK_SEMI);
            return newExprStmt(ve);
        }
    }
    // other expr stmt
    Expr *e = parseExpr(p);
    expect(p, TOK_SEMI);
    return newExprStmt(e);
}

Program *parseProgram(Parser *p) {
    Program *prog = newProgram();
    while (p->cur.kind != TOK_EOF) {
        // parse function: name ( params ) { body }
        if (p->cur.kind != TOK_IDENT) { fprintf(stderr,"expected function name\n"); exit(1); }
        char *fname = strDup(p->cur.text); next(p);
        expect(p, TOK_LPAREN);
        char **params = NULL; int pc = 0;
        if (p->cur.kind!=TOK_RPAREN) {
            while (1) {
                if (p->cur.kind!=TOK_IDENT) { fprintf(stderr,"expected param name\n"); exit(1); }
                params = realloc(params, sizeof(char*)*(pc+1)); params[pc++] = strDup(p->cur.text);
                next(p);
                if (p->cur.kind==TOK_COMMA) { next(p); continue; }
                break;
            }
        }
        expect(p, TOK_RPAREN);
        Stmt *block = parseBlock(p);
        Function *f = newFunction(fname, params, pc, block);
        // append to program
        f->next = prog->functions; prog->functions = f;
    }
    return prog;
}

