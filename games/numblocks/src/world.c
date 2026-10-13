/* The world around the player: the block cache (see nb.h), filled from the
 * generator a chunk at a time with the player's edits on top, and its light.
 *
 * The cache moves in steps of 8 blocks (4 vertically) once the player comes
 * near an edge: what it still covers is moved, the rest is generated. */
#include <stdlib.h>
#include "nb.h"
#include "edits.h"

uint8_t vc[VC_BYTES];
uint8_t vl[VCY * VCZ * VCX];
uint8_t vbiome[VCZ * VCX];
uint8_t vtop[VCZ * VCX];
uint8_t vmac[MCY * MCZ * MCX];   /* 4 x 4 x 4 regions: 0 all air, 2 water+air only, 1 other */
static uint8_t vgtop[VCZ * VCX];    /* per column: the generator's highest block (not air) */
int vc_x0, vc_y0, vc_z0;
static bool vc_valid;

/* one chunk of the generator's output; also the edit log's merge buffer. States are 9
 * bits each, as in the cache (16 x 16 x VCY cells = 6912 B) */
static uint32_t slab32[16 * 16 * VCY / 8 * 9 / 4 + 1];
#define slab ((uint8_t *)slab32)
#define SLAB_CELLS (16 * 16 * VCY)

static inline int floordiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static inline int ifloor(float v) { int i = (int)v; return v < (float)i ? i - 1 : i; }

/* light stops at opaque blocks; leaves and water let some through (1.8: opacity 1 and 3) */
static inline int opacity(int b) {
  if (blk_flags[b] & BF_OPAQUE) return 15;
  int m = blk_model[b];
  if (m == M_LEAVES || b == B_COBWEB) return 1;
  if (m == M_LIQUID) return 3;
  return 0;
}

/* ---------------------------------------------------------------- light */
/* Light in the box [x0,x1) x [y0,y1) x [z0,z1) of the cache, both kinds at
 * once. Sky light: 15 straight under the open sky; block light: what the
 * block gives (torch 14, lava and glowstone 15...). Then both spread sideways
 * and down, one level less a block (more through leaves and water), as
 * Minecraft's light does: in sweeps along each direction, twice. */
