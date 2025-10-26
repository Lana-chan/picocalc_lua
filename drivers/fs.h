#pragma once
#include <stdatomic.h>

extern volatile atomic_bool fs_needs_remount;

void fs_init();
int fs_mount();
int fs_unmount();
int fs_exists(const char* path);