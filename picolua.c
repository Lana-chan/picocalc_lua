/* 
 * minimal pico-sdk lua example
 *
 * Copyright 2023 Jeremy Grosser <jeremy@synack.me>
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "drivers/lcd.h"
#include "drivers/draw.h"
#include "drivers/keyboard.h"
#include "drivers/term.h"
#include "drivers/fs.h"
#include "drivers/multicore.h"
#include "modules/sys.h"

#include "modules/modules.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define PROMPT "lua> "

extern const char* GIT_DESC;

static void l_print (lua_State *L) {
	int n = lua_gettop(L);
	if (n > 0) {  /* any result to be printed? */
		luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
		lua_getglobal(L, "print");
		lua_insert(L, 1);
		if (lua_pcall(L, n, 0, 0) != LUA_OK) lua_writestringerror("error calling 'print' (%s)", lua_tostring(L, -1));
	}
}

void core1() {
	lcd_init();
	multicore_fifo_drain();

	while (true) {
		handle_multicore_fifo();
	}
}

void check_interrupt(lua_State *L, lua_Debug *ar) {
	input_event_t event = keyboard_poll_ex(false);
	if (event.code == KEY_BREAK) {
		luaL_error(L, "error: interrupted");
	} else {
		// prevent dropped input
		last_event = event;
	}
}

int main() {
	lua_State *L;
	int status;
	size_t len;
	char ch;

	multicore_launch_core1(core1);
	sleep_ms(200);

	//stdio_init_all();
	keyboard_init();
	stdio_picocalc_init(); 
	fs_init();

	L = luaL_newstate();
	lua_sethook(L, check_interrupt, LUA_MASKCOUNT, 10);
	luaL_openlibs(L);

	modules_register_wrappers(L);
	fs_mount();

	printf("    \x1b[93mPicoCalc Lua\x1b[m %s\n", GIT_DESC);
	printf("    %u bytes free\n", get_free_memory());
	// TODO: scale with font size
	draw_fifo_fill_circle(7, 9, 5, RGB(100,100,255));
	draw_fifo_fill_circle(14, 2, 2, RGB(100,100,255));
	draw_fifo_fill_circle(9, 8, 2, RGB(255,255,255));

	char* script = fs_readfile("main.lua");
	if (script != NULL) {
		if (luaL_loadbuffer(L, script, strlen(script), "=main.lua") != LUA_OK || lua_pcall(L, 0, 0, 0) != LUA_OK) {
			const char *msg = lua_tostring(L, -1);
			lua_writestringerror("%s\n", msg);
		}
		free(script);
	}

	while (1) {
		char line[256];
		int size = term_readline(PROMPT, line, 256);

		lua_settop(L, 0);
		//int num_results = 1;
		const char *retline = lua_pushfstring(L, "return %s;", line);
		status = luaL_loadbuffer(L, retline, strlen(retline), "=stdin");
		if (status != LUA_OK) {
			lua_pop(L, 2);
			status = luaL_loadbuffer(L, line, strlen(line), "=stdin");
			//num_results = 0;
		} else lua_remove(L, -2);
		if (status != LUA_OK) {
			const char *msg = lua_tostring(L, -1);
			lua_writestringerror("%s\n", msg);
		} else {
			status = lua_pcall(L, 0, LUA_MULTRET, 0);
			if(status != LUA_OK) {
				const char *msg = lua_tostring(L, -1);
				lua_writestringerror("%s\n", msg);
			} else {
				printf("\x1b[96m");
				l_print(L);
				printf("\x1b[m");
			}
		}
	}

	lua_close(L);
	return 0;
}
