/* The Final Earth 2: the platform (the calculator, or a computer for tests).
 *
 * plat_eadk.c: the calculator's EADK display, keyboard, time and files.
 * plat_host.c: a frame buffer that tests save as images, scripted keys and
 * simulated time (tests/play.c). */
#ifndef FE_PLAT_H
#define FE_PLAT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240

/* keys held now, as bits */
enum {
  K_LEFT = 1 << 0, K_RIGHT = 1 << 1, K_UP = 1 << 2, K_DOWN = 1 << 3,
  K_OK = 1 << 4,        /* OK, EXE */
  K_BACK = 1 << 5,      /* Back */
  K_HOME = 1 << 6,      /* Home, On/Off */
  K_TOOLBOX = 1 << 7,   /* Toolbox: the build menu */
  K_VAR = 1 << 8,       /* var: the city's menus (population, happiness, research...) */
  K_SHIFT = 1 << 9,     /* shift: faster cursor */
  K_ALPHA = 1 << 10,    /* alpha: the speed (pause, 1x, 2x, 3x) */
  K_PLUS = 1 << 11, K_MINUS = 1 << 12,   /* zoom */
  K_BACKSPACE = 1 << 13,
  K_ANY = 1 << 14
};
uint32_t plat_keys(void);
/* the keys that type (a city's name): the one held now, as its number on the NumWorks keyboard (TK_EXP to TK_ANS,
 * row by row; Epsilon's eadk_key_t), -1 none. Some of them are also K_* keys above (4, 6, 8, 2, 5, + and -) */
enum { TK_EXP = 18, TK_SQUARE = 29, TK_SEVEN = 30, TK_RPAREN = 34, TK_FOUR = 36, TK_FIVE = 37, TK_SIX = 38,
       TK_DIVISION = 40, TK_ONE = 42, TK_TWO = 43, TK_PLUS = 45, TK_MINUS = 46, TK_ZERO = 48, TK_ANS = 51, TK_EIGHT = 31 };
int plat_text_key(void);
uint32_t plat_millis(void);
void plat_sleep(uint32_t ms);
void plat_push(int x, int y, int w, int h, const uint16_t *px);
void plat_fill(int x, int y, int w, int h, uint16_t c);
bool plat_save(const char *name, const void *data, uint32_t len);
const uint8_t *plat_load(const char *name, uint32_t *len);   /* NULL if absent */
/* a file of len bytes to write in place (the calculator's storage is RAM): plat_reserve gives its
 * bytes, plat_reserved says they are written; NULL if there is no room */
uint8_t *plat_reserve(const char *name, uint32_t len);
void plat_reserved(const char *name, uint32_t len);
bool plat_remove(const char *name);
void plat_begin(void);
int plat_end(void);
/* an N0110 or N0115 (216 MHz, the N0120 has 550) */
bool plat_slow(void);
#endif
