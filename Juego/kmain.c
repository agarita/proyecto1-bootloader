#include "config.h"

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

/* IDs para mantener separadas distintas operaciones de Tiempo*/
enum timer {
  TIMER_ENEMY,
  TIMER_BULLET,
  TIMER_ANIMACIONES,
  TIMER__LENGTH
};

/* Entradas de teclado */
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

#define TITLE_X (COLS / 2 - 9)
#define TITLE_Y (ROWS / 2 - 1)

#define COLS (80)
#define ROWS (25)

u16* const vga = (u16*) 0xb8000;

u32 score = 0, speed_e = ENEMY_1_SPEED, speed_b = 0;
char option = 'G';

//DEBUG variables
u32 disparos = 0, caidas = 0;

bool paused = false, game_over = false;

u64 timers[TIMER__LENGTH] = {0};
u64 tpms;     // Ticks por milisegundo

/*==============================================================================
                      FUNCIONES DE ESCRITURA Y LECTURA
==============================================================================*/
/* Escribe un carácter*/
void putc(u8 x, u8 y, enum color fg, enum color bg, char c){
    u16 z = (bg << 12) | (fg << 8) | c;
    vga[y * COLS + x] = z;
}

/* Escribe un string*/
void puts(u8 x, u8 y, enum color fg, enum color bg, const char *s){
    for (; *s; s++, x++)
        putc(x, y, fg, bg, *s);
}

/* Retorna el carácter en la posición xXy*/
char getc(u8 x, u8 y){
  return vga[y * COLS + x];
}

/* Pinta la pantalla de un color*/
void clear(enum color bg){
    u8 x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(x, y, bg, bg, ' ');
}

