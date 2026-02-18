#include "lower.h"
#include <stdio.h>

int lowerProgram(Program *p) {
    // minimal: no-op lowering (AST used directly by codegen)
    (void)p;
    return 1;
}

