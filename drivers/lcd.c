#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "psram_spi.h"

#include "lcd.h"
#include "lcd_lut.h"
#include "default_font.h"
#include "fs.h"
#include "psram.h"
#include "../pico_fatfs/fatfs/ff.h"

#define LCD_SCK 10
#define LCD_TX  11
#define LCD_RX  12
#define LCD_CS  13
#define LCD_DC  14
#define LCD_RST 15
#define LCD_SPI spi1
#define LCD_SPI_SPEED (75 * 1000 * 1000)

void(*lcd_draw_ptr) (u16*,int,int,int,int);
void(*lcd_fill_ptr) (u16,int,int,int,int);
void(*lcd_point_ptr) (u16,int,int);
void(*lcd_clear_ptr) (void);
void(*lcd_buffer_read_ptr) (int);

uint8_t* framebuffer;
int framebuffer_mode;

#define LCD_TMPBUF_SIZE LCD_WIDTH*2
uint16_t lcd_tmpbuf[LCD_TMPBUF_SIZE];

static inline void lcd_set_dc_cs(bool dc, bool cs) {
	busy_wait_us(1);
	gpio_put_masked((1u << LCD_DC) | (1u << LCD_CS), !!dc << LCD_DC | !!cs << LCD_CS);
	busy_wait_us(1);
}

static inline void lcd_write_cmd(const uint8_t *cmd, size_t count) {
	lcd_set_dc_cs(0, 0);
	spi_write_blocking(LCD_SPI, cmd, 1);
	if (count >= 2) {
		lcd_set_dc_cs(1, 0);
		spi_write_blocking(LCD_SPI, cmd+1, count-1);
	}
	lcd_set_dc_cs(0, 1);
}

static inline void lcd_initcmd(const uint8_t *init_seq) {
	const uint8_t *cmd = init_seq;
	while (*cmd) {
		lcd_write_cmd(cmd + 2, *cmd);
		sleep_ms(*(cmd + 1) * 5);
		cmd += *cmd + 2;
	}
}

static void lcd_set_region(int x1, int y1, int x2, int y2) {
	lcd_set_dc_cs(0, 0);
	const uint8_t cmd1[] = {0x2A, (x1 >> 8), (x1 & 0xFF), (x2 >> 8), (x2 & 0xFF)};
	const uint8_t cmd2[] = {0x2B, (y1 >> 8), (y1 & 0xFF), (y2 >> 8), (y2 & 0xFF)};
	lcd_write_cmd(cmd1, 5);
	lcd_write_cmd(cmd2, 5);
	uint8_t cmd = 0x2c; // RAMWR
	lcd_write_cmd(&cmd, 1);
	lcd_set_dc_cs(1, 0);
}

void lcd_blank() {
	// Enter Sleep
	uint8_t cmd = 0x10;
	lcd_write_cmd(&cmd, 1);
}

void lcd_unblank() {
	// Exit Sleep
	uint8_t cmd = 0x11;
	lcd_write_cmd(&cmd, 1);
}

void lcd_on() {
	// Display on
	uint8_t cmd = 0x29;
	lcd_write_cmd(&cmd, 1);
}

void lcd_off() {
	// Display off
	uint8_t cmd = 0x29;
	lcd_write_cmd(&cmd, 1);
}

static inline void normalize_coords(int* x, int* y, int* width, int* height, int scrheight) {
	*y %= scrheight;
	*x = (*x < 0 ? 0 : (*x >= LCD_WIDTH ? LCD_WIDTH : *x));
	*width = (*width < 0 ? 0 : (*x + *width >= LCD_WIDTH ? LCD_WIDTH - *x : *width));
	*height = (*height < 0 ? 0 : (*y + *height >= scrheight ? scrheight - *y : *height));
}

static void lcd_direct_draw(u16* pixels, int x, int y, int width, int height) {
	normalize_coords(&x, &y, &width, &height, MEM_HEIGHT);
	lcd_set_region(x, y, x + width - 1, y + height - 1);

	//lcd_write_data((u8*) pixels, width * height * 2);
	spi_set_format(LCD_SPI, 16, 0, 0, SPI_MSB_FIRST);
	spi_write16_blocking(LCD_SPI, pixels, width * height);
	spi_set_format(LCD_SPI, 8, 0, 0, SPI_MSB_FIRST);
	lcd_set_dc_cs(0, 1);
}

