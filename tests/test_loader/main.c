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
 * @brief       Test application for the dynamic loader
 *
 * @author      Dimitri Schachmann <d.schachmann@fu-berlin.de>
 * @author      Simon Gene Gottlieb <s.gottlieb@fu-berlin.de>
 * @author      Ron Wenzel <ron.wenzel@fu-berlin.de>
 * @author      Christopher Pockrandt <christopher.pockrandt@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include "elfloader.h"

// provide the dynamic app as an array (workaround)
#include "../test_dyn_app/dyn_main.h"

typedef int dyn_entry_func(void);

int main(void)
{
	// relocate object file at char * dyn_app
	int entry = elfloader_load(dyn_app, "dyn_main");

	printf("Dynamic entry point address: 0x%x\n", elfloader_autostart_processes);

	// cast to function pointer
	dyn_entry_func* dyn_entry =
		(dyn_entry_func*) elfloader_autostart_processes ;

	// execute dynamic application at function pointer
	int result = dyn_entry();

	printf("==============================\n");
	printf("Result of dynamic function: %d\n", result);

    return 0;
}