static void light_box(int x0, int y0, int z0, int x1, int y1, int z1) {
  if (x0 < 0) x0 = 0;
  if (z0 < 0) z0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > VCX) x1 = VCX;
  if (z1 > VCZ) z1 = VCZ;
  if (y1 > VCY) y1 = VCY;
  for (int z = z0; z < z1; z++)
    for (int x = x0; x < x1; x++) {
      /* the nether has no sky (not even a lit one): its light is the blocks' alone,
       * so the sky pass is skipped and only block light spreads. Without any sky
       * the dark reads darker than the overworld's night, so everything glows
       * eight levels brighter (lava still stops at 15): the lava sea then reaches
       * and the wastes stay readable far from any glow. */
      if (world_dim == DIM_NETHER) {
        for (int y = VCY - 1; y >= y0; y--) {
          int i = VC_I(x, y, z), b = vc_get(i);
          int lb = blk_light[b] + 8;
          if (lb > 15) lb = 15;
          if (y < y1) vl[i] = (uint8_t)(lb << 4);
        }
        continue;
      }
      int top = vtop[z * VCX + x] - vc_y0;   /* local y of the first sky-lit cell */
      int lvl = 15;
      for (int y = VCY - 1; y >= y0; y--) {
        int i = VC_I(x, y, z), b = vc_get(i);
        if (y < top) lvl = 0;
        else if (b != B_AIR) {
          lvl -= opacity(b);
          if (lvl < 0) lvl = 0;
        }
        if (y < y1) vl[i] = (uint8_t)(lvl | blk_light[b] << 4);
      }
    }
  /* any block light here or at the edges? if not, only sky light spreads, and cells already at
   * full sky light (most of the air) have nothing to gain */
  bool blk = false;
  for (int y = y0 > 0 ? y0 - 1 : 0; y < y1 + 1 && y < VCY && !blk; y++)
    for (int z = z0 > 0 ? z0 - 1 : 0; z < z1 + 1 && z < VCZ && !blk; z++) {
      const uint8_t *r = &vl[VC_I(0, y, z)];
      for (int x = x0 > 0 ? x0 - 1 : 0; x < x1 + 1 && x < VCX; x++)
        if (r[x] >> 4) {
          blk = true;
          break;
        }
    }
  for (int round = 0; round < 2; round++)
    for (int dir = 0; dir < 2; dir++) {
      int ys = dir ? y0 : y1 - 1, ye = dir ? y1 : y0 - 1, yi = dir ? 1 : -1;
      int zs = dir ? z1 - 1 : z0, ze = dir ? z0 - 1 : z1, zi = dir ? -1 : 1;
      int xs = dir ? x1 - 1 : x0, xe = dir ? x0 - 1 : x1, xi = dir ? -1 : 1;
      for (int y = ys; y != ye; y += yi)
        for (int z = zs; z != ze; z += zi)
          for (int x = xs; x != xe; x += xi) {
            int i = VC_I(x, y, z), cur = vl[i];
            if ((cur & 15) == 15 && !blk) continue;
            int op = opacity(vc_get(i));
            if (op >= 15) continue;
            /* the brightest neighbours (sky and block light apart), less what this cell takes */
            int ms = 0, mb = 0;
            if (x > 0 && x < VCX - 1 && z > 0 && z < VCZ - 1 && y > 0 && y < VCY - 1) {
              const uint8_t *c = &vl[i];
              int a = c[-1], b = c[1], d = c[-VCX], e = c[VCX], f = c[VCX * VCZ], g = c[-VCX * VCZ];
#define MX(p, q) ((p) > (q) ? (p) : (q))
              ms = MX(MX(MX(a & 15, b & 15), MX(d & 15, e & 15)), MX(f & 15, g & 15));
              if (blk) mb = MX(MX(MX(a >> 4, b >> 4), MX(d >> 4, e >> 4)), MX(f >> 4, g >> 4));
#undef MX
            } else {
              /* Under the world's lowest layer is the void, which Minecraft's sky light layer
               * reads 15 outside the world: a cell of the bottom row takes light from below as
               * from any neighbour, so a hole in the floor is lit from it (14) rather than
               * pitch black, as the intact underside is lit by its 15 (face_lit) */
              if (y == 0 && vc_y0 == WORLD_MIN_Y && world_dim == DIM_OVER) ms = 15;
              int n[6] = {x > 0 ? i - 1 : -1, x < VCX - 1 ? i + 1 : -1, z > 0 ? i - VCX : -1,
                          z < VCZ - 1 ? i + VCX : -1, y < VCY - 1 ? i + VCX * VCZ : -1, y > 0 ? i - VCX * VCZ : -1};
              for (int k = 0; k < 6; k++)
                if (n[k] >= 0) {
                  int v = vl[n[k]];
                  if ((v & 15) > ms) ms = v & 15;
                  if ((v >> 4) > mb) mb = v >> 4;
                }
            }
            int dec = op ? op : 1, bs = cur & 15, bb = cur >> 4;
            if (ms - dec > bs) bs = ms - dec;
            if (mb - dec > bb) bb = mb - dec;
            int nv = bs | bb << 4;
            if (nv != cur) vl[i] = (uint8_t)nv;
          }
    }
}

/* Blocks changed (placed, broken, water flowing, a tree growing) relight the box around
 * them, but once a frame, all together (world_light_flush): a light_box each was slow */
static int dirty[6];   /* the box to relight, x0 y0 z0 x1 y1 z1 (cache), empty if x0 >= x1 */
static void light_mark(int x0, int y0, int z0, int x1, int y1, int z1) {
  if (dirty[0] >= dirty[3]) {
    dirty[0] = x0, dirty[1] = y0, dirty[2] = z0, dirty[3] = x1, dirty[4] = y1, dirty[5] = z1;
    return;
  }
  if (x0 < dirty[0]) dirty[0] = x0;
  if (y0 < dirty[1]) dirty[1] = y0;
  if (z0 < dirty[2]) dirty[2] = z0;
  if (x1 > dirty[3]) dirty[3] = x1;
  if (y1 > dirty[4]) dirty[4] = y1;
  if (z1 > dirty[5]) dirty[5] = z1;
}
void world_light_flush(void) {
  if (dirty[0] >= dirty[3]) return;
  light_box(dirty[0], dirty[1], dirty[2], dirty[3], dirty[4], dirty[5]);
  dirty[0] = dirty[3] = 0;
}

