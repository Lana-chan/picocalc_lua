#pragma once

#include "types.h"
#include "multicore.h"
#include <stdint.h>
#include <stdbool.h>

#define LCD_WIDTH 320
#define LCD_HEIGHT 320
#define MEM_HEIGHT 480

#define LCD_BUFFERMODE_DIRECT 0
#define LCD_BUFFERMODE_PSRAM 1
#define LCD_BUFFERMODE_RAM 2

#define RED(a)      ((((a) & 0xf800) >> 11) << 3)
#define GREEN(a)    ((((a) & 0x07e0) >> 5) << 2)
#define BLUE(a)     (((a) & 0x001f) << 3)

#define RGB(r,g,b) ((u16)(((r) >> 3) << 11 | ((g) >> 2) << 5 | (b >> 3)))

int lcd_fifo_receiver(uint32_t message);

void lcd_point_local(u16 color, int x, int y);
void lcd_draw_local(u16* pixels, int x, int y, int width, int height);
void lcd_fill_local(u16 color, int x, int y, int width, int height);
void lcd_clear_local();
bool lcd_buffer_enable_local(int mode);
void lcd_buffer_blit_local();
void lcd_draw_char_local(int x, int y, u16 fg, u16 bg, char c);
void lcd_draw_text_local(int x, int y, u16 fg, u16 bg, const char* text, size_t len);
void lcd_printf_local(int x, int y, u16 fg, u16 bg, const char* format, ...);
void lcd_scroll_local(int lines);
void lcd_clear_local();

int lcd_load_font(const char* filename);

typedef struct {
	u8* glyphs;
	uint8_t glyph_count;
	uint8_t glyph_width;
	uint8_t glyph_height;
	uint8_t bytewidth;
	uint8_t term_width;
	uint8_t term_height;
	u16* glyph_colorbuf;
	char firstcode;
} font_t;

extern font_t font;

void lcd_init();
void lcd_on();
void lcd_off();
void lcd_blank();
void lcd_unblank();
void lcd_setup_scrolling(int top_fixed_lines, int bottom_fixed_lines);

static inline void lcd_point(u16 color, int x, int y) {
	if (get_core_num() == 0) lcd_point_local(color, x, y);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_POINT);
		multicore_fifo_push_blocking_inline(color);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
	}
}

static inline void lcd_draw(u16* pixels, int x, int y, int width, int height) {
	if (get_core_num() == 0) lcd_draw_local(pixels, x, y, width, height);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_DRAW);
		multicore_fifo_push_blocking_inline((uint32_t)pixels);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
		multicore_fifo_push_blocking_inline(width);
		multicore_fifo_push_blocking_inline(height);
	}
}

static inline void lcd_fill(u16 color, int x, int y, int width, int height) {
	if (get_core_num() == 0) lcd_fill_local(color, x, y, width, height);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_FILL);
		multicore_fifo_push_blocking_inline(color);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
		multicore_fifo_push_blocking_inline(width);
		multicore_fifo_push_blocking_inline(height);
	}
}

static inline void lcd_clear() {
	if (get_core_num() == 0) lcd_clear_local();
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_CLEAR);
	}
}

static inline bool lcd_buffer_enable(int mode) {
	if (get_core_num() == 0) return lcd_buffer_enable_local(mode);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_BUFEN);
		multicore_fifo_push_blocking_inline(mode);
		return multicore_fifo_pop_blocking_inline();
	}
}

static inline void lcd_buffer_blit() {
	if (get_core_num() == 0) lcd_buffer_blit_local();
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_BUFBLIT);
	}
}

static inline void lcd_draw_char(int x, int y, u16 fg, u16 bg, char c) {
	if (get_core_num() == 0) lcd_draw_char_local(x, y, fg, bg, c);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_CHAR);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
		multicore_fifo_push_blocking_inline(fg);
		multicore_fifo_push_blocking_inline(bg);
		multicore_fifo_push_blocking_inline(c);
	}
}

static inline void lcd_draw_text(int x, int y, u16 fg, u16 bg, const char* text, size_t len) {
	if (get_core_num() == 0) lcd_draw_text_local(x, y, fg, bg, text, len);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_TEXT);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
		multicore_fifo_push_blocking_inline(fg);
		multicore_fifo_push_blocking_inline(bg);
		multicore_fifo_push_string(text, len);
	}
}

static inline void lcd_scroll(int lines) {
	if (get_core_num() == 0) lcd_scroll_local(lines);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_SCROLL);
		multicore_fifo_push_blocking_inline(lines);
	}
}