#pragma once

#include <lua.h>
#include <lauxlib.h>

int start_editor(lua_State* L, const char* filename);