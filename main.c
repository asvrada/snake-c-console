#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

////////////
// Helper //
////////////
int rand_max(int max)
{
    int ret = rand();
    return (int)((float)ret / RAND_MAX * max);
}

// Time related
#define MILLISEC_IN_SEC 1000
#define NANOSEC_IN_MILLISEC 1000000
#define INTERVAL 700

// Dimension
#define BOARD_WIDTH 15
#define BOARD_HEIGHT 15
#define BOARDER_WIDTH 1
#define UI_HEIGHT 5

// For the game board, draw 1 unit (body/food/border) with 1 chars
#define BUFFER_WIDTH ((BOARD_WIDTH + BOARDER_WIDTH * 2) * 1 + 1) // Plus 1 for newline char
#define BUFFER_HEIGHT (BOARD_HEIGHT + BOARDER_WIDTH * 2)

// Draw related
#define BOARDER_HORIZONTAL_LINE '-'
#define BOARDER_VERTICAL_LINE '|'
#define SNAKE_HEAD 'O'
#define SNAKE_BODY '#'
#define CHAR_FOOD 'Q'

// Score
#define SCORE_FOOD 100

typedef unsigned long millisec_t;

typedef struct coord coord_t;
struct coord
{
    int x;
    int y;
};


typedef struct snake_body snake_body_t;
struct snake_body
{
    snake_body_t *next;
    snake_body_t *prev;

    coord_t pos;
};

typedef enum
{
    Null = 0,
    Up,
    Left,
    Down,
    Right
} direction_t;

typedef struct
{
    snake_body_t *head;
    snake_body_t *tail;

    size_t length;
    direction_t direction;
} snake_t;

///////////////
// Game Data
///////////////
int should_exit = 0;
// Screen buffer
char *buffer;
// array of string
char **ui;
int ui_cur_row = 0;
struct timespec prev_time = {0, 0};
snake_t snake;
// If snake eat a food, grow tail
coord_t prev_tail = {-1, -1};
coord_t food = {-1, -1};
int food_eaten = 0;

//////////////////////
// Draw family APIs //
//////////////////////
void draw_clear_cli()
{
    system("clear");
    // printf("\n\n\n\n\n");
}

void draw_clear_buffer()
{
    static char *buffer_empty_line = NULL;
    // init once
    if (!buffer_empty_line)
    {
        // plus 1 because we add newline at the end
        buffer_empty_line = calloc(BUFFER_WIDTH, sizeof(char));
        memset(buffer_empty_line, ' ', BUFFER_WIDTH);
        buffer_empty_line[BUFFER_WIDTH - 1] = '\n';
    }

    for (int row = 0; row < BUFFER_HEIGHT; row++)
    {
        memcpy(buffer + row * BUFFER_WIDTH, buffer_empty_line, BUFFER_WIDTH);
    }
    
    // clear ui
    for (int i = 0; i < UI_HEIGHT; i++)
    {
        if (ui[i] != NULL)
        {
            free(ui[i]);
            ui[i] = NULL;
        }
    }
    ui_cur_row = 0;
}

void draw_snake()
{
    int offset_x = BOARDER_WIDTH;
    int offset_y = BOARDER_WIDTH;

    snake_body_t *cur = snake.head;
    int head_drawn = 0;
    while (cur != NULL)
    {
        int idx = (cur->pos.x + offset_x) * (BUFFER_WIDTH) + (cur->pos.y + offset_y);
        buffer[idx] = head_drawn ? SNAKE_BODY : SNAKE_HEAD;
        head_drawn = 1;
        cur = cur->next;
    }
}

void draw_food()
{
    int offset_x = BOARDER_WIDTH;
    int offset_y = BOARDER_WIDTH;

    if (food.x != -1 && food.y != -1)
    {
        int idx = (food.x + offset_x) * (BUFFER_WIDTH) + (food.y + offset_y);
        assert(idx < (BUFFER_WIDTH * BUFFER_HEIGHT));
        buffer[idx] = CHAR_FOOD;
    }
}