static void lcd_direct_fill(u16 color, int x, int y, int width, int height) {
	normalize_coords(&x, &y, &width, &height, MEM_HEIGHT);
	lcd_set_region(x, y, x + width - 1, y + height - 1);
	
	int remaining = width * height;
	int chunk_size = remaining < LCD_TMPBUF_SIZE ? remaining : LCD_TMPBUF_SIZE;
	for (int i = 0; i < chunk_size; i++) lcd_tmpbuf[i] = color;
	
	spi_set_format(LCD_SPI, 16, 0, 0, SPI_MSB_FIRST);

	while (remaining > chunk_size) {
		spi_write16_blocking(LCD_SPI, lcd_tmpbuf, chunk_size);
		remaining -= chunk_size;
	}
	spi_write16_blocking(LCD_SPI, lcd_tmpbuf, remaining);
	
	spi_set_format(LCD_SPI, 8, 0, 0, SPI_MSB_FIRST);
	lcd_set_dc_cs(0, 1);
}

static void lcd_direct_point(u16 color, int x, int y) {
	lcd_fill(color, x, y, 1, 1);
}

static void lcd_direct_clear() {
	lcd_fill(0, 0, 0, LCD_WIDTH, MEM_HEIGHT);
}

static void lcd_psram_draw(u16* pixels, int x, int y, int width, int height) {
	normalize_coords(&x, &y, &width, &height, LCD_HEIGHT);

	int remain;
	for (uint32_t iy = y * LCD_WIDTH; iy < (y + height) * LCD_WIDTH; iy += LCD_WIDTH) {
		remain = width;
		for (uint32_t ix = x; ix < (x + width); ix+=10) {
			psram_write(&psram_spi, (iy + ix)<<1, (uint8_t*)pixels, (remain < 10 ? remain<<1 : 20));
			pixels += (remain < 10 ? remain : 10);
			remain -= 10;
		}
	}
}

static void lcd_psram_fill(u16 color, int x, int y, int width, int height) {
	normalize_coords(&x, &y, &width, &height, LCD_HEIGHT);

	int remain;
	for (int i = 0; i < 10; i++) lcd_tmpbuf[i] = color;
	for (uint32_t iy = y * LCD_WIDTH; iy < (y + height) * LCD_WIDTH; iy += LCD_WIDTH) {
		remain = width;
		for (uint32_t ix = x; ix < (x + width); ix+=10) {
			psram_write(&psram_spi, (iy + ix)<<1, (uint8_t*)lcd_tmpbuf, (remain < 10 ? remain<<1 : 20));
			remain -= 10;
		}
	}
}

static void lcd_psram_point(u16 color, int x, int y) {
	if (x >= 0 && y >= 0 && x < LCD_WIDTH && y < LCD_HEIGHT)
		psram_write16(&psram_spi, (x + y * LCD_WIDTH)<<1, color);
}

static void lcd_psram_clear() {
	lcd_psram_fill(0, 0, 0, LCD_WIDTH, LCD_HEIGHT);
}

static void lcd_ram_draw(u16* pixels, int x, int y, int width, int height) {
	normalize_coords(&x, &y, &width, &height, LCD_HEIGHT);

	for (uint32_t iy = y * LCD_WIDTH; iy < (y + height) * LCD_WIDTH; iy += LCD_WIDTH) {
		for (uint32_t ix = x; ix < (x + width); ix++) {
			framebuffer[iy+ix] = lcd_to8[*pixels++];
		}
	}
}

static void lcd_ram_fill(u16 color, int x, int y, int width, int height) {
	normalize_coords(&x, &y, &width, &height, LCD_HEIGHT);

	for (uint32_t iy = y * LCD_WIDTH; iy < (y + height) * LCD_WIDTH; iy += LCD_WIDTH) {
		memset(framebuffer + x + iy, lcd_to8[color], width);
	}
}

static void lcd_ram_point(u16 color, int x, int y) {
	if (x >= 0 && y >= 0 && x < LCD_WIDTH && y < LCD_HEIGHT)
		framebuffer[(x + y * LCD_WIDTH)] = lcd_to8[color];
}

static void lcd_ram_clear() {
	memset(framebuffer, 0, LCD_WIDTH * LCD_HEIGHT);
}

static inline void lcd_buffer_read_psram(int y) {
	for (int x = 0; x < LCD_TMPBUF_SIZE; x+=10) {
		psram_read(&psram_spi, (x+y)<<1, (uint8_t*)(lcd_tmpbuf + x), 20);
	}
}

