#include "term.h"

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/stdio/driver.h"
#include <stdlib.h>
#include <string.h>

#include "font.h"
#include "lcd.h"
#include "keyboard.h"

stdio_driver_t stdio_picocalc;

void stdio_picocalc_init() {
	stdio_set_driver_enabled(&stdio_picocalc, true);
}

void stdio_picocalc_deinit() {
	stdio_set_driver_enabled(&stdio_picocalc, false);
}

static unsigned short palette[8] = {
	RGB(0, 0, 0),       // 0 black
	RGB(255, 0, 0),     // 1 red
	RGB(0, 255, 0),     // 2 green
	RGB(255, 255, 0),   // 3 yellow
	RGB(0, 0, 255),     // 4 blue
	RGB(255, 0, 255),   // 5 magenta
	RGB(0, 255, 255),     // 6 cyan
	RGB(255, 255, 255), // 7 white
};

enum {
	AnsiNone,
	AnsiEscape,
	AnsiBracket,
};

typedef struct {
	int state;
	int x, y;
	int fg, bg;
	char stack[16];
	int stack_size;
} ansi_t;

static ansi_t ansi = {
	.state=AnsiNone, .x=0, .y=0, .fg=7, .bg=0, .stack={0}, .stack_size=0
};

static int ansi_len_to_lcd_x(int len) {
	return ((ansi.x + len) % TERM_WIDTH) * GLYPH_WIDTH;
}

static int ansi_len_to_lcd_y(int len) {
	return (ansi.y + (len + ansi.x) / TERM_WIDTH) * GLYPH_HEIGHT;
}

static void erase_line(int y) {
	lcd_fill(palette[0], 0, y * GLYPH_HEIGHT, 320, GLYPH_HEIGHT);
}

static void draw_cursor(int x, int y) {
	lcd_fill(palette[7], x * GLYPH_WIDTH, y * GLYPH_HEIGHT, GLYPH_WIDTH, GLYPH_HEIGHT);
}

static void erase_cursor(int x, int y) {
	lcd_fill(palette[0], x * GLYPH_WIDTH, y * GLYPH_HEIGHT, GLYPH_WIDTH, GLYPH_HEIGHT);
}

static void out_char(char c) {
	erase_cursor(ansi.x, ansi.y);
	if (c == '\n') {
		ansi.x = 0;
		ansi.y += 1;
		erase_line(ansi.y);
	} else if (c == '\b') ansi.x -= 1;
	//else if (c == '\r') ansi.x = 0;
	else if (c == '\t') {
		lcd_draw_char(ansi.x * GLYPH_WIDTH, ansi.y * GLYPH_HEIGHT, palette[ansi.fg], palette[ansi.bg], ' ');
		ansi.x += 1;
	} else if (c >= 32 && c < 127) {
		lcd_draw_char(ansi.x * GLYPH_WIDTH, ansi.y * GLYPH_HEIGHT, palette[ansi.fg], palette[ansi.bg], c);
		ansi.x += 1;
	}
	if (ansi.x >= TERM_WIDTH) {
		ansi.x = 0;
		ansi.y += 1;
		erase_line(ansi.y);
	}
	if (ansi.y >= TERM_HEIGHT) {
		lcd_scroll((ansi.y - (TERM_HEIGHT - 1)) * GLYPH_HEIGHT);
	}
	draw_cursor(ansi.x, ansi.y);
}

static void stdio_picocalc_out_chars(const char *buf, int length) {
	while (length > 0) {
		if (ansi.state == AnsiNone) {
			if (*buf == 27) ansi.state = AnsiEscape;
			else if (*buf == '\t') {
				out_char(*buf); out_char(*buf); out_char(*buf); out_char(*buf);
			}
			else out_char(*buf);
		} else if (ansi.state == AnsiEscape) {
			if (*buf == '[') ansi.state = AnsiBracket;
			else ansi.state = AnsiNone;
			ansi.stack_size = 0;
		} else if (ansi.state == AnsiBracket) {
			ansi.stack[ansi.stack_size] = 0;
			int a = 0, b = 0, semi_column = 0;
			//lcd_printf(0, 39 * 8, 0xffff, 0, "buf = '%c' stack = %s         ", *buf, ansi.stack);
			switch (*buf) {
				case 'A': ansi.y -= atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor up
				case 'B': ansi.y += atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor down
				case 'C': ansi.x += atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor right
				case 'D': ansi.x -= atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor left
				case 'J': lcd_clear(); ansi.x = ansi.y = 0; ansi.state = AnsiNone; break; // erase display
				case 'm':
					if (ansi.stack_size == 0 || ansi.stack[0] == '0') {
						ansi.fg = 7;
						ansi.bg = 0;
					} else if (ansi.stack[1] >= '0' && ansi.stack[1] < '8') {
						if (ansi.stack[0] == '3') ansi.fg = ansi.stack[1] - '0';
						else if (ansi.stack[0] == '4') ansi.bg = ansi.stack[1] - '0';
					}
					ansi.state = AnsiNone;
					break;
				case 'H':
					a = atoi(ansi.stack);
					while (semi_column < ansi.stack_size && ansi.stack[semi_column] != ';') semi_column++;
					if (semi_column < ansi.stack_size) b = atoi(ansi.stack + semi_column);
					if (a > 0 && b > 0) {
						ansi.x = a - 1;
						ansi.y = b - 1;
					}
					ansi.state = AnsiNone;
					break;
				default:
					if (ansi.stack_size < 16) ansi.stack[ansi.stack_size++] = *buf;
					else ansi.state = AnsiNone;
			}
		}
		buf++;
		length--;
	}
}

