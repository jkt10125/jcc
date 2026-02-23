#!/bin/bash
# Test suite for jcc mini-compiler (single combined test file)
# Run from project root: ./tests/run_tests.sh

set -e
cd "$(dirname "$0")/.."

JCC="./jcc"
MEM=1024

# Build compiler
echo "Building jcc..."
make -s jcc 2>/dev/null || { echo "Build failed"; exit 1; }
echo ""

# Compile combined test
echo "Compiling tests/all.j..."
if ! $JCC -m $MEM tests/all.j 2>/dev/null; then
    echo "FAIL: compile error"
    exit 1
fi

# Run with stdin for I/O tests (read_int, read_char, read_str, read_str_byte)
echo "Running tests..."
if printf '42\nX\nhello\nworld\n' | ./a.out >/dev/null 2>&1; then
    echo "PASS: all 21 tests (100%)"
    exit 0
else
    echo "FAIL: exit code $?"
    exit 1
fi
