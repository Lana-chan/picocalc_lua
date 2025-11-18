/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "kilo.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "keyboard.h"
#include "term.h"
#include "lcd.h"
#include "fs.h"
#include "sound.h"
#include "corelua.h"

/*** defines ***/

#define KILO_VERSION "0.2-PicoLua"
#define KILO_TAB_STOP 2
#define KILO_QUIT_TIMES 3

#define STATUS_DEFAULT "\x1b[7mF1\x1b[27m Save \x1b[7mF2\x1b[27m Quit \x1b[7mF3\x1b[27m Find \x1b[7mF4\x1b[27m Mark \x1b[7mF5\x1b[27m Run"
#define STATUS_MARK    "Mark Mode \x1b[7m^X\x1b[27m Cut \x1b[7m^C\x1b[27m Copy \x1b[7m^V\x1b[27m Paste \x1b[7mESC\x1b[27m Leave"
#define TEMP_FILENAME "~tmp.lua"

enum editorKey {
	KEY_NULL = 0,       /* NULL */
	CTRL_C = 3,         /* Ctrl-c */
	CTRL_D = 4,         /* Ctrl-d */
	CTRL_F = 6,         /* Ctrl-f */
	CTRL_H = 8,         /* Ctrl-h */
	TAB = 9,            /* Tab */
	CTRL_L = 12,        /* Ctrl+l */
	ENTER = 13,         /* Enter */
	CTRL_Q = 17,        /* Ctrl-q */
	CTRL_S = 19,        /* Ctrl-s */
	CTRL_U = 21,        /* Ctrl-u */
	CTRL_V = 22,        /* Ctrl-v */
	CTRL_X = 23,        /* Ctrl-x */
	ESC = 27,           /* Escape */
	BACKSPACE =  127,   /* Backspace */

	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
	CTRL_TAB,
	SHIFT_ARROW_LEFT,
	SHIFT_ARROW_RIGHT,
};

enum editorHighlight {
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH
};

enum editorMode {
	MODE_INSERT,
	MODE_MARK
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/*** data ***/

struct editorSyntax {
	char *filetype;
	char **filematch;
	char **keywords;
	char *singleline_comment_start;
	char *multiline_comment_start;
	char *multiline_comment_end;
	int flags;
};

typedef struct erow {
	int idx;
	int size;
	int rsize;
	char *chars;
	char *render;
	unsigned char *hl;
	int hl_open_comment;
} erow;

struct editorConfig {
	int cx, cy, want_rx;
	int markx, marky;
	int rx;
	int rowoff;
	int coloff;
	int screenrows;
	int screencols;
	int numrows;
	erow *row;
	int numclipboard;
	erow *clipboard;
	int dirty;
	char *filename;
	char statusmsg[128];
	time_t statusmsg_time;
	struct editorSyntax *syntax;
	enum editorMode mode;
	bool redraw_rows;
	bool linenums;
};

struct editorConfig E;

lua_State* L_state;

/*** filetypes ***/

char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL };
char *C_HL_keywords[] = {
	"switch", "if", "while", "for", "break", "continue", "return", "else",
	"struct", "union", "typedef", "static", "enum", "class", "case",

	"int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
	"void|", NULL
};

char *Lua_HL_extensions[] = { ".lua", NULL };
char *Lua_HL_keywords[] = {
	/* Keywords */
	"and","break","do","else","elseif","end",
	"false","for","function","goto","if","in",
	"local","nil","not","or","repeat","return",
	"then","true","until","while",

	/* Libs (ending with pipe) will be marked as HL_KEYWORD2 */
	"math|","table|","string|","term|","draw|","keys|","sys|","fs|",
	"colors|","sound|",NULL
};

struct editorSyntax HLDB[] = {
	{
		"c",
		C_HL_extensions,
		C_HL_keywords,
		"//", "/*", "*/",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	},
	{
		"lua",
		Lua_HL_extensions,
		Lua_HL_keywords,
		"--", "--[[", "]]",
		HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
	},
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*** prototypes ***/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));
void editorEnterExitMark();

/*** terminal ***/

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit(1);
}

