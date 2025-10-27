#pragma once

#include "types.h"
#include "multicore.h"
#include "lcd.h"

typedef u16 Color;

Color draw_color_from_hsv(u8 h, u8 s, u8 v);
void draw_color_to_hsv(Color c, u8* h, u8* s, u8* v);
Color draw_color_add(Color c1, Color c2);
Color draw_color_subtract(Color c1, Color c2);
Color draw_color_mul(Color c, float factor);

void draw_clear_local();
void draw_rect_local(i32 x, i32 y, i32 width, i32 height, Color color);
void draw_fill_rect_local(i32 x, i32 y, i32 width, i32 height, Color color);
void draw_line_local(i32 x0, i32 y0, i32 x1, i32 y1, Color color);
void draw_circle_local(i32 xm, i32 ym, i32 r, Color color);
void draw_fill_circle_local(i32 xm, i32 ym, i32 r, Color color);
void draw_polygon_local(int n, float* points, Color color);
void draw_fill_polygon_local(int n, float* points, Color color);
void draw_triangle_shaded_local(Color c1, float x1, float y1, Color c2, float x2, float y2, Color c3, float x3, float y3);

int draw_fifo_receiver(uint32_t message);

#define MIRROR_V 1
#define MIRROR_H 2
void draw_blit(i32 x, i32 y, i32 source_x, i32 source_y, i32 width, i32 height, Color* source, i32 source_width, i32 source_height);
void draw_blit_masked_flipped(i32 x, i32 y, i32 source_x, i32 source_y, i32 width, i32 height, Color mask, u8 flip, Color* source, i32 source_width, i32 source_height);

static inline void draw_point(i32 x, i32 y, Color color) {
	if (get_core_num() == 0) lcd_point_local(color, x, y);
	else {
		multicore_fifo_push_blocking_inline(FIFO_LCD_POINT);
		multicore_fifo_push_blocking_inline(color);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
	}
}

static inline void draw_clear() {
	if (get_core_num() == 0) draw_clear_local();
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_CLEAR);
	}
}

static inline void draw_rect(i32 x, i32 y, i32 width, i32 height, Color color) {
	if (get_core_num() == 0) draw_rect_local(x, y, width, height, color);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_RECT);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
		multicore_fifo_push_blocking_inline(width);
		multicore_fifo_push_blocking_inline(height);
		multicore_fifo_push_blocking_inline(color);
	}
}

static inline void draw_fill_rect(i32 x, i32 y, i32 width, i32 height, Color color) {
	if (get_core_num() == 0) draw_fill_rect_local(x, y, width, height, color);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_RECTFILL);
		multicore_fifo_push_blocking_inline(x);
		multicore_fifo_push_blocking_inline(y);
		multicore_fifo_push_blocking_inline(width);
		multicore_fifo_push_blocking_inline(height);
		multicore_fifo_push_blocking_inline(color);
	}
}

static inline void draw_line(i32 x0, i32 y0, i32 x1, i32 y1, Color color) {
	if (get_core_num() == 0) draw_line_local(x0, y0, x1, y1, color);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_LINE);
		multicore_fifo_push_blocking_inline(x0);
		multicore_fifo_push_blocking_inline(y0);
		multicore_fifo_push_blocking_inline(x1);
		multicore_fifo_push_blocking_inline(y1);
		multicore_fifo_push_blocking_inline(color);
	}
}

static inline void draw_circle(i32 xm, i32 ym, i32 r, Color color) {
	if (get_core_num() == 0) draw_circle_local(xm, ym, r, color);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_CIRC);
		multicore_fifo_push_blocking_inline(xm);
		multicore_fifo_push_blocking_inline(ym);
		multicore_fifo_push_blocking_inline(r);
		multicore_fifo_push_blocking_inline(color);
	}
}

static inline void draw_fill_circle(i32 xm, i32 ym, i32 r, Color color) {
	if (get_core_num() == 0) draw_fill_circle_local(xm, ym, r, color);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_CIRCFILL);
		multicore_fifo_push_blocking_inline(xm);
		multicore_fifo_push_blocking_inline(ym);
		multicore_fifo_push_blocking_inline(r);
		multicore_fifo_push_blocking_inline(color);
	}
}

static inline void draw_polygon(int n, float* points, Color color) {
	if (get_core_num() == 0) draw_polygon_local(n, points, color);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_POLY);
		multicore_fifo_push_blocking_inline(n);
		multicore_fifo_push_blocking_inline((uint32_t)points);
		multicore_fifo_push_blocking_inline(color);
	}
}

static inline void draw_fill_polygon(int n, float* points, Color color) {
	if (get_core_num() == 0) draw_fill_polygon_local(n, points, color);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_POLYFILL);
		multicore_fifo_push_blocking_inline(n);
		multicore_fifo_push_blocking_inline((uint32_t)points);
		multicore_fifo_push_blocking_inline(color);
	}
}

static inline void draw_triangle_shaded(Color c1, float x1, float y1, Color c2, float x2, float y2, Color c3, float x3, float y3) {
	if (get_core_num() == 0) draw_triangle_shaded_local(c1, x1, y1, c2, x2, y2, c3, x3, y3);
	else {
		multicore_fifo_push_blocking_inline(FIFO_DRAW_TRI);
		multicore_fifo_push_blocking_inline(c1);
		multicore_fifo_push_blocking_inline(x1);
		multicore_fifo_push_blocking_inline(y1);
		multicore_fifo_push_blocking_inline(c2);
		multicore_fifo_push_blocking_inline(x2);
		multicore_fifo_push_blocking_inline(y2);
		multicore_fifo_push_blocking_inline(c3);
		multicore_fifo_push_blocking_inline(x3);
		multicore_fifo_push_blocking_inline(y3);
	}
}