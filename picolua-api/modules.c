#include <lua.h>
#include <lauxlib.h>

#include "sys.h"
#include "fs.h"
#include "draw.h"
#include "term.h"
#include "sound.h"
#include "../corelua.h"
#include "../drivers/draw.h"
#include "../drivers/term.h"
#include "../drivers/lcd.h"

#include "../submodules/kilo/kilo.h"

static int l_fs_editor(lua_State* L) {
	const char* path = luaL_optstring(L, 1, "");

	start_editor(L, path);

	return 0;
}

static int l_credits(lua_State* L) {
	lua_bootscreen();

	printf("\n");
	
	printf("\x1b[93mOriginal RP2040 Lua port\n\x1b[96m  Jeremy Grosser\n");
	printf("\x1b[93mInitial PicoCalc conversion\n\x1b[96m  Benoit Favre (benob)\n");
	printf("\x1b[93mExtensions and additional drivers\n\x1b[96m  maple \"mavica\" syrup\n\x1b[37m  <maple@maple.pet>\n");
	printf("\x1b[93mSpecial thanks\n\x1b[96m  Willem van der Jagt (wkjagt)\n  Blair Leduc\n  James Churchill (pelrun)\n\n\x1b[m");

	printf("         In memory of\n");
	printf("Rebecca \"Burger Becky\" Heineman\n");
	printf("         (1963-2025)\n\n");

	int width = font.glyph_width * 4;
	int height = font.glyph_height * 2 / 5;
	int y = term_get_y() * font.glyph_height;
	Color flag[] = {
		RGB(88, 199, 242),
		RGB(237, 163, 178),
		RGB(247, 247, 247),
		RGB(237, 163, 178),
		RGB(88, 199, 242)
	};

	printf("     This software was brought\n");
	printf("     to you by a trans woman!\n\n");
	for (int i = 0; i < 5; i++) {
		draw_fill_rect(0, y + i * height, width, height, flag[i]);
	}

	return 0;
}

void modules_register_wrappers(lua_State *L) {
	luaL_requiref(L, "sys", &luaopen_sys, 1);
	luaL_requiref(L, "keys", &luaopen_keys, 1);
	luaL_requiref(L, "fs", &luaopen_fs, 1);
	luaL_requiref(L, "term", &luaopen_term, 1);
	luaL_requiref(L, "draw", &luaopen_draw, 1);
	luaL_requiref(L, "colors", &luaopen_color, 1);
	luaL_requiref(L, "sound", &luaopen_sound, 1);

	lua_register(L, "edit", l_fs_editor);
	lua_register(L, "credits", l_credits);
}