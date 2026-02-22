main() {
    a = _alloc(1);
    b = _alloc(1);
    _buf_set_u8(a, 0, 0x68);
    _buf_set_u8(a, 1, 0x69);
    _buf_set_u8(a, 2, 0);
    _print_int(_buf_get_u8(a, 0)); _print_char(10);
    _print_int(_buf_get_u8(a, 1)); _print_char(10);
    _print_int(_buf_get_u8(a, 2)); _print_char(10);
    _buf_memmove_u8(b, a, 3);
    _print_int(_buf_get_u8(b, 0)); _print_char(10);
    _print_int(_buf_get_u8(b, 1)); _print_char(10);
    _print_int(_buf_get_u8(b, 2)); _print_char(10);
    return 0;
}
