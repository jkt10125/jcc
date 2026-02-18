add(a, b) {
    return a + b;
}

main() {
    mem[0] = &add;
    x = 5;
    y = 10;
    mem[1] = 20;
    result = mem[0](x, y) - mem[1];
    print(result);
    return result;
}

