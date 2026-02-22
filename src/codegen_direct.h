#ifndef CODEGEN_DIRECT_H
#define CODEGEN_DIRECT_H

#include "ast.h"

int emitDirectElfProgram(const char *outPath, Program *prog, int memEntries, int bufBytes);

#endif

