/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.	IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */


#include "elfloader.h"
#include "elfloader-arch.h"

#include "symtab.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#define EI_NIDENT 16


typedef struct elf32_ehdr {
  unsigned char ident[EI_NIDENT];	   /* ident bytes */
  elf32_half type;				   /* file type */
  elf32_half machine;				   /* target machine */
  elf32_word version;				   /* file version */
  elf32_addr entry;				   /* start address */
  elf32_off  phoff;				   /* phdr file offset */
  elf32_off  shoff;				   /* shdr file offset */
  elf32_word flags;				   /* file flags */
  elf32_half ehsize;				   /* sizeof ehdr */
  elf32_half phentsize;			   /* sizeof phdr */
  elf32_half phnum;				   /* number phdrs */
  elf32_half shentsize;			   /* sizeof shdr */
  elf32_half shnum;				   /* number shdrs */
  elf32_half shstrndx;			   /* shdr string index */
} elf32_ehdr_t;


typedef struct elf32_shdr {
  elf32_word name;		/* section name */
  elf32_word type;		/* SHT_... */
  elf32_word flags;			/* SHF_... */
  elf32_addr addr;		/* virtual address */
  elf32_off  offset;			/* file offset */
  elf32_word size;		/* section size */
  elf32_word link;		/* misc info */
  elf32_word info;		/* misc info */
  elf32_word addralign;	/* memory alignment */
  elf32_word entsize;	/* entry size if table */
} elf32_shdr_t;

/* sh_type */
#define SHT_NULL		0				/* inactive */
#define SHT_PROGBITS	1				/* program defined information */
#define SHT_SYMTAB		2				/* symbol table section */
#define SHT_STRTAB		3				/* string table section */
#define SHT_RELA		4				/* relocation section with addends*/
#define SHT_HASH		5				/* symbol hash table section */
#define SHT_DYNAMIC		6				/* dynamic section */
#define SHT_NOTE		7				/* note section */
#define SHT_NOBITS		8				/* no space section */
#define SHT_REL			9				/* relation section without addends */
#define SHT_SHLIB		10				/* reserved - purpose unknown */
#define SHT_DYNSYM		11				/* dynamic symbol table section */
#define SHT_LOPROC		0x70000000		/* reserved range for processor */
#define SHT_HIPROC		0x7fffffff		/* specific section header types */
#define SHT_LOUSER		0x80000000		/* reserved range for application */
#define SHT_HIUSER		0xffffffff		/* specific indexes */

typedef struct elf32_rel {
  elf32_addr	  offset;		  /* Location to be relocated. */
  elf32_word	  info;		  /* Relocation type and symbol index. */
} elf32_rel_t;

typedef struct elf32_sym {
  elf32_word	  name;		  /* String table index of name. */
  elf32_addr	  value;		  /* Symbol value. */
  elf32_word	  size;		  /* Size of associated object. */
  unsigned char	  info;		  /* Type and binding information. */
  unsigned char	  other;		  /* Reserved (not used). */
  elf32_half	  shndx;		  /* Section index of symbol. */
} elf32_sym_t;

#define ELF32_R_SYM(info)       ((info) >> 8)

typedef struct relevant_section {
  unsigned char number;
  unsigned int offset;
  char *address;
  unsigned short size;
} relevant_section_t;

char elfloader_unknown[30];	/* Name that caused link error. */
struct process * const * elfloader_autostart_process;

static relevant_section_t bss, data, rodata, text;

/*---------------------------------------------------------------------------*/
static char * seek_read(void * loc, size_t offset, char * destination, size_t size) {
	memcpy ( (void *)(destination), (void *)(loc + offset), size );
	return destination;
}

/*---------------------------------------------------------------------------*/
relevant_section_t* findSectionById(elf32_half id) {
	relevant_section_t* retSect = NULL;
	if(id == bss.number) {
		retSect = &bss;
	} else if(id == data.number) {
		retSect = &data;
	} else if(id == rodata.number) {
		retSect = &rodata;
	} else if(id == text.number) {
		retSect = &text;
	}
	return retSect;
}
/*---------------------------------------------------------------------------*/
static void *
find_local_symbol(void * fd, const char *symbol,
                  const elf32_shdr_t* symtab, const elf32_shdr_t* strtab)

