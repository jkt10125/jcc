// Math and utility helpers.

_abs(x) {
    if (x >= 0) { return x; }
    return 0 - x;
}

_min(a, b) {
    if (a < b) { return a; }
    return b;
}

_max(a, b) {
    if (a > b) { return a; }
    return b;
}
