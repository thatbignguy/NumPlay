/* The player's changes to the world (the generator makes everything else). */
#ifndef NB_EDITS_H
#define NB_EDITS_H
#include <stdbool.h>
#include <stdint.h>

void edits_setup(const char *world, uint32_t *scratch, uint32_t n);   /* the world's record prefix; a merge buffer */
void edits_clear(void);
void edits_set_dim(int dim);   /* the dimension the player's changes go to (DIM_*) */
bool edits_put(int x, int y, int z, int b);      /* false: no room left to keep it */
bool edits_flush(void);                          /* the journal into storage; false if some did not fit */
bool edits_full(void);
bool edits_convert(const char *world, int from);   /* the world's records from an old shape (0: 8-bit blocks, 1: 7-bit y), written back in the current one */
void edits_forget(void);   /* storage changed: records may have moved */
void edits_apply(int cx, int cz, int y0, int h, uint8_t *slab);   /* onto a chunk from gen_slab */
int edits_top(int x, int z, int top);            /* the column's sky top with the edits (1 + y) */
int edits_count(void);
void *edits_scratch(uint32_t n);   /* in the journal */
#if NB_DIMS
void edits_apply9(int cx, int cz, int y0, int h, uint8_t *slab, uint8_t *hb);   /* NumBlocks.nwa: 9-bit blocks */
void edits_dim(int d);   /* the dimension whose records are used */
bool edits_find(int x, int y, int z, int r, int b0, int b1, int *out);   /* the nearest changed b0 or b1 */
bool edits_changed(int x, int y, int z);   /* the block there is not as the world was made */
#endif
#endif
