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
	set_system_clk(150000);

	lcd_init();
	keyboard_init();
	stdio_picocalc_init(); 
	fs_init();
	
	multicore_launch_core1(lua_main);
	sleep_ms(200);

	multicore_fifo_drain();
	multicore_fifo_clear_irq();

	while (true) {
		handle_multicore_fifo();
		if (fs_needs_remount) {
			if (fs_mount()) {
				printf("\x1b[92mOK!\x1b[m\n");
			} else {
				printf("Failed to mount!\x1b[m\n");
			}
			fs_needs_remount = false;
		}
	}
}