/* ---------------------------------------------------------------- the empty regions */
static void mac_box(int x0, int y0, int z0, int x1, int y1, int z1) {
  for (int my = y0 >> 2; my <= (y1 - 1) >> 2 && my < MCY; my++)
    for (int mz = z0 >> 2; mz <= (z1 - 1) >> 2 && mz < MCZ; mz++)
      for (int mx = x0 >> 2; mx <= (x1 - 1) >> 2 && mx < MCX; mx++) {
        uint8_t any = 0;
        bool water = true;
        for (int y = my * 4; y < my * 4 + 4; y++)
          for (int z = mz * 4; z < mz * 4 + 4; z++) {
            int r = VC_I(mx * 4, y, z);
            int b0 = vc_get(r), b1 = vc_get(r + 1), b2 = vc_get(r + 2), b3 = vc_get(r + 3);
            any |= (uint8_t)(b0 | b1 | b2 | b3);
            for (int k = 0; k < 4; k++) water = water && (vc_get(r + k) == B_AIR || is_water(vc_get(r + k)));
          }
        vmac_set((my * MCZ + mz) * MCX + mx, !any ? 0 : water ? 2 : 1);
      }
}

/* ---------------------------------------------------------------- filling */
/* the chunk (cx, cz) for the cache cells not already filled: x, z in the cache
 * box [nx0, nx1) x [nz0, nz1) (world), every y of the cache */
/* the chunk (cx, cz), rows [ylo, yhi) of the cache, for the columns of the
 * cache inside it; columns the old cache had (keep) keep the rows it had
 * (old bottom oy) */
#define BIT(set, i) ((set)[(i) >> 3] >> ((i) & 7) & 1)
static void fill_chunk(int cx, int cz, const uint8_t *keep, int oy, int ylo, int yhi, bool air) {
  int h = yhi - ylo;
  if (air)
    memset(slab, 0, (size_t)h * 256 / 8 * 9);   /* (rows above all it generates: only the edits; air is 0, so the bits zero) */
  else gen_slab(cx, cz, vc_y0 + ylo, h, slab);
  edits_apply(cx, cz, vc_y0 + ylo, h, slab);
  for (int z = 0; z < 16; z++) {
    int lz = cz * 16 + z - vc_z0;
    if (lz < 0 || lz >= VCZ) continue;
    for (int x = 0; x < 16; x++) {
      int lx = cx * 16 + x - vc_x0;
      if (lx < 0 || lx >= VCX) continue;
      int col = lz * VCX + lx;
      bool old_col = keep && BIT(keep, col);
      for (int y = ylo; y < yhi; y++) {
        /* cells the old cache had are kept (their content is the same) */
        if (old_col && y + vc_y0 - oy >= 0 && y + vc_y0 - oy < VCY) continue;
        vc_set(VC_I(lx, y, lz), b9_get(slab, ((y - ylo) * 16 + z) * 16 + x));
      }
      if (!old_col) {
        vbiome[col] = (uint8_t)gen_biome(cx * 16 + x, cz * 16 + z);
        int gt = gen_top(cx * 16 + x, cz * 16 + z), lt = gt;
        vgtop[col] = (uint8_t)gt;
        /* the sky light's top is the highest block that stops light: snow layers, flowers, tall
         * grass on top let it all through (else they and what they stand on were lit too dark) */
        for (int ly = gt - vc_y0; ly >= 0 && ly < VCY && opacity(vc_get(VC_I(lx, ly, lz))) == 0; ly--) lt--;
        int t = edits_top(cx * 16 + x, cz * 16 + z, lt + 1);
        vtop[col] = (uint8_t)(t < 0 ? 0 : t > 255 ? 255 : t);
      }
    }
  }
}

static uint8_t kept[VCZ * VCX / 8];   /* columns the old cache covered (moved, not generated): a bit each */

