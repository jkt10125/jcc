#include "codegen_direct.h"
#include "codegen_bytes.h"
#include "runtime_bytes.h"
#include "elf.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct VarNode {
    char *name;
    int index;
    struct VarNode *next;
} VarNode;

typedef struct FnSigNode {
    char *name;
    int paramCount;
    struct FnSigNode *next;
} FnSigNode;

typedef struct {
    size_t *offsets;
    int count;
    int cap;
} BreakList;

static void breakListInit(BreakList *b) { b[0].offsets = NULL; b[0].count = 0; b[0].cap = 0; }
static void breakListAdd(BreakList *b, size_t off) {
    if (b[0].count >= b[0].cap) {
        b[0].cap = b[0].cap ? b[0].cap * 2 : 4;
        b[0].offsets = realloc(b[0].offsets, b[0].cap * sizeof(size_t));
    }
    b[0].offsets[b[0].count++] = off;
}
static void breakListPatch(ByteBuf *text, BreakList *b) {
    for (int i = 0; i < b[0].count; i++) {
        int32_t rel = (int32_t)((int64_t)text[0].size - (int64_t)(b[0].offsets[i] + 4));
        patchRel32(text, b[0].offsets[i], rel);
    }
}

static FnSigNode *fnSigList = NULL;
static int maxParamCount = 0;

static void addFnSig(const char *name, int paramCount) {
    FnSigNode *n = malloc(sizeof(FnSigNode));
    n[0].name = strDup(name);
    n[0].paramCount = paramCount;
    n[0].next = fnSigList;
    fnSigList = n;
    if (paramCount > maxParamCount) maxParamCount = paramCount;
}

static int findFnParamCount(const char *name, int *outCount) {
    for (FnSigNode *p = fnSigList; p; p = p[0].next) {
        if (strcmp(p[0].name, name) == 0) { outCount[0] = p[0].paramCount; return 1; }
    }
    return 0;
}

static int findVarIndex(VarNode *vars, const char *name) {
    for (VarNode *p = vars; p; p = p[0].next) {
        if (strcmp(p[0].name, name) == 0) return p[0].index;
    }
    return -1;
}

static void addVarNode(VarNode **vars, const char *name, int index) {
    VarNode *n = malloc(sizeof(VarNode));
    n[0].name = strDup(name);
    n[0].index = index;
    n[0].next = vars[0];
    vars[0] = n;
}

static void emitLoadLocal(ByteBuf *text, int varIndex) {
    int32_t disp = -(int32_t)(8 * (varIndex + 1));
    emitMovRegMemDisp(text, REG_RAX, REG_RBP, disp);
}

static void emitStoreLocal(ByteBuf *text, int varIndex) {
    int32_t disp = -(int32_t)(8 * (varIndex + 1));
    emitMovMemDispReg(text, REG_RBP, disp, REG_RAX);
}

static void genExpr(ByteBuf *text, PatchList *patches, Expr *e, VarNode *locals);

static void genMemLoad(ByteBuf *text, PatchList *patches, Expr *indexExpr, VarNode *locals) {
    genExpr(text, patches, indexExpr, locals);          // rax = index
    emitMovRegImm64Patch(text, patches, SEG_TEXT, REG_R10, "mem", 0); // r10 = &mem
    emitMovRegMemDisp(text, REG_R10, REG_R10, 0);       // r10 = mem base
    emitLeaRegBaseIndexScaleDisp(text, REG_R11, REG_R10, REG_RAX, 8, 0); // r11 = base + index*8
    emitMovRegMemDisp(text, REG_RAX, REG_R11, 0);       // rax = *(r11)
}

static void genMemStore(ByteBuf *text, PatchList *patches, Expr *indexExpr, Expr *valueExpr, VarNode *locals) {
    genExpr(text, patches, indexExpr, locals);          // rax = index
    emitPushReg(text, REG_RAX);
    genExpr(text, patches, valueExpr, locals);          // rax = value
    emitMovRegReg(text, REG_R11, REG_RAX);              // r11 = value
    emitPopReg(text, REG_RAX);                          // rax = index
    emitMovRegImm64Patch(text, patches, SEG_TEXT, REG_R10, "mem", 0); // r10 = &mem
    emitMovRegMemDisp(text, REG_R10, REG_R10, 0);       // r10 = mem base
    emitLeaRegBaseIndexScaleDisp(text, REG_R10, REG_R10, REG_RAX, 8, 0); // r10 = base + index*8
    emitMovMemDispReg(text, REG_R10, 0, REG_R11);
    emitMovRegImm64(text, REG_RAX, 0);
}

