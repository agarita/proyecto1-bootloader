#include "config.h"

typedef unsigned char      u8;
typedef signed   char      s8;
typedef unsigned short     u16;
typedef signed   short     s16;
typedef unsigned int       u32;
typedef signed   int       s32;
typedef unsigned long long u64;
typedef signed   long long s64;

#define noreturn __attribute__((noreturn)) void

typedef enum bool {
    false,
    true
} bool;

/* Simple math */

/* A very simple and stupid exponentiation algorithm */
static inline double pow(double a, double b)
{
    double result = 1;
    while (b-- > 0)
        result *= a;
    return result;
}

/* Port I/O */
static inline u8 inb(u16 p)
{
    u8 r;
    asm("inb %1, %0" : "=a" (r) : "dN" (p));
    return r;
}

static inline void outb(u16 p, u8 d)
{
    asm("outb %1, %0" : : "dN" (p), "a" (d));
}

/* Divide by zero (in a loop to satisfy the noreturn attribute) in order to
 * trigger a division by zero ISR, which is unhandled and causes a hard reset.
 */
noreturn reset(void)
{
    volatile u8 one = 1, zero = 0;
    while (true)
        one /= zero;
}

/* Timing */

/* Return the number of CPU ticks since boot. */
static inline u64 rdtsc(void)
{
    u32 hi, lo;
    asm("rdtsc" : "=a" (lo), "=d" (hi));
    return ((u64) lo) | (((u64) hi) << 32);
}

/* Return the current second field of the real-time-clock (RTC). Note that the
 * value may or may not be represented in such a way that it should be
 * formatted in hex to display the current second (i.e. 0x30 for the 30th
 * second). */
u8 rtcs(void)
{
    u8 last = 0, sec;
    do { /* until value is the same twice in a row */
        /* wait for update not in progress */
        do { outb(0x70, 0x0A); } while (inb(0x71) & 0x80);
        outb(0x70, 0x00);
        sec = inb(0x71);
    } while (sec != last && (last = sec));
    return sec;
}

/* The number of CPU ticks per millisecond */
u64 tpms;

/* Set tpms to the number of CPU ticks per millisecond based on the number of
 * ticks in the last second, if the RTC second has changed since the last call.
 * This gets called on every iteration of the main loop in order to provide
 * accurate timing. */
void tps(void)
{
    static u64 ti = 0;
    static u8 last_sec = 0xFF;
    u8 sec = rtcs();
    if (sec != last_sec) {
        last_sec = sec;
        u64 tf = rdtsc();
        tpms = (u32) ((tf - ti) >> 3) / 125; /* Less chance of truncation */
        ti = tf;
    }
}

/* IDs used to keep separate timing operations separate */
enum timer {
    TIMER_UPDATE,
    TIMER_CLEAR,
    TIMER__LENGTH
};

u64 timers[TIMER__LENGTH] = {0};

/* Return true if at least ms milliseconds have elapsed since the last call
 * that returned true for this timer. When called on each iteration of the main
 * loop, has the effect of returning true once every ms milliseconds. */
bool interval(enum timer timer, u32 ms)
{
    u64 tf = rdtsc();
    if (tf - timers[timer] >= tpms * ms) {
        timers[timer] = tf;
        return true;
    } else return false;
}

/* Return true if at least ms milliseconds have elapsed since the first call
 * for this timer and reset the timer. */
bool wait(enum timer timer, u32 ms)
{
    if (timers[timer]) {
        if (rdtsc() - timers[timer] >= tpms * ms) {
            timers[timer] = 0;
            return true;
        } else return false;
    } else {
        timers[timer] = rdtsc();
        return false;
    }
}

/* Video Output */

/* Seven possible display colors. Bright variations can be used by bitwise OR
 * with BRIGHT (i.e. BRIGHT | BLUE). */
enum color {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    YELLOW,
    GRAY,
    BRIGHT
};

#define COLS (80)
#define ROWS (25)
u16 *const video = (u16*) 0xB8000;

