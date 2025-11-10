#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <string.h>
#include "../drivers/sound.h"

#include "modules.h"

#define instrument "Instrument"

static inline instrument_t* l_checkinst(lua_State *L, int n) {
	void *ud = luaL_checkudata(L, n, instrument);
	luaL_argcheck(L, ud != NULL, n, "'Instrument' expected");
	return (instrument_t*)ud;
}

instrument_t* l_newinst(lua_State *L) {
	instrument_t* inst = lua_newuserdata(L, sizeof(instrument_t));
	luaL_getmetatable(L, instrument);
	lua_setmetatable(L, -2);
	inst->wave = 5;
	inst->volume = 1;
	inst->attack = 0;
	inst->decay = 0;
	inst->sustain = 1;
	inst->release = 0;
	inst->table_mode = TABLE_SINGLE;
	inst->table_start = 0;
	inst->table_end = 0;
	inst->table_playrate = 0;
	return inst;
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

int l_sound_setvolume(lua_State *L) {
	int ch = luaL_checkinteger(L, 1);
	float volume = luaL_checknumber(L, 2);
	bool relative = false;
	if (lua_type(L, 3) == LUA_TBOOLEAN) {
		if (lua_toboolean(L, 3)) relative = true;
	}
	
	sound_setvolume(ch, volume, relative);
	return 0;
}

int l_sound_setpitch(lua_State *L) {
	int ch = luaL_checkinteger(L, 1);
	float pitch = luaL_checknumber(L, 2);
	bool relative = false;
	if (lua_type(L, 3) == LUA_TBOOLEAN) {
		if (lua_toboolean(L, 3)) relative = true;
	}
	
	sound_setpitch(ch, pitch, relative);
	return 0;
}

int l_sound_stopall(lua_State *L) {
	sound_stopall();
	return 0;
}

int l_sound_setup(lua_State* L) {
	instrument_t* source = luaL_testudata(L, 1, instrument);

	instrument_t *inst = lua_newuserdata(L, sizeof(instrument_t));
	luaL_getmetatable(L, instrument);
	lua_setmetatable(L, -2);

	if (source) {
		// copy constructor
		*inst = *source;
	} else {
		// settings by arguments
		int wave = luaL_optinteger(L, 1, 0);
		if (inst->wave >= sound_getsamplecount()) luaL_argerror(L, 1, "wavetable outside bounds");
		float volume = luaL_optnumber(L, 2, 1);
		uint16_t attack = luaL_optinteger(L, 3, 0);
		uint16_t decay = luaL_optinteger(L, 4, 1000);
		float sustain = luaL_optnumber(L, 5, 0);
		uint16_t release = luaL_optinteger(L, 6, 0);
		enum TABLE_MODE table_mode = luaL_optinteger(L, 7, 0);
		if (table_mode >= TABLE_LOOP) luaL_argerror(L, 7, "wavetable mode outside bounds");
		uint16_t table_start = luaL_optinteger(L, 8, 0);
		int16_t table_playrate = luaL_optinteger(L, 9, 500);
		uint16_t table_len;
		sound_getsampledata(wave, &table_len, NULL);
		uint16_t table_end = luaL_optinteger(L, 10, table_len-1);

		inst->wave = wave;
		inst->volume = volume;
		inst->attack = attack;
		inst->decay = decay;
		inst->sustain = sustain;
		inst->release = release;
		inst->table_mode = table_mode;
		inst->table_start = table_start;
		inst->table_end = table_end;
		inst->table_playrate = table_playrate;
	}
	
	return 1;
}

int l_sound_instget(lua_State *L) {
	instrument_t* inst = l_checkinst(L, 1);
	const char* key = luaL_checkstring(L, 2);

	if (strcmp(key, "wave") == 0) { lua_pushinteger(L, inst->wave); }
	else if (strcmp(key, "volume") == 0) { lua_pushnumber(L, inst->volume); }
	else if (strcmp(key, "attack") == 0) { lua_pushinteger(L, inst->attack); }
	else if (strcmp(key, "decay") == 0) { lua_pushinteger(L, inst->decay); }
	else if (strcmp(key, "sustain") == 0) { lua_pushnumber(L, inst->sustain); }
	else if (strcmp(key, "release") == 0) { lua_pushinteger(L, inst->release); }
	else if (strcmp(key, "table_mode") == 0) { lua_pushinteger(L, inst->table_mode); }
	else if (strcmp(key, "table_start") == 0) { lua_pushinteger(L, inst->table_start); }
	else if (strcmp(key, "table_end") == 0) { lua_pushinteger(L, inst->table_end); }
	else if (strcmp(key, "table_playrate") == 0) { lua_pushinteger(L, inst->table_playrate); }
	else { lua_pushnil(L); }

	return 1;
}

int l_sound_instset(lua_State *L) {
	instrument_t* inst = l_checkinst(L, 1);
	const char* key = luaL_checkstring(L, 2);

	if (strcmp(key, "wave") == 0) {
		int wave = luaL_checkinteger(L, 3);
		if (wave >= sound_getsamplecount()) luaL_argerror(L, 3, "wavetable outside bounds");
		inst->wave = wave;
	}
	else if (strcmp(key, "volume") == 0) { float volume = luaL_checknumber(L, 3); inst->volume = volume; }
	else if (strcmp(key, "attack") == 0) { uint16_t attack = luaL_checkinteger(L, 3); inst->attack = attack; }
	else if (strcmp(key, "decay") == 0) { uint16_t decay = luaL_checkinteger(L, 3); inst->decay = decay; }
	else if (strcmp(key, "sustain") == 0) { float sustain = luaL_checknumber(L, 3); inst->sustain = sustain; }
	else if (strcmp(key, "release") == 0) { uint16_t release = luaL_checkinteger(L, 3); inst->release = release; }
	else if (strcmp(key, "table_mode") == 0) {
		int table_mode = luaL_checkinteger(L, 3);
		if (table_mode >= TABLE_LOOP) luaL_argerror(L, 3, "wavetable mode outside bounds");
		inst->table_mode = table_mode;
	}
	else if (strcmp(key, "table_start") == 0) { uint16_t table_start = luaL_checkinteger(L, 3); inst->table_start = table_start; }
	else if (strcmp(key, "table_end") == 0) { uint16_t table_end = luaL_checkinteger(L, 3); inst->table_end = table_end; }
	else if (strcmp(key, "table_playrate") == 0) { int16_t table_playrate = luaL_checkinteger(L, 3); inst->table_playrate = table_playrate; }

	return 0;
}

int luaopen_sound(lua_State *L) {
	static const luaL_Reg soundlib_inst[] = {
		{"__index", l_sound_instget},
		{"__newindex", l_sound_instset},
		{NULL, NULL}
	};

	static const luaL_Reg soundlib_funcs [] = {
		{"play", l_sound_playnote},
		{"playPitch", l_sound_playpitch},
		{"stop", l_sound_stop},
		{"stopAll", l_sound_stopall},
		{"off", l_sound_off},
		{"volume", l_sound_setvolume},
		{"pitch", l_sound_setpitch},
		{"instrument", l_sound_setup},
		{NULL, NULL}
	};
	
	luaL_newlib(L, soundlib_funcs);

	luaL_newmetatable(L, instrument);
	luaL_setfuncs(L, soundlib_inst, 0);

	lua_setfield(L, -2, instrument);

	lua_newtable(L);
	lua_pushintegerconstant(L, "single", TABLE_SINGLE);
	lua_pushintegerconstant(L, "oneShot", TABLE_ONESHOT);
	lua_pushintegerconstant(L, "pingPong", TABLE_PINGPONG);
	lua_pushintegerconstant(L, "loop", TABLE_LOOP);
	lua_setfield(L, -2, "tableModes");
	
	// instrument defaults
	lua_newtable(L);
	instrument_t *inst;

	inst = l_newinst(L);
	inst->wave = 0;
	inst->volume = 0.6;
	inst->attack = 0;
	inst->decay = 400;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_LOOP;
	inst->table_start = 32;
	inst->table_end = 32;
	inst->table_playrate = 0;
	lua_setfield(L, -2, "square");

	inst = l_newinst(L);
	inst->wave = 0;
	inst->volume = 0.6;
	inst->attack = 0;
	inst->decay = 400;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_LOOP;
	inst->table_start = 10;
	inst->table_end = 10;
	inst->table_playrate = 0;
	lua_setfield(L, -2, "thin");

	inst = l_newinst(L);
	inst->wave = 1;
	inst->volume = 0.6;
	inst->attack = 0;
	inst->decay = 500;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_PINGPONG;
	inst->table_start = 0;
	inst->table_end = 63;
	inst->table_playrate = 600;
	lua_setfield(L, -2, "string");
	
	inst = l_newinst(L);
	inst->wave = 3;
	inst->volume = 0.4;
	inst->attack = 50;
	inst->decay = 700;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_PINGPONG;
	inst->table_start = 48;
	inst->table_end = 0;
	inst->table_playrate = -600;
	lua_setfield(L, -2, "violin");
	
	inst = l_newinst(L);
	inst->wave = 4;
	inst->volume = 0.4;
	inst->attack = 50;
	inst->decay = 700;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_PINGPONG;
	inst->table_start = 0;
	inst->table_end = 63;
	inst->table_playrate = 600;
	lua_setfield(L, -2, "voice");
	
	inst = l_newinst(L);
	inst->wave = 4;
	inst->volume = 0.7;
	inst->attack = 0;
	inst->decay = 400;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_PINGPONG;
	inst->table_start = 63;
	inst->table_end = 0;
	inst->table_playrate = -600;
	lua_setfield(L, -2, "pluck");
	
	inst = l_newinst(L);
	inst->wave = 0;
	inst->volume = 0.6;
	inst->attack = 0;
	inst->decay = 400;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_LOOP;
	inst->table_start = 16;
	inst->table_end = 60;
	inst->table_playrate = 600;
	lua_setfield(L, -2, "pulse");

	inst = l_newinst(L);
	inst->wave = 1;
	inst->volume = 1;
	inst->attack = 0;
	inst->decay = 200;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_PINGPONG;
	inst->table_start = 0;
	inst->table_end = 63;
	inst->table_playrate = 50;
	lua_setfield(L, -2, "warp");
	
	inst = l_newinst(L);
	inst->wave = 1;
	inst->volume = 1;
	inst->attack = 0;
	inst->decay = 300;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_PINGPONG;
	inst->table_start = 56;
	inst->table_end = 56;
	inst->table_playrate = 0;
	lua_setfield(L, -2, "sine");

	inst = l_newinst(L);
	inst->wave = 2;
	inst->volume = 1;
	inst->attack = 0;
	inst->decay = 300;
	inst->sustain = 0;
	inst->release = 0;
	inst->table_mode = TABLE_PINGPONG;
	inst->table_start = 0;
	inst->table_end = 0;
	inst->table_playrate = 0;
	lua_setfield(L, -2, "triangle");

	lua_setfield(L, -2, "presets");

	// drum defaults
	lua_newtable(L);

	inst = l_newinst(L);
	inst->wave = 5;
	lua_setfield(L, -2, "kick1");

	inst = l_newinst(L);
	inst->wave = 6;
	lua_setfield(L, -2, "kick2");

	inst = l_newinst(L);
	inst->wave = 7;
	lua_setfield(L, -2, "snare1");

	inst = l_newinst(L);
	inst->wave = 8;
	lua_setfield(L, -2, "snare2");

	inst = l_newinst(L);
	inst->wave = 9;
	lua_setfield(L, -2, "hihat");

	inst = l_newinst(L);
	inst->wave = 10;
	lua_setfield(L, -2, "tom");

	inst = l_newinst(L);
	inst->wave = 11;
	lua_setfield(L, -2, "cowbell");

	lua_setfield(L, -2, "drums");

	return 1;
}