#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "draw.h"
#include "lcd.h"

#define abs(x) ((x) < 0 ? -(x) : (x))

static void draw_horizontal_line(i32 x1, i32 x2, i32 y, Color color) {
	if (x1 >= LCD_WIDTH || x2 < 0 || y < 0 || y >= MEM_HEIGHT) return;
	if (x1 < 0) x1 = 0;
	if (x2 >= LCD_WIDTH) x2 = LCD_WIDTH;
	int offset = y * LCD_WIDTH;
	lcd_fill(color, x1, y, x2 - x1 + 1, 1);
}

void draw_color_to_hsv(Color c, u8* h, u8* s, u8* v) {
	u8 r = RED(c), g = GREEN(c), b = BLUE(c);
	int min = r < g ? (r < b ? r : b) : (g < b ? g : b);
	int max = r > g ? (r > b ? r : b) : (g > b ? g : b);

	*h = 0;
	*s = 0;
	*v = max;

	if (*v != 0) {
		*s = 255 * ((long)max - min) / *v;
		if (*s != 0) {
			if (max == r) *h = 0 + 43 * (g - b) / (max - min);
			else if (max == g) *h = 85 + 43 * (b - r) / (max - min);
			else *h = 171 + 43 * (r - g) / (max - min);
		} else {
			*h = 0;
		}
	}
}

Color draw_color_from_hsv(u8 h, u8 s, u8 v) {
	u8 r, g, b;
	int region = h / 43;
	int remainder = (h - (region * 43)) * 6;

	int p = (v * (255 - s)) >> 8;
	int q = (v * (255 - ((s * remainder) >> 8))) >> 8;
	int t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

	switch (region) {
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		default: r = v; g = p; b = q; break;
	}
	return RGB(r, g, b);
}

void draw_clear_local() {
	lcd_clear();
	lcd_scroll(0);
}

void draw_fill_rect_local(i16 x, i16 y, i16 width, i16 height, Color color) {
	if (x < 0) {
		width += x;
		x = 0;
	}
	if (y < 0) {
		height += y;
		y = 0;
	}
	if (x + width > LCD_WIDTH) width = LCD_WIDTH - x;
	if (y + height > MEM_HEIGHT) height = MEM_HEIGHT - y;

	lcd_fill(color, x, y, width, height);
}

void draw_rect_local(i16 x, i16 y, i16 width, i16 height, Color color) {
	draw_horizontal_line(x, x + width, y, color);
	for (i16 i = 1; i < height - 1; i++) {
		draw_point(x, y + i, color);
		draw_point(x + width - 1, y + i, color);
	}
	draw_horizontal_line(x, x + width, y + height - 1, color);
}

