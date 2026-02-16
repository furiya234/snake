#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static struct termios orig_termios;
static int raw_mode_enabled = 0;

void clear_screen(void) {
    write(STDOUT_FILENO, "\033[2J\033[H", 7);
}

void get_screen_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    } else {
        *rows = 24;
        *cols = 80;
    }
}

void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

void draw_char(int row, int col, char c) {
    printf("\033[%d;%dH%c", row, col, c);
}

void hide_cursor(void) {
    write(STDOUT_FILENO, "\033[?25l", 6);
}

void show_cursor(void) {
    write(STDOUT_FILENO, "\033[?25h", 6);
}

void disable_raw_mode(void) {
    if (raw_mode_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_enabled = 0;
    }
}

void enable_raw_mode(void) {
    if (raw_mode_enabled) return;

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode_enabled = 1;
}

int read_key(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return -1;

    if (c == '\033') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\033';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
        }
        return '\033';
    }

    return c;
}
