#ifndef RUNTIME_BYTES_H
#define RUNTIME_BYTES_H

#include "codegen_bytes.h"

typedef struct {
    size_t startOffset;
    size_t putIntOffset;
    size_t getIntOffset;
    size_t putCharOffset;
    size_t readCharOffset;
    size_t exitOffset;
    size_t strBufPtrOffset;
} RuntimeOffsets;

void emitRuntime(ByteBuf *text, PatchList *patches, RuntimeOffsets *outOffsets);

#endif

