// Combined test suite - all tests in one file
// Run: ./jcc -m 1024 tests/all.j && printf '42\nX\nhello\nworld\n' | ./a.out

// --- Helpers (shared) ---
add10(a, b, c, d, e, f, g, h, i, j) {
    return a * 2 + b + c + d + e + f + g + h + i + j * 10;
}
add2(a, b) { return a + b; }
sub(a, b) { return a - b; }
mul(a, b) { return a * b; }
divi(a, b) { return a / b; }
modi(a, b) { return a % b; }
sum8(a, b, c, d, e, f, g, h) {
    return a + b + c + d + e + f + g + h;
}
mask7(n) { return n & 7; }

// --- Core: function pointers, _mem, indirect call ---
test_add() {
    _mem[0] = &add10;
    _mem[1] = 20;
    result = _mem[0](1,0,0,0,0,0,0,0,0,_mem[0](1)) - _mem[1];
    _print_int(result);
    _print_char(10);
}

// --- Core: arithmetic, >6 args ---
test_arithmetic() {
    _print_int(2 + 3 * 4);
    _print_char(10);
    _mem[0] = &add2;
    _print_int(_mem[0](_mem[0](1, 2), 3));
    _print_char(10);
    _mem[0] = &mul;
    _print_int(_mem[0](5, 6));
    _print_char(10);
    _print_int(divi(100, 3));
    _print_char(10);
    _print_int(modi(100, 3));
    _print_char(10);
    _print_int(sum8(1, 2, 3, 4, 5, 6, 7, 8));
    _print_char(10);
}

// --- Core: bitwise, address-of ---
test_bitwise() {
    a = 5;
    b = 3;
    _print_int(a & b);
    _print_char(10);
    _print_int(a | 2);
    _print_char(10);
    _print_int(1 << 3);
    _print_char(10);
    _print_int(-1 >> 1);
    _print_char(10);
    x = 42;
    p = &x;
    _print_int(p[0]);
    _print_char(10);
    _print_int((&x)[0]);
    _print_char(10);
    _mem[0] = &mask7;
    _print_int(_mem[0](42));
    _print_char(10);
}

// --- Core: if/else, while ---
test_control_flow() {
    x = 0;
    sum = 0;
    while (x < 5) {
        sum = sum + x;
        x = x + 1;
    }
    if (sum == 10) {
        _print_int(111);
        _print_char(10);
    } else {
        _print_int(222);
        _print_char(10);
    }
    _print_int(sum);
    _print_char(10);
}

// --- Core: break, continue ---
test_break_continue() {
    i = 0;
    sum = 0;
    while (i < 10) {
        i = i + 1;
        if (i == 5) { break; }
        sum = sum + i;
    }
    _print_int(sum);
    _print_char(10);
    _print_int(i);
    _print_char(10);
    i = 0;
    sum = 0;
    while (i < 10) {
        i = i + 1;
        if (i == 5) { continue; }
        sum = sum + i;
    }
    _print_int(sum);
    _print_char(10);
}

// --- Core: address-of locals, pointer indexing ---
test_local_pointers() {
    x = 11;
    y = 22;
    z = 33;
    p = &z;
    p[0] = 100;
    p[1] = 200;
    p[2] = 300;
    _print_int(x);
    _print_char(10);
    _print_int(y);
    _print_char(10);
    _print_int(z);
    _print_char(10);
}

// --- Core: comprehensive smoke ---
test_comprehensive() {
    _print_int(2 + 3 * 4);
    _print_char(10);
    _mem[0] = &add2;
    _print_int(_mem[0](1, 2));
    _print_char(10);
    _mem[0] = &mul;
    _print_int(_mem[0](5, 6));
    _print_char(10);
    _mem[1] = 20;
    _print_int(_mem[1]);
    _print_char(10);
    _print_int(0 - 7);
    _print_char(10);
    _print_int(100 / 3);
    _print_char(10);
    _print_int(100 % 3);
    _print_char(10);
}

// --- Buf: get/set u8 ---
test_buf_get_set() {
    p = _buf;
    _buf_set_u8(p, 0, 0x68);
    _buf_set_u8(p, 1, 0x69);
    _buf_set_u8(p, 2, 0);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _print_int(_buf_get_u8(p, 1));
    _print_char(10);
}

// --- Buf: u16, u32 packing ---
test_buf_packing() {
    p = _buf;
    _buf_set_u16(p, 0, 4660);
    _buf_set_u16(p, 1, 43981);
    _print_int(_buf_get_u16(p, 0));
    _print_char(10);
    _print_int(_buf_get_u16(p, 1));
    _print_char(10);
    _buf_set_u32(p, 0, 305419896);
    _buf_set_u32(p, 1, 2596069104);
    _print_int(_buf_get_u32(p, 0));
    _print_char(10);
    _print_int(_buf_get_u32(p, 1));
    _print_char(10);
}

// --- Buf: memset ---
test_buf_memset() {
    p = _buf;
    _buf_memset_u8(p, 65, 4);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _print_int(_buf_get_u8(p, 3));
    _print_char(10);
}

// --- Buf: memmove u8/u16/u32 ---
test_buf_memmove() {
    p = _buf;
    p = p + 128;
    q = _buf;
    q = q + 256;
    _buf_set_u8(p, 0, 10);
    _buf_set_u8(p, 1, 20);
    _buf_set_u8(p, 2, 30);
    _buf_memmove_u8(q, p, 3);
    ok = 1;
    if (_buf_get_u8(q, 0) != 10) { ok = 0; }
    if (_buf_get_u8(q, 2) != 30) { ok = 0; }
    _print_int(ok);
    _print_char(10);
    _buf_set_u16(p, 0, 100);
    _buf_set_u16(p, 1, 200);
    _buf_memmove_u16(q, p, 2);
    if (_buf_get_u16(q, 1) != 200) { ok = 0; }
    _print_int(ok);
    _print_char(10);
}

