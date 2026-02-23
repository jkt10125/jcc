#include "codegen_bytes.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

static void ensureCap(ByteBuf *b, size_t add) {
    size_t need = b[0].size + add;
    if (need <= b[0].cap) return;
    size_t newCap = b[0].cap ? b[0].cap : 256;
    while (newCap < need) newCap *= 2;
    b[0].data = realloc(b[0].data, newCap);
    b[0].cap = newCap;
}

void byteBufInit(ByteBuf *b) { b[0].data = NULL; b[0].size = 0; b[0].cap = 0; }
void byteBufFree(ByteBuf *b) { free(b[0].data); b[0].data = NULL; b[0].size = 0; b[0].cap = 0; }
void byteBufReserve(ByteBuf *b, size_t n) { ensureCap(b, n); }
void emitU8(ByteBuf *b, uint8_t v) { ensureCap(b,1); b[0].data[b[0].size++] = v; }
void emitU32(ByteBuf *b, uint32_t v) { ensureCap(b,4); memcpy(&b[0].data[b[0].size], &v, 4); b[0].size += 4; }
void emitU64(ByteBuf *b, uint64_t v) { ensureCap(b,8); memcpy(&b[0].data[b[0].size], &v, 8); b[0].size += 8; }

void patchListInit(PatchList *p) { p[0].items = NULL; p[0].count = 0; p[0].cap = 0; }
void patchListFree(PatchList *p) {
    for (int i=0;i<p[0].count;i++) free(p[0].items[i].symbolName);
    free(p[0].items);
    p[0].items = NULL; p[0].count = 0; p[0].cap = 0;
}
void addPatch(PatchList *p, Segment seg, size_t offset, const char *symbolName, int64_t addend) {
    if (p[0].count == p[0].cap) {
        p[0].cap = p[0].cap ? p[0].cap * 2 : 64;
        p[0].items = realloc(p[0].items, sizeof(Patch) * (size_t)p[0].cap);
    }
    p[0].items[p[0].count].seg = seg;
    p[0].items[p[0].count].offset = offset;
    p[0].items[p[0].count].symbolName = strDup(symbolName);
    p[0].items[p[0].count].addend = addend;
    p[0].count++;
}

void symbolTableInit(SymbolTable *t) { t[0].items = NULL; t[0].count = 0; t[0].cap = 0; }
void symbolTableFree(SymbolTable *t) {
    for (int i=0;i<t[0].count;i++) free(t[0].items[i].name);
    free(t[0].items);
    t[0].items = NULL; t[0].count = 0; t[0].cap = 0;
}
void symbolSet(SymbolTable *t, const char *name, uint64_t value) {
    for (int i=0;i<t[0].count;i++) {
        if (strcmp(t[0].items[i].name, name)==0) { t[0].items[i].value = value; return; }
    }
    if (t[0].count == t[0].cap) {
        t[0].cap = t[0].cap ? t[0].cap * 2 : 64;
        t[0].items = realloc(t[0].items, sizeof(Symbol) * (size_t)t[0].cap);
    }
    t[0].items[t[0].count].name = strDup(name);
    t[0].items[t[0].count].value = value;
    t[0].count++;
}
int symbolGet(SymbolTable *t, const char *name, uint64_t *outValue) {
    for (int i=0;i<t[0].count;i++) {
        if (strcmp(t[0].items[i].name, name)==0) { outValue[0] = t[0].items[i].value; return 1; }
    }
    return 0;
}

static uint8_t rexByte(int w, int r, int x, int b) {
    return (uint8_t)(0x40 | (w?8:0) | (r?4:0) | (x?2:0) | (b?1:0));
}
static void emitRexW(ByteBuf *b, int r, int x, int bb) {
    emitU8(b, rexByte(1, r, x, bb));
}
static void emitModRm(ByteBuf *b, int mod, int reg, int rm) {
    emitU8(b, (uint8_t)(((mod & 3) << 6) | ((reg & 7) << 3) | (rm & 7)));
}
static void emitSib(ByteBuf *b, int scale, int index, int base) {
    int ss = 0;
    if (scale == 1) ss = 0;
    else if (scale == 2) ss = 1;
    else if (scale == 4) ss = 2;
    else if (scale == 8) ss = 3;
    emitU8(b, (uint8_t)(((ss & 3) << 6) | ((index & 7) << 3) | (base & 7)));
}

