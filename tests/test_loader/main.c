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

int main(void)
{
	// relocate object file at char * dyn_app
	process_t dyn_entry;
	int status = elfloader_load(dyn_app, "dyn_main", &dyn_entry);

	printf("Dynamic entry point address: 0x%x\n", dyn_entry);

	// execute dynamic application at function pointer
	int result = dyn_entry();

	printf("==============================\n");
	printf("Result of dynamic function: %d\n", result);

    return 0;
}
