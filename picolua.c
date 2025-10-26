/* 
 * minimal pico-sdk lua example
 *
 * Copyright 2023 Jeremy Grosser <jeremy@synack.me>
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "drivers/lcd.h"
#include "drivers/term.h"
#include "drivers/keyboard.h"
#include "drivers/fs.h"
#include "drivers/multicore.h"
#include "picolua-api/sys.h"

#include "corelua.h"

int main() {
	set_system_mhz(150);

	lcd_init();
	keyboard_init();
	stdio_picocalc_init(); 
	fs_init();
		
	multicore_fifo_drain();
	multicore_fifo_clear_irq();

	multicore_launch_core1(lua_main);
	
	irq_set_exclusive_handler(SIO_FIFO_IRQ_NUM(0), handle_multicore_fifo);
	irq_set_enabled(SIO_FIFO_IRQ_NUM(0), true);

	while (true) {
		if (atomic_load(&fs_needs_remount) == true) {
			if (fs_mount()) {
				printf("\x1b[92mOK!\x1b[m\n");
			} else {
				printf("Failed to mount!\x1b[m\n");
			}
			atomic_store(&fs_needs_remount, false);
		}
		sleep_ms(10);
	}
}
