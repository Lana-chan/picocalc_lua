#pragma once

#include <lua.h>

#define lua_pushintegerconstant(L, n, v) (lua_pushinteger(L, v), lua_setfield(L, -2, n))
#define lua_pushcharconstant(L, n, v) (lua_pushfstring(L, "%c", v), lua_setfield(L, -2, n))

void modules_register_wrappers(lua_State *L);