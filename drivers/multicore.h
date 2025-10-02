#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void multicore_fifo_push_string(const char* string);
size_t multicore_fifo_pop_string(char** string);

void multicore_main();