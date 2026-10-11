/* The Final Earth 2: drawing the city (view.c). */
#ifndef FE_VIEW_H
#define FE_VIEW_H
#include "bg.h"
#include "fe.h"

extern BgCamera camera;
void view_draw(void);
void view_edges(BgEdges *e);
/* bld*.c: one layer (LAYER_*) of a permanent, its top-left at screen (x, y) */
void bld_draw(const GfxBand *b, const Perm *p, int layer, int x, int y);
#endif
