main() {
    n = 2;
    p = _alloc(n);
    _buf_set_u8(p, 0, 0x68);
    _buf_set_u8(p, 1, 0x69);
    _buf_set_u8(p, 2, 0);
    _print_str(p);
    _print_char(10);
    return 0;
}