static void genIndexLoad(ByteBuf *text, PatchList *patches, Expr *baseExpr, Expr *indexExpr, VarNode *locals) {
    genExpr(text, patches, baseExpr, locals); // rax = base
    emitPushReg(text, REG_RAX);
    genExpr(text, patches, indexExpr, locals); // rax = index
    emitPopReg(text, REG_R10); // r10 = base
    emitLeaRegBaseIndexScaleDisp(text, REG_R11, REG_R10, REG_RAX, 8, 0);
    emitMovRegMemDisp(text, REG_RAX, REG_R11, 0);
}

static void genIndexStore(ByteBuf *text, PatchList *patches, Expr *baseExpr, Expr *indexExpr, Expr *valueExpr, VarNode *locals) {
    genExpr(text, patches, baseExpr, locals); // rax = base
    emitPushReg(text, REG_RAX);
    genExpr(text, patches, indexExpr, locals); // rax = index
    emitPushReg(text, REG_RAX);
    genExpr(text, patches, valueExpr, locals); // rax = value
    emitMovRegReg(text, REG_R11, REG_RAX); // r11 = value
    emitPopReg(text, REG_RAX); // rax = index
    emitPopReg(text, REG_R10); // r10 = base
    emitLeaRegBaseIndexScaleDisp(text, REG_R10, REG_R10, REG_RAX, 8, 0);
    emitMovMemDispReg(text, REG_R10, 0, REG_R11);
    emitMovRegImm64(text, REG_RAX, 0);
}

static void genBinOp(ByteBuf *text, PatchList *patches, Expr *e, VarNode *locals) {
    genExpr(text, patches, e[0].binop.left, locals);
    emitPushReg(text, REG_RAX);
    genExpr(text, patches, e[0].binop.right, locals);
    emitPopReg(text, REG_R11); // left
    BinOpKind op = e[0].binop.op;
    if (op == BIN_ADD) {
        emitAddRegReg(text, REG_RAX, REG_R11);
        return;
    }
    if (op == BIN_MUL) {
        emitIMulRegReg(text, REG_RAX, REG_R11);
        return;
    }
    if (op == BIN_SUB) {
        emitMovRegReg(text, REG_R10, REG_R11);
        emitSubRegReg(text, REG_R10, REG_RAX);
        emitMovRegReg(text, REG_RAX, REG_R10);
        return;
    }
    if (op == BIN_DIV || op == BIN_MOD) {
        emitMovRegReg(text, REG_R10, REG_RAX); // divisor
        emitMovRegReg(text, REG_RAX, REG_R11); // dividend
        emitCqo(text);
        emitIDivReg(text, REG_R10);
        if (op == BIN_MOD) emitMovRegReg(text, REG_RAX, REG_RDX);
        return;
    }
    if (op == BIN_BAND) {
        emitAndRegReg(text, REG_RAX, REG_R11);
        return;
    }
    if (op == BIN_BOR) {
        emitOrRegReg(text, REG_RAX, REG_R11);
        return;
    }
    if (op == BIN_BXOR) {
        emitXorRegReg(text, REG_RAX, REG_R11);
        return;
    }
    if (op == BIN_SHL || op == BIN_SHR) {
        // right is shift count; hardware uses CL
        emitMovRegReg(text, REG_RCX, REG_RAX);
        emitMovRegReg(text, REG_RAX, REG_R11);
        if (op == BIN_SHL) emitShlRegCl(text, REG_RAX);
        else emitShrRegCl(text, REG_RAX);
        return;
    }
    // comparisons: result is 0 or 1 in rax
    if (op == BIN_EQ || op == BIN_NEQ || op == BIN_LT || op == BIN_GT || op == BIN_LE || op == BIN_GE) {
        // left in r11, right in rax
        emitCmpRegReg(text, REG_R11, REG_RAX);
        // cc mapping for SETcc
        // E=4, NE=5, L=12, G=15, LE=14, GE=13 (signed comparisons)
        uint8_t cc = 4;
        if (op == BIN_EQ) cc = 4;
        else if (op == BIN_NEQ) cc = 5;
        else if (op == BIN_LT) cc = 12;
        else if (op == BIN_GE) cc = 13;
        else if (op == BIN_LE) cc = 14;
        else if (op == BIN_GT) cc = 15;
        emitSetccAl(text, cc);
        emitMovzxRaxAl(text);
        return;
    }
    emitMovRegImm64(text, REG_RAX, 0);
}

