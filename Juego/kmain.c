
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
  TIMER_UPDATE,
  TIMER_CLEAR,
  TIMER__LENGTH
};

/* Entradas de teclado */

#define KEY_D     (0x20)
#define KEY_H     (0x23)
#define KEY_P     (0x19)
#define KEY_R     (0x13)
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

u64 timers[TIMER__LENGTH] = {0};
u64 tpms;     // Ticks por milisegundo

/*==============================================================================
                              FUNCIONES DE ESCRITURA
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

/* Pinta la pantalla de un color*/
void clear(enum color bg){
    u8 x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(x, y, bg, bg, ' ');
}

/*==============================================================================
                              FUNCIONES DE ENTRADA/SALIDA
==============================================================================*/
/* Recibe un valor de 8 bits de un puerto de I/O*/
static inline u8 inb(u16 p)
{
    u8 r;
    asm("inb %1, %0" : "=a" (r) : "dN" (p));
    return r;
}

/* Envía un valor de 8 bits a un puerto de I/O*/
static inline void outb(u16 p, u8 d)
{
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

/*Hace que la pantalla parpadee de un color por 2 segundos */
void strobe(void){
    /* Agregar funcion de garita para manejar el tiempo */
    for (int i=0;1;i++)
        {
        /* Changes screen color */
        clear(BRIGHT|GRAY);
        }
}


/* Draws intro Screnn */
void draw_intro(char option) {
    puts(39,5,BLUE,BLACK, "Lead");
    option == 'G' ? puts(38,10,BLACK,CYAN,"Start") : puts(38,10,BLUE,BLACK,"Start");
    option == 'L' ? puts(35,11,BLACK,CYAN,"Leaderboard") : puts(35,11,BLUE,BLACK,"Leaderboard");
    puts(39,15,BLUE,BLACK,"2008");
    puts(38,16,BLUE,BLACK,"Simone");
    puts(38,17,BLUE,BLACK,"Serra");
    puts(39,20,BLUE,BLACK,"Ship");
    puts(39,21,BLUE,BLACK,"0000");
}


/* Draws "will you lead" secuence in the screen */
void intro_secuence(void){
    /*Funcion de timer*/
    puts(35,10,BLACK,BLUE,"Will");
    /*Funcion de timer*/
    puts(35,10,BLACK,BLUE,"You");
    /*Funcion de timer*/
    puts(35,10,BLACK,BLUE,"Lead?");

}

void kmain()
{
  clear(BLACK);
  draw_about();

  // Espera un segundo para calibrar el tiempo.
  u32 itpms;
  tps();
  itpms = tpms; while(tpms == itpms) tps();
  itpms = tpms; while(tpms == itpms) tps();

  u8 key;
  u8 last_key;

  char option = 'G';
  bool updated = false;

  clear(BLACK);
start:
  tps();
  draw_intro(option);

  if((key = scan())) {
    last_key = key;
    switch(key) {
      case KEY_UP:
        option = (option == 'L') ? 'G' : 'L';
        break;
      case KEY_DOWN:
        option = (option == 'L') ? 'G' : 'L';
        break;
      case KEY_ENTER:
        clear(BLACK);
        if (option == 'G') goto game;
        else goto leaderboard;
        break;
    }
  }
  goto start;

game:
  puts(35,10,BLACK,BLUE,"GAME");

  goto game;

leaderboard:
  puts(35,10,BLACK,BLUE,"LEADERBOARD");

  goto leaderboard;

loop:
  // INICIO
  tps();    //Mantiene los timers calibrados.

  // SI PRESIONO TECLA
  if((key = scan())) {
    last_key = key;
    switch(key) {
      case KEY_UP:
        putc(0,0,YELLOW, BLACK, 'v');
        break;
      case KEY_DOWN:
        putc(0,0,YELLOW, BLACK, '^');
        break;
      case KEY_SPACE:     // Disparar
        putc(0,0,YELLOW, BLACK, '_');
        break;
      case KEY_ENTER:     // Siguiente
        putc(0,0,YELLOW, BLACK, '*');
        break;
      case KEY_LEFT:     // Izquierda
        putc(0,0,YELLOW, BLACK, '<');
        break;
      case KEY_RIGHT:     // Derecha
        putc(0,0,YELLOW, BLACK, '>');
        break;
      case KEY_P:         // Pausa
        putc(0,0,YELLOW, BLACK, 'P');
        break;
    }
    updated = true;
  }
  // SI EL JUEGO ESTÁ PAUSADO, NO HA PERDIDO Y SE CUMPLE EL INTERVALO

  goto loop;
}
