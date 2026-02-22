main() {
    tmp = buf;
    _buf_set_u8(tmp, 0, 0x68);
    _buf_set_u8(tmp, 1, 0x69);
    _buf_set_u8(tmp, 2, 0);
    a = mem;
    _str_cpy(a, tmp);
    _print_str(a);
    _print_char(10);
    return 0;
}
