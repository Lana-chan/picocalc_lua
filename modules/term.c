#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../drivers/term.h"

static int l_term_clear(lua_State* L) {
	term_clear();
	return 0;
}

int luaopen_term(lua_State *L) {
	static const luaL_Reg termlib_f [] = {
		{"clear", l_term_clear},
		{NULL, NULL}
	};
	
	luaL_newlib(L, termlib_f);
	
	return 1;
}