void emitPushReg(ByteBuf *b, Reg reg) {
    if (reg < 8) { emitU8(b, (uint8_t)(0x50 + reg)); return; }
    emitU8(b, rexByte(0,0,0,1));
    emitU8(b, (uint8_t)(0x50 + (reg & 7)));
}
void emitPopReg(ByteBuf *b, Reg reg) {
    if (reg < 8) { emitU8(b, (uint8_t)(0x58 + reg)); return; }
    emitU8(b, rexByte(0,0,0,1));
    emitU8(b, (uint8_t)(0x58 + (reg & 7)));
}
void emitMovRegReg(ByteBuf *b, Reg dst, Reg src) {
    // mov r/m64, r64 : 48 89 /r (dst is r/m, src is reg)
    int r = (src >> 3) & 1;
    int bb = (dst >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x89);
    emitModRm(b, 3, src & 7, dst & 7);
}
void emitMovRegImm64(ByteBuf *b, Reg dst, uint64_t imm) {
    if (imm == 0) {
        emitZeroReg(b, dst);
        return;
    }
    int bb = (dst >> 3) & 1;
    emitU8(b, rexByte(1,0,0,bb));
    emitU8(b, (uint8_t)(0xB8 + (dst & 7)));
    emitU64(b, imm);
}
size_t emitMovRegImm64Patch(ByteBuf *b, PatchList *p, Segment seg, Reg dst, const char *symbolName, int64_t addend) {
    int bb = (dst >> 3) & 1;
    emitU8(b, rexByte(1,0,0,bb));
    emitU8(b, (uint8_t)(0xB8 + (dst & 7)));
    size_t immOffset = b[0].size;
    emitU64(b, 0);
    addPatch(p, seg, immOffset, symbolName, addend);
    return immOffset;
}
void emitMovRegMemDisp(ByteBuf *b, Reg dst, Reg base, int32_t disp) {
    // mov r64, [base+disp32] : 48 8B /r (dst is reg)
    int r = (dst >> 3) & 1;
    int bb = (base >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x8B);
    emitModRm(b, 2, dst & 7, base & 7);
    emitU32(b, (uint32_t)disp);
}
void emitMovMemDispReg(ByteBuf *b, Reg base, int32_t disp, Reg src) {
    // mov [base+disp32], r64 : 48 89 /r
    int r = (src >> 3) & 1;
    int bb = (base >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x89);
    emitModRm(b, 2, src & 7, base & 7);
    emitU32(b, (uint32_t)disp);
}
void emitLeaRegBaseIndexScaleDisp(ByteBuf *b, Reg dst, Reg base, Reg index, int scale, int32_t disp) {
    // lea r64, [base + index*scale + disp32] : 48 8D /r with SIB
    int r = (dst >> 3) & 1;
    int x = (index >> 3) & 1;
    int bb = (base >> 3) & 1;
    emitRexW(b, r, x, bb);
    emitU8(b, 0x8D);
    emitModRm(b, 2, dst & 7, 4); // rm=100 => SIB
    emitSib(b, scale, index & 7, base & 7);
    emitU32(b, (uint32_t)disp);
}
void emitLeaRegBaseDisp(ByteBuf *b, Reg dst, Reg base, int32_t disp) {
    // lea r64, [base + disp32] : 48 8D /r  (uses SIB with index=RSP for no index)
    int r = (dst >> 3) & 1;
    int bb = (base >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x8D);
    emitModRm(b, 2, dst & 7, 4);
    emitSib(b, 1, 4, base & 7);
    emitU32(b, (uint32_t)disp);
}
void emitZeroReg(ByteBuf *b, Reg reg) {
    // xor reg, reg (2-3 bytes) instead of mov reg, 0 (7-8 bytes)
    emitXorRegReg(b, reg, reg);
}
void emitShrRegImm8(ByteBuf *b, Reg reg, uint8_t imm) {
    // shr r/m64, imm8 : 48 C1 /5 ib
    int bb = (reg >> 3) & 1;
    emitRexW(b, 0, 0, bb);
    emitU8(b, 0xC1);
    emitModRm(b, 3, 5, reg & 7);
    emitU8(b, imm);
}
void emitShlRegImm8(ByteBuf *b, Reg reg, uint8_t imm) {
    // shl r/m64, imm8 : 48 C1 /4 ib
    int bb = (reg >> 3) & 1;
    emitRexW(b, 0, 0, bb);
    emitU8(b, 0xC1);
    emitModRm(b, 3, 4, reg & 7);
    emitU8(b, imm);
}
void emitAddRegReg(ByteBuf *b, Reg dst, Reg src) {
    // add r/m64, r64 : 48 01 /r (dst is r/m, src is reg)
    int r = (src >> 3) & 1;
    int bb = (dst >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x01);
    emitModRm(b, 3, src & 7, dst & 7);
}
void emitAddRegImm8(ByteBuf *b, Reg dst, uint8_t imm) {
    // add r/m64, imm8 : 48 83 /0 ib
    int bb = (dst >> 3) & 1;
    emitRexW(b, 0, 0, bb);
    emitU8(b, 0x83);
    emitModRm(b, 3, 0, dst & 7);
    emitU8(b, imm);
}
void emitSubRegReg(ByteBuf *b, Reg dst, Reg src) {
    // sub r/m64, r64 : 48 29 /r
    int r = (src >> 3) & 1;
    int bb = (dst >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x29);
    emitModRm(b, 3, src & 7, dst & 7);
}
void emitAndRegReg(ByteBuf *b, Reg dst, Reg src) {
    // and r/m64, r64 : 48 21 /r
    int r = (src >> 3) & 1;
    int bb = (dst >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x21);
    emitModRm(b, 3, src & 7, dst & 7);
}
void emitAndRegImm32(ByteBuf *b, Reg reg, uint32_t imm) {
    // and r/m64, imm32 : 48 81 /4 id
    int bb = (reg >> 3) & 1;
    emitRexW(b, 0, 0, bb);
    emitU8(b, 0x81);
    emitModRm(b, 3, 4, reg & 7);
    emitU32(b, imm);
}
void emitOrRegReg(ByteBuf *b, Reg dst, Reg src) {
    // or r/m64, r64 : 48 09 /r
    int r = (src >> 3) & 1;
    int bb = (dst >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x09);
    emitModRm(b, 3, src & 7, dst & 7);
}
void emitXorRegReg(ByteBuf *b, Reg dst, Reg src) {
    // xor r/m64, r64 : 48 31 /r
    int r = (src >> 3) & 1;
    int bb = (dst >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x31);
    emitModRm(b, 3, src & 7, dst & 7);
}
static void emitShiftRegCl(ByteBuf *b, Reg dst, int subCode) {
    // shift r/m64, cl : 48 D3 /subCode
    int bb = (dst >> 3) & 1;
    emitRexW(b, 0, 0, bb);
    emitU8(b, 0xD3);
    emitModRm(b, 3, subCode & 7, dst & 7);
}
void emitShlRegCl(ByteBuf *b, Reg dst) { emitShiftRegCl(b, dst, 4); }
void emitShrRegCl(ByteBuf *b, Reg dst) { emitShiftRegCl(b, dst, 5); }
void emitIMulRegReg(ByteBuf *b, Reg dst, Reg src) {
    // imul r64, r/m64 : 48 0F AF /r (dst is reg, src is r/m)
    int r = (dst >> 3) & 1;
    int bb = (src >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x0F);
    emitU8(b, 0xAF);
    emitModRm(b, 3, dst & 7, src & 7);
}
void emitCqo(ByteBuf *b) { emitU8(b, 0x48); emitU8(b, 0x99); }
void emitIDivReg(ByteBuf *b, Reg divisor) {
    // idiv r/m64 : 48 F7 /7
    int bb = (divisor >> 3) & 1;
    emitRexW(b, 0, 0, bb);
    emitU8(b, 0xF7);
    emitModRm(b, 3, 7, divisor & 7);
}
void emitCallReg(ByteBuf *b, Reg reg) {
    // call r/m64 : FF /2
    int bb = (reg >> 3) & 1;
    if (bb) emitU8(b, rexByte(0,0,0,1));
    emitU8(b, 0xFF);
    emitModRm(b, 3, 2, reg & 7);
}
void emitRet(ByteBuf *b) { emitU8(b, 0xC3); }
void emitLeave(ByteBuf *b) { emitU8(b, 0xC9); }
void emitSubRspImm32(ByteBuf *b, uint32_t imm) {
    emitU8(b, 0x48); emitU8(b, 0x81); emitU8(b, 0xEC); emitU32(b, imm);
}
void emitAddRspImm32(ByteBuf *b, uint32_t imm) {
    emitU8(b, 0x48); emitU8(b, 0x81); emitU8(b, 0xC4); emitU32(b, imm);
}
void emitSyscall(ByteBuf *b) { emitU8(b, 0x0F); emitU8(b, 0x05); }

