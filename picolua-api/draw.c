#include <stdlib.h>
#include <malloc.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../drivers/lcd.h"
#include "../drivers/draw.h"
#include "../drivers/fs.h"
#include "modules.h"

#define spritesheet "Spritesheet"

static inline Spritesheet* l_checksprite(lua_State *L, int n) {
	return (Spritesheet*)luaL_checkudata(L, n, spritesheet);
}

Spritesheet* l_newsprite(lua_State *L) {
	Spritesheet* sprite = lua_newuserdata(L, sizeof(Spritesheet));
	luaL_getmetatable(L, spritesheet);
	lua_setmetatable(L, -2);
	return sprite;
}

static int l_draw_text(lua_State* L) {
	int x = luaL_checknumber(L, 1);
	int y = luaL_checknumber(L, 2);
	u16 fg = luaL_checkinteger(L, 3);
	u16 bg = luaL_checkinteger(L, 4);
	size_t len;
	const char* text = luaL_checklstring(L, 5, &len);
	lcd_draw_text(x, y, fg, bg, text, len);
	return 0;
}

static int l_draw_clear(lua_State* L) {
	draw_clear();
	return 0;
}

static int l_draw_buffer_enable(lua_State* L) {
	int mode = 0;
	if (lua_type(L, 1) == LUA_TBOOLEAN) {
		if (lua_toboolean(L, 1)) mode = 1;
	} else {
		mode = luaL_checkinteger(L, 1);
	}

	if (mode == LCD_BUFFERMODE_RAM) {
		lua_getglobal(L, "collectgarbage");
		lua_pcall(L, 0, 1, 0);
	}

	lua_pushboolean(L, lcd_buffer_enable(mode));
	return 1;
}

static int l_draw_buffer_blit(lua_State* L) {
	lcd_buffer_blit();
	return 0;
}

static int l_draw_color_from_rgb(lua_State* L) {
	u8 r = luaL_checkinteger(L, 1);
	u8 g = luaL_checkinteger(L, 2);
	u8 b = luaL_checkinteger(L, 3);
	Color color = RGB(r, g, b);
	lua_pushinteger(L, color);  
	return 1;
}

static int l_draw_color_to_rgb(lua_State* L) {
	Color c = luaL_checkinteger(L, 1);
	u8 r = RED(c), g = GREEN(c), b = BLUE(c);
	lua_pushinteger(L, r);
	lua_pushinteger(L, g);
	lua_pushinteger(L, b);
	return 3;
}

static int l_draw_color_from_hsv(lua_State* L) {
	u8 h = luaL_checkinteger(L, 1);
	u8 s = luaL_checkinteger(L, 2);
	u8 v = luaL_checkinteger(L, 3);
	Color color = draw_color_from_hsv(h, s, v);
	lua_pushinteger(L, color);  
	return 1;
}

static int l_draw_color_to_hsv(lua_State* L) {
	Color c = luaL_checkinteger(L, 1);
	u8 h, s, v;
	draw_color_to_hsv(c, &h, &s, &v);
	lua_pushinteger(L, h);
	lua_pushinteger(L, s);
	lua_pushinteger(L, v);
	return 3;
}

static int l_draw_color_add(lua_State* L) {
	Color c1 = luaL_checkinteger(L, 1);
	Color c2 = luaL_checkinteger(L, 2);
	Color c = draw_color_add(c1, c2);
	lua_pushinteger(L, c);
	return 1;
}

static int l_draw_color_subtract(lua_State* L) {
	Color c1 = luaL_checkinteger(L, 1);
	Color c2 = luaL_checkinteger(L, 2);
	Color c = draw_color_subtract(c1, c2);
	lua_pushinteger(L, c);
	return 1;
}

static int l_draw_color_mul(lua_State* L) {
	Color c = luaL_checkinteger(L, 1);
	float factor = luaL_checknumber(L, 2);
	Color result = draw_color_mul(c, factor);
	lua_pushinteger(L, result);
	return 1;
}

static int l_draw_point(lua_State* L) {
	i16 x = luaL_checknumber(L, 1);
	i16 y = luaL_checknumber(L, 2);
	Color color = luaL_checkinteger(L, 3);
	draw_point(x, y, color);
	return 0;
}