static inline void lcd_buffer_read_ram(int y) {
	for (int x = 0; x < LCD_TMPBUF_SIZE; x++) {
		lcd_tmpbuf[x] = lcd_to16[framebuffer[x + y]];
	}
}

void lcd_buffer_blit_local() {
	if (framebuffer_mode == LCD_BUFFERMODE_DIRECT) return;
	
	lcd_set_region(0, 0, 319, 319);

	spi_set_format(LCD_SPI, 16, 0, 0, SPI_MSB_FIRST);

	for (int y = 0; y < LCD_HEIGHT * LCD_WIDTH; y += LCD_TMPBUF_SIZE) {
		lcd_buffer_read_ptr(y);
		spi_write16_blocking(LCD_SPI, lcd_tmpbuf, LCD_TMPBUF_SIZE);
	}

	spi_set_format(LCD_SPI, 8, 0, 0, SPI_MSB_FIRST);
	lcd_set_dc_cs(0, 1);
}

void lcd_draw_local(u16* pixels, int x, int y, int width, int height) {
	lcd_draw_ptr(pixels, x, y, width, height);
}

void lcd_fill_local(u16 color, int x, int y, int width, int height) {
	lcd_fill_ptr(color, x, y, width, height);
}

void lcd_point_local(u16 color, int x, int y) {
	lcd_point_ptr(color, x, y);
}

void lcd_clear_local() {
	lcd_clear_ptr();
}

bool lcd_buffer_enable_local(int mode) {
	if (mode != LCD_BUFFERMODE_RAM) {
		if (framebuffer) {
			free(framebuffer);
			framebuffer = NULL;
		}
	}

	if (mode == LCD_BUFFERMODE_DIRECT) {
		lcd_draw_ptr = &lcd_direct_draw;
		lcd_fill_ptr = &lcd_direct_fill;
		lcd_point_ptr = &lcd_direct_point;
		lcd_clear_ptr = &lcd_direct_clear;
		lcd_buffer_read_ptr = NULL;
		framebuffer_mode = mode;
		return true;
	} else if (mode == LCD_BUFFERMODE_PSRAM) {
		lcd_draw_ptr = &lcd_psram_draw;
		lcd_fill_ptr = &lcd_psram_fill;
		lcd_point_ptr = &lcd_psram_point;
		lcd_clear_ptr = &lcd_psram_clear;
		lcd_buffer_read_ptr = &lcd_buffer_read_psram;
		framebuffer_mode = mode;
		return true;
	} else if (mode == LCD_BUFFERMODE_RAM) {
		if (!framebuffer) {
			framebuffer = malloc(LCD_WIDTH * LCD_HEIGHT);
			if (framebuffer) {
				lcd_draw_ptr = &lcd_ram_draw;
				lcd_fill_ptr = &lcd_ram_fill;
				lcd_point_ptr = &lcd_ram_point;
				lcd_clear_ptr = &lcd_ram_clear;
				lcd_buffer_read_ptr = &lcd_buffer_read_ram;
				framebuffer_mode = mode;
				return true;
			}
		}
	}

	return false;
}

void lcd_scroll_local(int lines) {
	lines %= MEM_HEIGHT;
	uint8_t cmd[] = {0x37, (lines >> 8), (lines & 0xFF)};
	lcd_write_cmd(cmd, 3);
}

void lcd_setup_scrolling(int top_fixed_lines, int bottom_fixed_lines) {
	int vertical_scrolling_area = LCD_HEIGHT - (top_fixed_lines + bottom_fixed_lines);
	const uint8_t cmd[] = {0x33,
		(top_fixed_lines >> 8), (top_fixed_lines & 0xFF),
		(vertical_scrolling_area >> 8), (vertical_scrolling_area & 0xFF),
		(bottom_fixed_lines >> 8), (bottom_fixed_lines & 0xff)
	};
	lcd_write_cmd(cmd, 7);
}

font_t font = {
	.glyphs = NULL,
	.glyph_width = 0,
	.glyph_height = 0,
	.glyph_colorbuf = NULL
};

