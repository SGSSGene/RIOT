/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test dynamic application for the dynamic loader
 *
 * @author      Dimitri Schachmann <d.schachmann@fu-berlin.de>
 * @author      Simon Gene Gottlieb <s.gottlieb@fu-berlin.de>
 * @author      Ron Wenzel <ron.wenzel@fu-berlin.de>
 * @author      Christopher Pockrandt <christopher.pockrandt@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>

extern void led_on(void);
extern void led_off(void);

int dyn_main(void) {
	int result = func1();

    uint16_t i, j;

    for (i = 1; i < 10; i++) {
        for (j = 1; j != 0; j++) {
            asm volatile(" nop ");
        }
    }
	led_on();
	
	for (j = 1; j != 0; j++) {
		asm volatile(" nop ");
	}
	led_off();

	#ifdef DOUBLE_BLINK
	for (j = 1; j != 0; j++) {
		asm volatile(" nop ");
	}
	led_on();
	for (j = 1; j != 0; j++) {
		asm volatile(" nop ");
	}
	led_off();
	#endif

	return result;
}

int func1(void) {
	return func2() + func2();
}

int func2(void) {
	return 21;
}
