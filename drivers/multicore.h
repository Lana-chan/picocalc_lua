#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FIFO_TIMEOUT 150000 // 150ms

void handle_multicore_fifo();

void multicore_fifo_push(uint32_t value);
bool multicore_fifo_pop(uint32_t* value);
void multicore_fifo_push_string(const char* string);
char* multicore_fifo_pop_string();