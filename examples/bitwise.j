mask7(n) {
    return n & 7;
}

main() {
    // --- pure bitwise ops ---
    a = 5;
    b = 3;
    __print_int(a & b);         // 1
    __print_int(a | 2);         // 7
    __print_int(a ^ 1);         // 4
    __print_int(1 << 3);        // 8

    // logical right shift (zero-fill), NOT arithmetic:
    __print_int(-1 >> 1);       // 9223372036854775807
    __print_int(-1 >> 63);      // 1

    // precedence sanity: shifts > & > ^ > | > comparisons
    __print_int((1 << 4) | (3 & 1));  // 17
    __print_int(1 << 4 | 3 & 1);      // also 17

    // --- address-of vs bitwise-and disambiguation ---
    x = 42;
    p = &x;                 // unary &: address-of
    __print_int(p[0]);          // 42

    // combine them: address is an int64, so it can be bitwise-anded too
    __print_int(p & 7);         // stack slot is 8-byte aligned => 0

    // (&x)[0] should parse as: address-of first, then indexing
    __print_int((&x)[0]);       // 42

    // bitwise-and between expressions (binary &)
    y = 15;
    __print_int(x & y);         // 10

    // store through pointer using bitwise operators
    p[0] = p[0] & 15;
    __print_int(x);             // 10

    // --- address-of function + indirect call via mem ---
    mem[0] = &mask7;
    __print_int(mem[0](42));    // 2

    return 0;
}

