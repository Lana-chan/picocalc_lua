#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../drivers/term.h"
#include "../drivers/lcd.h"
#include "types.h"

static int l_term_getCursorPos(lua_State* L) {
	lua_pushinteger(L, term_get_x());
	lua_pushinteger(L, term_get_y());
	return 2;
}

static int l_term_setCursorPos(lua_State* L) {
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	x = (x < 1 ? 1 : (x > font.term_width ? font.term_width : x));
	y = (y < 1 ? 1 : (y > font.term_height ? font.term_height : y));
	term_set_pos(x-1, y-1);
	return 0;
}

static int l_term_getCursorBlink(lua_State* L) {
	lua_pushboolean(L, term_get_blinking_cursor());
	return 1;
}

static int l_term_setCursorBlink(lua_State* L) {
	bool blink = lua_toboolean(L, 1);
	term_set_blinking_cursor(blink);
	return 0;
}

static int l_term_getSize(lua_State* L) {
	lua_pushinteger(L, font.term_width);
	lua_pushinteger(L, font.term_height);
	return 2;
}

static int l_term_clear(lua_State* L) {
	term_clear();
	return 0;
}

static int l_term_clearLine(lua_State* L) {
	term_erase_line(term_get_y());
	return 0;
}

static int l_term_getTextColor(lua_State* L) {
	lua_pushinteger(L, term_get_fg());
	return 1;
}

static int l_term_setTextColor(lua_State* L) {
	u16 color = luaL_checkinteger(L, 1);
	term_set_fg(color);
	return 0;
}

static int l_term_getBackgroundColor(lua_State* L) {
	lua_pushinteger(L, term_get_bg());
	return 1;
}

static int l_term_setBackgroundColor(lua_State* L) {
	u16 color = luaL_checkinteger(L, 1);
	term_set_bg(color);
	return 0;
}

static int l_term_write(lua_State* L) {
	size_t len;
	const char* text = luaL_checklstring(L, 1, &len);
	term_write(text, len);
	return 0;
}

static int l_term_blit(lua_State* L) {
	const char* text = luaL_checkstring(L, 1);
	const char* fg = luaL_checkstring(L, 2);
	const char* bg = luaL_checkstring(L, 3);
	term_blit(text, fg, bg);
	return 0;
}

static int l_term_loadfont(lua_State* L) {
	const char* filename = luaL_optstring(L, 1, NULL);
	lua_pushboolean(L, lcd_load_font(filename) == 0);
	return 1;
}

/*
-write(text)	Write text at the current cursor position, moving the cursor to the end of the text.
scroll(y)	Move all positions up (or down) by y pixels.
-getCursorPos()	Get the position of the cursor.
-setCursorPos(x, y)	Set the position of the cursor.
-getCursorBlink()	Checks if the cursor is currently blinking.
-setCursorBlink(blink)	Sets whether the cursor should be visible (and blinking) at the current cursor position.
-getSize()	Get the size of the terminal.
-clear()	Clears the terminal, filling it with the current background colour.
-clearLine()	Clears the line the cursor is currently on, filling it with the current background colour.
-getTextColor()	Return the colour that new text will be written as.
-setTextColor(colour)	Set the colour that new text will be written as.
-getBackgroundColor()	Return the current background colour.
-setBackgroundColor(colour)	Set the current background colour.
-blit(text, textColour, backgroundColour)	Writes text to the terminal with the specific foreground and background colours.
*/

int luaopen_term(lua_State *L) {
	static const luaL_Reg termlib_f [] = {
		{"getCursorPos", l_term_getCursorPos},
		{"setCursorPos", l_term_setCursorPos},
		{"getCursorBlink", l_term_getCursorBlink},
		{"setCursorBlink", l_term_setCursorBlink},
		{"getSize", l_term_getSize},
		{"clear", l_term_clear},
		{"clearLine", l_term_clearLine},
		{"getTextColor", l_term_getTextColor},
		{"setTextColor", l_term_setTextColor},
		{"getBackgroundColor", l_term_getBackgroundColor},
		{"setBackgroundColor", l_term_setBackgroundColor},
		{"write", l_term_write},
		{"blit", l_term_blit},
		{"loadFont", l_term_loadfont},
		{NULL, NULL}
	};
	
	luaL_newlib(L, termlib_f);
	
	return 1;
}