_str_len(ptr) {
    i = 0;
    while (1) {
        c = _buf_get_u8(ptr, i);
        if (c == 0) { return i; }
        i = i + 1;
    }
}

_str_cpy(dst, src) {
    return _buf_memmove_u8(dst, src, _str_len(src) + 1);
}

