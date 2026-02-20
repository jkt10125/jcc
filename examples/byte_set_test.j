main() {
    p = rt_str_buf_ptr();
    __str_set_byte(p, 0, 56);
    __print_int(__str_get_byte(p, 0));
    __str_set_byte(p, 1, 67);
    __print_int(__str_get_byte(p, 1));
    __str_set_byte(p, 2, 10);
    __print_int(__str_get_byte(p, 2));
    __print_str(p);
    return 0;
}

