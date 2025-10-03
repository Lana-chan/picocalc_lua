#pragma once

#include <stdbool.h>
#include "types.h"

#define TERM_WIDTH 53 // 320 / font width
#define TERM_HEIGHT 40 // 320 / font height

#define ANSI_STACK_SIZE 16

#define DEFAULT_FG 15
#define DEFAULT_BG 0

#define HISTORY_MAX 32
typedef struct {
	char* buffer[HISTORY_MAX];
	int current;
} history_t;

void stdio_picocalc_init();
void stdio_picocalc_deinit();
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
void term_write(const char* text);
void term_blit(const char* text, const char* fg, const char* bg);
int term_readline(char* prompt, char* buffer, int max_length, history_t* history);
