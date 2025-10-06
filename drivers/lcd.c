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
#include "font.h"
#include "multicore.h"

#define LCD_SCK 10
#define LCD_TX  11
#define LCD_RX  12
#define LCD_CS  13
#define LCD_DC  14
#define LCD_RST 15

#define FIFO_LCD         0
#define FIFO_LCD_DRAW    FIFO_LCD + 1
#define FIFO_LCD_FILL    FIFO_LCD + 2
#define FIFO_LCD_CLEAR   FIFO_LCD + 3
#define FIFO_LCD_BUFEN   FIFO_LCD + 4
#define FIFO_LCD_BUFBLIT FIFO_LCD + 5
#define FIFO_LCD_CHAR    FIFO_LCD + 6
#define FIFO_LCD_TEXT    FIFO_LCD + 7
#define FIFO_LCD_SCROLL  FIFO_LCD + 8

void(*lcd_draw_ptr) (u16*,int,int,int,int);
void(*lcd_fill_ptr) (u16,int,int,int,int);
void(*lcd_point_ptr) (u16,int,int);
void(*lcd_clear_ptr) (void);
psram_spi_inst_t psram_spi;

static int lcd_write_spi(void *buf, size_t len) {
	return spi_write_blocking(spi1, buf, len);
}

static inline void lcd_write_spi_dc(void *buf, size_t len, int dc) {
	gpio_put(LCD_DC, dc);
	lcd_write_spi(buf, len);
}

#define NUMARGS(...)  (sizeof((int[]){__VA_ARGS__}) / sizeof(int))
static int lcd_write_reg_num(int len, ...) {
	u8 buf[128];
	va_list args;
	int i;

	gpio_put(LCD_DC, 0);
	va_start(args, len);
	*buf = (u8)va_arg(args, unsigned int);
	spi_write_blocking(spi1, buf, sizeof(u8));
	len--;

	gpio_put(LCD_DC, 1);
	if (len > 0) {
		for (i = 0; i < len; i++) buf[i] = (u8)va_arg(args, unsigned int);
		spi_write_blocking(spi1, buf, len);
	}
	va_end(args);

	return 0;
}
#define lcd_write_reg(...) \
		lcd_write_reg_num(NUMARGS(__VA_ARGS__), __VA_ARGS__)

#define REGION_READ 0
#define REGION_WRITE 1

static void lcd_set_region(int x1, int y1, int x2, int y2, int rw) {
	gpio_put(LCD_CS, 0);
	lcd_write_reg(0x2A, (x1 >> 8), (x1 & 0xFF), (x2 >> 8), (x2 & 0xFF));
	lcd_write_reg(0x2B, (y1 >> 8), (y1 & 0xFF), (y2 >> 8), (y2 & 0xFF));
	if (rw) lcd_write_reg(0x2C);
	else lcd_write_reg(0x2E);
}

static int lcd_write_data(u8* buf, int len) {
	return lcd_write_spi(buf, len);
}

static void lcd_direct_draw(u16* pixels, int x, int y, int width, int height) {
	y %= MEM_HEIGHT;
	lcd_set_region(x, y, x + width - 1, y + height - 1, REGION_WRITE);

	//lcd_write_data((u8*) pixels, width * height * 2);
	spi_set_format(spi1, 16, 0, 0, SPI_MSB_FIRST);
	spi_write16_blocking(spi1, pixels, width * height);
	spi_set_format(spi1, 8, 0, 0, SPI_MSB_FIRST);
	gpio_put(LCD_CS, 1);
}

