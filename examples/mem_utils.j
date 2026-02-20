main() {
    a0 = __read_int();
    a1 = 22;
    a2 = 33;

    b0 = 0;
    b1 = 0;
    b2 = 0;

    // For local pointer indexing, use the lowest-address slot as the base.
    src_ptr = &a2;
    dst_ptr = &b2;

    __mem_copy_words(dst_ptr, src_ptr, 3);
    __print(dst_ptr[2]);
    __print(dst_ptr[1]);
    __print(dst_ptr[0]);

    __print(__mem_compare_words(src_ptr, dst_ptr, 3));
    dst_ptr[1] = 99;
    __print(__mem_compare_words(src_ptr, dst_ptr, 3));
    return 0;
}

