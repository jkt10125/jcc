main() {
    p = buf;

    // u16 packing (big-endian): index 0 at bits 48-63, index 1 at 32-47
    _buf_set_u16(p, 0, 4660);   // 0x1234
    _buf_set_u16(p, 1, 43981);  // 0xABCD
    _print_int(_buf_get_u16(p, 0));
    _print_char(10);
    _print_int(_buf_get_u16(p, 1));
    _print_char(10);

    // u32 packing (big-endian): index 0 at bits 32-63, index 1 at 0-31
    _buf_set_u32(p, 0, 305419896);   // 0x12345678
    _buf_set_u32(p, 1, 2596069104);  // 0x9ABCDEF0
    _print_int(_buf_get_u32(p, 0));
    _print_char(10);
    _print_int(_buf_get_u32(p, 1));
    _print_char(10);

    return 0;
}
