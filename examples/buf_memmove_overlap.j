main() {
    p = buf;
    p = p + 128;
    _buf_set_u8(p, 0, 65);
    _buf_set_u8(p, 1, 66);
    _buf_set_u8(p, 2, 67);
    _buf_set_u8(p, 3, 0);

    // overlap: move right by 1 (dst > src) => must copy backwards
    j = 3;
    while (j > 0) {
        j = j - 1;
        _buf_set_u8(p, j + 1, _buf_get_u8(p, j));
    }

    _print_int(_buf_get_u8(p, 0)); _print_char(10);
    _print_int(_buf_get_u8(p, 1)); _print_char(10);
    _print_int(_buf_get_u8(p, 2)); _print_char(10);
    _print_int(_buf_get_u8(p, 3)); _print_char(10);
    return 0;
}
