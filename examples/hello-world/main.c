/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include "../dyn_app/hex_app.h"

typedef unsigned long  elf32_word;
typedef   signed long  elf32_sword;
typedef unsigned short elf32_half;
typedef unsigned long  elf32_off;
typedef unsigned long  elf32_addr;

#define EI_NIDENT 16

struct elf32_ehdr {
  unsigned char e_ident[EI_NIDENT];    /* ident bytes */
  elf32_half e_type;                   /* file type */
  elf32_half e_machine;                /* target machine */
  elf32_word e_version;                /* file version */
  elf32_addr e_entry;                  /* start address */
  elf32_off e_phoff;                   /* phdr file offset */
  elf32_off e_shoff;                   /* shdr file offset */
  elf32_word e_flags;                  /* file flags */
  elf32_half e_ehsize;                 /* sizeof ehdr */
  elf32_half e_phentsize;              /* sizeof phdr */
  elf32_half e_phnum;                  /* number phdrs */
  elf32_half e_shentsize;              /* sizeof shdr */
  elf32_half e_shnum;                  /* number shdrs */
  elf32_half e_shstrndx;               /* shdr string index */
};

struct elf32_shdr {
  elf32_word sh_name; 		/* section name */
  elf32_word sh_type; 		/* SHT_... */
  elf32_word sh_flags; 	        /* SHF_... */
  elf32_addr sh_addr; 		/* virtual address */
  elf32_off sh_offset; 	        /* file offset */
  elf32_word sh_size; 		/* section size */
  elf32_word sh_link; 		/* misc info */
  elf32_word sh_info; 		/* misc info */
  elf32_word sh_addralign; 	/* memory alignment */
  elf32_word sh_entsize; 	/* entry size if table */
};

/* sh_type */
#define SHT_NULL        0               /* inactive */
#define SHT_PROGBITS    1               /* program defined information */
#define SHT_SYMTAB      2               /* symbol table section */
#define SHT_STRTAB      3               /* string table section */
#define SHT_RELA        4               /* relocation section with addends*/
#define SHT_HASH        5               /* symbol hash table section */
#define SHT_DYNAMIC     6               /* dynamic section */
#define SHT_NOTE        7               /* note section */
#define SHT_NOBITS      8               /* no space section */
#define SHT_REL         9               /* relation section without addends */
#define SHT_SHLIB       10              /* reserved - purpose unknown */
#define SHT_DYNSYM      11              /* dynamic symbol table section */
#define SHT_LOPROC      0x70000000      /* reserved range for processor */
#define SHT_HIPROC      0x7fffffff      /* specific section header types */
#define SHT_LOUSER      0x80000000      /* reserved range for application */
#define SHT_HIUSER      0xffffffff      /* specific indexes */

struct relevant_section {
  unsigned char number;
  unsigned int offset;
  char *address;
};

static struct relevant_section bss, data, rodata, text;

char * seek_read(void * loc, size_t offset, char * destination, size_t size) {
	memcpy ( (void *)(destination), (void *)(loc + offset), size );
	return destination;
}

