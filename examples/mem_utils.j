main() {
    a0 = _read_int();

    a0 = 22 / a0;

    a1 = 22;
    a2 = 33;

    b0 = 0;
    b1 = 0;
    b2 = 0;

    // For local pointer indexing, use the lowest-address slot as the base.
    src_ptr = &a2;
    dst_ptr = &b2;

    _buf_memmove(&_buf_get_u64, &_buf_set_u64, dst_ptr, src_ptr, 3);
    _print_int(dst_ptr[2]);
    _print_char(10);
    _print_int(dst_ptr[1]);
    _print_char(10);
    _print_int(dst_ptr[0]);
    _print_char(10);

    _print_int(_buf_cmp(&_buf_get_u64, src_ptr, dst_ptr, 3));
    _print_char(10);
    dst_ptr[1] = 99;
    _print_int(_buf_cmp(&_buf_get_u64, src_ptr, dst_ptr, 3));
    _print_char(10);
    return 0;
}