static int l_draw_rect(lua_State* L) {
	i16 x = luaL_checknumber(L, 1);
	i16 y = luaL_checknumber(L, 2);
	i16 width = luaL_checknumber(L, 3);
	i16 height = luaL_checknumber(L, 4);
	Color color = luaL_checkinteger(L, 5);
	draw_rect(x, y, width, height, color);
	return 0;
}

static int l_draw_fill_rect(lua_State* L) {
	i16 x = luaL_checknumber(L, 1);
	i16 y = luaL_checknumber(L, 2);
	i16 width = luaL_checknumber(L, 3);
	i16 height = luaL_checknumber(L, 4);
	Color color = luaL_checkinteger(L, 5);
	draw_fill_rect(x, y, width, height, color);
	return 0;
}

static int l_draw_line(lua_State* L) {
	i16 x1 = luaL_checknumber(L, 1);
	i16 y1 = luaL_checknumber(L, 2);
	i16 x2 = luaL_checknumber(L, 3);
	i16 y2 = luaL_checknumber(L, 4);
	Color color = luaL_checkinteger(L, 5);
	draw_line(x1, y1, x2, y2, color);
	return 0;
}

static int l_draw_circle(lua_State* L) {
	i16 xm = luaL_checknumber(L, 1);
	i16 ym = luaL_checknumber(L, 2);
	i16 r = luaL_checknumber(L, 3);
	Color color = luaL_checkinteger(L, 4);
	draw_circle(xm, ym, r, color);
	return 0;
}

static int l_draw_fill_circle(lua_State* L) {
	i16 xm = luaL_checknumber(L, 1);
	i16 ym = luaL_checknumber(L, 2);
	i16 r = luaL_checknumber(L, 3);
	Color color = luaL_checkinteger(L, 4);
	draw_fill_circle(xm, ym, r, color);
	return 0;
}

static int l_draw_polygon(lua_State* L) {
	if (!lua_istable(L, 1)) return luaL_error(L, "Expected table for argument #1 (points)");
	int num_coords = luaL_len(L, 1);
	if (num_coords % 2 != 0) return luaL_error(L, "Points table must contain an even number of values (x, y pairs)");
	int n = num_coords;
	float *points = (float *) malloc(num_coords * sizeof(float));
	if (!points) return luaL_error(L, "Memory allocation failed");

	for (int i = 0; i < num_coords; ++i) {
		lua_rawgeti(L, 1, i + 1);
		if (!lua_isnumber(L, -1)) {
			free(points);
			return luaL_error(L, "Non-numeric value in points table at index %d", i + 1);
		}
		points[i] = lua_tonumber(L, -1);
		lua_pop(L, 1); 
	}

	Color color = luaL_checkinteger(L, 2);
	draw_polygon(n, points, color);
	// we trust draw_polygon to eventually free the array
	// because it will be needed on the second core
	//free(points);
	return 0;
}

static int l_draw_fill_polygon(lua_State* L) {
	if (!lua_istable(L, 1)) return luaL_error(L, "Expected table for argument #1 (points)");
	int num_coords = luaL_len(L, 1);
	if (num_coords % 2 != 0) return luaL_error(L, "Points table must contain an even number of values (x, y pairs)");
	int n = num_coords / 2;
	float *points = (float *) malloc(num_coords * sizeof(float));
	if (!points) return luaL_error(L, "Memory allocation failed");

	for (int i = 0; i < num_coords; ++i) {
		lua_rawgeti(L, 1, i + 1);
		if (!lua_isnumber(L, -1)) {
			free(points);
			return luaL_error(L, "Non-numeric value in points table at index %d", i + 1);
		}
		points[i] = lua_tonumber(L, -1);
		lua_pop(L, 1); 
	}

	Color color = luaL_checkinteger(L, 2);
	draw_fill_polygon(n, points, color);
	//free(points);
	return 0;
}

static int l_draw_triangle_shaded(lua_State* L) {
	Color c1 = luaL_checkinteger(L, 1);
	float x1 = luaL_checknumber(L, 2);
	float y1 = luaL_checknumber(L, 3);
	Color c2 = luaL_checkinteger(L, 4);
	float x2 = luaL_checknumber(L, 5);
	float y2 = luaL_checknumber(L, 6);
	Color c3 = luaL_checkinteger(L, 7);
	float x3 = luaL_checknumber(L, 8);
	float y3 = luaL_checknumber(L, 9);
	draw_triangle_shaded(c1, x1, y1, c2, x2, y2, c3, x3, y3);
	return 0;
}