/* Display a character at x, y in fg foreground color and bg background color.
 */
void putc(u8 x, u8 y, enum color fg, enum color bg, char c)
{
    u16 z = (bg << 12) | (fg << 8) | c;
    video[y * COLS + x] = z;
}

/* Display a string starting at x, y in fg foreground color and bg background
 * color. Characters in the string are not interpreted (e.g \n, \b, \t, etc.).
 * */
void puts(u8 x, u8 y, enum color fg, enum color bg, const char *s)
{
    for (; *s; s++, x++)
        putc(x, y, fg, bg, *s);
}

/* Clear the screen to bg backround color. */
void clear(enum color bg)
{
    u8 x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(x, y, bg, bg, ' ');
}

/* Keyboard Input */

#define KEY_D     (0x20)
#define KEY_H     (0x23)
#define KEY_P     (0x19)
#define KEY_R     (0x13)
#define KEY_S     (0x1F)
#define KEY_UP    (0x48)
#define KEY_DOWN  (0x50)
#define KEY_LEFT  (0x4B)
#define KEY_RIGHT (0x4D)
#define KEY_ENTER (0x1C)
#define KEY_SPACE (0x39)

/* Return the scancode of the current up or down key if it has changed since
 * the last call, otherwise returns 0. When called on every iteration of the
 * main loop, returns non-zero on a key event. */
u8 scan(void)
{
    static u8 key = 0;
    u8 scan = inb(0x60);
    if (scan != key)
        return key = scan;
    else return 0;
}

/* PC Speaker */

/* Set the frequency of the PC speaker through timer 2 of the programmable
 * interrupt timer (PIT). */
void pcspk_freq(u32 hz)
{
    u32 div = 1193180 / hz;
    outb(0x43, 0xB6);
    outb(0x42, (u8) div);
    outb(0x42, (u8) (div >> 8));
}

/* Enable timer 2 of the PIT to drive the PC speaker. */
void pcspk_on(void)
{
    outb(0x61, inb(0x61) | 0x3);
}

/* Disable timer 2 of the PIT to drive the PC speaker. */
void pcspk_off(void)
{
    outb(0x61, inb(0x61) & 0xFC);
}

/* Formatting */

/* Format n in radix r (2-16) as a w length string. */
char *itoa(u32 n, u8 r, u8 w)
{
    static const char d[16] = "0123456789ABCDEF";
    static char s[34];
    s[33] = 0;
    u8 i = 33;
    do {
        i--;
        s[i] = d[n % r];
        n /= r;
    } while (i > 33 - w);
    return (char *) (s + i);
}

/* Random */

/* Generate a random number from 0 inclusive to range exclusive from the number
 * of CPU ticks since boot. */
u32 rand(u32 range)
{
    return (u32) rdtsc() % range;
}

/* Shuffle an array of bytes arr of length len in-place using Fisher-Yates. */
void shuffle(u8 arr[], u32 len)
{
    u32 i, j;
    u8 t;
    for (i = len - 1; i > 0; i--) {
        j = rand(i + 1);
        t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }
}

/* Tetris */

/* The seven tetriminos in each rotation. Each tetrimino is represented as an
 * array of 4 rotations, each represented by a 4x4 array of color values. */
