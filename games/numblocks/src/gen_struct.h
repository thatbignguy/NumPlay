/* The pieces the fortresses and strongholds share (from NumPlay 1.17's
 * gen_fortress.h): a box turned by its facing and the helpers that read a piece. */
#ifndef NB_GEN_STRUCT_H
#define NB_GEN_STRUCT_H
#include "gen.h"

enum { F_N, F_E, F_S, F_W };   /* EnumFacing.Plane.HORIZONTAL's order */
static inline int iabs(int v) { return v < 0 ? -v : v; }

typedef struct {
    int16_t x0, y0, z0, x1, y1, z1;
    uint8_t type, face, flag, depth;   /* flag: a corridor's chest; depth: getComponentType */
    i32 seed;                          /* an End's fillSeed */
} FPiece;

static void box_for(int x, int y, int z, const int8_t *b, int face, int16_t *o) {
    int ox = b[0], oy = b[1], oz = b[2], sx = b[3], sy = b[4], sz = b[5];
    switch (face) {
    case F_N: o[0] = x + ox, o[1] = y + oy, o[2] = z - sz + 1 + oz, o[3] = x + sx - 1 + ox, o[4] = y + sy - 1 + oy, o[5] = z + oz; break;
    case F_S: o[0] = x + ox, o[1] = y + oy, o[2] = z + oz, o[3] = x + sx - 1 + ox, o[4] = y + sy - 1 + oy, o[5] = z + sz - 1 + oz; break;
    case F_W: o[0] = x - sz + 1 + oz, o[1] = y + oy, o[2] = z + ox, o[3] = x + oz, o[4] = y + sy - 1 + oy, o[5] = z + sx - 1 + ox; break;
    default: o[0] = x + oz, o[1] = y + oy, o[2] = z + ox, o[3] = x + sz - 1 + oz, o[4] = y + sy - 1 + oy, o[5] = z + sx - 1 + ox; break;
    }
}
static int boxes_meet(const int16_t *a, const FPiece *p) {
    return a[3] >= p->x0 && a[0] <= p->x1 && a[5] >= p->z0 && a[2] <= p->z1 && a[4] >= p->y0 && a[1] <= p->y1;
}
static int fb_x(const FPiece *p, int x, int z) { return p->face == F_W ? p->x1 - z : p->face == F_E ? p->x0 + z : p->x0 + x; }
static int fb_z(const FPiece *p, int x, int z) { return p->face == F_N ? p->z1 - z : p->face == F_S ? p->z0 + z : p->z0 + x; }

#endif
