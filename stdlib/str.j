_str_len(ptr) {
    i = 0;
    while (1) {
        c = _buf_get_u8(ptr, i);
        if (c == 0) { return i; }
        i = i + 1;
    }
}

_str_cmp(a, b) {
    // it does not matter if we compare untill length
    // of a or length of b the important thing is that
    // strings must terminate with a 0 byte.
    return _buf_cmp_u8(a, b, _str_len(a) + 1);
}

