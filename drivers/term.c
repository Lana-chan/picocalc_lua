#include "term.h"

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/stdio/driver.h"
#include <stdlib.h>
#include <string.h>

#include "lcd.h"
#include "keyboard.h"

stdio_driver_t stdio_picocalc;
static void (*chars_available_callback)(void *) = NULL;
static void *chars_available_param = NULL;
static repeating_timer_t cursor_timer;

static void set_chars_available_callback(void (*fn)(void *), void *param) {
	chars_available_callback = fn;
	chars_available_param = param;
}

// Function to be called when characters become available
void chars_available_notify(void) {
	if (chars_available_callback) {
		chars_available_callback(chars_available_param);
	}
}

void stdio_picocalc_init() {
	keyboard_set_key_available_callback(chars_available_notify);
	stdio_set_driver_enabled(&stdio_picocalc, true);
}

void stdio_picocalc_deinit() {
	stdio_set_driver_enabled(&stdio_picocalc, false);
}

const static unsigned short palette[16] = {
	RGB(  0,   0,   0), // 0 black
	RGB(194,  54,  33), // 1 red
	RGB( 37, 188,  36), // 2 green
	RGB(173, 173,  39), // 3 yellow
	RGB( 73,  46, 225), // 4 blue
	RGB(211,  56, 211), // 5 magenta
	RGB( 51, 187, 200), // 6 cyan
	RGB(203, 204, 205), // 7 white
	// high intensity
	RGB( 85,  85,  85), // 8 black
	RGB(255,  85,  85), // 9 red
	RGB( 85, 255,  85), // a green
	RGB(255, 255,  85), // b yellow
	RGB(85,   85, 255), // c blue
	RGB(255,  85, 255), // d magenta
	RGB( 85, 255, 255), // e cyan
	RGB(255, 255, 255), // f white
};

enum {
	AnsiNone,
	AnsiEscape,
	AnsiBracket,
};

typedef struct {
	int state;
	int x, y, cx, cy, len;
	u16 fg, bg;
	char stack[ANSI_STACK_SIZE];
	int stack_size;
	int scroll;
	bool cursor_enabled;
	bool cursor_visible;
	bool cursor_manual;
	bool c_inverse;
	bool c_bold;
} ansi_t;

static ansi_t ansi = {
	.state=AnsiNone,
	.x=0, .y=0,
	.fg=palette[DEFAULT_FG],
	.bg=palette[DEFAULT_BG],
	.stack={0},
	.stack_size=0,
	.cursor_enabled=false,
	.c_inverse=false,
	.c_bold=false,
};

static int ansi_len_to_lcd_x(int len) {
	return ((ansi.x + len) % font.term_width) * font.glyph_width;
}

static int ansi_len_to_lcd_y(int len) {
	return (ansi.y + (len + ansi.x) / font.term_width) * font.glyph_height;
}

void term_scroll(int lines) {
	if (lines != ansi.scroll) {
		ansi.scroll = lines;
		lcd_scroll(lines * font.glyph_height);
		term_erase_line(lines+font.term_height-1);
	}
}

void term_clear() {
	ansi.x = ansi.y = ansi.len = 0;
	lcd_clear();
	lcd_scroll(0);
	ansi.scroll = 0;
}

void term_erase_line(int y) {
	lcd_fill(ansi.bg, 0, y * font.glyph_height, LCD_WIDTH, font.glyph_height);
}

void term_erase_from_cursor() {
	lcd_fill(
		ansi.bg,
		ansi.x * font.glyph_width,
		ansi.y * font.glyph_height,
		LCD_WIDTH - ansi.x * font.glyph_width,
		font.glyph_height
	);
}

static void draw_cursor() {
	if (ansi.cursor_enabled && !ansi.cursor_visible) {
		ansi.cx = ansi_len_to_lcd_x(ansi.len);
		ansi.cy = ansi_len_to_lcd_y(ansi.len);
		// this used to be an underline but without a buffer of what it draws over, it causes too many artifacts
		lcd_fill(ansi.fg, ansi.cx, ansi.cy, 1, font.glyph_height - 1);
		ansi.cursor_visible = true;
	}
}

static void erase_cursor() {
	if (ansi.cursor_enabled && ansi.cursor_visible) {
		lcd_fill(ansi.bg, ansi.cx, ansi.cy, 1, font.glyph_height - 1);
		ansi.cursor_visible = false;
	}
}

static bool on_cursor_timer(repeating_timer_t *rt) {
	if (!ansi.cursor_manual && ansi.cursor_visible) {
		erase_cursor();
	} else {
		ansi.cursor_manual = false;
		draw_cursor();
	}
	return true;
}

bool term_get_blinking_cursor() {
	return ansi.cursor_enabled;
}

void term_set_blinking_cursor(bool enabled) {
	if (enabled && !ansi.cursor_enabled) {
		ansi.cursor_manual = true;
		ansi.cursor_enabled = true;
		draw_cursor();
		add_repeating_timer_ms(CURSOR_BLINK_MS, on_cursor_timer, NULL, &cursor_timer);
	} else if (!enabled && ansi.cursor_enabled) {
		erase_cursor();
		ansi.cursor_enabled = false;
		cancel_repeating_timer(&cursor_timer);
	}
}

