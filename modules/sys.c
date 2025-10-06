#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>

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
	lua_pushfstring(L, "%c", event.code);
	return 3;
}

static int l_keyboard_isprint(lua_State* L) {
	const char* c = luaL_checkstring(L, 1);
	lua_pushboolean(L, isprint(c[0]));
	return 1;
}

static int l_keyboard_wait(lua_State* L) {
	bool nomod = luaL_opt(L, lua_toboolean, 1, false);
	bool onlypressed = luaL_opt(L, lua_toboolean, 2, false);
	input_event_t event = keyboard_wait_ex(nomod, onlypressed);
	lua_pushinteger(L, event.state);
	lua_pushinteger(L, event.modifiers);
	lua_pushfstring(L, "%c", event.code);
	return 3;
}

static int l_keyboard_state(lua_State* L) {
	const char* code = luaL_checkstring(L, 1);
	lua_pushboolean(L, keyboard_getstate(code[0]) == KEY_STATE_PRESSED);
	return 1;
}

static int l_keyboard_flush(lua_State* L) {
	keyboard_flush();
	return 0;
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
		{"flush", l_keyboard_flush},
		{"getState", l_keyboard_state},
		{"isPrintable", l_keyboard_isprint},
		{NULL, NULL}
	};

	luaL_newlib(L, keyslib_f);

	lua_pushcharconstant(L, "alt",        KEY_ALT);
	lua_pushcharconstant(L, "leftShift",  KEY_LSHIFT);
	lua_pushcharconstant(L, "rightShift", KEY_RSHIFT);
	lua_pushcharconstant(L, "control",    KEY_CONTROL);
	lua_pushcharconstant(L, "esc",        KEY_ESC);
	lua_pushcharconstant(L, "left",       KEY_LEFT);
	lua_pushcharconstant(L, "up",         KEY_UP);
	lua_pushcharconstant(L, "down",       KEY_DOWN);
	lua_pushcharconstant(L, "right",      KEY_RIGHT);
	lua_pushcharconstant(L, "backspace",  KEY_BACKSPACE);
	lua_pushcharconstant(L, "enter",      KEY_ENTER);
	lua_pushcharconstant(L, "capslock",   KEY_CAPSLOCK);
	lua_pushcharconstant(L, "pause",      KEY_PAUSE);
	lua_pushcharconstant(L, "home",       KEY_HOME);
	lua_pushcharconstant(L, "delete",     KEY_DELETE);
	lua_pushcharconstant(L, "end",        KEY_END);
	lua_pushcharconstant(L, "pageUp",     KEY_PAGEUP);
	lua_pushcharconstant(L, "pageDown",   KEY_PAGEDOWN);
	lua_pushcharconstant(L, "tab",        KEY_TAB);

	lua_newtable(L);
	lua_pushintegerconstant(L, "idle",     KEY_STATE_IDLE);
	lua_pushintegerconstant(L, "pressed",  KEY_STATE_PRESSED);
	lua_pushintegerconstant(L, "released", KEY_STATE_RELEASED);
	lua_pushintegerconstant(L, "hold",     KEY_STATE_HOLD);
	lua_pushintegerconstant(L, "longHold", KEY_STATE_LONG_HOLD);
	lua_setfield(L, -2, "states");

	lua_newtable(L);
	lua_pushintegerconstant(L, "ctrl",       MOD_CONTROL);
	lua_pushintegerconstant(L, "alt",        MOD_ALT);
	lua_pushintegerconstant(L, "shift",      MOD_SHIFT);
	lua_pushintegerconstant(L, "leftShift",  MOD_LSHIFT);
	lua_pushintegerconstant(L, "rightShift", MOD_RSHIFT);
	lua_setfield(L, -2, "modifiers");

	return 1;
}