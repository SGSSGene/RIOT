# name of your application
export APPLICATION = test_loader

# This has to be the absolute path to the RIOT base directory:
export RIOTBASE ?= $(CURDIR)/../..

# Change this to 0 show compiler invocation lines by default:
export QUIET ?= 1

# If no BOARD is found in the environment, use this default:
export BOARD ?= msb-430

RIOTBASE  = $(CURDIR)/../..

MAKE_EMPTY_SYMBOLS ?= $(RIOTBASE)/dist/tools/loader/make-empty-symbols
MAKESYMBOLS ?= $(RIOTBASE)/dist/tools/loader/msp430-make-symbols


all:
	make -C dyn_app
#	$(MAKE_EMPTY_SYMBOLS)
#	make clean all -f Makefile.real
#	$(MAKESYMBOLS) bin/$(BOARD)/$(APPLICATION).elf
	make clean all -f Makefile.real

clean:
	make clean -C dyn_app
	make clean -f Makefile.real
