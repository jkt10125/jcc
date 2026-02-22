# Minimalist 64-bit Word Language — Detailed Documentation

This document describes the language defined in `CompilerDesign.txt`, plus the pragmatic features that exist in the current `jcc` implementation.

**Specification source:** `CompilerDesign.txt` defines the core language. This doc extends it with implementation details and stdlib documentation.

The language is intentionally small: **one signed 64-bit integer type** and a **single global memory array** addressed by index.

---

## Table of contents

- **1. Core model**
  - 1.1 One type: signed 64-bit `int`
  - 1.2 Program = functions only
  - 1.3 Global memory: `mem`
  - 1.4 Standard library (`stdlib/`)
  - 1.5 Strings (null-terminated)
- **2. Names, variables, and scoping**
  - 2.1 Parameters and locals
  - 2.2 “Created on first assignment”
  - 2.3 Undefined variable errors
  - 2.4 Scoping rule (blocks do not create scopes)
- **3. Functions**
  - 3.1 Definition syntax
  - 3.2 Return values
  - 3.3 Entry point: `main()`
- **4. Statements**
  - 4.1 Assignment
  - 4.2 Return
  - 4.3 Expression statement
  - 4.4 Blocks
  - 4.5 Control flow (`if`, `else`, `while`, `break`, `continue`)
- **5. Expressions**
  - 5.1 Literals and identifiers
  - 5.2 Arithmetic operators
  - 5.3 Bitwise operators (`&`, `|`, `^`, `<<`, `>>`)
  - 5.4 Comparisons
  - 5.5 Parentheses and precedence
  - 5.6 Array indexing: `x[i]`
  - 5.7 Address-of: `&name`
  - 5.8 Function calls (direct and indirect)
  - 5.9 Calling convention notes (how args are passed)
  - 5.10 Argument count behavior (too few / too many)
- **6. Function pointers**
  - 6.1 Taking function addresses
  - 6.2 Storing in locals / `mem`
  - 6.3 Indirect calls
- **7. Standard library I/O**
  - 7.1 `_print_int(x)`
  - 7.2 `_print_char(x)`
  - 7.3 `_print_hex(x)`
  - 7.4 `_read_int()`
  - 7.5 `_exit(code)`
  - 7.6 Newlines (use `_print_char(0x0A)`)
  - 7a. Math and utilities (`_abs`, `_min`, `_max`)
- **8. Examples**
  - 8.1 Minimal program
  - 8.2 Using `mem` as a table
  - 8.3 Indirect calls via `mem`
  - 8.4 Recursion (language spec)
- **9. Compile-time configuration**
  - 9.1 `mem` size (`jcc -m N`)
  - 9.2 Read buffer size (`jcc -b N`)
- **10. Memory layout and endianness**
- **11. Spec vs implementation**
- **12. Notes about the current `jcc` implementation**

---

## 1. Core model

### 1.1 One type: signed 64-bit `int`

There is exactly **one** value type in the language:

- **signed 64-bit integer**

You can think of every value as `int64_t` in C:

- Function arguments are `int64`.
- Function returns are `int64`.
- Local variables hold `int64`.
- Each element of global memory holds `int64`.
- Function pointers are also stored as `int64` values (they are “just numbers” representing code addresses).

This simplicity is intentional: no floats, no structs, no strings, no booleans as a separate type.

### 1.2 Program = functions only

A program is just a list of function definitions.

There are **no global variables** other than `mem` (see below).

### 1.3 Global memory: `mem`

The language has one implicit global memory array named `mem`.

You access it with indexing syntax:

```c
mem[0] = 123;
x = mem[0];
```

Conceptually:

- `mem` is an array of signed 64-bit integers.
- The **size is fixed** and provided at compile time.
- You can store either “data” integers or “function pointer” integers in it.

In the current `jcc` implementation, `mem` is modeled as:

- A global 64-bit value `mem` that holds the base address of `memArray`
- `memArray` is a static data segment array of `MEM_ENTRIES` elements
- `mem[i]` means load/store at address `(mem + i*8)`
- `mem` itself is **read-only**: you cannot assign to it (e.g. `mem = x` is a semantic error).

---

### 1.4 Standard library (`stdlib/`)

