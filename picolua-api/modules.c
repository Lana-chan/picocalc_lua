#include <lua.h>
#include <lauxlib.h>

#include "sys.h"
#include "fs.h"
#include "draw.h"
#include "term.h"

#include "../submodules/kilo/kilo.h"

static int l_fs_editor(lua_State* L) {
	const char* path = luaL_optstring(L, 1, "");

	start_editor(L, path);

	return 0;
}

void modules_register_wrappers(lua_State *L) {
	luaL_requiref(L, "sys", &luaopen_sys, 1);
	luaL_requiref(L, "keys", &luaopen_keys, 1);
	luaL_requiref(L, "fs", &luaopen_fs, 1);
	luaL_requiref(L, "term", &luaopen_term, 1);
	luaL_requiref(L, "draw", &luaopen_draw, 1);
	luaL_requiref(L, "colors", &luaopen_color, 1);

	lua_register(L, "edit", l_fs_editor);
}