void draw_boarder()
{
    int offset_x = 0;
    int offset_y = 0;
    // Draw horizontal
    // Top
    offset_x = 0;
    offset_y = 1;
    for (int y = 0; y < BOARD_WIDTH; y++)
    {
        buffer[y + offset_y] = BOARDER_HORIZONTAL_LINE;
    }

    // Bottom
    offset_x = 0;
    offset_y = 1;
    int x = BOARD_HEIGHT + BOARDER_WIDTH;
    for (int y = 0; y < BOARD_WIDTH; y++)
    {
        buffer[(x * BUFFER_WIDTH) + (y + offset_y)] = BOARDER_HORIZONTAL_LINE;
    }

    // Draw vertical
    // Left
    offset_x = 1;
    for (int x = 0; x < BOARD_HEIGHT; x++)
    {
        buffer[(x + offset_x) * BUFFER_WIDTH] = BOARDER_VERTICAL_LINE;
    }

    // Right
    offset_x = 1;
    int y = BOARD_HEIGHT + BOARDER_WIDTH;
    for (int x = 0; x < BOARD_HEIGHT; x++)
    {
        buffer[(x + offset_x) * BUFFER_WIDTH + y] = BOARDER_VERTICAL_LINE;
    }
}

void draw_ui()
{
    // current score
    char* msg_cur_score = (char*)malloc(sizeof(char) * 100);
    sprintf(msg_cur_score, "Score: %d\n", food_eaten * SCORE_FOOD);
    ui[ui_cur_row] = msg_cur_score;
    ui_cur_row += 1;
}

void draw_message_to_cli()
{
    // start of message section in buffer
    for (int i = 0; i < ui_cur_row && ui[i]; i++) {
        printf("%s\n", ui[i]);
    }
}

void draw_buffer_to_cli()
{
    for (int i = 0; i < BUFFER_HEIGHT; i++)
    {
        fwrite(buffer + i * BUFFER_WIDTH, sizeof(char), BUFFER_WIDTH, stdout);
    }
}

void draw_string_to_ui()
{
}

//////////////////////
// End Draw family APIs //
//////////////////////

int abs(int a) {
    return a >= 0 ? a : -1;
}

void move_snake(direction_t dir)
{
    // determine actual snake direction
    if (dir != Null && dir != snake.direction) {
        // disallow turning 180 degree
        if (abs(dir - snake.direction) != 2) {
            snake.direction = dir;
        }
    }

    snake_body_t *old_head = snake.head;
    snake_body_t *old_tail = snake.tail;

    // update prev_tail
    prev_tail = old_tail->pos;

    // Move tail to head
    snake.tail = old_tail->prev;
    snake.head = old_tail;

    snake.tail->next = NULL;

    old_tail->prev = NULL;
    old_tail->next = old_head;

    old_head->prev = snake.head;

    // Update tail position
    switch (snake.direction)
    {
    case Right:
        old_tail->pos.x = old_head->pos.x;
        old_tail->pos.y = old_head->pos.y + 1;
        if (old_tail->pos.y >= BOARD_WIDTH)
        {
            should_exit = 1;
        }
        break;
    case Left:
        old_tail->pos.x = old_head->pos.x;
        old_tail->pos.y = old_head->pos.y - 1;
        if (old_tail->pos.y < 0)
        {
            should_exit = 1;
        }
        break;
    case Up:
        old_tail->pos.x = old_head->pos.x - 1;
        old_tail->pos.y = old_head->pos.y;
        if (old_tail->pos.x < 0)
        {
            should_exit = 1;
        }
        break;
    case Down:
        old_tail->pos.x = old_head->pos.x + 1;
        old_tail->pos.y = old_head->pos.y;
        if (old_tail->pos.x >= BOARD_HEIGHT)
        {
            should_exit = 1;
        }
        break;
    default:
        // Impossible
        break;
    }

    // check if head collide with body
    snake_body_t* cur = snake.head->next;
    while (cur)
    {
        if (cur->pos.x == snake.head->pos.x && cur->pos.y == snake.head->pos.y)
        {
            should_exit = 1;
            return;
        }
        cur = cur->next;
    }
}

void generate_food()
{
    if (food.x == -1 && food.y == -1)
    {
        // randomly generate a food
        int count = BOARD_HEIGHT * BOARD_WIDTH - snake.length;
        int idx_food = rand_max(count);
        // convert to x and y
        int x = idx_food / BOARD_WIDTH;
        int y = idx_food % BOARD_WIDTH;
        food.x = x;
        food.y = y;
        // move food to an empty space if overlap with snake
        // todo
    }
}

