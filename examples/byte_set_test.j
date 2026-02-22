main() {
    p = buf;
    _buf_set_u8(p, 0, 56);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _buf_set_u8(p, 1, 67);
    _print_int(_buf_get_u8(p, 1));
    _print_char(10);
    _buf_set_u8(p, 2, 10);
    _print_int(_buf_get_u8(p, 2));
    _print_char(10);
    _print_str(p);
    return 0;
}

