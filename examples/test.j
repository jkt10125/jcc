main() {
    tmp = _read_str();
    _str_cpy(mem + 8, tmp);
    a = 67;
    a = a | (a << 8);
    a = a | (a << 16);
    a = a | (a << 32);
    mem[0] = a;
    _print_str(mem);
    return 0;
}