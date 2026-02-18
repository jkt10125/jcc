#include "elf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Simple ELF64 writer: text and data segments, non-PIE. Places text at 0x400000+0x1000.
int write_elf64(const char *path, const uint8_t *text, uint64_t text_size, const uint8_t *data, uint64_t data_size, uint64_t entry_offset) {
    const uint64_t base = 0x400000;
    const uint64_t text_vaddr = base + 0x1000;
    uint64_t text_offset = 0x1000; // file offset where segments start
    uint64_t phoff = sizeof(Elf64_Ehdr);
    int phnum = 2;

    // compute data placement: align after text
    uint64_t text_filesz = text_size;
    uint64_t align = 0x1000;
    uint64_t data_offset = (text_offset + ((text_filesz + align -1) & ~(align-1)));
    uint64_t data_vaddr = text_vaddr + (data_offset - text_offset);

    // open file
    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0755);
    if (fd < 0) return -1;

    Elf64_Ehdr eh;
    memset(&eh,0,sizeof(eh));
    memcpy(eh.e_ident, ELFMAG, SELFMAG);
    eh.e_ident[EI_CLASS] = ELFCLASS64;
    eh.e_ident[EI_DATA] = ELFDATA2LSB;
    eh.e_ident[EI_VERSION] = EV_CURRENT;
    eh.e_type = ET_EXEC;
    eh.e_machine = EM_X86_64;
    eh.e_version = EV_CURRENT;
    eh.e_entry = text_vaddr + entry_offset;
    eh.e_phoff = phoff;
    eh.e_ehsize = sizeof(Elf64_Ehdr);
    eh.e_phentsize = sizeof(Elf64_Phdr);
    eh.e_phnum = phnum;

    // program headers
    Elf64_Phdr ph_text;
    memset(&ph_text,0,sizeof(ph_text));
    ph_text.p_type = PT_LOAD;
    ph_text.p_offset = text_offset;
    ph_text.p_vaddr = text_vaddr;
    ph_text.p_paddr = text_vaddr;
    ph_text.p_filesz = text_filesz;
    ph_text.p_memsz = text_filesz;
    ph_text.p_flags = PF_R | PF_X;
    ph_text.p_align = align;

    Elf64_Phdr ph_data;
    memset(&ph_data,0,sizeof(ph_data));
    ph_data.p_type = PT_LOAD;
    ph_data.p_offset = data_offset;
    ph_data.p_vaddr = data_vaddr;
    ph_data.p_paddr = data_vaddr;
    ph_data.p_filesz = data_size;
    ph_data.p_memsz = data_size;
    ph_data.p_flags = PF_R | PF_W;
    ph_data.p_align = align;

    // write headers
    if (write(fd, &eh, sizeof(eh)) != sizeof(eh)) goto err;
    if (write(fd, &ph_text, sizeof(ph_text)) != sizeof(ph_text)) goto err;
    if (write(fd, &ph_data, sizeof(ph_data)) != sizeof(ph_data)) goto err;

    // pad until text_offset
    off_t cur = lseek(fd, 0, SEEK_CUR);
    if (cur < 0) goto err;
    if ((uint64_t)cur > text_offset) goto err;
    uint64_t pad = text_offset - (uint64_t)cur;
    if (pad) {
        uint8_t *zeros = calloc(1,pad);
        if (!zeros) goto err;
        if (write(fd, zeros, pad) != (ssize_t)pad) { free(zeros); goto err; }
        free(zeros);
    }
    // write text
    if (text_size) {
        if (write(fd, text, text_size) != (ssize_t)text_size) goto err;
    }
    // pad to data_offset
    cur = lseek(fd, 0, SEEK_CUR);
    if ((uint64_t)cur > data_offset) goto err;
    pad = data_offset - (uint64_t)cur;
    if (pad) {
        uint8_t *zeros = calloc(1,pad);
        if (!zeros) goto err;
        if (write(fd, zeros, pad) != (ssize_t)pad) { free(zeros); goto err; }
        free(zeros);
    }
    // write data
    if (data_size) {
        if (write(fd, data, data_size) != (ssize_t)data_size) goto err;
    }

    close(fd);
    return 0;
err:
    close(fd);
    return -1;
}

