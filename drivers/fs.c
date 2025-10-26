#include <stdlib.h>
#include <stdio.h>

#include "../pico_fatfs/tf_card.h"
#include "../pico_fatfs/fatfs/ff.h"
#include "pico/time.h"

#include "fs.h"

static FATFS global_fs;
static bool mounted = false;
volatile atomic_bool fs_needs_remount = false;

#define SD_MISO   16
#define SD_MOSI   19
#define SD_CS     17
#define SD_SCK    18
#define SD_DETECT 22
#define DEBOUNCE_US 80 * 1000ull

static void sd_hotplug(uint gpio, uint32_t events) {
	// wait debounce_ms and make sure status is still the same, then act accordingly
	if (gpio == SD_DETECT) {
		if (events & GPIO_IRQ_EDGE_RISE) {
			// sd removed
			busy_wait_us(DEBOUNCE_US);
			if (gpio_get(SD_DETECT) == 1) {
				printf("\x1b[91mSD unplugged! ");
				if (fs_unmount()) {
					printf("Unmounted!\x1b[m\n");
				} else {
					printf("Failed to unmount!\x1b[m\n");
				}
			}
		} else if (events & GPIO_IRQ_EDGE_FALL) {
			// sd inserted
			busy_wait_us(DEBOUNCE_US);
			if (gpio_get(SD_DETECT) == 0) {
				printf("\x1b[91mSD inserted, mounting...");
				atomic_store(&fs_needs_remount, true);
			}
		}
		gpio_acknowledge_irq(SD_DETECT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL);
	}
}

void fs_init() {
	pico_fatfs_spi_config_t config = {
		.spi_inst = spi0,
		.clk_slow = CLK_SLOW_DEFAULT,
		.clk_fast = CLK_FAST_DEFAULT,
		.pin_miso = SD_MISO,
		.pin_cs = SD_CS,
		.pin_sck = SD_SCK,
		.pin_mosi = SD_MOSI,
		.pullup = true,
	};
	pico_fatfs_set_config(&config);
	gpio_init(SD_DETECT);
	gpio_set_irq_enabled_with_callback(SD_DETECT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &sd_hotplug);
}

int fs_mount() {
	if (!mounted) mounted = f_mount(&global_fs, "", 1) == FR_OK;
	return mounted;
}

int fs_unmount() {
	if (mounted) mounted = !(f_unmount("") == FR_OK);
	return !mounted;
}

int fs_exists(const char* path) {
	FILINFO info;
	FRESULT result = f_stat(path, &info);
	return result == FR_OK && info.fname[0] != 0;
}