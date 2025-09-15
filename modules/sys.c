#include <stdlib.h>
#include <malloc.h>

#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "modules.h"
#include "../drivers/keyboard.h"

uint32_t get_total_memory() {
  extern char __StackLimit, __bss_end__;
  return &__StackLimit  - &__bss_end__;
}

uint32_t get_free_memory() {
  struct mallinfo m = mallinfo();
  return get_total_memory() - m.uordblks;
}

static int l_get_total_memory(lua_State* L) {
  lua_pushinteger(L, get_total_memory());
  return 1;
}

static int l_get_free_memory(lua_State* L) {
  lua_pushinteger(L, get_free_memory());
  return 1;
}

static int l_reset(lua_State *L) {
  watchdog_reboot(0, 0, 0);
  return 0;
}

static int l_bootsel(lua_State *L) {
  reset_usb_boot(0, 0);
  return 0;
}

static int l_set_output(lua_State *L) {
  int pin = lua_tointeger(L, 1);
  int output = lua_toboolean(L, 2);

  gpio_init(pin);
  gpio_set_dir(pin, output);
  return 0;
}

static int l_set_pin(lua_State *L) {
  int pin = lua_tointeger(L, 1);
  int state = lua_toboolean(L, 2);

  gpio_put(pin, state == 1);
  return 0;
}

static int l_get_pin(lua_State *L) {
  int pin = lua_tointeger(L, 1);
  int state = gpio_get(pin);

  lua_pushboolean(L, state);
  return 1;
}

static int l_keyboard_poll(lua_State* L) {
  input_event_t event = keyboard_poll();
  lua_pushinteger(L, event.state);
  lua_pushinteger(L, event.modifiers);
  lua_pushinteger(L, event.code);
  return 3;
}

static int l_keyboard_wait(lua_State* L) {
  input_event_t event = keyboard_wait();
  lua_pushinteger(L, event.state);
  lua_pushinteger(L, event.modifiers);
  lua_pushinteger(L, event.code);
  return 3;
}

static int l_get_battery(lua_State* L) {
  int battery = get_battery();
  lua_pushinteger(L, battery);
  return 1;
}

int luaopen_sys(lua_State *L) {
  static const luaL_Reg syslib_f [] = {
    {"get_total_memory", l_get_total_memory},
    {"get_free_memory", l_get_free_memory},
    {"reset", l_reset},
    {"bootsel", l_bootsel},
    {"set_output", l_set_output},
    {"set_pin", l_set_pin},
    {"get_pin", l_get_pin},
    {"keyboard_wait", l_keyboard_wait},
    {"keyboard_poll", l_keyboard_poll},
    {"get_battery", l_get_battery},
    {NULL, NULL}
  };
  
  luaL_newlib(L, syslib_f);
  
  return 1;
}