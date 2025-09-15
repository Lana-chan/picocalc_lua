#pragma once

void fs_init();
int fs_mount();
int fs_unmount();
char* fs_readfile(const char* path);