int editorReadKey() {
	while(true) { // if we didn't return anything useful, do again
		input_event_t event = keyboard_wait_ex(true, true);
		
		if (event.modifiers & MOD_CONTROL) {
			switch(event.code) {
				case KEY_c:
					return CTRL_C;
				case KEY_d:
					return CTRL_D;
				case KEY_f:
					return CTRL_F;
				case KEY_h:
					return CTRL_H;
				case KEY_l:
					return CTRL_L;
				case KEY_q:
					return CTRL_Q;
				case KEY_s:
					return CTRL_S;
				case KEY_u:
					return CTRL_U;
				case KEY_v:
					return CTRL_V;
				case KEY_x:
					return CTRL_X;
				case KEY_TAB:
					return CTRL_TAB;
			}
		} else {
			switch(event.code) {
				case KEY_TAB:
					return TAB;
				case KEY_ENTER:
					return ENTER;
				case KEY_BACKSPACE:
					return BACKSPACE;
				case KEY_ESC:
					return ESC;
				case KEY_DELETE:
					return DEL_KEY;
				case KEY_PAGEUP:
					return PAGE_UP;
				case KEY_PAGEDOWN:
					return PAGE_DOWN;
				case KEY_UP:
				return ARROW_UP;
				case KEY_DOWN:
				return ARROW_DOWN;
				case KEY_RIGHT:
					if (event.modifiers & MOD_SHIFT) return SHIFT_ARROW_RIGHT;
					return ARROW_RIGHT;
				case KEY_LEFT:
					if (event.modifiers & MOD_SHIFT) return SHIFT_ARROW_LEFT;
					return ARROW_LEFT;
				case KEY_HOME:
					return HOME_KEY;
				case KEY_END:
					return END_KEY;
				case KEY_F1:
				case KEY_F2:
				case KEY_F3:
				case KEY_F4:
				case KEY_F5:
					return event.code;
				default:
					if(isprint(event.code))
						return event.code;
			}
		}
	}
	return KEY_NULL; // shouldn't reach
}

int getCursorPosition(int *rows, int *cols) {
	*cols = term_get_x();
	*rows = term_get_y();
	return 0;
}

int getWindowSize(int *rows, int *cols) {
	*cols = term_get_width();
	*rows = term_get_height();
	return 0;
}

/*** syntax highlighting ***/

int is_separator(int c) {
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
	row->hl = realloc(row->hl, row->rsize);
	memset(row->hl, HL_NORMAL, row->rsize);

	if (E.syntax == NULL) return;

	char **keywords = E.syntax->keywords;

	char *scs = E.syntax->singleline_comment_start;
	char *mcs = E.syntax->multiline_comment_start;
	char *mce = E.syntax->multiline_comment_end;

	int scs_len = scs ? strlen(scs) : 0;
	int mcs_len = mcs ? strlen(mcs) : 0;
	int mce_len = mce ? strlen(mce) : 0;

	int prev_sep = 1;
	int in_string = 0;
	int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

	int i = 0;
	while (i < row->rsize) {
		char c = row->render[i];
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

		if (mcs_len && mce_len && !in_string) {
			if (in_comment) {
				row->hl[i] = HL_MLCOMMENT;
				if (!strncmp(&row->render[i], mce, mce_len)) {
					memset(&row->hl[i], HL_MLCOMMENT, mce_len);
					i += mce_len;
					in_comment = 0;
					prev_sep = 1;
					continue;
				} else {
					i++;
					continue;
				}
			} else if (!strncmp(&row->render[i], mcs, mcs_len)) {
				memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
				i += mcs_len;
				in_comment = 1;
				continue;
			}
		}

		if (scs_len && !in_string && !in_comment) {
			if (!strncmp(&row->render[i], scs, scs_len)) {
				memset(&row->hl[i], HL_COMMENT, row->rsize - i);
				break;
			}
		}

		if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
			if (in_string) {
				row->hl[i] = HL_STRING;
				if (c == '\\' && i + 1 < row->rsize) {
					row->hl[i + 1] = HL_STRING;
					i += 2;
					continue;
				}
				if (c == in_string) in_string = 0;
				i++;
				prev_sep = 1;
				continue;
			} else {
				if (c == '"' || c == '\'') {
					in_string = c;
					row->hl[i] = HL_STRING;
					i++;
					continue;
				}
			}
		}

		if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
			if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
					(c == '.' && prev_hl == HL_NUMBER)) {
				row->hl[i] = HL_NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}
		}

		if (prev_sep) {
			int j;
			for (j = 0; keywords[j]; j++) {
				int klen = strlen(keywords[j]);
				int kw2 = keywords[j][klen - 1] == '|';
				if (kw2) klen--;

				if (!strncmp(&row->render[i], keywords[j], klen) &&
						is_separator(row->render[i + klen])) {
					memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
					i += klen;
					break;
				}
			}
			if (keywords[j] != NULL) {
				prev_sep = 0;
				continue;
			}
		}

		prev_sep = is_separator(c);
		i++;
	}

	int changed = (row->hl_open_comment != in_comment);
	row->hl_open_comment = in_comment;
	if (changed && row->idx + 1 < E.numrows)
		editorUpdateSyntax(&E.row[row->idx + 1]);
}

int editorSyntaxToColor(int hl) {
	switch (hl) {
		case HL_COMMENT:
		case HL_MLCOMMENT: return 36;
		case HL_KEYWORD1: return 93;
		case HL_KEYWORD2: return 92;
		case HL_STRING: return 95;
		case HL_NUMBER: return 91;
		case HL_MATCH: return 94;
		default: return 37;
	}
}