// Return 1 if snake eat food, 0 otherwise
int grow_snake()
{
    coord_t head = snake.head->pos;
    if (head.x == food.x && head.y == food.y)
    {
        // grow
        snake_body_t *new_body = (snake_body_t*)malloc(sizeof(snake_body_t));
        new_body->pos = prev_tail;

        // append to tail of snake
        new_body->prev = snake.tail;
        snake.tail->next = new_body;

        snake.tail = new_body;

        snake.length += 1;
        food.x = food.y = -1;
        generate_food();

        // update score
        food_eaten += 1;
    }
}

void update(direction_t dir)
{
    generate_food();
    move_snake(dir);
    grow_snake();
}

void draw()
{
    draw_clear_buffer();

    // Draw to buffer
    draw_snake();
    draw_food();
    draw_boarder();
    draw_ui();

    // Print buffer to cli
    draw_clear_cli();
    draw_buffer_to_cli();
    draw_message_to_cli();

    // debug
    // printf("%d,%d %d,%d %d,%d\n",
    // snake.head->x,snake.head->y,
    // snake.head->next->x,snake.head->next->y,
    // snake.head->next->next->x,snake.head->next->next->y);
    fflush(stdout);
}

void init()
{
    if ((buffer = (char *)calloc(BUFFER_WIDTH * BUFFER_HEIGHT,
                                 sizeof(char))) == NULL)
    {
        // Allocation failed
        fprintf(stderr, "Failed to allocate buffer\n");
        assert(0);
    }

    // init ui
    ui = (char**)calloc(UI_HEIGHT, sizeof(char*));

    draw_clear_buffer();

    // Init game
    // Init snake with 3 body
    snake.direction = Right;
    snake_body_t *body1 = (snake_body_t *)malloc(sizeof(snake_body_t));
    snake_body_t *body2 = (snake_body_t *)malloc(sizeof(snake_body_t));
    snake_body_t *body3 = (snake_body_t *)malloc(sizeof(snake_body_t));

    body1->next = body2;
    body2->next = body3;
    body3->next = NULL;

    body3->prev = body2;
    body2->prev = body1;
    body1->prev = NULL;

    body3->pos.x = body2->pos.x = body1->pos.x = BOARD_WIDTH / 2;
    body1->pos.y = BOARD_HEIGHT / 2;
    body2->pos.y = body1->pos.y - 1;
    body3->pos.y = body2->pos.y - 1;

    snake.length = 3;

    snake.head = body1;
    snake.tail = body3;

    // random seed
    srand(time(NULL));
}

void cleanup()
{
    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }

    // cleanup snake
    snake_body_t *cur = snake.head;
    while (cur != NULL)
    {
        snake_body_t *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    snake.length = 0;
    snake.head = snake.tail = NULL;
}

millisec_t to_millisec(struct timespec time)
{
    return time.tv_sec * MILLISEC_IN_SEC + time.tv_nsec / NANOSEC_IN_MILLISEC;
}

void to_timespec(millisec_t millisec, struct timespec *timespec)
{
    timespec->tv_nsec = (millisec % MILLISEC_IN_SEC) * NANOSEC_IN_MILLISEC;
    timespec->tv_sec = millisec / MILLISEC_IN_SEC;
}

millisec_t gen_wait_time(millisec_t elapsed_time)
{
    if (INTERVAL <= elapsed_time)
    {
        return 0;
    }
    return INTERVAL - elapsed_time;
}

void start_timing()
{
    clock_gettime(CLOCK_MONOTONIC, &prev_time);
}

void game_sleep()
{
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

// We use WASD to control snake
// User input: W, S, A, D
direction_t get_dir_input()
{
    char c = getchar();
    direction_t dir = Null;
    switch (c)
    {
    // W, w
    case 87:
    case 119:
        dir = Up;
        break;
    // S, s
    case 83:
    case 115:
        dir = Down;
        break;
    // A, a
    case 65:
    case 97:
        dir = Left;
        break;
    // D, d
    case 68:
    case 100:
        dir = Right;
        break;

    // newline, enter
    case 10:
        return Null;
    default:
        // todo: do something?
        break;
    }

    char newline = getchar();
    return dir;
}

int main()
{
    init();
    // main loop
    while (!should_exit)
    {
        // get input
        int dir = get_dir_input();
        update(dir);
        draw();
    }

    cleanup();
    return 0;
}
