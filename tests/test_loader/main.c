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
#include <board.h>
#include "msp430.h"

#include <cc110x/cc1100-interface.h>

// provide the dynamic app as an array (workaround)
#include "../test_dyn_app/dyn_main.h"
#define PACK_SIZE 54

typedef struct {
	uint16_t id;
	uint16_t size;
	uint8_t data[ PACK_SIZE ];
} __attribute__((packed)) code_transm_pkt_t;

volatile uint16_t transmit_size = 0;
volatile uint16_t transmit_max  = 0;
uint8_t  dyn_app_remote[1024];

const uint8_t * _dyn_app = dyn_app + 512;

packet_handler_t packethandler(void* payload, int size, packet_info_t* info)
{
	printf("Received\n");
	code_transm_pkt_t * pkt = (code_transm_pkt_t*)payload;
	printf("pkt->id: %d, pkt->size: %d\n", pkt->id, pkt->size);

	if (transmit_size == 0) {
		transmit_size = pkt->size;
	}

	if (pkt->id == (transmit_max+1)) {
		memcpy(dyn_app_remote + PACK_SIZE*(pkt->id-1), pkt->data, PACK_SIZE);
		transmit_max = pkt->id;
		printf("Received packet %d\n", transmit_max);
	}
}

void flash_remote(void)
{
	uint8_t segment[512]; // Buffering to avoid writes to flash
	uint8_t *segmentToWrite = (uint8_t*)((int)_dyn_app & ~(512-1));
	int offset = _dyn_app - segmentToWrite;

	memcpy(segment, segmentToWrite, 512);
	memcpy(segment + offset, dyn_app_remote, 512-offset);
 	flashrom_erase(segmentToWrite);
 	flashrom_write(segmentToWrite, segment, 512);

	memcpy(segment, dyn_app_remote+512-offset, 512);
 	flashrom_erase(segmentToWrite+512);
 	flashrom_write(segmentToWrite+512, segment, 512);

	if(offset != 0) {
		memcpy(segment, segmentToWrite+1024, 512);
		memcpy(segment, dyn_app_remote+1024-offset, offset);
		flashrom_erase(segmentToWrite+1024);
		flashrom_write(segmentToWrite+1024, segment, 512);
	}
}

void led_on(void)
{
	LED_RED_ON;
}

void led_off(void)
{
	LED_RED_OFF;
}

int main(void)
{
	cc1100_init();
	cc1100_set_address(1);
	cc1100_set_channel(0);

	cc1100_set_packet_handler(5, packethandler);

	LED_RED_OFF;
	
	// relocate object file at char * dyn_app
	process_t dyn_entry;
	int status = elfloader_load(_dyn_app, "dyn_main", &dyn_entry, 0);

	printf("Dynamic entry point address: 0x%x\n", dyn_entry);

	// execute dynamic application at function pointer
	while(1) {
		dyn_entry();
		if ((transmit_size != 0) && (transmit_size == transmit_max)) {
			printf("flashing update.\n");
			flash_remote();
			elfloader_load(_dyn_app, "dyn_main", &dyn_entry, 0);
			transmit_max++;
		}
	}

    return 0;
}
