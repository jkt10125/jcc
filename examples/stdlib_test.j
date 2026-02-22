// Quick test of new stdlib features
main() {
    _print_int_ln(_abs(-42));   // 42
    _print_int_ln(_abs(7));     // 7
    _print_int_ln(_min(3, 9));  // 3
    _print_int_ln(_max(3, 9));  // 9
    _print_ln();
    _print_int_ln(_buf_find_u8(mem, 0, 8));  // 0 (mem is zero-initialized)
    return 0;
}
