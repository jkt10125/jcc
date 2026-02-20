__print(x) {
    rt_put_int(x);
    return 0;
}

__exit(code) {
    rt_exit(code);
    return 0;
}

__read_int() {
    return rt_get_int();
}

