#include "ui/ui.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    int rows, cols;
    get_screen_size(&rows, &cols);

    enable_raw_mode();
    hide_cursor();
    clear_screen();

    draw_char(rows / 2, cols / 2, '@');
    draw_char(1, 1, 'q');
    move_cursor(1, 3);
    printf("= quit | arrows = move | %dx%d", cols, rows);
    fflush(stdout);

    int r = rows / 2, c = cols / 2;

    while (1) {
        int key = read_key();
        if (key == KEY_QUIT) break;

        if (key == KEY_UP || key == KEY_DOWN ||
            key == KEY_LEFT || key == KEY_RIGHT) {
            draw_char(r, c, ' ');

            if (key == KEY_UP    && r > 1)    r--;
            if (key == KEY_DOWN  && r < rows)  r++;
            if (key == KEY_LEFT  && c > 1)    c--;
            if (key == KEY_RIGHT && c < cols)  c++;

            draw_char(r, c, '@');
            fflush(stdout);
        }

        usleep(16000);
    }

    show_cursor();
    clear_screen();
    return 0;
}