`jcc` automatically prepends the standard library sources from the `stdlib/` folder to the user program *before parsing*.

Practical implications:

- Stdlib functions are written in the same language, live under `stdlib/*.j`, and are compiled like normal code.
- Stdlib functions use a `_` prefix (e.g. `_print_int`, `_read_int`, `_buf_get_u8`); internal helpers use `__` (e.g. `__mem_store`, `__index_store`, `__buf_get`, `__buf_set`).
- The runtime (embedded assembly bytes) exposes low-level helper functions (`rt_put_int`, `rt_get_int`, `rt_put_char`, `rt_read_char`, `rt_exit`) that stdlib calls; user code typically calls the `_...` wrappers instead.
- `_alloc` is a builtin: calls are inlined at the call site (no function call, no `rt_alloca`).

---

### 1.5 Strings (null-terminated)

`jcc` currently supports **null-terminated byte strings** via the standard library.

- A “string” is just an `int64` pointer to a byte buffer in memory.
- The string ends at the first `0` byte (`'\\0'`).
- There are no string literals yet; you obtain strings from input.

Stdlib functions:

- `_read_str()` reads a line from stdin (stops at newline), allocates space on the stack for the string (including null terminator), zero-initializes it, copies the bytes into it, and returns the starting address of that stack-allocated buffer. The buffer is automatically freed when the caller returns. Max length is bounded by `__buf_size` (configurable via `-b`).
- `_print_str(ptr)` prints bytes starting at `ptr` until it sees a `0` byte.
- `_str_len(ptr)` returns the length of the null-terminated string at `ptr` (number of bytes before the first `0`).
- `_str_cmp(a, b)` compares two null-terminated strings lexicographically; returns `0` if equal, `-1` if `a < b`, `1` if `a > b`.
- To copy a string: use `_buf_memmove_u8(dst, src, _str_len(src) + 1)` (there is no `_str_cpy`).
- `_buf_get_u8(ptr, idx)` returns the byte at the given index as an integer `0..255` (raw access).

Input primitive:

- `_read_char()` reads one byte from stdin and returns `0..255`, or `-1` on EOF/error.

Packed buffers:

- `_buf_get_u8/_buf_set_u8` treat `ptr` as a packed array of 8-bit elements (8 elements per `int64` word).
- `_buf_get_u16/_buf_set_u16`: packed 16-bit elements (4 per word).
- `_buf_get_u32/_buf_set_u32`: packed 32-bit elements (2 per word).
- `_buf_get_u64/_buf_set_u64`: 64-bit elements (1 per word).

All packing is **little-endian within each 64-bit word** and indices are **element indices** (for example u16 index 1 shifts by 16; u32 index 1 shifts by 32).

Generic operations (internal, `__` prefix):

- `__buf_memmove(get_fn, set_fn, dst_ptr, src_ptr, count)` overlap-safe copy; used internally by the typed helpers.
- `__buf_cmp(get_fn, a_ptr, b_ptr, count)` returns 0 if equal, -1/1 otherwise; used internally by the typed helpers.

User-facing convenience helpers (no function pointers):

- `_buf_memmove_u8/u16/u32/u64(dst, src, count)`, `_buf_cmp_u8/u16/u32/u64(a, b, count)`, and `_buf_memset_u8/u16/u32/u64(dst, val, count)`.
- `_buf_find_u8(ptr, val, count)` returns the index of the first byte equal to `val` in the buffer, or `-1` if not found.

Stack allocation (builtin):

- `_alloc(numIntegers)` allocates `numIntegers * 8` bytes on the **caller's** stack frame (aligned to 16 bytes), zero-initializes the memory, and returns the address as an `int64`. The allocation is automatically freed when the caller returns. Use it to get a pointer for string storage or other buffers. Calls are inlined; the buffer is placed directly below the locals with no overlap.

Example (read string):

```c
main() {
    p = _read_str();
    _print_str(p);
    return 0;
}
```

---

## 2. Names, variables, and scoping

### 2.1 Parameters and locals

Inside a function, identifiers refer to one of:

- **Parameters**: the names inside the function’s parentheses.
- **Locals**: names created by assignment inside the function.

Example:

```c
f(a, b) {
    x = a + b;   // x is a local (created here)
    return x;
}
```

### 2.2 “Created on first assignment”

A local variable does **not** need a declaration keyword.

The first time you do:

```c
x = 5;
```

the identifier `x` becomes a local variable for the entire function.

### 2.3 Undefined variable errors

Using a variable on the **right-hand side** before it has a value is an error.

Example (error):

```c
main() {
    y = x + 1;   // ERROR: x is undefined here
    x = 5;
    return y;
}
```

Parameters are considered defined at function entry.

### 2.4 Scoping rule (blocks do not create scopes)

The language uses **one local scope per function**.

Blocks `{ ... }` group statements, but do not create a new scope.

So:

```c
main() {
    if (1) { x = 10; }
    return x;  // x is still the same function-local variable
}
```

---

## 3. Functions

### 3.1 Definition syntax

Functions are defined like:

```c
name(param1, param2, ...) {
    statements...
}
```

There is no `func`/`int` keyword in the language syntax.

### 3.2 Return values

Every function returns an integer (`int64`).

Return statement:

```c
return expr;
```

### 3.3 Entry point: `main()`

One function must be named `main()`; it is the program entry point.

The program result is the integer returned by `main()`.

---

## 4. Statements

### 4.1 Assignment

Assignment statement:

```c
x = expr;
```

This creates `x` as a local if it does not exist yet (see 2.2).

Array assignment is also supported:

```c
mem[0] = 123;
```

### 4.2 Return

```c
return expr;
```

### 4.3 Expression statement

Any expression can be used as a statement by adding `;`.

Most commonly used for calls:

```c
_print_int(123);
f(1, 2);
mem[0](5, 6); // indirect call as a statement
```

### 4.4 Blocks

Blocks group statements:

```c
{
    x = 1;
    y = 2;
}
```

They do not create a new variable scope (2.4).

### 4.5 Control flow (`if`, `else`, `while`)

The language supports:

- `if (expr) { ... }` optionally followed by `else { ... }`
- `while (expr) { ... }`
- `break;` exits the innermost enclosing `while` loop
- `continue;` jumps to the next iteration of the innermost enclosing `while` loop (re-evaluates the condition)

Note: the current `jcc` parser requires the body of `if`, `else`, and `while` to be a block `{ ... }`. `break` and `continue` must appear inside a `while` loop; using them outside a loop is a semantic error.

Truthiness is based on integer values:

- **0** means false
- **non-zero** means true

Example:

```c
main() {
    x = 0;
    while (x < 5) {
        x = x + 1;
    }
    if (x == 5) { return 1; }
    return 0;
}
```

---

## 5. Expressions

### 5.1 Literals and identifiers

- Integer literal: `0`, `123`, `-7`, `0xFF`, `0x1234ABCD` (hex: `0x` or `0X` prefix)
- Identifier: `x`, `result`, `mem`, `add`

### 5.2 Arithmetic operators

Supported arithmetic:

- `+` addition
- `-` subtraction
- `*` multiplication
- `/` signed division
- `%` signed remainder

Examples:

```c
_print_int(7 + 8);     // 15
_print_int(7 - 8);     // -1
_print_int(7 * 8);     // 56
_print_int(100 / 3);   // 33 (integer division)
_print_int(100 % 3);   // 1
```

### 5.3 Bitwise operators (`&`, `|`, `^`, `<<`, `>>`)

Supported bitwise operations:

- `&` bitwise AND
- `|` bitwise OR
- `^` bitwise XOR
- `<<` left shift
- `>>` logical right shift (zero-fill)

Notes:

- All values are still `int64`; bitwise operators act on the 64-bit two’s-complement bit pattern.
- `>>` is **logical** right shift in `jcc` (it uses the CPU `shr` instruction, so high bits fill with 0).
- Shift counts use only the low 6 bits (hardware behavior), effectively `count & 63`.

Examples:

```c
main() {
    _print_int(5 & 3);      // 1
    _print_int(5 | 2);      // 7
    _print_int(5 ^ 1);      // 4
    _print_int(1 << 3);     // 8
    _print_int(-1 >> 1);    // 9223372036854775807
    return 0;
}
```