int term_get_x() {
	return ansi.x;
}
int term_get_y() {
	return ansi.y;
}

int term_get_width() {
	return font.term_width;
}

int term_get_height() {
	return font.term_height;
}

void term_set_pos(int x, int y) {
	ansi.cursor_manual = true;
	erase_cursor();
	if (x >= 0 && x < font.term_width) ansi.x = x;
	if (y >= 0 && y < font.term_height) ansi.y = y;
	draw_cursor();
}

u16 term_get_fg() {
	return ansi.fg;
}
u16 term_get_bg() {
	return ansi.bg;
}

void term_set_fg(u16 color) {
	ansi.fg = color;
}
void term_set_bg(u16 color) {
	ansi.bg = color;
}

void term_blit(const char* text, const char* fg, const char* bg) {
	u16 pfg = ansi.fg, pbg = ansi.bg;
	const char *lfg = fg, *lbg = bg;
	while(*text) {
		if (*lfg >= '0' && *lfg <= '9') pfg = palette[*lfg - '0'];
		else if (*lfg >= 'a' && *lfg <= 'f') pfg = palette[*lfg - 'a' + 10];
		else if (*lfg >= 'A' && *lfg <= 'F') pfg = palette[*lfg - 'A' + 10];
		if (*lbg >= '0' && *lbg <= '9') pbg = palette[*lbg - '0'];
		else if (*lbg >= 'a' && *lbg <= 'f') pbg = palette[*lbg - 'a' + 10];
		else if (*lbg >= 'A' && *lbg <= 'F') pbg = palette[*lbg - 'A' + 10];
		lcd_draw_char(ansi.x * font.glyph_width, ansi.y * font.glyph_height, pfg, pbg, *text);
		ansi.x += 1;
		if (ansi.x > font.term_width) return;
		text ++;
		lfg ++; if (!*lfg) lfg = fg;
		lbg ++; if (!*lbg) lbg = bg;
	}
}

static inline void should_scroll() {
	if (ansi.x >= font.term_width) {
		ansi.x = 0;
		ansi.y += 1;
	}
	if (ansi.y >= font.term_height) {
		term_scroll(ansi.y - (font.term_height - 1));
	}
}

static void out_char(char c) {
	u16 fg, bg;
	if (ansi.c_inverse) {
		fg = ansi.bg;
		bg = ansi.fg;
	} else {
		fg = ansi.fg;
		bg = ansi.bg;
	}
	
	if (c == '\n') {
		ansi.x = 0;
		ansi.y += 1;
		//term_erase_line(ansi.y);
		should_scroll();
	} else if (c == '\b') ansi.x -= 1;
	//else if (c == '\r') ansi.x = 0;
	else {
		if (c == '\t') c = ' ';
		if (c >= 32 && c < 127) {
			should_scroll();
			lcd_draw_char(ansi.x * font.glyph_width, ansi.y * font.glyph_height, fg, bg, c);
			ansi.x += 1;
		}
	}
}

