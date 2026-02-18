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
        char op = (p->cur.kind==TOK_STAR)?'*':(p->cur.kind==TOK_SLASH)?'/':'%';
        next(p);
        Expr *r = parsePostfix(p);
        e = newBinOpExpr(op,e,r);
    }
    return e;
}

static Expr *parseAddSub(Parser *p) {
    Expr *e = parseMulDiv(p);
    while (p->cur.kind==TOK_PLUS || p->cur.kind==TOK_MINUS) {
        char op = (p->cur.kind==TOK_PLUS)?'+':'-';
        next(p);
        Expr *r = parseMulDiv(p);
        e = newBinOpExpr(op,e,r);
    }
    return e;
}

static Expr *parseExpr(Parser *p) {
    return parseAddSub(p);
}

static Stmt *parseStmt(Parser *p) {
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
                // build index expr as LHS - represent as special assign to mem[index]
                Expr *arr = newVarExpr(name);
                Expr *lhsIndex = newIndexExpr(arr, idx);
                // create synthetic assign with lhs name "mem" and store index in rhs as needed
                // For simplicity, encode as assignment to variable name "mem" and RHS is a call to store (handled in codegen)
                // Instead, we will create an assignment where lhs is "mem_index" combined; but to keep AST simple, return an exprStmt that performs store via a special call
                // We'll create an assignment with lhs "mem" and rhs being a call-like expression: store(index, rhs)
                Expr **args = malloc(sizeof(Expr*)*2);
                args[0] = idx; args[1] = rhs;
                Expr *storeCall = newCallExpr(newVarExpr(strDup("__mem_store")), args, 2);
                return newExprStmt(storeCall);
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
        expect(p, TOK_LBRACE);
        Stmt *head = NULL, *tail = NULL;
        while (p->cur.kind!=TOK_RBRACE && p->cur.kind!=TOK_EOF) {
            Stmt *s = parseStmt(p);
            if (!head) head = tail = s; else { tail->next = s; tail = s; }
        }
        expect(p, TOK_RBRACE);
        Function *f = newFunction(fname, params, pc, head);
        // append to program
        f->next = prog->functions; prog->functions = f;
    }
    return prog;
}

