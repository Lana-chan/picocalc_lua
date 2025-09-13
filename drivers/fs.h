#pragma once

void fs_init();
int fs_mount();
char* fs_readfile(const char* path);