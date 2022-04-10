#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

// Time related
#define MILLISEC_IN_SEC 1000
#define NANOSEC_IN_MILLISEC 1000000
#define INTERVAL 2000

// Dimension
#define BOARD_WIDTH 15
#define BOARD_HEIGHT 15
#define BOARDER_WIDTH 1
#define UI_HEIGHT 10

// For the game board, draw 1 unit (body/food/border) with 1 chars
#define BUFFER_WIDTH ((BOARD_WIDTH + BOARDER_WIDTH * 2) * 1 + 1) // Plus 1 for newline char
#define BUFFER_HEIGHT (BOARD_HEIGHT + BOARDER_WIDTH * 2 + UI_HEIGHT)

// Draw related
#define BOARDER_HORIZONTAL_LINE '-'
#define BOARDER_VERTICAL_LINE '|'
#define SNAKE_HEAD 'O'
#define SNAKE_BODY '#'

typedef unsigned long millisec_t;
typedef struct snake_body snake_body_t;
struct snake_body {
    snake_body_t* next;

    int x;
    int y;
};

typedef enum {
    Right = 0,
    Left,
    Down,
    Up
} direction_t;

typedef struct {
    snake_body_t* head;
    snake_body_t* tail;

    size_t length;
    direction_t direction;
} snake_t;

///////////////
// Game Data
///////////////
// Screen buffer
char *buffer;
struct timespec prev_time = {0, 0};
snake_t snake;

//////////////////////
// Draw family APIs //
//////////////////////
void draw_clear_cli() {
    system("clear");
}

void draw_clear_buffer() {
    static char* buffer_empty_line = NULL;
    // init once
    if (!buffer_empty_line) {
        // plus 1 because we add newline at the end
        buffer_empty_line = calloc(BUFFER_WIDTH, sizeof(char));
        memset(buffer_empty_line, ' ', BUFFER_WIDTH);
        buffer_empty_line[BUFFER_WIDTH - 1] = '\n';
    }

    for (int row = 0; row < BUFFER_HEIGHT; row++) {
        memcpy(buffer + row * BUFFER_WIDTH, buffer_empty_line, BUFFER_WIDTH);
    }
}

void draw_snake() {
    int offset_x = BOARDER_WIDTH;
    int offset_y = BOARDER_WIDTH;

    snake_body_t* cur = snake.head;
    int head_drawn = 0;
    while (cur != NULL) {
        int idx = (cur->x + offset_x) * BUFFER_WIDTH + (cur->y + offset_y);
        buffer[idx] = head_drawn ? SNAKE_BODY : SNAKE_HEAD;
        head_drawn = 1;
        cur = cur->next;
    }
}

void draw_boarder() {
    int offset_x = 0;
    int offset_y = 0;
    // Draw horizontal
    // Top
    offset_x = 0;
    offset_y = 1;
    for (int y = 0; y < BOARD_WIDTH; y++) {
        buffer[y + offset_y] = BOARDER_HORIZONTAL_LINE;
    }

    // Bottom
    offset_x = 0;
    offset_y = 1;
    int x = BOARD_HEIGHT + BOARDER_WIDTH;
    for (int y = 0; y < BOARD_WIDTH; y++) {
        buffer[(x * BUFFER_WIDTH) + (y + offset_y)] = BOARDER_HORIZONTAL_LINE;
    }

    // Draw vertical
    // Left
    offset_x = 1;
    for (int x = 0; x < BOARD_HEIGHT; x++) {
        buffer[(x + offset_x) * BUFFER_WIDTH] = BOARDER_VERTICAL_LINE;
    }

    // Right
    offset_x = 1;
    int y = BOARD_HEIGHT + BOARDER_WIDTH;
    for (int x = 0; x < BOARD_HEIGHT; x++) {
        buffer[(x + offset_x) * BUFFER_WIDTH + y] = BOARDER_VERTICAL_LINE;
    }
}

void draw_to_buffer(int x, int y, char c) {

}

void draw_buffer_to_cli() {
    for (int i = 0; i < BUFFER_HEIGHT; i++) {
        fwrite(buffer + i * BUFFER_WIDTH, sizeof(char), BUFFER_WIDTH, stdout);
    }
}

void draw_string_to_ui() {

}

//////////////////////
// End Draw family APIs //
//////////////////////

void update() {

}

void draw() {
    // Draw to buffer
    draw_clear_buffer();
    draw_snake();
    draw_boarder();

    // Print buffer to cli
    draw_clear_cli();
    draw_buffer_to_cli();
}

void init() {
    if ((buffer = (char *) calloc(BUFFER_WIDTH * BUFFER_HEIGHT,
                                  sizeof(char))) == NULL) {
        // Allocation failed
        fprintf(stderr, "Failed to allocate buffer\n");
        assert(0);
    }

    draw_clear_buffer();

    // Init game
    // Init snake with 3 body
    snake.direction = Right;
    snake_body_t* body1 = (snake_body_t*)malloc(sizeof(snake_body_t));
    snake_body_t* body2 = (snake_body_t*)malloc(sizeof(snake_body_t));
    snake_body_t* body3 = (snake_body_t*)malloc(sizeof(snake_body_t));
    body1->next = body2;
    body2->next = body3;
    body3->x = body2->x = body1->x = BOARD_WIDTH / 2;
    body1->y = BOARD_HEIGHT / 2;
    body2->y = body1->y - 1;
    body3->y = body2->y - 1;

    snake.length = 3;

    snake.head = body1;
    snake.tail = body3;
}

void cleanup() {
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }

    // cleanup snake
    snake_body_t* cur = snake.head;
    while (cur != NULL) {
        snake_body_t* tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    snake.length = 0;
    snake.head = snake.tail = NULL;
}

millisec_t to_millisec(struct timespec time) {
    return time.tv_sec * MILLISEC_IN_SEC + time.tv_nsec / NANOSEC_IN_MILLISEC;
}

void to_timespec(millisec_t millisec, struct timespec *timespec) {
    timespec->tv_nsec = (millisec % MILLISEC_IN_SEC) * NANOSEC_IN_MILLISEC;
    timespec->tv_sec = millisec / MILLISEC_IN_SEC;
}

millisec_t gen_wait_time(millisec_t elapsed_time) {
    if (INTERVAL <= elapsed_time) {
        return 0;
    }
    return INTERVAL - elapsed_time;
}

void start_timing() {
    clock_gettime(CLOCK_MONOTONIC, &prev_time);
}

void game_sleep() {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    // 1. generate elapsed time
    millisec_t prev_milli = to_millisec(prev_time);
    millisec_t current_milli = to_millisec(current_time);
//    printf("Elapsed time: %lu\n", elapsed_time);

    // 2. generate sleep time
    struct timespec req = {0, 0};
    to_timespec(gen_wait_time(current_milli - prev_milli), &req);
    // printf("Wait time: %ld %ld\n", req.tv_sec, req.tv_nsec);

    // 3. sleep
    nanosleep(&req, NULL);
}

int main() {
    init();

    // main loop
    int shouldExit = 2;
    while (shouldExit) {
        start_timing();

        shouldExit--;

        update();
        draw();

        game_sleep();
    }

    cleanup();
    return 0;
}
