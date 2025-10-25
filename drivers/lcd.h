#pragma once

#include "types.h"
#include <stdint.h>
#include <stdbool.h>

#define LCD_WIDTH 320
#define LCD_HEIGHT 320
#define MEM_HEIGHT 480 

#define RED(a)      ((((a) & 0xf800) >> 11) << 3)
#define GREEN(a)    ((((a) & 0x07e0) >> 5) << 2)
#define BLUE(a)     (((a) & 0x001f) << 3)

#define RGB(r,g,b) ((u16)(((r) >> 3) << 11 | ((g) >> 2) << 5 | (b >> 3)))

int lcd_fifo_receiver(uint32_t message);

void lcd_fifo_draw(u16* pixels, int x, int y, int width, int height);
void lcd_fifo_fill(u16 color, int x, int y, int width, int height);
void lcd_fifo_clear();
void lcd_fifo_buffer_enable(bool enable);
void lcd_fifo_buffer_blit();
void lcd_fifo_draw_char(int x, int y, u16 fg, u16 bg, char c);
void lcd_fifo_draw_text(int x, int y, u16 fg, u16 bg, const char* text, size_t len);
void lcd_fifo_printf(int x, int y, u16 fg, u16 bg, const char* format, ...);
void lcd_fifo_scroll(int lines);

void lcd_draw(u16* pixels, int x, int y, int width, int height);
void lcd_fill(u16 color, int x, int y, int width, int height);
void lcd_point(u16 color, int x, int y);
void lcd_clear();

int lcd_load_font(const char* filename);
void lcd_draw_char(int x, int y, u16 fg, u16 bg, char c);
void lcd_draw_text(int x, int y, u16 fg, u16 bg, const char* text, size_t len);
void lcd_printf(int x, int y, u16 fg, u16 bg, const char* format, ...);
void lcd_scroll(int lines);

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

