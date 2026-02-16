#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

#define LOGO_MAX_ROWS 20
#define LOGO_MAX_LINE 256
#define MAX_DROPS     1024

struct drop {
    int target_row;
    int col;
    int current_row;
    int delay;
    int landed;
    int line_idx;
    int byte_off;
    int byte_len;
};

/** UTF-8 선행 바이트로부터 문자의 바이트 길이를 반환한다. */
static int utf8_char_len(unsigned char c) {
    if (c < 0x80)       return 1;
    if (c < 0xC0)       return 1; /* continuation — shouldn't happen at start */
    if (c < 0xE0)       return 2;
    if (c < 0xF0)       return 3;
    return 4;
}

/** 빗방울 위치에 이미 착지한 drop이 있는지 확인한다. */
static int has_landed_at(struct drop *drops, int ndrop, int row, int col) {
    for (int i = 0; i < ndrop; i++) {
        if (drops[i].landed && drops[i].target_row == row && drops[i].col == col)
            return 1;
    }
    return 0;
}

/** 로고 전체를 화면에 한꺼번에 그린다. */
static void draw_full_logo(char logo[][LOGO_MAX_LINE], struct drop *drops,
                           int ndrop) {
    for (int d = 0; d < ndrop; d++) {
        if (!drops[d].landed && drops[d].current_row > 0) {
            draw_char(drops[d].current_row, drops[d].col, ' ');
        }
        move_cursor(drops[d].target_row, drops[d].col);
        fwrite(&logo[drops[d].line_idx][drops[d].byte_off],
               1, drops[d].byte_len, stdout);
    }
    fflush(stdout);
}

void draw_welcome(void) {
    /* 1. 로고 파일 읽기 */
    FILE *fp = fopen("ui/logo.txt", "r");
    if (!fp) return;

    char logo[LOGO_MAX_ROWS][LOGO_MAX_LINE];
    int logo_h = 0;
    int logo_w = 0;

    while (logo_h < LOGO_MAX_ROWS && fgets(logo[logo_h], LOGO_MAX_LINE, fp)) {
        int len = (int)strlen(logo[logo_h]);
        if (len > 0 && logo[logo_h][len - 1] == '\n') {
            logo[logo_h][len - 1] = '\0';
            len--;
        }
        /* 디스플레이 너비 계산 (UTF-8 고려) */
        int dcols = 0;
        for (int i = 0; i < len; ) {
            i += utf8_char_len((unsigned char)logo[logo_h][i]);
            dcols++;
        }
        if (dcols > logo_w) logo_w = dcols;
        logo_h++;
    }
    fclose(fp);

    if (logo_h == 0) return;

    /* 2. 화면 중앙 오프셋 */
    int rows, cols;
    get_screen_size(&rows, &cols);
    int off_r = (rows - logo_h) / 2;
    int off_c = (cols - logo_w) / 2;
    if (off_r < 1) off_r = 1;
    if (off_c < 1) off_c = 1;

    /* 3. 빗방울(drop) 생성 */
    struct drop drops[MAX_DROPS];
    int ndrop = 0;
    srand((unsigned)time(NULL));

    for (int r = 0; r < logo_h && ndrop < MAX_DROPS; r++) {
        int len = (int)strlen(logo[r]);
        int i = 0;
        int dcol = 0;
        while (i < len && ndrop < MAX_DROPS) {
            int clen = utf8_char_len((unsigned char)logo[r][i]);
            if ((unsigned char)logo[r][i] != ' ') {
                drops[ndrop].target_row  = off_r + r + 1;
                drops[ndrop].col         = off_c + dcol + 1;
                drops[ndrop].current_row = 0;
                drops[ndrop].delay       = rand() % 20;
                drops[ndrop].landed      = 0;
                drops[ndrop].line_idx    = r;
                drops[ndrop].byte_off    = i;
                drops[ndrop].byte_len    = clen;
                ndrop++;
            }
            dcol++;
            i += clen;
        }
    }

    if (ndrop == 0) return;

    /* 4. 애니메이션 루프 */
    clear_screen();
    const char glyphs[] = "!@#$%^&*~+=<>?/|";
    int nglyphs = (int)sizeof(glyphs) - 1;
    int all_landed = 0;

    while (!all_landed) {
        all_landed = 1;

        for (int d = 0; d < ndrop; d++) {
            if (drops[d].landed) continue;

            if (drops[d].delay > 0) {
                drops[d].delay--;
                all_landed = 0;
                continue;
            }

            /* 이전 위치 지우기 (착지된 문자 보호) */
            if (drops[d].current_row > 0 &&
                drops[d].current_row < drops[d].target_row) {
                if (!has_landed_at(drops, ndrop,
                                   drops[d].current_row, drops[d].col))
                    draw_char(drops[d].current_row, drops[d].col, ' ');
            }

            drops[d].current_row++;

            if (drops[d].current_row >= drops[d].target_row) {
                /* 착지: 최종 문자 출력 */
                move_cursor(drops[d].target_row, drops[d].col);
                fwrite(&logo[drops[d].line_idx][drops[d].byte_off],
                       1, drops[d].byte_len, stdout);
                drops[d].landed = 1;
            } else {
                /* 랜덤 문자로 빗방울 표현 */
                if (!has_landed_at(drops, ndrop,
                                   drops[d].current_row, drops[d].col))
                    draw_char(drops[d].current_row, drops[d].col,
                              glyphs[rand() % nglyphs]);
                all_landed = 0;
            }
        }

        fflush(stdout);
        usleep(30000);

        /* 키 입력 → 애니메이션 스킵 */
        if (read_key() != -1) {
            draw_full_logo(logo, drops, ndrop);
            clear_screen();
            return;
        }
    }

    /* 5. logo 다음 줄에 Press any key to continue... 출력 */
    move_cursor(off_r + logo_h + 2, (cols - strlen("Press any key to continue...")) / 2);
    printf("Press any key to continue...");
    fflush(stdout);

    /* 6. 모든 drop 착지 → 키 입력 대기 */
    while (read_key() == -1)
        usleep(30000);

    clear_screen();
}
