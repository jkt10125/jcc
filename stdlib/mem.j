__mem_set_words(dst_ptr, value, count) {
    i = 0;
    while (i < count) {
        dst_ptr[i] = value;
        i = i + 1;
    }
    return dst_ptr;
}

__mem_copy_words(dst_ptr, src_ptr, count) {
    i = 0;
    while (i < count) {
        dst_ptr[i] = src_ptr[i];
        i = i + 1;
    }
    return dst_ptr;
}

__mem_move_words(dst_ptr, src_ptr, count) {
    if (count <= 0) { return dst_ptr; }
    if (dst_ptr < src_ptr) {
        i = 0;
        while (i < count) {
            dst_ptr[i] = src_ptr[i];
            i = i + 1;
        }
        return dst_ptr;
    }
    i = count - 1;
    while (i >= 0) {
        dst_ptr[i] = src_ptr[i];
        i = i - 1;
    }
    return dst_ptr;
}

__mem_compare_words(a_ptr, b_ptr, count) {
    i = 0;
    while (i < count) {
        a = a_ptr[i];
        b = b_ptr[i];
        if (a != b) { return a - b; }
        i = i + 1;
    }
    return 0;
}

