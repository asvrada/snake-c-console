#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <poll.h>

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

// For the game board, draw 1 unit (body/food/border) with 1 chars
#define BUFFER_WIDTH ((BOARD_WIDTH + BOARDER_WIDTH * 2) * 1 + 1) // Plus 1 for newline char
#define BUFFER_HEIGHT (BOARD_HEIGHT + BOARDER_WIDTH * 2)

// Draw related
#define BOARDER_HORIZONTAL_LINE '-'
#define BOARDER_VERTICAL_LINE '|'
#define SNAKE_HEAD 'O'
#define SNAKE_BODY '#'
#define CHAR_FOOD 'F'

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
snake_t snake;
// 1: snake, 2: food
int *buffer_board;
// If snake eat a food, grow tail
coord_t prev_tail = {-1, -1};
coord_t food = {-1, -1};
int food_eaten = 0;

coord_t to_board_xy(int idx)
{
    int x = idx / BOARD_WIDTH;
    int y = idx % BOARD_WIDTH;

    coord_t pos = {x, y};

    return pos;
}

int to_board_idx(int x, int y)
{
    return BOARD_WIDTH * x + y;
}

//////////////////////
// Draw family APIs //
//////////////////////
void draw_clear_cli()
{
    system("clear");
}

void draw_clear_buffer()
{
    for (int row = 0; row < BUFFER_HEIGHT; row++)
    {
        memset(buffer + row * BUFFER_WIDTH, ' ', BUFFER_WIDTH);
        buffer[(row + 1) * BUFFER_WIDTH - 1] = '\n';
    }
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

void draw_message_to_cli()
{
    // draw score
    printf("Score: %d\n", food_eaten * SCORE_FOOD);
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

int abs(int a)
{
    return a >= 0 ? a : -1;
}

void move_snake(direction_t dir)
{
    // determine actual snake direction
    if (dir != Null && dir != snake.direction)
    {
        // disallow turning 180 degree
        if (abs(dir - snake.direction) != 2)
        {
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

    // update buffer_board
    buffer_board[to_board_idx(prev_tail.x, prev_tail.y)] = 0;
    buffer_board[to_board_idx(old_tail->pos.x, old_tail->pos.y)] = 1;

    // check if head collide with body
    snake_body_t *cur = snake.head->next;
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

        int idx_cur = 0;
        while (idx_food--)
        {
            idx_cur++;
            while (buffer_board[idx_cur])
            {
                idx_cur++;
            }
        }

        // convert to x and y
        int x = idx_cur / BOARD_WIDTH;
        int y = idx_cur % BOARD_WIDTH;
        food.x = x;
        food.y = y;
    }
}

// Return 1 if snake eat food, 0 otherwise
int grow_snake()
{
    coord_t head = snake.head->pos;
    if (head.x == food.x && head.y == food.y)
    {
        // grow
        snake_body_t *new_body = (snake_body_t *)malloc(sizeof(snake_body_t));
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

    if ((buffer_board = (int *)calloc(BOARD_WIDTH * BOARD_HEIGHT,
                                      sizeof(int))) == NULL)
    {
        // Allocation failed
        fprintf(stderr, "Failed to allocate buffer_board\n");
        assert(0);
    }

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

    // init buffer_board
    buffer_board[to_board_idx(body1->pos.x, body1->pos.y)] = 1;
    buffer_board[to_board_idx(body2->pos.x, body2->pos.y)] = 1;
    buffer_board[to_board_idx(body3->pos.x, body3->pos.y)] = 1;

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

    if (buffer_board)
    {
        free(buffer_board);
        buffer_board = NULL;
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

direction_t map_input(char c)
{
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

    return dir;
}

// We use WASD to control snake
// User input: W, S, A, D
direction_t get_dir_input()
{
    char c = getchar();
    direction_t dir = map_input(c);
    // consume return key
    char newline = getchar();
    return dir;
}

int poll_timeout(int millisec_timeout)
{
    struct pollfd fd[] = {{0, POLLIN, 0}};
    int count = poll(fd, 1, millisec_timeout);
    // printf("ret poll: %d revents: %d\n", ret, fd[0].revents);

    return count;
}

direction_t get_dir_input_timeout(int millisec_timeout)
{
    int count = poll_timeout(millisec_timeout);
    if (count == 0)
    {
        // user failed to input something before timeout
        return Null;
    }
    char c = getchar();
    direction_t dir = map_input(c);
    if (dir != Null)
    {
        // consume return key
        char newline = getchar();
    }
    return dir;
}

int main()
{
    init();
    generate_food();
    draw();

    // main loop
    while (!should_exit)
    {
        // get input
        int dir = get_dir_input_timeout(500);
        update(dir);
        draw();
    }

    cleanup();
    return 0;
}