/* The generator places blocks; it does not tick them. Sand and gravel are left hanging
 * over the air a cave carved out from under them; water and lava stand where a cave, a
 * ravine or a spring opened into them. Nothing notices until a change beside them runs
 * neighbours_changed, so this is that first update, run once a move of the cache is
 * complete - every chunk of it whole, so what it reads is what the world holds - and over
 * the whole cache: the chunks just made are the ones entering the render distance (the
 * lazy "when the player renders it in"), and the kept ones cost a scan and find nothing.
 *   - a hanging sand or gravel block drops (settle_fall, player.c);
 *   - a liquid that can move at all is woken (fluid_wake, tick.c) and flows from there.
 * The move's chunks are made one a frame, so the pass runs when the last of them is made:
 * the cost lands once every few frames and never inside a chunk's own making. */
static void settle_cache(void) {
  for (int lz = 0; lz < VCZ; lz++)
    for (int lx = 0; lx < VCX; lx++) {
      int wx = vc_x0 + lx, wz = vc_z0 + lz;
      for (int ly = 0; ly < VCY; ly++) {
        int b = vc_get(VC_I(lx, ly, lz));
        if (b == B_SAND || b == B_RED_SAND || b == B_GRAVEL) settle_fall(wx, vc_y0 + ly, wz);
        else if (blk_model[b] == M_LIQUID) fluid_wake(wx, vc_y0 + ly, wz);
      }
    }
}

/* Chunks still to generate after a move: the one under the player is made
 * at once, the others one a frame (world_follow), nearest first. Until then
 * their columns are air to look at and solid to everything that moves. */
static uint8_t vpend[(VCZ * VCX + 7) / 8];   /* a bit a column (1600 cols -> 200 B) */
static inline int vpend_get(int col) { return (vpend[col >> 3] >> (col & 7)) & 1; }
static inline void vpend_set(int col) { vpend[col >> 3] |= (uint8_t)(1u << (col & 7)); }
static inline void vpend_clear(int col) { vpend[col >> 3] &= (uint8_t)~(1u << (col & 7)); }
typedef struct { int16_t cx, cz; int8_t ylo, yhi; } PendChunk;
#define NPEND 32
static PendChunk pend[NPEND];
static int npend, pend_oy;
static bool pend_any;

static void fill_pending(int k) {
  PendChunk p = pend[k];
  pend[k] = pend[--npend];
  fill_chunk(p.cx, p.cz, pend_any ? kept : NULL, pend_oy, p.ylo, p.yhi, false);
  int x0 = p.cx * 16 - vc_x0, z0 = p.cz * 16 - vc_z0;
  for (int z = z0 < 0 ? 0 : z0; z < z0 + 16 && z < VCZ; z++)
    for (int x = x0 < 0 ? 0 : x0; x < x0 + 16 && x < VCX; x++) vpend_clear(z * VCX + x);
  if (!npend) settle_cache();   /* (the last chunk of the move: the world is whole again) */
}

/* The chunks waiting to be made, meanwhile: the blocks about them carried on, not holes the sky shows
 * through (a whole stretch of ground blinked out and back as they were made, one a frame). A column the
 * cache kept gains rows: up to the generator's top its top row goes on up (above it, air), and its
 * bottom row goes on down (stone where that is open). A new column: the nearest column there is, on
 * the way to the player. */
static void stand_in(bool any, int oy, int px, int pz) {
  int lo = any ? oy - vc_y0 : VCY, hi = any ? lo + VCY : VCY;   /* (the rows the old cache had) */
  for (int col = 0; col < VCX * VCZ; col++) {
    if (!vpend_get(col) || !any || !BIT(kept, col)) continue;
    int lx = col % VCX, lz = col / VCX;
    if (lo > 0 && lo < VCY) {
      int b = vc_get(VC_I(lx, lo, lz));
      if (!(blk_flags[b] & BF_SOLID)) b = B_STONE;
      for (int y = 0; y < lo; y++) vc_set(VC_I(lx, y, lz), b);
    }
    if (hi > 0 && hi < VCY) {
      int b = vc_get(VC_I(lx, hi - 1, lz));
      if (!(blk_flags[b] & BF_OPAQUE)) b = B_AIR;
      for (int y = hi; y < VCY; y++) vc_set(VC_I(lx, y, lz), y + vc_y0 <= vgtop[col] ? b : B_AIR);
    }
  }
  int mx = px - vc_x0, mz = pz - vc_z0;   /* (the player's column: always made at once) */
  for (int col = 0; col < VCX * VCZ; col++) {
    if (!vpend_get(col) || (any && BIT(kept, col))) continue;
    /* towards the player until a column that is there */
    int lx = col % VCX, lz = col / VCX, sx = lx, sz = lz;
    for (int k = 0; k < VCX + VCZ; k++) {
      int s = sz * VCX + sx;
      if (!vpend_get(s) || (any && BIT(kept, s))) break;
      if (sx != mx && abs(sx - mx) >= abs(sz - mz)) sx += sx < mx ? 1 : -1;
      else if (sz != mz) sz += sz < mz ? 1 : -1;
      else break;
    }
    int s = sz * VCX + sx;
    for (int y = 0; y < VCY; y++) vc_set(VC_I(lx, y, lz), vc_get(VC_I(sx, y, sz)));
    vtop[col] = vtop[s], vbiome[col] = vbiome[s], vgtop[col] = vgtop[s];
  }
}