u8 TETRIS[7][4][4][4] = {
    { /* I */
        {{0,0,0,0},
         {4,4,4,4},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,4,0,0},
         {0,4,0,0},
         {0,4,0,0},
         {0,4,0,0}},
        {{0,0,0,0},
         {4,4,4,4},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,4,0,0},
         {0,4,0,0},
         {0,4,0,0},
         {0,4,0,0}}
    },
    { /* J */
        {{7,0,0,0},
         {7,7,7,0},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,7,7,0},
         {0,7,0,0},
         {0,7,0,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {7,7,7,0},
         {0,0,7,0},1
         {0,0,0,0}},
        {{0,7,0,0},
         {0,7,0,0},
         {7,7,0,0},
         {0,0,0,0}}
    },
    { /* L */
        {{0,0,5,0},
         {5,5,5,0},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,5,0,0},
         {0,5,0,0},
         {0,5,5,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {5,5,5,0},
         {5,0,0,0},
         {0,0,0,0}},
        {{5,5,0,0},
         {0,5,0,0},
         {0,5,0,0},
         {0,0,0,0}}
    },
    { /* O */
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,1,1,0},
         {0,1,1,0},
         {0,0,0,0}}
    },
    { /* S */
        {{0,0,0,0},
         {0,2,2,0},
         {2,2,0,0},
         {0,0,0,0}},
        {{0,2,0,0},
         {0,2,2,0},
         {0,0,2,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {0,2,2,0},
         {2,2,0,0},
         {0,0,0,0}},
        {{0,2,0,0},
         {0,2,2,0},
         {0,0,2,0},
         {0,0,0,0}}
    },
    { /* T */
        {{0,6,0,0},
         {6,6,6,0},
         {0,0,0,0},
         {0,0,0,0}},
        {{0,6,0,0},
         {0,6,6,0},
         {0,6,0,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {6,6,6,0},
         {0,6,0,0},
         {0,0,0,0}},
        {{0,6,0,0},
         {6,6,0,0},
         {0,6,0,0},
         {0,0,0,0}}
    },
    { /* Z */
        {{0,0,0,0},
         {3,3,0,0},
         {0,3,3,0},
         {0,0,0,0}},
        {{0,0,3,0},
         {0,3,3,0},
         {0,3,0,0},
         {0,0,0,0}},
        {{0,0,0,0},
         {3,3,0,0},
         {0,3,3,0},
         {0,0,0,0}},
        {{0,0,3,0},
         {0,3,3,0},
         {0,3,0,0},
         {0,0,0,0}}
    }
};

/* Two-dimensional array of color values */
u8 well[WELL_HEIGHT][WELL_WIDTH];

struct {
    u8 i, r; /* Index and rotation into the TETRIS array0 */
    u8 p;    /* Index into bag of preview tetrimino */
    s8 x, y; /* Coordinates */
    s8 g;    /* Y-coordinate of ghost */
} current;

/* Shuffled bag of next tetrimino indices */
#define BAG_SIZE (7)
u8 bag[BAG_SIZE] = {0, 1, 2, 3, 4, 5, 6};

u32 score = 0, level = 1, speed = INITIAL_SPEED;

bool paused = false, game_over = false;

/* Return true if the tetrimino i in rotation r will collide when placed at x,
 * y. */
bool collide(u8 i, u8 r, s8 x, s8 y)
{
    u8 xx, yy;
    for (yy = 0; yy < 4; yy++)
        for (xx = 0; xx < 4; xx++)
            if (TETRIS[i][r][yy][xx])
                if (x + xx < 0 || x + xx >= WELL_WIDTH ||
                    y + yy < 0 || y + yy >= WELL_HEIGHT ||
                    well[y + yy][x + xx])
                        return true;
    return false;
}

u32 stats[7];

/* Set the current tetrimino to the preview tetrimino in the default rotation
 * and place it in the top center. Increase the stats count for the spawned
 * tetrimino. Set the preview tetrimino to the next one in the shuffled bag. If
 * the spawned tetrimino was the last in the bag, re-shuffle the bag and set
 * the preview to the first in the bag. */
void spawn(void)
{
    current.i = bag[current.p];
    stats[current.i]++;
    current.r = 0;
    current.x = WELL_WIDTH / 2 - 2;
    current.y = 0;
    current.p++;
    if (current.p == BAG_SIZE) {
        current.p = 0;
        shuffle(bag, BAG_SIZE);
    }
}

/* Set the ghost y-coordinate by moving the current tetrimino down until it
 * collides. */
void ghost(void)
{
    s8 y;
    for (y = current.y; y < WELL_HEIGHT; y++)
        if (collide(current.i, current.r, current.x, y))
            break;
    current.g = y - 1;
}

/* Try to move the current tetrimino by dx, dy and return true if successful.
 */
bool move(s8 dx, s8 dy)
{
    if (game_over)
        return false;

    if (collide(current.i, current.r, current.x + dx, current.y + dy))
        return false;
    current.x += dx;
    current.y += dy;
    return true;
}

/* Try to rotate the current tetrimino clockwise and return true if successful.
 */
bool rotate(void)
{
    if (game_over)
        return false;

    u8 r = (current.r + 1) % 4;
    if (collide(current.i, r, current.x, current.y))
        return false;
    current.r = r;
    return true;
}

/* Try to move the current tetrimino down one and increase the score if
 * successful. */
void soft_drop(void)
{
    if (move(0, 1))
        score += SOFT_DROP_SCORE;
}

/* Lock the current tetrimino into the well. This is done by copying the color
 * values from the 4x4 array of the tetrimino into the well array. */
void lock(void)
{
    u8 x, y;
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            if (TETRIS[current.i][current.r][y][x])
                well[current.y + y][current.x + x] =
                    TETRIS[current.i][current.r][y][x];
}

/* The y-coordinates of the rows cleared in the last update, top down */
s8 cleared_rows[4];

/* Update the game state. Called at an interval relative to the current level.
 */
void update(void)
{
    /* Gravity: move the current tetrimino down by one. If it cannot be moved
     * and it is still in the top row, set game over state. If it cannot be
     * moved down but is not in the top row, lock it in place and spawn a new
     * tetrimino. */
    if (!move(0, 1)) {
        if (current.y == 0) {
            game_over = true;
            return;
        }
        lock();
        spawn();
    }

    /* Row clearing: check if any rows are full across and add them to the
     * cleared_rows array. */
    static u8 level_rows = 0; /* Rows cleared in the current level */

    u8 x, y, a, i = 0, rows = 0;
    for (y = 0; y < WELL_HEIGHT; y++) {
        for (a = 0, x = 0; x < WELL_WIDTH; x++)
            if (well[y][x])
                a++;
        if (a != WELL_WIDTH)
            continue;

        rows++;
        cleared_rows[i++] = y;
    }

    /* Scoring */
    switch (rows) {
    case 1: score += SCORE_FACTOR_1 * level; break;
    case 2: score += SCORE_FACTOR_2 * level; break;
    case 3: score += SCORE_FACTOR_3 * level; break;
    case 4: score += SCORE_FACTOR_4 * level; break;
    }

    /* Leveling: increase the level for every 10 rows cleared, increase game
     * speed. */
    level_rows += rows;
    if (level_rows >= ROWS_PER_LEVEL) {
        level++;
        level_rows -= ROWS_PER_LEVEL;

        double speed_s = pow(0.8 - (level - 1) * 0.007, level - 1);
        speed = speed_s * 1000;
    }
}

/* Clear the rows in the rows_cleared array and move all rows above them down.
 */
void clear_rows(void)
{
    s8 i, y, x;
    for (i = 0; i < 4; i++) {
        if (!cleared_rows[i])
            break;
        for (y = cleared_rows[i]; y > 0; y--)
            for (x = 0; x < WELL_WIDTH; x++)
                well[y][x] = well[y - 1][x];
        cleared_rows[i] = 0;
    }
}

/* Move the current tetrimino to the position of its ghost, increase the score
 * and trigger an update (to cause locking and clearing). */
void drop(void)
{
    if (game_over)
        return;

    score += HARD_DROP_SCORE_FACTOR * (current.g - current.y);
    current.y = current.g;
    update();
}

#define TITLE_X (COLS / 2 - 9)
#define TITLE_Y (ROWS / 2 - 1)

/* Draw about information in the centre. Shown on boot and pause. */
void draw_about(void) {
    puts(TITLE_X,      TITLE_Y,     BLACK,            RED,     "   ");
    puts(TITLE_X + 3,  TITLE_Y,     BLACK,            MAGENTA, "   ");
    puts(TITLE_X + 6,  TITLE_Y,     BLACK,            BLUE,    "   ");
    puts(TITLE_X + 9,  TITLE_Y,     BLACK,            GREEN,   "   ");
    puts(TITLE_X + 12, TITLE_Y,     BLACK,            YELLOW,  "   ");
    puts(TITLE_X + 15, TITLE_Y,     BLACK,            CYAN,    "   ");
    puts(TITLE_X,      TITLE_Y + 1, BRIGHT | RED,     RED,     " T ");
    puts(TITLE_X + 3,  TITLE_Y + 1, BRIGHT | MAGENTA, MAGENTA, " E ");
    puts(TITLE_X + 6,  TITLE_Y + 1, BRIGHT | BLUE,    BLUE,    " T ");
    puts(TITLE_X + 9,  TITLE_Y + 1, BRIGHT | GREEN,   GREEN,   " R ");
    puts(TITLE_X + 12, TITLE_Y + 1, BRIGHT | YELLOW,  YELLOW,  " I ");
    puts(TITLE_X + 15, TITLE_Y + 1, BRIGHT | CYAN,    CYAN,    " S ");
    puts(TITLE_X,      TITLE_Y + 2, BLACK,            RED,     "   ");
    puts(TITLE_X + 3,  TITLE_Y + 2, BLACK,            MAGENTA, "   ");
    puts(TITLE_X + 6,  TITLE_Y + 2, BLACK,            BLUE,    "   ");
    puts(TITLE_X + 9,  TITLE_Y + 2, BLACK,            GREEN,   "   ");
    puts(TITLE_X + 12, TITLE_Y + 2, BLACK,            YELLOW,  "   ");
    puts(TITLE_X + 15, TITLE_Y + 2, BLACK,            CYAN,    "   ");

    puts(0, ROWS - 1, BRIGHT | BLACK, BLACK,
         TETRIS_NAME " " TETRIS_VERSION " " TETRIS_URL);
}

#define WELL_X (COLS / 2 - WELL_WIDTH)

#define PREVIEW_X (COLS * 3/4 + 1)
#define PREVIEW_Y (2)

#define STATUS_X (COLS * 3/4)
#define STATUS_Y (ROWS / 2 - 4)

#define SCORE_X STATUS_X
#define SCORE_Y (ROWS / 2 - 1)

#define LEVEL_X SCORE_X
#define LEVEL_Y (SCORE_Y + 4)

/* Draw the well, current tetrimino, its ghost, the preview tetrimino, the
 * status, score and level indicators. Each well/tetrimino cell is drawn one
 * screen-row high and two screen-columns wide. The top two rows of the well
 * are hidden. Rows in the cleared_rows array are drawn as white rather than
 * their actual colors. */
void draw(void)
{
    u8 x, y;

    if (paused) {
        draw_about();
        goto status;
    }

    /* Border */
    for (y = 2; y < WELL_HEIGHT; y++) {
        putc(WELL_X - 1,            y, BLACK, GRAY, ' ');
        putc(COLS / 2 + WELL_WIDTH, y, BLACK, GRAY, ' ');
    }
    for (x = 0; x < WELL_WIDTH * 2 + 2; x++)
        putc(WELL_X + x - 1, WELL_HEIGHT, BLACK, GRAY, ' ');

    /* Well */
    for (y = 0; y < 2; y++)
        for (x = 0; x < WELL_WIDTH; x++)
            puts(WELL_X + x * 2, y, BLACK, BLACK, "  ");
    for (y = 2; y < WELL_HEIGHT; y++)
        for (x = 0; x < WELL_WIDTH; x++)
            if (well[y][x])
                if (cleared_rows[0] == y || cleared_rows[1] == y ||
                    cleared_rows[2] == y || cleared_rows[3] == y)
                    puts(WELL_X + x * 2, y, BLACK, BRIGHT | GRAY, "  ");
                else
                    puts(WELL_X + x * 2, y, BLACK, well[y][x], "  ");
            else
                puts(WELL_X + x * 2, y, BRIGHT, BLACK, "::");

    /* Ghost */
    if (!game_over)
        for (y = 0; y < 4; y++)
            for (x = 0; x < 4; x++)
                if (TETRIS[current.i][current.r][y][x])
                    puts(WELL_X + current.x * 2 + x * 2, current.g + y,
                        TETRIS[current.i][current.r][y][x], BLACK, "::");

    /* Current */
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            if (TETRIS[current.i][current.r][y][x])
                puts(WELL_X + current.x * 2 + x * 2, current.y + y, BLACK,
                     TETRIS[current.i][current.r][y][x], "  ");

    /* Preview */
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            if (TETRIS[bag[current.p]][0][y][x])
                puts(PREVIEW_X + x * 2, PREVIEW_Y + y, BLACK,
                     TETRIS[bag[current.p]][0][y][x], "  ");
            else
                puts(PREVIEW_X + x * 2, PREVIEW_Y + y, BLACK, BLACK, "  ");

status:
    if (paused)
        puts(STATUS_X + 2, STATUS_Y, BRIGHT | YELLOW, BLACK, "PAUSED");
    if (game_over)
        puts(STATUS_X, STATUS_Y, BRIGHT | RED, BLACK, "GAME OVER");

    /* Score */
    puts(SCORE_X + 2, SCORE_Y, BLUE, BLACK, "SCORE");
    puts(SCORE_X, SCORE_Y + 2, BRIGHT | BLUE, BLACK, itoa(score, 10, 10));

    /* Level */
    puts(LEVEL_X + 2, LEVEL_Y, BLUE, BLACK, "LEVEL");
    puts(LEVEL_X, LEVEL_Y + 2, BRIGHT | BLUE, BLACK, itoa(level, 10, 10));
}

noreturn main()
{
    clear(BLACK);
    draw_about();

    /* Wait a full second to calibrate timing. */
    u32 itpms;
    tps();
    itpms = tpms; while (tpms == itpms) tps();
    itpms = tpms; while (tpms == itpms) tps();

    /* Initialize game state. Shuffle bag of tetriminos until first tetrimino
     * is not S or Z. */
    do { shuffle(bag, BAG_SIZE); } while (bag[0] == 4 || bag[0] == 6);
    spawn();
    ghost();
    clear(BLACK);
    draw();

    bool debug = false, help = true, statistics = false;
    u8 last_key;
loop:
    tps();

    if (debug) {
        u32 i;
        puts(0,  0, BRIGHT | GREEN, BLACK, "RTC sec:");
        puts(10, 0, GREEN,          BLACK, itoa(rtcs(), 16, 2));
        puts(0,  1, BRIGHT | GREEN, BLACK, "ticks/ms:");
        puts(10, 1, GREEN,          BLACK, itoa(tpms, 10, 10));
        puts(0,  2, BRIGHT | GREEN, BLACK, "key:");
        puts(10, 2, GREEN,          BLACK, itoa(last_key, 16, 2));
        puts(0,  3, BRIGHT | GREEN, BLACK, "i,r,p:");
        puts(10, 3, GREEN,          BLACK, itoa(current.i, 10, 1));
        putc(11, 3, GREEN,          BLACK, ',');
        puts(12, 3, GREEN,          BLACK, itoa(current.r, 10, 1));
        putc(13, 3, GREEN,          BLACK, ',');
        puts(14, 3, GREEN,          BLACK, itoa(current.p, 10, 1));
        puts(0,  4, BRIGHT | GREEN, BLACK, "x,y,g:");
        puts(10, 4, GREEN,          BLACK, itoa(current.x, 10, 3));
        putc(13, 4, GREEN,          BLACK, ',');
        puts(14, 4, GREEN,          BLACK, itoa(current.y, 10, 3));
        putc(17, 4, GREEN,          BLACK, ',');
        puts(18, 4, GREEN,          BLACK, itoa(current.g, 10, 3));
        puts(0,  5, BRIGHT | GREEN, BLACK, "bag:");
        for (i = 0; i < 7; i++)
            puts(10 + i * 2, 5, GREEN, BLACK, itoa(bag[i], 10, 1));
        puts(0,  6, BRIGHT | GREEN, BLACK, "speed:");
        puts(10, 6, GREEN,          BLACK, itoa(speed, 10, 10));
        for (i = 0; i < TIMER__LENGTH; i++) {
            puts(0,  7 + i, BRIGHT | GREEN, BLACK, "timer:");
            puts(10, 7 + i, GREEN,          BLACK, itoa(timers[i], 10, 10));
        }
    }

    if (help) {
        puts(1, 12, BRIGHT | BLUE, BLACK, "LEFT");
        puts(7, 12, BLUE,          BLACK, "- Move left");
        puts(1, 13, BRIGHT | BLUE, BLACK, "RIGHT");
        puts(7, 13, BLUE,          BLACK, "- Move right");
        puts(1, 14, BRIGHT | BLUE, BLACK, "UP");
        puts(7, 14, BLUE,          BLACK, "- Rotate clockwise");
        puts(1, 15, BRIGHT | BLUE, BLACK, "DOWN");
        puts(7, 15, BLUE,          BLACK, "- Soft drop");
        puts(1, 16, BRIGHT | BLUE, BLACK, "ENTER");
        puts(7, 16, BLUE,          BLACK, "- Hard drop");
        puts(1, 17, BRIGHT | BLUE, BLACK, "P");
        puts(7, 17, BLUE,          BLACK, "- Pause");
        puts(1, 18, BRIGHT | BLUE, BLACK, "R");
        puts(7, 18, BLUE,          BLACK, "- Hard reset");
        puts(1, 19, BRIGHT | BLUE, BLACK, "S");
        puts(7, 19, BLUE,          BLACK, "- Toggle statistics");
        puts(1, 20, BRIGHT | BLUE, BLACK, "D");
        puts(7, 20, BLUE,          BLACK, "- Toggle debug info");
        puts(1, 21, BRIGHT | BLUE, BLACK, "H");
        puts(7, 21, BLUE,          BLACK, "- Toggle help");
    }

    if (statistics) {
        u8 i, x, y;
        for (i = 0; i < 7; i++) {
            for (y = 0; y < 4; y++)
                for (x = 0; x < 4; x++)
                    if (TETRIS[i][0][y][x])
                        puts(5 + x * 2, 1 + i * 3 + y, BLACK,
                             TETRIS[i][0][y][x], "  ");
            puts(14, 2 + i * 3, BLUE, BLACK, itoa(stats[i], 10, 10));
        }
    }

    bool updated = false;

    u8 key;
    if ((key = scan())) {
        last_key = key;
        switch(key) {
        case KEY_D:
            debug = !debug;
            if (debug)
                help = statistics = false;
            clear(BLACK);
            break;
        case KEY_H:
            help = !help;
            if (help)
                debug = statistics = false;
            clear(BLACK);
            break;
        case KEY_S:
            statistics = !statistics;
            if (statistics)
                debug = help = false;
            clear(BLACK);
            break;
        case KEY_R:
            reset();
        case KEY_LEFT:
            move(-1, 0);
            break;
        case KEY_RIGHT:
            move(1, 0);
            break;
        case KEY_DOWN:
            soft_drop();
            break;
        case KEY_UP:
            rotate();
            break;
        case KEY_ENTER:
            drop();
            break;
        case KEY_P:
            if (game_over)
                break;
            clear(BLACK);
            paused = !paused;
            break;
        }
        updated = true;
    }

    if (!paused && !game_over && interval(TIMER_UPDATE, speed)) {
        update();
        updated = true;
    }

    if (cleared_rows[0] && wait(TIMER_CLEAR, CLEAR_DELAY)) {
        clear_rows();
        updated = true;
    }

    if (updated) {
        ghost();
        draw();
    }

    goto loop;
}
