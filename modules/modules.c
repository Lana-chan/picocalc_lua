#include <lua.h>
#include <lauxlib.h>

#include "sys.h"
#include "fs.h"
#include "draw.h"
#include "term.h"

void _link() {} // TODO: handle those properly
void _unlink() {}

void modules_register_wrappers(lua_State *L) {
  sys_register_wrapper(L);
  luaL_requiref(L, "fs", &luaopen_fs, 1);
  luaL_requiref(L, "term", &luaopen_term, 1);
  draw_register_wrapper(L);
}