### 5.4 Comparisons

The language specification includes:

- `==`, `!=`, `<`, `>`, `<=`, `>=`

These produce an integer (conventionally `1` or `0`).

Example:

```c
x = (5 < 7);   // expected 1
y = (5 == 7);  // expected 0
```

### 5.5 Parentheses and precedence

Parentheses `(...)` group expressions.

Typical precedence (high to low):

1. Postfix: calls `f(...)`, indexing `x[i]`
2. Multiplicative: `* / %`
3. Additive: `+ -`
4. Shifts: `<< >>`
5. Bitwise AND: `&`
6. Bitwise XOR: `^`
7. Bitwise OR: `|`
8. Comparisons (`== != < > <= >=`)
9. Assignment `=`

Example:

```c
_print_int(2 + 3 * 4);     // 14
_print_int((2 + 3) * 4);   // 20
```

### 5.6 Array indexing: `x[i]`

Indexing reads an element:

```c
x = mem[10];
```

Indexing can be used on:

- the global `mem`
- locals that hold addresses/arrays (pointer-like `int64` values)

Index expressions are integer expressions:

```c
i = 3;
mem[i + 2] = 99;   // writes mem[5]
```

#### 5.6.1 Local pointer indexing

In this language, **addresses are integers** (still the same single `int64` type).

That means you can store an address into a local and then index it:

```c
main() {
    x = 123;
    p = &x;      // p is an int64 address
    _print_int(p[0]); // prints 123
    p[0] = 999;  // writes to x through the pointer
    _print_int(x);  // prints 999
    return 0;
}
```

In the current `jcc` implementation:

- `mem[i]` uses the global `mem` base pointer (special-cased).
- Any other `base[i]` is treated as pointer indexing:
  - evaluate `base` → base address (int64)
  - evaluate `i` → index (int64)
  - compute effective address \(base + i \times 8\)
  - load/store an 8-byte signed integer

This is intentionally low-level. Indexing an invalid address or going out-of-bounds is undefined behavior.

### 5.7 Address-of: `&name`

The address-of operator produces an integer “address”:

- `&functionName` produces a function pointer integer
- `&x` produces the address of a local variable slot (supported by current `jcc`)

Example (function pointer):

```c
mem[0] = &add;
```

Example (local address-of):

```c
main() {
    a = 10;
    b = 20;
    p = &b;
    p[0] = 777;
    _print_int(a);
    _print_int(b); // prints 777
    return 0;
}
```

#### 5.7.1 `&` disambiguation (address-of vs bitwise AND)

The token `&` has two meanings:

- **Unary** `&name` at the start of a primary expression means **address-of**.
- **Binary** `expr & expr` between two expressions means **bitwise AND**.

The parser disambiguates them based on position in the grammar (expression-start vs infix operator).

### 5.8 Function calls (direct and indirect)

Direct call:

```c
result = add(5, 10);
```

Indirect call:

```c
f = &add;
result = f(5, 10);
```

Indirect via `mem`:

```c
mem[0] = &add;
result = mem[0](5, 10);
```

### 5.9 Calling convention notes (how args are passed)

This section explains runtime behavior of calls. It is not part of the surface syntax, but it matters for performance and for understanding “too many / too few arguments”.

The direct-ELF backend in `jcc` follows Linux x86_64 System V calling convention style rules:

- **Args 1..6** are passed in registers:
  - 1 → `RDI`, 2 → `RSI`, 3 → `RDX`, 4 → `RCX`, 5 → `R8`, 6 → `R9`
- **Args 7+** are passed on the stack.
- Return value is in `RAX`.

### 5.10 Argument count behavior (too few / too many)

The language spec does not explicitly define arity mismatch behavior. The current `jcc` makes it **predictable**:

- **Extra arguments**: ignored by the callee.
- **Missing arguments**: treated as `0`.

More precisely:

- For a **direct call** like `add(5)`, missing arguments are padded with `0` up to that function’s parameter count.
- For an **indirect call** like `mem[0](5)`, the callee is not statically known, so `jcc` pads missing arguments with `0` up to the **maximum parameter count of any function in the program**. This keeps indirect calls predictable too.

Example:

