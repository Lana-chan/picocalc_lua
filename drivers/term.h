#pragma once

#define TERM_WIDTH 53 // 320 / font width
#define TERM_HEIGHT 40 // 320 / font height

void stdio_picocalc_init();
void stdio_picocalc_deinit();
int term_readline(char* prompt, char* buffer, int max_length);
char** term_get_history();
