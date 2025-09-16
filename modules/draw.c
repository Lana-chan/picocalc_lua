#include <stdlib.h>
#include <malloc.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../drivers/lcd.h"
#include "../drivers/draw.h"

static int l_draw_text(lua_State* L) {
	int x = luaL_checknumber(L, 1);
	int y = luaL_checknumber(L, 2);
	u16 fg = luaL_checkinteger(L, 3);
	u16 bg = luaL_checkinteger(L, 4);
	const char* text = luaL_checkstring(L, 5);
	lcd_draw_text(x, y, fg, bg, text);
	return 0;
}

static int l_draw_clear(lua_State* L) {
	draw_clear();
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
	i32 x = luaL_checknumber(L, 1);
	i32 y = luaL_checknumber(L, 2);
	Color color = luaL_checkinteger(L, 3);
	draw_point(x, y, color);
	return 0;
}

static int l_draw_rect(lua_State* L) {
	i32 x = luaL_checknumber(L, 1);
	i32 y = luaL_checknumber(L, 2);
	i32 width = luaL_checknumber(L, 3);
	i32 height = luaL_checknumber(L, 4);
	Color color = luaL_checkinteger(L, 5);
	draw_rect(x, y, width, height, color);
	return 0;
}

static int l_draw_fill_rect(lua_State* L) {
	i32 x = luaL_checknumber(L, 1);
	i32 y = luaL_checknumber(L, 2);
	i32 width = luaL_checknumber(L, 3);
	i32 height = luaL_checknumber(L, 4);
	Color color = luaL_checkinteger(L, 5);
	draw_fill_rect(x, y, width, height, color);
	return 0;
}

static int l_draw_line(lua_State* L) {
	i32 x1 = luaL_checknumber(L, 1);
	i32 y1 = luaL_checknumber(L, 2);
	i32 x2 = luaL_checknumber(L, 3);
	i32 y2 = luaL_checknumber(L, 4);
	Color color = luaL_checkinteger(L, 5);
	draw_line(x1, y1, x2, y2, color);
	return 0;
}

static int l_draw_circle(lua_State* L) {
	i32 xm = luaL_checknumber(L, 1);
	i32 ym = luaL_checknumber(L, 2);
	i32 r = luaL_checknumber(L, 3);
	Color color = luaL_checkinteger(L, 4);
	draw_circle(xm, ym, r, color);
	return 0;
}

static int l_draw_fill_circle(lua_State* L) {
	i32 xm = luaL_checknumber(L, 1);
	i32 ym = luaL_checknumber(L, 2);
	i32 r = luaL_checknumber(L, 3);
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
	free(points);
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
	free(points);
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

int luaopen_draw(lua_State *L) {
	static const luaL_Reg drawlib_f [] = {
		{"text", l_draw_text},
		{"clear", l_draw_clear},
		{"color_from_rgb", l_draw_color_from_rgb},
		{"color_to_rgb", l_draw_color_to_rgb},
		{"color_from_hsv", l_draw_color_from_hsv},
		{"color_to_hsv", l_draw_color_to_hsv},
		{"color_add", l_draw_color_add},
		{"color_subtract", l_draw_color_subtract},
		{"color_mul", l_draw_color_mul},
		{"point", l_draw_point},
		{"rect", l_draw_rect},
		{"fill_rect", l_draw_fill_rect},
		{"line", l_draw_line},
		{"circle", l_draw_circle},
		{"fill_circle", l_draw_fill_circle},
		{"polygon", l_draw_polygon},
		{"fill_polygon", l_draw_fill_polygon},
		{"triangle_shaded", l_draw_triangle_shaded},
		{NULL, NULL}
	};
	
	luaL_newlib(L, drawlib_f);
	
	return 1;
}