void draw_sprite_local(i16 x, i16 y, Spritesheet* sprite, u8 spriteid, u8 flip) {
	if (x <= -sprite->width || y <= -sprite->height || x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
	
	i16 x_source = 0; // clip left
	if (x < 0) { x_source = -x; x = 0; }	
	i16 y_source = 0; // clip top
	if (y < 0) { y_source = -y; y = 0; }
	i16 w_stop = sprite->width - x_source; // clip right
	if (x + sprite->width >= LCD_WIDTH) { w_stop -= x + sprite->width - LCD_WIDTH - 1; }
	i16 h_stop = sprite->height - y_source; // clip bottom
	if (y + sprite->height >= LCD_HEIGHT) { h_stop -= y + sprite->height - LCD_HEIGHT - 1; }

	int y_offset;
	int spr_offset = sprite->width * sprite->height * (spriteid % sprite->count);
	Color c;
	for (int j = 0; j < h_stop; j++) {
		if (flip & DRAW_MIRROR_V) y_offset = (sprite->height - j - y_source) * sprite->width;
		else y_offset = (j + y_source) * sprite->width;
		for (int i = 0; i < w_stop; i++) {
			if (flip & DRAW_MIRROR_H) c = sprite->bitmap[spr_offset + y_offset + (sprite->width - i - x_source)];
			else c = sprite->bitmap[spr_offset + y_offset + i + x_source];
			if (c != sprite->mask) draw_point(x + i, y + j, c);
		}
	}
}

void draw_line_local(i16 x0, i16 y0, i16 x1, i16 y1, Color color) {
	i16 dx =  abs(x1-x0);
	i16 sx = x0 < x1 ? 1 : -1;
	i16 dy = -abs(y1-y0);
	i16 sy = y0<y1 ? 1 : -1;
	i16 err = dx + dy;  /* error value e_xy */
	while (1) {   /* loop */
		draw_point(x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		i16 e2 = 2*err;
		if (e2 >= dy) { /* e_xy+e_x > 0 */
			err += dy;
			x0 += sx;
		}
		if (e2 <= dx) { /* e_xy+e_y < 0 */
			err += dx;
			y0 += sy;
		}
	}
}

void draw_circle_local(i16 xm, i16 ym, i16 r, Color color) {
	 i16 x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
	 do {
			draw_point(xm - x, ym + y, color); /*   I. Quadrant */
			draw_point(xm - y, ym - x, color); /*  II. Quadrant */
			draw_point(xm + x, ym - y, color); /* III. Quadrant */
			draw_point(xm + y, ym + x, color); /*  IV. Quadrant */
			r = err;
			if (r <= y) err += ++y * 2 + 1;           /* e_xy+e_y < 0 */
			if (r > x || err > y) err += ++x * 2 + 1; /* e_xy+e_x > 0 or no 2nd y-step */
	 } while (x < 0);
}

void draw_fill_circle_local(i16 xm, i16 ym, i16 r, Color color) {
	 i16 x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
	 do {
			draw_horizontal_line(xm + x, xm - x, ym - y, color);
			draw_horizontal_line(xm + x, xm - x, ym + y, color);
			r = err;
			if (r <= y) err += ++y * 2 + 1;           /* e_xy+e_y < 0 */
			if (r > x || err > y) err += ++x * 2 + 1; /* e_xy+e_x > 0 or no 2nd y-step */
	 } while (x < 0);
}

void draw_polygon_local(int n, float* points, Color color) {
	for (int i = 0; i < n - 2; i += 2) {
		draw_line(points[i], points[i + 1], points[i + 2], points[i + 3], color);
	}
}

void draw_fill_polygon_local(int n, float* points, Color color) {
	int polyCorners = n >> 1;
	int nodes, nodeX[n], pixelX, pixelY, i, j, swap;

	// determine bounding segment on Y axis
	int minY = MEM_HEIGHT, maxY = 0;
	for (int i = 0; i < n; i += 2) {
		if (points[i + 1] < minY) minY = points[i + 1];
		if (points[i + 1] > maxY) maxY = points[i + 1];
	}

	for (pixelY = minY; pixelY <= maxY; pixelY++) {

		//  Build a list of nodes.
		nodes = 0; j = polyCorners - 1;
		for (i = 0; i < polyCorners; i++) {
			int ii = i << 1;
			int jj = j << 1;
			if (points[ii + 1] < pixelY && points[jj + 1] >= pixelY || points[jj + 1] < pixelY && points[ii + 1] >= pixelY) {
				nodeX[nodes++] = (int) (points[ii] + (pixelY - points[ii + 1]) / (points[jj + 1] - points[ii + 1]) *(points[jj] - points[ii])); 
			}
			j = i; 
		}

		//  Sort the nodes, via a simple "Bubble" sort.
		i = 0;
		while (i < nodes - 1) {
			if (nodeX[i] > nodeX[i + 1]) {
				swap = nodeX[i]; nodeX[i] = nodeX[i + 1]; nodeX[i + 1] = swap; if (i) i--; 
			} else {
				i++; 
			}
		}

		//  Fill the pixels between node pairs.
		for (i = 0; i < nodes; i += 2) {
			if   (nodeX[i] >= LCD_WIDTH) break;
			if   (nodeX[i + 1] > 0 ) {
				if (nodeX[i] < 0 ) nodeX[i] = 0;
				if (nodeX[i + 1] >= LCD_WIDTH ) nodeX[i + 1] = LCD_WIDTH - 1;
				draw_horizontal_line(nodeX[i], nodeX[i + 1], pixelY, color);
			}
		}
	}
}

Color draw_color_add(Color c1, Color c2) {
	u8 r1=RED(c1), g1=GREEN(c1), b1=BLUE(c1);
	u8 r2=RED(c2), g2=GREEN(c2), b2=BLUE(c2);
	return RGB(r1 + r2, g1 + g2, b1 + b2);
}

Color draw_color_subtract(Color c1, Color c2) {
	u8 r1=RED(c1), g1=GREEN(c1), b1=BLUE(c1);
	u8 r2=RED(c2), g2=GREEN(c2), b2=BLUE(c2);
	return RGB(r1 - r2, g1 - g2, b1 - b2);
}

Color draw_color_mul(Color c, float factor) {
	u8 r=RED(c), g=GREEN(c), b=BLUE(c);
	return RGB((u8)(r * factor), (u8)(g * factor), (u8)(b * factor));
}

typedef struct {
	Color c1, c2;
	int x1, x2;
} span_t;

static void draw_span(span_t span, int y) {
	int xdiff = span.x2 - span.x1;
	if(xdiff == 0)
		return;

	Color colordiff = draw_color_subtract(span.c2, span.c1);

	float factor = 0.0f;
	float factorStep = 1.0f / (float)xdiff;

	// draw each pixel in the span
	for(int x = span.x1; x < span.x2; x++) {
		draw_point(x, y, draw_color_add(span.c1, draw_color_mul(colordiff, factor)));
		factor += factorStep;
	}
}

typedef struct {
	Color c1, c2;
	int x1, y1, x2, y2;
} edge_t;

static void draw_spans_between_edges(edge_t e1, edge_t e2) {
	// calculate difference between the y coordinates
	// of the first edge and return if 0
	float e1ydiff = (float)(e1.y2 - e1.y1);
	if(e1ydiff == 0.0f)
		return;

	// calculate difference between the y coordinates
	// of the second edge and return if 0
	float e2ydiff = (float)(e2.y2 - e2.y1);
	if(e2ydiff == 0.0f)
		return;

	// calculate differences between the x coordinates
	// and colors of the points of the edges
	float e1xdiff = (float)(e1.x2 - e1.x1);
	float e2xdiff = (float)(e2.x2 - e2.x1);
	Color e1colordiff = (e1.c2 - e1.c1);
	Color e2colordiff = (e2.c2 - e2.c1);

	// calculate factors to use for interpolation
	// with the edges and the step values to increase
	// them by after drawing each span
	float factor1 = (float)(e2.y1 - e1.y1) / e1ydiff;
	float factorStep1 = 1.0f / e1ydiff;
	float factor2 = 0.0f;
	float factorStep2 = 1.0f / e2ydiff;

	// loop through the lines between the edges and draw spans
	for(int y = e2.y1; y < e2.y2; y++) {
		// create and draw span
		span_t span = {e1.c1 + (e1colordiff * factor1),
							e1.x1 + (int)(e1xdiff * factor1),
							e2.c1 + (e2colordiff * factor2),
							e2.x1 + (int)(e2xdiff * factor2)};
		draw_span(span, y);

		// increase factors
		factor1 += factorStep1;
		factor2 += factorStep2;
	}
}

void draw_triangle_shaded_local(Color c1, float x1, float y1, Color c2, float x2, float y2, Color c3, float x3, float y3) {
	// create edges for the triangle
	edge_t edges[3] = {
		{c1, (int)x1, (int)y1, c2, (int)x2, (int)y2},
		{c2, (int)x2, (int)y2, c3, (int)x3, (int)y3},
		{c3, (int)x3, (int)y3, c1, (int)x1, (int)y1}
	};

	int maxLength = 0;
	int longEdge = 0;

	// find edge with the greatest length in the y axis
	for(int i = 0; i < 3; i++) {
		int length = edges[i].y2 - edges[i].y1;
		if(length > maxLength) {
			maxLength = length;
			longEdge = i;
		}
	}

	int shortEdge1 = (longEdge + 1) % 3;
	int shortEdge2 = (longEdge + 2) % 3;

	// draw spans between edges; the long edge can be drawn
	// with the shorter edges to draw the full triangle
	draw_spans_between_edges(edges[longEdge], edges[shortEdge1]);
	draw_spans_between_edges(edges[longEdge], edges[shortEdge2]);
}

// todo: blit, roto-scale, etc.

int draw_fifo_receiver(uint32_t message) {
	uint32_t x1, y1, c1, x2, y2, c2, x3, y3, c3;
	
	switch (message) {
		case FIFO_DRAW_CLEAR:
			draw_clear_local();
			return 1;

		case FIFO_DRAW_RECT:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			x2 = multicore_fifo_pop_blocking_inline();
			y2 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			draw_rect_local((i16)x1, (i16)y1, (i16)x2, (i16)y2, (Color)c1);
			return 1;

		case FIFO_DRAW_RECTFILL:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			x2 = multicore_fifo_pop_blocking_inline();
			y2 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			draw_fill_rect_local((i16)x1, (i16)y1, (i16)x2, (i16)y2, (Color)c1);
			return 1;

		case FIFO_DRAW_LINE:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			x2 = multicore_fifo_pop_blocking_inline();
			y2 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			draw_line_local((i16)x1, (i16)y1, (i16)x2, (i16)y2, (Color)c1);
			return 1;

		case FIFO_DRAW_CIRC:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			x2 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			draw_circle_local((i16)x1, (i16)y1, (i16)x2, (Color)c1);
			return 1;

		case FIFO_DRAW_CIRCFILL:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			x2 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			draw_fill_circle_local((i16)x1, (i16)y1, (i16)x2, (Color)c1);
			return 1;

		case FIFO_DRAW_POLY:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			draw_polygon_local((int)x1, (float*)y1, (Color)c1);
			free((float*)y1);
			return 1;

		case FIFO_DRAW_POLYFILL:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			draw_fill_polygon_local((int)x1, (float*)y1, (Color)c1);
			free((float*)y1);
			return 1;

		case FIFO_DRAW_TRI:
			c1 = multicore_fifo_pop_blocking_inline();
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			c2 = multicore_fifo_pop_blocking_inline();
			x2 = multicore_fifo_pop_blocking_inline();
			y2 = multicore_fifo_pop_blocking_inline();
			c3 = multicore_fifo_pop_blocking_inline();
			x3 = multicore_fifo_pop_blocking_inline();
			y3 = multicore_fifo_pop_blocking_inline();
			draw_triangle_shaded_local((Color)c1, (float)x1, (float)y1, (Color)c2, (float)x2, (float)y2, (Color)c3, (float)x3, (float)y3);
			return 1;

		case FIFO_DRAW_SPRITE:
			x1 = multicore_fifo_pop_blocking_inline();
			y1 = multicore_fifo_pop_blocking_inline();
			c1 = multicore_fifo_pop_blocking_inline();
			x2 = multicore_fifo_pop_blocking_inline();
			y2 = multicore_fifo_pop_blocking_inline();
			draw_sprite_local((i16)x1, (i16)y1, (Spritesheet*)c1, (u8)x2, (u8)y2);
			return 1;

		default:
			return 0;
	}
}
