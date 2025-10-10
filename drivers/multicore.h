#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum FIFO_CODES {
	FIFO_LCD = 256,
	FIFO_LCD_DRAW,
	FIFO_LCD_FILL,
	FIFO_LCD_CLEAR,
	FIFO_LCD_BUFEN,
	FIFO_LCD_BUFBLIT,
	FIFO_LCD_CHAR,
	FIFO_LCD_TEXT,
	FIFO_LCD_SCROLL,

	FIFO_DRAW,
	FIFO_DRAW_POINT,
	FIFO_DRAW_CLEAR,
	FIFO_DRAW_RECT,
	FIFO_DRAW_RECTFILL,
	FIFO_DRAW_LINE,
	FIFO_DRAW_CIRC,
	FIFO_DRAW_CIRCFILL,
	FIFO_DRAW_POLY,
	FIFO_DRAW_POLYFILL,
	FIFO_DRAW_TRI,
};

void multicore_fifo_push_string(const char* string, size_t len);
size_t multicore_fifo_pop_string(char** string);

void multicore_main();