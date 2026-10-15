/* Bakes the test world's structures: the desert pyramid, a dungeon and a desert well,
 * as the world generator makes them, into src/test_data.c. The test world then stamps
 * those bytes instead of running the generator, so the showcase is exactly the same
 * every time and cannot come out half-built: no seed, no random stream, no view and
 * no write rules at runtime, just fixed blocks at fixed places.
 *
 *   make bake-test   (writes src/test_data.c for the first seed below with all three)
 *
 * The bake takes a few minutes: the well is 1 in 1000 desert chunks. Everything is
 * asserted on the way out (the pyramid's nine TNT, its plate and its four chests; the
 * dungeon's spawner, its chest and a room closed on every side; the well's five water
 * blocks), so a bad bake fails here instead of shipping. src/test_data.c says which
 * seed it came from; if the structures' own code changes and the showcase should
 * follow, bake again - until then the showcase stays as it was baked, on purpose. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blocks.h"
#include "states.h"
#include "gen.h"

static uint8_t gs[GEN_SCRATCH];
#define BH_Y0 (-64)
#define BH_H 192
static uint8_t slab[BH_H * 256 / 8 * 9 + 8];

static void gen_full(int cx, int cz) {
  gen_scratch(gs);
  gen_slab(cx, cz, BH_Y0, BH_H, slab);
  gen_scratch(NULL);
}
static int at(int lx, int y, int lz) { return b9_get(slab, (y - BH_Y0) * 256 + lz * 16 + lx); }

static int fails;
#define CHECK(c) do { if (!(c)) { printf("BAKE FAIL %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* a captured box of the world: cells[(y * sz + z) * sx + x], world (x0 + x, y0 + y, z0 + z) */
typedef struct { int x0, y0, z0, sx, sy, sz; uint16_t *cells; } Box;
static uint16_t boxmem[30 * 40 * 30];
static Box take_box(int x0, int y0, int z0, int sx, int sy, int sz) {
  Box b = {x0, y0, z0, sx, sy, sz, boxmem};
  CHECK((unsigned)sx * (unsigned)sy * (unsigned)sz <= sizeof boxmem / 2);
  if (fails) return b;
  memset(boxmem, 0, (size_t)sx * sy * sz * 2);
  for (int cz = z0 >> 4; cz <= (z0 + sz - 1) >> 4; cz++)
    for (int cx = x0 >> 4; cx <= (x0 + sx - 1) >> 4; cx++) {
      gen_full(cx, cz);
      for (int lz = 0; lz < 16; lz++)
        for (int lx = 0; lx < 16; lx++) {
          int x = cx * 16 + lx, z = cz * 16 + lz;
          if (x < x0 || x >= x0 + sx || z < z0 || z >= z0 + sz) continue;
          for (int y = y0; y < y0 + sy; y++)
            b.cells[((y - y0) * sz + (z - z0)) * sx + (x - x0)] = (uint16_t)at(lx, y, lz);
        }
    }
  return b;
}
/* the showcase never moves on its own, so nothing in it may hang: sand, gravel and
 * concrete powder with nothing solid under them (the settle pass drops them, as the
 * first block update would) come out, and the structure keeps the shape it was made
 * with. Below the box is the void. */
static int falling(int b) {
  return b == B_SAND || b == B_RED_SAND || b == B_GRAVEL || (b >= B_CONCRETE_POWDER_WHITE &&
         b <= B_CONCRETE_POWDER_BLACK);
}
static void stabilize(Box *b) {
  extern const uint8_t blk_flags[NSTATES];
  for (int z = 0; z < b->sz; z++)
    for (int x = 0; x < b->sx; x++)
      for (int y = 0; y < b->sy; y++) {
        int i = (y * b->sz + z) * b->sx + x;
        if (!falling(b->cells[i])) continue;
        int below = y > 0 ? b->cells[((y - 1) * b->sz + z) * b->sx + x] : B_AIR;
        if (!(blk_flags[below] & 16)) {   /* BF_SOLID: anything else gives way */
          printf("bake: hanging %d at local %d %d %d comes out\n", b->cells[i], x, y, z);
          b->cells[i] = B_AIR;
        }
      }
}
static int cell_at(const Box *b, int x, int y, int z) {
  if (x < b->x0 || x >= b->x0 + b->sx || y < b->y0 || y >= b->y0 + b->sy || z < b->z0 || z >= b->z0 + b->sz) return -1;
  return b->cells[((y - b->y0) * b->sz + (z - b->z0)) * b->sx + (x - b->x0)];
}

