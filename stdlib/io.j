_print_int(x) {
    rt_put_int(x);
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