static void recenter(int nx0, int ny0, int nz0, int px, int pz) {
  edits_forget();
  bool was_valid = vc_valid;
  while (npend) fill_pending(0);   /* (a move before the last one finished: finish it first) */
  int dx = nx0 - vc_x0, dy = ny0 - vc_y0, dz = nz0 - vc_z0;
  bool any = vc_valid && dx > -VCX && dx < VCX && dz > -VCZ && dz < VCZ && dy > -VCY && dy < VCY;
  /* move what stays: memmove handles overlap within a row; rows go in an order
   * that never overwrites a cell before it is read */
  if (any) {
    int ys = dy >= 0 ? 0 : VCY - 1, ye = dy >= 0 ? VCY : -1, yi = dy >= 0 ? 1 : -1;
    int zs = dz >= 0 ? 0 : VCZ - 1, ze = dz >= 0 ? VCZ : -1, zi = dz >= 0 ? 1 : -1;
    int xs = dx >= 0 ? 0 : VCX - 1, xe = dx >= 0 ? VCX : -1, xi = dx >= 0 ? 1 : -1;
    for (int y = ys; y != ye; y += yi)
      for (int z = zs; z != ze; z += zi) {
        int sy = y + dy, sz = z + dz;
        if (sy < 0 || sy >= VCY || sz < 0 || sz >= VCZ) continue;
        int x0 = dx >= 0 ? 0 : -dx, x1 = dx >= 0 ? VCX - dx : VCX;
        /* the slide moves whole multiples of 8 cells (9 bytes each): byte-aligned */
        if (x0 < x1)
          memmove(vc + (VC_I(x0, y, z) >> 3) * 9, vc + (VC_I(x0 + dx, sy, sz) >> 3) * 9, (size_t)(x1 - x0) / 8 * 9);
      }
    for (int z = zs; z != ze; z += zi)
      for (int x = xs; x != xe; x += xi) {
        int sx = x + dx, sz = z + dz;
        bool in = sx >= 0 && sx < VCX && sz >= 0 && sz < VCZ;
        int col = z * VCX + x;
        kept[col >> 3] = (uint8_t)((kept[col >> 3] & ~(1 << (col & 7))) | in << (col & 7));
        if (in) {
          vbiome[z * VCX + x] = vbiome[sz * VCX + sx];
          vtop[z * VCX + x] = vtop[sz * VCX + sx];
          vgtop[z * VCX + x] = vgtop[sz * VCX + sx];
        }
      }
  } else memset(kept, 0, sizeof kept);
  int oy = vc_y0;
  vc_x0 = nx0;
  vc_y0 = ny0;
  vc_z0 = nz0;
  pend_oy = any ? oy : vc_y0 + 100000;
  pend_any = any;
  for (int cz = floordiv(nz0, 16); cz <= floordiv(nz0 + VCZ - 1, 16); cz++)
    for (int cx = floordiv(nx0, 16); cx <= floordiv(nx0 + VCX - 1, 16); cx++) {
      /* what this chunk needs: new columns (every row), or only the new rows */
      bool cols = !any;
      for (int z = 0; z < 16 && !cols; z++)
        for (int x = 0; x < 16 && !cols; x++) {
          int lx = cx * 16 + x - vc_x0, lz = cz * 16 + z - vc_z0;
          if (lx >= 0 && lx < VCX && lz >= 0 && lz < VCZ && !BIT(kept, lz * VCX + lx)) cols = true;
        }
      int ylo = 0, yhi = VCY;
      bool air = false;
      if (!cols) {
        if (dy == 0) continue;
        if (dy > 0) ylo = VCY - dy;
        else yhi = -dy;
        /* new rows above everything the generator put in these columns: air, at once (going up a
         * hill, or falling from high up, moves the cache, and making every chunk again for its new
         * rows was slow) */
        air = true;
        for (int z = 0; z < 16 && air; z++)
          for (int x = 0; x < 16; x++) {
            int lx = cx * 16 + x - vc_x0, lz = cz * 16 + z - vc_z0;
            if (lx >= 0 && lx < VCX && lz >= 0 && lz < VCZ && vgtop[lz * VCX + lx] >= vc_y0 + ylo) {
              air = false;
              break;
            }
          }
      }
      /* the player's chunk (and any within 3 blocks of the player), or a first fill: now */
      bool near = px >= cx * 16 - 3 && px < cx * 16 + 19 && pz >= cz * 16 - 3 && pz < cz * 16 + 19;
      if (!was_valid || near || air || npend >= 24) {
        fill_chunk(cx, cz, any ? kept : NULL, pend_oy, ylo, yhi, air);
        continue;
      }
      pend[npend++] = (PendChunk){(int16_t)cx, (int16_t)cz, (int8_t)ylo, (int8_t)yhi};
      for (int z = 0; z < 16; z++)
        for (int x = 0; x < 16; x++) {
          int lx = cx * 16 + x - vc_x0, lz = cz * 16 + z - vc_z0;
          if (lx < 0 || lx >= VCX || lz < 0 || lz >= VCZ) continue;
          vpend_set(lz * VCX + lx);
        }
    }
  stand_in(any, oy, px, pz);
  vc_valid = true;
  if (!npend) settle_cache();   /* (no chunk of the move waiting: its blocks get their first update) */
  dirty[0] = dirty[3] = 0;   /* (all of it, now) */
  light_box(0, 0, 0, VCX, VCY, VCZ);
  mac_box(0, 0, 0, VCX, VCY, VCZ);
}