/* BMP loading */

typedef struct __attribute__((__packed__)) {
	unsigned char fileMarker1;
	unsigned char fileMarker2;
	unsigned int   bfSize;
	uint16_t unused1;
	uint16_t unused2;
	unsigned int   imageDataOffset;
} FILEHEADER;

typedef struct __attribute__((__packed__)) {
	unsigned int   biSize;
	int            width;
	int            height;
	uint16_t planes;
	uint16_t bitPix;
	unsigned int   biCompression;
	unsigned int   biSizeImage;
	int            biXPelsPerMeter;
	int            biYPelsPerMeter;
	unsigned int   biClrUsed;
	unsigned int   biClrImportant;
} INFOHEADER;

typedef struct __attribute__((__packed__)) {
	unsigned char  b;
	unsigned char  g;
	unsigned char  r;
} IMAGE;

static int l_draw_load_spritesheet(lua_State* L) {
	const char* filename = luaL_checkstring(L, 1);
	int spr_width = luaL_optinteger(L, 2, 0);
	int spr_height = luaL_optinteger(L, 3, 0);
	Color spr_mask = luaL_optinteger(L, 4, RGB(255, 0, 255));
	FIL file;
	FILEHEADER fh;
	INFOHEADER ih;
	
	FRESULT res = f_open(&file, filename, FA_READ);
	if (res != FR_OK) return luaL_error(L, fs_error_strings[res]);
	res = f_read(&file, &fh, sizeof(FILEHEADER), NULL);
	if (res != FR_OK) return luaL_error(L, fs_error_strings[res]);
	res = f_read(&file, &ih, sizeof(INFOHEADER), NULL);
	if (res != FR_OK) return luaL_error(L, fs_error_strings[res]);

	if (!(fh.fileMarker1 == 'B' && fh.fileMarker2 == 'M' && (ih.bitPix == 24 || ih.bitPix == 32))) {
		f_close(&file);
		return luaL_error(L, "invalid or unsupported BMP format");
	}

	if (spr_width == 0) spr_width = ih.width;
	if (spr_height == 0) spr_height = ih.height;

	if (ih.width % spr_width != 0 || ih.height % spr_height != 0) {
		f_close(&file);
		return luaL_error(L, "image size is not a multiple of sprite size");
	}

	Spritesheet* sprite = l_newsprite(L);
	sprite->width = spr_width;
	sprite->height = spr_height;
	sprite->count = (ih.width * ih.height) / (spr_width * spr_height);
	sprite->mask = spr_mask;
	sprite->bitmap = (Color*)malloc(ih.width * ih.height * sizeof(Color));

	if (!sprite->bitmap) {
		f_close(&file);
		return luaL_error(L, "failed to allocate space for bitmap");
	}

	int pix_bytes = ih.bitPix / 8;
	int atlas_width = ih.width / sprite->width; // width of bmp in sprites
	int padding_bytes = 0;
	if (pix_bytes == 3) padding_bytes = 4 - (ih.width % 4);
	u8 b, g, r;
	int bmp_x, bmp_y;
	for (int i = 0; i < sprite->count; i++) {
		for (int y = 0; y < sprite->height; y++) {
			// find one sequential sprite's row in bmp
			bmp_x = (i % atlas_width) * sprite->width;
			bmp_y = ih.height - 1 - ((i / atlas_width) * (sprite->height) + y);
			f_lseek(&file, fh.imageDataOffset + (bmp_x + bmp_y * ih.width) * pix_bytes + bmp_y * padding_bytes);
			for (int x = 0; x < sprite->width; x++) {
				f_read(&file, &b, 1, NULL);
				f_read(&file, &g, 1, NULL);
				f_read(&file, &r, 1, NULL);
				if (pix_bytes == 4) f_read(&file, NULL, 1, NULL); // 32bit alpha
				sprite->bitmap[x + (y * sprite->width) + (i * sprite->width * sprite->height)] = RGB(r, g, b);
			}
		}
	}

	f_close(&file);

	return 1;
}

