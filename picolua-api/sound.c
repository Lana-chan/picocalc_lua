#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "../drivers/sound.h"

#include "modules.h"

#define instrument "Instrument"

static inline instrument_t* l_checkinst(lua_State *L, int n) {
	void *ud = luaL_checkudata(L, n, instrument);
	luaL_argcheck(L, ud != NULL, n, "'Instrument' expected");
	return (instrument_t*)ud;
}

int l_sound_playnote(lua_State *L) {
	int ch = luaL_checkinteger(L, 1);
	int note = luaL_checkinteger(L, 2);
	instrument_t* inst = l_checkinst(L, 3);
	sound_playnote(ch, note, inst);
	return 0;
}

int l_sound_playpitch(lua_State *L) {
	int ch = luaL_checkinteger(L, 1);
	float pitch = luaL_checknumber(L, 2);
	instrument_t* inst = l_checkinst(L, 3);
	sound_playpitch(ch, pitch, inst);
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
	int wave = luaL_optinteger(L, 1, 0);
	float volume = luaL_optnumber(L, 2, 1);
	float attack = luaL_optnumber(L, 3, 0);
	float decay = luaL_optnumber(L, 4, 1000);
	float sustain = luaL_optnumber(L, 5, 0);
	float release = luaL_optnumber(L, 6, 0);
	enum TABLE_MODE table_mode = luaL_optinteger(L, 7, 0);
	uint16_t table_pos = luaL_optinteger(L, 8, 0);
	int16_t table_playtarget = luaL_optinteger(L, 9, 0);

	instrument_t *inst = lua_newuserdata(L, sizeof(instrument_t));
	luaL_getmetatable(L, instrument);
	lua_setmetatable(L, -2);

	inst->wave = wave;
	inst->volume = volume;
	inst->attack = attack;
	inst->decay = decay;
	inst->sustain = sustain;
	inst->release = release;
	inst->table_mode = table_mode;
	inst->table_pos = table_pos;
	inst->table_playtarget = table_playtarget;
	
	return 1;
}

int luaopen_sound(lua_State *L) {
	static const luaL_Reg soundlib_inst[] = {
		{NULL, NULL}
	};

	static const luaL_Reg soundlib_funcs [] = {
		{"play", l_sound_playnote},
		{"playpitch", l_sound_playpitch},
		{"stop", l_sound_stop},
		{"off", l_sound_off},
		{"instrument", l_sound_setup},
		{NULL, NULL}
	};
	
	luaL_newlib(L, soundlib_funcs);

	luaL_newmetatable(L, instrument);
	luaL_setfuncs(L, soundlib_inst, 0);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	lua_setfield(L, -2, instrument);
	
	return 1;
}