/* ---------------------------------------------------------------- the pyramid */
/* A desert pyramid's own blocks: its sandstone, the terracotta pattern, its slab and
 * stairs, its TNT, its plate and its chests. (The sand and rock it stands on come
 * along in the box.) */
static int temple_block(int b) {
  return b == B_SANDSTONE || b == B_SANDSTONE_CHISELED || b == B_SANDSTONE_SMOOTH || b == B_SANDSTONE_SLAB ||
         b == B_SANDSTONE_STAIRS || b == B_SANDSTONE_STAIRS_W || b == B_SANDSTONE_STAIRS_S ||
         b == B_SANDSTONE_STAIRS_N || b == B_STAINED_CLAY_ORANGE || b == B_STAINED_CLAY_BLUE || b == B_TNT ||
         b == B_STONE_PRESSURE_PLATE || b == B_CHEST;
}

static int bake_temple(FILE *fh, FILE *fc, int fx, int fz) {
  /* the footprint, then the pyramid's own rows: the base sits near the desert's
   * surface, its top about fifteen rows over it, its chamber a dozen under */
  int llo = 200, hhi = 0, tnt = 0, plate = 0, chest = 0;
  for (int cz = (fz - 2) >> 4; cz <= (fz + 22) >> 4; cz++)
    for (int cx = (fx - 2) >> 4; cx <= (fx + 22) >> 4; cx++) {
      gen_full(cx, cz);
      for (int lz = 0; lz < 16; lz++)
        for (int lx = 0; lx < 16; lx++) {
          int x = cx * 16 + lx, z = cz * 16 + lz;
          if (x < fx - 2 || x > fx + 22 || z < fz - 2 || z > fz + 22) continue;
          for (int y = 30; y < 110; y++) {
            int b = at(lx, y, lz);
            if (!temple_block(b)) continue;
            if (y < llo) llo = y;
            if (y > hhi) hhi = y;
            if (b == B_TNT) tnt++;
            if (b == B_STONE_PRESSURE_PLATE) plate++;
            if (b == B_CHEST) chest++;
          }
        }
    }
  printf("temple at %d %d: rows %d..%d, TNT %d, plate %d, chests %d\n", fx, fz, llo, hhi, tnt, plate, chest);
  CHECK(tnt == 9 && plate == 1 && chest == 4);
  CHECK(llo < 200 && hhi - llo > 20 && hhi - llo < 40);
  if (fails) return 0;
  /* a neighbor's dungeon fused to it would come along in the box: another seed's */
  {
    int foreign = 0;
    for (int cz = (fz - 2) >> 4; cz <= (fz + 22) >> 4; cz++)
      for (int cx = (fx - 2) >> 4; cx <= (fx + 22) >> 4; cx++) {
        gen_full(cx, cz);
        for (int lz = 0; lz < 16 && !foreign; lz++)
          for (int lx = 0; lx < 16 && !foreign; lx++) {
            int x = cx * 16 + lx, z = cz * 16 + lz;
            if (x < fx - 2 || x > fx + 22 || z < fz - 2 || z > fz + 22) continue;
            for (int y = 30; y < 110; y++) {
              int b = at(lx, y, lz);
              if (b == B_COBBLESTONE || b == B_MOSSY_COBBLESTONE || b == B_MOB_SPAWNER) foreign = 1;
            }
          }
      }
    if (foreign) {
      printf("temple at %d %d: a neighbor's blocks in its box - another seed's\n", fx, fz);
      return 0;
    }
  }
  /* the box: the pyramid's own rows and three rows of the ground under them, two
   * blocks of the desert around the footprint */
  Box b = take_box(fx - 2, llo - 3, fz - 2, 25, hhi - (llo - 3) + 1, 25);
  if (fails) return 0;
  stabilize(&b);
  int gt = b.y0;
  for (int z = 0; z < b.sz; z++)
    for (int x = 0; x < b.sx; x++)
      for (int y = b.sy - 1; y >= 0; y--)
        if (b.cells[(y * b.sz + z) * b.sx + x]) {
          if (b.y0 + y + 1 > gt) gt = b.y0 + y + 1;
          break;
        }
  fprintf(fh, "/* the desert pyramid as the world generates it: stamped at TEST_TEMPLE_OX/OZ\n");
  fprintf(fh, " * (gen.h), same rows. Air is 0 and is not stamped. Box: %d x %d x %d. */\n", b.sx, b.sy, b.sz);
  fprintf(fh, "#define TEST_TEMPLE_SX %d\n#define TEST_TEMPLE_SY %d\n#define TEST_TEMPLE_SZ %d\n", b.sx, b.sy, b.sz);
  fprintf(fh, "#define TEST_TEMPLE_Y0 %d\n", b.y0);
  /* stamped lower, so the platform's window keeps the entrance and the base: the top
   * lands at row 60 (fly up a little and the whole of it is in view) */
  fprintf(fh, "#define TEST_TEMPLE_DY %d\n", 60 - (b.y0 + b.sy - 1));
  fprintf(fh, "#define TEST_TEMPLE_TOP %d\n", gt + (60 - (b.y0 + b.sy - 1)));
  fprintf(fh, "extern const uint16_t test_temple[TEST_TEMPLE_SX * TEST_TEMPLE_SY * TEST_TEMPLE_SZ];\n");
  fprintf(fc, "const uint16_t test_temple[TEST_TEMPLE_SX * TEST_TEMPLE_SY * TEST_TEMPLE_SZ] = {");
  for (int i = 0; i < b.sx * b.sy * b.sz; i++) fprintf(fc, "%s%d,", i % 12 == 0 ? "\n  " : "", b.cells[i]);
  fprintf(fc, "\n};\n");
  return 1;
}

