main() {
    p = _read_str();
    _print_int(_buf_get_u8(p, 0));
    _print_char(10);
    _print_str(p);
    return 0;
}

