#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdint.h>

typedef enum {
    TOK_EOF,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACK, TOK_RBRACK, TOK_SEMI, TOK_COMMA, TOK_ASSIGN,
    TOK_AMP,
    TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_EQ, TOK_NEQ,
    TOK_RETURN,
    TOK_IF, TOK_ELSE, TOK_WHILE
} TokenKind;

typedef struct {
    TokenKind kind;
    char *text;
    int64_t num;
} Token;

typedef struct {
    const char *src;
    int pos;
    Token cur;
} Lexer;

void lexerInit(Lexer *lx, const char *src);
Token lexerNext(Lexer *lx);
Token lexerPeek(Lexer *lx);

#endif

