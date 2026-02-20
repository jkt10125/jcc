main() {
    p = __read_str();
    __print_str(p);
    __print_int(__str_get_byte(p, 0));
    __print_int(__str_get_byte(p, 1));
    __print_int(__str_get_byte(p, 2));
    __print_int(__str_get_byte(p, 1000)); // past end => 0
    return 0;
}

