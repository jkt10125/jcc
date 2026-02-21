main() {
    i = 0;
    sum = 0;
    while (i < 10) {
        i = i + 1;
        if (i == 5) { break; }
        sum = sum + i;
    }
    _print_int(sum);
    _print_char(10);
    _print_int(i);
    _print_char(10);

    i = 0;
    sum = 0;
    while (i < 10) {
        i = i + 1;
        if (i == 5) { continue; }
        sum = sum + i;
    }
    _print_int(sum);
    _print_char(10);
    _print_int(i);
    _print_char(10);

    i = 0;
    j = 0;
    while (i < 3) {
        i = i + 1;
        while (j < 5) {
            j = j + 1;
            if (j == 3) { break; }
        }
    }
    _print_int(i);
    _print_char(10);
    _print_int(j);
    _print_char(10);
}
