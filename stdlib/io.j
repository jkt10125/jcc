_print_int(x) {
    rt_put_int(x);
    return 0;
}

_print_hex(x) {
    _print_char(0x30); _print_char(0x78);  // "0x"
    i = 15;
    while (i >= 0) {
        nibble = (x >> (i << 2)) & 0xF;
        if (nibble < 10) { _print_char(0x30 + nibble); }
        else { _print_char(0x37 + nibble); }
        i = i - 1;
    }
    _print_char(0x0A);  // newline
    return 0;
}

_print_char(x) {
    rt_put_char(x);
    return 0;
}

_exit(code) {
    rt_exit(code);
    return 0;
}

_read_int() {
    return rt_get_int();
}

_read_char() {
    return rt_read_char();
}

__read_str_into_buf() {
    i = 0;
    while (i < _buf_size) {
        c = _read_char();
        if ((c <= 0) | (c == 0x0A)) {
            _buf_set_u8(_buf, i, 0);
            return i;
        }
        _buf_set_u8(_buf, i, c);
        i = i + 1;
    }
    _buf_set_u8(_buf, _buf_size, 0);
    return _buf_size;
}

_print_str(x) {
    i = 0;
    while (1) {
        c = _buf_get_u8(x, i);
        if (c == 0) { return 0; }
        _print_char(c);
        i = i + 1;
    }
}
