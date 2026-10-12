/* Block states are 9 bits each (blocks.h's NSTATES), so the arrays that hold them
 * are packed: 8 cells are exactly 9 bytes, and a cell i sits at byte
 * (i >> 3) * 9 + (i & 7), shifted by its 9 bits. The world's block cache and its
 * chunk slab (world.c), the generator's columns (gen.c) and the edit log's slab
 * (edits.c) are all packed this way; 8-cell runs of them stay byte-aligned, which
 * is what the cache's slides move. A 32-bit load reads 4 bytes, so an array's last
 * cells run into a few bytes of tail padding it carries. */
#ifndef NB_STATES_H
#define NB_STATES_H
#include <stdint.h>
#include <string.h>

/* the state at cell i of the packed array p */
static inline int b9_get(const uint8_t *p, int i) {
  uint32_t v;
  memcpy(&v, p + (i >> 3) * 9 + (i & 7), 4);
  return (int)((v >> (i & 7)) & 511);
}
/* the state at cell i of the packed array p, put there */
static inline void b9_set(uint8_t *p, int i, int b) {
  uint8_t *q = p + (i >> 3) * 9 + (i & 7);
  int s = i & 7;
  uint32_t v, m = 511u << s;
  memcpy(&v, q, 4);
  v = (v & ~m) | ((uint32_t)b << s);
  memcpy(q, &v, 4);
}
#endif
