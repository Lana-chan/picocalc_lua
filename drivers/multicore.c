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

// these exist only because the calls were originally to timeout functions which return values
void multicore_fifo_push(uint32_t value) {
	multicore_fifo_push_blocking_inline(value);
}

bool multicore_fifo_pop(uint32_t* value) {
	*value = multicore_fifo_pop_blocking_inline();
	return true;
}

void multicore_fifo_push_string(const char* string) {
	size_t len = strlen(string);
	multicore_fifo_push_timeout_us(len, FIFO_TIMEOUT);
	while(*string) {
		multicore_fifo_push_timeout_us(*string, FIFO_TIMEOUT);
		string++;
	}
}

char* multicore_fifo_pop_string() {
	uint32_t len;
	if (!multicore_fifo_pop(&len)) return NULL;
	char* string = malloc((size_t)len+1);
	for (int i = 0; i < len; i++) {
		if (multicore_fifo_rvalid()) string[i] = multicore_fifo_pop_blocking();
		else string[i] = 0;
	}
	string[len] = 0;
	return string;
}