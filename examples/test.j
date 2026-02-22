print_char(a) {
        tmp = (a >> 56) & 255;
        _print_char(tmp);
    _print_char(10);
    return 0;
}

main() {
    h = 90;
    a = _read_str();
    print_char();
    b = _read_str();

    _print_hex(a);
    _print_hex(b);

    _print_hex(a[0]);
    _print_hex((a + 8)[0]);
    _print_hex(b[0]);
    
    return 0;
}