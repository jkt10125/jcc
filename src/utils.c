#include "utils.h"
#include <stdlib.h>
#include <string.h>

char *strDup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char *p = malloc(n+1);
    memcpy(p, s, n);
    p[n]=0;
    return p;
}
char *strNDup(const char *s, size_t n) {
    char *p = malloc(n+1);
    memcpy(p, s, n);
    p[n]=0;
    return p;
}

