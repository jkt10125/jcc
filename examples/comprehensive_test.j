// Comprehensive test suite: all features + edge cases.
// Run: ./jcc examples/comprehensive_test.j -m 1024 && ./a.out
//
// Covers: arithmetic, bitwise, control flow, break/continue, pointers, function ptrs,
// math helpers, buf u8/u16/u32 get/set, string len/cpy, buf_memset, buf_memmove,
// buf_cmp, edge cases (count 0, same ptr, full-word cmp).

add(a, b, c, d, e, f, g, h, i, j) { return a * 2 + b + c + d + e + f + g + h + i + j * 10; }
mask7(n) { return n & 7; }
arithSuite(x, y) {
    t1 = x + y;
    t2 = x - y;
    t3 = x * y;
    t4 = x / y;
    t5 = x % y;
    return t1 + t2 * 10 + t3 * 100 + t4 * 1000 + t5 * 10000;
}
sum8(a, b, c, d, e, f, g, h) { return a + b + c + d + e + f + g + h; }

main() {
    // === 1. Arithmetic & precedence ===
    _print_int(2 + 3 * 4);
    _print_char(10);
    _print_int(17 / 5);
    _print_char(10);
    _print_int(17 % 5);
    _print_char(10);
    _print_int(arithSuite(17, 5));
    _print_char(10);
    _print_int(sum8(1, 2, 3, 4, 5, 6, 7, 8));
    _print_char(10);

    // === 2. Bitwise operators ===
    _print_int(5 & 3);
    _print_char(10);
    _print_int(5 | 2);
    _print_char(10);
    _print_int(5 ^ 1);
    _print_char(10);
    _print_int(1 << 3);
    _print_char(10);
    _print_int(-1 >> 1);
    _print_char(10);
    _print_int((1 << 4) | (3 & 1));
    _print_char(10);

    // === 3. Control flow (if/else, while) ===
    x = 0;
    sum = 0;
    while (x < 5) {
        sum = sum + x;
        x = x + 1;
    }
    _print_int(sum);
    _print_char(10);
    if (sum == 10) { _print_int(111); } else { _print_int(222); }
    _print_char(10);

    // === 4. Break & continue ===
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
    _print_int(i);
    _print_char(10);
    i = 0;
    j = 0;
    while (i < 3) {
        i = i + 1;
        while (j < 5) {
            j = j + 1;
            if (j == 3) { break; }
        }
    }
    _print_int(i);
    _print_char(10);
    _print_int(j);
    _print_char(10);

    // === 5. Local pointer indexing & address-of ===
    a = 10;
    b = 20;
    p = &b;
    p[0] = 777;
    _print_int(a);
    _print_char(10);
    _print_int(b);
    _print_char(10);
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

    // === 6. Function pointers & indirect calls ===
    mem[0] = &add;
    _print_int(mem[0](7, 8));
    _print_char(10);
    mem[0] = &mask7;
    _print_int(mem[0](42));
    _print_char(10);
    mem[0] = &add;
    mem[1] = 20;
    result = mem[0](1, 0, 0, 0, 0, 0, 0, 0, 0, mem[0](1)) - mem[1];
    _print_int(result);
    _print_char(10);

    // === 7. Math helpers ===
    _print_int(_abs(-42));
    _print_char(10);
    _print_int(_abs(7));
    _print_char(10);
    _print_int(_min(10, 3));
    _print_char(10);
    _print_int(_max(10, 3));
    _print_char(10);

    // === 8. Buffer u8 get/set & string length & copy ===
    p = rt_str_buf_ptr();
    _buf_set_u8(p, 0, 72);
    _buf_set_u8(p, 1, 105);
    _buf_set_u8(p, 2, 0);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _print_int(_buf_get_u8(p, 1));
    _print_char(10);
    _print_int(_str_len(p));
    _print_char(10);
    q = rt_str_buf_ptr();
    q = q + 512;
    _str_cpy(q, p);
    _print_int(_str_len(q));
    _print_char(10);

    // === 9. Buffer u16/u32 packing (big-endian) ===
    p2 = rt_str_buf_ptr();
    p2 = p2 + 64;
    _buf_set_u16(p2, 0, 4660);
    _buf_set_u16(p2, 1, 43981);
    _print_int(_buf_get_u16(p2, 0));
    _print_char(10);
    _print_int(_buf_get_u16(p2, 1));
    _print_char(10);
    _buf_set_u32(p2, 0, 305419896);
    _buf_set_u32(p2, 1, 2596069104);
    _print_int(_buf_get_u32(p2, 0));
    _print_char(10);
    _print_int(_buf_get_u32(p2, 1));
    _print_char(10);

    // === 10. buf_memset ===
    p = rt_str_buf_ptr();
    p = p + 256;
    _buf_memset_u8(p, 65, 4);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _print_int(_buf_get_u8(p, 1));
    _print_char(10);
    _buf_memset_u16(p, 4660, 2);
    _print_int(_buf_get_u16(p, 0));
    _print_char(10);
    _print_int(_buf_get_u16(p, 1));
    _print_char(10);

    // === 11. buf_memmove (non-overlap) + manual overlap copy ===
    p = rt_str_buf_ptr();
    p = p + 128;
    _buf_set_u8(p, 0, 65);
    _buf_set_u8(p, 1, 66);
    _buf_set_u8(p, 2, 67);
    _buf_set_u8(p, 3, 0);
    q = rt_str_buf_ptr();
    q = q + 200;
    _buf_memmove_u8(q, p, 4);
    _print_int(_buf_get_u8(q, 0));
    _print_char(10);
    _print_int(_buf_get_u8(q, 1));
    _print_char(10);
    _buf_set_u8(p, 0, 65);
    _buf_set_u8(p, 1, 66);
    _buf_set_u8(p, 2, 67);
    _buf_set_u8(p, 3, 0);
    j = 3;
    while (j > 0) {
        j = j - 1;
        _buf_set_u8(p, j + 1, _buf_get_u8(p, j));
    }
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _print_int(_buf_get_u8(p, 1));
    _print_char(10);
    _print_int(_buf_get_u8(p, 2));
    _print_char(10);
    _print_int(_buf_get_u8(p, 3));
    _print_char(10);

    // === 12. buf_cmp ===
    slot = 0;
    q = &slot;
    _buf_set_u8(p, 0, 65);
    _buf_set_u8(p, 1, 66);
    _buf_set_u8(p, 2, 67);
    _buf_set_u8(p, 3, 0);
    _buf_set_u8(q, 0, 65);
    _buf_set_u8(q, 1, 66);
    _buf_set_u8(q, 2, 67);
    _buf_set_u8(q, 3, 0);
    _print_int(_buf_cmp_u8(p, q, 4));
    _print_char(10);
    _buf_set_u8(q, 2, 68);
    _print_int(_buf_cmp_u8(p, q, 4));
    _print_char(10);
    _buf_set_u8(q, 2, 66);
    _buf_set_u8(q, 1, 65);
    _print_int(_buf_cmp_u8(p, q, 4));
    _print_char(10);

    // === 13. Edge cases: count 0, equal buffers, u64 cmp ===
    _print_int(_buf_cmp_u8(p, q, 0));
    _print_char(10);
    _print_int(_buf_cmp_u64(p, p, 1));
    _print_char(10);

    // === 14. Edge: buf_memset count 0, memmove same ptr ===
    _buf_memset_u8(p, 99, 0);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _buf_memmove_u8(p, p, 4);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);

    // === 15. Edge: negative, division by identity ===
    _print_int(0 - 7);
    _print_char(10);
    _print_int(_abs(-9223372036854775807));
    _print_char(10);

    // === 16. Edge: full-word packed cmp (8 bytes) ===
    _buf_set_u8(p, 0, 1);
    _buf_set_u8(p, 1, 2);
    _buf_set_u8(p, 2, 3);
    _buf_set_u8(p, 3, 4);
    _buf_set_u8(p, 4, 5);
    _buf_set_u8(p, 5, 6);
    _buf_set_u8(p, 6, 7);
    _buf_set_u8(p, 7, 8);
    _buf_set_u8(q, 0, 1);
    _buf_set_u8(q, 1, 2);
    _buf_set_u8(q, 2, 3);
    _buf_set_u8(q, 3, 4);
    _buf_set_u8(q, 4, 5);
    _buf_set_u8(q, 5, 6);
    _buf_set_u8(q, 6, 7);
    _buf_set_u8(q, 7, 8);
    _print_int(_buf_cmp_u8(p, q, 8));
    _print_char(10);
    _buf_set_u8(q, 0, 2);
    _print_int(_buf_cmp_u8(p, q, 8));
    _print_char(10);

    return 0;
}