void world_new(int64_t seed, const char *name) {
  world_dim = DIM_OVER;   /* a world made now starts in the overworld */
  if (world_type == WT_FLAT) gen_set_flat_preset(world_preset);
  else gen_set_flat(0);
  gen_init(seed);
  gen_set_dim(world_dim);   /* the dimension's generator (one, shared with the overworld) */
  edits_set_dim(world_dim);   /* and its records */
  gen_spawner_hook = spawners_made;   /* a dungeon's spawner gets its mob, and the world keeps it */
  edits_setup(name, slab32, sizeof slab32 / 4);
  edits_clear();
  vc_valid = false;
  npend = 0;
  memset(vpend, 0, sizeof vpend);
}

/* The world spawn point. gen_spawn runs the whole generator, so it takes the scratch with it
 * (gen_scratch), here on this function's stack. */
void world_spawn(int *x, int *y, int *z) {
  uint8_t gs[GEN_SCRATCH];
  memset(gs, 0, GEN_SCRATCH);
  gen_scratch(gs);
  gen_spawn(x, y, z);
  gen_scratch(NULL);   /* the buffer goes with this frame */
}

static void world_follow_gen(float x, float y, float z);
static int vc_dim;   /* the dimension the cache is filled for (world_dim's) */

/* The dimension the world is in: the whole volume again, with the other
 * dimension's generator (one cache, shared: the blocks the player left behind
 * come back from the edit records when they return). */
void world_switch_dim(int dim) {
  world_dim = (uint8_t)dim;
  gen_set_dim(dim);
  edits_set_dim(dim);
  npend = 0;
  memset(vpend, 0, sizeof vpend);
  vc_valid = false;
  vc_dim = dim;
  edits_forget();
}

/* Keeps the cache around (x, y, z). The generator's scratch rides along for the whole of the
 * move (gen.c: its passes never overlap, so one buffer serves them all): on this function's
 * stack, where the game would otherwise keep 7.7 KB of it in the app's RAM. */
void world_follow(float x, float y, float z) {
  uint8_t gs[GEN_SCRATCH];
  memset(gs, 0, GEN_SCRATCH);
  gen_scratch(gs);
  world_follow_gen(x, y, z);
  gen_scratch(NULL);   /* the buffer goes with this frame */
}

