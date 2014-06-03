all:
	rm -f app.o app.hex
	arm-none-eabi-gcc  -c --specs=nosys.specs -mthumb app.c -o app.o
	hexdump -v -e '1/1 "0x%x" ", "' app.o | sed 's/\(.*\)..$$/unsigned char hex_app\[\] = \{\1\}\;\n/' > hex_app.h
