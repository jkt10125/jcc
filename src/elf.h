#ifndef ELF_H
#define ELF_H

#include <stdint.h>

int write_elf64(const char *path, const uint8_t *text, uint64_t text_size, const uint8_t *data, uint64_t data_size, uint64_t entry_offset);

#endif

