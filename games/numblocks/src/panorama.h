/* The title screen's panoramas (src/panorama.c, tools/panorama.py): three Minecraft
 * versions of six 160 x 160 faces, RGB565 (the colour the screen holds), in a flat
 * table. The faces run 0..5 as the client shoots its own panorama_0..5.png: 0..3 the
 * horizon clockwise (south, west, north, east), 4 up, 5 down; the renderer samples
 * them by the ray's direction (render.c, render_panorama). */
#ifndef NB_PANORAMA_H
#define NB_PANORAMA_H
#include <stdint.h>

#define PAN_FACE 160
#define PAN_N 3   /* 1.16, 1.20, 1.21: the versions the option offers */

extern const uint16_t pan_px[PAN_N][6][PAN_FACE * PAN_FACE];

#endif