static void genCall(ByteBuf *text, PatchList *patches, Expr *e, VarNode *locals) {
    // builtins
    if (e[0].call.fn->kind == EX_VAR && strcmp(e[0].call.fn->varName, "__mem_store") == 0) {
        genMemStore(text, patches, e[0].call.args[0], e[0].call.args[1], locals);
        return;
    }
    if (e[0].call.fn->kind == EX_VAR && strcmp(e[0].call.fn->varName, "__index_store") == 0) {
        genIndexStore(text, patches, e[0].call.args[0], e[0].call.args[1], e[0].call.args[2], locals);
        return;
    }

    Reg argRegs[6] = { REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9 };
    int argCount = e[0].call.argCount;

    // Predictable arity behavior:
    // - If the callee is a known direct function name, missing parameters are treated as 0 up to its paramCount.
    // - For indirect calls (function expression not a plain name), missing parameters are treated as 0 up to maxParamCount.
    int targetParamCount = 0;
    if (e[0].call.fn->kind == EX_VAR) {
        if (!findFnParamCount(e[0].call.fn->varName, &targetParamCount)) {
            targetParamCount = maxParamCount;
        }
    } else {
        targetParamCount = maxParamCount;
    }
    if (targetParamCount < 0) targetParamCount = 0;
    int effectiveArgCount = argCount;
    if (effectiveArgCount < targetParamCount) effectiveArgCount = targetParamCount;

    int stackArgCount = effectiveArgCount > 6 ? (effectiveArgCount - 6) : 0;

    // SysV AMD64: args 7+ are passed on the stack.
    // Ensure stack is 16-byte aligned at call boundary.
    // Our code generally keeps RSP 16-byte aligned inside functions; pushing an odd number of
    // stack args would break alignment, so we add one 8-byte pad slot if needed.
    int needsPad = (stackArgCount & 1) ? 1 : 0;
    if (needsPad) {
        emitMovRegImm64(text, REG_RAX, 0);
        emitPushReg(text, REG_RAX);
    }

    // push args N..7 so that at callee entry:
    // [rsp+8] = arg7, [rsp+16] = arg8, ...
    for (int i = effectiveArgCount - 1; i >= 6; --i) {
        if (i < argCount) {
            genExpr(text, patches, e[0].call.args[i], locals);
        } else {
            emitMovRegImm64(text, REG_RAX, 0);
        }
        emitPushReg(text, REG_RAX);
    }

    // first 6 args in registers (evaluate in reverse order so nested calls don't overwrite earlier args)
    int regProvided = argCount < 6 ? argCount : 6;
    for (int i = regProvided; i < 6; i++) {
        emitMovRegImm64(text, REG_RAX, 0);
        emitMovRegReg(text, argRegs[i], REG_RAX);
    }
    for (int i = regProvided - 1; i >= 0; i--) {
        // Save already-computed args (regs i+1..5) so arg expr won't clobber them (e.g. BIN_SHL uses rcx)
        for (int j = i + 1; j < 6; j++) {
            emitPushReg(text, argRegs[j]);
        }
        genExpr(text, patches, e[0].call.args[i], locals);
        emitMovRegReg(text, argRegs[i], REG_RAX);
        // Restore
        for (int j = 5; j >= i + 1; j--) {
            emitPopReg(text, argRegs[j]);
        }
    }

    if (e[0].call.fn->kind == EX_VAR) {
        int fnIdx = findVarIndex(locals, e[0].call.fn->varName);
        if (fnIdx >= 0) {
            emitLoadLocal(text, fnIdx);
            emitCallReg(text, REG_RAX);
        } else {
            emitMovRegImm64Patch(text, patches, SEG_TEXT, REG_RAX, e[0].call.fn->varName, 0);
            emitCallReg(text, REG_RAX);
        }
    } else {
        genExpr(text, patches, e[0].call.fn, locals);
        emitCallReg(text, REG_RAX);
    }

    // caller stack cleanup for args 7+ and optional pad
    if (stackArgCount || needsPad) {
        uint32_t bytes = (uint32_t)(8 * (stackArgCount + needsPad));
        emitAddRspImm32(text, bytes);
    }
}

