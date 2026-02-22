main() {
    tmp = buf;
    _buf_set_u8(tmp, 0, 0x68);
    _buf_set_u8(tmp, 1, 0x65);
    _buf_set_u8(tmp, 2, 0x6C);
    _buf_set_u8(tmp, 3, 0x6C);
    _buf_set_u8(tmp, 4, 0x6F);
    _buf_set_u8(tmp, 5, 0);
    len = _str_len(tmp);
    a = _alloc((len + 8) / 8);
    _str_cpy(a, tmp);
    _print_str(a);
    _print_char(10);
    return 0;
}
