#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer lx;
    Token cur;
} Parser;

void parserInit(Parser *p, const char *src);
Program *parseProgram(Parser *p);

#endif

