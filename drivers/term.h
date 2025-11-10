#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "types.h"

#define ANSI_STACK_SIZE 16

#define DEFAULT_FG 15
#define DEFAULT_BG 0

#define HISTORY_MAX 32
typedef struct {
	char* buffer[HISTORY_MAX];
	int current;
} history_t;

#define CURSOR_BLINK_MS 300

void stdio_picocalc_init();
void stdio_picocalc_deinit();
void stdio_picocalc_out_chars(const char *buf, int length);
void term_clear();
void term_erase_line(int y);
int term_get_width();
int term_get_height();
int term_get_x();
int term_get_y();
void term_set_pos(int x, int y);
u16 term_get_fg();
u16 term_get_bg();
void term_set_fg(u16 color);
void term_set_bg(u16 color);
bool term_get_blinking_cursor();
void term_set_blinking_cursor(bool enabled);
void term_blit(const char* text, const char* fg, const char* bg);
int term_readline(char* prompt, char* buffer, int max_length, history_t* history);