/* Near a side of the cache the next move is coming: the chunks it will make there (one row of
 * them, 8 blocks in) need the terrain of the chunks around them, which takes most of making a
 * chunk. Made ahead, one chunk's a frame, the move's frames making the chunks are shorter.
 * (mx, mz: where the camera is headed, so only that side warms; still: the nearer side.) */
static void prepare_ahead(int lx, int lz, int mx, int mz) {
  int dx = mx > 0 ? (lx >= 18 ? 1 : 0) : mx < 0 ? (lx < 22 ? -1 : 0) : (lx >= 21 ? 1 : lx < 19 ? -1 : 0);
  int dz = mz > 0 ? (lz >= 18 ? 1 : 0) : mz < 0 ? (lz < 22 ? -1 : 0) : (lz >= 21 ? 1 : lz < 19 ? -1 : 0);
  if (dx && dz) {   /* (one side at a time: the nearer) */
    if ((dx > 0 ? VCX - lx : lx) < (dz > 0 ? VCZ - lz : lz)) dz = 0;
    else dx = 0;
  }
  if (dx) {
    int cx = floordiv(dx > 0 ? vc_x0 + VCX : vc_x0 - 8, 16);
    for (int cz = floordiv(vc_z0, 16); cz <= floordiv(vc_z0 + VCZ - 1, 16); cz++)
      if (gen_prepare(cx, cz)) return;
  } else if (dz) {
    int cz = floordiv(dz > 0 ? vc_z0 + VCZ : vc_z0 - 8, 16);
    for (int cx = floordiv(vc_x0, 16); cx <= floordiv(vc_x0 + VCX - 1, 16); cx++)
      if (gen_prepare(cx, cz)) return;
  }
}

static void world_follow_gen(float x, float y, float z) {
  int px = ifloor(x), py = ifloor(y), pz = ifloor(z);
  /* a new dimension: the whole volume again (nothing of the other one's is here) */
  if (vc_dim != world_dim) {
    vc_valid = false;
    vc_dim = world_dim;
    npend = 0;
    memset(vpend, 0, sizeof vpend);
    edits_forget();
  }
  static int wfx, wfz;
  static bool wfgot;
  int lx = px - vc_x0, ly = py - vc_y0, lz = pz - vc_z0;
  /* how many of the cache's blocks the camera keeps to each side. The slides are 8 blocks,
   * so the camera ends up 8 blocks either side of the middle of that band after one: 16 is
   * the farthest from both sides a 40 x 40 cache can keep it. The fog closes at the
   * cache's edge (view_reach), so the ground fills the view out to the render distance. */
  static const int M = VCX / 2 - 4;
  /* High above the ground (flying, falling) the cache goes down as far as it can with room above the
   * player to build: what is in sight below shows, and falling makes nothing for the air it passes */
  static bool high;
  int ground = vc_valid && (unsigned)lx < VCX && (unsigned)lz < VCZ ? vgtop[lz * VCX + lx] : py;
  if (py - ground > 10) high = true;
  if (py - ground < 6) high = false;
  int ny_high = (py + 8 - VCY) & ~3;
  bool ok = high ? ly >= 4 && ly < VCY - 4 && !(vc_y0 > ground - 4 && ny_high < vc_y0) : ly >= 8 && ly < VCY - 8;
  if (!(vc_valid && lx >= M && lx < VCX - M && lz >= M && lz < VCZ - M && ok)) {
    /* (the slide lands the camera M blocks in, and M or M+7 out: never on a side) */
    int nx0 = (px - M) & ~7, nz0 = (pz - M) & ~7;
    int ny0 = high ? ny_high : (py - VCY / 2 + 2) & ~3;
    if (ny0 < dim_min_y()) ny0 = dim_min_y();
    if (ny0 > dim_height() - VCY) ny0 = dim_height() - VCY;
    if (!vc_valid || nx0 != vc_x0 || ny0 != vc_y0 || nz0 != vc_z0) {
      recenter(nx0, ny0, nz0, px, pz);
      return;
    }
  }
  if (npend) {
    /* one waiting chunk a frame, the nearest first; then its light and empty regions */
    int best = 0, bd = 1 << 30;
    for (int k = 0; k < npend; k++) {
      int ddx = pend[k].cx * 16 + 8 - px, ddz = pend[k].cz * 16 + 8 - pz, d = ddx * ddx + ddz * ddz;
      if (d < bd) bd = d, best = k;
    }
    int x0 = pend[best].cx * 16 - vc_x0, z0 = pend[best].cz * 16 - vc_z0;
    fill_pending(best);
    light_mark(x0 - 4, 0, z0 - 4, x0 + 20, VCY, z0 + 20);
    mac_box(x0 < 0 ? 0 : x0, 0, z0 < 0 ? 0 : z0, x0 + 16 > VCX ? VCX : x0 + 16, VCY, z0 + 16 > VCZ ? VCZ : z0 + 16);
  } else prepare_ahead(lx, lz, wfgot ? px - wfx : 0, wfgot ? pz - wfz : 0);
  wfx = px, wfz = pz, wfgot = true;
  world_light_flush();
}

