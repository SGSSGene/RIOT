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

void loop_delay(void)
{
    int i, j;

    for (i = 1; i < 300; i++) {
        for (j = 1; j != 0; j++) {
            asm volatile(" nop ");
        }
    }
}

typedef int (*func_ptr_t)(void);



func_ptr_t riot_open(void* ptr) {
	return (func_ptr_t) ptr;
}

unsigned char a;

//const unsigned char foo[] = {0x0, 0x48, 0x2d, 0xe9, 0x4, 0xb0, 0x8d, 0xe2, 0xfe, 0xff, 0xff, 0xeb, 0x0, 0x30, 0xa0, 0xe1, 0x3, 0x0, 0xa0, 0xe1, 0x4, 0xd0, 0x4b, 0xe2, 0x0, 0x48, 0xbd, 0xe8, 0x1e, 0xff, 0x2f, 0xe1, 0x4, 0xb0, 0x2d, 0xe5, 0x0, 0xb0, 0x8d, 0xe2, 0x2a, 0x30, 0xa0, 0xe3, 0x3, 0x0, 0xa0, 0xe1, 0x0, 0xd0, 0x4b, 0xe2, 0x4, 0xb0, 0x9d, 0xe4, 0x1e, 0xff, 0x2f, 0xe1};

//f7 ff ff fe
//f0 00 f8 06



const unsigned char foo[] = {0x80,
					   0xb5,
					   0x00,
					   0xaf,
					   0x00, 0xf0,  0x06, 0xf8,
					   0x03,
					   0x1c,
					   0x18, 0x1c, 0xbd, 0x46, 0x80, 0xbc, 0x02, 0xbc, 0x08, 0x47, 0x80, 0xb5, 0x00, 0xaf, 0x2a, 0x23, 0x18, 0x1c, 0xbd, 0x46, 0x80, 0xbc, 0x02, 0xbc, 0x08, 0x47};

const char HALLO_SIMON = 'A';

int bar(void);

int main(void)
{
	char * foo_string = "Hallo Simon!";
    puts("Hello World! 1");
	func_ptr_t bar_ptr = riot_open(bar);
	func_ptr_t foo_ptr = riot_open(foo + 1);
    puts("Hello World! 2");
    printf("bar_ptr: %p, %p\n", bar_ptr, bar);
    printf("foo_ptr: %p, %p\n", foo_ptr, foo);
	int result_bar = bar_ptr();
	int result_foo = foo_ptr();
    printf("result bar: %d\n", result_bar);
    printf("result foo: %d\n", result_foo);
	while(1);
    return 0;
}

int bar(void) {
	return 23;
}
