APPLICATION = test_loader

RIOTBASE  = $(CURDIR)/../..

MAKE_EMPTY_SYMBOLS ?= $(RIOTBASE)/dist/tools/loader/make-empty-symbols
MAKESYMBOLS ?= $(RIOTBASE)/dist/tools/loader/msp430-make-symbols

all:
	$(MAKE_EMPTY_SYMBOLS)
	make clean all -f Makefile.real
	$(MAKESYMBOLS) bin/$(BOARD)/$(APPLICATION).elf
	make -f Makefile.real
	$(MAKESYMBOLS) bin/$(BOARD)/$(APPLICATION).elf
	make -f Makefile.real

clean:
	make clean -f Makefile.real
	rm -f symbols.c