void parse_hex_app(void) {
	void * fd =  (void *) hex_app;

	int i;
	unsigned int strs;

	struct elf32_ehdr ehdr;
	struct elf32_shdr shdr;
	unsigned int shdrptr;
	unsigned short shdrnum, shdrsize;
	unsigned int nameptr;
	char name[12];
	struct elf32_shdr strtable;

  unsigned char using_relas = -1;
  unsigned short textoff = 0, textsize, textrelaoff = 0, textrelasize;
  unsigned short dataoff = 0, datasize, datarelaoff = 0, datarelasize;
  unsigned short rodataoff = 0, rodatasize, rodatarelaoff = 0, rodatarelasize;
  unsigned short symtaboff = 0, symtabsize;
  unsigned short strtaboff = 0, strtabsize;
  unsigned short bsssize = 0;

	printf("=== ELF HEADER INFO ===\n");

	// read the elf header
	seek_read(fd, 0, (char *)&ehdr, sizeof(ehdr));

	// print the magic number
	printf("\tMagic:    ");
	for (i = 0; i < EI_NIDENT; i++) {
		printf("0x%0.x ",ehdr.e_ident[i]);
	} printf("\n");

	// get the offset of the section table
	shdrptr = ehdr.e_shoff;

	// read the header of the section table
	seek_read(fd, shdrptr, (char *)&shdr, sizeof(shdr));

	shdrsize = ehdr.e_shentsize;
	shdrnum = ehdr.e_shnum;

	printf("\tSection header: size %d num %d\n", shdrsize, shdrnum);

	/* The string table section: holds the names of the sections. */
	seek_read(fd, ehdr.e_shoff + shdrsize * ehdr.e_shstrndx,
			  (char *)&strtable, sizeof(strtable));

  /* Get a pointer to the actual table of strings. This table holds
     the names of the sections, not the names of other symbols in the
     file (these are in the sybtam section). */
  strs = strtable.sh_offset;

  /* Initialize the segment sizes to zero so that we can check if
     their sections was found in the file or not. */
  textsize = textrelasize = datasize = datarelasize =
    rodatasize = rodatarelasize = symtabsize = strtabsize = 0;

  bss.number = data.number = rodata.number = text.number = -1;

  shdrptr = ehdr.e_shoff;

  printf("Strtable offset %d\n", strs);

  for(i = 0; i < shdrnum; ++i) {

	seek_read(fd, shdrptr, (char *)&shdr, sizeof(shdr));
	
	/* The name of the section is contained in the strings table. */
	nameptr = strs + shdr.sh_name;
	seek_read(fd, nameptr, name, sizeof(name));
	printf("Section shdrptr 0x%x, %d + %d type %d\n",
	   shdrptr,
	   strs, shdr.sh_name,
	   (int)shdr.sh_type);
	/* Match the name of the section with a predefined set of names
	   (.text, .data, .bss, .rela.text, .rela.data, .symtab, and
	   .strtab). */
	/* added support for .rodata, .rel.text and .rel.data). */

	if(shdr.sh_type == SHT_SYMTAB/*strncmp(name, ".symtab", 7) == 0*/) {
	  printf("symtab\n");
	  symtaboff = shdr.sh_offset;
	  symtabsize = shdr.sh_size;
	} else if(shdr.sh_type == SHT_STRTAB/*strncmp(name, ".strtab", 7) == 0*/) {
	  printf("strtab\n");
	  strtaboff = shdr.sh_offset;
	  strtabsize = shdr.sh_size;
	} else if(strncmp(name, ".text", 5) == 0) {
	  textoff = shdr.sh_offset;
	  textsize = shdr.sh_size;
	  text.number = i;
	  text.offset = textoff;
	} else if(strncmp(name, ".rel.text", 9) == 0) {
	  using_relas = 0;
	  textrelaoff = shdr.sh_offset;
	  textrelasize = shdr.sh_size;
	} else if(strncmp(name, ".rela.text", 10) == 0) {
	  using_relas = 1;
	  textrelaoff = shdr.sh_offset;
	  textrelasize = shdr.sh_size;
	} else if(strncmp(name, ".data", 5) == 0) {
	  dataoff = shdr.sh_offset;
	  datasize = shdr.sh_size;
	  data.number = i;
	  data.offset = dataoff;
	} else if(strncmp(name, ".rodata", 7) == 0) {
	  /* read-only data handled the same way as regular text section */
	  rodataoff = shdr.sh_offset;
	  rodatasize = shdr.sh_size;
	  rodata.number = i;
	  rodata.offset = rodataoff;
	} else if(strncmp(name, ".rel.rodata", 11) == 0) {
	  /* using elf32_rel instead of rela */
	  using_relas = 0;
	  rodatarelaoff = shdr.sh_offset;
	  rodatarelasize = shdr.sh_size;
	} else if(strncmp(name, ".rela.rodata", 12) == 0) {
	  using_relas = 1;
	  rodatarelaoff = shdr.sh_offset;
	  rodatarelasize = shdr.sh_size;
	} else if(strncmp(name, ".rel.data", 9) == 0) {
	  /* using elf32_rel instead of rela */
	  using_relas = 0;
	  datarelaoff = shdr.sh_offset;
	  datarelasize = shdr.sh_size;
	} else if(strncmp(name, ".rela.data", 10) == 0) {
	  using_relas = 1;
	  datarelaoff = shdr.sh_offset;
	  datarelasize = shdr.sh_size;
	} else if(strncmp(name, ".bss", 4) == 0) {
	  bsssize = shdr.sh_size;
	  bss.number = i;
	  bss.offset = 0;
	}

	/* Move on to the next section header. */
	shdrptr += shdrsize;
  }


}

int main(void)
{
	parse_hex_app();
    puts("Done Parsing!");
    return 0;
}
