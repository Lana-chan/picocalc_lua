#include <stdlib.h>

#include "../pico_fatfs/tf_card.h"
#include "../pico_fatfs/fatfs/ff.h"

static FATFS global_fs;

void fs_init() {
  pico_fatfs_spi_config_t config = {
    .spi_inst = spi0,
    .clk_slow = CLK_SLOW_DEFAULT,
    .clk_fast = CLK_FAST_DEFAULT,
    .pin_miso = 16,
    .pin_cs = 17,
    .pin_sck = 18,
    .pin_mosi = 19,
    .pullup = true,
  };
  pico_fatfs_set_config(&config);
}

int fs_mount() {
  return f_mount(&global_fs, "", 0) == FR_OK;
}

char* fs_readfile(const char* path) {
  FIL fp;
  FRESULT result = f_open(&fp, path, FA_READ);
  if (result != FR_OK) return NULL;
  FSIZE_t size = f_size(&fp);
  char* buffer = malloc(size + 1);
  if (buffer == NULL) {
    f_close(&fp);
    return NULL;
  }
  UINT read;
  result = f_read(&fp, buffer, size, &read);
  if (result != FR_OK) {
    f_close(&fp);
    return NULL;
  }
  buffer[size] = '\0';
  return buffer;
}

int fs_writefile(const char* path, const char* data, int length) {
}
