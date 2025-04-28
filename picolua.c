/* 
 * minimal pico-sdk lua example
 *
 * Copyright 2023 Jeremy Grosser <jeremy@synack.me>
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>

#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include "drivers/lcd.h"
#include "drivers/keyboard.h"
#include "drivers/term.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define PROMPT "lua> "

// reset()
static int l_reset(lua_State *L) {
    watchdog_reboot(0, 0, 0);
    return 0;
}

// bootsel()
static int l_bootsel(lua_State *L) {
    reset_usb_boot(0, 0);
    return 0;
}

// set_output(pin, bool)
static int l_set_output(lua_State *L) {
    int pin = lua_tointeger(L, 1);
    int output = lua_toboolean(L, 2);
    lua_pop(L, 2);
    gpio_init(pin);
    gpio_set_dir(pin, output);
    return 0;
}

// set_pin(pin, bool)
static int l_set_pin(lua_State *L) {
    int pin = lua_tointeger(L, 1);
    int state = lua_toboolean(L, 2);
    lua_pop(L, 2);
    gpio_put(pin, state == 1);
    return 0;
}

// bool get_pin(pin)
static int l_get_pin(lua_State *L) {
    int pin = lua_tointeger(L, 1);
    int state = gpio_get(pin);
    lua_pop(L, 1);
    lua_pushboolean(L, state);
    return 1;
}

static void l_print (lua_State *L) {
  int n = lua_gettop(L);
  if (n > 0) {  /* any result to be printed? */
    luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
    lua_getglobal(L, "print");
    lua_insert(L, 1);
    if (lua_pcall(L, n, 0, 0) != LUA_OK) lua_writestringerror("error calling 'print' (%s)", lua_tostring(L, -1));
  }
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

  lua_register(L, "reset", l_reset);
  lua_register(L, "bootsel", l_bootsel);
  lua_register(L, "set_output", l_set_output);
  lua_register(L, "set_pin", l_set_pin);
  lua_register(L, "get_pin", l_get_pin);

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
      lua_writestringerror("parse error: %s\n", msg);
    } else {
      status = lua_pcall(L, 0, num_results, 0);
      if(status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        lua_writestringerror("execute error: %s\n", msg);
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
