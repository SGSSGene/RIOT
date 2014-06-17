export BOARD ?= msb-430
EXAMPLE = hello-world
export APPLICATION = test_loader_hello_world


all:
	cp ../../examples/$(EXAMPLE)/main.c dyn_main.c # copy main.c file from example 
	sed -i "s/int.*main(void)/int dyn_main(void)/g" dyn_main.c # replace main() by dyn_main

	cp ../../examples/$(EXAMPLE)/Makefile Makefile.copy # copy Makefile from example
	sed -i "s/export APPLICATION = ${EXAMPLE}/export APPLICATION = ${APPLICATION}/g" Makefile.copy 

	make -f Makefile.copy
	msp430-strip -g bin/$(BOARD)/$(APPLICATION)/dyn_main.o # strip debugging sections
	@hexdump -v -e '1/1 "0x%x" ", "' bin/$(BOARD)/$(APPLICATION)/dyn_main.o \
    | sed 's/\(.*\)..$$/unsigned char dyn_app\[\] = \{\1\}\;\n/' > bin/$(BOARD)/dyn_main.h


clean:
	cp ../../examples/$(EXAMPLE)/Makefile Makefile.copy
	make clean -f Makefile.copy
	rm -f dyn_main.c
	rm -f Makefile.copy

