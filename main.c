#include "ui/ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_SNAKE_LEN 1024

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

void init_screen(void) {
    enable_raw_mode();
    hide_cursor();
    clear_screen();
}

int is_opposite(Direction a, Direction b) {
    return (a == DIR_UP && b == DIR_DOWN) ||
           (a == DIR_DOWN && b == DIR_UP) ||
           (a == DIR_LEFT && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

int key_to_direction(int key, Direction *out) {
    if (key == KEY_UP || key == 'w' || key == 'W') {
        *out = DIR_UP;
        return 1;
    }
    if (key == KEY_DOWN || key == 's' || key == 'S') {
        *out = DIR_DOWN;
        return 1;
    }
    if (key == KEY_LEFT || key == 'a' || key == 'A') {
        *out = DIR_LEFT;
        return 1;
    }
    if (key == KEY_RIGHT || key == 'd' || key == 'D') {
        *out = DIR_RIGHT;
        return 1;
    }
    return 0;
}

int main(void) {
    int rows, cols;
    get_screen_size(&rows, &cols);
    init_screen();
    draw_welcome();
    srand((unsigned)time(NULL));

    int r = rows / 2;
    int c = cols / 2;
    int snake_r[MAX_SNAKE_LEN];
    int snake_c[MAX_SNAKE_LEN];
    int snake_len = 1;
    Direction dir = DIR_RIGHT;
    int item_r = rows / 3;
    int item_c = cols / 3;
    snake_r[0] = r;
    snake_c[0] = c;

    if (item_r < 2) item_r = 2;
    if (item_c < 1) item_c = 1;
    if (item_r == r && item_c == c) item_c = (item_c % cols) + 1;

    draw_char(r, c, '@');
    draw_char(item_r, item_c, '*');
    draw_char(1, 1, 'q');
    move_cursor(1, 3);
    printf("= quit | arrows/WASD = change direction | auto move | %dx%d", cols, rows);
    fflush(stdout);

    while (1) {
        int should_quit = 0;
        int key = read_key();
        Direction requested = dir;
        int has_direction_input = 0;

        // Drain all pending input this tick to avoid dropped direction changes.
        while (key != -1) {
            if (key == KEY_QUIT) {
                should_quit = 1;
                break;
            }
            if (key_to_direction(key, &requested)) {
                has_direction_input = 1;
            }
            key = read_key();
        }

        if (should_quit) {
            break;
        }

        if (has_direction_input && !is_opposite(dir, requested)) {
            dir = requested;
        }

        int next_r = r;
        int next_c = c;

        if (dir == DIR_UP) next_r--;
        if (dir == DIR_DOWN) next_r++;
        if (dir == DIR_LEFT) next_c--;
        if (dir == DIR_RIGHT) next_c++;

        if (next_r < 2) next_r = rows;
        if (next_r > rows) next_r = 2;
        if (next_c < 1) next_c = cols;
        if (next_c > cols) next_c = 1;

        int ate_item = (next_r == item_r && next_c == item_c);
        int hit_self = 0;
        int collision_limit = ate_item ? snake_len : snake_len - 1;
        for (int i = 0; i < collision_limit; ++i) {
            if (snake_r[i] == next_r && snake_c[i] == next_c) {
                hit_self = 1;
                break;
            }
        }
        if (hit_self) {
            move_cursor(1, 3);
            printf("= game over (self collision) | q=quit");
            fflush(stdout);
            break;
        }

        int old_tail_r = snake_r[snake_len - 1];
        int old_tail_c = snake_c[snake_len - 1];
        if (ate_item && snake_len < MAX_SNAKE_LEN) {
            snake_len++;
            snake_r[snake_len - 1] = old_tail_r;
            snake_c[snake_len - 1] = old_tail_c;
        }

        for (int i = snake_len - 1; i > 0; --i) {
            snake_r[i] = snake_r[i - 1];
            snake_c[i] = snake_c[i - 1];
        }
        snake_r[0] = next_r;
        snake_c[0] = next_c;
        r = snake_r[0];
        c = snake_c[0];

        if (!ate_item) {
            draw_char(old_tail_r, old_tail_c, ' ');
        }

        // Item spawn only: show item and move it when snake reaches it.
        if (r == item_r && c == item_c) {
            do {
                item_r = 2 + rand() % (rows - 1);
                item_c = 1 + rand() % cols;
            } while (item_r == r && item_c == c);
        }

        draw_char(item_r, item_c, '*');
        for (int i = snake_len - 1; i > 0; --i) {
            draw_char(snake_r[i], snake_c[i], 'o');
        }
        draw_char(snake_r[0], snake_c[0], '@');

        move_cursor(1, 3);
        printf("= quit | arrows/WASD = change direction | auto move | %dx%d", cols, rows);
        fflush(stdout);

        usleep(200000);
    }

    show_cursor();
    clear_screen();
    return 0;
}
