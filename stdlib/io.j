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

_read_str() {
    buf = rt_str_buf_ptr();
    i = 0;
    while (i < 0x1000) {
        c = _read_char();
        if (c < 0) { _buf_set_u8(buf, i, 0); return buf; }
        if (c == 0x0A) { _buf_set_u8(buf, i, 0); return buf; }  // newline
        _buf_set_u8(buf, i, c);
        i = i + 1;
    }
    _buf_set_u8(buf, 0x1000, 0);
    return buf;
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