int lcd_load_font(const char* filename) {
	if (font.glyphs) { free(font.glyphs); font.glyphs = NULL; }
	if (font.glyph_colorbuf) { free(font.glyph_colorbuf); font.glyph_colorbuf = NULL; }

	if (!filename || filename[0] == '\0') {
		font.glyphs = malloc(2049 * sizeof(u8));
		memcpy(font.glyphs, (u8*)DEFAULT_GLYPHS, 2049);
		font.glyph_count = 255;
		font.glyph_width = DEFAULT_GLYPH_WIDTH;
		font.glyph_height = DEFAULT_GLYPH_HEIGHT;
		font.firstcode = 0;
	} else {
		FRESULT res;
		FIL fp;
		uint8_t bytesize;
		res = f_open(&fp, filename, FA_READ);
		if (res != FR_OK) { lcd_load_font(NULL); return res; }
		res = f_lseek(&fp, 2); // skip length
		if (res != FR_OK) { lcd_load_font(NULL); return res; }
		res = f_read(&fp, &font.glyph_count, 1, NULL);
		if (res != FR_OK) { lcd_load_font(NULL); return res; }
		res = f_read(&fp, &font.firstcode, 1, NULL);
		if (res != FR_OK) { lcd_load_font(NULL); return res; }
		res = f_read(&fp, &font.glyph_width, 1, NULL);
		if (res != FR_OK) { lcd_load_font(NULL); return res; }
		res = f_read(&fp, &font.glyph_height, 1, NULL);
		if (res != FR_OK) { lcd_load_font(NULL); return res; }
		res = f_read(&fp, &bytesize, 1, NULL);
		if (res != FR_OK) { lcd_load_font(NULL); return res; }
		font.glyphs = malloc(font.glyph_count * bytesize * sizeof(u8));
		res = f_read(&fp, font.glyphs, font.glyph_count * bytesize, NULL);
		if (res != FR_OK) return res;
		f_close(&fp);
	}

	font.bytewidth = font.glyph_width/8 + (font.glyph_width % 8 != 0);
	font.glyph_colorbuf = malloc(font.glyph_height * font.glyph_width * sizeof(u16));
	font.term_width = LCD_WIDTH / font.glyph_width;
	font.term_height = LCD_HEIGHT / font.glyph_height;
	return FR_OK;
}

void lcd_draw_char_local(int x, int y, u16 fg, u16 bg, char c) {
	if (c > font.glyph_count + font.firstcode) c = 0;
	int offset = ((u8)(c - font.firstcode)) * font.bytewidth * font.glyph_height;
	for (int j = 0; j < font.glyph_height; j++) {
		for (int i = 0; i < font.glyph_width; i++) {
			int mask = (1 << (7 - i%8));
			font.glyph_colorbuf[i + j * font.glyph_width] = (font.glyphs[offset + i / 8] & mask) ? fg : bg;
		}
		offset+=font.bytewidth;
	}

	lcd_draw(font.glyph_colorbuf, x, y, font.glyph_width, font.glyph_height);
}

void lcd_draw_text_local(int x, int y, u16 fg, u16 bg, const char* text, size_t len) {
	//if (y <= -font.glyph_height || y >= HEIGHT) return;
	for (int i = 0; i < len; i++) {
		lcd_draw_char(x, y, fg, bg, *text);
		x += font.glyph_width;
		if (x > LCD_WIDTH) return;
		text ++;
	}
}

void lcd_printf(int x, int y, u16 fg, u16 bg, const char* format, ...) {
	char buffer[512];
	va_list list;
	va_start(list, format);
	int result = vsnprintf(buffer, 512, format, list);
	if (result > -1) {
		lcd_draw_text(x, y, fg, bg, buffer, result);
	}
}

static const uint8_t st7789_init_seq[] = {
	// Positive Gamma Control
	16, 0, 0xE0, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F,
	// Negative Gamma Control
	16, 0, 0xE1, 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F,
	// Power Control 1
	3, 0, 0xC0, 0x17, 0x15,
	// Power Control 2
	2, 0, 0xC1, 0x41,
	// VCOM Control
	4, 0, 0xC5, 0x00, 0x12, 0x80,
	// Memory Access Control (0x48=BGR, 0x40=RGB)
	2, 0, 0x36, 0x48,
	// Pixel Interface Format  16 bit colour for SPI
	2, 0, 0x3A, 0x55,
	// Interface Mode Control
	2, 0, 0xB0, 0x00,
	// 60Hz
	3, 0, 0xB1, 0xD0, 0x11,
	// Invert colors on
	1, 0, 0x21,
	// Display Inversion Control
	2, 0, 0xB4, 0x02,
	// Display Function Control
	4, 0, 0xB6, 0x02, 0x02, 0x3B,
	// Entry Mode Set
	2, 0, 0xB7, 0xC6,
	2, 0, 0xE9, 0x00,
	// Adjust Control 3
	5, 0, 0xF7, 0xA9, 0x51, 0x2C, 0x82,
	// Exit Sleep
	1, 0, 0x11,
	// Terminate list
	0
};

