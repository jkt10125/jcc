#ifndef CODEGEN_BYTES_H
#define CODEGEN_BYTES_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
    REG_RBX = 3,
    REG_RSP = 4,
    REG_RBP = 5,
    REG_RSI = 6,
    REG_RDI = 7,
    REG_R8  = 8,
    REG_R9  = 9,
    REG_R10 = 10,
    REG_R11 = 11,
    REG_R12 = 12,
    REG_R13 = 13,
    REG_R14 = 14,
    REG_R15 = 15
} Reg;

typedef struct {
    uint8_t *data;
    size_t size;
    size_t cap;
} ByteBuf;

typedef enum { SEG_TEXT = 0, SEG_DATA = 1 } Segment;

typedef struct {
    Segment seg;
    size_t offset;      // offset within segment where imm64 starts
    char *symbolName;   // owned
    int64_t addend;
} Patch;

typedef struct {
    Patch *items;
    int count;
    int cap;
} PatchList;

typedef struct {
    char *name;     // owned
    uint64_t value; // virtual address
} Symbol;

typedef struct {
    Symbol *items;
    int count;
    int cap;
} SymbolTable;

void byteBufInit(ByteBuf *b);
void byteBufFree(ByteBuf *b);
void byteBufReserve(ByteBuf *b, size_t n);
void emitU8(ByteBuf *b, uint8_t v);
void emitU32(ByteBuf *b, uint32_t v);
void emitU64(ByteBuf *b, uint64_t v);
void patchListInit(PatchList *p);
void patchListFree(PatchList *p);
void addPatch(PatchList *p, Segment seg, size_t offset, const char *symbolName, int64_t addend);

void symbolTableInit(SymbolTable *t);
void symbolTableFree(SymbolTable *t);
void symbolSet(SymbolTable *t, const char *name, uint64_t value);
int symbolGet(SymbolTable *t, const char *name, uint64_t *outValue);

// instruction encoders (minimal set)
void emitPushReg(ByteBuf *b, Reg reg);
void emitPopReg(ByteBuf *b, Reg reg);
void emitMovRegReg(ByteBuf *b, Reg dst, Reg src);
void emitMovRegImm64(ByteBuf *b, Reg dst, uint64_t imm);
size_t emitMovRegImm64Patch(ByteBuf *b, PatchList *p, Segment seg, Reg dst, const char *symbolName, int64_t addend);
void emitMovRegMemDisp(ByteBuf *b, Reg dst, Reg base, int32_t disp);
void emitMovMemDispReg(ByteBuf *b, Reg base, int32_t disp, Reg src);
void emitLeaRegBaseIndexScaleDisp(ByteBuf *b, Reg dst, Reg base, Reg index, int scale, int32_t disp);
void emitAddRegReg(ByteBuf *b, Reg dst, Reg src);
void emitSubRegReg(ByteBuf *b, Reg dst, Reg src);
void emitIMulRegReg(ByteBuf *b, Reg dst, Reg src);
void emitCqo(ByteBuf *b);
void emitIDivReg(ByteBuf *b, Reg divisor);
void emitCallReg(ByteBuf *b, Reg reg);
void emitRet(ByteBuf *b);
void emitLeave(ByteBuf *b);
void emitSubRspImm32(ByteBuf *b, uint32_t imm);
void emitAddRspImm32(ByteBuf *b, uint32_t imm);
void emitSyscall(ByteBuf *b);

// branching helpers for runtime
size_t emitJmpRel32Placeholder(ByteBuf *b);
void patchRel32(ByteBuf *b, size_t atOffset, int32_t rel);
size_t emitJccRel32Placeholder(ByteBuf *b, uint8_t cc);
void emitCmpRegImm8(ByteBuf *b, Reg reg, uint8_t imm);
void emitTestRegReg(ByteBuf *b, Reg a, Reg bReg);
void emitCmpRegReg(ByteBuf *b, Reg left, Reg right);
void emitSetccAl(ByteBuf *b, uint8_t cc);
void emitMovzxRaxAl(ByteBuf *b);

#endif

