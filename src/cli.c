#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "parser.h"
#include "codegen_direct.h"
#include "utils.h"

static char *readFile(const char *path) {
    FILE *f = fopen(path,"rb");
    if (!f) { perror("fopen"); return NULL; }
    fseek(f,0,SEEK_END); long sz = ftell(f); fseek(f,0,SEEK_SET);
    char *buf = malloc(sz+1); fread(buf,1,sz,f); buf[sz]=0; fclose(f); return buf;
}

static int endsWith(const char *s, const char *suffix) {
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    if (m > n) return 0;
    return strcmp(s + (n - m), suffix) == 0;
}

static void appendStr(char **buf, size_t *len, size_t *cap, const char *s) {
    size_t n = strlen(s);
    size_t need = len[0] + n + 1;
    if (need > cap[0]) {
        size_t newCap = cap[0] ? cap[0] : 4096;
        while (newCap < need) newCap *= 2;
        buf[0] = realloc(buf[0], newCap);
        cap[0] = newCap;
    }
    memcpy(buf[0] + len[0], s, n);
    len[0] += n;
    buf[0][len[0]] = 0;
}

static int cmpCstrPtr(const void *a, const void *b) {
    const char *sa = *(const char * const *)a;
    const char *sb = *(const char * const *)b;
    return strcmp(sa, sb);
}

static char *readStdlibSources(const char *dirPath) {
    DIR *d = opendir(dirPath);
    if (!d) { perror("opendir stdlib"); return NULL; }

    char **names = NULL;
    int count = 0;
    int cap = 0;
    for (;;) {
        struct dirent *ent = readdir(d);
        if (!ent) break;
        const char *name = ent->d_name;
        if (name[0] == '.') continue;
        if (!endsWith(name, ".j")) continue;
        if (count == cap) {
            cap = cap ? cap * 2 : 16;
            names = realloc(names, sizeof(char*) * (size_t)cap);
        }
        names[count++] = strDup(name);
    }
    closedir(d);

    qsort(names, (size_t)count, sizeof(char*), cmpCstrPtr);

    char *out = NULL;
    size_t outLen = 0;
    size_t outCap = 0;
    for (int i = 0; i < count; i++) {
        const char *name = names[i];
        size_t pathLen = strlen(dirPath) + 1 + strlen(name) + 1;
        char *path = malloc(pathLen);
        snprintf(path, pathLen, "%s/%s", dirPath, name);
        char *src = readFile(path);
        free(path);
        if (!src) {
            for (int j = 0; j < count; j++) free(names[j]);
            free(names);
            free(out);
            return NULL;
        }
        appendStr(&out, &outLen, &outCap, src);
        appendStr(&out, &outLen, &outCap, "\n");
        free(src);
    }
    for (int j = 0; j < count; j++) free(names[j]);
    free(names);

    if (!out) {
        out = malloc(1);
        out[0] = 0;
    }
    return out;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,"usage: jcc -m <memEntries> [ -o <out> ] <source>\n");
        return 1;
    }
    int memEntries = 0;
    char *outName = "a.out";
    char *srcPath = NULL;
    // parse options
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i],"-m")==0 && i+1<argc) { memEntries = atoi(argv[++i]); continue; }
        if (strcmp(argv[i],"-o")==0 && i+1<argc) { outName = argv[++i]; continue; }
        if (argv[i][0]=='-') { fprintf(stderr,"unknown option %s\n", argv[i]); return 1; }
        srcPath = argv[i];
    }
    if (!srcPath || memEntries<=0) { fprintf(stderr,"missing source or -m\n"); return 1; }
    char *userSrc = readFile(srcPath);
    if (!userSrc) return 1;

    char *stdlibSrc = readStdlibSources("stdlib");
    if (!stdlibSrc) return 1;

    size_t stdLen = strlen(stdlibSrc);
    size_t userLen = strlen(userSrc);
    char *src = malloc(stdLen + userLen + 1);
    memcpy(src, stdlibSrc, stdLen);
    memcpy(src + stdLen, userSrc, userLen);
    src[stdLen + userLen] = 0;
    free(stdlibSrc);
    free(userSrc);

    Parser p; parserInit(&p, src);
    Program *prog = parseProgram(&p);
    // semantic checks
    extern int semaCheck(Program *p);
    if (!semaCheck(prog)) { fprintf(stderr,"sema failed\n"); return 1; }
    if (!emitDirectElfProgram(outName, prog, memEntries)) return 1;
    printf("built %s (direct-elf)\n", outName);
    return 0;
}

