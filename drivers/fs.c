#include <stdlib.h>
#include <stdio.h>

#include "../pico_fatfs/tf_card.h"
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

char* fs_error_strings[20] = {
	"Succeeded",
	"A hard error occurred in the low level disk I/O layer",
	"Assertion failed",
	"The physical drive cannot work",
	"Could not find the file",
	"Could not find the path",
	"The path name format is invalid",
	"Access denied due to prohibited access or directory full",
	"Access denied due to prohibited access",
	"The file/directory object is invalid",
	"The physical drive is write protected",
	"The logical drive number is invalid",
	"The volume has no work area",
	"There is no valid FAT volume",
	"The f_mkfs() aborted due to any problem",
	"Could not get a grant to access the volume within defined period",
	"The operation is rejected according to the file sharing policy",
	"LFN working buffer could not be allocated",
	"Number of open files > FF_FS_LOCK",
	"Given parameter is invalid",
};

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

FRESULT fs_readline(FIL* fp, char** buffer, UINT* read) {
	FRESULT result;

	if (*buffer) {
		free(*buffer);
		*buffer = NULL;
	}

	if (f_eof(fp)) return FR_OK;

	FSIZE_t old_fptr = fp->fptr;

	char c;
	while (c != '\n' && !f_eof(fp)) {
		result = f_read(fp, &c, 1, NULL);
		if (result != FR_OK) return result;
	}
	// length of file from old_fptr until newline, minus newline
	UINT to_read = fp->fptr - old_fptr;
	if (c == '\n') to_read--;
	f_lseek(fp, old_fptr);
	
	*buffer = malloc(to_read);
	result = f_read(fp, *buffer, to_read, read);
	if (result != FR_OK) return result;

	// skip that newline
	if (c == '\n') f_lseek(fp, fp->fptr + 1);

	return result;
}

FRESULT fs_writeline(FIL* fp, const char* line, UINT to_write, UINT* written) {
	UINT add;
	
	FRESULT result = f_write (fp, line, to_write, written);
	if (result != FR_OK) return result;

	const char n = '\n';
	result = f_write (fp, &n, 1, &add);
	if (result != FR_OK) return result;

	*written += add;
	return result;
}