{
	void* retAddr = NULL;

	for(const elf32_sym_t* s = fd+symtab->offset; s < fd+symtab->offset + symtab->size; ++s) {
		if(s->name != 0) {
			const char* name = fd + strtab->offset + s->name;
			if(strcmp(name, symbol) == 0) {
				relevant_section_t* sect = findSectionById(s->shndx);
				if (sect != NULL) {
					retAddr = &(sect->address[s->value]);
				}
				break;
			}
		}
	}
	return retAddr;
}
/*---------------------------------------------------------------------------*/
static int
relocate_section(void * fd,
				 const elf32_shdr_t* section,
				 const relevant_section_t* sectionAddr,
				 const elf32_shdr_t* strtab,
				 const elf32_shdr_t* symtab,
				 unsigned char using_relas)
{
  /* sectionbase added; runtime start address of current section */
  elf32_rela_t rela; /* Now used both for rel and rela data! */
  int rel_size = 0;
  elf32_sym_t s;
  unsigned int a;
  char* addr;

  /* determine correct relocation entry sizes */
  if(using_relas) {
	rel_size = sizeof(elf32_rela_t);
  } else {
	rel_size = sizeof(elf32_rel_t);
  }
  
  for(a = section->offset; a < section->offset + section->size; a += rel_size) {
	seek_read(fd, a, (char *)&rela, rel_size);
	seek_read(fd,
		  symtab->offset + sizeof(elf32_sym_t) * ELF32_R_SYM(rela.info),
		  (char *)&s, sizeof(s));
	if(s.name != 0) {
	  const char* name = fd + strtab->offset + s.name;
	  addr = (char *)symtab_lookup(name);
	  /* ADDED */
	  if(addr == NULL) {
	PRINTF("name not found in global: %s\n", name);
	addr = find_local_symbol(fd, name, symtab, strtab);
	PRINTF("found address %p\n", addr);
	  }
	  if(addr == NULL) {
	  	// This doesn't look save
		relevant_section_t* sect = findSectionById(s.shndx);
		if (sect == NULL) {
		  PRINTF("elfloader unknown name: '%30s'\n", name);
		  memcpy(elfloader_unknown, name, sizeof(elfloader_unknown));
		  elfloader_unknown[sizeof(elfloader_unknown) - 1] = 0;
		  return ELFLOADER_SYMBOL_NOT_FOUND;
		}

	addr = sect->address;
	  }
	} else {
		relevant_section_t* sect = findSectionById(s.shndx);
		if (sect == NULL) {
			return ELFLOADER_SEGMENT_NOT_FOUND;
		}
		addr = sect->address;
	}

	if(!using_relas) {
	  /* copy addend to rela structure */
	  seek_read(fd, sectionAddr->offset + rela.offset, (char *)&rela.addend, 4);
	}

	elfloader_arch_relocate(fd, sectionAddr->offset, sectionAddr->address, &rela, addr);
  }
  return ELFLOADER_OK;
}

/*---------------------------------------------------------------------------*/

int check_if_correct_elfheader(void const* ptr) {
    static const unsigned char elf_magic_header[] =
        {0x7f, 0x45, 0x4c, 0x46,  /* 0x7f, 'E', 'L', 'F' */
         0x01,                    /* Only 32-bit objects. */
         0x01,                    /* Only LSB data. */
         0x01,                    /* Only ELF version 1. */
        };

    return memcmp(ptr, elf_magic_header, sizeof(elf_magic_header)) == 0;
}