static void genExpr(ByteBuf *text, PatchList *patches, Expr *e, VarNode *locals) {
    if (!e) { emitMovRegImm64(text, REG_RAX, 0); return; }
    switch (e[0].kind) {
        case EX_INT:
            emitMovRegImm64(text, REG_RAX, (uint64_t)e[0].intValue);
            return;
        case EX_VAR: {
            int idx = findVarIndex(locals, e[0].varName);
            if (idx >= 0) { emitLoadLocal(text, idx); return; }
            emitMovRegImm64(text, REG_RAX, 0);
            return;
        }
        case EX_ADDR:
            // &local or &function
            {
                int idx = findVarIndex(locals, e[0].addrName);
                if (idx >= 0) {
                    // rax = rbp - 8*(idx+1)
                    emitMovRegReg(text, REG_RAX, REG_RBP);
                    emitMovRegImm64(text, REG_R10, (uint64_t)(8 * (idx + 1)));
                    emitSubRegReg(text, REG_RAX, REG_R10);
                } else {
                    emitMovRegImm64Patch(text, patches, SEG_TEXT, REG_RAX, e[0].addrName, 0);
                }
            }
            return;
        case EX_INDEX:
            if (e[0].index.arr->kind == EX_VAR && strcmp(e[0].index.arr->varName, "mem") == 0) {
                genMemLoad(text, patches, e[0].index.index, locals);
                return;
            }
            genIndexLoad(text, patches, e[0].index.arr, e[0].index.index, locals);
            return;
        case EX_CALL:
            genCall(text, patches, e, locals);
            return;
        case EX_BINOP:
            genBinOp(text, patches, e, locals);
            return;
    }
    emitMovRegImm64(text, REG_RAX, 0);
}

static void genStmtListInternal(ByteBuf *text, PatchList *patches, Stmt *s, VarNode *locals, uint32_t stackAlloc, int emitDefaultReturn, size_t loopStart, BreakList *breakList) {
    for (Stmt *p = s; p; p = p[0].next) {
        if (p[0].kind == NODE_STMT_ASSIGN) {
            genExpr(text, patches, p[0].assign.rhs, locals);
            int idx = findVarIndex(locals, p[0].assign.lhs);
            if (idx >= 0) emitStoreLocal(text, idx);
        } else if (p[0].kind == NODE_STMT_RETURN) {
            genExpr(text, patches, p[0].retExpr, locals);
            emitLeave(text);
            emitRet(text);
            return;
        } else if (p[0].kind == NODE_STMT_EXPR) {
            genExpr(text, patches, p[0].exprStmt, locals);
        } else if (p[0].kind == NODE_STMT_BLOCK) {
            genStmtListInternal(text, patches, p[0].blockBody, locals, stackAlloc, 0, loopStart, breakList);
        } else if (p[0].kind == NODE_STMT_IF) {
            genExpr(text, patches, p[0].ifStmt.cond, locals);
            emitTestRegReg(text, REG_RAX, REG_RAX);
            size_t jeElse = emitJccRel32Placeholder(text, 0x4); // JE
            // then
            genStmtListInternal(text, patches, p[0].ifStmt.thenBranch, locals, stackAlloc, 0, loopStart, breakList);
            if (p[0].ifStmt.elseBranch) {
                size_t jmpEnd = emitJmpRel32Placeholder(text);
                int32_t relElse = (int32_t)((int64_t)text[0].size - (int64_t)(jeElse + 4));
                patchRel32(text, jeElse, relElse);
                genStmtListInternal(text, patches, p[0].ifStmt.elseBranch, locals, stackAlloc, 0, loopStart, breakList);
                int32_t relEnd = (int32_t)((int64_t)text[0].size - (int64_t)(jmpEnd + 4));
                patchRel32(text, jmpEnd, relEnd);
            } else {
                int32_t relElse = (int32_t)((int64_t)text[0].size - (int64_t)(jeElse + 4));
                patchRel32(text, jeElse, relElse);
            }
        } else if (p[0].kind == NODE_STMT_WHILE) {
            size_t innerLoopStart = text[0].size;
            BreakList innerBreaks;
            breakListInit(&innerBreaks);
            genExpr(text, patches, p[0].whileStmt.cond, locals);
            emitTestRegReg(text, REG_RAX, REG_RAX);
            size_t jeEnd = emitJccRel32Placeholder(text, 0x4); // JE
            genStmtListInternal(text, patches, p[0].whileStmt.body, locals, stackAlloc, 0, innerLoopStart, &innerBreaks);
            // jmp back
            size_t jmpBack = emitJmpRel32Placeholder(text);
            int32_t relBack = (int32_t)((int64_t)innerLoopStart - (int64_t)(jmpBack + 4));
            patchRel32(text, jmpBack, relBack);
            breakListPatch(text, &innerBreaks);
            free(innerBreaks.offsets);
            int32_t relEnd = (int32_t)((int64_t)text[0].size - (int64_t)(jeEnd + 4));
            patchRel32(text, jeEnd, relEnd);
        } else if (p[0].kind == NODE_STMT_BREAK) {
            size_t jmpOff = emitJmpRel32Placeholder(text);
            breakListAdd(breakList, jmpOff);
        } else if (p[0].kind == NODE_STMT_CONTINUE) {
            size_t jmpOff = emitJmpRel32Placeholder(text);
            int32_t rel = (int32_t)((int64_t)loopStart - (int64_t)(jmpOff + 4));
            patchRel32(text, jmpOff, rel);
        }
    }
    if (emitDefaultReturn) {
        emitMovRegImm64(text, REG_RAX, 0);
        emitLeave(text);
        emitRet(text);
    }
    (void)stackAlloc;
}

