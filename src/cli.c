#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

extern void genProgram(const char *outPath, Program *prog);

static char *readFile(const char *path) {
    FILE *f = fopen(path,"rb");
    if (!f) { perror("fopen"); return NULL; }
    fseek(f,0,SEEK_END); long sz = ftell(f); fseek(f,0,SEEK_SET);
    char *buf = malloc(sz+1); fread(buf,1,sz,f); buf[sz]=0; fclose(f); return buf;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr,"usage: jcc -m <memEntries> [ -o <out> ] <source>\n"); return 1;
    }
    int memEntries = 0;
    char *outName = "a.out";
    char *srcPath = NULL;
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i],"-m")==0 && i+1<argc) { memEntries = atoi(argv[++i]); }
        else if (strcmp(argv[i],"-o")==0 && i+1<argc) { outName = argv[++i]; }
        else if (argv[i][0]=='-') { fprintf(stderr,"unknown option %s\n", argv[i]); return 1; }
        else srcPath = argv[i];
    }
    if (!srcPath || memEntries<=0) { fprintf(stderr,"missing source or -m\n"); return 1; }
    char *src = readFile(srcPath);
    if (!src) return 1;
    Parser p; parserInit(&p, src);
    Program *prog = parseProgram(&p);
    // semantic checks
    extern int semaCheck(Program *p);
    if (!semaCheck(prog)) { fprintf(stderr,"sema failed\n"); return 1; }
    // emit assembly
    const char *asmPath = "prog.s";
    genProgram(asmPath, prog);
    // compile asm to object
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc -c -o prog.o %s", asmPath);
    if (system(cmd)!=0) { fprintf(stderr,"assemble failed\n"); return 1; }
    // compile runtime
    snprintf(cmd, sizeof(cmd), "gcc -c -o rt.o runtime/rt.c -DMEM_ENTRIES=%d", memEntries);
    if (system(cmd)!=0) { fprintf(stderr,"compile runtime failed\n"); return 1; }
    // link
    snprintf(cmd, sizeof(cmd), "gcc -no-pie -o %s prog.o rt.o", outName);
    if (system(cmd)!=0) { fprintf(stderr,"link failed\n"); return 1; }
    printf("built %s\n", outName);
    return 0;
}