static void lcd_direct_fill(u16 color, int x, int y, int width, int height) {
	y %= MEM_HEIGHT;

	x = (x < 0 ? 0 : (x >= WIDTH ? WIDTH : x));
	width = (width < 0 ? 0 : (x + width - 1 >= WIDTH ? WIDTH - x - 1 : width));

	lcd_set_region(x, y, x + width - 1, y + height - 1, REGION_WRITE);

	gpio_put(LCD_DC, 1);
	u16 color2 = (color >> 8) | (color << 8);
	spi_set_format(spi1, 16, 0, 0, SPI_MSB_FIRST);

	u16 buf[WIDTH];
	int remaining = width * height;
	int chunk_size = remaining < WIDTH ? remaining : WIDTH;
	for (int i = 0; i < chunk_size; i++) buf[i] = color;

	while (remaining > chunk_size) {
		spi_write16_blocking(spi1, buf, chunk_size);
		remaining -= chunk_size;
	}
	spi_write16_blocking(spi1, buf, remaining);
	//for (int i = 0; i < width * height; i++) spi_write16_blocking(spi1, &color2, 1);
	spi_set_format(spi1, 8, 0, 0, SPI_MSB_FIRST);
	gpio_put(LCD_CS, 1);
}

static void lcd_direct_point(u16 color, int x, int y) {
	lcd_fill(color, x, y, 1, 1);
}

static void lcd_direct_clear() {
	lcd_fill(0, 0, 0, WIDTH, MEM_HEIGHT);
}

static void lcd_buffer_draw(u16* pixels, int x, int y, int width, int height) {
	for (uint32_t iy = y * WIDTH; iy < (y + height) * WIDTH; iy += WIDTH) {
		for (uint32_t ix = x; ix < (x + width); ix++) {
			psram_write16(&psram_spi, (iy + ix)<<1, *pixels++);
		}
	}
}

static void lcd_buffer_fill(u16 color, int x, int y, int width, int height) {
	for (uint32_t iy = y * WIDTH; iy < (y + height) * WIDTH; iy += WIDTH) {
		for (uint32_t ix = x; ix < (x + width); ix++) {
			psram_write16(&psram_spi, (iy + ix)<<1, color);
		}
	}
}

static void lcd_buffer_point(u16 color, int x, int y) {
	psram_write16(&psram_spi, (x + y * WIDTH)<<1, color);
}

static void lcd_buffer_clear() {
	lcd_buffer_fill(0, 0, 0, WIDTH, HEIGHT);
}

void lcd_buffer_blit() {
	lcd_set_region(0, 0, 319, 319, REGION_WRITE);

	u16 buf[WIDTH];
	spi_set_format(spi1, 16, 0, 0, SPI_MSB_FIRST);
	gpio_put(LCD_DC, 1);

	for (int y = 0; y < HEIGHT * WIDTH; y += WIDTH) {
		for (int x = 0; x < WIDTH; x++) {
			buf[x] = psram_read16(&psram_spi, (x + y)<<1);
		}
		spi_write16_blocking(spi1, buf, WIDTH);
	}

	spi_set_format(spi1, 8, 0, 0, SPI_MSB_FIRST);
	gpio_put(LCD_CS, 1);
}

void lcd_draw(u16* pixels, int x, int y, int width, int height) {
	lcd_draw_ptr(pixels, x, y, width, height);
}

void lcd_fill(u16 color, int x, int y, int width, int height) {
	lcd_fill_ptr(color, x, y, width, height);
}

void lcd_point(u16 color, int x, int y) {
	lcd_point_ptr(color, x, y);
}

void lcd_clear() {
	lcd_clear_ptr();
}

void lcd_buffer_enable(bool enable) {
	if (enable) {
		lcd_draw_ptr = &lcd_buffer_draw;
		lcd_fill_ptr = &lcd_buffer_fill;
		lcd_point_ptr = &lcd_buffer_point;
		lcd_clear_ptr = &lcd_buffer_clear;
	} else {
		lcd_draw_ptr = &lcd_direct_draw;
		lcd_fill_ptr = &lcd_direct_fill;
		lcd_point_ptr = &lcd_direct_point;
		lcd_clear_ptr = &lcd_direct_clear;
	}
}

void lcd_scroll(int lines) {
	lines %= MEM_HEIGHT;
	gpio_put(LCD_CS, 0);
	lcd_write_reg(0x37, (lines >> 8), (lines & 0xFF));
	gpio_put(LCD_CS, 1);
}