void stdio_picocalc_out_chars(const char *buf, int length) {
	while (length > 0) {
		if (ansi.state == AnsiNone) {
			if (*buf == 27 || *buf == '\x1b') ansi.state = AnsiEscape;
			else if (*buf == '\t') {
				out_char(*buf); out_char(*buf);
			}
			else out_char(*buf);
		} else if (ansi.state == AnsiEscape) {
			if (*buf == '[') ansi.state = AnsiBracket;
			else ansi.state = AnsiNone;
			ansi.stack_size = 0;
		} else if (ansi.state == AnsiBracket) {
			ansi.stack[ansi.stack_size] = 0;
			int a = 0, b = 0, semicolon = 0;
			//lcd_printf(0, 39 * 8, 0xffff, 0, "buf = '%c' stack = %s         ", *buf, ansi.stack);
			//keyboard_wait();
			switch (*buf) {
				case 'A': ansi.y -= atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor up
				case 'B': ansi.y += atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor down
				case 'C': ansi.x += atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor right
				case 'D': ansi.x -= atoi(ansi.stack); ansi.state = AnsiNone; break; // cursor left
				case 'J': term_clear(); ansi.state = AnsiNone; break; // erase display
				case 'K':	term_erase_from_cursor(); ansi.state = AnsiNone; break;
				case 'm':
					if (ansi.stack_size == 0) {
						ansi.fg = palette[DEFAULT_FG];
						ansi.bg = palette[DEFAULT_BG];
						ansi.c_inverse = false;
						ansi.c_bold = false;
					} else {
						while (semicolon <= ansi.stack_size-1) {
							a = atoi(ansi.stack + semicolon);
							b = a % 10;
							if (a == 0) { // reset all
								ansi.fg = palette[DEFAULT_FG];
								ansi.bg = palette[DEFAULT_BG];
								ansi.c_inverse = false;
								ansi.c_bold = false;
							}
							else if (a == 7)  ansi.c_inverse = true;
							else if (a == 27) ansi.c_inverse = false;
							else if (a == 1)  ansi.c_bold = true;
							else if (a == 22) ansi.c_bold = false;
							else if (a >= 30 && a <= 39) { // dim foreground
								if (b == 9) ansi.fg = palette[DEFAULT_FG];
								else if (b <= 7) ansi.fg = palette[b + ansi.c_bold * 8];
							}
							else if (a >= 40 && a <= 49) { // dim background
								if (b == 9) ansi.bg = palette[DEFAULT_BG];
								else if (b <= 7) ansi.bg = palette[b + ansi.c_bold * 8];
							}
							else if (a >= 90 && a <= 97) { // bright foreground
								ansi.fg = palette[b + 8];
							}
							else if (a >= 100 && a <= 107) { // bright background
								ansi.bg = palette[b + 8];
							}
							while (semicolon < ansi.stack_size-1 && ansi.stack[semicolon] != ';') semicolon++;
							semicolon++;
						}
					}
					ansi.state = AnsiNone;
					break;
				case 'H':
					if (ansi.stack_size == 0) {
						term_scroll(0);
						term_set_pos(0, 0);
					} else {
						a = atoi(ansi.stack);
						while (semicolon < ansi.stack_size && ansi.stack[semicolon] != ';') semicolon++;
						if (semicolon < ansi.stack_size - 1) b = atoi(ansi.stack + semicolon + 1);
						if (a > 0 && b > 0) {
							term_set_pos(b-1, a-1);
						}
					}
					ansi.state = AnsiNone;
					break;
				case 'l':
				case 'h':
					if (strncmp(ansi.stack, "?25", 3) == 0) {
						if (*buf == 'l') term_set_blinking_cursor(false);
						if (*buf == 'h') term_set_blinking_cursor(true);
					}
					ansi.state = AnsiNone;
					break;
				default:
					if (ansi.stack_size < ANSI_STACK_SIZE) ansi.stack[ansi.stack_size++] = *buf;
					else ansi.state = AnsiNone;
			}
		}
		buf++;
		length--;
	}
}

static int stdio_picocalc_in_chars(char *buf, int length) {
	input_event_t event = keyboard_poll(false);
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
	.set_chars_available_callback = set_chars_available_callback,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
	.crlf_enabled = true,
#endif
};

static void term_erase_input(int size) {
	for (int i = 0; i < size + 1; i++) {
		int x = ansi_len_to_lcd_x(i), y = ansi_len_to_lcd_y(i);
		lcd_fill(ansi.bg, x, y, font.glyph_width, font.glyph_height);
	}
}

static void term_draw_input(char* buffer, int size, int cursor) {
	if (ansi.y + (size + ansi.x) / font.term_width >= font.term_height) term_scroll(ansi.y + (size + ansi.x) / font.term_width - (font.term_height-1));
	for (int i = 0; i < size + 1; i++) {
		int x = ansi_len_to_lcd_x(i), y = ansi_len_to_lcd_y(i);
		if (i < size) lcd_draw_char(x, y, ansi.fg, ansi.bg, buffer[i]);
		else lcd_fill(ansi.bg, x, y, font.glyph_width, font.glyph_height);
		//if (ansi.cursor_enabled && i == cursor) lcd_fifo_fill(ansi.fg, x, y + font.glyph_height - 3, font.glyph_width, 2);
	}
}

static void history_save(history_t* history, int entry, char* text, int size) {
	if (entry >= 0 && entry < HISTORY_MAX) {
		if (history->buffer[entry] != NULL) free(history->buffer[entry]);
		history->buffer[entry] = strndup(text, size);
	}
}

int term_readline(char* prompt, char* buffer, int max_length, history_t* history) {
	int cursor = 0;
	int size = 0;

	buffer[size] = '\0';

	if (history) {
		history->current = 0;
		if (history->buffer[0] != NULL && history->buffer[0][0] != '\0') memmove(history->buffer + 1, history->buffer, 31 * sizeof(char*));
		history->buffer[0] = strdup(buffer);
	}

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
				ansi.x += size % font.term_width;
				ansi.y += size / font.term_width;
				stdio_picocalc_out_chars("\n", 1);
				history_save(history, 0, buffer, size);
				ansi.len = 0;
				return size;
			} else if (history && event.code == KEY_UP && history->current < 31 && history->buffer[history->current + 1] != NULL) {
				term_erase_input(size);
				history_save(history, history->current, buffer, size);
				history->current++;
				size = cursor = strlen(history->buffer[history->current]);
				memcpy(buffer, history->buffer[history->current], size);
			} else if (history && event.code == KEY_DOWN && history->current > 0) {
				term_erase_input(size);
				history_save(history, history->current, buffer, size);
				history->current--;
				size = cursor = strlen(history->buffer[history->current]);
				memcpy(buffer, history->buffer[history->current], size);
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
			if (cursor != ansi.len) {
				ansi.cursor_manual = true;
				erase_cursor();
			}
			term_draw_input(buffer, size, cursor);
			if (cursor != ansi.len) {
				ansi.len = cursor;
				draw_cursor();
			}
		}
	}
	return 0;
}
