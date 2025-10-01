#include <lua.h>
#include <lauxlib.h>

#include "sys.h"
#include "fs.h"
#include "draw.h"
#include "term.h"

void modules_register_wrappers(lua_State *L) {
	luaL_requiref(L, "sys", &luaopen_sys, 1);
	luaL_requiref(L, "keys", &luaopen_keys, 1);
	luaL_requiref(L, "fs", &luaopen_fs, 1);
	luaL_requiref(L, "term", &luaopen_term, 1);
	luaL_requiref(L, "draw", &luaopen_draw, 1);
	luaL_requiref(L, "colors", &luaopen_color, 1);
}