// Packed buffer helpers.
// ptr is an int64 pointer; elements are packed BIG-ENDIAN within each 64-bit word.
// Byte 0 (idx 0) is MSB, so comparing two words as integers matches lexicographic byte comparison.
// offset: 3=u8, 2=u16, 1=u32 (elements per word = 2^offset).

__buf_get(ptr, idx, offset) {
    word_index = idx >> offset;
    offset_mask = (1 << offset) - 1;
    bits = 8 << (3 - offset);
    offset_power_mask = (1 << bits) - 1;
    elem_in_word = idx & offset_mask;
    bit_shift = ((1 << offset) - 1 - elem_in_word) << (6 - offset);
    word = ptr[word_index];
    return (word >> bit_shift) & offset_power_mask;
}

__buf_set(ptr, idx, val, offset) {
    word_index = idx >> offset;
    offset_mask = (1 << offset) - 1;
    bits = 8 << (3 - offset);
    offset_power_mask = (1 << bits) - 1;
    elem_in_word = idx & offset_mask;
    bit_shift = ((1 << offset) - 1 - elem_in_word) << (6 - offset);
    word = ptr[word_index];
    mask = offset_power_mask << bit_shift;
    inv_mask = mask ^ -1;
    new_word = (word & inv_mask) | ((val & offset_power_mask) << bit_shift);
    ptr[word_index] = new_word;
    return 0;
}

__buf_memmove(get_fn, set_fn, dst_ptr, src_ptr, count) {
    if (count <= 0) { return dst_ptr; }
    if (dst_ptr == src_ptr) { return dst_ptr; }
    if (dst_ptr < src_ptr) {
        i = 0;
        while (i < count) {
            val = get_fn(src_ptr, i);
            set_fn(dst_ptr, i, val);
            i = i + 1;
        }
        return dst_ptr;
    }
    i = count;
    while (i > 0) {
        i = i - 1;
        val = get_fn(src_ptr, i);
        set_fn(dst_ptr, i, val);
    }
    return dst_ptr;
}

__buf_cmp(get_fn, a_ptr, b_ptr, count) {
    i = 0;
    while (i < count) {
        a = get_fn(a_ptr, i);
        b = get_fn(b_ptr, i);
        if (a != b) {
            if (a < b) { return -1; }
            return 1;
        }
        i = i + 1;
    }
    return 0;
}

__buf_memset(set_fn, dst_ptr, val, count) {
    i = 0;
    while (i < count) {
        set_fn(dst_ptr, i, val);
        i = i + 1;
    }
    return dst_ptr;
}

_buf_get_u8(ptr, idx) { return __buf_get(ptr, idx, 3); }
_buf_get_u16(ptr, idx) { return __buf_get(ptr, idx, 2); }
_buf_get_u32(ptr, idx) { return __buf_get(ptr, idx, 1); }
_buf_get_u64(ptr, idx) { return ptr[idx]; }

_buf_set_u8(ptr, idx, val) { __buf_set(ptr, idx, val, 3); return 0; }
_buf_set_u16(ptr, idx, val) { __buf_set(ptr, idx, val, 2); return 0; }
_buf_set_u32(ptr, idx, val) { __buf_set(ptr, idx, val, 1); return 0; }
_buf_set_u64(ptr, idx, val) { ptr[idx] = val; return 0; }

_buf_memmove_u8(dst, src, count) {
    if (count <= 0) { return dst; }
    if (dst == src) { return dst; }
    fullWords = count >> 3;
    remainder = count - (fullWords << 3);
    __buf_memmove(&_buf_get_u64, &_buf_set_u64, dst, src, fullWords);
    dstBase = dst + (fullWords << 3);
    srcBase = src + (fullWords << 3);
    __buf_memmove(&_buf_get_u8, &_buf_set_u8, dstBase, srcBase, remainder);
    return dst;
}

