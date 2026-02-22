main() {
    tmp = _read_str();
    len = _str_len(tmp);
    a = _alloc((len + 8) / 8);
    _str_cpy(a, tmp);
    _print_str(a);
    _print_char(10);
    return 0;
}
