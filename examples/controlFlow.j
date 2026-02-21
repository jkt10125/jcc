main() {
    x = 0;
    sum = 0;

    while (x < 5) {
        sum = sum + x;
        x = x + 1;
    }

    if (sum == 10) {
        _print_int(111);
        _print_char(10);
    } else {
        _print_int(222);
        _print_char(10);
    }

    if (sum != 10) {
        _print_int(333);
        _print_char(10);
    } else {
        _print_int(444);
        _print_char(10);
    }

    // sum should be 10
    _print_int(sum);
    _print_char(10);
    return sum;
}