static int stdio_picocalc_in_chars(char *buf, int length) {
	input_event_t event = keyboard_poll();
	if (event.state == KEY_STATE_PRESSED && event.code > 0) {
		if (event.modifiers & MOD_CONTROL && event.code >= 'a' && event.code < 'z') {
			*buf = event.code - 'a' + 1;
		} else {
			*buf = event.code;
		}
		//stdio_picocalc_out_chars(buf, 1);
		return 1;
	}
	return 0;
}

stdio_driver_t stdio_picocalc = {
	.out_chars = stdio_picocalc_out_chars,
	.in_chars = stdio_picocalc_in_chars,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
	.crlf_enabled = true,
#endif
};

static void term_erase_input(int size) {
	for (int i = 0; i < size + 1; i++) {
		int x = ansi_len_to_lcd_x(i), y = ansi_len_to_lcd_y(i);
		lcd_fill(palette[0], x, y, GLYPH_WIDTH, GLYPH_HEIGHT);
	}
}

static void term_draw_input(char* buffer, int size, int cursor) {
	for (int i = 0; i < size + 1; i++) {
		int x = ansi_len_to_lcd_x(i), y = ansi_len_to_lcd_y(i);
		if (i == cursor) lcd_fill(palette[7], x, y, GLYPH_WIDTH, GLYPH_HEIGHT);
		else if (i < size) lcd_draw_char(x, y, palette[7], palette[0], buffer[i]);
		else lcd_fill(palette[0], x, y, GLYPH_WIDTH, GLYPH_HEIGHT);
	}
	if (ansi.y + (size + ansi.x) / TERM_WIDTH >= TERM_HEIGHT) lcd_scroll((ansi.y + (size + ansi.x) / TERM_WIDTH - (TERM_HEIGHT-1)) * GLYPH_HEIGHT);
}

void term_clear() {
	ansi.x = ansi.y = 0;
	lcd_clear();
	lcd_scroll(0);
}

static char* history[32] = {0};
static int history_current;

static void history_save(int entry, char* text, int size) {
	if (entry >= 0 && entry < 32) {
		if (history[entry] != NULL) free(history[entry]);
		history[entry] = strndup(text, size);
	}
}

char** term_get_history() {
	return history;
}

int term_readline(char* prompt, char* buffer, int max_length) {
	int cursor = 0;
	int size = 0;

	history_current = 0;
	if (history[0] != NULL && history[0][0] != '\0') memmove(history + 1, history, 31 * sizeof(char*));
	
	buffer[size] = '\0';
	history[0] = strdup(buffer);

	stdio_picocalc_out_chars(prompt, strlen(prompt));

	while (true) {
		input_event_t event = keyboard_wait();
		if (event.state == KEY_STATE_PRESSED) {
			if (event.code == 'c' && event.modifiers & MOD_CONTROL) {
				term_erase_input(size);
				size = cursor = 0;
			} else if (event.code == 'l' && event.modifiers & MOD_CONTROL) {
				term_clear();
				stdio_picocalc_out_chars(prompt, strlen(prompt));
			} else if (event.code == KEY_ENTER) {
				term_draw_input(buffer, size, -1);
				buffer[size] = '\0';
				ansi.x += size % TERM_WIDTH;
				ansi.y += size / TERM_WIDTH;
				stdio_picocalc_out_chars("\n", 1);
				history_save(0, buffer, size);
				return size;
			} else if (event.code == KEY_UP && history_current < 31 && history[history_current + 1] != NULL) {
				term_erase_input(size);
				history_save(history_current, buffer, size);
				history_current++;
				size = cursor = strlen(history[history_current]);
				memcpy(buffer, history[history_current], size);
			} else if (event.code == KEY_DOWN && history_current > 0) {
				term_erase_input(size);
				history_save(history_current, buffer, size);
				history_current--;
				size = cursor = strlen(history[history_current]);
				memcpy(buffer, history[history_current], size);
			} else if (event.code == KEY_LEFT) {
				if (event.modifiers & MOD_CONTROL) {
					while (cursor > 0 && buffer[cursor] != ' ') cursor--;
				} else if (cursor > 0) cursor -= 1;
			} else if (event.code == KEY_RIGHT) {
				if (event.modifiers & MOD_CONTROL) {
					while (cursor < size && buffer[cursor] != ' ') cursor++;
				} else if (cursor < size) cursor += 1;
			} else if (event.code == KEY_HOME) {
				cursor = 0;
			} else if (event.code == KEY_END) {
				cursor = size;
			} else if (event.code == KEY_BACKSPACE && cursor > 0) {
				term_erase_input(size);
				cursor -= 1;
				size -= 1;
				memmove(buffer + cursor, buffer + cursor + 1, size - cursor);
			} else if (event.code >= 32 && event.code < 127) {
				if (size < max_length - 1) {
					if (cursor < size) {
						memmove(buffer + cursor + 1, buffer + cursor, size - cursor);
					}
					buffer[cursor] = event.code;
					size += 1;
					cursor += 1;
				}
			}
			term_draw_input(buffer, size, cursor);
		}
	}
	return 0;
}
