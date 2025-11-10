#pragma once
#include <stdbool.h>

void lua_main();
bool sys_timer_callback(repeating_timer_t *rt);
void lua_pre_script(lua_State *L);
void lua_post_script(lua_State *L);