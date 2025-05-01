/* 
 * minimal pico-sdk lua example
 *
 * Copyright 2023 Jeremy Grosser <jeremy@synack.me>
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>

#include "drivers/lcd.h"
#include "drivers/keyboard.h"
#include "drivers/term.h"
#include "lua_wrapper.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define PROMPT "lua> "

static void l_print (lua_State *L) {
  int n = lua_gettop(L);
  if (n > 0) {  /* any result to be printed? */
    luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
    lua_getglobal(L, "print");
    lua_insert(L, 1);
    if (lua_pcall(L, n, 0, 0) != LUA_OK) lua_writestringerror("error calling 'print' (%s)", lua_tostring(L, -1));
  }
}

void check_interrupt(lua_State *L, lua_Debug *ar) {
  input_event_t event = keyboard_poll();
  if (event.code == KEY_BREAK) luaL_error(L, "error: interrupted");
}

int main() {
  lua_State *L;
  //luaL_Buffer buf;
  int status;
  size_t len;
  char ch;

  //stdio_init_all();
  lcd_init();
  lcd_clear();
  lcd_on();
  keyboard_init();
  stdio_picocalc_init(); 

  L = luaL_newstate();
  luaL_openlibs(L);
  //luaL_buffinit(L, &buf);

  register_wrapper(L);
  lua_sethook(L, check_interrupt, LUA_MASKCOUNT, 1000);

  printf("Welcome to \x1b[33mpico lua\x1b[m\n");
  while (1) {
    char line[256];
    int size = term_readline(PROMPT, line, 256);

    lua_settop(L, 0);
    int num_results = 1;
    const char *retline = lua_pushfstring(L, "return %s;", line);
    status = luaL_loadbuffer(L, retline, strlen(retline), "=stdin");
    if (status != LUA_OK) {
      lua_pop(L, 2);
      status = luaL_loadbuffer(L, line, strlen(line), "=stdin");
      num_results = 0;
    } else lua_remove(L, -2);
    if (status != LUA_OK) {
      const char *msg = lua_tostring(L, -1);
      lua_writestringerror("%s\n", msg);
    } else {
      status = lua_pcall(L, 0, num_results, 0);
      if(status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        lua_writestringerror("%s\n", msg);
      } else {
        printf("\x1b[36m");
        l_print(L);
        printf("\x1b[m");
      }
    }
  }

  lua_close(L);
  return 0;
}
