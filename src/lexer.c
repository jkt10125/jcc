#include "lexer.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "utils.h"

static void setToken(Lexer *lx, TokenKind k, const char *text, int64_t num) {
    if (lx->cur.text) free(lx->cur.text);
    lx->cur.kind = k;
    lx->cur.num = num;
    if (text) lx->cur.text = strDup(text); else lx->cur.text = NULL;
}

void lexerInit(Lexer *lx, const char *src) {
    lx->src = src;
    lx->pos = 0;
    lx->cur.text = NULL;
}

static void skipSpace(Lexer *lx) {
    for (;;) {
        while (lx->src[lx->pos] && isspace((unsigned char)lx->src[lx->pos])) lx->pos++;
        // line comment //
        if (lx->src[lx->pos] == '/' && lx->src[lx->pos + 1] == '/') {
            lx->pos += 2;
            while (lx->src[lx->pos] && lx->src[lx->pos] != '\n') lx->pos++;
            continue;
        }
        break;
    }
}

Token lexerNext(Lexer *lx) {
    skipSpace(lx);
    const char *s = lx->src + lx->pos;
    if (*s == '\0') { setToken(lx, TOK_EOF, NULL, 0); return lx->cur; }
    if (isalpha((unsigned char)*s) || *s=='_') {
        int start = lx->pos;
        while (isalnum((unsigned char)lx->src[lx->pos]) || lx->src[lx->pos]=='_') lx->pos++;
        int len = lx->pos - start;
        char *t = strNDup(lx->src + start, len);
        if (strcmp(t, "return")==0) { setToken(lx, TOK_RETURN, t, 0); }
        else if (strcmp(t, "if")==0) { setToken(lx, TOK_IF, t, 0); }
        else if (strcmp(t, "else")==0) { setToken(lx, TOK_ELSE, t, 0); }
        else if (strcmp(t, "while")==0) { setToken(lx, TOK_WHILE, t, 0); }
        else setToken(lx, TOK_IDENT, t, 0);
        return lx->cur;
    }
    if (isdigit((unsigned char)*s) || (*s=='-' && isdigit((unsigned char)s[1]))) {
        int start = lx->pos;
        if (*s=='-') lx->pos++;
        while (isdigit((unsigned char)lx->src[lx->pos])) lx->pos++;
        int len = lx->pos - start;
        char *t = strNDup(lx->src + start, len);
        int64_t val = atoll(t);
        setToken(lx, TOK_NUMBER, t, val);
        return lx->cur;
    }
    // symbols
    char c = lx->src[lx->pos++];
    switch (c) {
        case '+': setToken(lx, TOK_PLUS, "+",0); break;
        case '-': setToken(lx, TOK_MINUS, "-",0); break;
        case '*': setToken(lx, TOK_STAR, "*",0); break;
        case '/': setToken(lx, TOK_SLASH, "/",0); break;
        case '%': setToken(lx, TOK_PERCENT, "%",0); break;
        case '(': setToken(lx, TOK_LPAREN, "(",0); break;
        case ')': setToken(lx, TOK_RPAREN, ")",0); break;
        case '{': setToken(lx, TOK_LBRACE, "{",0); break;
        case '}': setToken(lx, TOK_RBRACE, "}",0); break;
        case '[': setToken(lx, TOK_LBRACK, "[",0); break;
        case ']': setToken(lx, TOK_RBRACK, "]",0); break;
        case ';': setToken(lx, TOK_SEMI, ";",0); break;
        case ',': setToken(lx, TOK_COMMA, ",",0); break;
        case '=':
            if (lx->src[lx->pos]=='=') { lx->pos++; setToken(lx, TOK_EQ,"==",0); }
            else setToken(lx, TOK_ASSIGN,"=",0);
            break;
        case '&': setToken(lx, TOK_AMP,"&",0); break;
        case '|': setToken(lx, TOK_PIPE,"|",0); break;
        case '^': setToken(lx, TOK_CARET,"^",0); break;
        case '<':
            if (lx->src[lx->pos]=='<') { lx->pos++; setToken(lx, TOK_SHL,"<<",0); }
            else if (lx->src[lx->pos]=='=') { lx->pos++; setToken(lx, TOK_LE,"<=",0); }
            else setToken(lx, TOK_LT,"<",0);
            break;
        case '>':
            if (lx->src[lx->pos]=='>') { lx->pos++; setToken(lx, TOK_SHR,">>",0); }
            else if (lx->src[lx->pos]=='=') { lx->pos++; setToken(lx, TOK_GE,">=",0); }
            else setToken(lx, TOK_GT,">",0);
            break;
        case '!':
            if (lx->src[lx->pos]=='=') { lx->pos++; setToken(lx, TOK_NEQ,"!=",0); }
            break;
        default:
            setToken(lx, TOK_EOF, NULL, 0);
    }
    return lx->cur;
}

Token lexerPeek(Lexer *lx) {
    return lx->cur;
}

