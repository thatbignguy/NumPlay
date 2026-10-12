/* The nether portal: the obsidian frame, lighting it, standing in it, and the
 * one on the far side.
 *
 * The frame is vanilla's: either axis, at least 2 x 3 of air inside an obsidian
 * rectangle 4 x 5 (the corners free), lit with flint and steel. Standing in it
 * for a few seconds travels: to the nether at the portal's position divided by
 * eight, back through the portal the world remembers (or at the position times
 * eight), and the far side's portal is made where there is none. */
#include <math.h>
#include "nb.h"
#pragma GCC optimize("Os")   /* (not where the time goes: small) */

/* Entity.setInPortal's wait before travelling: a few seconds */
#define PORTAL_WAIT 80
/* and the cooldown after one, so arriving in a portal is not leaving at once */
#define PORTAL_COOL 60

static int portal_time;   /* ticks stood in the portal */
static int portal_cool;   /* ticks since travelling */
static int portal_axis;   /* the axis of the frame the player last stood in (0 x, 1 z) */

/* air a portal's frame may be filled with: air, or either portal state */
static bool portal_open(int b) { return b == B_AIR || is_portal(b); }

/* the block of a column at (i, y, z) on axis a (a: which of x, z runs) */
static inline int ax_get(int a, int i, int y, int r) { return world_get(a ? r : i, y, a ? i : r); }

/* The frame the player stands in (their feet in its air): vanilla's scan, both
 * axes. The width grows at this row, the height in whole rows of that width,
 * and the obsidian around it (the corners free). False: no frame here.
 *
 * Every scan is bounded (vanilla's largest portal is 21 x 21, and the frame is
 * inside the world): outside the cache world_get reads as air, so an unbounded
 * scan past a missing frame would walk off to infinity and hang the game. */
#define PORTAL_MAX 21
static bool portal_frame(int x, int y, int z, int *ox, int *oy, int *oz, int *axis, int *w, int *h) {
  for (int a = 0; a < 2; a++) {
    int lo = a ? z : x, hi = lo, y0 = y, y1 = y;
    for (int k = 0; k < PORTAL_MAX && portal_open(ax_get(a, lo - 1, y, a ? x : z)); k++) lo--;
    for (int k = 0; k < PORTAL_MAX && portal_open(ax_get(a, hi + 1, y, a ? x : z)); k++) hi++;
    if (hi - lo + 1 < 2 || hi - lo + 1 > PORTAL_MAX) continue;
    for (int k = 0; k < PORTAL_MAX; k++) {
      bool row = y0 > 0;
      for (int i = lo; i <= hi && row; i++)
        if (!portal_open(ax_get(a, i, y0 - 1, a ? x : z))) row = false;
      if (!row) break;
      y0--;
    }
    for (int k = 0; k < PORTAL_MAX; k++) {
      bool row = y1 + 1 < WORLD_H;
      for (int i = lo; i <= hi && row; i++)
        if (!portal_open(ax_get(a, i, y1 + 1, a ? x : z))) row = false;
      if (!row) break;
      y1++;
    }
    if (y1 - y0 + 1 < 3 || y1 - y0 + 1 > PORTAL_MAX) continue;
    bool ok = true;
    for (int j = y0 - 1; j <= y1 + 1 && ok; j++)
      for (int i = lo - 1; i <= hi + 1; i++) {
        bool corner = (i == lo - 1 || i == hi + 1) && (j == y0 - 1 || j == y1 + 1);
        bool inner = i >= lo && i <= hi && j >= y0 && j <= y1;
        if (corner || inner) continue;
        if (ax_get(a, i, j, a ? x : z) != B_OBSIDIAN) {
          ok = false;
          break;
        }
      }
    if (!ok) continue;
    *axis = a;
    *w = hi - lo + 1;
    *h = y1 - y0 + 1;
    *oy = y0;
    if (a) *oz = lo, *ox = x;
    else *ox = lo, *oz = z;
    return true;
  }
  return false;
}

/* the frame's air, filled with portal blocks of its axis (the corners stay as
 * they are): NS faces north/south (thin in Z) for a frame running along X,
 * EW faces east/west (thin in X) for one running along Z */
