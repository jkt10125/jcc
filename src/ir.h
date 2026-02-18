#ifndef IR_H
#define IR_H

// Minimal IR placeholder for future passes
typedef enum { IR_NOP } IrKind;

typedef struct IrInst {
    IrKind kind;
    struct IrInst *next;
} IrInst;

typedef struct IrFunction {
    char *name;
    IrInst *insts;
    struct IrFunction *next;
} IrFunction;

typedef struct IrModule {
    IrFunction *functions;
} IrModule;

IrModule *newIrModule(void);

#endif