void editorSelectSyntaxHighlight() {
	E.syntax = NULL;
	if (E.filename == NULL) return;

	char *ext = strrchr(E.filename, '.');

	for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
		struct editorSyntax *s = &HLDB[j];
		unsigned int i = 0;
		while (s->filematch[i]) {
			int is_ext = (s->filematch[i][0] == '.');
			if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
					(!is_ext && strstr(E.filename, s->filematch[i]))) {
				E.syntax = s;

				int filerow;
				for (filerow = 0; filerow < E.numrows; filerow++) {
					editorUpdateSyntax(&E.row[filerow]);
				}

				return;
			}
			i++;
		}
	}
}

/*** row operations ***/

int editorRowCxToRx(erow *row, int cx) {
	int rx = 0;
	int j;
	for (j = 0; j < cx; j++) {
		if (row->chars[j] == '\t')
			rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
		rx++;
	}
	return rx;
}

int editorRowRxToCx(erow *row, int rx) {
	int cur_rx = 0;
	int cx;
	for (cx = 0; cx < row->size; cx++) {
		if (row->chars[cx] == '\t')
			cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
		cur_rx++;

		if (cur_rx > rx) return cx;
	}
	return cx;
}

void editorUpdateRow(erow *row) {
	int tabs = 0;
	int j;
	for (j = 0; j < row->size; j++)
		if (row->chars[j] == '\t') tabs++;

	free(row->render);
	row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);

	int idx = 0;
	for (j = 0; j < row->size; j++) {
		if (row->chars[j] == '\t') {
			row->render[idx++] = ' ';
			while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
		} else {
			row->render[idx++] = row->chars[j];
		}
	}
	row->render[idx] = '\0';
	row->rsize = idx;

	editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
	if (at < 0 || at > E.numrows) return;

	E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
	memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
	for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

	E.row[at].idx = at;

	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';

	E.row[at].rsize = 0;
	E.row[at].render = NULL;
	E.row[at].hl = NULL;
	E.row[at].hl_open_comment = 0;
	editorUpdateRow(&E.row[at]);

	E.numrows++;
	E.dirty++;
}

void editorFreeRow(erow *row) {
	free(row->render);
	free(row->chars);
	free(row->hl);
}

void editorDelRow(int at) {
	if (at < 0 || at >= E.numrows) return;
	editorFreeRow(&E.row[at]);
	memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
	for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
	E.numrows--;
	E.dirty++;
}

void editorRowInsertString(erow *row, int at, char *s, size_t len) {
	if (len < 1) return;
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + len + 1);
	memmove(&row->chars[at + len], &row->chars[at], row->size - at + 1);
	memcpy(&row->chars[at], s, len);
	row->size += len;
	row->chars[row->size] = '\0';
	editorUpdateRow(row);
	E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	editorUpdateRow(row);
	E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
	editorRowInsertString(row, row->size, s, len);
}

void editorRowDelChar(erow *row, int at) {
	if (at < 0 || at >= row->size) return;
	memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
	row->size--;
	editorUpdateRow(row);
	E.dirty++;
}

void editorCopyRow(erow* source, int idx, int at, int len) {
	erow* dest = &E.clipboard[idx];
	dest->idx = idx;
	dest->size = len;
	dest->chars = malloc(len + 1);
	memcpy(dest->chars, &source->chars[at], len);
	dest->chars[len] = '\0';
	dest->rsize = 0;
	dest->render = NULL;
	dest->hl = NULL;
	dest->hl_open_comment = 0;
}

/*** editor operations ***/

void editorOrderMarkPos(int* m1x, int* m1y, int* m2x, int* m2y) {
	if (E.mode == MODE_MARK) {
		if (E.cy < E.marky || (E.cx < E.markx && E.cy == E.marky)) {
			*m1x = E.cx;
			*m1y = E.cy;
			*m2x = E.markx;
			*m2y = E.marky;
		} else {
			*m1x = E.markx;
			*m1y = E.marky;
			*m2x = E.cx;
			*m2y = E.cy;
		}
	}
}

void editorInsertChar(int c) {
	if (E.cy == E.numrows) {
		editorInsertRow(E.numrows, "", 0);
	}
	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	E.cx++;
	E.redraw_rows = true;
}

void editorInsertNewline(bool tabs) {
	int new_x = 0;
	if (E.cx == 0) {
		editorInsertRow(E.cy, "", 0);
	} else {
		erow *row = &E.row[E.cy];
		// count tab indentation in current row
		int num_tabs = 0;
		while (*&row->chars[num_tabs] == '\t') num_tabs++;
		editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
		// add tabs to new row
		if (tabs) {
			new_x = num_tabs;
			while (num_tabs--) editorRowInsertChar(&E.row[E.cy+1], 0, '\t');
		}
		row = &E.row[E.cy];
		row->size = E.cx;
		row->chars[row->size] = '\0';
		editorUpdateRow(row);
	}
	E.cy++;
	E.cx = new_x;
	E.redraw_rows = true;
}