/*---------------------------------------------------------------------------*/
int
elfloader_load(void * fd, const char * entry_point_name)
{
    const elf32_shdr_t* strtable;

    unsigned char using_relas = -1;

    /* Initialize the segment sizes to zero so that we can check if
       their sections was found in the file or not. */
	const elf32_shdr_t* textrela   = NULL;
	const elf32_shdr_t* datarela   = NULL;
	const elf32_shdr_t* rodatarela = NULL;
	const elf32_shdr_t* symtab     = NULL;
	const elf32_shdr_t* strtab     = NULL;

    /* The ELF header is located at the start of the buffer. */
    const elf32_ehdr_t* ehdr = (const elf32_ehdr_t*)fd;

    elfloader_unknown[0] = 0;

    /* Make sure that we have a correct and compatible ELF header. */
    if (!check_if_correct_elfheader(ehdr->ident)) {
        PRINTF("ELF header problems\n");
        return ELFLOADER_BAD_ELF_HEADER;
    }


    PRINTF("Section header: size %d num %d\n", ehdr->shentsize, ehdr->shnum);

    /* The string table section: holds the names of the sections. */
    strtable = fd + ehdr->shoff + ehdr->shentsize * ehdr->shstrndx;

    PRINTF("Strtable offset %d\n", strtable->offset);

    /* Go through all sections and pick out the relevant ones. The
       ".text" segment holds the actual code from the ELF file, the
       ".data" segment contains initialized data, the ".bss" segment
       holds the size of the unitialized data segment. The ".rel[a].text"
       and ".rel[a].data" segments contains relocation information for the
       contents of the ".text" and ".data" segments, respectively. The
       ".symtab" segment contains the symbol table for this file. The
       ".strtab" segment points to the actual string names used by the
       symbol table.

       In addition to grabbing pointers to the relevant sections, we
       also save the section number for resolving addresses in the
       relocator code.
    */

    bss.number = data.number = rodata.number = text.number = -1;

    /* Grab the section header. */
    const void* shdrptr = fd + ehdr->shoff;

    for(int i=0; i<ehdr->shnum; ++i) {
		const elf32_shdr_t* shdr = shdrptr + ehdr->shentsize*i;

        /* The name of the section is contained in the strings table. */
        const void const* nameptr = fd + strtable->offset + shdr->name;

        PRINTF("Section shdrptr 0x%x, %d + %d type %d\n",
               shdrptr,
               strtable->offset, shdr->name,
               (int)shdr->type);
        /* Match the name of the section with a predefined set of names
           (.text, .data, .bss, .rela.text, .rela.data, .symtab, and
           .strtab). */
        /* added support for .rodata, .rel.text and .rel.data). */

		if (strncmp(nameptr, ".rela", 5) == 0) {
			using_relas = 1;
		} else if (strncmp(nameptr, ".rel", 4) == 0) {
			using_relas = 0;
		}

        if(shdr->type == SHT_SYMTAB) {
			symtab = shdr;
        } else if(shdr->type == SHT_STRTAB) {
			strtab = shdr;
        } else if(strcmp(nameptr, ".text") == 0) {
            text.number = i;
            text.offset = shdr->offset;
			text.size   = shdr->size;
        } else if(strcmp(nameptr, ".rel.text") == 0) {
			textrela = shdr;
        } else if(strcmp(nameptr, ".rela.text") == 0) {
			textrela = shdr;
        } else if(strcmp(nameptr, ".data") == 0) {
            data.number = i;
            data.offset = shdr->offset;
			data.size   = shdr->size;
        } else if(strcmp(nameptr, ".rodata") == 0) {
            /* read-only data handled the same way as regular text section */
            rodata.number = i;
            rodata.offset = shdr->offset;
			rodata.size   = shdr->size;
        } else if(strcmp(nameptr, ".rel.rodata") == 0) {
            /* using elf32_rel instead of rela */
			rodatarela = shdr;
        } else if(strcmp(nameptr, ".rela.rodata") == 0) {
			rodatarela = shdr;
        } else if(strcmp(nameptr, ".rel.data") == 0) {
            /* using elf32_rel instead of rela */
			datarela = shdr;
        } else if(strcmp(nameptr, ".rela.data") == 0) {
			datarela = shdr;
        } else if(strcmp(nameptr, ".bss") == 0) {
            bss.number = i;
            bss.offset = 0;
			bss.size   = shdr->size;
        }
    }

    /* Error checking, symbol table, string table and text (code) should be available */
    if(symtab == NULL || symtab->size == 0) {
        return ELFLOADER_NO_SYMTAB;
    }
    if(strtab == NULL || strtab->size == 0) {
        return ELFLOADER_NO_STRTAB;
    }
    if(text.size == 0) {
        return ELFLOADER_NO_TEXT;
    }

    PRINTF("before allocate ram\n");
	bss.address = (char*)malloc(bss.size + data.size);
    data.address = bss.address + bss.size;
    PRINTF("before allocate rom\n");
	text.address = fd + text.offset;
	rodata.address = fd + rodata.offset;

    PRINTF("bss base address: bss.address = 0x%08x\n", bss.address);
    PRINTF("data base address: data.address = 0x%08x\n", data.address);
    PRINTF("text base address: text.address = 0x%08x\n", text.address);
    PRINTF("rodata base address: rodata.address = 0x%08x\n", rodata.address);


    /* If we have text segment relocations, we process them. */
    PRINTF("elfloader: relocate text\n");
    if(textrela != NULL) {
        int ret = relocate_section(fd, textrela, &text, strtab, symtab, using_relas);
        if(ret != ELFLOADER_OK) {
            return ret;
        }
    }

    /* If we have any rodata segment relocations, we process them too. */
    PRINTF("elfloader: relocate rodata\n");
    if(rodatarela != NULL) {
        int ret = relocate_section(fd, rodatarela, &rodata, strtab, symtab, using_relas);
        if(ret != ELFLOADER_OK) {
            PRINTF("elfloader: data failed\n");
            return ret;
        }
    }

    /* If we have any data segment relocations, we process them too. */
    PRINTF("elfloader: relocate data\n");
    if(datarela != NULL) {
        int ret = relocate_section(fd, datarela, &data, strtab, symtab, using_relas);
        if(ret != ELFLOADER_OK) {
            PRINTF("elfloader: data failed\n");
            return ret;
        }
    }

    memset(bss.address, 0, bss.size);
    memcpy(fd+data.offset, data.address, data.size);

    PRINTF("elfloader: autostart search\n");
    elfloader_autostart_process = (struct process **) find_local_symbol(fd, entry_point_name, symtab, strtab);
    if(elfloader_autostart_process == NULL) {
		return ELFLOADER_NO_STARTPOINT;
    }
    return ELFLOADER_OK;
}
/*---------------------------------------------------------------------------*/