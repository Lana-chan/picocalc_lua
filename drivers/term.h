#pragma once

#define TERM_WIDTH 53 // 320 / font width
#define TERM_HEIGHT 40 // 320 / font height

#define ANSI_STACK_SIZE 16

#define DEFAULT_FG 15
#define DEFAULT_BG 0

void stdio_picocalc_init();
void stdio_picocalc_deinit();
void term_clear();
int term_readline(char* prompt, char* buffer, int max_length);
char** term_get_history();
