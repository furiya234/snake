#ifndef UI_H
#define UI_H

#define KEY_UP    1000
#define KEY_DOWN  1001
#define KEY_LEFT  1002
#define KEY_RIGHT 1003
#define KEY_QUIT  'q'

/** 화면을 지우고 커서를 좌상단으로 이동한다. */
void clear_screen(void);

/**
 * 터미널 크기(행, 열)를 가져온다.
 * @param rows 행 수를 저장할 포인터
 * @param cols 열 수를 저장할 포인터
 */
void get_screen_size(int *rows, int *cols);

/**
 * 커서를 지정 위치로 이동한다 (1-based).
 * @param row 행 번호
 * @param col 열 번호
 */
void move_cursor(int row, int col);

/**
 * 지정 위치에 문자 하나를 출력한다.
 * @param row 행 번호
 * @param col 열 번호
 * @param c   출력할 문자
 */
void draw_char(int row, int col, char c);

/** 커서를 숨긴다. */
void hide_cursor(void);

/** 커서를 다시 표시한다. */
void show_cursor(void);

/**
 * raw 모드로 진입한다.
 * - 에코 끔: 키를 눌러도 화면에 문자가 표시되지 않는다.
 * - 논블로킹: 입력이 없으면 기다리지 않고 즉시 리턴한다.
 * atexit()으로 disable_raw_mode를 등록하여 종료 시 자동 복원한다.
 */
void enable_raw_mode(void);

/** 터미널을 원래 설정으로 복원한다. */
void disable_raw_mode(void);

/**
 * 키 입력을 논블로킹으로 읽는다.
 * 입력이 없으면 기다리지 않고 즉시 -1을 반환한다.
 * 화살표 키는 KEY_UP/KEY_DOWN/KEY_LEFT/KEY_RIGHT 상수를 반환한다.
 * @return 읽은 키 값. 입력이 없으면 -1.
 */
int  read_key(void);

/**
 * 웰컴 화면을 표시한다.
 * 글자가 비처럼 떨어진 뒤 ui/logo.txt의 ASCII 아트가 나타난다.
 * 아무 키나 누르면 종료한다.
 */
void draw_welcome(void);

/**
 * 게임 오버 화면을 표시한다.
 * 화면 중앙에 "GAME OVER"와 점수를 출력하고 키 입력을 대기한다.
 * @param score 표시할 점수
 */
void draw_game_over(int score);

#endif
