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
#include "picolua-api/sys.h"

#include "picolua-api/modules.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <ldebug.h>

#define PROMPT "lua> "
#define STARTUP_FILE "main.lua"

extern const char* GIT_DESC;
lua_State *L;
history_t term_history = {{0}, 0};

static void l_print (lua_State *L) {
	int n = lua_gettop(L);
	if (n > 0) {  /* any result to be printed? */
		luaL_checkstack(L, LUA_MINSTACK, "too many results to print");
		lua_getglobal(L, "print");
		lua_insert(L, 1);
		if (lua_pcall(L, n, 0, 0) != LUA_OK) lua_writestringerror("error calling 'print' (%s)", lua_tostring(L, -1));
	}
}

bool should_interrupt = false;

static void keyboard_interrupt() {
	should_interrupt = true;
}

void lua_interrupt(lua_State *L, lua_Debug *ar) {
  if (should_interrupt) {
		should_interrupt = false;
		luaG_runerror(L, "interrupted");
	}
}

int main() {
	int status;
	size_t len;
	char ch;

	set_system_clk(150000);

	multicore_launch_core1(multicore_main);
	sleep_ms(200);

	//stdio_init_all();
	keyboard_init();
	stdio_picocalc_init(); 
	fs_init();

	L = luaL_newstate();
	// i keep fussing with this value but as low as 10 doesn't seem to give much or any performance downgrade
	// lower makes interrupt more responsive
	// instant interrupt (calling luaG_runerror inside keyboard_interrupt) causes hang
	lua_sethook(L, lua_interrupt, LUA_MASKCOUNT, 50);
	luaL_openlibs(L);

	modules_register_wrappers(L);
	int mounted = fs_mount();

	if (mounted) lcd_load_font("default.fnt");

	term_clear();
	printf("    \x1b[93mPicoCalc Lua\x1b[m %s\n", GIT_DESC);
	printf("    %u bytes free\n", get_free_memory());
	// TODO: scale with font size
	draw_fifo_fill_circle(7, 9, 5, RGB(100,100,255));
	draw_fifo_fill_circle(14, 2, 2, RGB(100,100,255));
	draw_fifo_fill_circle(9, 8, 2, RGB(255,255,255));
	if (mounted) {
		printf("\x1b[92mSD card mounted!\x1b[m\n");

		if (fs_exists(STARTUP_FILE)) {
			keyboard_set_interrupt_callback(keyboard_interrupt);
			luaL_dofile(L, STARTUP_FILE);
			keyboard_set_interrupt_callback(NULL);
		}
	}

	while (1) {
		char line[256];
		keyboard_flush();
		lcd_fifo_buffer_enable(false);
		term_set_blinking_cursor(true);
		int size = term_readline(PROMPT, line, 256, &term_history);
		term_set_blinking_cursor(false);

		lua_settop(L, 0);
		//int num_results = 1;
		keyboard_set_interrupt_callback(keyboard_interrupt);
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
		keyboard_set_interrupt_callback(NULL);
	}

	lua_close(L);
	return 0;
}
