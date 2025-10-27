#include "multicore.h"

#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "lcd.h"
#include "draw.h"

void handle_multicore_fifo() {
	// take first FIFO packet and pass to different handlers
	while (multicore_fifo_rvalid()) {
		uint32_t packet = multicore_fifo_pop_blocking_inline();
		
		if (lcd_fifo_receiver(packet)) break;
		if (draw_fifo_receiver(packet)) break;
	}

	multicore_fifo_clear_irq();
}

void multicore_fifo_push_string(const char* source, size_t len) {
	char* dest = strndup(source, len);
	if (!dest) {
		multicore_fifo_push_blocking_inline(0);
		multicore_fifo_push_blocking_inline((uint32_t)NULL);
	}
	multicore_fifo_push_blocking_inline(len);
	multicore_fifo_push_blocking_inline((uint32_t)dest);
}

size_t multicore_fifo_pop_string(char** string) {
	size_t len = multicore_fifo_pop_blocking_inline();
	*string = (char*)multicore_fifo_pop_blocking_inline();
	return len;
}

void multicore_init() {
	multicore_fifo_drain();
	multicore_fifo_clear_irq();
	irq_set_exclusive_handler(SIO_FIFO_IRQ_NUM(0), handle_multicore_fifo);
	irq_set_enabled(SIO_FIFO_IRQ_NUM(0), true);
}