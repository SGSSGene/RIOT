all:
	rm -f app.o app.hex
	arm-none-eabi-gcc  --specs=nosys.specs -mthumb app.c -o app.o
	#cp app.o app.hex
	arm-none-eabi-objcopy -I elf32-little -O binary app.o app.hex
