#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "../drivers/sound.h"

#include "modules.h"

int l_sound_playnote(lua_State *L) {
	int ch = luaL_checkinteger(L, 1);
	int note = luaL_checkinteger(L, 2);
	sound_playnote(ch, note);
	return 0;
}

int l_sound_playpitch(lua_State *L) {
	int ch = luaL_checkinteger(L, 1);
	float pitch = luaL_checknumber(L, 2);
	sound_playpitch(ch, pitch);
	return 0;
}

int l_sound_stop(lua_State* L) {
	int ch = luaL_checkinteger(L, 1);
	sound_stop(ch);
	return 0;
}

int l_sound_off(lua_State* L) {
	int ch = luaL_checkinteger(L, 1);
	sound_off(ch);
	return 0;
}

int l_sound_setup(lua_State* L) {
	int ch = luaL_checkinteger(L, 1);
	int wave = luaL_optinteger(L, 2, 0);
	float volume = luaL_optnumber(L, 3, 1);
	float attack = luaL_optnumber(L, 4, 0);
	float decay = luaL_optnumber(L, 5, 1000);
	float sustain = luaL_optnumber(L, 6, 0);
	float release = luaL_optnumber(L, 7, 0);
	sound_setup(ch, wave, volume, attack, decay, sustain, release);
	return 0;
}

int luaopen_sound(lua_State *L) {
	static const luaL_Reg soundlib_f [] = {
		{"play", l_sound_playnote},
		{"playpitch", l_sound_playpitch},
		{"stop", l_sound_stop},
		{"off", l_sound_off},
		{"setup", l_sound_setup},
		{NULL, NULL}
	};
	
	luaL_newlib(L, soundlib_f);
	
	return 1;
}