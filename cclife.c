#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define min(a,b) a < b ? a : b
#define max(a,b) a > b ? a : b

#define COLOR_BACKGROUND 15
#define COLOR_SURVIVE 8
#define COLOR_BIRTH 0
#define COLOR_DEAD_STARVE 7
#define COLOR_DEAD_OVERPOPULATED 11

#define B "3"
#define S "23"
#define MIN_SPEED 1
#define MAX_SPEED 5000
#define SLEEP_TIME 200
#define SPEED_CHANGE_AMOUNT 5
#define DATA_WIDTH 8
#define BIT_CHAR ' '
#define COLS_PER_BIT 2
#define FONT_SIZE 8
#define MAX_WIDTH 1920/(FONT_SIZE/2*COLS_PER_BIT)
#define MAX_HEIGHT 1080/FONT_SIZE

typedef struct board_t {
	uint8_t *data;
	unsigned int w, h;	
} board_t;

static char life_b[10], life_s[10];
static unsigned int last_width, last_height;
static int speed;
static int cursor_x, cursor_y;
static struct timespec start_clock, end_clock;
static struct board_t board, board_buffer;

unsigned int get_aligned_width (const board_t *b)
{
	return b->w+DATA_WIDTH-(b->w%DATA_WIDTH); /* align by DATA_WIDTH bytes */
}

unsigned int get_board_size (const board_t *b)
{
	return get_aligned_width(b)*b->h/DATA_WIDTH;
}

unsigned int offset (const int x,const int y, const board_t *b)
{
	return (x+y*get_aligned_width(b))/DATA_WIDTH;
}

bool get_bit (const int x, const int y, const board_t *b)
{
	assert(b != NULL && b->data != NULL && x >= 0 && y >= 0 && x < b->w && y < b->h);
	return 1 & (b->data[offset(x,y,b)] >> (x%DATA_WIDTH));
}

void set_bit (const int x, const int y, const bool p, board_t *b)
{
	assert(b != NULL && b->data != NULL && x >= 0 && y >= 0 && x < b->w && y < b->h);
	if (get_bit(x,y,b) != p)
		b->data[offset(x,y,b)] ^= 1 << (x%DATA_WIDTH);
}

void create_board (const int w, const int h, board_t *b)
{
	if (b->data != NULL)
		free(b->data);
	b->w = w;
	b->h = h;
	assert((b->data = calloc(get_board_size(b), sizeof(uint8_t))) != NULL); /* calloc failed */
}

void resize_life (void)
{
	const int new_width = min(MAX_WIDTH, COLS/COLS_PER_BIT);
	const int new_height = min(MAX_HEIGHT, LINES);
	if (new_width != last_width || new_height != last_height) {
		last_width = new_width;
		last_height = new_height;	
		create_board(new_width, new_height, &board_buffer);
		int i, j;
		for (i=0;i<new_height && i<board.h;i++)
			for (j=0;j<new_width && j<board.w;j++)
				set_bit(j, i, get_bit(j, i, &board), &board_buffer);
		create_board(new_width, new_height, &board);
		memcpy(board.data, board_buffer.data, sizeof(uint8_t)*get_board_size(&board));
	}	
}

uint8_t count_neighbors (const int x, const int y, const board_t *b)
{
	uint8_t count = 0;
	int i, tar_x, tar_y;
	#define clip_bounds(tar, max, val) for(tar=val;tar<0;tar+=max); tar=tar%max
	for (i=0;i<9;i++) {
		if (i==4)
			continue;
		/* get x, y of target neighbor looped across board */
		clip_bounds(tar_x, b->w, x-1+(i%3));
		clip_bounds(tar_y, b->h, y-1+(i/3));
		if (get_bit(tar_x, tar_y, b))
			count++;
	}
	return count;
}

void draw_life (void)
{
	int i, j, k;
	uint8_t p;
	for (i=0;i<board.h;i++)
		for (j=0;j<board.w;j++) {
			p = 5;
			if (get_bit(j, i, &board)) {
				p = count_neighbors(j, i, &board);
				if (strchr(life_b, '0'+p) != NULL)
					p=2;
				else if (strchr(life_s, '0'+p) != NULL)
					p=1;
				else if (p<3)
					p=3;
				else
					p=4;
			}
			attron(COLOR_PAIR(p));
			move(i, j*COLS_PER_BIT);
			for (k=0;k<COLS_PER_BIT;k++)
				addch(BIT_CHAR);
			attroff(COLOR_PAIR(p));
		}
}

void randomize_life (void)
{
	int i, j;
	for (i=0;i<board.h;i++)
		for (j=0;j<board.w;j++)
			set_bit(j, i, rand()%2, &board);
}

