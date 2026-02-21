print_char(a) {
        tmp = (a >> 56) & 255;
        _print_char(tmp);
    _print_char(10);
    return 0;
}

main() {
    a = 67;
    a = a | (a << 8);
    a = a | (a << 16);
    a = a | (a << 32);
    mem[0] = a;
    //mem[0] = 0;
    _buf_set_u8(mem, 8, 68);
    _print_hex(mem[1]);
    _buf_memmove_u8(mem + 4, mem, 9);
    _print_hex(mem[1]);
    return 0;
}