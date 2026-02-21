#!/bin/sh
# Run all buf-related tests
cd "$(dirname "$0")"
failed=0

run_test() {
    name="$1"
    expect="$2"
    ./jcc "$name" -m 1024 >/dev/null 2>&1 || { echo "FAIL $name: compile error"; return 1; }
    out=$(./a.out 2>&1)
    if [ "$out" != "$expect" ]; then
        echo "FAIL $name: output mismatch"
        echo "Expected: $expect"
        echo "Got: $out"
        return 1
    fi
    echo "OK $name"
    return 0
}

# buf_full_test: 10 lines of "1"
run_test examples/buf_full_test.j "1
1
1
1
1
1
1
1
1
1" || failed=$((failed+1))

# buf_packing: 4660, 43981, 305419896, 2596069104
run_test examples/buf_packing.j "4660
43981
305419896
2596069104" || failed=$((failed+1))

# buf_memmove_overlap: 65, 65, 66, 67
run_test examples/buf_memmove_overlap.j "65
65
66
67" || failed=$((failed+1))

# buf_memset_test: 65, 65, 65, 0
run_test examples/buf_memset_test.j "65
65
65
0" || failed=$((failed+1))

# comprehensive_test
./jcc examples/comprehensive_test.j -m 1024 >/dev/null 2>&1 || { echo "FAIL comprehensive_test: compile error"; failed=$((failed+1)); }
./a.out 2>/dev/null | diff - examples/comprehensive_test.expected >/dev/null 2>&1 || { echo "FAIL comprehensive_test: output mismatch"; failed=$((failed+1)); }
[ $failed -eq 0 ] && echo "OK comprehensive_test"

echo ""
[ $failed -eq 0 ] && echo "All buf tests passed" || echo "$failed test(s) failed"
exit $failed
