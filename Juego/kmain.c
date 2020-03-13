
typedef unsigned char      u8;
typedef signed   char      s8;
typedef unsigned short     u16;
typedef signed   short     s16;
typedef unsigned int       u32;
typedef signed   int       s32;
typedef unsigned long long u64;
typedef signed   long long s64;

typedef enum bool {
    false,
    true
} bool;

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
u16* const vga = (u16*) 0xb8000;

/*Escribe un carácter*/
void putc(u8 x, u8 y, enum color fg, enum color bg, char c)
{
    u16 z = (bg << 12) | (fg << 8) | c;
    vga[y * COLS + x] = z;
}

/*Escribe un string*/
void puts(u8 x, u8 y, enum color fg, enum color bg, const char *s)
{
    for (; *s; s++, x++)
        putc(x, y, fg, bg, *s);
}

/*Pinta la pantalla de un color*/
void clear(enum color bg)
{
    u8 x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(x, y, bg, bg, ' ');
}

void kmain()
{
  clear(RED);
}