void editorInsertClipboard() {
	if (E.numclipboard < 1) return;

	for (int y = 0; y < E.numclipboard; y++) {
		if (y > 0) editorInsertNewline(false);
		editorRowInsertString(&E.row[E.cy], E.cx, E.clipboard[y].chars, E.clipboard[y].size);
		E.cx += E.clipboard[y].size;
	}

	E.redraw_rows = true;
}

void editorCopyMark() {
	if (E.mode != MODE_MARK) return;
	if (E.cy == E.marky && E.cx == E.markx) return; // make sure mark is not empty

	int m1x, m1y, m2x, m2y, y;
	editorOrderMarkPos(&m1x, &m1y, &m2x, &m2y);

	// free old clipboard
	for (int i = 0; i < E.numclipboard; ++i) {
		erow* row = &E.clipboard[i];
		if (row->chars) { free(row->chars); row->chars = NULL; }
		if (row->render) { free(row->render); row->render = NULL; }
		if (row->hl) { free(row->hl); row->hl = NULL; }
	}
	free(E.clipboard);
	E.clipboard = NULL;

	// make new clipboard
	E.numclipboard = m2y - m1y + 1;
	E.clipboard = malloc(sizeof(erow) * (E.numclipboard));
	
	if (E.numclipboard > 1) {
		// first line
		editorCopyRow(&E.row[m1y], 0, m1x, E.row[m1y].size - m1x);
		// intermediaries
		for (y = m1y + 1; y < m2y; y++) {
			editorCopyRow(&E.row[y], y - m1y, 0, E.row[y].size);
		}
		// last line
		editorCopyRow(&E.row[y], y - m1y, 0, m2x);
	} else {
		editorCopyRow(&E.row[m1y], 0, m1x, m2x - m1x);
	}
}

void editorDelChar() {
	if (E.cy == E.numrows) return;
	if (E.cx == 0 && E.cy == 0) return;

	erow *row = &E.row[E.cy];
	if (E.cx > 0) {
		editorRowDelChar(row, E.cx - 1);
		E.cx--;
	} else {
		E.cx = E.row[E.cy - 1].size;
		editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
		editorDelRow(E.cy);
		E.cy--;
	}

	E.redraw_rows = true;
}

void editorDelMark() {
	if (E.mode != MODE_MARK) return;
	if (E.cy == E.marky && E.cx == E.markx) return; // make sure mark is not empty

	int m1x, m1y, m2x, m2y, tail_len, y;
	char* tail;

	editorOrderMarkPos(&m1x, &m1y, &m2x, &m2y);

	// pick up what's right of mark
	tail_len = E.row[m2y].size - m2x;

	if (m2y == m1y) { // selection is in the same row
		// put it in what's left of mark
		strncpy(&E.row[m1y].chars[m1x], &E.row[m2y].chars[m2x], tail_len);
		E.row[m1y].size -= m2x - m1x;
		E.row[m1y].chars[E.row[m1y].size] = '\0';
		editorUpdateRow(&E.row[m1y]);
		E.dirty++;
	} else { // multiple rows
		// put it in what's left of mark
		E.row[m1y].size = m1x;
		E.row[m1y].chars[m1x] = '\0';
		editorUpdateRow(&E.row[m1y]);
		editorRowAppendString(&E.row[m1y], &E.row[m2y].chars[m2x], tail_len);
	
		// delete everything that was inbetween
		for (y = 0; y < m2y - m1y; y++) {
			editorDelRow(m1y + 1);
		}
	}

	E.cx = m1x;
	E.cy = m1y;

	editorEnterExitMark();
}

void editorIndentMark(int tab) {
	int m1x, m1y, m2x, m2y, y;
	editorOrderMarkPos(&m1x, &m1y, &m2x, &m2y);

	for (y = m1y; y <= m2y; y++) {
		if (tab == TAB) {
			if (y == E.cy) E.cx++;
			if (y == E.marky) E.markx++;
			editorRowInsertChar(&E.row[y], 0, '\t');
		} else if (tab == CTRL_TAB) {
			if (E.row[y].chars[0] == '\t') {
				if (y == E.cy) E.cx--;
				if (y == E.marky) E.markx--;
				editorRowDelChar(&E.row[y], 0);
			}
		}
	}

	E.redraw_rows = true;
}

/*** file i/o ***/

char *editorRowsToString(int *buflen) {
	int totlen = 0;
	int j;
	for (j = 0; j < E.numrows; j++)
		totlen += E.row[j].size + 1;
	*buflen = totlen;

	char *buf = malloc(totlen);
	char *p = buf;
	for (j = 0; j < E.numrows; j++) {
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}

	return buf;
}

