/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
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

#include <stdint.h>
#include <string.h>
#include "loader/elfloader-arch.h"


#define ELFLOADER_DATAMEMORY_SIZE 0x100

static uint16_t datamemory_aligned[ELFLOADER_DATAMEMORY_SIZE/2+1];
static uint8_t *datamemory = (uint8_t *)datamemory_aligned;
#if ELFLOADER_CONF_TEXT_IN_ROM
static const char textmemory[ELFLOADER_TEXTMEMORY_SIZE] = {0};
#else /* ELFLOADER_CONF_TEXT_IN_ROM */
static char textmemory[ELFLOADER_TEXTMEMORY_SIZE];
#endif /* ELFLOADER_CONF_TEXT_IN_ROM */
/*---------------------------------------------------------------------------*/
void *
elfloader_arch_allocate_ram(int size)
{
  return datamemory;
}
/*---------------------------------------------------------------------------*/
void *
elfloader_arch_allocate_rom(int size)
{
  return textmemory;
}
/*---------------------------------------------------------------------------*/
void
elfloader_arch_write_rom(void * fd, unsigned short textoff, unsigned int size, char *mem)
{
  memcpy(mem, fd + textoff, size);
}
/*---------------------------------------------------------------------------*/
void
elfloader_arch_relocate(void * fd, unsigned int sectionoffset,
			char *sectionaddr,
			struct elf32_rela *rela, char *addr)
{
  addr += rela->r_addend;

  memcpy(fd + sectionoffset + rela->r_offset, addr, 2);
}
/*---------------------------------------------------------------------------*/
