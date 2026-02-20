add(a, b) { return a + b; }
sub(a, b) { return a - b; }
mul(a, b) { return a * b; }
divi(a, b) { return a / b; }
modi(a, b) { return a % b; }

sum8(a, b, c, d, e, f, g, h) {
    return a + b + c + d + e + f + g + h;
}

getAdd() { return &add; }

callViaMem(fn, a, b) {
    mem[10] = fn;
    return mem[10](a, b);
}

arithSuite(x, y) {
    t1 = x + y;
    t2 = x - y;
    t3 = x * y;
    t4 = x / y;
    t5 = x % y;
    return t1 + t2 * 10 + t3 * 100 + t4 * 1000 + t5 * 10000;
}

main() {
    __print_int(2 + 3 * 4);

    mem[0] = &add;
    __print_int(mem[0](mem[0](1, 2), 3));
    __print_int(mem[0](5, 6));

    mem[0] = &mul;
    __print_int(mem[0](5, 6));

    mem[0] = getAdd();
    __print_int(mem[0](7, 8));

    __print_int(callViaMem(&add, 9, 10));

    mem[1] = 20;
    mem[2] = 3;
    mem[3] = 4;

    mem[5] = 123;
    __print_int(mem[5]);

    mem[5] = 0 - 7;
    __print_int(mem[5]);
    __print_int(0 - mem[2]);

    __print_int(divi(100, 3));
    __print_int(modi(100, 3));

    __print_int(arithSuite(17, 5));

    // >6 arguments (stack arguments test)
    __print_int(sum8(1, 2, 3, 4, 5, 6, 7, 8));

    mem[6] = &add;
    mem[7] = &sub;
    __print_int(mem[6](50, 1));
    __print_int(mem[7](50, 1));

    mem[8] = callViaMem(mem[6], 1, 2);
    __print_int(mem[8]);

    mem[0] = &add;
    result = 0;
    result = result + 1;
    result = result + mem[1];
    result = result + mem[0](1, 2);
    __print_int(result);
    return result;
}

