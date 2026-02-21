_read_str() {
    buf = rt_str_buf_ptr();
    i = 0;
    while (i < 4096) {
        c = _read_char();
        if (c < 0) { _buf_set_u8(buf, i, 0); return buf; }
        if (c == 10) { _buf_set_u8(buf, i, 0); return buf; } // newline
        _buf_set_u8(buf, i, c);
        i = i + 1;
    }
    _buf_set_u8(buf, 4096, 0);
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

