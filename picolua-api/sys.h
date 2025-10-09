#pragma once

#include <lua.h>
#include <stdbool.h>

uint32_t get_total_memory();
uint32_t get_free_memory();
uint32_t get_system_clk();
bool set_system_clk(uint32_t clk);
int luaopen_sys(lua_State *L);
int luaopen_keys(lua_State *L);
