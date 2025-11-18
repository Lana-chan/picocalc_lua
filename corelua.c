#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "drivers/lcd.h"
#include "drivers/draw.h"
#include "drivers/keyboard.h"
#include "drivers/term.h"
#include "drivers/fs.h"
#include "drivers/sound.h"
#include "drivers/multicore.h"
#include "picolua-api/sys.h"

#include "picolua-api/modules.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <ldebug.h>

#define PROMPT "lua> "
#define STARTUP_FILE "main.lua"
#define STARTUP_FONT "default.fnt"

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
bool should_run_timer = false;

static void keyboard_interrupt() {
	should_interrupt = true;
}

bool sys_timer_callback(repeating_timer_t *rt) {
	should_run_timer = true;
	return true;
}

void lua_interrupt(lua_State *L, lua_Debug *ar) {
  if (should_interrupt) {
		should_interrupt = false;
		luaG_runerror(L, "interrupted");
	}
	else if (should_run_timer) {
		should_run_timer = false;
		sys_timer_execute(L);
	}
}

void lua_pre_script(lua_State *L) {
	keyboard_set_interrupt_callback(keyboard_interrupt);
}

void lua_post_script(lua_State *L) {
	sound_stopall();
	sys_stoptimer(L);
	lcd_buffer_enable(0);
	lua_getglobal(L, "collectgarbage");
	lua_pcall(L, 0, 0, 0);
	keyboard_set_interrupt_callback(NULL);
}

void lua_bootscreen() {
	term_clear();
	printf("    \x1b[93mPicoCalc Lua\x1b[m %s\n", GIT_DESC);
	printf("    %u bytes free\n", get_free_memory());
	draw_fill_circle(2*font.glyph_width - font.glyph_height / 2, font.glyph_height * 1.125, font.glyph_height / 1.6, RGB(100,100,255));
	draw_fill_circle(2*font.glyph_width + font.glyph_height * 0.375, font.glyph_height * 0.25, font.glyph_height / 4, RGB(100,100,255));
	draw_fill_circle(2*font.glyph_width - font.glyph_height / 4, font.glyph_height * 0.875, font.glyph_height / 4, RGB(255,255,255));
}

void lua_main() {
	int status;
	size_t len;
	char ch;

	L = luaL_newstate();
	// i keep fussing with this value but as low as 10 doesn't seem to give much or any performance downgrade
	// lower makes interrupt more responsive
	// instant interrupt (calling luaG_runerror inside keyboard_interrupt) causes hang
	lua_sethook(L, lua_interrupt, LUA_MASKCOUNT, 50);
	luaL_openlibs(L);

	modules_register_wrappers(L);
	int mounted = fs_mount();

	if (mounted && fs_exists(STARTUP_FONT)) lcd_load_font(STARTUP_FONT);

	lua_bootscreen();
	if (mounted) {
		printf("\x1b[92mSD card mounted!\x1b[m\n");

		if (fs_exists(STARTUP_FILE)) {
			lua_pre_script(L);
			luaL_dofile(L, STARTUP_FILE);
			lua_post_script(L);
		}
	}

	while (1) {
		char line[256];
		keyboard_flush();
		lcd_buffer_enable(0);
		term_set_blinking_cursor(true);
		int size = term_readline(PROMPT, line, 256, &term_history);
		term_set_blinking_cursor(false);

		lua_settop(L, 0);
		//int num_results = 1;
		
		lua_pre_script(L);

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
		
		lua_post_script(L);
	}

	lua_close(L);
}