void editorOpen(const char *filename) {
	FIL fp;
	FRESULT res;

	E.dirty = 0;
	free(E.filename);
	size_t fnlen = strlen(filename)+1;
	E.filename = malloc(fnlen);
	memcpy(E.filename,filename,fnlen);

	editorSelectSyntaxHighlight();

	if (fs_exists(filename)) { // if it doesn't exist we're just starting a new file
		res = f_open(&fp, filename, FA_READ);
		if (res != FR_OK) {
			if (errno != ENOENT) {
				perror("Opening file");
				exit(1);
			}
			return;
		}
	
		char *line = NULL;
		UINT len;
		while(!f_eof(&fp)) {
			fs_readline(&fp, &line, &len);
			editorInsertRow(E.numrows,line,len);
		}
		free(line);
		f_close(&fp);
		E.dirty = 0;
	}
	return;
}

void editorSave(bool temp) {
	const char* filename = (temp ? TEMP_FILENAME : E.filename);

	if (filename == NULL) {
    E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
    editorSelectSyntaxHighlight();
		filename = E.filename;
  }

	int len;
	int wlen;
	char *buf = editorRowsToString(&len);

	FIL fp;
	FRESULT res;

	res = f_open(&fp, filename, FA_CREATE_ALWAYS|FA_READ|FA_WRITE);
	if (res != FR_OK) {
		free(buf);
		f_close(&fp);
		editorSetStatusMessage("Can't save! I/O error: %s", fs_error_strings[res]);
		return;
	}

	f_write(&fp, buf, len, &wlen);

	f_close(&fp);
	free(buf);
	if (!temp) E.dirty = 0;
	editorSetStatusMessage("%d bytes written on disk", wlen);
	return;
}

/*** find ***/

void editorFindCallback(char *query, int key) {
	static int last_match = -1;
	static int direction = 1;

	static int saved_hl_line;
	static char *saved_hl = NULL;

	if (saved_hl) {
		memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
		free(saved_hl);
		saved_hl = NULL;
	}

	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;
		return;
	} else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
		direction = 1;
	} else if (key == ARROW_LEFT || key == ARROW_UP) {
		direction = -1;
	} else {
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1) direction = 1;
	int current = last_match;
	int i;
	for (i = 0; i < E.numrows; i++) {
		current += direction;
		if (current == -1) current = E.numrows - 1;
		else if (current == E.numrows) current = 0;

		erow *row = &E.row[current];
		char *match = strstr(row->render, query);
		if (match) {
			last_match = current;
			E.cy = current;
			E.cx = editorRowRxToCx(row, match - row->render);
			E.rowoff = E.numrows;

			saved_hl_line = current;
			saved_hl = malloc(row->rsize);
			memcpy(saved_hl, row->hl, row->rsize);
			memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
			break;
		}
	}
	E.redraw_rows = true;
}

void editorFind() {
	int saved_cx = E.cx;
	int saved_cy = E.cy;
	int saved_coloff = E.coloff;
	int saved_rowoff = E.rowoff;

	char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",
														 editorFindCallback);

	if (query) {
		free(query);
	} else {
		E.cx = saved_cx;
		E.cy = saved_cy;
		E.coloff = saved_coloff;
		E.rowoff = saved_rowoff;
	}
	E.redraw_rows = true;
}

/*** append buffer ***/

struct abuf {
	char *b;
	int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);

	if (new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}

/*** output ***/

int editorScroll() {
	int old_rowoff = E.rowoff;
	int old_coloff = E.coloff;

	E.rx = 0;
	if (E.cy < E.numrows) {
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
	}

	if (E.cy < E.rowoff) {
		E.rowoff = E.cy;
	}
	if (E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1;
	}

	// line number margin, measures longest number width possible on screen
	int margin = 0;
	int longest = (E.screenrows + E.rowoff > E.numrows ? E.numrows : E.screenrows + E.rowoff);
	if (E.linenums) margin = snprintf(NULL, 0, "%d", longest) + 1;
	int screencols = E.screencols - margin;

	if (E.rx < E.coloff) {
		E.coloff = E.rx;
	}
	if (E.rx >= E.coloff + screencols) {
		E.coloff = E.rx - screencols + 1;
	}

	if (E.rowoff != old_rowoff || E.coloff != old_coloff) E.redraw_rows = true;

	return margin;
}