bool world_pending(void) { return npend > 0; }

bool world_pending_at(int x, int y, int z) {
  if (!world_loaded(x, y, z)) return false;
  return vpend_get((z - vc_z0) * VCX + x - vc_x0);
}

bool world_loaded(int x, int y, int z) {
  return (unsigned)(x - vc_x0) < VCX && (unsigned)(y - vc_y0) < VCY && (unsigned)(z - vc_z0) < VCZ;
}

int world_get(int x, int y, int z) {
  if (y < WORLD_MIN_Y) return B_AIR;   /* under the world is the void, as in Minecraft below its lowest blocks */
  if (!world_loaded(x, y, z)) return B_AIR;
  if (vpend_get((z - vc_z0) * VCX + x - vc_x0)) return B_STONE;   /* not made yet: nothing goes in */
  return vc_get(VC_I(x - vc_x0, y - vc_y0, z - vc_z0));
}

void world_set(int x, int y, int z, int b) {
  if (!world_loaded(x, y, z) || y < dim_min_y() || y >= dim_height()) return;
  if (vpend_get((z - vc_z0) * VCX + x - vc_x0)) return;
  int lx = x - vc_x0, ly = y - vc_y0, lz = z - vc_z0;
  vc_set(VC_I(lx, ly, lz), b);
  edits_put(x, y, z, b);
  /* the column's sky top, then the light around */
  int col = lz * VCX + lx, t = vtop[col];
  if (opacity(b) >= 15 && y + 1 > t) vtop[col] = (uint8_t)(y + 1);
  else if (y + 1 == t && opacity(b) < 15) {
    int ny = y;
    while (ny > vc_y0 && opacity(vc_get(VC_I(lx, ny - vc_y0 - 1, lz))) < 15) ny--;
    vtop[col] = (uint8_t)(ny > vc_y0 ? ny : 0);
  }
  light_mark(lx - 15, ly - 15, lz - 15, lx + 16, VCY, lz + 16);
  mac_box(lx, ly, lz, lx + 1, ly + 1, lz + 1);
}

/* Chunk.getPrecipitationHeight: the first y above the blocks rain lands on
 * (anything that stops movement, and liquids: not plants, torches, snow
 * layers or rails). Above the cache only the sky light's top is known. */
static bool stops_rain(int b) {
  switch (blk_model[b]) {
    case M_NONE: case M_CROSS: case M_PORTAL: case M_TORCH: case M_LAYER: case M_VINE: case M_FLAT: case M_LADDER: return false;
    default: return true;
  }
}
int world_rain_top(int x, int z) {
  int lx = x - vc_x0, lz = z - vc_z0;
  if ((unsigned)lx >= VCX || (unsigned)lz >= VCZ) return 255;
  int col = lz * VCX + lx, t = vtop[col];
  if (vpend_get(col)) return 255;
  if (t > vc_y0 + VCY) return t;
  for (int y = VCY - 1; y >= 0 && y + vc_y0 >= t; y--)
    if (stops_rain(vc_get(VC_I(lx, y, lz)))) return y + vc_y0 + 1;
  return t;
}

/* the chunk buffer, free while a frame is drawn (the rain uses it) */
void *world_scratch(uint32_t n) { return n <= sizeof slab32 ? (void *)slab32 : NULL; }