static int l_draw_free_spritesheet(lua_State* L) {
	Spritesheet* sprite = l_checksprite(L, 1);

	if(sprite->bitmap) free(sprite->bitmap);
	sprite->bitmap = NULL;

	return 0;
}

static int l_draw_blitsprite(lua_State* L) {
	i16 x = luaL_checkinteger(L, 1);
	i16 y = luaL_checkinteger(L, 2);
	Spritesheet* sprite = l_checksprite(L, 3);
	u8 spriteid = luaL_optinteger(L, 4, 0);
	u8 flip = luaL_optinteger(L, 5, 0);

	draw_sprite(x, y, sprite, spriteid, flip);

	return 0;
}

int luaopen_draw(lua_State *L) {
	static const luaL_Reg drawlib_f [] = {
		{"text", l_draw_text},
		{"clear", l_draw_clear},
		{"point", l_draw_point},
		{"rect", l_draw_rect},
		{"rectFill", l_draw_fill_rect},
		{"line", l_draw_line},
		{"circle", l_draw_circle},
		{"circleFill", l_draw_fill_circle},
		{"polygon", l_draw_polygon},
		{"polygonFill", l_draw_fill_polygon},
		{"triangle", l_draw_triangle_shaded},
		{"enableBuffer", l_draw_buffer_enable},
		{"blitBuffer", l_draw_buffer_blit},
		{"loadSprites", l_draw_load_spritesheet},
		{"blitSprite", l_draw_blitsprite},
		{NULL, NULL}
	};
	
	static const luaL_Reg drawlib_spritemeta[] = {
		{"__index", NULL},
		{"__gc", l_draw_free_spritesheet},
		{"__close", l_draw_free_spritesheet},
		{NULL, NULL}
	};
	
	luaL_newlib(L, drawlib_f);

	luaL_newmetatable(L, spritesheet);
	luaL_setfuncs(L, drawlib_spritemeta, 0);
	lua_setfield(L, -2, spritesheet);

	lua_pushintegerconstant(L, "flip_horizontal", DRAW_MIRROR_H);
	lua_pushintegerconstant(L, "flip_vertical", DRAW_MIRROR_V);
	lua_pushintegerconstant(L, "flip_both", DRAW_MIRROR_H | DRAW_MIRROR_V);
	
	return 1;
}

int luaopen_color(lua_State *L) {
	static const luaL_Reg colorlib_f [] = {
		{"fromRGB", l_draw_color_from_rgb},
		{"toRGB", l_draw_color_to_rgb},
		{"fromHSV", l_draw_color_from_hsv},
		{"toHSV", l_draw_color_to_hsv},
		{"add", l_draw_color_add},
		{"subtract", l_draw_color_subtract},
		{"multiply", l_draw_color_mul},
		{NULL, NULL}
	};
	
	luaL_newlib(L, colorlib_f);

	lua_pushintegerconstant(L, "white", RGB(240, 240, 240));
	lua_pushintegerconstant(L, "orange", RGB(242, 178, 51));
	lua_pushintegerconstant(L, "magenta", RGB(229, 127, 216));
	lua_pushintegerconstant(L, "lightBlue", RGB(153, 178, 242));
	lua_pushintegerconstant(L, "yellow", RGB(222, 222, 108));
	lua_pushintegerconstant(L, "lime", RGB(127, 204, 25));
	lua_pushintegerconstant(L, "pink", RGB(242, 178, 204));
	lua_pushintegerconstant(L, "gray", RGB(76, 76, 76));
	lua_pushintegerconstant(L, "lightGray", RGB(153, 153, 153));
	lua_pushintegerconstant(L, "cyan", RGB(76, 153, 178));
	lua_pushintegerconstant(L, "purple", RGB(178, 102, 229));
	lua_pushintegerconstant(L, "blue", RGB(51, 102, 204));
	lua_pushintegerconstant(L, "brown", RGB(127, 102, 76));
	lua_pushintegerconstant(L, "green", RGB(87, 166, 78));
	lua_pushintegerconstant(L, "red", RGB(204, 76, 76));
	lua_pushintegerconstant(L, "black", RGB(17, 17, 17));
	
	return 1;
}