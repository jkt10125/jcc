#include <stdint.h>

extern int64_t lang_main(void);
int64_t memArray[MEM_ENTRIES];
uint64_t mem;

int main(void) {
    mem = (uint64_t)memArray;
    int64_t ret = lang_main();
    return (int)(ret & 0xFFFFFFFF);
}

// simple print helper exposed to generated code
void printInt(long long v) {
    /* use printf from libc */
    extern int printf(const char *, ...);
    printf("%lld\n", v);
}

