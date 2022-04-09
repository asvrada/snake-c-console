#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

typedef unsigned long millisec_t;
#define MILLISEC_IN_SEC 1000
#define NANOSEC_IN_MILLISEC 1000000
#define INTERVAL 2000

#define BOARD_WIDTH 15
#define BOARD_HEIGHT 15
#define BOARDER_WIDTH 1
#define UI_HEIGHT 10

// For the game board, draw 1 unit (body/food/border) with 1 chars
#define BUFFER_WIDTH ((BOARD_WIDTH + BOARDER_WIDTH * 2) * 1 + 1) // Plus 1 for newline char
#define BUFFER_HEIGHT (BOARD_HEIGHT + BOARDER_WIDTH * 2 + UI_HEIGHT)

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
        memset(buffer_empty_line, '#', BUFFER_WIDTH);
        buffer_empty_line[BUFFER_WIDTH - 1] = '\n';
    }

    for (int row = 0; row < BUFFER_HEIGHT; row++) {
        memcpy(buffer + row * BUFFER_WIDTH, buffer_empty_line, BUFFER_WIDTH);
    }
}

void draw_board_to_buffer() {

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
    snake.length = 3;
}

void cleanup() {
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
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
