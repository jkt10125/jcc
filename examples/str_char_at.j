main() {
    p = _read_str();
    _print_str(p);
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _print_int(_buf_get_u8(p, 1));
    _print_char(10);
    _print_int(_buf_get_u8(p, 2));
    _print_char(10);
    return 0;
}