void lcd_setup_scrolling(int top_fixed_lines, int bottom_fixed_lines) {
	int vertical_scrolling_area = HEIGHT - (top_fixed_lines + bottom_fixed_lines);
	gpio_put(LCD_CS, 0);
	lcd_write_reg(0x33, (top_fixed_lines >> 8), (top_fixed_lines & 0xFF), (vertical_scrolling_area >> 8), 
		(vertical_scrolling_area & 0xFF), (bottom_fixed_lines >> 8), (bottom_fixed_lines & 0xff));
	gpio_put(LCD_CS, 1);
}

font_t font = {
	.glyphs = (u8*)GLYPHS,
	.glyph_width = GLYPH_WIDTH,
	.glyph_height = GLYPH_HEIGHT,
};


u16 glyph_buf[GLYPH_WIDTH * GLYPH_HEIGHT] __attribute__((aligned(4))); 

void lcd_draw_char(int x, int y, u16 fg, u16 bg, char c) {
	int offset = ((u8)c) * GLYPH_HEIGHT;
	for (int j = 0; j < GLYPH_HEIGHT; j++) {
		for (int i = 0; i < GLYPH_WIDTH; i++) {
			int mask = 1 << i;
			glyph_buf[i + j * GLYPH_WIDTH] = (font.glyphs[offset] & mask) ? fg : bg;
		}
		offset++;
	}

	lcd_draw(glyph_buf, x, y, GLYPH_WIDTH, GLYPH_HEIGHT);
}

void lcd_draw_text(int x, int y, u16 fg, u16 bg, const char* text) {
	//if (y <= -font.glyph_height || y >= HEIGHT) return;
	while(*text) {
		lcd_draw_char(x, y, fg, bg, *text);
		x += font.glyph_width;
		if (x > WIDTH) return;
		text ++;
	}
}

void lcd_printf(int x, int y, u16 fg, u16 bg, const char* format, ...) {
	char buffer[256];
	va_list list;
	va_start(list, format);
	int result = vsnprintf(buffer, 256, format, list);
	if (result > -1) {
		lcd_draw_text(x, y, fg, bg, buffer);
	}
}


#define LCD_SPI_SPEED   (75 * 1000 * 1000)
#define PORTCLR             1
#define PORTSET             2
#define PORTINV             3
#define LAT                 4
#define LATCLR              5
#define LATSET              6
#define LATINV              7
#define ODC                 8
#define ODCCLR              9
#define ODCSET              10
#define CNPU                12
#define CNPUCLR             13
#define CNPUSET             14
#define CNPUINV             15
#define CNPD                16
#define CNPDCLR             17
#define CNPDSET             18
#define ANSELCLR            -7
#define ANSELSET            -6
#define ANSELINV            -5
#define TRIS                -4
#define TRISCLR             -3
#define TRISSET             -2

