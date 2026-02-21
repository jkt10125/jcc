mask7(n) {
    return n & 7;
}

main() {
    // --- pure bitwise ops ---
    a = 5;
    b = 3;
    _print_int(a & b);         // 1
    _print_char(10);
    _print_int(a | 2);         // 7
    _print_char(10);
    _print_int(a ^ 1);         // 4
    _print_char(10);
    _print_int(1 << 3);        // 8
    _print_char(10);

    // logical right shift (zero-fill), NOT arithmetic:
    _print_int(-1 >> 1);       // 9223372036854775807
    _print_char(10);
    _print_int(-1 >> 63);      // 1
    _print_char(10);

    // precedence sanity: shifts > & > ^ > | > comparisons
    _print_int((1 << 4) | (3 & 1));  // 17
    _print_char(10);
    _print_int(1 << 4 | 3 & 1);      // also 17
    _print_char(10);

    // --- address-of vs bitwise-and disambiguation ---
    x = 42;
    p = &x;                 // unary &: address-of
    _print_int(p[0]);          // 42
    _print_char(10);

    // combine them: address is an int64, so it can be bitwise-anded too
    _print_int(p & 7);         // stack slot is 8-byte aligned => 0
    _print_char(10);

    // (&x)[0] should parse as: address-of first, then indexing
    _print_int((&x)[0]);       // 42
    _print_char(10);

    // bitwise-and between expressions (binary &)
    y = 15;
    _print_int(x & y);         // 10
    _print_char(10);

    // store through pointer using bitwise operators
    p[0] = p[0] & 15;
    _print_int(x);             // 10
    _print_char(10);

    // --- address-of function + indirect call via mem ---
    mem[0] = &mask7;
    _print_int(mem[0](42));    // 2
    _print_char(10);

    return 0;
}