```c
add3(a, b, c) { return a + b + c; }

main() {
    _print_int(add3(5));      // prints 5 (treated as add3(5,0,0))
    mem[0] = &add3;
    _print_int(mem[0](7));    // prints 7 (pads missing args to 0)
    return 0;
}
```

---

## 6. Function pointers

### 6.1 Taking function addresses

Any function can be referenced as:

```c
&functionName
```

This value is an integer address.

### 6.2 Storing in locals / `mem`

```c
main() {
    f = &add;
    mem[0] = &add;
    return 0;
}
```

### 6.3 Indirect calls

Once you have a function pointer integer, you can call it:

```c
(f)(1, 2);     // allowed by spec
f(1, 2);       // also used (depending on parser)
mem[0](1, 2);  // common pattern
```

---

## 7. Standard library I/O

### 7.1 `_print_int(x)`

`_print_int(x);` prints the signed 64-bit integer `x` (no newline).

Example:

```c
main() {
    x = -7;
    _print_int(x);
    return 0;
}
```

In the current `jcc` direct-ELF backend, printing is implemented via a runtime helper (`rt_put_int`) that uses Linux syscalls:

- `sys_write(1, buf, len)` to stdout

### 7.2 `_print_char(x)`

`_print_char(x);` prints the low 8 bits of `x` as a single byte to stdout, with **no newline**.

This is implemented via a runtime helper (`rt_put_char`) that performs `sys_write(1, &byte, 1)`.

### 7.3 `_print_hex(x)`

`_print_hex(x);` prints the signed 64-bit integer `x` in hexadecimal with a `0x` prefix and 16 hex digits, followed by a newline.

Example:

```c
main() {
    _print_hex(255);      // 0x00000000000000FF
    _print_hex(-1);      // 0xFFFFFFFFFFFFFFFF
    return 0;
}
```

### 7.4 `_read_int()`

`_read_int();` reads a signed decimal integer from stdin and returns it. On empty input or EOF, it returns `0`.

The current `jcc` implementation reads from stdin using a runtime helper (`rt_get_int`) that uses:

- `sys_read(0, buf, n)` from stdin

### 7.5 `_exit(code)`

`_exit(code);` terminates the process with the given exit code via a runtime helper (`rt_exit`).

### 7.6 Newlines

There is no dedicated newline function. Use `_print_char(0x0A);` to print a newline. To print an integer followed by a newline: `_print_int(x); _print_char(0x0A);`.

### 7a. Math and utilities (`stdlib/math.j`)

- `_abs(x)` returns the absolute value of `x`.
- `_min(a, b)` returns the smaller of `a` and `b`.
- `_max(a, b)` returns the larger of `a` and `b`.

---

## 8. Examples

### 8.1 Minimal program

```c
main() {
    return 0;
}
```

### 8.2 Using `mem` as a table

```c
main() {
    mem[0] = 10;
    mem[1] = 20;
    _print_int(mem[0] + mem[1]); // prints 30
    return mem[0] + mem[1];
}
```

### 8.3 Indirect calls via `mem`

```c
add(a, b) { return a + b; }

main() {
    mem[0] = &add;
    _print_int(mem[0](5, 10)); // prints 15
    return mem[0](5, 10);
}
```

### 8.4 Recursion (language spec)

Recursion is allowed by the spec.

```c
fact(n) {
    if (n <= 1) { return 1; }
    return n * fact(n - 1);
}

main() {
    _print_int(fact(5)); // 120
    return fact(5);
}
```

---

## 9. Compile-time configuration

### 9.1 `mem` size (`jcc -m N`)

The size of the global memory array is specified at compile time.

With `jcc` the interface is:

```bash
jcc -m <memEntries> [ -b <bufBytes> ] [ -o <out> ] <source>
```

Example:

```bash
jcc -m 1024 -o myprog program.j
```

This sets `MEM_ENTRIES = 1024` and the generated executable contains:

- `memArray[1024]` in `.data`

### 9.2 Read buffer size (`jcc -b N`)

The internal read buffer used by `_read_str()` and other I/O is configurable:

```bash
jcc -m <memEntries> [ -b <bufBytes> ] [ -o <out> ] <source>
```

