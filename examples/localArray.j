main() {
    x = 11;
    y = 22;
    z = 33;

    // Take address of a local.
    p = &z;

    // Local array-style access via pointer:
    // Because locals are laid out on the stack, `&z` points to the lowest-address slot
    // among (x,y,z) in this function, so p[1] refers to y and p[2] refers to x.
    p[0] = 100;
    p[1] = 200;
    p[2] = 300;

    __print_int(x);
    __print_int(y);
    __print_int(z);
    __print_int(p[1]);

    __print_int(x + y + z);

    return x + y + z;
}

