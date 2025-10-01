#pragma once

#include <lua.h>

uint32_t get_total_memory();
uint32_t get_free_memory();
int luaopen_sys(lua_State *L);
int luaopen_keys(lua_State *L);