static uint32_t align16(uint32_t x) { return (x + 15u) & ~15u; }

static void collectAssignedVars(Stmt *s, VarNode **locals, int *localCount) {
    for (Stmt *p = s; p; p = p[0].next) {
        if (p[0].kind == NODE_STMT_ASSIGN) {
            if (findVarIndex(locals[0], p[0].assign.lhs) < 0) {
                addVarNode(locals, p[0].assign.lhs, localCount[0]);
                localCount[0] += 1;
            }
        } else if (p[0].kind == NODE_STMT_BLOCK) {
            collectAssignedVars(p[0].blockBody, locals, localCount);
        } else if (p[0].kind == NODE_STMT_IF) {
            collectAssignedVars(p[0].ifStmt.thenBranch, locals, localCount);
            if (p[0].ifStmt.elseBranch) collectAssignedVars(p[0].ifStmt.elseBranch, locals, localCount);
        } else if (p[0].kind == NODE_STMT_WHILE) {
            collectAssignedVars(p[0].whileStmt.body, locals, localCount);
        }
    }
}

static void genFunctionBytes(ByteBuf *text, PatchList *patches, Function *fn) {
    VarNode *locals = NULL;
    for (int i=0;i<fn[0].paramCount;i++) addVarNode(&locals, fn[0].params[i], i);
    int localCount = fn[0].paramCount;
    collectAssignedVars(fn[0].body, &locals, &localCount);
    uint32_t stackAlloc = align16((uint32_t)(localCount * 8));

    // prologue
    emitPushReg(text, REG_RBP);
    emitMovRegReg(text, REG_RBP, REG_RSP);
    if (stackAlloc) emitSubRspImm32(text, stackAlloc);

    // store params to locals
    Reg argRegs[6] = { REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9 };
    int n = fn[0].paramCount < 6 ? fn[0].paramCount : 6;
    for (int i=0;i<n;i++) {
        int32_t disp = -(int32_t)(8 * (i + 1));
        emitMovMemDispReg(text, REG_RBP, disp, argRegs[i]);
    }
    // params 7+ come from the caller stack:
    // after `push rbp; mov rbp, rsp`, the layout is:
    //   [rbp+8]  = return address
    //   [rbp+16] = arg7
    //   [rbp+24] = arg8
    //   ...
    for (int i = 6; i < fn[0].paramCount; i++) {
        int32_t srcDisp = 16 + 8 * (i - 6);
        emitMovRegMemDisp(text, REG_RAX, REG_RBP, srcDisp);
        emitStoreLocal(text, i);
    }
    // zero non-parameter locals (parameters must keep passed values)
    for (int i = fn[0].paramCount; i < localCount; i++) {
        emitMovRegImm64(text, REG_RAX, 0);
        emitStoreLocal(text, i);
    }

    genStmtListInternal(text, patches, fn[0].body, locals, stackAlloc, 1, 0, NULL);
}

static uint64_t computeDataVaddr(uint64_t textSize) {
    uint64_t base = 0x400000;
    uint64_t textVaddr = base + 0x1000;
    uint64_t textOffset = 0x1000;
    uint64_t align = 0x1000;
    uint64_t dataOffset = textOffset + ((textSize + align - 1) & ~(align - 1));
    return textVaddr + (dataOffset - textOffset);
}