size_t emitJmpRel32Placeholder(ByteBuf *b) {
    emitU8(b, 0xE9);
    size_t off = b[0].size;
    emitU32(b, 0);
    return off;
}
size_t emitJccRel32Placeholder(ByteBuf *b, uint8_t cc) {
    emitU8(b, 0x0F);
    emitU8(b, (uint8_t)(0x80 | (cc & 0x0F)));
    size_t off = b[0].size;
    emitU32(b, 0);
    return off;
}
void patchRel32(ByteBuf *b, size_t atOffset, int32_t rel) {
    memcpy(&b[0].data[atOffset], &rel, 4);
}
void emitCmpRegImm8(ByteBuf *b, Reg reg, uint8_t imm) {
    // cmp r/m64, imm8 : 48 83 /7 ib
    int bb = (reg >> 3) & 1;
    emitU8(b, rexByte(1,0,0,bb));
    emitU8(b, 0x83);
    emitModRm(b, 3, 7, reg & 7);
    emitU8(b, imm);
}
void emitTestRegReg(ByteBuf *b, Reg a, Reg bReg) {
    // test r/m64, r64 : 48 85 /r
    int r = (bReg >> 3) & 1;
    int bb = (a >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x85);
    emitModRm(b, 3, bReg & 7, a & 7);
}