// --- Buf: overlap ---
test_buf_overlap() {
    p = _buf;
    p = p + 128;
    _buf_set_u8(p, 0, 65);
    _buf_set_u8(p, 1, 66);
    _buf_set_u8(p, 2, 67);
    _buf_set_u8(p, 3, 0);
    j = 3;
    while (j > 0) {
        j = j - 1;
        _buf_set_u8(p, j + 1, _buf_get_u8(p, j));
    }
    _print_int(_buf_get_u8(p, 1));
    _print_char(10);
    _print_int(_buf_get_u8(p, 2));
    _print_char(10);
}

// --- Buf: cmp ---
test_buf_cmp() {
    p = _buf;
    q = _buf;
    q = q + 64;
    _buf_set_u8(p, 0, 1);
    _buf_set_u8(p, 1, 2);
    _buf_set_u8(q, 0, 1);
    _buf_set_u8(q, 1, 2);
    _print_int(_buf_cmp_u8(p, q, 2));
    _print_char(10);
    _buf_set_u8(q, 1, 3);
    _print_int(_buf_cmp_u8(p, q, 2));
    _print_char(10);
}

// --- Buf: full ---
test_buf_full() {
    p = _buf;
    p = p + 128;
    q = _buf;
    q = q + 256;
    _buf_set_u8(p, 0, 10);
    _buf_set_u8(p, 1, 20);
    _buf_set_u8(p, 2, 30);
    _buf_memmove_u8(q, p, 3);
    ok = 1;
    if (_buf_get_u8(q, 0) != 10) { ok = 0; }
    if (_buf_get_u8(q, 2) != 30) { ok = 0; }
    _buf_set_u16(p, 0, 100);
    _buf_set_u16(p, 1, 200);
    _buf_memmove_u16(q, p, 2);
    if (_buf_get_u16(q, 1) != 200) { ok = 0; }
    _buf_set_u32(p, 0, 1111);
    _buf_set_u32(p, 1, 2222);
    _buf_memmove_u32(q, p, 2);
    if (_buf_get_u32(q, 1) != 2222) { ok = 0; }
    _buf_cmp_u8(p, q, 2);
    _buf_memset_u8(p, 65, 3);
    if (_buf_get_u8(p, 2) != 65) { ok = 0; }
    _buf_memmove_u8(q, p, 0);
    _print_int(ok);
    _print_char(10);
}

// --- Buf: generic (__buf_memmove, __buf_cmp) ---
test_buf_generic() {
    a1 = 22;
    a2 = 33;
    b0 = 0;
    b1 = 0;
    b2 = 0;
    src_ptr = &a2;
    dst_ptr = &b2;
    __buf_memmove(&_buf_get_u64, &_buf_set_u64, dst_ptr, src_ptr, 3);
    _print_int(dst_ptr[2]);
    _print_char(10);
    _print_int(dst_ptr[1]);
    _print_char(10);
    _print_int(dst_ptr[0]);
    _print_char(10);
    _print_int(__buf_cmp(&_buf_get_u64, src_ptr, dst_ptr, 3));
    _print_char(10);
    dst_ptr[1] = 99;
    _print_int(__buf_cmp(&_buf_get_u64, src_ptr, dst_ptr, 3));
    _print_char(10);
}

// --- Alloc: basic ---
test_alloc_basic() {
    n = 2;
    p = _alloc(n);
    _buf_set_u8(p, 0, 0x68);
    _buf_set_u8(p, 1, 0x69);
    _buf_set_u8(p, 2, 0);
    _print_str(p);
    _print_char(10);
}

// --- Alloc: memmove ---
test_alloc_memmove() {
    a = _alloc(1);
    b = _alloc(1);
    _buf_set_u8(a, 0, 0x68);
    _buf_set_u8(a, 1, 0x69);
    _buf_set_u8(a, 2, 0);
    _buf_memmove_u8(b, a, 3);
    _print_int(_buf_get_u8(b, 0));
    _print_char(10);
    _print_int(_buf_get_u8(b, 1));
    _print_char(10);
}

// --- I/O: read_int (consumes first line of stdin) ---
test_read_int() {
    x = _read_int();
    _print_int(x);
    _print_char(10);
}

// --- I/O: read_char (consumes next byte) ---
test_read_char() {
    c = _read_char();
    _print_int(c);
    _print_char(10);
}

// --- I/O: read_str (consumes next line) ---
test_read_str() {
    s = _read_str();
    _print_str(s);
    return 0;
}

// --- I/O: read_str_byte (consumes next line, prints first byte) ---
test_read_str_byte() {
    p = _read_str();
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
}

main() {
    test_add();
    test_arithmetic();
    test_bitwise();
    test_control_flow();
    test_break_continue();
    test_local_pointers();
    test_comprehensive();
    test_buf_get_set();
    test_buf_packing();
    test_buf_memset();
    test_buf_memmove();
    test_buf_overlap();
    test_buf_cmp();
    test_buf_full();
    test_buf_generic();
    test_alloc_basic();
    test_alloc_memmove();
    test_read_int();
    test_read_char();
    test_read_str();
    test_read_str_byte();
    return 0;
}