static void applyPatches(ByteBuf *text, ByteBuf *data, PatchList *patches, SymbolTable *symbols) {
    for (int i=0;i<patches[0].count;i++) {
        Patch *p = &patches[0].items[i];
        uint64_t sym;
        if (!symbolGet(symbols, p[0].symbolName, &sym)) {
            fprintf(stderr, "patch error: missing symbol %s\n", p[0].symbolName);
            exit(1);
        }
        uint64_t val = sym + (uint64_t)p[0].addend;
        if (p[0].seg == SEG_TEXT) {
            memcpy(&text[0].data[p[0].offset], &val, 8);
        } else {
            memcpy(&data[0].data[p[0].offset], &val, 8);
        }
    }
}

int emitDirectElfProgram(const char *outPath, Program *prog, int memEntries) {
    ByteBuf text; byteBufInit(&text);
    ByteBuf data; byteBufInit(&data);
    PatchList patches; patchListInit(&patches);
    SymbolTable symbols; symbolTableInit(&symbols);

    RuntimeOffsets rtOff;
    emitRuntime(&text, &patches, &rtOff);
    symbolSet(&symbols, "_start", (0x400000 + 0x1000) + rtOff.startOffset);
    symbolSet(&symbols, "rt_put_int", (0x400000 + 0x1000) + rtOff.putIntOffset);
    symbolSet(&symbols, "rt_get_int", (0x400000 + 0x1000) + rtOff.getIntOffset);
    symbolSet(&symbols, "rt_put_char", (0x400000 + 0x1000) + rtOff.putCharOffset);
    symbolSet(&symbols, "rt_read_char", (0x400000 + 0x1000) + rtOff.readCharOffset);
    symbolSet(&symbols, "rt_exit", (0x400000 + 0x1000) + rtOff.exitOffset);
    symbolSet(&symbols, "rt_str_buf_ptr", (0x400000 + 0x1000) + rtOff.strBufPtrOffset);

    // collect function signatures for arity padding
    fnSigList = NULL;
    maxParamCount = 0;
    // runtime helper signatures (so calls don't get padded up to maxParamCount)
    addFnSig("rt_put_int", 1);
    addFnSig("rt_get_int", 0);
    addFnSig("rt_put_char", 1);
    addFnSig("rt_read_char", 0);
    addFnSig("rt_exit", 1);
    addFnSig("rt_str_buf_ptr", 0);
    for (Function *f = prog[0].functions; f; f = f[0].next) {
        const char *symName = (strcmp(f[0].name, "main") == 0) ? "lang_main" : f[0].name;
        addFnSig(symName, f[0].paramCount);
        // also allow looking up by original name for &main or direct call style, if used
        addFnSig(f[0].name, f[0].paramCount);
    }

    // functions: emit in list order
    for (Function *f = prog[0].functions; f; f = f[0].next) {
        const char *name = (strcmp(f[0].name, "main") == 0) ? "lang_main" : f[0].name;
        uint64_t funcVaddr = (0x400000 + 0x1000) + text.size;
        symbolSet(&symbols, name, funcVaddr);
        genFunctionBytes(&text, &patches, f);
    }

    // data: [mem (u64)] [memArray (i64[memEntries])] [rt_str_buf (bytes)]
    uint64_t rtStrBufSize = 4097ull;
    uint64_t dataSize = 8ull + (uint64_t)memEntries * 8ull + rtStrBufSize;
    byteBufReserve(&data, dataSize);
    for (uint64_t i=0;i<dataSize;i++) emitU8(&data, 0);

    uint64_t dataVaddr = computeDataVaddr(text.size);
    uint64_t memVaddr = dataVaddr;
    uint64_t memArrayVaddr = dataVaddr + 8;
    uint64_t rtStrBufVaddr = memArrayVaddr + (uint64_t)memEntries * 8ull;
    symbolSet(&symbols, "mem", memVaddr);
    symbolSet(&symbols, "memArray", memArrayVaddr);
    symbolSet(&symbols, "rt_str_buf", rtStrBufVaddr);

    // initialize mem = memArrayVaddr
    memcpy(&data.data[0], &memArrayVaddr, 8);

    applyPatches(&text, &data, &patches, &symbols);

    // entry is _start at offset rtOff.startOffset (usually 0)
    if (write_elf64(outPath, text.data, (uint64_t)text.size, data.data, (uint64_t)data.size, (uint64_t)rtOff.startOffset) != 0) {
        fprintf(stderr, "write_elf64 failed\n");
        return 0;
    }
    return 1;
}