void emitCmpRegReg(ByteBuf *b, Reg left, Reg right) {
    // cmp r/m64, r64 : 48 39 /r  (computes left - right)
    int r = (right >> 3) & 1;
    int bb = (left >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0x39);
    emitModRm(b, 3, right & 7, left & 7);
}

void emitSetccAl(ByteBuf *b, uint8_t cc) {
    // setcc al : 0F 90+cc C0
    emitU8(b, 0x0F);
    emitU8(b, (uint8_t)(0x90 | (cc & 0x0F)));
    emitU8(b, 0xC0);
}

void emitMovzxRaxAl(ByteBuf *b) {
    // movzx eax, al : 0F B6 C0  (zero-extends into eax, which also clears upper 32 of rax)
    emitU8(b, 0x0F);
    emitU8(b, 0xB6);
    emitU8(b, 0xC0);
}
void emitMovAlMemBase(ByteBuf *b, Reg base) {
    // mov al, [base] : 8A /r  (reg=0 for AL, mod=00, rm=base)
    int bb = (base >> 3) & 1;
    if (bb) emitU8(b, rexByte(0, 0, 0, 1));
    emitU8(b, 0x8A);
    emitModRm(b, 0, 0, base & 7);
}
void emitMovMemBaseAl(ByteBuf *b, Reg base) {
    // mov [base], al : 88 /r
    int bb = (base >> 3) & 1;
    if (bb) emitU8(b, rexByte(0, 0, 0, 1));
    emitU8(b, 0x88);
    emitModRm(b, 0, 0, base & 7);
}
void emitIncReg(ByteBuf *b, Reg reg) {
    // inc r64 : FF /0
    int r = (reg >> 3) & 1;
    int bb = (reg >> 3) & 1;
    emitRexW(b, r, 0, bb);
    emitU8(b, 0xFF);
    emitModRm(b, 3, 0, reg & 7);
}