void editorDrawRows(struct abuf *ab, int margin) {
	int y;
	int m1x, m1y, m2x, m2y, rowmx, in_mark = 0;
	editorOrderMarkPos(&m1x, &m1y, &m2x, &m2y);

	// shorten screencols by margin
	int screencols = E.screencols - margin;

	for (y = 0; y < E.screenrows; y++) {
		if (E.redraw_rows) {
			int filerow = y + E.rowoff;
			if (filerow >= E.numrows) {
				int padding = margin;
				while (padding--) abAppend(ab, " ", 1);
				if (E.numrows == 0 && y == E.screenrows / 3) {
					char welcome[80];
					int welcomelen = snprintf(welcome, sizeof(welcome),
						"Kilo editor -- version %s", KILO_VERSION);
					if (welcomelen > screencols) welcomelen = screencols;
					padding = (screencols - welcomelen) / 2;
					if (padding) {
						abAppend(ab, "~", 1);
						padding--;
					}
					while (padding--) abAppend(ab, " ", 1);
					abAppend(ab, welcome, welcomelen);
				} else {
					abAppend(ab, "~", 1);
				}
			} else {
				int len = E.row[filerow].rsize - E.coloff;
				if (len < 0) len = 0;
				if (len > screencols) len = screencols;
				char *c = &E.row[filerow].render[E.coloff];
				unsigned char *hl = &E.row[filerow].hl[E.coloff];
				int current_color = -1;
				int j;
				if (E.linenums) {
					char linenumbuf[32];
					int linenumlen = snprintf(linenumbuf, sizeof(linenumbuf), "\x1b[90m%*d \x1b[39m", margin-1, filerow+1);
					abAppend(ab, linenumbuf, linenumlen);
				}
				if (E.mode == MODE_MARK) {
					if (m1y < filerow && m2y > filerow) in_mark = 1;
					if (in_mark == 1) abAppend(ab, "\x1b[7m", 4);
				}
				for (j = 0; j < len; j++) {
					if (E.mode == MODE_MARK) {
						if (in_mark == 0 && filerow == m1y) {
							rowmx = editorRowCxToRx(&E.row[filerow], m1x);
							if (rowmx <= j + E.coloff) {
								in_mark = 1;
								abAppend(ab, "\x1b[7m", 4);
							}
						}
						if (in_mark == 1 && filerow == m2y) {
							rowmx = editorRowCxToRx(&E.row[filerow], m2x);
							if (rowmx <= j + E.coloff) {
								in_mark = 0;
								abAppend(ab, "\x1b[27m", 5);
							}
						}
					}
					if (iscntrl(c[j])) {
						char sym = (c[j] <= 26) ? '@' + c[j] : '?';
						abAppend(ab, "\x1b[7m", 4);
						abAppend(ab, &sym, 1);
						abAppend(ab, "\x1b[m", 3);
						if (current_color != -1) {
							char buf[16];
							int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
							abAppend(ab, buf, clen);
						}
					} else if (hl[j] == HL_NORMAL) {
						if (current_color != -1) {
							abAppend(ab, "\x1b[39m", 5);
							current_color = -1;
						}
						abAppend(ab, &c[j], 1);
					} else {
						int color = editorSyntaxToColor(hl[j]);
						if (color != current_color) {
							current_color = color;
							char buf[16];
							int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
							abAppend(ab, buf, clen);
						}
						abAppend(ab, &c[j], 1);
					}
				}
				if (E.mode == MODE_MARK) {
					// edge case: mark starts at end of line
					if (in_mark == 0 && filerow == m1y) {
						rowmx = editorRowCxToRx(&E.row[filerow], m2x);
						if (rowmx >= len) in_mark = 1;
					}
					// edge case: mark ends at end of line
					if (in_mark == 1 && filerow == m2y) {
						rowmx = editorRowCxToRx(&E.row[filerow], m2x);
						if (rowmx >= len) {
							in_mark = 0;
							abAppend(ab, "\x1b[27m", 5);
						}
					}
				}
				abAppend(ab, "\x1b[39m", 8);
			}
			abAppend(ab, "\x1b[K", 3);
		}
		abAppend(ab, "\r\n", 2);
	}
	E.redraw_rows = false;
}

int strlenWithoutANSI(char* msg, int* real_len) {
	bool in_escape = 0;
	int count = 0;
	if (real_len) (*real_len) = 0;
	while (*msg && count < E.screencols) {
		if (*msg == '\x1b') in_escape = true;
		if (!in_escape) count++;
		if (in_escape && *msg == 'm') in_escape = false;
		*msg++;
		if (real_len) (*real_len)++;
	}
	return count;
}

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[7m", 4);
	char status[80], rstatus[80];
	int len = snprintf(status, sizeof(status), "%.20s %s",
		E.filename ? E.filename : "[No Name]",
		E.dirty ? "(modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d \x1b[92m %d%%",
		E.cy + 1, E.numrows,
		get_battery(NULL)
		//E.syntax ? E.syntax->filetype : "no ft",
	);
	int rlen_ansi = strlenWithoutANSI(rstatus, NULL);
	if (len > E.screencols) len = E.screencols;
	abAppend(ab, status, len);
	while (len < E.screencols) {
		if (E.screencols - len == rlen_ansi) {
			abAppend(ab, rstatus, rlen);
			break;
		} else {
			abAppend(ab, " ", 1);
			len++;
		}
	}
	abAppend(ab, "\x1b[m", 4);
	abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
	if (time(NULL) - E.statusmsg_time > 5)
		editorSetStatusMessage(STATUS_DEFAULT);
	abAppend(ab, "\x1b[K", 3);
	int msglen = 0;
	strlenWithoutANSI(E.statusmsg, &msglen);
	abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() {
	int margin = editorScroll();

	// permanent statuses
	if (E.mode == MODE_MARK) editorSetStatusMessage(STATUS_MARK);

	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[m", 3);
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);

	editorDrawRows(&ab, margin);
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);

	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
																						(E.rx - E.coloff) + 1 + margin);
	abAppend(&ab, buf, strlen(buf));

	abAppend(&ab, "\x1b[?25h", 6);

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}

