#include "multicore.h"

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

void multicore_fifo_push_string(const char* source) {
	size_t len = strlen(source);
	char* dest = malloc((size_t)len+1);
	if (!dest) {
		multicore_fifo_push_blocking_inline(0);
		multicore_fifo_push_blocking_inline((uint32_t)NULL);
	}
	strncpy(dest, source, len);
	dest[len] = 0;
	multicore_fifo_push_blocking_inline(len);
	multicore_fifo_push_blocking_inline((uint32_t)dest);
}

size_t multicore_fifo_pop_string(char** string) {
	size_t len = multicore_fifo_pop_blocking_inline();
	*string = (char*)multicore_fifo_pop_blocking_inline();
	return len;
}

void multicore_main() {
	lcd_init();
	multicore_fifo_drain();

	while (true) {
		handle_multicore_fifo();
	}
}