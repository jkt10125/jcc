_str_len(ptr) {
    i = 0;
    while (1) {
        c = _buf_get_u8(ptr, i);
        if (c == 0) { return i; }
        i = i + 1;
    }
}

_str_cpy(dst, src) {
    i = 0;
    while (1) {
        c = _buf_get_u8(src, i);
        _buf_set_u8(dst, i, c);
        if (c == 0) { return dst; }
        i = i + 1;
    }
}

