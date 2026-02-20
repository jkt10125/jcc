main() {
    x = 0;
    sum = 0;

    while (x < 5) {
        sum = sum + x;
        x = x + 1;
    }

    if (sum == 10) {
        print(111);
    } else {
        print(222);
    }

    if (sum != 10) {
        print(333);
    } else {
        print(444);
    }

    // sum should be 10
    print(sum);
    return sum;
}