_buf_memmove_u16(dst, src, count) {
    if (count <= 0) { return dst; }
    if (dst == src) { return dst; }
    fullWords = count >> 2;
    remainder = count - (fullWords << 2);
    __buf_memmove(&_buf_get_u64, &_buf_set_u64, dst, src, fullWords);
    dstBase = dst + (fullWords << 3);
    srcBase = src + (fullWords << 3);
    __buf_memmove(&_buf_get_u16, &_buf_set_u16, dstBase, srcBase, remainder);
    return dst;
}

_buf_memmove_u32(dst, src, count) {
    if (count <= 0) { return dst; }
    if (dst == src) { return dst; }
    fullWords = count >> 1;
    remainder = count - (fullWords << 1);
    __buf_memmove(&_buf_get_u64, &_buf_set_u64, dst, src, fullWords);
    dstBase = dst + (fullWords << 3);
    srcBase = src + (fullWords << 3);
    __buf_memmove(&_buf_get_u32, &_buf_set_u32, dstBase, srcBase, remainder);
    return dst;
}

_buf_memmove_u64(dst, src, count) { return __buf_memmove(&_buf_get_u64, &_buf_set_u64, dst, src, count); }

_buf_cmp_u8(a, b, count) {
    if (count <= 0) { return 0; }
    fullWords = count >> 3;
    remainder = count - (fullWords << 3);
    cmp = __buf_cmp(&_buf_get_u64, a, b, fullWords);
    if (cmp != 0) { return cmp; }
    return __buf_cmp(&_buf_get_u8, a + (fullWords << 3), b + (fullWords << 3), remainder);
}
_buf_cmp_u16(a, b, count) {
    if (count <= 0) { return 0; }
    fullWords = count >> 2;
    remainder = count - (fullWords << 2);
    cmp = __buf_cmp(&_buf_get_u64, a, b, fullWords);
    if (cmp != 0) { return cmp; }
    return __buf_cmp(&_buf_get_u16, a + (fullWords << 3), b + (fullWords << 3), remainder);
}
_buf_cmp_u32(a, b, count) {
    if (count <= 0) { return 0; }
    fullWords = count >> 1;
    remainder = count - (fullWords << 1);
    cmp = __buf_cmp(&_buf_get_u64, a, b, fullWords);
    if (cmp != 0) { return cmp; }
    return __buf_cmp(&_buf_get_u32, a + (fullWords << 3), b + (fullWords << 3), remainder);
}
_buf_cmp_u64(a, b, count) { return __buf_cmp(&_buf_get_u64, a, b, count); }

_buf_memset_u8(dst, val, count) {
    if (count <= 0) { return dst; }
    packed = val | (val << 8);
    packed = packed | (packed << 16);
    packed = packed | (packed << 32);
    fullWords = count >> 3;
    remainder = count - (fullWords << 3);
    __buf_memset(&_buf_set_u64, dst, packed, fullWords);
    __buf_memset(&_buf_set_u8, dst + (fullWords << 3), val, remainder);
    return dst;
}

_buf_memset_u16(dst, val, count) {
    if (count <= 0) { return dst; }
    packed = val | (val << 16);
    packed = packed | (packed << 32);
    fullWords = count >> 2;
    remainder = count - (fullWords << 2);
    __buf_memset(&_buf_set_u64, dst, packed, fullWords);
    __buf_memset(&_buf_set_u16, dst + (fullWords << 3), val, remainder);
    return dst;
}

_buf_memset_u32(dst, val, count) {
    if (count <= 0) { return dst; }
    packed = val | (val << 32);
    fullWords = count >> 1;
    remainder = count - (fullWords << 1);
    __buf_memset(&_buf_set_u64, dst, packed, fullWords);
    __buf_memset(&_buf_set_u32, dst + (fullWords << 3), val, remainder);
    return dst;
}

_buf_memset_u64(dst, val, count) { return __buf_memset(&_buf_set_u64, dst, val, count); }
