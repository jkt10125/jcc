#include "ir.h"
#include <stdlib.h>

IrModule *newIrModule(void) {
    IrModule *m = calloc(1, sizeof(IrModule));
    return m;
}

