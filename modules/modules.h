#pragma once

#include <lua.h>

#define lua_table_register(L,n,f) (lua_pushcfunction(L, (f)), lua_setfield(L, -2, (n)))

void modules_register_wrappers(lua_State *L);