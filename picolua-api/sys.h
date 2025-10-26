#pragma once

#include <lua.h>
#include <stdbool.h>

uint32_t get_total_memory();
uint32_t get_free_memory();
uint16_t get_system_mhz();
bool set_system_mhz(uint16_t clk);
int luaopen_sys(lua_State *L);
int luaopen_keys(lua_State *L);