void lcd_init() {
	// Init GPIO
	gpio_init(LCD_SCK);
	gpio_init(LCD_TX);
	gpio_init(LCD_RX);
	gpio_init(LCD_CS);
	gpio_init(LCD_DC);
	gpio_init(LCD_RST);

	gpio_set_dir(LCD_SCK, GPIO_OUT);
	gpio_set_dir(LCD_TX, GPIO_OUT);
	//gpio_set_dir(LCD_RX, GPIO_IN);
	gpio_set_dir(LCD_CS, GPIO_OUT);
	gpio_set_dir(LCD_DC, GPIO_OUT);
	gpio_set_dir(LCD_RST, GPIO_OUT);

	// Init SPI
	spi_init(LCD_SPI, LCD_SPI_SPEED);
	gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);
	gpio_set_function(LCD_TX, GPIO_FUNC_SPI);
	gpio_set_function(LCD_RX, GPIO_FUNC_SPI);
	gpio_set_input_hysteresis_enabled(LCD_RX, true);

	lcd_set_dc_cs(0, 1);
	gpio_put(LCD_RST, 1);

	// Reset controller
	gpio_put(LCD_RST, 0);
	busy_wait_us(20); // 20µs reset pulse (10µs minimum)
	gpio_put(LCD_RST, 1);
	busy_wait_us(120000); // 5ms required after reset, but 120ms needed before sleep out command

	// Setup LCD
	lcd_initcmd(st7789_init_seq);
	lcd_set_dc_cs(0, 1);

	lcd_buffer_enable(0);

	lcd_load_font(NULL);

	lcd_clear();
	lcd_on();
}

int lcd_fifo_receiver(uint32_t message) {
	uint32_t x, y, fg, bg, width, height, c;
	char* text;

	switch (message) {
		case FIFO_LCD_POINT:
			fg = multicore_fifo_pop_blocking_inline();
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			lcd_point_local((u16)fg, (int)x, (int)y);
			return 1;

		case FIFO_LCD_DRAW:
			fg = multicore_fifo_pop_blocking_inline();
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			width = multicore_fifo_pop_blocking_inline();
			height = multicore_fifo_pop_blocking_inline();
			lcd_draw_local((u16*)fg, (int)x, (int)y, (int)width, (int)height);
			return 1;

		case FIFO_LCD_FILL:
			fg = multicore_fifo_pop_blocking_inline();
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			width = multicore_fifo_pop_blocking_inline();
			height = multicore_fifo_pop_blocking_inline();
			lcd_fill_local((u16)fg, (int)x, (int)y, (int)width, (int)height);
			return 1;

		case FIFO_LCD_CLEAR:
			lcd_clear_local();
			return 1;

		case FIFO_LCD_BUFEN:
			x = multicore_fifo_pop_blocking_inline();
			multicore_fifo_push_blocking_inline(lcd_buffer_enable_local(x));
			return 1;

		case FIFO_LCD_BUFBLIT:
			lcd_buffer_blit_local();
			return 1;

		case FIFO_LCD_CHAR:
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			fg = multicore_fifo_pop_blocking_inline();
			bg = multicore_fifo_pop_blocking_inline();
			c = multicore_fifo_pop_blocking_inline();
			lcd_draw_char_local((int)x, (int)y, (u16)fg, (u16)bg, (char)c);
			return 1;

		case FIFO_LCD_TEXT:
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			fg = multicore_fifo_pop_blocking_inline();
			bg = multicore_fifo_pop_blocking_inline();
			width = multicore_fifo_pop_string(&text);
			lcd_draw_text_local((int)x, (int)y, (u16)fg, (u16)bg, text, width);
			free(text);
			return 1;

		case FIFO_LCD_SCROLL:
			height = multicore_fifo_pop_blocking_inline();
			lcd_scroll_local((int)height);
			return 1;

		default:
			return 0;
	}
}