void step_life (void)
{
	memcpy(board_buffer.data, board.data, sizeof(uint8_t)*get_board_size(&board));
	int i, j;
	uint8_t n;
	bool p;
	for (i=0;i<board.h;i++)
		for (j=0;j<board.w;j++) {
			p = get_bit(j, i, &board_buffer);
			n = count_neighbors(j, i, &board_buffer);
			if(!p && strchr(life_b, '0'+n) != NULL)
				set_bit(j, i, TRUE, &board);
			else if(p && strchr(life_s, '0'+n) == NULL)
				set_bit(j, i, FALSE, &board);
		}
}

void reset_life (void)
{
	last_width = last_height = 0;
	cursor_x = cursor_y = 0;
	speed = SLEEP_TIME;
	create_board(0, 0, &board);
	resize_life();
}

void pause_life (void);
void handle_input (const int c, const bool cursor)
{
	switch (c) {
		case ERR: return;
		case 'c':
			reset_life(); /* clear */
			draw_life();
			return;
		case 'r':
			randomize_life();
			step_life();
			draw_life();
			return;
		case 'q': exit(EXIT_SUCCESS);
	}
	if (!cursor) /* game is playing */
		switch (c) {
			case 'p': pause_life(); return;
			case 'm': /* speed control */
			case 'n':
				speed += c=='n' ? SPEED_CHANGE_AMOUNT : -SPEED_CHANGE_AMOUNT;
				speed = max(speed, MIN_SPEED);
				speed = min(speed, MAX_SPEED);
				return;
		}
	else { /* game is paused */
		#define move_cursor(x, y) move((cursor_y=y), (cursor_x=x))
		switch (c) {
			case 'a': /* add or delete at cursor */
			case 'd':
				set_bit(cursor_x/COLS_PER_BIT, cursor_y, c=='a', &board);
				draw_life();
				move_cursor(cursor_x, cursor_y);
				return;
			case 'h':
			case KEY_LEFT: move_cursor(max(0,cursor_x-COLS_PER_BIT), cursor_y); return;
			case 'l':
			case KEY_RIGHT: move_cursor(min(board.w*COLS_PER_BIT, cursor_x+COLS_PER_BIT), cursor_y); return;
			case 'k':
			case KEY_UP: move_cursor(cursor_x, max(0, cursor_y-1)); return;
			case 'j':
			case KEY_DOWN: move_cursor(cursor_x, min(board.h, cursor_y+1)); return;
		}
	}
}

void pause_life (void)
{
	curs_set(1); /* turn on cursor */
	move(min(cursor_y, board.h),min(cursor_x, board.w));
	int input;
	while ((input = getch()) != 'p')
		handle_input(input, TRUE);
	curs_set(0);
}

void init (char *rules)
{
	srand(time(NULL));
	if (rules != NULL) {
		int i;
		char *p = rules;
		for(i=0;i<13 && *p;i++, p++)
			if (*p >= 'a' && *p <= 'z')
				*p += 'A'-'a';
		if (sscanf(rules, "B%9[0-8]/S%9[0-8]", life_b, life_s) < 1) {
			fprintf(stderr, "Error: Could not parse rules.\nCorrect format: B3/S23");
			exit(EXIT_FAILURE);
		}
	} else {
		strcpy(life_b, B);
		strcpy(life_s, S);
	}
	memset(&start_clock, 0, sizeof(struct timespec));
	memset(&board_buffer, 0, sizeof(struct board_t));
	memset(&board, 0, sizeof(struct board_t));
	/* curses config */
	initscr();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	curs_set(0);
	noecho();
	if(has_colors()) {
		start_color();
		init_pair(1, COLOR_BACKGROUND, COLOR_SURVIVE);
		init_pair(2, COLOR_BACKGROUND, COLOR_BIRTH);
		init_pair(3, COLOR_BACKGROUND, COLOR_DEAD_STARVE);
		init_pair(4, COLOR_BACKGROUND, COLOR_DEAD_OVERPOPULATED);
		init_pair(5, COLOR_BACKGROUND, COLOR_BACKGROUND);
	}
	/* start a new game with random data */
	reset_life();
	randomize_life();
}

void cleanup_life (void)
{
	if (board.data != NULL) 
		free(board.data);
	if (board_buffer.data != NULL)
		free(board_buffer.data);
	endwin(); /* close curses */
}

void run_life (void)
{
	int input = ERR;
	do {
		clock_gettime(CLOCK_MONOTONIC, &end_clock);	
		if ((end_clock.tv_sec-start_clock.tv_sec)*1000 + (end_clock.tv_nsec-start_clock.tv_nsec)/1000000 > speed) {
			start_clock = end_clock; 
			resize_life();
			step_life();
			draw_life();
			refresh();
		}
		handle_input(input, FALSE);
	} while ((input=getch()) != 'q');
}

void handle_signal (const int signal)
{
	exit(EXIT_SUCCESS);
}

int main (int argc, char **argv)
{
	atexit(cleanup_life);
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	init(argc > 1 ?  argv[1] : NULL);
	run_life();
	return EXIT_SUCCESS;
}
