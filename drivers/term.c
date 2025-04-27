#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/stdio/driver.h"
#include <stdlib.h>
#include <string.h>

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
  RGB(0, 0, 0),
  RGB(255, 0, 0),
  RGB(0, 255, 0),
  RGB(255, 255, 0),
  RGB(0, 0, 255),
  RGB(255, 0, 255),
  RGB(0, 0, 255),
  RGB(255, 255, 255),
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

static void erase_line(int y) {
  lcd_fill(palette[0], 0, y * 8, 320, 8);
}

static void draw_cursor(int x, int y) {
  lcd_fill(palette[7], x * 6, y * 8, 6, 8);
}

static void erase_cursor(int x, int y) {
  lcd_fill(palette[0], x * 6, y * 8, 6, 8);
}

static void out_char(char c) {
  erase_cursor(ansi.x, ansi.y);
  if (c == '\n') {
    ansi.x = 0;
    ansi.y += 1;
    erase_line(ansi.y);
  } else if (c == '\b') ansi.x -= 1;
  //else if (c == '\r') ansi.x = 0;
  else if (c >= 32 && c < 127) {
    lcd_draw_char(ansi.x * 6, ansi.y * 8, palette[ansi.fg], palette[ansi.bg], c);
    ansi.x += 1;
  }
  if (ansi.x >= 53) {
    ansi.x = 0;
    ansi.y += 1;
    erase_line(ansi.y);
  }
  if (ansi.y > 39) {
    lcd_scroll((ansi.y - 39) * 8);
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
  sleep_ms(10);
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

