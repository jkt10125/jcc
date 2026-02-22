#include "runtime_bytes.h"

// Emits:
// _start: call lang_main; exit(return)
// rt_put_int: syscall-only decimal print (no newline)
// rt_get_int: syscall-only read + parse signed decimal from stdin
// (old string syscall helpers removed; string helpers live in stdlib now)
// rt_read_char: syscall-only read one byte
// rt_exit: syscall-only process exit
//
// Notes:
// - We keep it minimal; caller-saved regs only.
// - rt_put_int expects value in RDI.
// - rt_exit expects code in RDI.

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

    // rt_put_int:
    outOffsets[0].putIntOffset = text[0].size;
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

    // rsi = &bufEnd (rbp-16): mov rsi, rbp; sub rsi, 16
    emitMovRegReg(text, REG_RSI, REG_RBP);
    emitMovRegImm64Const(text, REG_RAX, 16);
    emitSubRegReg(text, REG_RSI, REG_RAX);

    // r8 = 0 (len)
    emitMovRegImm64Const(text, REG_R8, 0);

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

    // rt_put_char(x):
    // prints the low byte of x (x in RDI) to stdout, no newline.
    outOffsets[0].putCharOffset = text[0].size;
    // sub rsp, 8
    emitSubRspImm32(text, 8);
    // mov byte [rsp], dil : 40 88 3C 24
    emitU8(text, 0x40); emitU8(text, 0x88); emitU8(text, 0x3C); emitU8(text, 0x24);
    // sys_write(1, rsp, 1)
    emitMovRegImm64Const(text, REG_RAX, 1);
    emitMovRegImm64Const(text, REG_RDI, 1);
    emitMovRegReg(text, REG_RSI, REG_RSP);
    emitMovRegImm64Const(text, REG_RDX, 1);
    emitSyscall(text);
    // add rsp, 8
    emitAddRspImm32(text, 8);
    emitMovRegImm64Const(text, REG_RAX, 0);
    emitRet(text);

    // rt_read_char():
    // returns 0..255 for a byte, or -1 on EOF/error.
    outOffsets[0].readCharOffset = text[0].size;
    // sub rsp, 8
    emitSubRspImm32(text, 8);
    // sys_read(0, rsp, 1)
    emitMovRegImm64Const(text, REG_RAX, 0);
    emitMovRegImm64Const(text, REG_RDI, 0);
    emitMovRegReg(text, REG_RSI, REG_RSP);
    emitMovRegImm64Const(text, REG_RDX, 1);
    emitSyscall(text);
    // if (rax == 1) return byte; else return -1
    emitCmpRegImm8(text, REG_RAX, 1);
    size_t jneEof = emitJccRel32Placeholder(text, 0x5); // JNE
    // movzx eax, byte [rsp]
    emitU8(text, 0x0F); emitU8(text, 0xB6); emitU8(text, 0x04); emitU8(text, 0x24);
    // add rsp, 8
    emitAddRspImm32(text, 8);
    emitRet(text);
    // eof:
    size_t eof = text[0].size;
    patchRel32(text, jneEof, (int32_t)((int64_t)eof - (int64_t)(jneEof + 4)));
    emitAddRspImm32(text, 8);
    emitMovRegImm64Const(text, REG_RAX, (uint64_t)(int64_t)-1);
    emitRet(text);

    // rt_exit:
    outOffsets[0].exitOffset = text[0].size;
    emitMovRegImm64Const(text, REG_RAX, 60);
    emitSyscall(text);
    emitRet(text);

    // rt_get_int:
    outOffsets[0].getIntOffset = text[0].size;
    // prologue: push rbp; mov rbp, rsp; sub rsp, 160 (buffer + scratch)
    emitPushReg(text, REG_RBP);
    emitMovRegReg(text, REG_RBP, REG_RSP);
    emitSubRspImm32(text, 160);

    // rsi = bufStart (rbp-160)
    emitMovRegReg(text, REG_RSI, REG_RBP);
    emitMovRegImm64Const(text, REG_RAX, 160);
    emitSubRegReg(text, REG_RSI, REG_RAX);

    // sys_read(0, rsi, 128)
    emitMovRegImm64Const(text, REG_RAX, 0);  // read
    emitMovRegImm64Const(text, REG_RDI, 0);  // fd=0
    emitMovRegImm64Const(text, REG_RDX, 128);
    emitSyscall(text);
    // if (rax <= 0) return 0;
    emitTestRegReg(text, REG_RAX, REG_RAX);
    size_t jleReturnZero = emitJccRel32Placeholder(text, 0xE); // JLE

    // r9 = end = bufStart + bytesRead
    emitMovRegReg(text, REG_R9, REG_RSI);
    emitAddRegReg(text, REG_R9, REG_RAX);

    // r10 = sign (1)
    emitMovRegImm64Const(text, REG_R10, 1);
    // r8 = acc (0)
    emitMovRegImm64Const(text, REG_R8, 0);

    // skip_ws:
    size_t skipWsStart = text[0].size;
    // if (rsi >= r9) goto finish
    emitCmpRegReg(text, REG_RSI, REG_R9);
    size_t jgeFinishFromWs = emitJccRel32Placeholder(text, 0xD); // JGE
    // c = *rsi (movzx eax, byte [rsi])
    emitU8(text, 0x0F); emitU8(text, 0xB6); emitU8(text, 0x06);
    // if c == ' ' || '\n' || '\t' || '\r' then ++rsi and loop
    emitU8(text, 0x3C); emitU8(text, (uint8_t)' ');
    size_t jeSpace = emitJccRel32Placeholder(text, 0x4); // JE
    emitU8(text, 0x3C); emitU8(text, (uint8_t)'\n');
    size_t jeNl = emitJccRel32Placeholder(text, 0x4); // JE
    emitU8(text, 0x3C); emitU8(text, (uint8_t)'\t');
    size_t jeTab = emitJccRel32Placeholder(text, 0x4); // JE
    emitU8(text, 0x3C); emitU8(text, (uint8_t)'\r');
    size_t jeCr = emitJccRel32Placeholder(text, 0x4); // JE
    size_t jmpAfterWsChecks = emitJmpRel32Placeholder(text);
    // ws_consume:
    size_t wsConsume = text[0].size;
    emitU8(text, 0x48); emitU8(text, 0xFF); emitU8(text, 0xC6); // inc rsi
    size_t jmpBackSkipWs = emitJmpRel32Placeholder(text);

    // patch JE targets to wsConsume
    patchRel32(text, jeSpace, (int32_t)((int64_t)wsConsume - (int64_t)(jeSpace + 4)));
    patchRel32(text, jeNl,    (int32_t)((int64_t)wsConsume - (int64_t)(jeNl + 4)));
    patchRel32(text, jeTab,   (int32_t)((int64_t)wsConsume - (int64_t)(jeTab + 4)));
    patchRel32(text, jeCr,    (int32_t)((int64_t)wsConsume - (int64_t)(jeCr + 4)));
    patchRel32(text, jmpBackSkipWs, (int32_t)((int64_t)skipWsStart - (int64_t)(jmpBackSkipWs + 4)));

    // continue after whitespace checks
    size_t afterWsChecks = text[0].size;
    patchRel32(text, jmpAfterWsChecks, (int32_t)((int64_t)afterWsChecks - (int64_t)(jmpAfterWsChecks + 4)));

    // if (rsi >= r9) goto finish
    emitCmpRegReg(text, REG_RSI, REG_R9);
    size_t jgeFinishFromSign = emitJccRel32Placeholder(text, 0xD); // JGE
    // c = *rsi
    emitU8(text, 0x0F); emitU8(text, 0xB6); emitU8(text, 0x06);
    // if c == '-' set sign=-1 and ++rsi
    emitU8(text, 0x3C); emitU8(text, (uint8_t)'-');
    size_t jneNotMinus = emitJccRel32Placeholder(text, 0x5); // JNE
    emitMovRegImm64Const(text, REG_R10, (uint64_t)(int64_t)-1);
    emitU8(text, 0x48); emitU8(text, 0xFF); emitU8(text, 0xC6); // inc rsi
    size_t jmpAfterSign = emitJmpRel32Placeholder(text);
    // not_minus:
    size_t notMinus = text[0].size;
    patchRel32(text, jneNotMinus, (int32_t)((int64_t)notMinus - (int64_t)(jneNotMinus + 4)));
    // if c == '+' just ++rsi
    emitU8(text, 0x3C); emitU8(text, (uint8_t)'+');
    size_t jneNotPlus = emitJccRel32Placeholder(text, 0x5); // JNE
    emitU8(text, 0x48); emitU8(text, 0xFF); emitU8(text, 0xC6); // inc rsi
    size_t afterSign = text[0].size;
    patchRel32(text, jneNotPlus, (int32_t)((int64_t)afterSign - (int64_t)(jneNotPlus + 4)));
    patchRel32(text, jmpAfterSign, (int32_t)((int64_t)afterSign - (int64_t)(jmpAfterSign + 4)));

    // digit_loop:
    size_t digitLoop = text[0].size;
    emitCmpRegReg(text, REG_RSI, REG_R9);
    size_t jgeFinishFromDigits = emitJccRel32Placeholder(text, 0xD); // JGE
    // c = *rsi
    emitU8(text, 0x0F); emitU8(text, 0xB6); emitU8(text, 0x06);
    // if c < '0' break
    emitU8(text, 0x3C); emitU8(text, (uint8_t)'0');
    size_t jlFinish = emitJccRel32Placeholder(text, 0xC); // JL
    // if c > '9' break
    emitU8(text, 0x3C); emitU8(text, (uint8_t)'9');
    size_t jgFinish = emitJccRel32Placeholder(text, 0xF); // JG
    // digit = c - '0'  (sub al, imm8)
    emitU8(text, 0x2C); emitU8(text, (uint8_t)'0');
    // movzx edx, al
    emitU8(text, 0x0F); emitU8(text, 0xB6); emitU8(text, 0xD0);
    // acc *= 10
    emitMovRegImm64Const(text, REG_R11, 10);
    emitIMulRegReg(text, REG_R8, REG_R11);
    // acc += digit
    emitAddRegReg(text, REG_R8, REG_RDX);
    // ++rsi
    emitU8(text, 0x48); emitU8(text, 0xFF); emitU8(text, 0xC6); // inc rsi
    size_t jmpBackDigits = emitJmpRel32Placeholder(text);

    // finish:
    size_t finish = text[0].size;
    patchRel32(text, jgeFinishFromWs,    (int32_t)((int64_t)finish - (int64_t)(jgeFinishFromWs + 4)));
    patchRel32(text, jgeFinishFromSign,  (int32_t)((int64_t)finish - (int64_t)(jgeFinishFromSign + 4)));
    patchRel32(text, jgeFinishFromDigits,(int32_t)((int64_t)finish - (int64_t)(jgeFinishFromDigits + 4)));
    patchRel32(text, jlFinish,           (int32_t)((int64_t)finish - (int64_t)(jlFinish + 4)));
    patchRel32(text, jgFinish,           (int32_t)((int64_t)finish - (int64_t)(jgFinish + 4)));
    patchRel32(text, jmpBackDigits,      (int32_t)((int64_t)digitLoop - (int64_t)(jmpBackDigits + 4)));

    // move acc into rax and apply sign: rax *= r10
    emitMovRegReg(text, REG_RAX, REG_R8);
    emitIMulRegReg(text, REG_RAX, REG_R10);
    emitLeave(text);
    emitRet(text);

    // return_zero:
    size_t returnZero = text[0].size;
    patchRel32(text, jleReturnZero, (int32_t)((int64_t)returnZero - (int64_t)(jleReturnZero + 4)));
    emitMovRegImm64Const(text, REG_RAX, 0);
    emitLeave(text);
    emitRet(text);

}

