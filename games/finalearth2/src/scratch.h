/* The Final Earth 2: memory that is only needed while a search or a pass
 * runs. It lives on main()'s stack (a region of its own on the calculator,
 * not counted in the app's RAM), and the passes that use it never overlap. */
#ifndef FE_SCRATCH_H
#define FE_SCRATCH_H
#include "fe.h"

typedef union {
 struct {
  /* the searches (path.c's pathfinder, assign.c's PermanentFinder) */
  float prio[MAX_PERMS];
  uint16_t prev[MAX_PERMS];
  uint8_t in_edge[MAX_PERMS];
  uint8_t seen[MAX_PERMS];
  uint16_t heap[MAX_PERMS + 1], heap_pos[MAX_PERMS];
  uint8_t no_elevators[MAX_PERMS];
  /* passes over the city (city.c's shards, assign.c's assigners) */
  union {
    struct { uint16_t stack[MAX_PERMS]; uint8_t seen[MAX_PERMS]; uint8_t handled_ls[MAX_PERMS]; } flood;
    struct { uint16_t list[MAX_PERMS]; uint8_t movable[MAX_PERMS]; uint8_t impossible[MAX_SHARDS];
             int16_t tp_left[MAX_SHARDS]; uint8_t shard_no_options[MAX_SHARDS]; int16_t short_by[256]; } assign;
    struct { uint16_t list[MAX_PERMS]; } happy;
    struct { uint16_t order[MAX_CITS]; } compact;
    /* the flying pathfinder's waypoints (bld_transport.c): each one's y, column and free segment */
    struct { int16_t y[MAX_PERMS]; uint8_t col[MAX_PERMS]; uint8_t seg[MAX_PERMS]; uint16_t colstart[257]; } fly;
  } u;
 };
  /* saving and loading (save.c), when nothing else runs: a chunk of the save and the packer's table */
  struct { uint8_t chunk[4096]; uint16_t table[4096]; } save;
  /* the band being drawn (gfx.c), only during gfx_end: no search or pass may run while the city is drawn */
  uint16_t band[320 * 16];
} Scratch;

extern Scratch *scratch;
#endif
