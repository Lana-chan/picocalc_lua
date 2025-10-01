#include "multicore.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "lcd.h"
#include "draw.h"

void handle_multicore_fifo() {
	// take first FIFO packet and pass to different handlers
	if (multicore_fifo_rvalid()) {
		uint32_t packet = multicore_fifo_pop_blocking();
		
		if (lcd_fifo_receiver(packet)) return;
		if (draw_fifo_receiver(packet)) return;
	}
}

void multicore_fifo_push_string(const char* string) {
	size_t len = strlen(string);
	multicore_fifo_push_blocking_inline(len);
	for (int i = 0; i < len; i++) {
		multicore_fifo_push_blocking_inline(string[i]);
	}
}

char* multicore_fifo_pop_string() {
	uint32_t len = multicore_fifo_pop_blocking_inline();
	char* string = malloc((size_t)len+1);
	for (int i = 0; i < len; i++) {
		if (multicore_fifo_rvalid()) string[i] = multicore_fifo_pop_blocking_inline();
		else string[i] = 0;
	}
	string[len] = 0;
	return string;
}