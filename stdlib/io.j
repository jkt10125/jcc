__print_int(x) {
    rt_put_int(x);
    return 0;
}

__print_char(x) {
    rt_put_char(x);
    return 0;
}

__exit(code) {
    rt_exit(code);
    return 0;
}

__read_int() {
    return rt_get_int();
}

__read_char() {
    return rt_read_char();
}

