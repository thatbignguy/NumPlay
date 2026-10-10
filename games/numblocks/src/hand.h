/* The hand and what it holds, drawn over the world (hand.c).
 *
 * hand_frame sets the frame's model up once, hand_strip draws what of it is on
 * each strip. The state they share lives on render_frame's stack, as the strip
 * buffer already does (nb.h: the calculator gives apps 32 KB of stack), since
 * none of it outlives a frame. */
#ifndef NB_HAND_H
#define NB_HAND_H
#include <stdbool.h>
#include <stdint.h>

/* the faces turned to the eye (at most 3 a box), with their corners on the screen */
typedef struct { int8_t b, f; bool near; float sx[4], sy[4], ymin, ymax; } HandFace;

typedef struct {
  int kind, model_id, icon;   /* what is drawn: the block, or the item's icon */
  int nbox;
  float box[5][6];              /* the model's boxes, in its pixels (x0 y0 z0 x1 y1 z1) */
  float org[3], dx0[3], ddx[3], ddy[3];   /* the eye and the ray of pixel (x, y), in the model */
  int face_sh[6];               /* each face's shade, 0..32: -x +x -y +y -z +z */
  int nfaces;
  HandFace faces[15];
} HandFrame;

void hand_frame(HandFrame *h, int light);   /* light: how bright the hand is drawn (0..32) */
void hand_none(HandFrame *h);   /* nothing held: what a frame that builds no model draws */
void hand_strip(uint16_t *buf, int y0, int rows, const HandFrame *h);

#endif
