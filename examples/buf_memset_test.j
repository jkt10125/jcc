main() {
    p = rt_str_buf_ptr();
    _buf_memset_u8(p, 65, 4);
    _print_int(_buf_get_u8(p, 0)); _print_char(10);
    _print_int(_buf_get_u8(p, 1)); _print_char(10);
    _print_int(_buf_get_u8(p, 2)); _print_char(10);
    _print_int(_buf_get_u8(p, 3)); _print_char(10);
    return 0;
}
