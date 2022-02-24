#ifndef ELF_H
#define ELF_H

#include "types.h"

typedef struct elf_flag
{
    uint8 E : 1;
    uint8 W : 1;
    uint8 R : 1;
    uint32 : 29;
} elf_flag;

typedef struct elf_header
{
    uint32 magic;
    uint8 elf[12];
    uint16 type;
    uint16 machine;
    uint32 version;
    uint64 entry;
    uint64 phoff;
    uint64 shoff;
    uint32 flags;
    uint16 ehsize;
    uint16 phentsize;
    uint16 phnum;
    uint16 shentsize;
    uint16 shnum;
    uint16 shstrndx;
} elf_header;

typedef struct program_header
{
    uint32 type;
    elf_flag flags;
    uint64 off;
    uint64 vaddr;
    uint64 paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
} program_header;

#endif