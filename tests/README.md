# Test Suite

All tests are combined in a single file: `tests/all.j`

## Run

From project root:

```bash
make test
```

Or directly:

```bash
./jcc -m 1024 tests/all.j
printf '42\nX\nhello\nworld\n' | ./a.out
```

## Input

The I/O tests (`test_read_int`, `test_read_char`, `test_read_str`, `test_read_str_byte`) consume stdin in order:

1. `42` — for _read_int
2. `X` — for _read_char (prints 88)
3. `hello` — for _read_str (prints "hello")
4. `world` — for _read_str_byte (prints 119, first byte 'w')

## Test coverage

- **Core:** add, arithmetic, bitwise, control_flow, break_continue, local_pointers, comprehensive
- **Buf:** get_set, packing, memset, memmove, overlap, cmp, full, generic
- **Alloc:** basic, memmove
- **I/O:** read_int, read_char, read_str, read_str_byte
