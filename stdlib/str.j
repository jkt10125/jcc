__read_str() {
    buf = rt_str_buf_ptr();
    i = 0;
    while (i < 4096) {
        c = __read_char();
        if ((c < 0) | (c == 10)) {
            __str_set_byte(buf, i, 0);
            return buf;
        }
        __str_set_byte(buf, i, c);
        i = i + 1;
    }
    __str_set_byte(buf, 4096, 0);
    return buf;
}

__str_set_byte(ptr, idx, val) {
    word_index = idx >> 3;
    byte_shift = (idx & 7) << 3;
    word = ptr[word_index];
    mask = 255 << byte_shift;
    inv_mask = mask ^ -1;
    new_word = (word & inv_mask) | ((val & 255) << byte_shift);
    ptr[word_index] = new_word;
    return 0;
}

__str_get_byte(ptr, idx) {
    word_index = idx >> 3;
    byte_shift = (idx & 7) << 3;
    word = ptr[word_index];
    return (word >> byte_shift) & 255;
}

__print_str(ptr) {
    i = 0;
    while (1) {
        c = __str_get_byte(ptr, i);
        if (c == 0) { return 0; }
        __print_char(c);
        i = i + 1;
    }
}

