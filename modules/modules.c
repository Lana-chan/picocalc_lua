#include <lua.h>

#include "sys.h"
#include "fs.h"
#include "draw.h"

void _link() {} // TODO: handle those properly
void _unlink() {}

void modules_register_wrappers(lua_State *L) {
  sys_register_wrapper(L);
  fs_register_wrapper(L);
  draw_register_wrapper(L);
}