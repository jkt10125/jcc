main() {
    a = _alloc(1);
    _buf_set_u8(a, 0, 0x68);
    _buf_set_u8(a, 1, 0x69);
    _buf_set_u8(a, 2, 0);
    _print_str(a);
    _print_char(10);
    return 0;
}
