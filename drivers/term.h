#pragma once

void stdio_picocalc_init();
void stdio_picocalc_deinit();
int term_readline(char* prompt, char* buffer, int max_length);
char** term_get_history();
