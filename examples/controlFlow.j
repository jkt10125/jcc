main() {
    x = 0;
    sum = 0;

    while (x < 5) {
        sum = sum + x;
        x = x + 1;
    }

    if (sum == 10) {
        __print(111);
    } else {
        __print(222);
    }

    if (sum != 10) {
        __print(333);
    } else {
        __print(444);
    }

    // sum should be 10
    __print(sum);
    return sum;
}

