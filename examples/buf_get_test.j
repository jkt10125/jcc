main() {
    p = buf;
    _buf_set_u8(p, 0, 0x68);
    _buf_set_u8(p, 1, 0x69);
    _buf_set_u8(p, 2, 0);
    _print_int(_buf_get_u8(p, 0)); _print_char(10);
    _print_int(_buf_get_u8(p, 1)); _print_char(10);
    _print_int(_buf_get_u8(p, 2)); _print_char(10);
    return 0;
}