static void pin_set_bit(int pin, unsigned int offset) {
	switch (offset) {
		case LATCLR:
			gpio_set_pulls(pin, false, false);
			gpio_pull_down(pin);
			gpio_put(pin, 0);
			return;
		case LATSET:
			gpio_set_pulls(pin, false, false);
			gpio_pull_up(pin);
			gpio_put(pin, 1);
			return;
		case LATINV:
			gpio_xor_mask(1 << pin);
			return;
		case TRISSET:
			gpio_set_dir(pin, GPIO_IN);
			sleep_us(2);
			return;
		case TRISCLR:
			gpio_set_dir(pin, GPIO_OUT);
			gpio_set_drive_strength(pin, GPIO_DRIVE_STRENGTH_12MA);
			sleep_us(2);
			return;
		case CNPUSET:
			gpio_set_pulls(pin, true, false);
			return;
		case CNPDSET:
			gpio_set_pulls(pin, false, true);
			return;
		case CNPUCLR:
		case CNPDCLR:
			gpio_set_pulls(pin, false, false);
			return;
		case ODCCLR:
			gpio_set_dir(pin, GPIO_OUT);
			gpio_put(pin, 0);
			sleep_us(2);
			return;
		case ODCSET:
			gpio_set_pulls(pin, true, false);
			gpio_set_dir(pin, GPIO_IN);
			sleep_us(2);
			return;
		case ANSELCLR:
			gpio_set_function(pin, GPIO_FUNC_SIO);
			gpio_set_dir(pin, GPIO_IN);
			return;
		default:
			printf("unknown pin_set_bit command\n");
			break;
	}
}

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
	spi_init(spi1, LCD_SPI_SPEED);
	gpio_set_function(LCD_SCK, GPIO_FUNC_SPI);
	gpio_set_function(LCD_TX, GPIO_FUNC_SPI);
	gpio_set_function(LCD_RX, GPIO_FUNC_SPI);
	gpio_set_input_hysteresis_enabled(LCD_RX, true);

	gpio_put(LCD_CS, 1);
	gpio_put(LCD_RST, 1);

	// Reset controller
	pin_set_bit(LCD_RST, LATSET);
	sleep_ms(10);
	pin_set_bit(LCD_RST, LATCLR);
	sleep_ms(10);
	pin_set_bit(LCD_RST, LATSET);
	sleep_ms(200);

	// Setup LCD
	gpio_put(LCD_CS, 0);
	// Positive Gamma Control
	lcd_write_reg(0xE0, 0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F);

	// Negative Gamma Control
	lcd_write_reg(0xE1, 0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F);

	lcd_write_reg(0xC0, 0x17, 0x15);          // Power Control 1
	lcd_write_reg(0xC1, 0x41);                // Power Control 2
	lcd_write_reg(0xC5, 0x00, 0x12, 0x80);    // VCOM Control
	lcd_write_reg(0x36, 0x48);                // Memory Access Control (0x48=BGR, 0x40=RGB)
	lcd_write_reg(0x3A, 0x55);                // Pixel Interface Format  16 bit colour for SPI
	lcd_write_reg(0xB0, 0x00);                // Interface Mode Control

	// Frame Rate Control
	//lcd_write_reg(0xB1, 0xA0);
	lcd_write_reg(0xB1, 0xD0, 0x11);          // 60Hz
	//lcd_write_reg(0xB1, 0xD0, 0x14);          // 90Hz
	lcd_write_reg(0x21);                      // Invert colors on

	lcd_write_reg(0xB4, 0x02);                // Display Inversion Control
	lcd_write_reg(0xB6, 0x02, 0x02, 0x3B);    // Display Function Control
	lcd_write_reg(0xB7, 0xC6);                // Entry Mode Set
	lcd_write_reg(0xE9, 0x00);
	lcd_write_reg(0xF7, 0xA9, 0x51, 0x2C, 0x82);  // Adjust Control 3
	lcd_write_reg(0x11);                      // Exit Sleep
	//sleep_ms(120);
	//lcd_write_reg(0x29);                      // Display on
	//sleep_ms(120);
	gpio_put(LCD_CS, 1);

	psram_spi = psram_spi_init(pio0, -1);
	lcd_buffer_enable(false);

	lcd_clear();
	lcd_on();
}

void lcd_blank() {
	gpio_put(LCD_CS, 0);
	lcd_write_reg(0x10);                      // Enter Sleep
	gpio_put(LCD_CS, 1);
}

void lcd_unblank() {
	gpio_put(LCD_CS, 0);
	lcd_write_reg(0x11);                      // Exit Sleep
	gpio_put(LCD_CS, 1);
}

void lcd_on() {
	gpio_put(LCD_CS, 0);
	lcd_write_reg(0x29);                      // Display on
	gpio_put(LCD_CS, 1);
}

void lcd_off() {
	gpio_put(LCD_CS, 0);
	lcd_write_reg(0x29);                      // Display off
	gpio_put(LCD_CS, 1);
}

