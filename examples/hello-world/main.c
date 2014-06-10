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
#include <string.h>
#include "loader/elfloader.h"
#include "../dyn_app/hex_app.h"

typedef int dyn_entry_func(void);

int main(void)
{
	elfloader_init();
	int entry = elfloader_load(hex_app);
	printf("Loaded Entry Point: 0x%x\n", elfloader_autostart_processes);
	dyn_entry_func* dyn_entry = (dyn_entry_func*) elfloader_autostart_processes ;
	int result = dyn_entry();
	printf("result: %d\n", result);
    puts("Done!");
    return 0;
}