- `-b <bufBytes>` sets the size of the static read buffer (default: 4096).
- The buffer is accessible via the read-only variable `buf` (its address).
- `__buf_size` is a read-only constant holding the buffer size in bytes.
- You cannot assign to `buf`; it always points to the static buffer.

---

## 10. Memory layout and endianness

All storage in the current `jcc` implementation uses **little-endian** byte order.

| Component | Layout |
|-----------|--------|
| `mem[i]` | Raw x86-64 load/store; little-endian (LSB at lowest address) |
| buf u8/u16/u32 | Little-endian within each 64-bit word (byte 0 = LSB) |
| buf u64 | Raw `ptr[idx]`; little-endian |
| ELF data segment | `ELFDATA2LSB` (little-endian) |
| Stack (locals) | Little-endian (x86-64) |

This matches x86-64 and ARM (e.g. Raspberry Pi) Linux systems, which use little-endian by default.

---

## 11. Spec vs implementation

`CompilerDesign.txt` defines the core language. The following table maps spec sections to implementation status:

| Spec (CompilerDesign.txt) | Implementation |
|---------------------------|----------------|
| §1 Overview (single type, mem, functions) | ✓ Full |
| §2 Program structure (main entry) | ✓ Full |
| §3 Variables (params, locals, first-assign) | ✓ Full |
| §4 Global memory (mem[index]) | ✓ Full |
| §5 Functions (def, return, direct/indirect call) | ✓ Full |
| §6 Statements (assign, return, if/else, while, blocks) | ✓ Full |
| §7 Expressions (arithmetic, comparison, &, x[i], f(...)) | ✓ Full + bitwise `& \| ^ << >>` |
| §8 Function pointers (&fn, store, indirect call) | ✓ Full |

**Extensions beyond spec:**

- Bitwise operators: `&`, `|`, `^`, `<<`, `>>` (logical right shift)
- Hex literals: `0x` / `0X` prefix (e.g. `0xFF`, `0x1234ABCD`)
- `break;` and `continue;` inside `while` loops
- Standard library (stdlib) prepended from `stdlib/`: I/O, buffers, strings

The example program in CompilerDesign.txt §9 is fully supported.

---

## 12. Notes about the current `jcc` implementation

The **language specification** in `CompilerDesign.txt` is the source of truth for the language. The compiler in this repository is intentionally minimalist, but it now covers the essential spec features and includes pragmatic extensions.

As of the current implementation:

- **Supported**:
  - Function definitions and calls (direct + indirect through `mem`)
  - `mem[index]` load and `mem[index] = value` store
  - Local pointer features:
    - `&x` for locals (address-of local slot)
    - `p[i]` load and `p[i] = v` store (pointer indexing)
  - `+ - * / %` arithmetic
  - bitwise operators: `& | ^ << >>` (`>>` is logical right shift)
  - comparison operators (`== != < > <= >=`)
  - `return`
  - blocks `{ ... }` (no new scope; function-level locals)
  - `if (...) { ... } else { ... }` (block bodies required by parser)
  - `while (...) { ... }` (block body required by parser)
  - `break;` and `continue;` (inside `while` loops only)
  - hex literals (`0x` / `0X` prefix)
  - standard library auto-prelude from `stdlib/` (`_print_int`, `_print_char`, `_print_hex`, `_read_int`, `_read_char`, `_read_str`, `_print_str`, `_str_len`, `_str_cmp`, `_alloc`, `_abs`, `_min`, `_max`, `_exit`, buf helpers including `_buf_find_u8`, `_buf_memmove_u8/u16/u32/u64`, `_buf_cmp_u8/u16/u32/u64`, `_buf_memset_u8/u16/u32/u64`)
  - `//` line comments
  - Calls:
    - more than 6 arguments supported (stack arguments)
    - missing arguments padded with 0 (see 5.9)
- **Not yet implemented** (spec exists, compiler work may be needed):
  - None of the essential items in `CompilerDesign.txt` remain missing.
  - `_str_cpy`, `_print_ln`, `_print_int_ln` are not provided; use `_buf_memmove_u8` for string copy and `_print_char(0x0A)` for newlines.
  - Future work is quality-of-implementation (better error messages, more static checks, optimizations, more tests).


