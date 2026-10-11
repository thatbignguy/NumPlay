/* The Final Earth 2: keys, as pressed this tick or held, with repeats. */
#ifndef FE_INPUT_H
#define FE_INPUT_H
#include "plat.h"

typedef struct {
  uint32_t held;      /* K_* held now */
  uint32_t pressed;   /* went down this tick */
  uint32_t repeat;    /* pressed, or held long enough to repeat (arrows, + and -) */
  uint32_t released;
  uint16_t held_ticks[16];
  int8_t text_key;    /* a typing key that went down this tick (TK_*), -1 none */
  int8_t text_held;   /* the typing key held, -1 none */
  bool typing;        /* a text is being typed: the typing keys don't also act as the arrows, OK, + and - */
} Input;
extern Input input;
void input_update(void);
static inline bool key_pressed(uint32_t k) { return (input.pressed & k) != 0; }
static inline bool key_repeat(uint32_t k) { return (input.repeat & k) != 0; }
static inline bool key_held(uint32_t k) { return (input.held & k) != 0; }
#endif
