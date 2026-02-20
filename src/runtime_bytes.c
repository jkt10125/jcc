#include "runtime_bytes.h"

// Emits:
// _start: call lang_main; exit(return)
// printInt: syscall-only decimal print with newline
//
// Notes:
// - We keep it minimal; caller-saved regs only.
// - printInt expects value in RDI.

static void emitMovRegImm64Const(ByteBuf *text, Reg reg, uint64_t imm) {
    emitMovRegImm64(text, reg, imm);
}

void emitRuntime(ByteBuf *text, PatchList *patches, RuntimeOffsets *outOffsets) {
    outOffsets[0].startOffset = text[0].size;

    // _start:
    // align stack for call: sub rsp, 8
    emitSubRspImm32(text, 8);
    // movabs rax, lang_main ; call *rax
    emitMovRegImm64Patch(text, patches, SEG_TEXT, REG_RAX, "lang_main", 0);
    emitCallReg(text, REG_RAX);
    // add rsp, 8
    emitAddRspImm32(text, 8);
    // mov rdi, rax
    emitMovRegReg(text, REG_RDI, REG_RAX);
    // movabs rax, 60 ; syscall
    emitMovRegImm64Const(text, REG_RAX, 60);
    emitSyscall(text);

    // printInt:
    outOffsets[0].printIntOffset = text[0].size;
    // prologue: push rbp; mov rbp, rsp; sub rsp, 96 (buffer + locals)
    emitPushReg(text, REG_RBP);
    emitMovRegReg(text, REG_RBP, REG_RSP);
    emitSubRspImm32(text, 96);

    // stack layout:
    // [rbp-8]  signFlag (0/1)
    // buffer at [rbp-96 .. rbp-17] (80 bytes), write from end.

    // signFlag = 0
    emitMovRegImm64Const(text, REG_RAX, 0);
    emitMovMemDispReg(text, REG_RBP, -8, REG_RAX);

    // if (rdi < 0) { signFlag=1; rdi = -rdi; }
    emitTestRegReg(text, REG_RDI, REG_RDI);
    size_t jgeOff = emitJccRel32Placeholder(text, 0xD); // JGE
    // signFlag=1
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitMovMemDispReg(text, REG_RBP, -8, REG_RAX);
    // neg rdi: 48 F7 DF
    emitU8(text, 0x48); emitU8(text, 0xF7); emitU8(text, 0xDF);
    // patch jge to here
    int32_t relJge = (int32_t)((int64_t)text[0].size - (int64_t)(jgeOff + 4));
    patchRel32(text, jgeOff, relJge);

    // rsi = &bufEnd (rbp-17): mov rsi, rbp; sub rsi, 17
    emitMovRegReg(text, REG_RSI, REG_RBP);
    emitMovRegImm64Const(text, REG_RAX, 17);
    emitSubRegReg(text, REG_RSI, REG_RAX);

    // write newline at [rsi]
    emitMovRegImm64Const(text, REG_RAX, (uint64_t)'\n');
    // mov byte ptr [rsi], al : 88 06
    emitU8(text, 0x88); emitU8(text, 0x06);

    // r8 = 1 (len)
    emitMovRegImm64Const(text, REG_R8, 1);

    // if value == 0: store '0'
    emitCmpRegImm8(text, REG_RDI, 0);
    size_t jneValue = emitJccRel32Placeholder(text, 0x5); // JNE
    // --rsi; [rsi]='0'; ++len
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitSubRegReg(text, REG_RSI, REG_RAX);
    emitMovRegImm64Const(text, REG_RAX, (uint64_t)'0');
    emitU8(text, 0x88); emitU8(text, 0x06);
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitAddRegReg(text, REG_R8, REG_RAX);
    size_t jmpAfterDigits = emitJmpRel32Placeholder(text);
    // patch jne to digit loop start
    int32_t relJne = (int32_t)((int64_t)text[0].size - (int64_t)(jneValue + 4));
    patchRel32(text, jneValue, relJne);

    // digitLoop:
    size_t digitLoopStart = text[0].size;
    // rax = rdi
    emitMovRegReg(text, REG_RAX, REG_RDI);
    emitCqo(text);
    // r10 = 10
    emitMovRegImm64Const(text, REG_R10, 10);
    emitIDivReg(text, REG_R10); // quotient in rax, remainder in rdx
    // remainder in rdx, quotient in rax
    // rdi = rax
    emitMovRegReg(text, REG_RDI, REG_RAX);
    // rdx += '0'
    emitMovRegImm64Const(text, REG_RAX, (uint64_t)'0');
    emitAddRegReg(text, REG_RDX, REG_RAX);
    // --rsi
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitSubRegReg(text, REG_RSI, REG_RAX);
    // mov byte [rsi], dl : 88 16
    emitU8(text, 0x88); emitU8(text, 0x16);
    // ++len
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitAddRegReg(text, REG_R8, REG_RAX);
    // if (rdi != 0) loop
    emitCmpRegImm8(text, REG_RDI, 0);
    size_t jneLoop = emitJccRel32Placeholder(text, 0x5); // JNE
    int32_t relBack = (int32_t)((int64_t)digitLoopStart - (int64_t)(jneLoop + 4));
    patchRel32(text, jneLoop, relBack);

    // patch jmpAfterDigits to here
    int32_t relAfter = (int32_t)((int64_t)text[0].size - (int64_t)(jmpAfterDigits + 4));
    patchRel32(text, jmpAfterDigits, relAfter);

    // if signFlag: prepend '-'
    // load signFlag into rax
    emitMovRegMemDisp(text, REG_RAX, REG_RBP, -8);
    emitCmpRegImm8(text, REG_RAX, 0);
    size_t jeNoSign = emitJccRel32Placeholder(text, 0x4); // JE
    // --rsi; [rsi]='-'; ++len
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitSubRegReg(text, REG_RSI, REG_RAX);
    emitMovRegImm64Const(text, REG_RAX, (uint64_t)'-');
    emitU8(text, 0x88); emitU8(text, 0x06);
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitAddRegReg(text, REG_R8, REG_RAX);
    int32_t relNoSign = (int32_t)((int64_t)text[0].size - (int64_t)(jeNoSign + 4));
    patchRel32(text, jeNoSign, relNoSign);

    // sys_write(1, rsi, r8)
    // rax=1, rdi=1, rdx=len, rsi=bufStart
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitMovRegImm64Const(text, REG_RDI, 1);
    emitMovRegReg(text, REG_RDX, REG_R8);
    emitSyscall(text);

    // epilogue
    emitLeave(text);
    emitRet(text);
}

