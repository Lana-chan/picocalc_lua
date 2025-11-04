#include "psram.h"

psram_spi_inst_t psram_spi;
psram_spi_inst_t* async_spi_inst;

void psram_init() {
	psram_spi = psram_spi_init(pio0, -1);
}