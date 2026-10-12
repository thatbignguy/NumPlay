/* NumPowder: keys, as pressed this tick or held, with repeats. */
#include "input.h"

Input input;

void input_update(void) {
  uint32_t now = plat_keys();
  int tk = plat_text_key();
  input.text_key = (int8_t)(tk >= 0 && tk != input.text_held ? tk : -1);
  input.text_held = (int8_t)tk;
  if (input.typing) {
    /* 4, 6, 8, 2 and 5 type digits (or letters), + and - their signs, not the arrows, OK and the zoom */
    static const struct { int8_t tk; uint16_t k; } alias[] = {
      {TK_FOUR, K_LEFT}, {TK_SIX, K_RIGHT}, {TK_EIGHT, K_UP}, {TK_TWO, K_DOWN}, {TK_FIVE, K_OK}, {TK_PLUS, K_PLUS},
      {TK_MINUS, K_MINUS}};
    for (unsigned i = 0; i < sizeof alias / sizeof alias[0]; i++)
      if (tk == alias[i].tk) now &= ~(uint32_t)alias[i].k;
  }
  input.pressed = now & ~input.held;
  input.released = input.held & ~now;
  input.held = now;
  input.repeat = input.pressed;
  for (int i = 0; i < 16; i++) {
    if (now & (1u << i)) {
      input.held_ticks[i]++;
      /* a held key repeats after 12 ticks, then every 3 */
      if (input.held_ticks[i] > 12 && (input.held_ticks[i] - 12) % 3 == 0) input.repeat |= 1u << i;
    } else {
      input.held_ticks[i] = 0;
    }
  }
}
