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
		{"totalMemory", l_get_total_memory},
		{"freeMemory", l_get_free_memory},
		{"reset", l_reset},
		{"bootsel", l_bootsel},
		{"setOutput", l_set_output},
		{"setPin", l_set_pin},
		{"getPin", l_get_pin},
		{"battery", l_get_battery},
		{NULL, NULL}
	};
	
	luaL_newlib(L, syslib_f);
	
	return 1;
}

int luaopen_keys(lua_State *L) {
	static const luaL_Reg keyslib_f [] = {
		{"wait", l_keyboard_wait},
		{"poll", l_keyboard_poll},
		{NULL, NULL}
	};

	luaL_newlib(L, keyslib_f);

	lua_pushintegerconstant(L, "alt",        0xA1);
	lua_pushintegerconstant(L, "leftShift",  0xA2);
	lua_pushintegerconstant(L, "rightShift", 0xA3);
	lua_pushintegerconstant(L, "control",    0xA5);
	lua_pushintegerconstant(L, "esc",        0xB1);
	lua_pushintegerconstant(L, "left",       0xB4);
	lua_pushintegerconstant(L, "up",         0xB5);
	lua_pushintegerconstant(L, "down",       0xB6);
	lua_pushintegerconstant(L, "right",      0xB7);
	lua_pushintegerconstant(L, "backspace",  '\b');
	lua_pushintegerconstant(L, "enter",      '\n');
	lua_pushintegerconstant(L, "capslock",   0xC1);
	lua_pushintegerconstant(L, "pause",      0xD0);
	lua_pushintegerconstant(L, "home",       0xD2);
	lua_pushintegerconstant(L, "delete",     0xD4);
	lua_pushintegerconstant(L, "end",        0xD5);
	lua_pushintegerconstant(L, "pageUp",     0xD6);
	lua_pushintegerconstant(L, "pageDown",   0xD7);
	lua_pushintegerconstant(L, "tab",        0x09);

	return 1;
}