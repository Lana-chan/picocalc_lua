#pragma once

void register_fs_wrapper(lua_State* L);
void fs_init();
int fs_mount();
char* fs_readfile(const char* path);
