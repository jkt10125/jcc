// Comprehensive buf test: u8/u16/u32 memmove, cmp, memset with multi-word counts.
// Exercises fullWords << 3 fix for u16 and u32.

main() {
    p = rt_str_buf_ptr();
    p = p + 128;
    q = rt_str_buf_ptr();
    q = q + 256;

    // === u8: 10 bytes (1 full word + 2 remainder) ===
    _buf_set_u8(p, 0, 10);
    _buf_set_u8(p, 1, 20);
    _buf_set_u8(p, 2, 30);
    _buf_set_u8(p, 3, 40);
    _buf_set_u8(p, 4, 50);
    _buf_set_u8(p, 5, 60);
    _buf_set_u8(p, 6, 70);
    _buf_set_u8(p, 7, 80);
    _buf_set_u8(p, 8, 90);
    _buf_set_u8(p, 9, 99);
    _buf_memmove_u8(q, p, 10);
    ok = 1;
    if (_buf_get_u8(q, 0) != 10) { ok = 0; }
    if (_buf_get_u8(q, 7) != 80) { ok = 0; }
    if (_buf_get_u8(q, 9) != 99) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === u16: 5 elements (1 full word + 1 remainder) ===
    _buf_set_u16(p, 0, 100);
    _buf_set_u16(p, 1, 200);
    _buf_set_u16(p, 2, 300);
    _buf_set_u16(p, 3, 400);
    _buf_set_u16(p, 4, 500);
    _buf_memmove_u16(q, p, 5);
    if (_buf_get_u16(q, 0) != 100) { ok = 0; }
    if (_buf_get_u16(q, 3) != 400) { ok = 0; }
    if (_buf_get_u16(q, 4) != 500) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === u32: 3 elements (1 full word + 1 remainder) ===
    _buf_set_u32(p, 0, 1111);
    _buf_set_u32(p, 1, 2222);
    _buf_set_u32(p, 2, 3333);
    _buf_memmove_u32(q, p, 3);
    if (_buf_get_u32(q, 0) != 1111) { ok = 0; }
    if (_buf_get_u32(q, 1) != 2222) { ok = 0; }
    if (_buf_get_u32(q, 2) != 3333) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === u16 cmp: 5 elements ===
    _buf_set_u16(p, 0, 100);
    _buf_set_u16(p, 1, 200);
    _buf_set_u16(p, 2, 300);
    _buf_set_u16(p, 3, 400);
    _buf_set_u16(p, 4, 500);
    _buf_set_u16(q, 0, 100);
    _buf_set_u16(q, 1, 200);
    _buf_set_u16(q, 2, 300);
    _buf_set_u16(q, 3, 400);
    _buf_set_u16(q, 4, 500);
    if (_buf_cmp_u16(p, q, 5) != 0) { ok = 0; }
    _buf_set_u16(q, 2, 999);
    if (_buf_cmp_u16(p, q, 5) != -1) { ok = 0; }
    _buf_set_u16(q, 2, 1);
    if (_buf_cmp_u16(p, q, 5) != 1) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === u32 cmp: 3 elements ===
    _buf_set_u32(p, 0, 1111);
    _buf_set_u32(p, 1, 2222);
    _buf_set_u32(p, 2, 3333);
    _buf_set_u32(q, 0, 1111);
    _buf_set_u32(q, 1, 2222);
    _buf_set_u32(q, 2, 3333);
    if (_buf_cmp_u32(p, q, 3) != 0) { ok = 0; }
    _buf_set_u32(q, 1, 9999);
    if (_buf_cmp_u32(p, q, 3) != -1) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === u16 memset: 5 elements ===
    _buf_memset_u16(p, 43981, 5);
    if (_buf_get_u16(p, 0) != 43981) { ok = 0; }
    if (_buf_get_u16(p, 4) != 43981) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === u32 memset: 3 elements ===
    _buf_memset_u32(p, 305419896, 3);
    if (_buf_get_u32(p, 0) != 305419896) { ok = 0; }
    if (_buf_get_u32(p, 2) != 305419896) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === edge: count 0 ===
    _buf_memmove_u16(q, p, 0);
    _buf_memset_u32(p, 99, 0);
    if (_buf_cmp_u16(p, q, 0) != 0) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    // === edge: exact full words (u16: 4, u32: 2) ===
    _buf_set_u16(p, 0, 1);
    _buf_set_u16(p, 1, 2);
    _buf_set_u16(p, 2, 3);
    _buf_set_u16(p, 3, 4);
    _buf_memmove_u16(q, p, 4);
    if (_buf_get_u16(q, 3) != 4) { ok = 0; }
    _buf_set_u32(p, 0, 111);
    _buf_set_u32(p, 1, 222);
    _buf_memmove_u32(q, p, 2);
    if (_buf_get_u32(q, 1) != 222) { ok = 0; }
    _print_int(ok);
    _print_char(10);

    _print_int(ok);
    _print_char(10);
    return 0;
}