/*** input ***/

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
	size_t bufsize = 128;
	char *buf = malloc(bufsize);

	size_t buflen = 0;
	buf[0] = '\0';

	while (1) {
		editorSetStatusMessage(prompt, buf);
		editorRefreshScreen();

		int c = editorReadKey();
		if (c == DEL_KEY || c == CTRL_H || c == BACKSPACE) {
			if (buflen != 0) buf[--buflen] = '\0';
		} else if (c == '\x1b') {
			editorSetStatusMessage(STATUS_DEFAULT);
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		} else if (c == '\r') {
			if (buflen != 0) {
				editorSetStatusMessage(STATUS_DEFAULT);
				if (callback) callback(buf, c);
				return buf;
			}
		} else if (!iscntrl(c) && c < 128) {
			if (buflen == bufsize - 1) {
				bufsize *= 2;
				buf = realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}

		if (callback) callback(buf, c);
	}
}

int editorMoveCursor(int key) {
	erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
	int old_cx = E.cx;
	int old_cy = E.cy;

	switch (key) {
		case ARROW_LEFT:
			if (E.cx != 0) {
				E.cx--;
			} else if (E.cy > 0) {
				E.cy--;
				E.cx = E.row[E.cy].size;
			}
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size) {
				E.cx++;
			} else if (row && E.cx == row->size) {
				E.cy++;
				E.cx = 0;
			}
			break;
		case ARROW_UP:
			if (E.cy != 0) {
				E.cy--;
			}
			break;
		case ARROW_DOWN:
			if (E.cy < E.numrows) {
				E.cy++;
			}
			break;
		case HOME_KEY:
			E.cx = 0;
			break;
		case END_KEY:
			if (E.cy < E.numrows)
				E.cx = E.row[E.cy].size;
			break;
	}

	row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

	if (row) {
		if (!(key == ARROW_UP || key == ARROW_DOWN)) E.want_rx = editorRowCxToRx(row, E.cx);
		else E.cx = editorRowRxToCx(row, E.want_rx);
	}

	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen) {
		E.cx = rowlen;
	}

	if (E.mode == MODE_MARK) E.redraw_rows = true;
	return (E.cx - old_cx) + (E.cy - old_cy);
}

bool in_word(int off) {
	int look_at = E.cx + off;
	if (look_at < 0) return false;
	if (look_at >= E.row[E.cy].size) return false;
	char c = E.row[E.cy].chars[look_at];
	if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			(c == '_'))
		return true;
	return false;
}

void editorMoveCursorWord(int key) {
	int arrow = (key == SHIFT_ARROW_LEFT ? ARROW_LEFT : ARROW_RIGHT);
	int off = (key == SHIFT_ARROW_LEFT ? -1 : 0);
	int old_cy = E.cy;
	if (!in_word(off)) while (!in_word(off)) if (E.cy != old_cy || editorMoveCursor(arrow) == 0) break;
	if (in_word(off)) while (in_word(off)) if (E.cy != old_cy || editorMoveCursor(arrow) == 0) break;
}

void editorEnterExitMark() {
	if (E.mode == MODE_INSERT) {
		E.mode = MODE_MARK;
		E.markx = E.cx;
		E.marky = E.cy;
	} else {
		E.mode = MODE_INSERT;
		editorSetStatusMessage(STATUS_DEFAULT);
		E.redraw_rows = true;
	}
}

void runProgram() {
	editorSave(true);

	term_clear();
	keyboard_flush();
	lua_getglobal(L_state, "collectgarbage");
	lua_pcall(L_state, 0, 0, 0);
	term_set_blinking_cursor(false);

	lua_pre_script(L_state);

	int status = luaL_dofile(L_state, TEMP_FILENAME);

	lua_post_script(L_state);

	if(status != LUA_OK) {
		const char *msg = lua_tostring(L_state, -1);
		lua_writestringerror("%s\n", msg);
	}
	
	printf("\x1b[%d;%dHPress any key...", E.screenrows+2, 1);
	keyboard_flush();
	keyboard_wait_ex(false, true);

	term_clear();
	E.redraw_rows = true;
}

