/* The nether portal: the obsidian frame, lighting it, standing in it, and the
 * one on the far side.
 *
 * The frame is vanilla's: either axis, at least 2 x 3 of air inside an obsidian
 * rectangle 4 x 5 (the corners free), lit with flint and steel. Standing in it
 * for a few seconds travels like vanilla: the other dimension at the 1:8
 * mapping (overworld / 8 to the nether, nether * 8 to the overworld), to the
 * nearest portal around there, or to a new portal made at the mapped position
 * where there is none. Portals pair this way: each side finds the other's
 * closest, so two portals stay linked instead of sharing one. The search only
 * covers the loaded cache (40 x 40 x 24 around the arrival, ~20 blocks out:
 * vanilla looks 128, which does not fit), so very close portals still share
 * and very far ones always pair one-to-one. */
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
      bool row = y0 > dim_min_y();
      for (int i = lo; i <= hi && row; i++)
        if (!portal_open(ax_get(a, i, y0 - 1, a ? x : z))) row = false;
      if (!row) break;
      y0--;
    }
    for (int k = 0; k < PORTAL_MAX; k++) {
      bool row = y1 + 1 < dim_height();
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

/* Lighting the frame the player stands in (flint and steel): true if one lit.
 * (Linking needs nothing remembered: each travel maps 1:8 and takes the
 * nearest portal there, so a lit portal pairs with whatever is closest on the
 * far side.) */
bool portal_light_here(int x, int y, int z) {
  int ox, oy, oz, axis, w, h;
  if (!portal_frame(x, y, z, &ox, &oy, &oz, &axis, &w, &h)) return false;
  portal_fill(ox, oy, oz, axis, w, h);
  return true;
}

/* Liquid a new portal must not spawn in: standing water or lava (an ocean, a
 * lake, the lava sea). The frame is carved out of whatever is there, so without
 * this a portal aimed at an ocean ends up submerged. */
static bool portal_wet(int b) { return is_water(b) || is_lava(b); }

/* A new portal fits at (x, y, z) on its axis with nothing liquid: none in the
 * whole volume portal_build carves, and none in the ground under it (where the
 * obsidian floor goes - portal_build only lays it on air, so over water the
 * portal would float with no floor). Rock in the volume is fine: it is carved. */
static bool portal_dry(int x, int y, int z, int axis) {
  if (y < dim_min_y() + 1 || y + 3 >= dim_height()) return false;
  int ox = axis ? x : x - 1, oz = axis ? z - 1 : z;
  /* outside the loaded cache reads as air: never approve what is not loaded
   * (the build below writes nothing there, stranding the player). The box covers
   * either axis' footprint. */
  if (!world_loaded(ox - 2, y - 1, oz - 2) || !world_loaded(ox + 3, y + 3, oz + 3)) return false;
  for (int i = -2; i <= 3; i++)
    for (int j = 0; j <= 3; j++)
      for (int k = -1; k <= 1; k++) {
        int bx = axis ? ox + k : ox + i, by = y + j, bz = axis ? oz + i : oz + k;
        if (portal_wet(world_get(bx, by, bz))) return false;
      }
  for (int i = -2; i <= 3; i++)
    for (int k = -1; k <= 1; k++) {
      int bx = axis ? ox + k : ox + i, bz = axis ? oz + i : oz + k;
      if (portal_wet(world_get(bx, y - 1, bz))) return false;
    }
  return true;
}

/* A dry spot for the new portal near the mapped position (vanilla picks the
 * nearest ground the same way): rings out to 8 blocks, upward first so an
 * ocean crossing lands above the water instead of in it. Only loaded ground
 * counts, so mid-ocean with no land in the cache falls back to the mapped
 * spot (vanilla looks 128 blocks out, which does not fit). False: nowhere dry
 * near, and the caller builds at the mapped spot as before. */
static bool portal_spot(int x, int y, int z, int axis, int *ox, int *oy, int *oz) {
  if (y < dim_min_y() + 1) y = dim_min_y() + 1;
  if (y + 3 >= dim_height()) y = dim_height() - 4;
  for (int r = 0; r <= 8; r++)
    for (int dz = -r; dz <= r; dz++)
      for (int dx = -r; dx <= r; dx++) {
        if (dx != -r && dx != r && dz != -r && dz != r) continue;   /* the ring */
        for (int yy = y; yy + 3 < dim_height(); yy++)
          if (portal_dry(x + dx, yy, z + dz, axis)) {
            *ox = x + dx, *oy = yy, *oz = z + dz;
            return true;
          }
        for (int yy = y - 1; yy >= dim_min_y() + 1; yy--)
          if (portal_dry(x + dx, yy, z + dz, axis)) {
            *ox = x + dx, *oy = yy, *oz = z + dz;
            return true;
          }
      }
  return false;
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
  /* the floor on both sides of the portal (two blocks a side, one out from its
   * plane, under the interior), with air over it, and the player standing on
   * the front side facing the portal: with only one side the player arriving
   * walks one block forward off the edge */
  for (int s = 0; s < 2; s++) {
    for (int i = 0; i < 2; i++) {
      int bxx = axis ? (s ? ox + 1 : ox - 1) : ox + i;
      int bzz = axis ? oz + i : (s ? oz + 1 : oz - 1);
      if (world_get(bxx, y - 1, bzz) == B_AIR) world_set(bxx, y - 1, bzz, B_OBSIDIAN);
      if (world_get(bxx, y, bzz) != B_AIR) world_set(bxx, y, bzz, B_AIR);
      if (world_get(bxx, y + 1, bzz) != B_AIR) world_set(bxx, y + 1, bzz, B_AIR);
    }
  }
  int sx = axis ? ox - 1 : ox + 1, sz = axis ? oz + 1 : oz - 1;
  pl.x = sx + 0.5f, pl.y = (float)y, pl.z = sz + 0.5f;
  pl.yaw = axis ? -90.0f : 0.0f, pl.pitch = 0.0f, pl.vx = pl.vy = pl.vz = 0.0f;
  portal_axis = axis;
}

/* The portal on the far side: the nearest one the loaded cache has around the
 * mapped position (x, y, z), else one made there. Vanilla searches 128 blocks;
 * the cache only holds 40 x 40 x 24, so this is the nearest within ~20 blocks.
 * The nearest wins (not the first scanned), so each portal pairs with the
 * closest on the other side instead of every portal sharing the first-made. */
static void portal_far(int x, int y, int z, int axis) {
  int found = 0, best_x = x, best_y = y, best_z = z, best_d = 0;
  for (int lz = 0; lz < VCZ; lz++)
    for (int lx = 0; lx < VCX; lx++)
      for (int ly = 0; ly < VCY; ly++) {
        int bx = vc_x0 + lx, by = vc_y0 + ly, bz = vc_z0 + lz;
        if (!is_portal(world_get(bx, by, bz))) continue;
        int dx = bx - x, dy = by - y, dz = bz - z;
        int d = dx * dx + dy * dy + dz * dz;
        if (!found || d < best_d) found = 1, best_d = d, best_x = bx, best_y = by, best_z = bz;
      }
  if (found) {
    /* the bottom of this portal's column: standing on its floor, not halfway up */
    int bx = best_x, by = best_y, bz = best_z;
    for (int k = 0; k < 5; k++) {
      if (!is_portal(world_get(bx, by - 1, bz))) break;
      by--;
    }
    pl.x = bx + 0.5f, pl.y = (float)by, pl.z = bz + 0.5f;
    pl.vx = pl.vy = pl.vz = 0.0f;
    /* its axis, from its shape (a neighbour along X runs along X, NS): the next
     * travel builds with it */
    if (is_portal(world_get(bx + 1, by, bz)) || is_portal(world_get(bx - 1, by, bz))) portal_axis = 0;
    else if (is_portal(world_get(bx, by, bz + 1)) || is_portal(world_get(bx, by, bz - 1))) portal_axis = 1;
    else portal_axis = axis;
    /* the cache around the arrival (not the mapped spot a dozen blocks off):
     * what is built next lands in a loaded cache */
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    return;
  }
  /* no portal near: a new one at the mapped spot, moved to dry ground where it
   * aimed underwater (portal_spot), so crossings never land submerged */
  int nx = x, ny = y, nz = z;
  portal_spot(x, y, z, axis, &nx, &ny, &nz);
  portal_build(nx, ny, nz, axis);
}

/* floor division by 8 (vanilla's coord mapping, right for negatives too) */
static inline int div8(int v) { return v >= 0 ? v / 8 : -((-v + 7) / 8); }

/* Travelling: to the other dimension at vanilla's 1:8 mapping, both ways */
static void portal_travel(void) {
  int axis = portal_axis;
  int px = (int)floorf(pl.x), py = (int)floorf(pl.y), pz = (int)floorf(pl.z);
  if (world_dim == DIM_OVER) {
    /* vanilla's mapping: a nether coordinate is an eighth of the overworld's */
    int dx = div8(px), dz = div8(pz), dy = py >= 32 && py <= 118 ? py : 40;
    world_switch_dim(DIM_NETHER);
    pl.x = dx + 0.5f, pl.y = (float)dy, pl.z = dz + 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    portal_far(dx, dy, dz, axis);
  } else {
    /* and back: eight overworld blocks to one of the nether's */
    int tx = px * 8, tz = pz * 8, ty = py >= 32 && py <= 118 ? py : 64;
    world_switch_dim(DIM_OVER);
    pl.x = tx + 0.5f, pl.y = (float)ty, pl.z = tz + 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    portal_far(tx, ty, tz, axis);
  }
  portal_cool = PORTAL_COOL;
  portal_time = 0;
}

/* The End's portal: to the obsidian ledge at (100, 49, 0), as vanilla's
 * WorldProviderEnd.getSpawnCoordinate and Teleporter.placeInPortal put it; and back to
 * the Overworld's spawn. The End has no nether portal frame of its own. */
static void end_travel(void) {
  if (world_dim == DIM_END) {
    world_switch_dim(DIM_OVER);
    player_respawn();
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    gui_message("You came back from the End");
  } else {
    world_switch_dim(DIM_END);
    pl.x = 100.5f, pl.y = 49.0f, pl.z = 0.5f;
    pl.vx = pl.vy = pl.vz = 0, pl.fall = 0;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    for (int dz = -2; dz <= 2; dz++)
      for (int dx = -2; dx <= 2; dx++) {
        world_set(100 + dx, 48, dz, B_OBSIDIAN);
        for (int y = 49; y <= 51; y++) world_set(100 + dx, y, dz, B_AIR);
      }
    pl.yaw = 90, pl.pitch = 0;
    gui_message("You entered the End");
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
  if (portal_cool == 0 && world_get(x, y, z) == B_END_PORTAL) {
    if (++portal_time >= (pl.mode == 1 ? 1 : PORTAL_WAIT / 2)) end_travel();
  } else if (portal_cool == 0 && is_portal(world_get(x, y, z))) {
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
