#include "../src/lexer.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc<2) { printf("usage: lexdump file\n"); return 1; }
    FILE *f = fopen(argv[1],"rb"); if (!f) { perror("fopen"); return 1; }
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    char *buf = malloc(sz+1); fread(buf,1,sz,f); buf[sz]=0; fclose(f);
    Lexer lx; lexerInit(&lx, buf);
    Token t;
    do {
        t = lexerNext(&lx);
        printf("tok %d text='%s' num=%lld\n", t.kind, t.text?t.text:"", (long long)t.num);
    } while (t.kind!=TOK_EOF);
    return 0;
}