/* ---------------------------------------------------------------- the dungeon */
static int dung_wall(int b) { return b == B_COBBLESTONE || b == B_MOSSY_COBBLESTONE; }

/* the room's air, flood-filled from the spawner through air alone: 1 where the room
 * is. The fill reaching the box means the room is open to a cave - not a closed
 * dungeon, take another. */
static uint8_t fill[17 * 13 * 17];
static int fill_room(const Box *b, int sx, int sy, int sz, int *x0, int *y0, int *z0, int *x1, int *y1, int *z1) {
  memset(fill, 0, sizeof fill);
  int stack[17 * 13 * 17], n = 0;
#define AT(x, y, z) (cell_at(b, x, y, z))
#define FF(x, y, z) (fill[((y - b->y0) * b->sz + (z - b->z0)) * b->sx + (x - b->x0)])
  /* from the spawner's air neighbours (the spawner's own cell is no air) */
  static const int8_t d[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  for (int k = 0; k < 6; k++) {
    int nx = sx + d[k][0], ny = sy + d[k][1], nz = sz + d[k][2];
    if (nx < b->x0 || nx >= b->x0 + b->sx || ny < b->y0 || ny >= b->y0 + b->sy || nz < b->z0 ||
        nz >= b->z0 + b->sz)
      return 0;
    stack[n++] = ((ny - b->y0) * b->sz + (nz - b->z0)) * b->sx + (nx - b->x0);
  }
  *x0 = *y0 = *z0 = 1 << 30, *x1 = *y1 = *z1 = -(1 << 30);
  while (n) {
    int i = stack[--n], lx = i % b->sx, t = i / b->sx, lz = t % b->sz, ly = t / b->sz;
    int x = b->x0 + lx, y = b->y0 + ly, z = b->z0 + lz;
    if (fill[i] || AT(x, y, z) != B_AIR) continue;
    fill[i] = 1;
    if (x == b->x0 || x == b->x0 + b->sx - 1 || y == b->y0 || y == b->y0 + b->sy - 1 || z == b->z0 ||
        z == b->z0 + b->sz - 1)
      return 0;   /* open to the box's edge */
    if (x < *x0) *x0 = x;
    if (x > *x1) *x1 = x;
    if (y < *y0) *y0 = y;
    if (y > *y1) *y1 = y;
    if (z < *z0) *z0 = z;
    if (z > *z1) *z1 = z;
    for (int k = 0; k < 6; k++) {
      int nx = x + d[k][0], ny = y + d[k][1], nz = z + d[k][2];
      if (nx < b->x0 || nx >= b->x0 + b->sx || ny < b->y0 || ny >= b->y0 + b->sy || nz < b->z0 ||
          nz >= b->z0 + b->sz)
        return 0;
      stack[n++] = ((ny - b->y0) * b->sz + (nz - b->z0)) * b->sx + (nx - b->x0);
    }
  }
  return n >= 0 && *x0 <= *x1;
#undef AT
#undef FF
}

static int bake_dungeon(FILE *fh, FILE *fc, int sx, int sy, int sz) {
  Box b = take_box(sx - 8, sy - 5, sz - 8, 17, 13, 17);
  if (fails) return 0;
  stabilize(&b);
  int spawner = 0, chest = 0, cobble = 0;
  for (int i = 0; i < b.sx * b.sy * b.sz; i++) {
    if (b.cells[i] == B_MOB_SPAWNER) spawner++;
    if (b.cells[i] == B_CHEST) chest++;
    if (dung_wall(b.cells[i])) cobble++;
  }
  int x0, y0, z0, x1, y1, z1;
  int closed = fill_room(&b, sx, sy, sz, &x0, &y0, &z0, &x1, &y1, &z1);
  printf("dungeon at %d %d %d: spawner %d, chests %d, walls %d, room %d..%d %d..%d %d..%d%s\n", sx, sy, sz, spawner,
         chest, cobble, x0, x1, y0, y1, z0, z1, closed ? "" : " OPEN");
  CHECK(spawner == 1 && chest >= 1 && cobble > 40 && closed);
  CHECK(x1 - x0 >= 4 && x1 - x0 <= 9 && z1 - z0 >= 4 && z1 - z0 <= 9 && y1 - y0 >= 2 && y1 - y0 <= 4);
  if (fails) return 0;
  /* the doorway in the room's north wall: the wall's own blocks, as the showcase
   * leaves them (air there would be a cave through the wall, and the room would not
   * be closed) */
  int door_y = y0, door_z = z0 - 1;
  CHECK(cell_at(&b, sx, door_y, door_z) == B_COBBLESTONE || cell_at(&b, sx, door_y, door_z) == B_MOSSY_COBBLESTONE);
  CHECK(cell_at(&b, sx, door_y + 1, door_z) == B_COBBLESTONE ||
        cell_at(&b, sx, door_y + 1, door_z) == B_MOSSY_COBBLESTONE);
  if (fails) return 0;
  /* the showcase's own touches, baked in: the ceiling cobbled (a real room's top is
   * the rock above it), one ceiling block a glowstone so the room can be seen, and
   * the doorway cut. The room itself - its floor, its walls, its spawner, its
   * chest - is exactly as generated. */
  int glow_y = y1 + 1;
  for (int z = z0 - 1; z <= z1 + 1; z++)
    for (int x = x0 - 1; x <= x1 + 1; x++) {
      int i = ((glow_y - b.y0) * b.sz + (z - b.z0)) * b.sx + (x - b.x0);
      if (x == sx && z == sz) b.cells[i] = B_GLOWSTONE;
      else b.cells[i] = B_COBBLESTONE;
    }
  b.cells[((door_y - b.y0) * b.sz + (door_z - b.z0)) * b.sx + (sx - b.x0)] = B_AIR;
  b.cells[((door_y + 1 - b.y0) * b.sz + (door_z - b.z0)) * b.sx + (sx - b.x0)] = B_AIR;
  /* the sky light's top over it: the rock's top row, plus one (the room stays dark) */
  int gt = b.y0;
  for (int z = 0; z < b.sz; z++)
    for (int x = 0; x < b.sx; x++)
      for (int y = b.sy - 1; y >= 0; y--)
        if (b.cells[(y * b.sz + z) * b.sx + x]) {
          if (b.y0 + y + 1 > gt) gt = b.y0 + y + 1;
          break;
        }
  fprintf(fh, "/* the dungeon as the world generates it: stamped at TEST_DUNG_X/Z (gen.h), same\n");
  fprintf(fh, " * rows. Air is 0 and is not stamped. Box: %d x %d x %d. */\n", b.sx, b.sy, b.sz);
  fprintf(fh, "#define TEST_DUNG_SX %d\n#define TEST_DUNG_SY %d\n#define TEST_DUNG_SZ %d\n", b.sx, b.sy, b.sz);
  fprintf(fh, "#define TEST_DUNG_Y0 %d\n", b.y0);
  /* stamped with the room's floor at row 44, on the showcase's own row */
  fprintf(fh, "#define TEST_DUNG_DY %d\n", 44 - y0);
  fprintf(fh, "#define TEST_DUNG_TOP %d\n", gt + (44 - y0));
  fprintf(fh, "/* the room's middle (its spawner), its doorway and its ceiling light, local */\n");
  fprintf(fh, "#define TEST_DUNG_SPAWN_X %d\n#define TEST_DUNG_SPAWN_Y %d\n#define TEST_DUNG_SPAWN_Z %d\n",
          sx - b.x0, sy - b.y0, sz - b.z0);
  fprintf(fh, "#define TEST_DUNG_DOOR_X %d\n#define TEST_DUNG_DOOR_Y %d\n#define TEST_DUNG_DOOR_Z %d\n", sx - b.x0,
          door_y - b.y0, door_z - b.z0);
  fprintf(fh, "#define TEST_DUNG_GLOW_X %d\n#define TEST_DUNG_GLOW_Y %d\n#define TEST_DUNG_GLOW_Z %d\n", sx - b.x0,
          glow_y - b.y0, sz - b.z0);
  fprintf(fh, "extern const uint16_t test_dungeon[TEST_DUNG_SX * TEST_DUNG_SY * TEST_DUNG_SZ];\n");
  fprintf(fc, "const uint16_t test_dungeon[TEST_DUNG_SX * TEST_DUNG_SY * TEST_DUNG_SZ] = {");
  for (int i = 0; i < b.sx * b.sy * b.sz; i++) fprintf(fc, "%s%d,", i % 12 == 0 ? "\n  " : "", b.cells[i]);
  fprintf(fc, "\n};\n");
  return 1;
}

/* ---------------------------------------------------------------- the well */
static int bake_well(FILE *fh, FILE *fc, int cx, int cy, int cz) {
  /* the roof's middle (the sandstone over the water) is the middle: a 3 x 3 of slabs
   * with sandstone in its middle, all on one row */
  /* the ground it stands on with it: the water sits at the ground, rows under the roof */
  Box b = take_box(cx - 4, cy - 6, cz - 4, 9, 12, 9);
  if (fails) return 0;
  stabilize(&b);
  int water = 0, slabs = 0;
  for (int i = 0; i < b.sx * b.sy * b.sz; i++) {
    if (b.cells[i] == B_WATER || b.cells[i] == B_FLOWING_WATER) water++;
    if (b.cells[i] == B_SANDSTONE_SLAB) slabs++;
  }
  printf("well at %d %d %d: water %d, slabs %d\n", cx, cy, cz, water, slabs);
  CHECK(water == 5 && slabs == 12);
  if (fails) return 0;
  int gt = b.y0;
  for (int z = 0; z < b.sz; z++)
    for (int x = 0; x < b.sx; x++)
      for (int y = b.sy - 1; y >= 0; y--)
        if (b.cells[(y * b.sz + z) * b.sx + x]) {
          if (b.y0 + y + 1 > gt) gt = b.y0 + y + 1;
          break;
        }
  fprintf(fh, "/* the desert well as the world generates it: stamped at TEST_WELL_X/Z (gen.h),\n");
  fprintf(fh, " * same rows. Air is 0 and is not stamped. Box: %d x %d x %d. */\n", b.sx, b.sy, b.sz);
  fprintf(fh, "#define TEST_WELL_SX %d\n#define TEST_WELL_SY %d\n#define TEST_WELL_SZ %d\n", b.sx, b.sy, b.sz);
  fprintf(fh, "#define TEST_WELL_Y0 %d\n", b.y0);
  /* stamped with the box's bottom row at 43, on the showcase's own row */
  fprintf(fh, "#define TEST_WELL_DY %d\n", 43 - b.y0);
  fprintf(fh, "#define TEST_WELL_TOP %d\n", gt + (43 - b.y0));
  fprintf(fh, "/* the roof's middle, local */\n");
  fprintf(fh, "#define TEST_WELL_MID_X %d\n#define TEST_WELL_MID_Y %d\n#define TEST_WELL_MID_Z %d\n", cx - b.x0,
          cy - b.y0, cz - b.z0);
  fprintf(fh, "extern const uint16_t test_well[TEST_WELL_SX * TEST_WELL_SY * TEST_WELL_SZ];\n");
  fprintf(fc, "const uint16_t test_well[TEST_WELL_SX * TEST_WELL_SY * TEST_WELL_SZ] = {");
  for (int i = 0; i < b.sx * b.sy * b.sz; i++) fprintf(fc, "%s%d,", i % 12 == 0 ? "\n  " : "", b.cells[i]);
  fprintf(fc, "\n};\n");
  return 1;
}

/* ---------------------------------------------------------------- the hunt */
/* A well's roof: eight sandstone slabs around a sandstone middle, all on one row. */
static int well_roof(int cx, int wx, int y, int wz) {
  (void)cx;
  int lx = wx & 15, lz = wz & 15;
  for (int dz = -1; dz <= 1; dz++)
    for (int dx = -1; dx <= 1; dx++) {
      int b = at(lx + dx, y, lz + dz);
      if (dx == 0 && dz == 0) {
        if (b != B_SANDSTONE) return 0;
      } else if (b != B_SANDSTONE_SLAB)
        return 0;
    }
  return 1;
}

static int desert_id;

static const int64_t temple_seeds[] = {20260614, 11, 777, 31337, 123, 100, 2024, 999, 616, 1337, 424242,
                                       8675309, 2000, 7007, 31415, 555, 271828, 314159};

int main(void) {
  desert_id = gen_biome_id("desert");
  printf("bake: desert id %d\n", desert_id);
  CHECK(desert_id == 2);
  /* the pyramid: the first seed whose temple stands alone (no neighbor's blocks in
   * its box - a dungeon fused to seed 123's is why it is not simply seed 123) */
  int64_t temple_seed = 0;
  int fx = 0, fz = 0;
  for (unsigned s = 0; s < sizeof temple_seeds / sizeof temple_seeds[0] && !temple_seed; s++) {
    gen_init(temple_seeds[s]);
    gen_set_dim(0);
    gen_set_flat(0);
    gen_set_test(0);
    gen_scratch(gs);
    int has_temple = gen_find_temple(0, 0, &fx, &fz);
    gen_scratch(NULL);
    if (!has_temple) {
      printf("bake: seed %lld has no temple near the origin\n", (long long)temple_seeds[s]);
      continue;
    }
    printf("bake: seed %lld temple corner %d %d\n", (long long)temple_seeds[s], fx, fz);
    int foreign = 0;
    for (int cz = (fz - 2) >> 4; cz <= (fz + 22) >> 4 && !foreign; cz++)
      for (int cx = (fx - 2) >> 4; cx <= (fx + 22) >> 4 && !foreign; cx++) {
        gen_full(cx, cz);
        for (int lz = 0; lz < 16 && !foreign; lz++)
          for (int lx = 0; lx < 16 && !foreign; lx++) {
            int x = cx * 16 + lx, z = cz * 16 + lz;
            if (x < fx - 2 || x > fx + 22 || z < fz - 2 || z > fz + 22) continue;
            for (int y = 30; y < 110; y++) {
              int b = at(lx, y, lz);
              if (b == B_COBBLESTONE || b == B_MOSSY_COBBLESTONE || b == B_MOB_SPAWNER) foreign = 1;
            }
          }
      }
    if (foreign) {
      printf("bake: seed %lld temple has a neighbor in its box\n", (long long)temple_seeds[s]);
      continue;
    }
    temple_seed = temple_seeds[s];
  }
  if (!temple_seed) {
    printf("bake: no seed has a clean temple - giving up\n");
    return 1;
  }
  /* the dungeon and the well: seed 123 has both (closed rooms and a roof pattern) */
  gen_init(123);
  gen_set_dim(0);
  gen_set_flat(0);
  gen_set_test(0);
  int cand[40][3], ncand = 0;
  for (int r = 0; r <= 12 && ncand < 40; r++)
    for (int dz = -r; dz <= r && ncand < 40; dz++)
      for (int dx = -r; dx <= r && ncand < 40; dx++) {
        if (r && dx > -r && dx < r && dz > -r && dz < r) continue;
        gen_full(dx, dz);
        for (int lz = 0; lz < 16 && ncand < 40; lz++)
          for (int lx = 0; lx < 16 && ncand < 40; lx++)
            for (int y = 5; y < 70 && ncand < 40; y++)
              if (at(lx, y, lz) == B_MOB_SPAWNER)
                cand[ncand][0] = dx * 16 + lx, cand[ncand][1] = y, cand[ncand][2] = dz * 16 + lz, ncand++;
      }
  if (!ncand) {
    printf("bake: seed 123 has no dungeon near the origin\n");
    return 1;
  }
  for (int i = 0; i < ncand; i++)
    for (int j = i + 1; j < ncand; j++)
      if (abs(cand[j][1] - 45) < abs(cand[i][1] - 45)) {
        int t[3];
        memcpy(t, cand[i], sizeof t), memcpy(cand[i], cand[j], sizeof t), memcpy(cand[j], t, sizeof t);
      }
  printf("bake: %d spawner candidates (shallowest %d %d %d)\n", ncand, cand[0][0], cand[0][1], cand[0][2]);
  /* the well stands where the last bake found its roof: captured directly */
  int wx = 3525, wy = 71, wz = -610;
  {
    gen_full(wx >> 4, wz >> 4);
    if (!well_roof(wx >> 4, wx, wy, wz)) {
      printf("bake: seed 123 well gone from %d %d %d\n", wx, wy, wz);
      return 1;
    }
  }
  printf("bake: well %d %d %d\n", wx, wy, wz);
  /* the files, through temporary names: a failed bake leaves no half bake behind */
  FILE *fh = fopen("src/test_data.h.new", "w");
  FILE *fc = fopen("src/test_data.c.new", "w");
  if (!fh || !fc) {
    printf("BAKE FAIL: cannot write src/test_data\n");
    return 1;
  }
  fprintf(fh, "/* Generated by `make bake-test` (tests/bake.c): the test world's structures\n");
  fprintf(fh, " * as the world generator makes them - the desert pyramid (seed %lld), a dungeon\n", (long long)temple_seed);
  fprintf(fh, " * and a desert well (both seed 123). The test world stamps these bytes instead of\n");
  fprintf(fh, " * running the generator, so the showcase is exactly the same every time. DO NOT\n");
  fprintf(fh, " * EDIT: bake again (the seeds are above). The pyramid's footprint starts at\n");
  fprintf(fh, " * (TEST_TEMPLE_OX * 16, TEST_TEMPLE_OZ * 16), the dungeon's room is middle at\n");
  fprintf(fh, " * (TEST_DUNG_X, TEST_DUNG_Z), the well's roof middle at (TEST_WELL_X, TEST_WELL_Z):\n");
  fprintf(fh, " * gen.h says where those are. Until the structures' own code changes and the\n");
  fprintf(fh, " * showcase should follow, this file stays as it was baked, on purpose. */\n");
  fprintf(fh, "#ifndef NB_TEST_DATA_H\n#define NB_TEST_DATA_H\n");
  fprintf(fh, "#include <stdint.h>\n");
  fprintf(fc, "/* See test_data.h. */\n#include \"test_data.h\"\n");
  fails = 0;
  gen_init(temple_seed);
  gen_set_dim(0);
  gen_set_flat(0);
  gen_set_test(0);
  int ok = bake_temple(fh, fc, fx, fz);
  gen_init(123);
  gen_set_dim(0);
  gen_set_flat(0);
  gen_set_test(0);
  /* the first closed dungeon of the candidates, in the file's order (each tried on
   * temporary streams: a failed candidate leaves no writes behind) */
  for (int i = 0; ok && i < ncand; i++) {
    FILE *th = tmpfile(), *tc = tmpfile();
    if (!th || !tc) {
      printf("BAKE FAIL: no temporary files\n");
      return 1;
    }
    fails = 0;
    if (bake_dungeon(th, tc, cand[i][0], cand[i][1], cand[i][2])) {
      printf("bake: dungeon %d %d %d taken\n", cand[i][0], cand[i][1], cand[i][2]);
      FILE *from[2] = {th, tc}, *to[2] = {fh, fc};
      for (int k = 0; k < 2; k++) {
        int c;
        rewind(from[k]);
        while ((c = fgetc(from[k])) != EOF) fputc(c, to[k]);
      }
      fclose(th);
      fclose(tc);
      break;
    }
    fclose(th);
    fclose(tc);
    if (i == ncand - 1) ok = 0;
  }
  ok = ok && bake_well(fh, fc, wx, wy, wz);
  fprintf(fh, "#endif\n");
  fclose(fh);
  fclose(fc);
  if (!ok || fails) {
    printf("bake: failed its checks\n");
    remove("src/test_data.h.new");
    remove("src/test_data.c.new");
    return 1;
  }
  rename("src/test_data.h.new", "src/test_data.h");
  rename("src/test_data.c.new", "src/test_data.c");
  printf("bake: wrote src/test_data.h and src/test_data.c\n");
  return 0;
}
