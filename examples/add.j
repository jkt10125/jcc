add(a, b, c, d, e, f, g, h, i, j) {
    return a * 2 + b + c + d + e + f + g + h + i + j * 10;
}

main() {
    mem[0] = &add;
    x = 1;
    y = 1;
    mem[1] = 20;
    result = mem[0](1,0,0,0,0,0,0,0,0,mem[0](1)) - mem[1];
    __print(result);
    return 0;
}