static void portal_fill(int ox, int oy, int oz, int axis, int w, int h) {
  int pb = axis ? B_PORTAL_EW : B_PORTAL_NS;
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) {
      int bx = axis ? ox : ox + i, by = oy + j, bz = axis ? oz + i : oz;
      if (world_get(bx, by, bz) == B_AIR) world_set(bx, by, bz, pb);
    }
  portal_axis = axis;
}

/* The connected portal blocks around (x, y, z) go, framed or not (no drops:
 * a broken portal leaves nothing). Bounded: interiors are small rectangles,
 * and every cleared block returns at once, so the depth is the longest path
 * through what is left (giant portals leave what is past it to the next
 * update). */
static int portal_depth;
#define PORTAL_CLEAR_MAX 64
static void portal_clear(int x, int y, int z) {
  if (!is_portal(world_get(x, y, z)) || portal_depth >= PORTAL_CLEAR_MAX) return;
  portal_depth++;
  world_set(x, y, z, B_AIR);
  portal_clear(x + 1, y, z);
  portal_clear(x - 1, y, z);
  portal_clear(x, y + 1, z);
  portal_clear(x, y - 1, z);
  portal_clear(x, y, z + 1);
  portal_clear(x, y, z - 1);
  portal_depth--;
}

/* A portal block by a change: without its whole frame around it it goes too.
 * Breaking or blowing up the frame brings the whole connected interior down. */
void portal_changed(int x, int y, int z) {
  if (!is_portal(world_get(x, y, z))) return;
  int ox, oy, oz, axis, w, h;
  if (portal_frame(x, y, z, &ox, &oy, &oz, &axis, &w, &h)) return;   /* still framed */
  portal_clear(x, y, z);
}

/* A portal block broken outright (mined, blown up): the rest of the interior
 * around (x, y, z) goes with it, framed or not. */
void portal_broken(int x, int y, int z) {
  portal_clear(x + 1, y, z);
  portal_clear(x - 1, y, z);
  portal_clear(x, y + 1, z);
  portal_clear(x, y - 1, z);
  portal_clear(x, y, z + 1);
  portal_clear(x, y, z - 1);
}

/* Lighting the frame the player stands in (flint and steel): true if one lit */
bool portal_light_here(int x, int y, int z) {
  int ox, oy, oz, axis, w, h;
  if (!portal_frame(x, y, z, &ox, &oy, &oz, &axis, &w, &h)) return false;
  portal_fill(ox, oy, oz, axis, w, h);
  if (world_dim == DIM_OVER) {
    /* the portal the player leaves by leads back here */
    nb_ret_x = ox, nb_ret_y = oy, nb_ret_z = oz, nb_ret_ok = true;
  }
  return true;
}

/* A portal at (x, y, z), made where there is none: room carved around its
 * place, the frame's obsidian put in whole, the interior cleared and filled,
 * and a floor laid in front so the player arriving has somewhere to stand.
 * The portal is the smallest there is: 2 x 3 of air in a 4 x 5 of obsidian.
 *
 * The arrival is often inside solid rock: without the carving the player
 * comes out entombed in a wall, and a frame put in only where air was has
 * holes in it (and what portal blocks there are have no frame to hang in). */
static void portal_build(int x, int y, int z, int axis) {
  int ox = axis ? x : x - 1, oz = axis ? z - 1 : z;   /* the frame's near corner */
  for (int i = -2; i <= 3; i++)
    for (int j = 0; j <= 3; j++)
      for (int k = -1; k <= 1; k++) {
        int bx = axis ? ox + k : ox + i, by = y + j, bz = axis ? oz + i : oz + k;
        if (world_get(bx, by, bz) != B_AIR) world_set(bx, by, bz, B_AIR);
      }
  for (int i = -1; i <= 2; i++)
    for (int j = -1; j <= 3; j++) {
      bool corner = (i == -1 || i == 2) && (j == -1 || j == 3);
      bool inner = i >= 0 && i <= 1 && j >= 0 && j <= 2;
      if (corner || inner) continue;
      int bx = axis ? ox : ox + i, by = y + j, bz = axis ? oz + i : oz;
      if (world_get(bx, by, bz) != B_OBSIDIAN) world_set(bx, by, bz, B_OBSIDIAN);
    }
  portal_fill(ox, y, oz, axis, 2, 3);
  /* the floor in front of the portal's middle (one block out from its plane),
   * with air over it, and the player standing on it facing the portal */
  int sx = axis ? ox - 1 : ox + 1, sz = axis ? oz + 1 : oz - 1;
  for (int k = -1; k <= 1; k++) {
    int bxx = axis ? sx : sx + k, byy = y - 1, bzz = axis ? sz + k : sz;
    if (world_get(bxx, byy, bzz) == B_AIR) world_set(bxx, byy, bzz, B_OBSIDIAN);
    if (world_get(bxx, y, bzz) != B_AIR) world_set(bxx, y, bzz, B_AIR);
    if (world_get(bxx, y + 1, bzz) != B_AIR) world_set(bxx, y + 1, bzz, B_AIR);
  }
  pl.x = sx + 0.5f, pl.y = (float)y, pl.z = sz + 0.5f;
  pl.yaw = axis ? -90.0f : 0.0f, pl.pitch = 0.0f, pl.vx = pl.vy = pl.vz = 0.0f;
  portal_axis = axis;
}

