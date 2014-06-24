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
} relevant_section_t;

char elfloader_unknown[30];	/* Name that caused link error. */

struct process * const * elfloader_autostart_processes = NULL;

static relevant_section_t bss, data, rodata, text;

static const unsigned char elf_magic_header[] =
  {0x7f, 0x45, 0x4c, 0x46,	/* 0x7f, 'E', 'L', 'F' */
   0x01,					/* Only 32-bit objects. */
   0x01,					/* Only LSB data. */
   0x01,					/* Only ELF version 1. */
  };


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
		  unsigned int symtab, unsigned short symtabsize,
		  unsigned int strtab)
{
	elf32_sym_t s;
	unsigned int a;
	void* retAddr = NULL;

	for(a = symtab; a < symtab + symtabsize; a += sizeof(s)) {
		seek_read(fd, a, (char *)&s, sizeof(s));
		if(s.name != 0) {
			const char* name = fd + strtab + s.name;
			if(strcmp(name, symbol) == 0) {
				relevant_section_t* sect = findSectionById(s.shndx);
				if (sect != NULL) {
					retAddr = &(sect->address[s.value]);
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
                 unsigned int section, unsigned short size,
                 unsigned int sectionaddr,
                 char *sectionbase,
                 unsigned int strtab,
                 unsigned int symtab, unsigned short symtabsize,
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
  
  for(a = section; a < section + size; a += rel_size) {
	seek_read(fd, a, (char *)&rela, rel_size);
	seek_read(fd,
		  symtab + sizeof(elf32_sym_t) * ELF32_R_SYM(rela.info),
		  (char *)&s, sizeof(s));
	if(s.name != 0) {
	  const char* name = fd + strtab + s.name;
	  addr = (char *)symtab_lookup(name);
	  /* ADDED */
	  if(addr == NULL) {
	PRINTF("name not found in global: %s\n", name);
	addr = find_local_symbol(fd, name, symtab, symtabsize, strtab);
	PRINTF("found address %p\n", addr);
	  }
	  if(addr == NULL) {
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
	  seek_read(fd, sectionaddr + rela.offset, (char *)&rela.addend, 4);
	}

	elfloader_arch_relocate(fd, sectionaddr, sectionbase, &rela, addr);
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
    const elf32_ehdr_t* ehdr;
    const elf32_shdr_t* strtable;
    unsigned int strs;

    unsigned short shdrnum, shdrsize;

    unsigned char using_relas = -1;

    /* Initialize the segment sizes to zero so that we can check if
       their sections was found in the file or not. */
    unsigned short textoff = 0, textsize = 0, textrelaoff = 0, textrelasize = 0;
    unsigned short dataoff = 0, datasize = 0, datarelaoff = 0, datarelasize = 0;
    unsigned short rodataoff = 0, rodatasize = 0, rodatarelaoff = 0, rodatarelasize = 0;
    unsigned short symtaboff = 0, symtabsize = 0;
    unsigned short strtaboff = 0, strtabsize = 0;
    unsigned short bsssize = 0;

    struct process **process;

    elfloader_unknown[0] = 0;

    /* The ELF header is located at the start of the buffer. */
    ehdr = (const elf32_ehdr_t*)fd;

    /* Make sure that we have a correct and compatible ELF header. */
    if (!check_if_correct_elfheader(ehdr->ident)) {
        PRINTF("ELF header problems\n");
        return ELFLOADER_BAD_ELF_HEADER;
    }


    /* Get the size and number of entries of the section header. */
    shdrsize = ehdr->shentsize;
    shdrnum = ehdr->shnum;

    PRINTF("Section header: size %d num %d\n", shdrsize, shdrnum);

    /* The string table section: holds the names of the sections. */
    strtable = fd + ehdr->shoff + shdrsize * ehdr->shstrndx;

    /* Get a pointer to the actual table of strings. This table holds
       the names of the sections, not the names of other symbols in the
       file (these are in the sybtam section). */
    strs = strtable->offset;

    PRINTF("Strtable offset %d\n", strs);

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
    const void* const shdrptr = fd + ehdr->shoff;

    for(int i=0; i<shdrnum; ++i) {
        elf32_shdr_t shdr;
        memcpy(&shdr, shdrptr + shdrsize*i, sizeof(shdr));

        /* The name of the section is contained in the strings table. */
        const void const* nameptr = fd + strs + shdr.name;

        PRINTF("Section shdrptr 0x%x, %d + %d type %d\n",
               shdrptr,
               strs, shdr.name,
               (int)shdr.type);
        /* Match the name of the section with a predefined set of names
           (.text, .data, .bss, .rela.text, .rela.data, .symtab, and
           .strtab). */
        /* added support for .rodata, .rel.text and .rel.data). */

        if(shdr.type == SHT_SYMTAB) {
            PRINTF("symtab\n");
            symtaboff = shdr.offset;
            symtabsize = shdr.size;
        } else if(shdr.type == SHT_STRTAB) {
            PRINTF("strtab\n");
            strtaboff = shdr.offset;
            strtabsize = shdr.size;
        } else if(strcmp(nameptr, ".text") == 0) {
            textoff = shdr.offset;
            textsize = shdr.size;
            text.number = i;
            text.offset = textoff;
        } else if(strcmp(nameptr, ".rel.text") == 0) {
            using_relas = 0;
            textrelaoff = shdr.offset;
            textrelasize = shdr.size;
        } else if(strcmp(nameptr, ".rela.text") == 0) {
            using_relas = 1;
            textrelaoff = shdr.offset;
            textrelasize = shdr.size;
        } else if(strcmp(nameptr, ".data") == 0) {
            dataoff = shdr.offset;
            datasize = shdr.size;
            data.number = i;
            data.offset = dataoff;
        } else if(strcmp(nameptr, ".rodata") == 0) {
            /* read-only data handled the same way as regular text section */
            rodataoff = shdr.offset;
            rodatasize = shdr.size;
            rodata.number = i;
            rodata.offset = rodataoff;
        } else if(strcmp(nameptr, ".rel.rodata") == 0) {
            /* using elf32_rel instead of rela */
            using_relas = 0;
            rodatarelaoff = shdr.offset;
            rodatarelasize = shdr.size;
        } else if(strcmp(nameptr, ".rela.rodata") == 0) {
            using_relas = 1;
            rodatarelaoff = shdr.offset;
            rodatarelasize = shdr.size;
        } else if(strcmp(nameptr, ".rel.data") == 0) {
            /* using elf32_rel instead of rela */
            using_relas = 0;
            datarelaoff = shdr.offset;
            datarelasize = shdr.size;
        } else if(strcmp(nameptr, ".rela.data") == 0) {
            using_relas = 1;
            datarelaoff = shdr.offset;
            datarelasize = shdr.size;
        } else if(strcmp(nameptr, ".bss") == 0) {
            bsssize = shdr.size;
            bss.number = i;
            bss.offset = 0;
        }
    }

    /* Error checking, symbol table, string table and text (code) should be available */
    if(symtabsize == 0) {
        return ELFLOADER_NO_SYMTAB;
    }
    if(strtabsize == 0) {
        return ELFLOADER_NO_STRTAB;
    }
    if(textsize == 0) {
        return ELFLOADER_NO_TEXT;
    }

    PRINTF("before allocate ram\n");
	bss.address = (char*)malloc(bsssize + datasize);
    data.address = bss.address + bsssize;
    PRINTF("before allocate rom\n");
	text.address = fd + textoff;
	rodata.address = fd + rodataoff;

    PRINTF("bss base address: bss.address = 0x%08x\n", bss.address);
    PRINTF("data base address: data.address = 0x%08x\n", data.address);
    PRINTF("text base address: text.address = 0x%08x\n", text.address);
    PRINTF("rodata base address: rodata.address = 0x%08x\n", rodata.address);


    /* If we have text segment relocations, we process them. */
    PRINTF("elfloader: relocate text\n");
    if(textrelasize > 0) {
        int ret = relocate_section(fd,
                               textrelaoff, textrelasize,
                               textoff,
                               text.address,
                               strtaboff,
                               symtaboff, symtabsize, using_relas);
        if(ret != ELFLOADER_OK) {
            return ret;
        }
    }

    /* If we have any rodata segment relocations, we process them too. */
    PRINTF("elfloader: relocate rodata\n");
    if(rodatarelasize > 0) {
        int ret = relocate_section(fd,
                               rodatarelaoff, rodatarelasize,
                               rodataoff,
                               rodata.address,
                               strtaboff,
                               symtaboff, symtabsize, using_relas);
        if(ret != ELFLOADER_OK) {
            PRINTF("elfloader: data failed\n");
            return ret;
        }
    }

    /* If we have any data segment relocations, we process them too. */
    PRINTF("elfloader: relocate data\n");
    if(datarelasize > 0) {
        int ret = relocate_section(fd,
                               datarelaoff, datarelasize,
                               dataoff,
                               data.address,
                               strtaboff,
                               symtaboff, symtabsize, using_relas);
        if(ret != ELFLOADER_OK) {
            PRINTF("elfloader: data failed\n");
            return ret;
        }
    }

    memset(bss.address, 0, bsssize);
    memcpy(fd+dataoff, data.address, datasize);

    PRINTF("elfloader: autostart search\n");
    process = (struct process **) find_local_symbol(fd, entry_point_name, symtaboff, symtabsize, strtaboff);
    if(process != NULL) {
        PRINTF("elfloader: autostart found\n");
        elfloader_autostart_processes = process;
        return ELFLOADER_OK;
    }
}
/*---------------------------------------------------------------------------*/
