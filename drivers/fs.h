#pragma once

void fs_init();
int fs_mount();
int fs_unmount();
int fs_exists(const char* path);