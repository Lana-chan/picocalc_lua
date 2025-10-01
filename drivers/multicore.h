#pragma once

#include <stdbool.h>
#include <stdint.h>

void handle_multicore_fifo();

void multicore_fifo_push_string(const char* string);
char* multicore_fifo_pop_string();