char* itoa(u32 n, u8 r, u8 w){
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

/*==============================================================================
                              FUNCIONES DE ENTRADA/SALIDA
==============================================================================*/
/* Recibe un valor de 8 bits de un puerto de I/O*/
static inline u8 inb(u16 p){
    u8 r;
    asm("inb %1, %0" : "=a" (r) : "dN" (p));
    return r;
}

/* Envía un valor de 8 bits a un puerto de I/O*/
static inline void outb(u16 p, u8 d){
    asm("outb %1, %0" : : "dN" (p), "a" (d));
}

/*==============================================================================
                              FUNCIONES DE TIEMPO
==============================================================================*/
/* ReaD Time-Stamp Counter, retorna el número ticks del CPU desde que se inicio*/
static inline u64 rdtsc(void){
  u32 a, b;
  asm("rdtsc" : "=a" (a), "=d" (b));
  return ((u64) a) | (((u64) b) << 32);
}

/* Real-Time-Clock-Second, retorna el segundo actual en el RTC.*/
u8 rtcs(void){
  u8 last = 0, sec;

  do{
    do {
      outb(0x70, 0x0A);
    } while (inb(0x71) & 0x80);
    outb(0x70, 0x00);
    sec = inb(0x71);
  } while (sec != last && (last = sec));
  return sec;
}

/* Setea la variable tpms al número de ticks del CPU por milisegundo, basado en
 * el número de ticks del último segundo. Esto se llama en cada iteración del
 * loop principal, para estar bien sincronizado.*/
void tps(void){
  static u64 ti = 0;
  static u8 last_sec = 0xFF;
  u8 sec = rtcs();
  if(sec != last_sec) {
    last_sec = sec;
    u64 tf = rdtsc();   //ticks desde el boot
    tpms = (u32) ((tf - ti) >> 3) / 125; //Para evitar truncamientos.
    ti = tf;
  }
}

/* Función que revisa si se han pasado ms milisegundos desde la última vez que
 * este timer devolvió true. En el ciclo principal devuelve true una vez ms
 * milisegundos.*/
bool interval(enum timer timer, u32 ms){
  u64 tf = rdtsc();   //ticks desde el boot
  if (tf - timers[timer] >= tpms * ms) {
    timers[timer] = tf;
    return true;
  }
  else return false;
}

/* Función que revisa si han pasado ms milisegundos desde la primera llamada de
 * este timer, y lo resetea.*/
bool wait(enum timer timer, u32 ms){
  if(timers[timer]) {
    if(rdtsc() - timers[timer] >= tpms * ms) {
      timers[timer] = 0;
      return true;
    }
    else return false;
  }
  else {
    timers[timer] = rdtsc();
    return false;
  }
}

/*Un generador de randoms, basado en el número de ticks desde el inicio*/
u32 rand(u32 range){
  return (u32) rdtsc() % range;
}

/*==============================================================================
                              ESCANEO DE TECLAS
==============================================================================*/
u8 scan(void){
  static u8 key = 0;
  u8 scan = inb(0x60);
  if(scan != key)
    return key = scan;
  else
    return 0;
}

/*==============================================================================
                              FUNCIONES DE JUEGO
==============================================================================*/
void move_char(int row,int column,int row_direction,int column_direction) {
  char c;
  u16 actual_char, replacemente_char;
  actual_char = vga[row*COLS + column];
  replacemente_char = vga[(row+row_direction)*COLS + (column+column_direction)];
  enum color fg,bg;

  bg = (actual_char & 0xF000) >> 12;
  fg = (actual_char & 0x0F00) >> 8;
  c = getc(column,row);

  putc(column,row,fg,bg,c);

  bg = (replacemente_char & 0xF000) >> 12;
  fg = (replacemente_char & 0x0F00) >> 8;
  c = getc(column+column_direction,row+row_direction);

  putc(column+column_direction,row+row_direction,fg,bg,c);

}

int valid_vga_position(int row, int column) { //80 columnas y 25 filas (voy a usar 24 para dejar la fila de abajo para el score)
  return (row >= 0 && row < 24 && column >= 0 && column < 80);
}

int verify_colition(int row, int column, int row_direction, int column_direction) {
  if(!valid_vga_position(row_direction,column_direction)) {
    return 1;
  }
  char c = getc(row_direction,column_direction);
  if (c != ' ') {
    return 1;
  }
  return 0;
}

void create_walls(int row, int column,int len,enum color fg,enum color bg,char c) {
  int i,rows_max;
  rows_max = 23;
  for (i=row;i<rows_max;i++) {
    putc(column,i,fg,bg,c);
    putc(column+len,i,fg,bg,c);
    move_char(i,column,1,0);
    move_char(i,column+len,1,0);
  }
}

/*==============================================================================
                              FUNCIONES DE PINTADO
==============================================================================*/
/* Draw about information in the centre. Shown on boot and pause. */
void draw_about(void) {
    puts(TITLE_X + 2,  TITLE_Y,     BLACK,   BRIGHT | YELLOW,  " ");
    puts(TITLE_X + 3,  TITLE_Y,     BLACK,   BRIGHT | YELLOW,  "   ");
    puts(TITLE_X + 6,  TITLE_Y,     BLACK,   BRIGHT | YELLOW,  "   ");
    puts(TITLE_X + 9,  TITLE_Y,     BLACK,   BRIGHT | YELLOW,  "   ");
    puts(TITLE_X + 12, TITLE_Y,     BLACK,   BRIGHT | YELLOW,  "   ");
    puts(TITLE_X + 15, TITLE_Y,     BLACK,   BRIGHT | YELLOW,  " ");
    puts(TITLE_X + 2,  TITLE_Y + 1, BRIGHT | YELLOW, BRIGHT | YELLOW, " ");
    puts(TITLE_X + 3,  TITLE_Y + 1, BRIGHT | YELLOW, BLACK,   " L ");
    puts(TITLE_X + 6,  TITLE_Y + 1, BRIGHT | YELLOW, BLACK,   " E ");
    puts(TITLE_X + 9,  TITLE_Y + 1, BRIGHT | YELLOW, BLACK,   " A ");
    puts(TITLE_X + 12, TITLE_Y + 1, BRIGHT | YELLOW, BLACK,   " D ");
    puts(TITLE_X + 15, TITLE_Y + 1, BRIGHT | YELLOW, BRIGHT | YELLOW, " ");
    puts(TITLE_X + 2,  TITLE_Y + 2, BLACK,   BRIGHT | YELLOW, "   ");
    puts(TITLE_X + 3,  TITLE_Y + 2, BLACK,   BRIGHT | YELLOW, "   ");
    puts(TITLE_X + 6,  TITLE_Y + 2, BLACK,   BRIGHT | YELLOW, "   ");
    puts(TITLE_X + 9,  TITLE_Y + 2, BLACK,   BRIGHT | YELLOW, "   ");
    puts(TITLE_X + 12, TITLE_Y + 2, BLACK,   BRIGHT | YELLOW, "   ");
    puts(TITLE_X + 15, TITLE_Y + 2, BLACK,   BRIGHT | YELLOW, " ");
}

/* Draws intro Screnn */
void draw_intro(char option) {
    puts(39,5,BLUE,BLACK, "Lead");
    option == 'G' ? puts(38,10,BLACK,CYAN,"Start") : puts(38,10,BLUE,BLACK,"Start");
    option == 'L' ? puts(35,11,BLACK,CYAN,"Leaderboard") : puts(35,11,BLUE,BLACK,"Leaderboard");
    puts(39,15,BLUE,BLACK,"2020");
    puts(33,16,BLUE,BLACK,"Aymaru Castillo");
    puts(33,17,BLUE,BLACK,"Alejandro Garita");
    puts(34,18,BLUE,BLACK,"Alberto Obando");
    puts(34,21,BLUE,BLACK,"Ernesto Rivera");
}

/* Draws intro Screnn */
void draw_world(char option) {
    puts(35,5,YELLOW,BLACK, "Select the level");
    option == '1' ? puts(40,10,BLACK,YELLOW,"Level 1") : puts(40,10,BRIGHT|YELLOW,BLACK,"Level 1");
    option == '2' ? puts(40,11,BLACK,YELLOW,"Level 2") : puts(40,11,BRIGHT|YELLOW,BLACK,"Level 2");
    option == '3' ? puts(40,12,BLACK,YELLOW,"Level 3") : puts(40,12,BRIGHT|YELLOW,BLACK,"Level 3");
    option == '4' ? puts(40,13,BLACK,YELLOW,"Level 4") : puts(40,13,BRIGHT|YELLOW,BLACK,"Level 4");
    option == 'V' ? puts(75,20,BLACK,YELLOW,"Back") : puts(75,20,BRIGHT|YELLOW,BLACK,"Back");
}

/* Draws intro Screnn */
void draw_leaderboard(char option) {
    puts(38,5,BLUE,BLACK, "Leaderboard");
    puts(38,8,BLUE,BLACK,"AOZ   10000000");
    puts(38,9,BLUE,BLACK,"AGC    5858588");
    puts(38,10,BLUE,BLACK,"ACF      40000");
    puts(38,11,BLUE,BLACK,"FSC          3");
    option == 'V' ? puts(41,20,BLACK,YELLOW,"Back") : puts(41,20,BRIGHT|YELLOW,BLACK,"Back");
}

/* Draws intro Screnn */
void draw_win(char option) {
    puts(38,9,BLUE,BLACK, "Congratulations");
    puts(38,10,BLUE,BLACK,"You Won!!!");
    option == 'V' ? puts(41,20,BLACK,YELLOW,"Continue") : puts(41,20,BRIGHT|YELLOW,BLACK,"Continue");
}

/* Draws intro Screnn */
void draw_game_over(char option) {
    puts(38,9,RED,BLACK, "Game Over");
    puts(38,10,RED,BLACK,"You Lost");
    puts(38,11,RED,BLACK,":( :( :( ");
    option == 'V' ? puts(41,20,BLACK,YELLOW,"Continue") : puts(41,20,BRIGHT|YELLOW,BLACK,"Continue");
}

void draw(void){

  if (paused) {
    goto status;
  }

  // DIBUJAR PAREDES

  // DIBUJAR ENEMIGOS

  // DIBUJAR JUGADOR

status:
  if(paused)
    puts(70, 0, BRIGHT | YELLOW, BLACK, "PAUSED");
  puts(38,23, BRIGHT | YELLOW, BLACK, "0000");
  puts(38,23, BRIGHT | YELLOW, BLACK, itoa(score, 10, 4));
}

void kmain(){
  clear(BLACK);

  draw_about();

  // Espera un segundo para calibrar el tiempo.
  u32 itpms;
  tps();
  itpms = tpms; while(tpms == itpms) tps();
  itpms = tpms; while(tpms == itpms) tps();

  u8 key;
  bool updated = false;

  clear(BLACK);
start:
  tps();
  draw_intro(option);

  if((key = scan())) {
    switch(key) {
      case KEY_UP:
        option = (option == 'L') ? 'G' : 'L';
        break;
      case KEY_DOWN:
        option = (option == 'L') ? 'G' : 'L';
        break;
      case KEY_ENTER:
        clear(BLACK);
        if (option == 'G'){
          option = '1';
          goto game;
        }
        else{
          option = 'V';
          goto leaderboard;
        }
        break;
    }
  }
  goto start;

game:
  tps();
  draw_world(option);

  if((key = scan())) {
    switch(key) {
      case KEY_UP:
        if(option == '1')      option = 'V';
        else if(option == '2') option = '1';
        else if(option == '3') option = '2';
        else if(option == '4') option = '3';
        else option = '4';
        break;
      case KEY_DOWN:
        if(option == '1')      option = '2';
        else if(option == '2') option = '3';
        else if(option == '3') option = '4';
        else if(option == '4') option = 'V';
        else option = '1';
        break;
      case KEY_ENTER:
        clear(BLACK);
        if (option == '1' || option == '2' ||
            option == '3' || option == '4') goto loop;
        else goto start;
        break;
    }
  }

  goto game;

leaderboard:
  tps();
  draw_leaderboard(option);

  if((key = scan())) {
    switch(key) {
      case KEY_ENTER:
        clear(BLACK);
        goto start;
        break;
    }
  }

  goto leaderboard;

loop:
  // INICIO
  tps();    //Mantiene los timers calibrados.

  // ACTUALIZAR SCORE

  // SEÑAL DE NIVEL
  if(option == '1'){
    speed_b = BULLET_SPEED;
    speed_e = ENEMY_1_SPEED;
    puts(1,0,RED, BRIGHT | RED, "-1-");
  }
  else if(option == '2'){
    speed_b = 0;
    speed_e = ENEMY_2_SPEED;
    puts(1,0,YELLOW, BRIGHT | YELLOW, "-2-");
  }
  else if(option == '3'){
    speed_b = BULLET_SPEED;
    speed_e = ENEMY_3_SPEED;
    puts(1,0,BLUE, BRIGHT | BLUE, "-3-");}
  else if(option == '4'){
    speed_b = 0;
    speed_e = ENEMY_4_SPEED;
    puts(1,0,MAGENTA, BRIGHT | MAGENTA, "-4-");}

  // SI PRESIONO TECLA
  if((key = scan())) {
    switch(key) {
      case KEY_UP:        // Arriba
        break;
      case KEY_DOWN:      // Abajo
        break;
      case KEY_SPACE:     // Disparar
        break;
      case KEY_ENTER:     // Siguiente
        break;
      case KEY_LEFT:     // Izquierda
        break;
      case KEY_RIGHT:     // Derecha
        break;
      case KEY_P:         // Pausa
        break;
      case KEY_S:         // Siguiente nivel
        caidas = disparos = 0;
        puts(1,21, GREEN, BRIGHT | GREEN, itoa(disparos, 10, 4));
        puts(1,20, BLUE, BRIGHT | BLUE, itoa(caidas, 10, 4));
        if(option == '1') option = '2';
        else if(option == '2') option = '3';
        else if(option == '3') option = '4';
        else{clear(BLACK); option = 'G'; goto start;}
        break;
    }
    updated = true;
  }

  // ACTUALIZAR ENEMIGO
  if(!paused && !game_over && interval(TIMER_ENEMY, speed_e)){
    caidas++;
    puts(1,20, BLUE, BRIGHT | BLUE, itoa(caidas, 10, 4));
  }

  // ACTUALIZAR BALAS
  if((option == '1' || option == '3') && interval(TIMER_BULLET, speed_b)){
    disparos++;
    puts(1,21, GREEN, BRIGHT | GREEN, itoa(disparos, 10, 4));
  }

  // draw update parametro opcion
  if (updated){
    draw();
  }
  goto loop;
}