/* The portal on the far side: one there if the chunks around have one, else
 * one made at the position. */
static void portal_far(int x, int y, int z, int axis) {
  for (int dz = -8; dz <= 8; dz++)
    for (int dx = -8; dx <= 8; dx++) {
      int bx = x + dx, bz = z + dz;
      for (int by = 34; by < 118; by++)
        if (is_portal(world_get(bx, by, bz))) {
          pl.x = bx + 0.5f, pl.y = (float)by, pl.z = bz + 0.5f;
          pl.vx = pl.vy = pl.vz = 0.0f;
          return;
        }
    }
  portal_build(x, y, z, axis);
}

/* Travelling: to the other dimension, at the portal's mapped position */
static void portal_travel(void) {
  int axis = portal_axis;
  int px = (int)floorf(pl.x), py = (int)floorf(pl.y), pz = (int)floorf(pl.z);
  int dx = px >= 0 ? px / 8 : -((-px + 7) / 8);
  int dz = pz >= 0 ? pz / 8 : -((-pz + 7) / 8);
  if (world_dim == DIM_OVER) {
    /* vanilla's mapping: a nether coordinate is an eighth of the overworld's */
    nb_ret_x = px, nb_ret_y = py, nb_ret_z = pz, nb_ret_ok = true;
    world_switch_dim(DIM_NETHER);
    pl.x = dx + 0.5f, pl.y = (float)py, pl.z = dz + 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    portal_far(dx, py >= 32 && py <= 118 ? py : 40, dz, axis);
  } else {
    int tx, ty, tz;
    if (nb_ret_ok) {
      tx = nb_ret_x, ty = nb_ret_y, tz = nb_ret_z;   /* the portal the player came through */
    } else {
      tx = px * 8, ty = py, tz = pz * 8;   /* (none remembered: the mapped position) */
    }
    world_switch_dim(DIM_OVER);
    pl.x = tx + 0.5f, pl.y = (float)ty, pl.z = tz + 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    portal_far(tx, ty >= 32 && ty <= 118 ? ty : 64, tz, axis);
    nb_ret_ok = false;   /* the return position is used once: arriving home */
  }
  portal_cool = PORTAL_COOL;
  portal_time = 0;
}

/* Every tick: standing in a portal waits a few seconds, then travels
 * (creative goes at once, as in Minecraft) */
void portal_tick(void) {
  if (portal_cool > 0) portal_cool--;
  if (pl.mode == 2) return;   /* a spectator goes through the blocks, not the portal */
  int x = (int)floorf(pl.x), y = (int)floorf(pl.y), z = (int)floorf(pl.z);
  if (portal_cool == 0 && is_portal(world_get(x, y, z))) {
    if (++portal_time >= (pl.mode == 1 ? 1 : PORTAL_WAIT)) portal_travel();
  } else if (portal_time)
    portal_time--;
}

/* how far through the wait standing in a portal is, 0..24: the screen's
 * portal-swirl overlay (nothing while travelling creative, which goes at once) */
int portal_anim(void) {
  if (portal_time <= 0) return 0;
  int a = portal_time * 24 / PORTAL_WAIT;
  return a > 24 ? 24 : a;
}
