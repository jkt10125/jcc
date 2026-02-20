# Minimalist 64-bit Word Language — Detailed Documentation

This document describes the language defined in `CompilerDesign.txt`, plus the `print(x)` builtin that exists in the current `jcc` implementation.

The language is intentionally small: **one signed 64-bit integer type** and a **single global memory array** addressed by index.

---

## Table of contents

- **1. Core model**
  - 1.1 One type: signed 64-bit `int`
  - 1.2 Program = functions only
  - 1.3 Global memory: `mem`
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
  - 4.5 Control flow (`if`, `else`, `while`)
- **5. Expressions**
  - 5.1 Literals and identifiers
  - 5.2 Arithmetic operators
  - 5.3 Comparisons
  - 5.4 Parentheses and precedence
  - 5.5 Array indexing: `x[i]`
  - 5.6 Address-of: `&name`
  - 5.7 Function calls (direct and indirect)
  - 5.8 Calling convention notes (how args are passed)
  - 5.9 Argument count behavior (too few / too many)
- **6. Function pointers**
  - 6.1 Taking function addresses
  - 6.2 Storing in locals / `mem`
  - 6.3 Indirect calls
- **7. Builtin I/O**
  - 7.1 `print(x)`
- **8. Examples**
  - 8.1 Minimal program
  - 8.2 Using `mem` as a table
  - 8.3 Indirect calls via `mem`
  - 8.4 Recursion (language spec)
- **9. Compile-time configuration**
  - 9.1 `mem` size (`jcc -m N`)
- **10. Notes about the current `jcc` implementation**

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
print(123);
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

### 4.5 Control flow (`if`, `while`) (language spec)

The language specification includes:

- `if (expr) statement` optionally followed by `else statement`
- `while (expr) statement`

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
    if (x == 5) return 1;
    return 0;
}
```

---

## 5. Expressions

### 5.1 Literals and identifiers

- Integer literal: `0`, `123`, `-7`
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
print(7 + 8);     // 15
print(7 - 8);     // -1
print(7 * 8);     // 56
print(100 / 3);   // 33 (integer division)
print(100 % 3);   // 1
```

### 5.3 Comparisons

The language specification includes:

- `==`, `!=`, `<`, `>`, `<=`, `>=`

These produce an integer (conventionally `1` or `0`).

Example:

```c
x = (5 < 7);   // expected 1
y = (5 == 7);  // expected 0
```

### 5.4 Parentheses and precedence

Parentheses `(...)` group expressions.

Typical precedence (high to low):

1. Postfix: calls `f(...)`, indexing `x[i]`
2. Multiplicative: `* / %`
3. Additive: `+ -`
4. Comparisons
5. Assignment `=`

Example:

```c
print(2 + 3 * 4);     // 14
print((2 + 3) * 4);   // 20
```

### 5.5 Array indexing: `x[i]`

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

#### 5.5.1 Local pointer indexing

In this language, **addresses are integers** (still the same single `int64` type).

That means you can store an address into a local and then index it:

```c
main() {
    x = 123;
    p = &x;      // p is an int64 address
    print(p[0]); // prints 123
    p[0] = 999;  // writes to x through the pointer
    print(x);    // prints 999
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

### 5.6 Address-of: `&name`

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
    print(a);
    print(b); // prints 777
    return 0;
}
```

### 5.7 Function calls (direct and indirect)

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

### 5.8 Calling convention notes (how args are passed)

This section explains runtime behavior of calls. It is not part of the surface syntax, but it matters for performance and for understanding “too many / too few arguments”.

The direct-ELF backend in `jcc` follows Linux x86_64 System V calling convention style rules:

- **Args 1..6** are passed in registers:
  - 1 → `RDI`, 2 → `RSI`, 3 → `RDX`, 4 → `RCX`, 5 → `R8`, 6 → `R9`
- **Args 7+** are passed on the stack.
- Return value is in `RAX`.

### 5.9 Argument count behavior (too few / too many)

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
    print(add3(5));      // prints 5 (treated as add3(5,0,0))
    mem[0] = &add3;
    print(mem[0](7));    // prints 7 (pads missing args to 0)
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

## 7. Builtin I/O

### 7.1 `print(x)`

`print(x);` prints the signed 64-bit integer `x` followed by a newline.

Example:

```c
main() {
    x = -7;
    print(x);
    return 0;
}
```

In the current `jcc` direct-ELF backend, printing is implemented using Linux syscalls:

- `sys_write(1, buf, len)` to stdout

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
    print(mem[0] + mem[1]); // prints 30
    return mem[0] + mem[1];
}
```

### 8.3 Indirect calls via `mem`

```c
add(a, b) { return a + b; }

main() {
    mem[0] = &add;
    print(mem[0](5, 10)); // prints 15
    return mem[0](5, 10);
}
```

### 8.4 Recursion (language spec)

Recursion is allowed by the spec.

```c
fact(n) {
    if (n <= 1) return 1;
    return n * fact(n - 1);
}

main() {
    print(fact(5)); // 120
    return fact(5);
}
```

---

## 9. Compile-time configuration

### 9.1 `mem` size (`jcc -m N`)

The size of the global memory array is specified at compile time.

With `jcc` the interface is:

```bash
jcc -m 1024 program.j
```

This sets `MEM_ENTRIES = 1024` and the generated executable contains:

- `memArray[1024]` in `.data`

---

## 10. Notes about the current `jcc` implementation

The **language specification** in `CompilerDesign.txt` is the source of truth for the language. The compiler in this repository is intentionally minimalist, but it now covers the essential spec features and includes a few pragmatic extensions.

As of the current implementation:

- **Supported**:
  - Function definitions and calls (direct + indirect through `mem`)
  - `mem[index]` load and `mem[index] = value` store
  - Local pointer features:
    - `&x` for locals (address-of local slot)
    - `p[i]` load and `p[i] = v` store (pointer indexing)
  - `+ - * / %` arithmetic
  - comparison operators (`== != < > <= >=`)
  - `return`
  - blocks `{ ... }` (no new scope; function-level locals)
  - `if (...) ... else ...`
  - `while (...) ...`
  - `print(x)` builtin
  - `//` line comments
  - Calls:
    - more than 6 arguments supported (stack arguments)
    - missing arguments padded with 0 (see 5.9)
- **Not yet implemented** (spec exists, compiler work may be needed):
  - None of the essential items in `CompilerDesign.txt` remain missing.\n+    Future work is quality-of-implementation (better error messages, more static checks, optimizations, more tests).