int lcd_fifo_receiver(uint32_t message) {
	uint32_t x, y, fg, bg, width, height, c;
	char* text;

	switch (message) {
		case FIFO_LCD_DRAW:
			fg = multicore_fifo_pop_blocking_inline();
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			width = multicore_fifo_pop_blocking_inline();
			height = multicore_fifo_pop_blocking_inline();
			lcd_draw((u16*)fg, (int)x, (int)y, (int)width, (int)height);
			return 1;

		case FIFO_LCD_FILL:
			fg = multicore_fifo_pop_blocking_inline();
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			width = multicore_fifo_pop_blocking_inline();
			height = multicore_fifo_pop_blocking_inline();
			lcd_fill((u16)fg, (int)x, (int)y, (int)width, (int)height);
			return 1;

		case FIFO_LCD_CLEAR:
			lcd_clear();
			return 1;

		case FIFO_LCD_BUFEN:
			x = multicore_fifo_pop_blocking_inline();
			lcd_buffer_enable(x);
			return 1;

		case FIFO_LCD_BUFBLIT:
			lcd_buffer_blit();
			return 1;

		case FIFO_LCD_CHAR:
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			fg = multicore_fifo_pop_blocking_inline();
			bg = multicore_fifo_pop_blocking_inline();
			c = multicore_fifo_pop_blocking_inline();
			lcd_draw_char((int)x, (int)y, (u16)fg, (u16)bg, (char)c);
			return 1;

		case FIFO_LCD_TEXT:
			x = multicore_fifo_pop_blocking_inline();
			y = multicore_fifo_pop_blocking_inline();
			fg = multicore_fifo_pop_blocking_inline();
			bg = multicore_fifo_pop_blocking_inline();
			multicore_fifo_pop_string(&text);
			lcd_draw_text((int)x, (int)y, (u16)fg, (u16)bg, text);
			free(text);
			return 1;

		case FIFO_LCD_SCROLL:
			height = multicore_fifo_pop_blocking_inline();
			lcd_scroll((int)height);
			return 1;

		default:
			return 0;
	}
}

void lcd_fifo_draw(u16* pixels, int x, int y, int width, int height) {
	multicore_fifo_push_blocking_inline(FIFO_LCD_DRAW);
	multicore_fifo_push_blocking_inline((uint32_t)pixels);
	multicore_fifo_push_blocking_inline(x);
	multicore_fifo_push_blocking_inline(y);
	multicore_fifo_push_blocking_inline(width);
	multicore_fifo_push_blocking_inline(height);
}

void lcd_fifo_fill(u16 color, int x, int y, int width, int height) {
	multicore_fifo_push_blocking_inline(FIFO_LCD_FILL);
	multicore_fifo_push_blocking_inline(color);
	multicore_fifo_push_blocking_inline(x);
	multicore_fifo_push_blocking_inline(y);
	multicore_fifo_push_blocking_inline(width);
	multicore_fifo_push_blocking_inline(height);
}

void lcd_fifo_clear() {
	multicore_fifo_push_blocking_inline(FIFO_LCD_CLEAR);
}

void lcd_fifo_buffer_enable(bool enable) {
	multicore_fifo_push_blocking_inline(FIFO_LCD_BUFEN);
	multicore_fifo_push_blocking_inline(enable);
}

void lcd_fifo_buffer_blit() {
	multicore_fifo_push_blocking_inline(FIFO_LCD_BUFBLIT);
}

void lcd_fifo_draw_char(int x, int y, u16 fg, u16 bg, char c) {
	multicore_fifo_push_blocking_inline(FIFO_LCD_CHAR);
	multicore_fifo_push_blocking_inline(x);
	multicore_fifo_push_blocking_inline(y);
	multicore_fifo_push_blocking_inline(fg);
	multicore_fifo_push_blocking_inline(bg);
	multicore_fifo_push_blocking_inline(c);
}

void lcd_fifo_draw_text(int x, int y, u16 fg, u16 bg, const char* text) {
	multicore_fifo_push_blocking_inline(FIFO_LCD_TEXT);
	multicore_fifo_push_blocking_inline(x);
	multicore_fifo_push_blocking_inline(y);
	multicore_fifo_push_blocking_inline(fg);
	multicore_fifo_push_blocking_inline(bg);
	multicore_fifo_push_string(text);
}

void lcd_fifo_printf(int x, int y, u16 fg, u16 bg, const char* format, ...) {
	char buffer[256];
	va_list list;
	va_start(list, format);
	int result = vsnprintf(buffer, 256, format, list);
	if (result > -1) {
		lcd_fifo_draw_text(x, y, fg, bg, buffer);
	}
}

void lcd_fifo_scroll(int lines) {
	multicore_fifo_push_blocking_inline(FIFO_LCD_SCROLL);
	multicore_fifo_push_blocking_inline(lines);
}