int editorProcessKeypress() {
	static int quit_times = KILO_QUIT_TIMES;

	int c = editorReadKey();
	bool handled = true;
	// keypresses for specific modes only
	if (E.mode == MODE_INSERT) {
		switch (c) {
			case CTRL_C:
			case CTRL_X:
			case ESC:
				break;

			case KEY_F1:
				editorSave(false);
				break;

			case KEY_F2:
				if (E.dirty && quit_times > 0) {
					editorSetStatusMessage("\x1b[7;91m!! Unsaved !!\x1b[m"
						"Press F2 %d more times to quit.", quit_times);
					quit_times--;
					return 0;
				}
				return 1;

			case KEY_F3:
				editorFind();
				break;

			case KEY_F5:
				runProgram();
				break;

			case CTRL_TAB:
				if (E.row[E.cy].chars[0] == '\t') {
					editorRowDelChar(&E.row[E.cy], 0);
					E.redraw_rows = true;
				}
				break;

			default:
				handled = false;
		}
	} else if (E.mode == MODE_MARK) {
		switch (c) {
			case KEY_F1:
			case KEY_F2:
			case KEY_F3:
			case KEY_F5:
				break;

			case TAB:
			case CTRL_TAB:
				editorIndentMark(c);
				break;

			case CTRL_C:
				editorCopyMark();
				editorEnterExitMark();
				break;

			case CTRL_X:
				editorCopyMark();
				editorDelMark();
				break;

			case ESC:
				editorEnterExitMark();
				break;

			default:
				handled = false;
		}
	}

	// common keypresses
	if (!handled) {
		switch (c) {
			case ENTER:
				if (E.mode == MODE_MARK) editorDelMark();
				editorInsertNewline(true);
				break;
	
			case KEY_F4:
				editorEnterExitMark();
				break;
	
			case DEL_KEY:
				if (E.mode != MODE_MARK) editorMoveCursor(ARROW_RIGHT);
			case BACKSPACE:
			case CTRL_H:
				if (E.mode == MODE_MARK) editorDelMark();
				else editorDelChar();
				break;
	
			case PAGE_UP:
			case PAGE_DOWN:
				{
					int times = E.screenrows - 1;
					while (times--)
						editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
				}
				break;
	
			case SHIFT_ARROW_LEFT:
			case SHIFT_ARROW_RIGHT:
				editorMoveCursorWord(c);
				break;
	
			case HOME_KEY:
			case END_KEY:
			case ARROW_UP:
			case ARROW_DOWN:
			case ARROW_LEFT:
			case ARROW_RIGHT:
				editorMoveCursor(c);
				break;
	
			case CTRL_L:
				E.linenums = !E.linenums;
				E.redraw_rows = true;
				break;
	
			case CTRL_V:
				if (E.mode == MODE_MARK) editorDelMark();
				editorInsertClipboard();
				break;
	
			default:
				if (E.mode == MODE_MARK) editorDelMark();
				editorInsertChar(c);
				break;
		}
	}

	quit_times = KILO_QUIT_TIMES;
	return 0;
}

/*** init ***/

void initEditor() {
	E.cx = 0;
	E.cy = 0;
	E.rx = 0;
	E.rowoff = 0;
	E.coloff = 0;
	E.numrows = 0;
	E.row = NULL;
	E.numclipboard = 0;
	E.clipboard = NULL;
	E.dirty = 0;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;
	E.syntax = NULL;
	E.redraw_rows = true;
	E.mode = MODE_INSERT;
	E.linenums = false;

	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
	E.screenrows -= 2;
}

int start_editor(lua_State* L, const char* filename) {
	L_state = L;

	// consider kilo to be outside lua context
	// mostly because of keybaord interrupt
	lua_post_script(L);

	initEditor();
	if (filename && filename[0] != '\0') editorOpen(filename);

	editorSetStatusMessage(STATUS_DEFAULT);

	while (1) {
		editorRefreshScreen();
		if (editorProcessKeypress() != 0) break;
	}

	// quitting, clear everything
	for (int i = 0; i < E.numrows; ++i) {
		erow* row = &E.row[i];
		if (row->chars) { free(row->chars); row->chars = NULL; }
		if (row->render) { free(row->render); row->render = NULL; }
		if (row->hl) { free(row->hl); row->hl = NULL; }
	}
	free(E.row);
	E.row = NULL;

	for (int i = 0; i < E.numclipboard; ++i) {
		erow* row = &E.clipboard[i];
		if (row->chars) { free(row->chars); row->chars = NULL; }
		if (row->render) { free(row->render); row->render = NULL; }
		if (row->hl) { free(row->hl); row->hl = NULL; }
	}
	free(E.clipboard);
	E.clipboard = NULL;

	if (E.filename) {
		free(E.filename);
		E.filename = NULL;
	}

	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	// returning to lua context
	lua_pre_script(L);

	return 0;
}
