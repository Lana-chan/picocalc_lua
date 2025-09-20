#pragma once

#include "types.h"

typedef u16 Color;

Color draw_color_from_hsv(u8 h, u8 s, u8 v);
void draw_color_to_hsv(Color c, u8* h, u8* s, u8* v);
Color draw_color_add(Color c1, Color c2);
Color draw_color_subtract(Color c1, Color c2);
Color draw_color_mul(Color c, float factor);

void draw_point(i32 x, i32 y, Color color);
void draw_clear();
void draw_rect(i32 x, i32 y, i32 width, i32 height, Color color);
void draw_fill_rect(i32 x, i32 y, i32 width, i32 height, Color color);
void draw_line(i32 x0, i32 y0, i32 x1, i32 y1, Color color);
void draw_circle(i32 xm, i32 ym, i32 r, Color color);
void draw_fill_circle(i32 xm, i32 ym, i32 r, Color color);
void draw_polygon(int n, float* points, Color color);
void draw_fill_polygon(int n, float* points, Color color);
void draw_triangle_shaded(Color c1, float x1, float y1, Color c2, float x2, float y2, Color c3, float x3, float y3);

int draw_fifo_receiver(uint32_t message);
void draw_fifo_point(i32 x, i32 y, Color color);
void draw_fifo_clear();
void draw_fifo_rect(i32 x, i32 y, i32 width, i32 height, Color color);
void draw_fifo_fill_rect(i32 x, i32 y, i32 width, i32 height, Color color);
void draw_fifo_line(i32 x0, i32 y0, i32 x1, i32 y1, Color color);
void draw_fifo_circle(i32 xm, i32 ym, i32 r, Color color);
void draw_fifo_fill_circle(i32 xm, i32 ym, i32 r, Color color);
void draw_fifo_polygon(int n, float* points, Color color);
void draw_fifo_fill_polygon(int n, float* points, Color color);
void draw_fifo_triangle_shaded(Color c1, float x1, float y1, Color c2, float x2, float y2, Color c3, float x3, float y3);

#define MIRROR_V 1
#define MIRROR_H 2
void draw_blit(i32 x, i32 y, i32 source_x, i32 source_y, i32 width, i32 height, Color* source, i32 source_width, i32 source_height);
void draw_blit_masked_flipped(i32 x, i32 y, i32 source_x, i32 source_y, i32 width, i32 height, Color mask, u8 flip, Color* source, i32 source_width, i32 source_height);

