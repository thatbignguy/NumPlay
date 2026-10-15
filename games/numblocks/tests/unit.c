/* Checks of the item rules: crafting, stacking, smelting, drops. */
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include "nb.h"
#include "mob.h"
#include "edits.h"
#include "gen.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int craft(const int *ids, int w) {
  Stack g[9] = {{0}};
  for (int i = 0; i < w * w; i++) g[i].id = (uint16_t)ids[i], g[i].aux = ids[i] ? 1 : 0;
  int r = craft_find(g, w);
  return r < 0 ? 0 : recipes[r].out * 100 + recipes[r].n;
}

int main(void) {
  /* The pressed-plate and nether states are appended; existing saved block ids,
   * especially the sandstone stairs, must not move. */
  CHECK(B_SANDSTONE_STAIRS == 443 && B_SANDSTONE_STAIRS_W == 444 && B_SANDSTONE_STAIRS_S == 445 &&
        B_SANDSTONE_STAIRS_N == 446 && I_PIGLIN_SPAWN_EGG == 447 && I_ZOMBIFIED_PIGLIN_SPAWN_EGG == 448 &&
        B_STONE_PRESSURE_PLATE_DOWN == 449);
  /* planks from a log, anywhere in the grid */
  int a[4] = {0, B_LOG_BIRCH, 0, 0};
  CHECK(craft(a, 2) == B_PLANKS_BIRCH * 100 + 4);
  /* sticks from any planks */
  int b[4] = {B_PLANKS_JUNGLE, 0, B_PLANKS_OAK, 0};
  CHECK(craft(b, 2) == I_STICK * 100 + 4);
  /* crafting table */
  int c[4] = {B_PLANKS_OAK, B_PLANKS_SPRUCE, B_PLANKS_OAK, B_PLANKS_OAK};
  CHECK(craft(c, 2) == B_CRAFTING_TABLE * 100 + 1);
  /* a pickaxe needs the 3 x 3 grid */
  int p[9] = {B_COBBLESTONE, B_COBBLESTONE, B_COBBLESTONE, 0, I_STICK, 0, 0, I_STICK, 0};
  CHECK(craft(p, 3) == I_STONE_PICKAXE * 100 + 1);
  /* an axe, mirrored */
  int x1[9] = {I_IRON_INGOT, I_IRON_INGOT, 0, I_IRON_INGOT, I_STICK, 0, 0, I_STICK, 0};
  int x2[9] = {0, I_IRON_INGOT, I_IRON_INGOT, 0, I_STICK, I_IRON_INGOT, 0, I_STICK, 0};
  CHECK(craft(x1, 3) == I_IRON_AXE * 100 + 1);
  CHECK(craft(x2, 3) == I_IRON_AXE * 100 + 1);
  /* torches from coal or charcoal */
  int t1[4] = {I_COAL, 0, I_STICK, 0}, t2[4] = {0, I_CHARCOAL, 0, I_STICK};
  CHECK(craft(t1, 2) == B_TORCH * 100 + 4);
  CHECK(craft(t2, 2) == B_TORCH * 100 + 4);
  /* shapeless: stew, in any order */
  int s[9] = {I_BOWL, 0, 0, 0, B_RED_MUSHROOM, 0, 0, 0, B_BROWN_MUSHROOM};
  CHECK(craft(s, 3) == I_MUSHROOM_STEW * 100 + 1);
  /* the quartz family's cut blocks: a pillar from two blocks in a column, bricks
   * and chiseled from 2 x 2, smooth from the furnace */
  int qp[4] = {B_QUARTZ_BLOCK, 0, B_QUARTZ_BLOCK, 0};
  CHECK(craft(qp, 2) == B_QUARTZ_PILLAR * 100 + 2);
  int qb[9] = {B_QUARTZ_BLOCK, B_QUARTZ_BLOCK, 0, B_QUARTZ_BLOCK, B_QUARTZ_BLOCK, 0, 0, 0, 0};
  CHECK(craft(qb, 3) == B_QUARTZ_BRICKS * 100 + 4);
  int qc[4] = {B_QUARTZ_PILLAR, B_QUARTZ_PILLAR, B_QUARTZ_PILLAR, B_QUARTZ_PILLAR};
  CHECK(craft(qc, 2) == B_CHISELED_QUARTZ_BLOCK * 100 + 1);
  /* concrete powder: four sand and four gravel with a dye, shapeless, eight at a time */
  int cp[9] = {B_SAND, B_GRAVEL, B_SAND, B_GRAVEL, I_DYE_BLACK, B_SAND, B_GRAVEL, B_SAND, B_GRAVEL};
  CHECK(craft(cp, 3) == B_CONCRETE_POWDER_BLACK * 100 + 8);
  int cw[9] = {B_GRAVEL, I_DYE_WHITE, B_SAND, B_GRAVEL, B_SAND, B_SAND, B_GRAVEL, B_SAND, B_GRAVEL};
  CHECK(craft(cw, 3) == B_CONCRETE_POWDER_WHITE * 100 + 8);
  /* nothing */
  int n[4] = {B_DIRT, 0, 0, 0};
  CHECK(craft(n, 2) == 0);
  /* a furnace and a chest */
  int f[9] = {B_COBBLESTONE, B_COBBLESTONE, B_COBBLESTONE, B_COBBLESTONE, 0, B_COBBLESTONE, B_COBBLESTONE,
              B_COBBLESTONE, B_COBBLESTONE};
  CHECK(craft(f, 3) == B_FURNACE * 100 + 1);
  int bed[9] = {B_WOOL_RED, B_WOOL_WHITE, B_WOOL_RED, B_PLANKS_OAK, B_PLANKS_BIRCH, B_PLANKS_OAK, 0, 0, 0};
  CHECK(craft(bed, 3) == I_BED * 100 + 1);
  /* stacking: 64 at most, the same items first */
  Stack inv[36] = {{0}};
  CHECK(inv_add(inv, 36, B_DIRT, 100, 0) == 0);
  CHECK(inv[0].id == B_DIRT && inv[0].aux == 64 && inv[1].aux == 36);
  CHECK(inv_add(inv, 36, I_IRON_PICKAXE, 1, 5) == 0 && inv[2].id == I_IRON_PICKAXE && inv[2].aux == 5);
  CHECK(inv_add(inv, 36, B_DIRT, 30, 0) == 0 && inv[1].aux == 64 && inv[3].aux == 2);
  CHECK(inv_add(inv, 36, I_EGG, 20, 0) == 0 && inv[4].aux == 16 && inv[5].aux == 4);
  /* smelting */
  CHECK(smelt_of(I_RAW_IRON) == I_IRON_INGOT);
  CHECK(smelt_of(I_RAW_GOLD) == I_GOLD_INGOT);
  CHECK(smelt_of(B_IRON_ORE) == 0);
  CHECK(smelt_of(B_GOLD_ORE) == 0);
  CHECK(smelt_of(B_LOG_ACACIA) == I_CHARCOAL);
  CHECK(smelt_of(B_QUARTZ_BLOCK) == B_SMOOTH_QUARTZ);
  CHECK(smelt_of(B_DIRT) == 0);
  CHECK(item_fuel(I_COAL) == 1600 && item_fuel(B_PLANKS_OAK) == 300 && item_fuel(I_STICK) == 100);
  /* harvesting */
  CHECK(!can_harvest(B_STONE, 0) && can_harvest(B_STONE, I_WOODEN_PICKAXE));
  CHECK(!can_harvest(B_DIAMOND_ORE, I_STONE_PICKAXE) && can_harvest(B_DIAMOND_ORE, I_IRON_PICKAXE));
  CHECK(can_harvest(B_DIRT, 0) && dig_speed(B_DIRT, I_DIAMOND_SHOVEL) == 8 && dig_speed(B_DIRT, I_DIAMOND_PICKAXE) == 1);
  Stack out[2];
  CHECK(block_drops(B_STONE, I_WOODEN_PICKAXE, out) == 1 && out[0].id == B_COBBLESTONE);
  CHECK(block_drops(B_STONE, 0, out) == 0);
  CHECK(block_drops(B_GRASS, 0, out) == 1 && out[0].id == B_DIRT);
  CHECK(block_drops(B_LOG_OAK_X, 0, out) == 1 && out[0].id == B_LOG_OAK);
  CHECK(block_drops(B_COAL_ORE, I_GOLDEN_PICKAXE, out) == 1 && out[0].id == I_COAL);
  CHECK(block_drops(B_IRON_ORE, I_STONE_PICKAXE, out) == 1 && out[0].id == I_RAW_IRON);
  CHECK(block_drops(B_GOLD_ORE, I_IRON_PICKAXE, out) == 1 && out[0].id == I_RAW_GOLD);
  CHECK(block_drops(B_TORCH_E, 0, out) == 1 && out[0].id == B_TORCH);
  /* concrete and its powder: the powder mines like sand and gravel (anything drops it), the
   * concrete like stone (a pickaxe; bare hands break it and get nothing) */
  CHECK(can_harvest(B_CONCRETE_POWDER_RED, 0) && dig_speed(B_CONCRETE_POWDER_RED, I_DIAMOND_SHOVEL) == 8);
  CHECK(block_drops(B_CONCRETE_POWDER_RED, 0, out) == 1 && out[0].id == B_CONCRETE_POWDER_RED);
  CHECK(can_harvest(B_CONCRETE_LIGHT_BLUE, I_WOODEN_PICKAXE) && !can_harvest(B_CONCRETE_LIGHT_BLUE, 0));
  CHECK(block_drops(B_CONCRETE_LIGHT_BLUE, I_WOODEN_PICKAXE, out) == 1 && out[0].id == B_CONCRETE_LIGHT_BLUE);
  CHECK(block_drops(B_CONCRETE_LIGHT_BLUE, 0, out) == 0);
  /* changes survive: through the journal, into storage, back after a reload */
  {
    extern void host_save_dir(const char *d);
    host_save_dir("build/unit-saves");
    remove("build/unit-saves/ut1r0_0.nbe");
    remove("build/unit-saves/ut1r-1_0.nbe");
    world_new(0, "ut1");
    world_follow(8, 70, 8);
    int base = world_get(5, 62, 5);
    world_set(5, 62, 5, B_GOLD_BLOCK);
    world_set(-3, 63, 7, B_GLASS);
    for (int i = 0; i < 300; i++) world_set(i % 20 - 5, 61 + i / 20, 12, B_DIAMOND_BLOCK);   /* more than the journal */
    CHECK(world_get(5, 62, 5) == B_GOLD_BLOCK);
    CHECK(edits_flush());
    world_new(0, "ut1");
    world_follow(8, 70, 8);
    CHECK(world_get(5, 62, 5) == B_GOLD_BLOCK);
    CHECK(world_get(-3, 63, 7) == B_GLASS);
    CHECK(world_get(14, 75, 12) == B_DIAMOND_BLOCK && world_get(-5, 61, 12) == B_DIAMOND_BLOCK);
    world_set(5, 62, 5, base);
    CHECK(edits_flush());
    world_new(0, "ut1");
    world_follow(8, 70, 8);
    CHECK(world_get(5, 62, 5) == base);
  }
  /* water: 7 blocks out on a flat floor, then dry when the source is gone; lava meets water */
  {
    world_new(0, "ut2");
    world_follow(8, 70, 8);
    int y = 70;
    for (int z = -12; z <= 12; z++)
      for (int x = -12; x <= 12; x++) {
        world_set(x + 8, y - 1, z + 8, B_STONE);
        for (int k = 0; k < 3; k++) world_set(x + 8, y + k, z + 8, B_AIR);
      }
    world_set(8, y, 8, B_WATER);
    neighbours_changed(8, y, 8);
    for (int t = 0; t < 400; t++) ticks_run++, world_tick();
    CHECK(world_get(9, y, 8) == B_FLOWING_WATER);
    CHECK(world_get(15, y, 8) == B_FLOWING_WATER_7);
    CHECK(world_get(16, y, 8) == B_AIR);
    CHECK(world_get(11, y, 10) == B_FLOWING_WATER_5);
    world_set(8, y, 8, B_AIR);
    neighbours_changed(8, y, 8);
    for (int t = 0; t < 600; t++) ticks_run++, world_tick();
    CHECK(world_get(9, y, 8) == B_AIR && world_get(12, y, 8) == B_AIR);
    /* lava source next to water: obsidian */
    world_set(4, y, 4, B_WATER);
    world_set(5, y, 4, B_LAVA);
    neighbours_changed(5, y, 4);
    for (int t = 0; t < 60; t++) ticks_run++, world_tick();
    CHECK(world_get(5, y, 4) == B_OBSIDIAN);
    plat_remove_prefix("ut2");
  }
  {
    /* liquids at the cache's edge: a stream whose source leaves the loaded cache stands
     * instead of draining - an unloaded neighbour reads unknown, never air - and is
     * unchanged when the source is back. A source removed beside a loaded stream still
     * drains it (the edge does not freeze live flow). Java has no such edge; the game's
     * cache does, so the rule is the game's own (tick.c). */
    world_new(0, "utdrain");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    int y = 70;
    for (int z = 0; z <= 8; z++)
      for (int x = 16; x <= 28; x++) {
        world_set(x, y - 1, z, B_STONE);
        for (int k = 0; k < 3; k++) world_set(x, y + k, z, B_AIR);
      }
    for (int z = 20; z <= 28; z++)
      for (int x = 16; x <= 28; x++) {
        world_set(x, y - 1, z, B_STONE);
        for (int k = 0; k < 3; k++) world_set(x, y + k, z, B_AIR);
      }
    /* water spreads seven out; lava three (decay 2): one source each, far apart */
    world_set(22, y, 4, B_WATER);
    neighbours_changed(22, y, 4);
    world_set(22, y, 24, B_LAVA);
    neighbours_changed(22, y, 24);
    for (int t = 0; t < 900; t++) ticks_run++, world_tick();
    CHECK(world_get(23, y, 4) == B_FLOWING_WATER);
    CHECK(world_get(24, y, 4) == B_FLOWING_WATER_2);
    CHECK(is_lava(world_get(24, y, 24)) && world_get(24, y, 24) != B_LAVA);
    int wl = world_get(24, y, 4), ll = world_get(24, y, 24);
    /* the cache 16 right: the sources sit two blocks past its edge, the flows at it.
     * Ticks run while the move is still queued, as they do in play. */
    world_follow(40, 70, 8);
    for (int t = 0; t < 60; t++) ticks_run++, world_tick(), world_follow(40, 70, 8);
    while (world_pending()) world_follow(40, 70, 8);
    CHECK(!world_loaded(22, y, 4) && world_loaded(24, y, 4));
    CHECK(!world_loaded(22, y, 24) && world_loaded(24, y, 24));
    /* force their updates (the settle pass would only wake what can move): with an
     * unloaded neighbour they stand instead of re-levelling from an unseen side */
    fluid_schedule(24, y, 4);
    fluid_schedule(24, y, 24);
    for (int t = 0; t < 400; t++) ticks_run++, world_tick();
    CHECK(world_get(24, y, 4) == wl && wl != B_AIR);
    CHECK(world_get(24, y, 24) == ll && ll != B_AIR);
    /* and back: the sources come back with the records, the streams unchanged */
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    CHECK(world_get(22, y, 4) == B_WATER && world_get(22, y, 24) == B_LAVA);
    CHECK(world_get(24, y, 4) == wl && world_get(24, y, 24) == ll);
    /* a source taken away beside a loaded stream still drains it */
    world_set(22, y, 4, B_AIR);
    neighbours_changed(22, y, 4);
    for (int t = 0; t < 600; t++) ticks_run++, world_tick();
    CHECK(world_get(24, y, 4) == B_AIR);
    plat_remove_prefix("utdrain");
  }
  {
    /* the same standing fall: a source high with its fall left behind as the cache moves
     * down does not drain - the fall's cells stay liquid while the source is out of it */
    world_new(0, "utdrain2");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    for (int z = 6; z <= 10; z++)
      for (int x = 6; x <= 10; x++)
        for (int kk = 60; kk <= 80; kk++) world_set(x, kk, z, kk == 60 ? B_STONE : B_AIR);
    for (int z = 6; z <= 10; z++)
      for (int x = 18; x <= 22; x++)
        for (int kk = 60; kk <= 80; kk++) world_set(x, kk, z, kk == 60 ? B_STONE : B_AIR);
    world_set(8, 73, 8, B_WATER);
    neighbours_changed(8, 73, 8);
    world_set(20, 73, 8, B_LAVA);
    neighbours_changed(20, 73, 8);
    for (int t = 0; t < 900; t++) ticks_run++, world_tick();
    CHECK(world_get(8, 72, 8) == B_FALLING_WATER && world_get(8, 65, 8) == B_FALLING_WATER);
    CHECK(is_lava(world_get(20, 72, 8)) && world_get(20, 72, 8) == B_FALLING_LAVA);
    /* the cache down to y0 48: the sources sit two blocks past its top, the falls in it.
     * Ticks run while the move's chunks are still queued (a jump down makes several) -
     * a neighbour still to be made reads as stone, equally unknown, so the falls wait
     * through the queue too instead of re-levelling against it and leaving holes. */
    world_follow(8, 60, 8);
    for (int t = 0; t < 60; t++) ticks_run++, world_tick(), world_follow(8, 60, 8);
    while (world_pending()) world_follow(8, 60, 8);
    CHECK(!world_loaded(8, 73, 8) && world_loaded(8, 71, 8));
    CHECK(!world_loaded(20, 73, 8) && world_loaded(20, 71, 8));
    for (int t = 0; t < 400; t++) ticks_run++, world_tick();
    CHECK(world_get(8, 71, 8) == B_FALLING_WATER && world_get(8, 65, 8) == B_FALLING_WATER);
    CHECK(world_get(20, 71, 8) == B_FALLING_LAVA);
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    CHECK(world_get(8, 73, 8) == B_WATER && world_get(20, 73, 8) == B_LAVA);
    plat_remove_prefix("utdrain2");
  }
  {
    /* generation's own blocks get their first update: the generator places blocks, it does
     * not tick them, so a chunk it made holds sand and gravel hanging over the air a cave
     * carved out from under them. The settle pass (world.c) runs when a move of the cache is
     * whole - its chunks are the ones just entering the render distance - and drops them. */
    world_new(0, "uts");
    world_follow(-300, 70, -270);   /* (a beach over a cave pocket: red sand hanging at 63) */
    while (world_pending()) world_follow(-300, 70, -270);
    CHECK(world_get(-286, 63, -250) == B_AIR);        /* (was red sand, hanging in the air) */
    CHECK(world_get(-286, 62, -250) == B_RED_SAND);   /* (it dropped onto the ground under it) */
    plat_remove_prefix("uts");
  }
  {
    /* the same for what a save brings back: a record puts a block where it was left - no
     * block update has ticked it - so a sand with nothing under it drops and a water source
     * with an open cell below it is woken, the frame the chunk is made again */
    world_new(12345, "uts2");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    int bx = 8, bz = 8, by = 72;
    /* a stone floor with a clear room over it: a sand block hanging over the room, and a
     * water source beside it with air under it (both written as the player's changes are,
     * without the neighbours_changed a placement would run) */
    for (int z = bz - 1; z <= bz + 1; z++)
      for (int x = bx - 1; x <= bx + 4; x++)
        for (int y = by - 1; y <= by + 4; y++) world_set(x, y, z, y == by - 1 ? B_STONE : B_AIR);
    world_set(bx, by + 4, bz, B_SAND);
    world_set(bx + 2, by + 4, bz, B_WATER);
    CHECK(world_get(bx, by + 4, bz) == B_SAND && world_get(bx + 2, by + 4, bz) == B_WATER);
    /* the cache away and back: the records put them back, and the settle pass with them */
    world_follow(bx + 400, 70, bz + 400);
    while (world_pending()) world_follow(bx + 400, 70, bz + 400);
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    CHECK(world_get(bx, by + 4, bz) == B_AIR && world_get(bx, by, bz) == B_SAND);
    CHECK(world_get(bx + 2, by + 4, bz) == B_WATER);   /* (a source stays a source) */
    /* and the water flows on its own from there: down to the floor, and out along it */
    for (int t = 0; t < 400; t++) ticks_run++, world_tick();
    CHECK(world_get(bx + 2, by + 3, bz) == B_FALLING_WATER);   /* (the stream under the source) */
    CHECK(world_get(bx + 2, by, bz) == B_FALLING_WATER);       /* (fallen to the floor) */
    CHECK(world_get(bx - 1, by, bz) == B_FLOWING_WATER_4);     /* (and out along it, four steps) */
    plat_remove_prefix("uts2");
  }
  {
    /* a falling block placed with nothing under it drops the moment it is placed: the
     * placement asks neighbours_changed of the cell it placed into, and that cell is now
     * checked for what is under it too, not only its neighbours - before, sand set into
     * the air hung there until something beside it changed */
    world_new(12345, "utf");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    int bx = 8, bz = 8, by = 72;
    for (int z = bz - 1; z <= bz + 1; z++)
      for (int x = bx - 1; x <= bx + 3; x++)
        for (int y = by - 1; y <= by + 4; y++) world_set(x, y, z, y == by - 1 ? B_STONE : B_AIR);
    /* set_block, as a placement runs it (world_set, then the cell's own update) */
    world_set(bx, by + 3, bz, B_SAND);
    neighbours_changed(bx, by + 3, bz);
    CHECK(world_get(bx, by + 3, bz) == B_AIR && world_get(bx, by, bz) == B_SAND);
    world_set(bx + 1, by + 3, bz, B_GRAVEL);
    neighbours_changed(bx + 1, by + 3, bz);
    CHECK(world_get(bx + 1, by + 3, bz) == B_AIR && world_get(bx + 1, by, bz) == B_GRAVEL);
    /* a dry powder too; beside water it still sets where it is placed */
    world_set(bx + 2, by + 3, bz, B_CONCRETE_POWDER_RED);
    neighbours_changed(bx + 2, by + 3, bz);
    CHECK(world_get(bx + 2, by + 3, bz) == B_AIR && world_get(bx + 2, by, bz) == B_CONCRETE_POWDER_RED);
    world_set(bx + 3, by + 3, bz, B_WATER);
    world_set(bx + 2, by + 3, bz, B_CONCRETE_POWDER_RED);
    neighbours_changed(bx + 2, by + 3, bz);
    CHECK(world_get(bx + 2, by + 3, bz) == B_CONCRETE_RED);
    /* the command path runs the same update: a powder set into the air drops (as the
     * command needs cheats, they are lent here and given back) */
    bool was_cheats = world_cheats;
    world_cheats = true;
    pl.x = (float)(bx - 1) + 0.5f, pl.y = (float)by, pl.z = (float)bz + 0.5f;
    command_run("/setblock ~ ~3 ~ black_concrete_powder");
    CHECK(world_get(bx - 1, by + 3, bz) == B_AIR && world_get(bx - 1, by, bz) == B_CONCRETE_POWDER_BLACK);
    world_cheats = was_cheats;
    plat_remove_prefix("utf");
  }
  /* two cows fed wheat make a calf (EntityAIMate) */
  {
    extern uint32_t game_time;
    world_new(0, "ut3");
    world_follow(8, 70, 8);
    int y = 70;
    for (int z = 0; z <= 16; z++)
      for (int x = 0; x <= 16; x++) {
        world_set(x, y - 1, z, B_STONE);
        for (int k = 0; k < 3; k++) world_set(x, y + k, z, B_AIR);
      }
    memset(ents, 0, sizeof ents);
    opt.difficulty = 0;   /* (no monsters) */
    Entity *a = ent_new(E_COW, 6.5f, (float)y, 8.5f), *b = ent_new(E_COW, 9.5f, (float)y, 8.5f);
    a->health = b->health = 10;
    pl.inv[0].id = I_WHEAT, pl.inv[0].aux = 5, pl.slot = 0, pl.mode = 0;
    CHECK(mob_use(a) && mob_use(b) && pl.inv[0].aux == 3);
    int calves = 0;
    for (int t = 0; t < 400; t++) ents_tick(), game_time++;
    for (int i = 0; i < N_ENT; i++)
      if (ents[i].type == E_COW && ents[i].growth < 0) calves++;
    CHECK(calves == 1);
    plat_remove_prefix("ut3");
  }
  /* a zombie after the player climbs a one-block step (the jump's first rise is the whole 0.42) */
  {
    extern uint32_t game_time;
    world_new(0, "ut3z");
    world_follow(8, 70, 8);
    int y = 70;
    for (int z = 0; z <= 16; z++)
      for (int x = 0; x <= 16; x++) {
        world_set(x, y - 1, z, B_STONE);
        world_set(x, y, z, x >= 10 ? B_STONE : B_AIR);
        for (int k = 1; k < 4; k++) world_set(x, y + k, z, B_AIR);
      }
    memset(ents, 0, sizeof ents);
    opt.difficulty = 2, pl.mode = 0, pl.dead = false, pl.health = 20;
    pl.x = 12.5f, pl.y = (float)y + 1, pl.z = 8.5f;
    game_time = 18000;   /* (midnight: no burning) */
    Entity *zb = ent_new(E_ZOMBIE, 5.5f, (float)y, 8.5f);
    zb->health = 20, zb->panic = 60;
    for (int t = 0; t < 100; t++) ticks_run++, ents_tick(), game_time++;
    CHECK(zb->type == E_ZOMBIE && zb->y >= y + 1 && zb->x > 10);
    plat_remove_prefix("ut3z");
  }
  /* a skeleton stopped to shoot turns all the way round to face the player: EntityAIArrowAttack
   * looks at its target, so the 60-degree limiter that keeps a walking mob on its path must not
   * leave it firing over its shoulder once the player steps behind it */
  {
    extern uint32_t game_time;
    world_new(0, "ut3s");
    world_follow(8, 70, 8);
    int y = 70;
    for (int z = 0; z <= 16; z++)
      for (int x = 0; x <= 16; x++) {
        world_set(x, y - 1, z, B_STONE);
        for (int k = 0; k < 4; k++) world_set(x, y + k, z, B_AIR);
      }
    memset(ents, 0, sizeof ents);
    opt.difficulty = 2, pl.mode = 0, pl.dead = false, pl.health = 20;
    pl.x = 8.5f, pl.y = (float)y, pl.z = 8.5f;
    game_time = 18000;   /* (midnight: no burning) */
    Entity *sk = ent_new(E_SKELETON, 8.5f, (float)y, 15.5f);   /* seven ahead */
    sk->health = 20, sk->yaw = 0, sk->panic = 60;             /* (facing +z: away from the player) */
    CHECK(fabsf(sk->yaw) < 1);
    for (int t = 0; t < 30; t++) ticks_run++, ents_tick(), game_time++;
    /* (the shortest way round lands on -180, which faces the player just as +180 does) */
    CHECK(sk->type == E_SKELETON && fabsf(fabsf(sk->yaw) - 180) < 1);   /* (it turned to face the player) */
    plat_remove_prefix("ut3s");
  }
  {
    /* a mob near the player looks at them for a short while, then looks away and idles: the
     * head is not held on the player (EntityAIWatchClosest's maxRandomLookTime), and its pitch
     * comes level again. The hall has walls and no floor outside, so the mobs stay in it. */
    world_new(0, "ut13");
    world_follow(8, 70, 8);
    int y = 70;
    for (int z = 3; z <= 15; z++)
      for (int x = 3; x <= 15; x++) {
        for (int k = 0; k < 6; k++) world_set(x, y + k, z, B_AIR);
        if (x > 3 && x < 15 && z > 3 && z < 15) world_set(x, y - 1, z, B_STONE);
        else world_set(x, y - 1, z, B_STONE), world_set(x, y, z, B_STONE), world_set(x, y + 1, z, B_STONE);
      }
    memset(ents, 0, sizeof ents);
    memset(&pl, 0, sizeof pl);
    opt.difficulty = 2;
    game_rules &= ~(uint8_t)GR_MOB_SPAWNING;   /* (the hall is a dark box: without this the world's own
                                                 * monsters come out of the dark about it, and a creeper's
                                                 * blast takes the floor from under the cow) */
    pl.x = 8.5f, pl.y = (float)y, pl.z = 6.5f, pl.health = 20, pl.dead = false, pl.mode = 0, pl.slot = 0;
    /* a cow with its food in hand, standing two blocks away (EntityAITempt brings it that close) */
    pl.inv[0] = (Stack){I_WHEAT, 2};
    Entity *cow = ent_new(E_COW, 8.5f, (float)y, 8.5f);
    cow->health = 10, cow->tx = 8, cow->tz = 8;
    int run = 0, longest = 0, level = 0;
    for (int t = 0; t < 1000; t++) {
      pl.health = 20, pl.dead = false;
      ents_tick();
      if (cow->look > 0 && cow->look < 80) run++;
      else {
        if (run > longest) longest = run;
        run = 0;
      }
      if (fabsf(cow->pitch) < 1) level++;
    }
    CHECK(cow->type == E_COW && longest <= 80 && longest >= 40);   /* a watch is 40 to 80 ticks */
    CHECK(level > 200);                    /* its head is level for most of the time */
    /* in creative nothing a monster does involves the player: it only looks about */
    extern uint32_t game_time;
    memset(ents, 0, sizeof ents);
    memset(&pl, 0, sizeof pl);
    pl.x = 8.5f, pl.y = (float)y, pl.z = 6.5f, pl.health = 20, pl.dead = false, pl.mode = 1;
    game_time = 18000;                     /* (midnight: no burning) */
    Entity *zb = ent_new(E_ZOMBIE, 8.5f, (float)y, 8.5f);
    zb->health = 20, zb->tx = 8, zb->tz = 8;
    int watched = 0, idle = 0;
    for (int t = 0; t < 600; t++) {
      pl.health = 20, pl.dead = false;
      ents_tick();
      if (zb->look >= 80) idle++;
      else if (zb->look > 0) watched++;
    }
    CHECK(zb->type == E_ZOMBIE && !watched && idle > 20);
    game_rules = GR_DEFAULT;
    plat_remove_prefix("ut13");
  }
  {
    /* weather: the clock, the strength, the light, and where rain falls */
    extern uint32_t game_time;
    world_new(0, "ut4");
    world_follow(8, 110, 8);   /* (high above the ground: only the test's blocks under the sky) */
    for (int i = 0; i < VCX * VCZ; i++) vbiome[i] = 1;   /* plains */
    memset(&weather, 0, sizeof weather);
    rain_str = thunder_str = 0;
    game_time = 6000;   /* noon */
    CHECK(sky_sub() == 0);
    world_tick();
    CHECK(!weather.raining && weather.rain_time >= 12000 - 1 && weather.rain_time < 180000);
    weather.rain_time = 1;
    world_tick();
    CHECK(weather.raining && rain_str > 0 && rain_str < 0.05f);
    for (int t = 0; t < 120; t++) world_tick();
    CHECK(rain_str == 1 && sky_sub() == 3);   /* rain alone: still day, no sleeping */
    weather.thundering = 1, thunder_str = 1;
    CHECK(sky_sub() >= 4);                    /* a storm: dark enough to sleep */
    int y = 110;
    for (int z = 0; z <= 4; z++)
      for (int x = 0; x <= 4; x++) {
        world_set(x, y - 1, z, B_STONE);
        for (int k = 0; k < 6; k++) world_set(x, y + k, z, B_AIR);
      }
    CHECK(rain_at(2, y, 2));
    world_set(2, y + 3, 2, B_LEAVES_OAK);      /* leaves keep it off */
    CHECK(!rain_at(2, y, 2) && world_rain_top(2, 2) == y + 4);
    world_set(3, y, 3, B_TALL_GRASS);          /* plants do not */
    CHECK(world_rain_top(3, 3) == y);
    vbiome[(2 - vc_z0) * VCX + 1 - vc_x0] = 2;  /* deserts get none */
    CHECK(!rain_at(1, y, 2));
    vbiome[(2 - vc_z0) * VCX + 1 - vc_x0] = 12; /* snowy biomes get snow */
    CHECK(!rain_at(1, y, 2));
    /* a bolt on the player: 5 damage and fire, and the sky flashes */
    pl.x = 2.5f, pl.y = (float)y, pl.z = 2.5f, pl.mode = 0, pl.dead = false, pl.health = 20, pl.invuln = 0, pl.fire = 0;
    memset(pl.armor, 0, sizeof pl.armor);
    bolt = (Bolt){2.5f, (float)y, 2.5f, 2, 0, 1, 7};
    world_tick();
    CHECK(pl.health == 15 && pl.fire > 0 && last_bolt == 2 && bolt.on);
    for (int t = 0; t < 3; t++) world_tick();
    CHECK(!bolt.on);
    weather_clear();
    world_tick();
    CHECK(!weather.raining && !weather.thundering);
    plat_remove_prefix("ut4");
  }
  {
    /* fishing: cast into a pond, wait for a bite, reel it in */
    world_new(0, "ut5");
    world_follow(8, 70, 8);
    memset(&weather, 0, sizeof weather);
    rain_str = thunder_str = 0;
    int y = 70;
    for (int z = 0; z <= 16; z++)
      for (int x = 0; x <= 16; x++)
        for (int k = -3; k < 5; k++) world_set(x, y + k, z, k >= 0 ? B_AIR : z >= 6 && k >= -2 ? B_WATER : B_STONE);
    memset(ents, 0, sizeof ents);
    memset(pl.inv, 0, sizeof pl.inv);
    pl.x = 8.5f, pl.y = (float)y, pl.z = 2.5f, pl.yaw = 0, pl.pitch = 30, pl.dead = false, pl.mode = 0;
    pl.inv[0].id = I_FISHING_ROD, pl.slot = 0;
    fish_cast();
    Entity *b = bobber();
    CHECK(b != NULL);
    int t = 0;
    while (t < 3000 && (b = bobber()) && b->timer <= 0) ents_tick(), t++;
    CHECK(b && b->timer > 0 && is_water(world_get((int)b->x, (int)b->y, (int)b->z)));
    CHECK(fish_reel(b) == 1 && !bobber());
    for (int k = 0; k < 60; k++) ents_tick();
    int caught = 0;
    for (int i = 1; i < 36; i++) caught += pl.inv[i].id != 0;
    CHECK(caught == 1);
    plat_remove_prefix("ut5");
  }
  {
    /* the picture, straight up and straight down at every turn: no ray may get stuck
     * (an empty-region jump once handed a ray back and forth forever); the alarm ends a hang */
    world_new(0, "ut6");
    world_follow(8, 70, 8);
    alarm(60);
    for (int p = -90; p <= 90; p += 180)
      for (float yaw = -180; yaw < 180; yaw += 7.5f) {
        Camera c = {8.5f, 71.62f, 8.5f, yaw, (float)p};
        render_frame(&c, 6000);
      }
    /* and from the void under the world, at every turn: the rays going up come in
     * through the bottom of the cache (the ones going down find only the void) */
    for (int p = -90; p <= 90; p += 90)
      for (float yaw = -180; yaw < 180; yaw += 7.5f) {
        Camera c = {8.5f, -6.38f, 8.5f, yaw, (float)p};
        render_frame(&c, 6000);
      }
    alarm(0);
    plat_remove_prefix("ut6");
  }
  {
    /* out of a pool one block deep: swim into the bank holding jump (the 0.3 lift) */
    world_new(0, "ut7");
    world_follow(8, 110, 8);
    int g = 110;
    for (int z = 2; z <= 14; z++)
      for (int x = 2; x <= 14; x++)
        for (int y = g - 3; y < g + 4; y++)
          world_set(x, y, z, y >= g ? B_AIR : (x >= 7 && x <= 9 && z >= 7 && z <= 9 && y == g - 1) ? B_WATER : B_DIRT);
    memset(&pl, 0, sizeof pl);
    pl.x = 8.5f, pl.z = 8.5f, pl.y = (float)(g - 1), pl.health = 20, pl.food = 20, pl.mode = 0;
    for (int t = 0; t < 10; t++) player_tick(0, 0);
    CHECK(pl.in_water);
    int t = 0;
    for (; t < 60 && !(pl.on_ground && pl.y >= g); t++) player_tick(K_FWD | K_JUMP, t == 0 ? K_JUMP : 0);
    CHECK(pl.on_ground && pl.y >= g && pl.z > 10);
    plat_remove_prefix("ut7");
  }
  {
    /* the void under the world: nothing under its bedrock, and 4 damage a tick from 64
     * blocks below the world (Entity.checkBelowWorld), which kills survival and creative
     * alike; /kill goes on killing a spectator, which the void does not touch. */
    world_new(0, "utvoid");
    int y = 12;
    world_follow(8, y, 8);   /* (low, so the cache holds the whole shaft down to the bedrock) */
    for (int z = 2; z <= 14; z++)
      for (int x = 2; x <= 14; x++) {
        world_set(x, y, z, B_AIR);   /* the room the player is in */
        for (int k = 1; k <= 8; k++) world_set(x, y - k, z, B_STONE);
        world_set(x, 0, z, B_BEDROCK);   /* the floor of the world */
      }
    CHECK(world_get(8, 0, 8) == B_BEDROCK && world_get(8, -1, 8) == B_AIR && world_get(8, -64, 8) == B_AIR);
    for (int k = 0; k < y; k++) world_set(8, k, 8, B_AIR);   /* a shaft through the bedrock */
    memset(ents, 0, sizeof ents);
    opt.difficulty = 2;
    /* a tick in the void, 4 of it, and none of it a tick higher up */
    memset(&pl, 0, sizeof pl);
    pl.x = 8.5f, pl.z = 8.5f, pl.y = (float)(VOID_Y + 1), pl.health = 20, pl.food = 20, pl.mode = 0;
    player_tick(0, 0);
    CHECK(pl.health == 20 && pl.y > (float)VOID_Y);
    pl.y = (float)VOID_Y - 1, pl.health = 20, pl.invuln = 0, pl.last_damage = 0;
    player_tick(0, 0);
    CHECK(pl.health == 16 && !pl.dead);
    /* falling out of the world kills, in survival and in creative: nothing catches you there */
    for (int m = 0; m <= 1; m++) {
      memset(&pl, 0, sizeof pl);
      pl.x = 8.5f, pl.z = 8.5f, pl.y = (float)y, pl.health = 20, pl.food = 20, pl.mode = (uint8_t)m;
      int t = 0;
      for (; t < 400 && !pl.dead; t++) player_tick(0, 0);
      CHECK(t < 400 && pl.dead && pl.health == 0 && pl.y < (float)VOID_Y);
    }
    /* the spectator flies down into the void and nothing there hurts it; /kill still does */
    memset(&pl, 0, sizeof pl);
    pl.x = 8.5f, pl.z = 8.5f, pl.y = (float)(VOID_Y - 4), pl.health = 20, pl.mode = 2, pl.flying = true;
    for (int t = 0; t < 10; t++) player_tick(0, 0);
    CHECK(pl.health == 20 && !pl.dead && pl.y < (float)VOID_Y);
    player_hurt(4, DMG_VOID);
    CHECK(pl.health == 20 && !pl.dead);
    player_hurt(3.4e38f, DMG_KILL);
    CHECK(pl.dead);
    /* the picture from under the world, up and down at every turn: no ray may get stuck */
    alarm(60);
    for (int p = -90; p <= 90; p += 90)
      for (float yaw = -180; yaw < 180; yaw += 45.0f) {
        Camera c = {8.5f, -3.38f, 8.5f, yaw, (float)p};
        render_frame(&c, 6000);
      }
    alarm(0);
    plat_remove_prefix("utvoid");
  }
  {
    /* sprinting (EntityPlayerSP): with the sprint key; it lasts while forward is held */
    world_new(0, "ut8");
    world_follow(8, 110, 8);
    int g = 110;
    for (int z = -8; z <= 60; z++)
      for (int x = 2; x <= 14; x++)
        for (int y = g - 2; y < g + 4; y++) world_set(x, y, z, y >= g ? B_AIR : B_STONE);
    memset(&pl, 0, sizeof pl);
    pl.x = 8.5f, pl.z = 0.5f, pl.y = (float)g, pl.health = 20, pl.food = 20, pl.mode = 0, pl.on_ground = true;
    for (int t = 0; t < 3; t++) player_tick(0, 0);
    player_tick(K_FWD, K_FWD), player_tick(0, 0), player_tick(0, 0);
    CHECK(!pl.sprinting);
    player_tick(K_FWD, K_FWD);   /* forward twice: only walking */
    CHECK(!pl.sprinting);
    for (int t = 0; t < 5; t++) player_tick(K_FWD, 0);
    CHECK(!pl.sprinting);
    /* the key: held while it is held, and letting go of it ends the sprint (Hold) */
    player_tick(K_FWD | K_SPRINT, K_SPRINT);
    CHECK(pl.sprinting);
    player_tick(K_FWD, 0);
    CHECK(!pl.sprinting);            /* the key let go: it ends */
    player_tick(K_FWD, 0);
    CHECK(!pl.sprinting);            /* (forward alone is only walking) */
    player_tick(K_FWD | K_SNEAK, K_SNEAK);
    CHECK(!pl.sprinting);            /* sneaking ends it */
    pl.food = 6;
    player_tick(K_FWD | K_SPRINT, K_SPRINT);
    CHECK(!pl.sprinting);
    /* and it is quicker than walking: the same forward key, the same ticks, the flat field
     * (vanilla walks 4.317 blocks a second and sprints 5.612; over 30 ticks here, 6.5 and 8.3) */
    pl.food = 20;   /* (fed again: a hungry player cannot sprint) */
    float z0 = pl.z;
    for (int t = 0; t < 30; t++) player_tick(K_FWD | K_SPRINT, t == 0 ? K_SPRINT : 0);
    CHECK(pl.sprinting);
    float ran = pl.z - z0;
    z0 = pl.z;
    pl.sprinting = false;
    for (int t = 0; t < 30; t++) player_tick(K_FWD, 0);
    CHECK(!pl.sprinting);
    CHECK(ran > (pl.z - z0) * 1.2f && ran < (pl.z - z0) * 1.4f);   /* (1.3 times as quick) */
    /* Sprint: Toggle (Options, Controls): a tap of the key switches it on and off, so the key
     * need not be held while walking; letting go of forward ends it either way */
    opt.sprint_toggle = 1;
    pl.sprinting = false;
    player_tick(K_FWD | K_SPRINT, K_SPRINT);   /* a tap: on */
    CHECK(pl.sprinting);
    player_tick(K_FWD, 0);
    CHECK(pl.sprinting);                       /* the key let go: it holds */
    player_tick(K_FWD | K_SPRINT, K_SPRINT);
    CHECK(!pl.sprinting);                       /* another tap: off */
    player_tick(K_FWD | K_SPRINT, K_SPRINT);
    CHECK(pl.sprinting);                       /* a tap again: on */
    for (int t = 0; t < 5; t++) player_tick(0, 0);
    CHECK(!pl.sprinting);                       /* forward let go: it ends, as it does on Hold */
    player_tick(K_FWD, K_FWD), player_tick(0, 0), player_tick(K_FWD, K_FWD);   /* forward twice */
    CHECK(!pl.sprinting);                      /* (forward twice no longer sprints) */
    player_tick(K_FWD | K_SNEAK, K_SNEAK);
    CHECK(!pl.sprinting);                       /* sneaking ends it */
    pl.food = 6;
    player_tick(K_SPRINT, K_SPRINT);
    CHECK(!pl.sprinting);                       /* hungry: it cannot start */
    opt.sprint_toggle = 0;
    plat_remove_prefix("ut8");
  }
  {
    /* Sprint: Toggle is kept with the options; a record an older version wrote (ending
     * where the Sprint option does not live yet) reads as Hold */
    remove("build/unit-saves/numblocks.cfg");
    opt.sprint_toggle = 1;
    save_options();
    opt.sprint_toggle = 0;
    load_options();
    CHECK(opt.sprint_toggle == 1);
    /* the record as the older version wrote it: its last byte is not there */
    remove("build/unit-saves/numblocks.cfg");
    plat_save("numblocks.cfg", &opt, offsetof(Options, sprint_toggle));
    opt.sprint_toggle = 0;
    load_options();
    CHECK(opt.sprint_toggle == 0);   /* (Hold, as the options fresh from boot have it) */
    opt.sprint_toggle = 0;
    remove("build/unit-saves/numblocks.cfg");
  }
  {
    /* Fullbright is kept with the options the same way, in the byte the Options struct
     * had as padding (41: the record does not grow), and a record from before it reads
     * as off */
    CHECK(offsetof(Options, fullbright) == 41 && sizeof opt == 44);
    remove("build/unit-saves/numblocks.cfg");
    opt.fullbright = 1;
    save_options();
    opt.fullbright = 0;
    load_options();
    CHECK(opt.fullbright == 1);
    remove("build/unit-saves/numblocks.cfg");
    plat_save("numblocks.cfg", &opt, offsetof(Options, fullbright));   /* (the record older versions wrote) */
    opt.fullbright = 0;
    load_options();
    CHECK(opt.fullbright == 0);   /* (off, as the options fresh from boot have it) */
    remove("build/unit-saves/numblocks.cfg");
  }
  {
    /* Personalisation's two picks are kept the same way, in the last of the bytes the
     * struct had as padding (42 and 43): the record stays 44, a record from before
     * them reads as off/Random, and a value a record cannot hold is put back on the
     * default */
    CHECK(offsetof(Options, contrast) == 42 && offsetof(Options, panorama) == 43 && sizeof opt == 44);
    remove("build/unit-saves/numblocks.cfg");
    opt.contrast = 1, opt.panorama = PAN_NETHER;
    save_options();
    opt.contrast = 0, opt.panorama = PAN_RANDOM;
    load_options();
    CHECK(opt.contrast == 1 && opt.panorama == PAN_NETHER);
    remove("build/unit-saves/numblocks.cfg");
    plat_save("numblocks.cfg", &opt, offsetof(Options, contrast));   /* (the record older versions wrote) */
    opt.contrast = 0, opt.panorama = PAN_RANDOM;
    load_options();
    CHECK(opt.contrast == 0 && opt.panorama == PAN_RANDOM);   /* (off and Random, as fresh options have them) */
    opt.panorama = 200;
    save_options();
    opt.panorama = PAN_OVER;
    load_options();
    CHECK(opt.panorama == PAN_RANDOM);   /* (past the last pick: back on the default) */
    remove("build/unit-saves/numblocks.cfg");
  }
  {
    /* the cache moving up a step at a time (new rows above the terrain are not generated, only
     * aired) holds the same blocks and light as one made there at once. The two can come to
     * rest in different bands (the climb flies, arriving at once does not yet know to), so
     * what they hold of the world is compared where the two windows overlap */
    static uint8_t a[VC_BYTES], l[VCY * VCZ * VCX];   /* the cache is packed: 9 bits a state */
    int ay = 0;
    for (int k = 0; k < 2; k++) {
      world_new(77, "ut9");
      if (k == 0)
        for (int y = 64; y < 100; y += 3) {
          world_follow(8, (float)y, 8);
          while (world_pending()) world_follow(8, (float)y, 8);
        }
      world_follow(8, 100, 8);
      while (world_pending()) world_follow(8, 100, 8);
      if (k == 0) memcpy(a, vc, sizeof a), memcpy(l, vl, sizeof l), ay = vc_y0;
    }
    int lo = ay > vc_y0 ? ay : vc_y0, hi = VCY + (ay < vc_y0 ? ay : vc_y0), bad = 0;
    for (int wy = lo; wy < hi; wy++)
      for (int z = 0; z < VCZ; z++)
        for (int x = 0; x < VCX; x++) {
          int i = z * VCX + x;
          bad += b9_get(a, (wy - ay) * VCZ * VCX + i) != vc_get((wy - vc_y0) * VCZ * VCX + i);
          bad += l[(wy - ay) * VCZ * VCX + i] != vl[(wy - vc_y0) * VCZ * VCX + i];
        }
    CHECK(hi > lo && bad == 0);
    /* and coming down from high up (the new rows in the air made as air): as the generator makes them */
    world_new(77, "ut9");
    for (int y = 160; y >= 92; y -= 3)
      for (int r = 0; r < 3; r++) {
        world_follow(8, (float)y, 8);
        while (world_pending()) world_follow(8, (float)y, 8);
      }
    static uint8_t g[16 * 16 * VCY / 8 * 9 + 4];   /* a chunk slab: 9 bits a state */
    uint8_t gs[GEN_SCRATCH];
    gen_scratch(gs);
    int off = 0;
    for (int cz = -2; cz <= 1; cz++)
      for (int cx = -2; cx <= 1; cx++) {
        gen_slab(cx, cz, vc_y0, VCY, g);
        for (int y = 0; y < VCY; y++)
          for (int z = 0; z < 16; z++)
            for (int x = 0; x < 16; x++) {
              int lx = cx * 16 + x - vc_x0, lz = cz * 16 + z - vc_z0;
              if (lx >= 0 && lx < VCX && lz >= 0 && lz < VCZ)
                off += vc_get(VC_I(lx, y, lz)) != b9_get(g, (y * 16 + z) * 16 + x);
            }
      }
    CHECK(off == 0);
    plat_remove_prefix("ut9");
  }
  {
    /* worlds in slots: names kept, made unique, renamed; one deleted leaves the others */
    for (int s = 1; s <= MAX_WORLDS; s++) delete_world(s);
    char n[WORLD_NAME + 1];
    WorldInfo w;
    world_unique_name("New World", n);
    CHECK(!strcmp(n, "New World"));
    new_world(world_free_slot(), 5, 0, false, WT_DEFAULT, n, NULL);
    save_world();
    world_unique_name("New World", n);
    CHECK(!strcmp(n, "New World (2)"));
    int s2 = world_free_slot();
    new_world(s2, 6, 1, true, WT_DEFAULT, n, NULL);
    save_world();
    CHECK(s2 == 2 && world_info(1, &w) && !strcmp(w.name, "New World") && w.mode == 0 && w.seed == 5);
    CHECK(world_info(2, &w) && !strcmp(w.name, "New World (2)") && w.mode == 1 && w.seed == 6);
    CHECK(rename_world(1, "Base") && world_info(1, &w) && !strcmp(w.name, "Base"));
    world_unique_name("New World", n);
    CHECK(!strcmp(n, "New World"));
    delete_world(1);
    CHECK(!world_info(1, &w) && world_info(2, &w) && world_free_slot() == 1);
    CHECK(load_world(2) && pl.mode == 1 && world_seed == 6);
    for (int s = 1; s <= MAX_WORLDS; s++) delete_world(s);
  }
  {
    /* the block looked at: a ray over a slab or beside a flower goes on to what is behind */
    extern float hit_t;
    world_new(5, "ut10");
    world_follow(8, 100, 8);
    while (world_pending()) world_follow(8, 100, 8);
    for (int z = 4; z < 16; z++)
      for (int x = 4; x < 12; x++)
        for (int y = 99; y < 104; y++) world_set(x, y, z, y == 99 ? B_STONE : B_AIR);
    world_set(8, 100, 12, B_STONE);
    world_set(8, 100, 10, B_STONE_SLAB);
    pl.mode = 0;
    player_look(8.5f, 100.75f, 8.5f, 0, 0);
    CHECK(pl.hit_face == 2 && pl.hit_x == 8 && pl.hit_y == 100 && pl.hit_z == 12);
    player_look(8.5f, 100.25f, 8.5f, 0, 0);
    CHECK(pl.hit_face == 2 && pl.hit_z == 10 && fabsf(hit_t - 1.5f) < 0.01f);
    player_look(8.5f, 102, 10.5f, 0, 90);
    CHECK(pl.hit_face == 1 && pl.hit_y == 100 && pl.hit_z == 10 && fabsf(hit_t - 1.5f) < 0.01f);
    world_set(8, 100, 10, B_POPPY);
    player_look(8.9f, 100.25f, 8.5f, 0, 0);
    CHECK(pl.hit_z == 12);
    player_look(8.5f, 100.25f, 8.5f, 0, 0);
    CHECK(pl.hit_face == 2 && pl.hit_z == 10);
    /* a mob in front of the block takes the crosshair (no outline); past 3 blocks nothing is in reach */
    world_set(8, 100, 10, B_AIR), world_set(8, 100, 12, B_AIR), world_set(8, 100, 13, B_STONE);
    memset(ents, 0, sizeof ents);
    player_look(8.5f, 100.75f, 8.6f, 0, 0);
    CHECK(pl.hit_face == 2 && pl.hit_z == 13);
    Entity *cow = ent_new(E_COW, 8.5f, 100, 10.5f);
    player_look(8.5f, 100.75f, 8.6f, 0, 0);
    CHECK(pl.hit_face < 0 && entity_looked_at(3, 1e9f) == cow);
    cow->z = 12.3f;
    player_look(8.5f, 100.75f, 8.6f, 0, 0);
    CHECK(pl.hit_face < 0 && !entity_looked_at(3, 1e9f));
    plat_remove_prefix("ut10");
  }
  /* the names are the modern ones: 1.13's ids renamed the blocks and items (and 1.12's the
   * terracotta, 1.14's the dyes), and the inventory and the /give chat show these */
  CHECK(!strcmp(item_label(B_PLANKS_OAK), "Oak Planks"));
  CHECK(!strcmp(item_label(B_LOG_OAK), "Oak Log"));
  CHECK(!strcmp(item_label(B_LEAVES_OAK), "Oak Leaves"));
  CHECK(!strcmp(item_label(B_WOOD_SLAB_OAK), "Oak Slab"));
  CHECK(!strcmp(item_label(B_SAPLING_OAK), "Oak Sapling"));
  CHECK(!strcmp(item_label(B_HARDENED_CLAY), "Terracotta"));
  CHECK(!strcmp(item_label(B_STAINED_CLAY_WHITE), "White Terracotta"));
  CHECK(!strcmp(item_label(B_STAINED_CLAY_BLACK), "Black Terracotta"));
  CHECK(!strcmp(item_label(B_STAINED_CLAY_LIGHT_BLUE), "Light Blue Terracotta"));
  CHECK(!strcmp(item_label(B_MOSSY_COBBLESTONE), "Mossy Cobblestone"));
  CHECK(!strcmp(item_label(B_SNOW), "Snow Block"));
  CHECK(!strcmp(item_label(B_SNOW_LAYER), "Snow"));
  CHECK(!strcmp(item_label(B_STONE_SLAB), "Smooth Stone Slab"));
  CHECK(!strcmp(item_label(B_STONE_SLAB_DOUBLE), "Smooth Stone"));
  CHECK(!strcmp(item_label(B_WOOD_SLAB_OLD), "Petrified Oak Slab"));
  CHECK(!strcmp(item_label(B_BRICK_SLAB), "Brick Slab"));
  CHECK(!strcmp(item_label(B_STONE_BRICK_SLAB), "Stone Brick Slab"));
  CHECK(!strcmp(item_label(B_DOUBLE_GRASS_LOWER), "Tall Grass"));
  CHECK(!strcmp(item_label(B_DEAD_SHRUB), "Dead Bush"));
  CHECK(!strcmp(item_label(B_BROWN_MUSHROOM), "Brown Mushroom"));
  CHECK(!strcmp(item_label(B_RED_MUSHROOM), "Red Mushroom"));
  CHECK(!strcmp(item_label(B_BROWN_MUSHROOM_CAP), "Brown Mushroom Block"));
  CHECK(!strcmp(item_label(B_OAK_STAIRS), "Oak Stairs"));
  CHECK(!strcmp(item_label(B_GRASS), "Grass Block"));
  CHECK(!strcmp(item_label(I_REEDS), "Sugar Cane"));
  CHECK(!strcmp(item_label(I_MELON_SLICE), "Melon Slice"));
  CHECK(!strcmp(item_label(I_CLAY_BALL), "Clay Ball"));
  CHECK(!strcmp(item_label(I_WHEAT_SEEDS), "Wheat Seeds"));
  CHECK(!strcmp(item_label(I_MILK_BUCKET), "Milk Bucket"));
  CHECK(!strcmp(item_label(I_BED), "White Bed"));
  CHECK(!strcmp(item_label(I_DYE_RED), "Red Dye"));
  CHECK(!strcmp(item_label(I_DYE_GREEN), "Green Dye"));
  CHECK(!strcmp(item_label(I_DYE_YELLOW), "Yellow Dye"));
  CHECK(!strcmp(item_label(I_DYE_WHITE), "Bone Meal"));
  CHECK(!strcmp(item_label(I_RAW_FISH), "Raw Cod"));
  CHECK(!strcmp(item_label(I_COOKED_FISH), "Cooked Cod"));
  CHECK(!strcmp(item_label(I_CLOWNFISH), "Tropical Fish"));
  CHECK(!strcmp(item_label(I_RAW_SALMON), "Raw Salmon"));
  CHECK(!strcmp(item_label(B_CONCRETE_RED), "Red Concrete"));
  CHECK(!strcmp(item_label(B_CONCRETE_SILVER), "Light Gray Concrete"));
  CHECK(!strcmp(item_label(B_CONCRETE_POWDER_LIGHT_BLUE), "Light Blue Concrete Powder"));
  {
    /* commands: what they do; most need cheats */
    extern uint32_t game_time;
    new_world(1, 9, 0, false, WT_DEFAULT, "Commands", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    command_run("/gamemode 1");
    CHECK(pl.mode == 0);
    world_cheats = true;
    command_run("/gamemode c");
    CHECK(pl.mode == 1);
    command_run("/gamemode survival");
    CHECK(pl.mode == 0);
    command_run("/gamemode spectator");
    CHECK(pl.mode == 2 && pl.flying);   /* a spectator: always flying */
    command_run("/gamemode sp");
    CHECK(pl.mode == 2);
    command_run("/gamemode 3");
    CHECK(pl.mode == 2);
    command_run("/gamemode adventure");
    CHECK(pl.mode == 2);   /* (NumBlocks has no adventure) */
    memset(pl.inv, 0, sizeof pl.inv);
    command_run("/give @p minecraft:diamond_sword");
    CHECK(pl.inv[0].id == I_DIAMOND_SWORD);
    command_run("/give Player red_wool 5");
    CHECK(pl.inv[1].id == B_WOOL_RED && item_count(&pl.inv[1]) == 5);
    command_run("/give @p 5 64 2");
    CHECK(pl.inv[2].id == B_PLANKS_BIRCH && item_count(&pl.inv[2]) == 64);
    command_run("/give @p bone_meal 1");
    CHECK(pl.inv[3].id == I_DYE_WHITE);
    command_run("/give @p 276 1 100");
    CHECK(pl.inv[4].id == I_DIAMOND_SWORD && pl.inv[4].aux == 100);
    command_run("/give @p nothing");
    CHECK(!pl.inv[5].id);
    command_run("/clear @p red_wool");
    CHECK(!pl.inv[1].id && pl.inv[0].id);
    command_run("/clear");
    CHECK(!pl.inv[0].id && !pl.inv[2].id);
    command_run("/time set night");
    CHECK(game_time == 13000);
    command_run("/time add 100");
    CHECK(game_time == 13100);
    command_run("/weather thunder 60");
    CHECK(weather.thundering && weather.raining && weather.rain_time == 1200);
    command_run("/weather clear");
    CHECK(!weather.raining);
    command_run("/xp 5L");
    CHECK(pl.xp_level == 5);
    command_run("/xp -2L");
    CHECK(pl.xp_level == 3);
    int sx = (int)floorf(pl.x), sy = (int)floorf(pl.y + 0.5f), sz = (int)floorf(pl.z);
    command_run("/setblock ~ ~3 ~ yellow_wool");
    CHECK(world_get(sx, sy + 3, sz) == B_WOOL_YELLOW);
    /* 1.13's names: a name says which block state or item it is, and no data value goes
     * with one - the woods, the colours and the slabs one 1.8 id could not name */
    memset(pl.inv, 0, sizeof pl.inv);
    command_run("/give @p oak_planks 3");
    CHECK(pl.inv[0].id == B_PLANKS_OAK && item_count(&pl.inv[0]) == 3);
    command_run("/give @p spruce_planks 3");
    CHECK(pl.inv[1].id == B_PLANKS_SPRUCE);
    command_run("/give @p smooth_stone_slab 1");
    CHECK(pl.inv[2].id == B_STONE_SLAB);
    command_run("/give @p smooth_stone 1");
    CHECK(pl.inv[3].id == B_STONE_SLAB_DOUBLE);
    command_run("/give @p petrified_oak_slab 1");
    CHECK(pl.inv[4].id == B_WOOD_SLAB_OLD);
    command_run("/give @p white_terracotta 1");
    CHECK(pl.inv[5].id == B_STAINED_CLAY_WHITE);
    command_run("/give @p orange_terracotta 1");
    CHECK(pl.inv[6].id == B_STAINED_CLAY_ORANGE);
    command_run("/give @p snow_block 1");
    CHECK(pl.inv[7].id == B_SNOW);
    command_run("/give @p grass 1");
    CHECK(pl.inv[8].id == B_TALL_GRASS);
    command_run("/give @p tall_grass 1");
    CHECK(pl.inv[9].id == B_DOUBLE_GRASS_LOWER);
    command_run("/give @p dead_bush 1");
    CHECK(pl.inv[10].id == B_DEAD_SHRUB);
    command_run("/give @p zombie_spawn_egg 1");
    CHECK(pl.inv[11].id == I_ZOMBIE_SPAWN_EGG);
    command_run("/give @p red_dye 1");
    CHECK(pl.inv[12].id == I_DYE_RED);
    command_run("/give @p bone_meal 1");
    CHECK(pl.inv[13].id == I_DYE_WHITE);
    command_run("/give @p tropical_fish 1");
    CHECK(pl.inv[14].id == I_CLOWNFISH);
    command_run("/give @p melon_slice 1");
    CHECK(pl.inv[15].id == I_MELON_SLICE);
    command_run("/give @p sugar_cane 1");
    CHECK(pl.inv[16].id == I_REEDS);
    /* the quartz family's cut blocks: /give names, and Building Blocks (tab 1).
     * (/give stacks into the first free slot: 17..20 after the 0..16 above) */
    command_run("/give @p chiseled_quartz_block 1");
    CHECK(pl.inv[17].id == B_CHISELED_QUARTZ_BLOCK);
    command_run("/give @p quartz_pillar 1");
    CHECK(pl.inv[18].id == B_QUARTZ_PILLAR);
    command_run("/give @p smooth_quartz 1");
    CHECK(pl.inv[19].id == B_SMOOTH_QUARTZ);
    command_run("/give @p quartz_bricks 1");
    CHECK(pl.inv[20].id == B_QUARTZ_BRICKS);
    {
      int lo = tab_start[1], hi = tab_start[2], in_tab[4] = {0, 0, 0, 0};
      static const int qids[4] = {B_CHISELED_QUARTZ_BLOCK, B_QUARTZ_PILLAR, B_SMOOTH_QUARTZ, B_QUARTZ_BRICKS};
      for (int i = lo; i < hi; i++)
        for (int k = 0; k < 4; k++)
          if (tab_items[i] == qids[k]) in_tab[k] = 1;
      CHECK(in_tab[0] && in_tab[1] && in_tab[2] && in_tab[3]);
    }
    /* the concrete pair: /give names, and Building Blocks (tab 1) */
    command_run("/give @p white_concrete 3");
    CHECK(pl.inv[21].id == B_CONCRETE_WHITE && item_count(&pl.inv[21]) == 3);
    command_run("/give @p light_gray_concrete_powder 1");
    CHECK(pl.inv[22].id == B_CONCRETE_POWDER_SILVER);
    {
      int lo = tab_start[1], hi = tab_start[2], in_tab[2] = {0, 0};
      for (int i = lo; i < hi; i++) {
        if (tab_items[i] == B_CONCRETE_RED) in_tab[0] = 1;
        if (tab_items[i] == B_CONCRETE_POWDER_BLACK) in_tab[1] = 1;
      }
      CHECK(in_tab[0] && in_tab[1]);
    }
    /* The nether biomes' blocks: modern /give names, drops' items, and their
     * Natural, Building and Decorative creative tabs. */
    command_run("/give @p crimson_nylium 1");
    CHECK(pl.inv[23].id == B_CRIMSON_NYLIUM);
    command_run("/give @p warped_stem 1");
    CHECK(pl.inv[24].id == B_WARPED_STEM);
    command_run("/give @p crimson_planks 1");
    CHECK(pl.inv[25].id == B_CRIMSON_PLANKS);
    command_run("/give @p soul_soil 1");
    CHECK(pl.inv[26].id == B_SOUL_SOIL);
    command_run("/give @p bone_block 1");
    CHECK(pl.inv[27].id == B_BONE_BLOCK);
    command_run("/give @p weeping_vines 1");
    CHECK(pl.inv[28].id == B_WEEPING_VINES);
    {
      int nat = 0, build = 0, deco = 0;
      for (int i = tab_start[0]; i < tab_start[1]; i++)
        nat |= tab_items[i] == B_CRIMSON_NYLIUM;
      for (int i = tab_start[1]; i < tab_start[2]; i++)
        build |= tab_items[i] == B_CRIMSON_PLANKS || tab_items[i] == B_BONE_BLOCK;
      for (int i = tab_start[2]; i < tab_start[3]; i++)
        deco |= tab_items[i] == B_WEEPING_VINES;
      CHECK(nat && build && deco);
    }
    memset(pl.inv, 0, sizeof pl.inv);
    /* the 1.8 spellings are gone */
    command_run("/give @p wool 5 14");
    CHECK(!pl.inv[0].id);
    command_run("/give @p stained_hardened_clay 1");
    CHECK(!pl.inv[1].id);
    command_run("/give @p tallgrass 1");
    CHECK(!pl.inv[2].id);
    /* /setblock: the modern name places exactly that state, its handling beside the name */
    command_run("/setblock ~ ~3 ~ orange_terracotta");
    CHECK(world_get(sx, sy + 3, sz) == B_STAINED_CLAY_ORANGE);
    command_run("/setblock ~ ~2 ~ soul_soil");
    command_run("/setblock ~ ~3 ~ soul_fire");
    CHECK(world_get(sx, sy + 3, sz) == B_SOUL_FIRE);   /* a fire state, not a /give item */
    {
      char co[40];
      int ca;
      CHECK(command_complete("/setblock ~ ~ ~ soul_fi", 0, &ca, co, sizeof co) &&
            !strcmp(co, "minecraft:soul_fire"));
    }
    command_run("/setblock ~ ~3 ~ infested_stone");
    CHECK(world_get(sx, sy + 3, sz) == B_MONSTER_EGG_STONE);
    command_run("/setblock ~ ~3 ~ oak_stairs destroy");
    CHECK(world_get(sx, sy + 3, sz) == B_OAK_STAIRS);
    command_run("/setblock ~ ~3 ~ light_blue_concrete");
    CHECK(world_get(sx, sy + 3, sz) == B_CONCRETE_LIGHT_BLUE);
    /* (a powder is a falling block: set in the air it drops at once, so it goes on the
     * ground here, where the name is what is being checked) */
    command_run("/setblock ~ ~ ~ black_concrete_powder");
    CHECK(world_get(sx, sy, sz) == B_CONCRETE_POWDER_BLACK);
    /* the barrier: one /give name, the operator items tab, and nothing out of it */
    memset(pl.inv, 0, sizeof pl.inv);
    command_run("/give @p barrier 1");
    CHECK(pl.inv[0].id == B_BARRIER && !strcmp(item_label(B_BARRIER), "Barrier"));
    CHECK(item_icon(B_BARRIER) >= 0 && blk_hard[B_BARRIER] == 255);   /* its own icon, bedrock-hard */
    {
      /* Operator Utilities (tab 12) is where it lives, one tab left of the Survival
       * Inventory tab (13, empty as always), and Search (5) holds it once like everything */
      int where = 0;
      for (int t = 0; t < 14; t++)
        for (int i = tab_start[t]; i < tab_start[t + 1]; i++)
          if (tab_items[i] == B_BARRIER) where |= t == 12 || t == 5 ? 1 << t : 0x8000;
      CHECK(where == (1 << 12 | 1 << 5) && tab_start[12] + 1 == tab_start[13] && tab_start[13] == tab_start[14]);
      /* it gives nothing back (BlockBarrier has no loot table), even where a block is dropped */
      Stack drop[2];
      CHECK(block_drops(B_BARRIER, B_BARRIER, drop) == 0);
    }
    command_run("/setblock ~ ~3 ~ barrier");
    CHECK(world_get(sx, sy + 3, sz) == B_BARRIER);
    /* /clear: a name clears the one item it names */
    memset(pl.inv, 0, sizeof pl.inv);
    command_run("/give @p red_wool 2");
    command_run("/give @p white_wool 2");
    command_run("/clear @p red_wool");
    CHECK(!pl.inv[0].id && pl.inv[1].id == B_WOOL_WHITE);
    command_run("/clear @p oak_planks");
    CHECK(pl.inv[1].id == B_WOOL_WHITE);
    command_run("/gamerule keepInventory true");
    CHECK(rule(GR_KEEP_INVENTORY) && rule(GR_DAYLIGHT_CYCLE));
    command_run("/gamerule doDaylightCycle false");
    CHECK(!rule(GR_DAYLIGHT_CYCLE));
    command_run("/effect @p poison 10 1");
    CHECK(pl.eff[EF_POISON] == 200 && pl.eff_amp[EF_POISON] == 1);
    command_run("/effect @p clear");
    CHECK(!pl.eff[EF_POISON]);
    command_run("/tp 10 80 -20");
    CHECK(pl.x == 10.5f && pl.y == 80 && pl.z == -19.5f);
    command_run("/tp ~1 ~ ~-0.5");
    CHECK(pl.x == 11.5f && pl.z == -20);
    /* the world keeps its cheats and rules */
    save_world();
    world_cheats = false, game_rules = GR_DEFAULT;
    CHECK(load_world(1) && world_cheats && rule(GR_KEEP_INVENTORY) && !rule(GR_DAYLIGHT_CYCLE));
    WorldInfo wi;
    CHECK(world_info(1, &wi) && wi.cheats);
    /* finishing words, as Tab does */
    char o[40];
    int at;
    CHECK(command_complete("/gam", 0, &at, o, sizeof o) && at == 1 && !strcmp(o, "gamemode"));
    CHECK(command_complete("/gam", 1, &at, o, sizeof o) && !strcmp(o, "gamerule"));
    CHECK(!command_complete("/gam", 2, &at, o, sizeof o));
    CHECK(command_complete("/give @p diamond_sw", 0, &at, o, sizeof o) && at == 9 && !strcmp(o, "minecraft:diamond_sword"));
    CHECK(command_complete("/give @p oak_pl", 0, &at, o, sizeof o) && at == 9 && !strcmp(o, "minecraft:oak_planks"));
    CHECK(command_complete("/give @p red_w", 0, &at, o, sizeof o) && at == 9 && !strcmp(o, "minecraft:red_wool"));
    CHECK(command_complete("/setblock ~ ~ ~ ", 0, &at, o, sizeof o) && at == 16 && !strcmp(o, "minecraft:stone"));
    CHECK(command_complete("/setblock ~ ~ ~ wat", 0, &at, o, sizeof o) && at == 16 && !strcmp(o, "minecraft:water"));
    CHECK(command_complete("/setblock ~ ~ ~ warp", 0, &at, o, sizeof o) && at == 16 && !strcmp(o, "minecraft:warped_fungus"));
    CHECK(command_complete("/setblock ~ ~3 ~ oak_stairs des", 0, &at, o, sizeof o) && !strcmp(o, "destroy"));
    CHECK(command_complete("/gamemode cr", 0, &at, o, sizeof o) && !strcmp(o, "creative"));
    CHECK(command_complete("/time set ", 1, &at, o, sizeof o) && !strcmp(o, "night"));
    delete_world(1);
  }
  {
    /* the game mode switcher (sin + cos): cheats first, the pair through the modes */
    world_new(0, "ut20");
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    world_cheats = false;
    pl.mode = 0, gui = GUI_NONE;
    mode_open();
    CHECK(gui == GUI_NONE && pl.mode == 0);   /* no cheats: the message, not the screen */
    world_cheats = true;
    /* in Creative it opens on Survival, the next one round the cycle (Creative,
     * Survival, Spectator, back to Creative) */
    pl.mode = 1;
    mode_open();
    CHECK(gui == GUI_MODE);
    mode_next();   /* and cos goes on to Spectator, the third mode */
    mode_use();
    CHECK(gui == GUI_NONE && pl.mode == 2 && pl.flying);   /* a spectator is always flying */
    /* Back, mid-way: nothing changes */
    pl.mode = 1;
    mode_open();
    mode_cancel();
    CHECK(gui == GUI_NONE && pl.mode == 1);
    /* in Spectator it opens on Creative, so confirming switches to Creative */
    pl.mode = 2, pl.flying = true;
    mode_open();
    mode_use();
    CHECK(gui == GUI_NONE && pl.mode == 1);
    /* in Survival it opens on Spectator, the next one round the cycle */
    pl.mode = 0, pl.flying = true;
    mode_open();
    mode_use();
    CHECK(gui == GUI_NONE && pl.mode == 2 && pl.flying);
    /* in Survival again: cos goes on to Creative ... */
    pl.mode = 0, pl.flying = true;
    mode_open(), mode_next(), mode_use();
    CHECK(gui == GUI_NONE && pl.mode == 1 && pl.flying);
    /* ... and round again, where confirming brings flying to an end */
    pl.mode = 0, pl.flying = true;
    mode_open(), mode_next(), mode_next(), mode_use();
    CHECK(gui == GUI_NONE && pl.mode == 0 && !pl.flying);
    gui = GUI_NONE;
    /* and the mode is kept with the world */
    save_world();
    pl.mode = 1;
    CHECK(load_world(1) && pl.mode == 0);
    plat_remove_prefix("ut20");
  }
  {
    /* spectator: flying free through the blocks, nothing to touch, and a mob's eyes */
    world_new(0, "ut21");
    int g = 110;
    for (int z = -8; z <= 60; z++)
      for (int x = 2; x <= 14; x++)
        for (int y = g - 2; y < g + 4; y++) world_set(x, y, z, y >= g ? B_AIR : B_STONE);
    memset(&pl, 0, sizeof pl);
    pl.x = 8.5f, pl.z = 0.5f, pl.y = (float)g, pl.health = 20, pl.food = 20, pl.mode = 2, pl.flying = true;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    /* free flight: forward carries it, and nothing makes it fall */
    float z0 = pl.z;
    for (int t = 0; t < 10; t++) player_tick(K_FWD, t == 0 ? K_FWD : 0);
    CHECK(pl.z - z0 > 1.0f && pl.z - z0 < 10.0f);   /* flying forward, as in creative */
    CHECK(!pl.on_ground);
    float fy = pl.y;
    for (int t = 0; t < 20; t++) player_tick(0, 0);
    CHECK(pl.y == fy);            /* no gravity: it hangs where it is left */
    /* through the rock: inside the stone it goes on, straight, and still does not fall */
    pl.y = (float)(g - 1);
    float sy = pl.y;
    z0 = pl.z;
    for (int t = 0; t < 20; t++) player_tick(K_FWD, t == 0 ? K_FWD : 0);
    CHECK(pl.z - z0 > 1.0f && pl.y == sy);
    /* the void under the world: it flies down into it, and nothing there hurts it
     * (Player.isInvulnerableTo); the top of the world still stops it */
    for (int t = 0; t < 600; t++) player_tick(K_SNEAK, 0);
    CHECK(pl.y < (float)WORLD_MIN_Y && pl.health == 20 && !pl.dead);   /* under the deep layer's floor */
    for (int t = 0; t < 900; t++) player_tick(K_JUMP, 0);
    CHECK(pl.y > WORLD_H + 20 && pl.y <= WORLD_H + 24);
    /* nothing hurts a spectator */
    pl.health = 20;
    player_hurt(5, DMG_GENERIC), player_hurt(5, DMG_FALL), player_hurt(5, DMG_LAVA), player_hurt(4, DMG_VOID);
    CHECK(pl.health == 20 && !pl.dead);
    /* nothing is in reach: no block is mined or placed, none is outlined */
    player_look(pl.x, pl.y, pl.z, 0, 0);
    CHECK(pl.hit_face < 0);
    player_tick(K_ATTACK | K_USE, K_ATTACK | K_USE);
    CHECK(pl.health == 20);
    /* a mob in front: the eyes' pick finds it, OK takes its eyes, alpha gives them back */
    pl.x = 8.5f, pl.z = 8.5f, pl.y = (float)g;
    world_set((int)pl.x, (int)pl.y, (int)pl.z + 4, B_AIR);
    Entity *m = ent_new(E_ZOMBIE, pl.x, (float)g, pl.z + 4);
    CHECK(m != NULL);
    player_look(pl.x, pl.y, pl.z, 0, 0);
    CHECK(spec_target == (int8_t)(m - ents));
    player_tick(K_ATTACK, K_ATTACK);
    CHECK(spectate == (int8_t)(m - ents));
    CHECK(spec_eye > 1.5f && spec_eye < 1.8f);   /* the zombie's eyes: 0.85 of its 1.95 */
    player_tick(0, 0);
    CHECK(pl.z == m->z && pl.yaw == m->yaw);     /* the camera is the mob's */
    player_tick(K_SNEAK, K_SNEAK);
    CHECK(spectate < 0 && spec_eye == 0);
    plat_remove_prefix("ut21");
  }
  {
    /* fire: lit on stone it goes out; by planks it spreads and burns them; flint and steel lights it */
    world_new(0, "ut12");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    memset(&weather, 0, sizeof weather);
    weather.rain_time = 1000000, rain_str = 0;
    game_rules = GR_DEFAULT;
    for (int z = 0; z < 20; z++)
      for (int x = 0; x < 20; x++)
        for (int y = 69; y < 76; y++) world_set(x, y, z, y == 69 ? B_STONE : B_AIR);
    fire_set(8, 70, 4, 0);
    CHECK(world_get(8, 70, 4) == B_FIRE);
    for (int t = 0; t < 1200; t++) ticks_run++, world_tick();
    CHECK(world_get(8, 70, 4) == B_AIR);
    int planks = 0;
    for (int x = 4; x < 13; x++)
      for (int y = 70; y < 73; y++) world_set(x, y, 12, B_PLANKS_OAK);
    fire_set(8, 70, 11, 0);
    for (int t = 0; t < 4000; t++) ticks_run++, world_tick();
    for (int x = 4; x < 13; x++)
      for (int y = 70; y < 73; y++) planks += world_get(x, y, 12) == B_PLANKS_OAK;
    CHECK(planks < 27);
    /* with doFireTick off it stays as it is */
    game_rules &= (uint8_t)~GR_FIRE_TICK;
    fire_set(2, 70, 2, 0);
    for (int t = 0; t < 1200; t++) ticks_run++, world_tick();
    CHECK(world_get(2, 70, 2) == B_FIRE);
    game_rules = GR_DEFAULT;
    world_set(2, 70, 2, B_AIR);
    /* flint and steel on the floor: fire on it */
    memset(&pl, 0, sizeof pl);
    pl.x = 16.5f, pl.y = 70, pl.z = 16.5f, pl.health = 20, pl.food = 20, pl.mode = 0, pl.on_ground = true;
    pl.inv[0] = (Stack){I_FLINT_AND_STEEL, 0};
    player_look(pl.x, pl.y + 1.62f, pl.z, 0, 60);
    player_tick(K_USE, K_USE);
    CHECK(world_get(16, 70, 17) == B_FIRE && pl.inv[0].aux == 1);
    plat_remove_prefix("ut12");
  }
  {
    /* superflat: bedrock, two dirt, grass; the player on it; kept as superflat */
    new_world(2, 77, 0, false, WT_FLAT, "Flat", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int x = (int)floorf(pl.x) + 7, z = (int)floorf(pl.z) - 5;
    CHECK(pl.y == 4 && world_get(x, 0, z) == B_BEDROCK && world_get(x, 1, z) == B_DIRT && world_get(x, 2, z) == B_DIRT &&
          world_get(x, 3, z) == B_GRASS && world_get(x, 4, z) == B_AIR && world_get(x, 40, z) == B_AIR);
    save_world();
    world_type = WT_DEFAULT;
    WorldInfo wi;
    CHECK(load_world(2) && world_type == WT_FLAT && world_info(2, &wi) && wi.type == WT_FLAT);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(x + 30, 3, z) == B_GRASS || !world_loaded(x + 30, 3, z));
    delete_world(2);
    world_type = WT_DEFAULT;
  }
  {
    /* superflat presets: plain block lists, one layer each; legacy version;layers;biome still loads */
    char e[48], simple[FLAT_MAX + 1];
    CHECK(flat_validate("bedrock,dirt,dirt,grass", e, sizeof e));
    CHECK(flat_validate("bedrock,stone,stone,dirt,dirt,grass", e, sizeof e));
    CHECK(flat_validate("7,3,3,2", e, sizeof e));
    CHECK(flat_validate("bedrock,159:14,glass", e, sizeof e));
    CHECK(!flat_validate("bedrock,2xdirt,grass", e, sizeof e) && e[0]);   /* no counts */
    CHECK(!flat_validate("bedrock,nosuchblock", e, sizeof e) && e[0]);
    CHECK(!flat_validate("2;7,9999;1", e, sizeof e) && e[0]);
    CHECK(flat_validate("2;7,2x3,2;1", e, sizeof e));
    CHECK(flat_simplify("2;7,2x3,2;1", simple, sizeof simple) && !strcmp(simple, "bedrock,dirt,dirt,grass"));
    CHECK(flat_simplify("bedrock,stone,stone,dirt,dirt,grass", simple, sizeof simple) &&
          !strcmp(simple, "bedrock,stone,stone,dirt,dirt,grass"));
    /* 1.13's names, with the 1.8 spellings as aliases: an old preset still loads, and one
     * kept with them is shown with the modern names */
    CHECK(flat_validate("bedrock,terracotta,whiteterracotta,orangeterracotta", e, sizeof e));
    CHECK(flat_validate("bedrock,chiseledquartz,quartzpillar,smoothquartz,quartzbricks", e, sizeof e));
    CHECK(flat_validate("bedrock,hardenedclay,stainedclay,whiteclay", e, sizeof e));
    CHECK(flat_validate("bedrock,oakplanks,spruceplanks,oaklog,snowblock", e, sizeof e));
    CHECK(flat_validate("bedrock,planks,logoak,snow,mossstone,wool", e, sizeof e));
    CHECK(flat_simplify("bedrock,hardenedclay,whiteclay,wool", simple, sizeof simple) &&
          !strcmp(simple, "bedrock,terracotta,whiteterracotta,whitewool"));
    new_world(2, 78, 0, false, WT_FLAT, "Simple", "bedrock,stone,stone,dirt,dirt,grass");
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int x = (int)floorf(pl.x), z = (int)floorf(pl.z);
    CHECK(pl.y == 6 && gen_top(x, z) == 5 && world_get(x, 0, z) == B_BEDROCK && world_get(x, 1, z) == B_STONE &&
          world_get(x, 3, z) == B_DIRT && world_get(x, 5, z) == B_GRASS && world_get(x, 6, z) == B_AIR);
    save_world();
    WorldInfo wi;
    CHECK(load_world(2) && !strcmp(world_preset, "bedrock,stone,stone,dirt,dirt,grass") && world_info(2, &wi) &&
          !strcmp(wi.preset, "bedrock,stone,stone,dirt,dirt,grass"));
    delete_world(2);
    world_type = WT_DEFAULT;
    /* and the terracotta names put their blocks down */
    new_world(2, 79, 0, false, WT_FLAT, "Terracotta", "bedrock,terracotta,orangeterracotta");
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    x = (int)floorf(pl.x), z = (int)floorf(pl.z);
    CHECK(world_get(x, 0, z) == B_BEDROCK && world_get(x, 1, z) == B_HARDENED_CLAY &&
          world_get(x, 2, z) == B_STAINED_CLAY_ORANGE);
    save_world();
    WorldInfo wi2;
    CHECK(load_world(2) && world_info(2, &wi2) && !strcmp(wi2.preset, "bedrock,terracotta,orangeterracotta"));
    delete_world(2);
    world_type = WT_DEFAULT;
  }
  {
    /* the test world: an empty world that is always the same, whatever the seed, holding
     * everything the game has once - every block state in a grid, every mob in a cage of
     * its own, the structures on their own platforms - and nothing in it moves on its own */
    static uint8_t gs[GEN_SCRATCH];
    static uint8_t sa[64 * 256 / 8 * 9 + 4], sb[64 * 256 / 8 * 9 + 4];
    int t0 = 32, th = 44;   /* the rows the whole showcase stands in: 32..76 */
    memset(ents, 0, sizeof ents);   /* (the cages put back only what is gone) */
    /* the showcase's slab, through the generator's own write path */
#define TEST_SLAB(cx, cz, buf) (gen_scratch(gs), gen_slab(cx, cz, t0, th, buf), gen_scratch(NULL))
#define TEST_CELL(buf, y, lx, lz) b9_get(buf, ((y - t0) * 16 + (lz)) * 16 + (lx))
    new_world(6, 31337, 1, true, WT_TEST, "Test", NULL);
    save_world();
    WorldInfo twi;
    CHECK(world_type == WT_TEST && world_info(6, &twi) && twi.type == WT_TEST && twi.mode == 1);
    /* the sun stands still: it is a showcase, not a world to be played through */
    CHECK(game_rules == (GR_DEFAULT & ~GR_DAYLIGHT_CYCLE));
    /* the spawn: the middle of the spawn platform, standing on it */
    int sx, sy, sz;
    gen_spawn(&sx, &sy, &sz);
    CHECK(sx == (TEST_SPAWN_X0 + TEST_SPAWN_X1) / 2 && sz == (TEST_SPAWN_Z0 + TEST_SPAWN_Z1) / 2 &&
          sy == TEST_PLATFORM_Y + 1);
    /* the same chunk whatever the seed (a second world of another seed gives the same slab) */
    TEST_SLAB(1, 2, sa);
    gen_init(-4242);
    TEST_SLAB(1, 2, sb);
    CHECK(!memcmp(sa, sb, (size_t)th * 256 / 8 * 9));
    /* and the same whatever window is asked for it (its rows overlap exactly) */
    gen_init(31337);
    TEST_SLAB(1, 2, sa);
    gen_scratch(gs);
    gen_slab(1, 2, 60, 12, sb);
    gen_scratch(NULL);
    for (int y = 60; y < 72; y++)
      for (int lz = 0; lz < 16; lz++)
        for (int lx = 0; lx < 16; lx++) CHECK(TEST_CELL(sa, y, lx, lz) == TEST_CELL(sb, y, lx, lz));
    /* every block state in the grid: one air gap between each, each on its own barrier,
     * and the liquids held in by barriers on their four sides */
    static uint8_t seen[NSTATES];
    static int liq[16][2];
    int n = 0, liquids = 0;
    for (int cz = (TEST_GRID_Z0 >> 4); cz <= (TEST_GRID_Z1 >> 4); cz++)
      for (int cx = (TEST_GRID_X0 >> 4); cx <= (TEST_GRID_X1 >> 4); cx++) {
        TEST_SLAB(cx, cz, sb);
        for (int j = 0; j < TEST_GRID_ROWS; j++)
          for (int i = 0; i < TEST_GRID_COLS; i++) {
            int x = TEST_GRID_X0 + i * 2, z = TEST_GRID_Z0 + j * 2;
            if (x >> 4 != cx || z >> 4 != cz) continue;
            int lx = x & 15, lz = z & 15, b = TEST_CELL(sb, TEST_BLOCK_Y, lx, lz);
            seen[b]++, n++;
            CHECK(TEST_CELL(sb, TEST_BLOCK_Y - 1, lx, lz) == B_BARRIER);   /* what it floats on */
            CHECK(TEST_CELL(sb, TEST_BLOCK_Y + 1, lx, lz) == B_AIR);       /* air above it */
            int side = b == B_WATER || b == B_FLOWING_WATER || (b >= B_FLOWING_WATER_2 && b <= B_FALLING_WATER) ||
                      b == B_LAVA || b == B_FLOWING_LAVA || (b >= B_FLOWING_LAVA_4 && b <= B_FALLING_LAVA);
            if (side) {
              liquids++;
              if (liquids <= 16) liq[liquids - 1][0] = x, liq[liquids - 1][1] = z;
            }
            if (side)   /* held in: a barrier on each of its four sides */
              for (int dx = -1; dx <= 1; dx += 2) {
                if ((x + dx) >> 4 == cx && TEST_CELL(sb, TEST_BLOCK_Y, lx + dx, lz) != B_BARRIER) CHECK(false);
                if ((z + dx) >> 4 == cz && TEST_CELL(sb, TEST_BLOCK_Y, lx, lz + dx) != B_BARRIER) CHECK(false);
              }
          }
      }
    /* the gaps between the blocks: air, and a barrier only where a liquid beside it is
     * held in by it (a second pass: every liquid of the grid is known by now) */
    for (int cz = (TEST_GRID_Z0 >> 4); cz <= (TEST_GRID_Z1 >> 4); cz++)
      for (int cx = (TEST_GRID_X0 >> 4); cx <= (TEST_GRID_X1 >> 4); cx++) {
        TEST_SLAB(cx, cz, sb);
        for (int lz = 0; lz < 16; lz++)
          for (int lx = 0; lx < 16; lx++) {
            int x = cx * 16 + lx, z = cz * 16 + lz;
            if (x < TEST_GRID_X0 || x > TEST_GRID_X1 || z < TEST_GRID_Z0 || z > TEST_GRID_Z1) continue;
            if (!((x - TEST_GRID_X0) & 1) && !((z - TEST_GRID_Z0) & 1)) continue;   /* a block's cell */
            bool need = false;
            for (int k = 0; k < liquids && k < 16; k++)
              need |= abs(liq[k][0] - x) + abs(liq[k][1] - z) == 1;
            CHECK(TEST_CELL(sb, TEST_BLOCK_Y, lx, lz) == (need ? B_BARRIER : B_AIR));
          }
      }
    int blocks = 0;
    for (int ssi = 1; ssi < B_COUNT; ssi++)
      if (blk_id[ssi]) blocks++;
    CHECK(n == blocks && liquids == 14);   /* one of each block state; air and the item ids are not */
    for (int ssi = 1; ssi < B_COUNT; ssi++)
      if (blk_id[ssi]) CHECK(seen[ssi] == 1);
    /* the chests of every item that has no block of its own: floating on their barriers,
     * in a row after the grid, their contents one stack of each of those items */
    int itemz = 0;
    for (int id = 256; id < 256 + ITEM_SLOTS; id++)
      if (is_item_id(id) && it_place[id - 256] == 255) itemz++;
    int nchests = (itemz + 26) / 27;
    CHECK(nchests == TEST_ITEM_CHESTS);
    {
      TEST_SLAB(TEST_ITEM_X0 >> 4, TEST_ITEM_Z >> 4, sb);
      for (int k = 0; k < TEST_ITEM_CHESTS; k++) {
        int lx = (TEST_ITEM_X0 + k) & 15, lz = TEST_ITEM_Z & 15;
        CHECK(TEST_CELL(sb, TEST_SECTION_Y, lx, lz) == B_BARRIER);   /* what it floats on */
        CHECK(TEST_CELL(sb, TEST_BLOCK_Y, lx, lz) == B_CHEST);
      }
    }
    /* the chests' contents: one stack of each item that has no block, 27 a chest. A chest
     * is 6 bytes (x, z, y, used) then its 27 stacks of four, and the world's tiles are
     * eight chests of them */
    {
      extern void *tiles_data(uint32_t *len);
      uint32_t tl;
      const uint8_t *t = tiles_data(&tl);
      int used = 0, items = 0;
      for (int i = 0; i < 8; i++) {
        const uint8_t *c = t + i * (6 + 27 * 4);
        if (!c[5]) continue;   /* used */
        used++;
        for (int q = 0; q < 27; q++) {
          uint16_t id, aux;
          memcpy(&id, c + 6 + q * 4, 2), memcpy(&aux, c + 6 + q * 4 + 2, 2);
          if (!id) continue;
          items++;
          CHECK(is_item_id(id) && it_place[id - 256] == 255 && aux == 1);
        }
      }
      CHECK(used == TEST_ITEM_CHESTS && items == itemz);
    }
    /* the cages: one for each mob, 5 x 5 of grass of its own floating, the mob on the
     * middle block, a ring of barriers on its four sides and one over its head */
    for (int k = 0; k < 10; k++) {
      int cx, cy, cz;
      test_cage(k, &cx, &cy, &cz);
      TEST_SLAB(cx >> 4, cz >> 4, sb);
      int lx = cx & 15, lz = cz & 15;
      for (int dz = -2; dz <= 2; dz++)
        for (int dx = -2; dx <= 2; dx++)
          if ((cx + dx) >> 4 == cx >> 4 && (cz + dz) >> 4 == cz >> 4)
            CHECK(TEST_CELL(sb, TEST_SECTION_Y, lx + dx, lz + dz) == B_GRASS);
      CHECK(TEST_CELL(sb, cy, lx, lz) == B_AIR);              /* where the mob stands */
      CHECK(TEST_CELL(sb, cy + 1, lx, lz) == B_BARRIER);      /* and over its head */
      static const int d[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
      for (int q = 0; q < 4; q++)
        for (int y = TEST_CAGE_Y; y <= TEST_CAGE_Y + 1; y++)
          if ((cx + d[q][0]) >> 4 == cx >> 4 && (cz + d[q][1]) >> 4 == cz >> 4)
            CHECK(TEST_CELL(sb, y, lx + d[q][0], lz + d[q][1]) == B_BARRIER);
      /* nothing of the showcase stands on the spawn platform's row here */
      CHECK(TEST_CELL(sb, TEST_PLATFORM_Y, lx, lz) == B_AIR);
    }
    /* the mobs: one of each in its own cage, standing where the cage says, and nothing else */
    mobs_spawn();
    int mobs = 0;
    for (int i = 0; i < N_ENT; i++) {
      int t = ents[i].type;
      if (!is_mob(t)) continue;
      mobs++;
      int cx, cy, cz;
      test_cage(t - E_ZOMBIE, &cx, &cy, &cz);
      CHECK(floorf(ents[i].x) == cx && floorf(ents[i].y) == cy && floorf(ents[i].z) == cz);
      CHECK(ents[i].state != 255);
    }
    CHECK(mobs == 10);
    /* one taken away comes back, and a second pass adds nothing */
    for (int i = 0; i < N_ENT; i++)
      if (is_mob(ents[i].type)) {
        ents[i].type = E_NONE;
        break;
      }
    mobs_spawn();
    mobs = 0;
    for (int i = 0; i < N_ENT; i++)
      if (is_mob(ents[i].type)) mobs++;
    CHECK(mobs == 10);
    /* the structures, stamped from the baked bytes (test_data.h): the pyramid's sandstone
     * and its nine TNT, plate and four chests, the dungeon's walls, spawner, ceiling
     * light and doorway, the well's water */
    int sand = 0, tnt = 0, plate = 0, chest = 0, terra = 0;
    int tx0 = TEST_TEMPLE_OX * 16 - 2, ty0 = TEST_TEMPLE_Y0 + TEST_TEMPLE_DY, tz0 = TEST_TEMPLE_OZ * 16 - 2;
    for (int cz = tz0 >> 4; cz <= (tz0 + TEST_TEMPLE_SZ - 1) >> 4; cz++)
      for (int cx = tx0 >> 4; cx <= (tx0 + TEST_TEMPLE_SX - 1) >> 4; cx++) {
        TEST_SLAB(cx, cz, sb);
        for (int lz = 0; lz < 16; lz++)
          for (int lx = 0; lx < 16; lx++) {
            int x = cx * 16 + lx, z = cz * 16 + lz;
            if (x < tx0 || x >= tx0 + TEST_TEMPLE_SX || z < tz0 || z >= tz0 + TEST_TEMPLE_SZ) continue;
            for (int y = ty0; y < ty0 + TEST_TEMPLE_SY; y++) {
              int b = TEST_CELL(sb, y, lx, lz);
              if (b == B_SANDSTONE || b == B_SANDSTONE_SMOOTH || b == B_SANDSTONE_CHISELED ||
                  b == B_SANDSTONE_SLAB || b == B_SANDSTONE_STAIRS || b == B_SANDSTONE_STAIRS_W ||
                  b == B_SANDSTONE_STAIRS_S || b == B_SANDSTONE_STAIRS_N)
                sand++;
              if (b == B_TNT) tnt++;
              if (b == B_STONE_PRESSURE_PLATE) plate++;
              if (b == B_CHEST) chest++;
              if (b == B_STAINED_CLAY_ORANGE || b == B_STAINED_CLAY_BLUE) terra++;
            }
          }
      }
    CHECK(sand > 1000 && tnt == 9 && plate == 1 && chest == 4 && terra > 20);
    /* the dungeon: its spawner where the room's middle is, its doorway open, its
     * ceiling light over it, its floor and ceiling cobble under and over the room,
     * and the room closed on every side (flood-filled from the spawner through air:
     * the fill must stay in the box) */
    int dx0 = TEST_DUNG_X - TEST_DUNG_SPAWN_X, dy0 = TEST_DUNG_Y0 + TEST_DUNG_DY,
        dz0 = TEST_DUNG_Z - TEST_DUNG_SPAWN_Z;
    int spawner = 0, dchest = 0, cobble = 0;
    for (int cz = dz0 >> 4; cz <= (dz0 + TEST_DUNG_SZ - 1) >> 4; cz++)
      for (int cx = dx0 >> 4; cx <= (dx0 + TEST_DUNG_SX - 1) >> 4; cx++) {
        TEST_SLAB(cx, cz, sb);
        for (int lz = 0; lz < 16; lz++)
          for (int lx = 0; lx < 16; lx++) {
            int x = cx * 16 + lx, z = cz * 16 + lz;
            if (x < dx0 || x >= dx0 + TEST_DUNG_SX || z < dz0 || z >= dz0 + TEST_DUNG_SZ) continue;
            for (int y = dy0; y < dy0 + TEST_DUNG_SY; y++) {
              int b = TEST_CELL(sb, y, lx, lz);
              if (b == B_COBBLESTONE || b == B_MOSSY_COBBLESTONE) cobble++;
              if (b == B_MOB_SPAWNER) spawner++;
              if (b == B_CHEST) dchest++;
            }
          }
      }
    CHECK(spawner == 1 && dchest >= 1 && cobble > 40);
    {
      int spx = dx0 + TEST_DUNG_SPAWN_X, spy = dy0 + TEST_DUNG_SPAWN_Y, spz = dz0 + TEST_DUNG_SPAWN_Z;
      TEST_SLAB(spx >> 4, spz >> 4, sb);
      CHECK(TEST_CELL(sb, spy, spx & 15, spz & 15) == B_MOB_SPAWNER);
      int dox = dx0 + TEST_DUNG_DOOR_X, doy = dy0 + TEST_DUNG_DOOR_Y, doz = dz0 + TEST_DUNG_DOOR_Z;
      TEST_SLAB(dox >> 4, doz >> 4, sb);
      CHECK(TEST_CELL(sb, doy, dox & 15, doz & 15) == B_AIR);
      CHECK(TEST_CELL(sb, doy + 1, dox & 15, doz & 15) == B_AIR);
      /* the entrance passage to it, a block wide and two high from the box's face */
      int pdx0 = dx0, pdz0 = dz0;
      for (int ly = 0; ly < 2; ly++)
        for (int lz = 0; lz < TEST_DUNG_DOOR_Z; lz++) {
          int px = pdx0 + TEST_DUNG_DOOR_X, py = doy + ly, pz = pdz0 + lz;
          TEST_SLAB(px >> 4, pz >> 4, sb);
          CHECK(TEST_CELL(sb, py, px & 15, pz & 15) == B_AIR);
        }
      int glx = dx0 + TEST_DUNG_GLOW_X, gly = dy0 + TEST_DUNG_GLOW_Y, glz = dz0 + TEST_DUNG_GLOW_Z;
      TEST_SLAB(glx >> 4, glz >> 4, sb);
      CHECK(TEST_CELL(sb, gly, glx & 15, glz & 15) == B_GLOWSTONE);
      /* closed but for the entrance passage: the room's air, flood-filled from beside
       * the spawner without stepping into the passage, stays in the box */
      CHECK(TEST_DUNG_SX == 17 && TEST_DUNG_SY == 13 && TEST_DUNG_SZ == 17);   /* the baker's box */
      static uint8_t seen[17 * 13 * 17];
      memset(seen, 0, sizeof seen);
      int stack[17 * 13 * 17], n = 0, escaped = 0, vol = 0;
      static const int8_t d[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
      for (int k = 0; k < 6; k++) {
        int nx = spx + d[k][0], ny = spy + d[k][1], nz = spz + d[k][2];
        if (nx < dx0 || nx >= dx0 + TEST_DUNG_SX || ny < dy0 || ny >= dy0 + TEST_DUNG_SY || nz < dz0 ||
            nz >= dz0 + TEST_DUNG_SZ)
          continue;
        stack[n++] = ((ny - dy0) * TEST_DUNG_SZ + (nz - dz0)) * TEST_DUNG_SX + (nx - dx0);
      }
      while (n && !escaped) {
        int ii = stack[--n], lx = ii % TEST_DUNG_SX, t = ii / TEST_DUNG_SX, lz = t % TEST_DUNG_SZ,
            ly = t / TEST_DUNG_SZ;
        int x = dx0 + lx, y = dy0 + ly, z = dz0 + lz;
        if (seen[ii]) continue;
        seen[ii] = 1;
        TEST_SLAB(x >> 4, z >> 4, sb);
        if (TEST_CELL(sb, y, x & 15, z & 15) != B_AIR) continue;
        vol++;
        if (lx == 0 || ly == 0 || lz == 0 || lx == TEST_DUNG_SX - 1 || ly == TEST_DUNG_SY - 1 ||
            lz == TEST_DUNG_SZ - 1)
          escaped = 1;
        for (int k = 0; k < 6; k++) {
          int nx = lx + d[k][0], ny = ly + d[k][1], nz = lz + d[k][2];
          if (nx < 0 || nx >= TEST_DUNG_SX || ny < 0 || ny >= TEST_DUNG_SY || nz < 0 || nz >= TEST_DUNG_SZ)
            continue;
          /* not into the entrance passage (its own air is checked above) */
          if (dx0 + nx == dox && dz0 + nz < doz && (dy0 + ny == doy || dy0 + ny == doy + 1)) continue;
          stack[n++] = (ny * TEST_DUNG_SZ + nz) * TEST_DUNG_SX + nx;
        }
      }
      CHECK(!escaped && vol > 50 && vol < 400);
      /* its floor and its ceiling, cobble all across the room */
      int fry = spy - 1, cry = spy + 4;
      for (int dz = -4; dz <= 4; dz++)
        for (int dx = -4; dx <= 4; dx++) {
          TEST_SLAB((spx + dx) >> 4, (spz + dz) >> 4, sb);
          int fb = TEST_CELL(sb, fry, (spx + dx) & 15, (spz + dz) & 15);
          int cb = TEST_CELL(sb, cry, (spx + dx) & 15, (spz + dz) & 15);
          if (abs(dx) <= 3 && abs(dz) <= 3) {
            CHECK(fb == B_COBBLESTONE || fb == B_MOSSY_COBBLESTONE);
            CHECK(cb == B_COBBLESTONE || cb == B_MOSSY_COBBLESTONE || cb == B_GLOWSTONE);
          }
        }
    }
    /* the well: its five water blocks and its twelve slabs, floating nowhere (its sand
     * comes with it) */
    int ex0 = TEST_WELL_X - TEST_WELL_MID_X, ey0 = TEST_WELL_Y0 + TEST_WELL_DY, ez0 = TEST_WELL_Z - TEST_WELL_MID_Z;
    int water = 0, slabs = 0;
    for (int cz = ez0 >> 4; cz <= (ez0 + TEST_WELL_SX - 1) >> 4; cz++)
      for (int cx = ex0 >> 4; cx <= (ex0 + TEST_WELL_SX - 1) >> 4; cx++) {
        TEST_SLAB(cx, cz, sb);
        for (int lz = 0; lz < 16; lz++)
          for (int lx = 0; lx < 16; lx++) {
            int x = cx * 16 + lx, z = cz * 16 + lz;
            if (x < ex0 || x >= ex0 + TEST_WELL_SX || z < ez0 || z >= ez0 + TEST_WELL_SZ) continue;
            for (int y = ey0; y < ey0 + TEST_WELL_SY; y++) {
              int b = TEST_CELL(sb, y, lx, lz);
              if (b == B_WATER) water++;
              if (b == B_SANDSTONE_SLAB) slabs++;
            }
          }
      }
    CHECK(water == 5 && slabs == 12);
    /* barriers under the whole well (its sand stands over the void) */
    for (int z = ez0; z < ez0 + TEST_WELL_SZ; z++)
      for (int x = ex0; x < ex0 + TEST_WELL_SX; x++) {
        TEST_SLAB(x >> 4, z >> 4, sb);
        CHECK(TEST_CELL(sb, ey0 - 1, x & 15, z & 15) == B_BARRIER);
      }
    /* nothing in it moves on its own: the same blocks after a while of ticks */
    TEST_SLAB(2, 6, sa);
    for (int t = 0; t < 400; t++) ticks_run++, world_tick();
    TEST_SLAB(2, 6, sb);
    CHECK(!memcmp(sa, sb, (size_t)th * 256 / 8 * 9));
    /* and nothing moves in the live world either: cache each structure (the settle
     * pass runs as the cache moves), tick a while (fluids!), and every cell reads
     * back what the generator stamped - no sand falls, no water runs */
    {
      /* the structures and the grid (its liquids sit on barriers too) */
      int bx[4][6] = {{tx0, ty0, tz0, TEST_TEMPLE_SX, TEST_TEMPLE_SY, TEST_TEMPLE_SZ},
                      {dx0, dy0, dz0, TEST_DUNG_SX, TEST_DUNG_SY, TEST_DUNG_SZ},
                      {ex0, ey0, ez0, TEST_WELL_SX, TEST_WELL_SY, TEST_WELL_SZ},
                      {TEST_GRID_X0, TEST_SECTION_Y, TEST_GRID_Z0, TEST_GRID_X1 - TEST_GRID_X0 + 1, 2,
                       TEST_GRID_Z1 - TEST_GRID_Z0 + 1}};
      /* bottom and top each (the cache is 24 rows and the temple's box is 28): cache
       * it, tick it (fluids!), and every cached cell reads back what was stamped */
      /* bottom and top each: the cache's windows must cover the box between them */
      int wy0[4][2];
      for (int q = 0; q < 4; q++)
        for (int e = 0; e < 2; e++) {
          int cx = bx[q][0] + bx[q][3] / 2, cz = bx[q][2] + bx[q][5] / 2;
          int cy = e ? bx[q][1] + bx[q][4] - 6 : bx[q][1] + 6;
          world_follow((float)cx + 0.5f, (float)cy, (float)cz + 0.5f);
          while (world_pending()) world_follow((float)cx + 0.5f, (float)cy, (float)cz + 0.5f);
          wy0[q][e] = vc_y0;
          for (int t = 0; t < 100; t++) ticks_run++, world_tick();
          for (int dz = (bx[q][2] >> 4); dz <= ((bx[q][2] + bx[q][5] - 1) >> 4); dz++)
            for (int dx = (bx[q][0] >> 4); dx <= ((bx[q][0] + bx[q][3] - 1) >> 4); dx++) {
              TEST_SLAB(dx, dz, sb);
              for (int lz = 0; lz < 16; lz++)
                for (int lx = 0; lx < 16; lx++) {
                  int x = dx * 16 + lx, z = dz * 16 + lz;
                  if (x < bx[q][0] || x >= bx[q][0] + bx[q][3] || z < bx[q][2] || z >= bx[q][2] + bx[q][5]) continue;
                  for (int y = bx[q][1]; y < bx[q][1] + bx[q][4]; y++) {
                    if (!world_loaded(x, y, z) || world_pending_at(x, y, z)) continue;
                    int w = world_get(x, y, z), g = TEST_CELL(sb, y, lx, lz);
                    if (w != g) printf("  drift box %d at %d %d %d: world %d, stamped %d\n", q, x, y, z, w, g);
                    CHECK(w == g);
                  }
                }
            }
        }
      for (int q = 0; q < 4; q++)
        CHECK(wy0[q][0] <= bx[q][1] && wy0[q][1] + VCY >= bx[q][1] + bx[q][4] &&
              wy0[q][1] <= wy0[q][0] + VCY);
    }
    /* the weather never starts, and the fire does not spread or burn out */
    CHECK(!weather.raining && !weather.thundering);
    extern uint32_t game_time;
    game_time = 6000;   /* the showcase's sun: noon, and standing still */
    world_follow(sx + 0.5f, (float)sy, sz + 0.5f);
    while (world_pending()) world_follow(sx + 0.5f, (float)sy, sz + 0.5f);
    CHECK(light_at(VC_I(sx - vc_x0, sy + 1 - vc_y0, sz - vc_z0)) == 15);   /* the platform, in the open sky */
    world_set(sx + 1, sy + 1, sz, B_FIRE);
    world_set(sx + 1, sy, sz, B_PLANKS_OAK);
    world_set(sx + 1, sy, sz + 1, B_SAPLING_OAK);
    for (int t = 0; t < 200; t++) ticks_run++, world_tick();
    CHECK(world_get(sx + 1, sy + 1, sz) == B_FIRE);
    CHECK(world_get(sx + 1, sy, sz) == B_PLANKS_OAK);
    CHECK(world_get(sx + 1, sy, sz + 1) == B_SAPLING_OAK);
    /* and the world comes back the same out of a save, with its cages filled again */
    save_world();
    CHECK(load_world(6) && world_type == WT_TEST && world_info(6, &twi) && twi.type == WT_TEST);
    gen_spawn(&sx, &sy, &sz);
    CHECK(sx == (TEST_SPAWN_X0 + TEST_SPAWN_X1) / 2 && sy == TEST_PLATFORM_Y + 1);
    world_follow(sx + 0.5f, (float)sy, sz + 0.5f);
    while (world_pending()) world_follow(sx + 0.5f, (float)sy, sz + 0.5f);
    CHECK(world_get(sx, sy - 1, sz) == B_BEDROCK && world_get(sx, sy, sz) == B_AIR);
    mobs_spawn();
    mobs = 0;
    for (int i = 0; i < N_ENT; i++)
      if (is_mob(ents[i].type)) mobs++;
    CHECK(mobs == 10);
    delete_world(6);
    world_type = WT_DEFAULT;
#undef TEST_SLAB
#undef TEST_CELL
  }
  {
    /* the copy in numblocks_saves.py: every save, and after an install (only scripts left) they come back */
    for (int s = 1; s <= MAX_WORLDS; s++) delete_world(s);
    new_world(3, 42, 0, false, WT_DEFAULT, "Kept", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    for (int k = 0; k < 40; k++) world_set((int)pl.x + k, 70, (int)pl.z + 200, B_GOLD_BLOCK);   /* (another region too) */
    for (int k = 0; k < 40; k++) world_set((int)pl.x + k, 70, (int)pl.z, B_DIAMOND_BLOCK);
    save_world();
    char name[40];
    static uint8_t before[8][4096];
    static char names[8][40];
    uint32_t lens[8];
    int n = 0;
    for (int i = 0; plat_record(i, name, sizeof name) && n < 8; i++) {
      uint32_t len;
      const uint8_t *d = plat_load(name, &len);
      if (!d || (strncmp(name, "nb3", 3) && strcmp(name, "numblocks.cfg"))) continue;
      if (len > sizeof before[0]) continue;
      memcpy(before[n], d, len), lens[n] = len, strcpy(names[n], name), n++;
    }
    CHECK(n >= 3);   /* options, the world, its regions */
    uint32_t cl;
    CHECK(plat_load("numblocks_saves.py", &cl) && cl > 100);
    /* the install: every save gone, the script left */
    for (int i = 0; i < n; i++) plat_remove_prefix(names[i]);
    copy_restore();
    for (int i = 0; i < n; i++) {
      uint32_t len;
      const uint8_t *d = plat_load(names[i], &len);
      CHECK(d && len == lens[i] && !memcmp(d, before[i], len));
    }
    /* inside NumPlay, NumPlay keeps the copy */
    extern bool host_launcher;
    plat_remove_prefix("numblocks_saves.py");
    host_launcher = true;
    save_world();
    CHECK(!plat_load("numblocks_saves.py", &cl));
    host_launcher = false;
    delete_world(3);
    plat_remove_prefix("numblocks_saves.py");
  }
  /* sneaking along a one-block bridge over the void: it walks to the end and stops there (bridging);
   * not sneaking, it walks off */
  for (int sneak = 1; sneak >= 0; sneak--) {
    world_new(0, "ut4");
    world_follow(8, 70, 8);
    int y = 70;
    for (int z = 0; z <= 20; z++)
      for (int x = 0; x <= 16; x++)
        for (int k = -6; k < 4; k++) world_set(x, y + k, z, B_AIR);
    for (int z = 8; z <= 12; z++) world_set(8, y - 1, z, B_COBBLESTONE);
    memset(ents, 0, sizeof ents);
    pl.mode = 0, pl.dead = false, pl.health = 20, pl.flying = false;
    pl.x = 8.5f, pl.y = (float)y, pl.z = 8.5f, pl.vx = pl.vy = pl.vz = 0, pl.yaw = 0, pl.pitch = 0;
    pl.on_ground = true;
    uint32_t k = K_FWD | (sneak ? K_SNEAK : 0);
    for (int t = 0; t < 100; t++) player_tick(k, t ? 0 : k);
    if (sneak) CHECK(pl.y == (float)y && pl.z > 12.9f && pl.z < 13.31f);
    else CHECK(pl.y < (float)y - 1);
    plat_remove_prefix("ut4");
  }
  /* a spawn egg makes its mob against the block looked at (ItemMonsterPlacer.onItemUse) */
  {
    world_new(0, "ut5");
    world_follow(8, 70, 8);
    int y = 70;
    for (int z = 0; z <= 16; z++)
      for (int x = 0; x <= 16; x++) {
        world_set(x, y - 1, z, B_STONE);
        for (int k = 0; k < 4; k++) world_set(x, y + k, z, B_AIR);
      }
    memset(ents, 0, sizeof ents);
    opt.difficulty = 0, pl.mode = 0, pl.dead = false, pl.health = 20;
    pl.x = 8.5f, pl.y = (float)y, pl.z = 8.5f, pl.vx = pl.vy = pl.vz = 0, pl.slot = 0;
    static const int eggs[10] = {I_ZOMBIE_SPAWN_EGG, I_SKELETON_SPAWN_EGG, I_CREEPER_SPAWN_EGG,
                               I_SPIDER_SPAWN_EGG, I_PIG_SPAWN_EGG, I_COW_SPAWN_EGG, I_SHEEP_SPAWN_EGG,
                               I_CHICKEN_SPAWN_EGG, I_PIGLIN_SPAWN_EGG, I_ZOMBIFIED_PIGLIN_SPAWN_EGG};
    static const int mobs[10] = {E_ZOMBIE, E_SKELETON, E_CREEPER, E_SPIDER, E_PIG, E_COW, E_SHEEP, E_CHICKEN,
                                 E_PIGLIN, E_ZPIGLIN};
    for (int i = 0; i < 10; i++) {
      CHECK(mob_of_egg(eggs[i]) == mobs[i]);
      CHECK(egg_of_mob(mobs[i]) == eggs[i]);
      CHECK(item_label(eggs[i])[0] && item_kind(eggs[i]) == IK_SPAWN_EGG);
      CHECK(item_max(eggs[i]) == 64 && item_dur(eggs[i]) == 0 && item_icon(eggs[i]) >= 0);
      /* used on the ground two blocks ahead, in survival: the egg is spent, the mob is there */
      pl.inv[0].id = (uint16_t)eggs[i], pl.inv[0].aux = 3;
      player_look(pl.x, pl.y + 1.62f, pl.z, 0, 60);
      CHECK(pl.hit_face == 1);
      player_tick(K_USE, K_USE);
      int n = 0;
      for (int e = 0; e < N_ENT; e++)
        if (ents[e].type == mobs[i]) n++;
      CHECK(n == 1 && pl.inv[0].aux == 2);
      memset(ents, 0, sizeof ents);
      pl.inv[0].id = 0, pl.inv[0].aux = 0;
    }
    CHECK(mob_of_egg(B_DIRT) == 0 && mob_of_egg(0) == 0 && mob_of_egg(I_END) == 0);
    CHECK(mob_of_egg(I_QUARTZ) == 0 && egg_of_mob(0) == 0);   /* (QUARTZ sits between the eggs: not one) */
    /* nothing to look at: nothing spawns (creative: the egg stays) */
    pl.mode = 1;
    pl.inv[0].id = I_CREEPER_SPAWN_EGG, pl.inv[0].aux = 1;
    player_look(pl.x, pl.y + 1.62f, pl.z, 0, -80);   /* the sky */
    player_tick(K_USE, K_USE);
    CHECK(pl.inv[0].id == I_CREEPER_SPAWN_EGG);
    plat_remove_prefix("ut5");
  }
  /* the world's caps: N_ENT entity slots, shared by mobs, items, arrows and TNT, and
   * N_PART particles. Every slot a mob: nothing more may be made. Full of items, one
   * more gives way to the oldest item's slot (a big dig never locks the world out) */
  {
    new_world(2, 5, 0, false, WT_DEFAULT, "Caps", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int px = (int)floorf(pl.x), py = (int)floorf(pl.y), pz = (int)floorf(pl.z);
    memset(ents, 0, sizeof ents);
    /* every slot a mob: the next is turned away (and there is no item to give way) */
    for (int i = 0; i < N_ENT; i++)
      CHECK(ent_new(E_ZOMBIE, px + 0.5f + i * 0.1f, (float)py, pz + 0.5f) != NULL);
    CHECK(ent_new(E_SKELETON, px + 0.5f, (float)py, pz + 0.5f) == NULL);
    /* full of items, aged: the oldest one's slot is the one that gives way */
    memset(ents, 0, sizeof ents);
    for (int i = 0; i < N_ENT; i++) CHECK(ent_drop(B_DIRT, 1, 0, px + 0.5f, py + 0.5f, pz + 0.5f, false) != NULL);
    for (int i = 0; i < N_ENT; i++) ents[i].age = (int16_t)(N_ENT - i);   /* (the first made is the oldest) */
    int oldest = 0;
    for (int i = 1; i < N_ENT; i++)
      if (ents[i].age > ents[oldest].age) oldest = i;
    CHECK(ent_drop(B_STONE, 1, 0, px + 0.5f, py + 0.5f, pz + 0.5f, false) != NULL);
    CHECK(ents[oldest].item.id == B_STONE && ents[oldest].age == 0);   /* (its slot, reset) */
    int items = 0;
    for (int i = 0; i < N_ENT; i++) items += ents[i].type == E_ITEM;
    CHECK(items == N_ENT);
    /* a mob takes an item's slot too: the drops never keep the mobs out for good */
    int pigs = 0;
    for (int i = 0; i < N_ENT; i++) pigs += ents[i].type == E_PIG;
    CHECK(pigs == 0);
    CHECK(ent_new(E_PIG, px + 0.5f, (float)py, pz + 0.5f) != NULL);
    for (int i = 0; i < N_ENT; i++) pigs += ents[i].type == E_PIG;
    CHECK(pigs == 1);
    /* particles: N_PART at a time (a free one, or else any) */
    memset(ents, 0, sizeof ents);
    memset(parts, 0, sizeof parts);
    for (int i = 0; i < N_PART * 2; i++) {
      particle_add(px + 0.5f, py + 1.0f, pz + 0.5f, 0, 0.1f, 0, 0xFFFF, 20);
      int live = 0;
      for (int k = 0; k < N_PART; k++) live += parts[k].age < (parts[k].life & P_LIFE);
      CHECK(live == (i < N_PART ? i + 1 : N_PART));
    }
    plat_remove_prefix("ut5");
    delete_world(2);
  }
  /* a spawn egg used on a monster spawner sets it, and it makes that mob (MobSpawner) */
  {
    extern uint32_t game_time;
    new_world(2, 5, 0, false, WT_DEFAULT, "Spawners", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int y = (int)floorf(pl.y), px = (int)floorf(pl.x), pz = (int)floorf(pl.z);
    /* a grass patch, one spawner on it, and the player standing on it looking at it */
    for (int z = pz - 4; z <= pz + 8; z++)
      for (int x = px - 4; x <= px + 8; x++) {
        bool grass = x >= px - 2 && x <= px + 2 && z >= pz - 2 && z <= pz + 3;
        world_set(x, y - 1, z, grass ? B_GRASS : B_STONE);
        for (int k = 0; k < 3; k++) world_set(x, y + k, z, B_AIR);
      }
    world_set(px, y, pz + 1, B_MOB_SPAWNER);
    memset(ents, 0, sizeof ents);
    pl.x = px + 0.5f, pl.y = (float)y, pl.z = pz + 0.5f, pl.mode = 0, pl.dead = false, pl.slot = 0;
    /* the cow egg on the spawner: it is set to cows (and the egg is spent) */
    pl.inv[0] = (Stack){I_COW_SPAWN_EGG, 5};
    spawners_clear();
    player_look(pl.x, pl.y + 1.62f, pl.z, 0, 70);
    CHECK(pl.hit_face >= 0 && world_get(pl.hit_x, pl.hit_y, pl.hit_z) == B_MOB_SPAWNER);
    player_tick(K_USE, K_USE);
    CHECK(pl.inv[0].aux == 4 && spawners_save(NULL) == 10 * 1 + 1);
    /* the table saves and loads: a count, then each as x, z, y and its mob */
    uint8_t buf[512];
    int len = spawners_save(buf);
    spawners_clear();
    spawners_load(buf, len);
    CHECK(spawners_save(NULL) == len);
    CHECK(spawners_count() == 1);
    int gx, gy, gz;
    CHECK(spawners_get(0, &gx, &gy, &gz) == E_COW && spawners_get(1, &gx, &gy, &gz) == -1);
    /* an unset spawner is empty: no egg, and nothing out of it until an egg is used on it */
    CHECK(spawner_mob_at(px + 40, y, pz + 40) == 0);
    CHECK(spawner_mob_at(px, y, pz + 1) == E_COW);   /* (the set one: what it is set to) */
    /* noting a fresh spawner does not overwrite a set one */
    spawners_note(px, y, pz + 1);
    CHECK(spawner_mob_at(px, y, pz + 1) == E_COW && spawners_count() == 1);
    spawners_note(px + 60, y, pz);
    CHECK(spawners_count() == 2 && spawner_mob_at(px + 60, y, pz) == 0);
    /* full: an unset one gives way first, so a set one keeps its mob */
    spawners_clear();
    for (int i = 0; i < 30; i++) spawners_note(1000 + i, 10, 1000);
    CHECK(spawners_count() == 24);
    spawners_set(2000, 10, 2000, E_ZOMBIE);
    CHECK(spawners_count() == 24 && spawner_mob_at(2000, 10, 2000) == E_ZOMBIE);
    spawners_set(2001, 10, 2000, E_SKELETON);
    CHECK(spawner_mob_at(2000, 10, 2000) == E_ZOMBIE && spawner_mob_at(2001, 10, 2000) == E_SKELETON);
    /* every mob survives the save and load */
    {
      uint8_t b2[512];
      spawners_clear();
      for (int i = 0; i < 10; i++) spawners_set(3000 + i * 2, 10, 3000, E_ZOMBIE + i);
      int l2 = spawners_save(b2);
      spawners_clear();
      spawners_load(b2, l2);
      for (int i = 0; i < 10; i++) CHECK(spawner_mob_at(3000 + i * 2, 10, 3000) == E_ZOMBIE + i);
      CHECK(spawners_count() == 10 && spawners_save(NULL) == l2);
    }
    spawners_load(buf, len);             /* (and a load puts the saved ones back) */
    CHECK(spawners_save(NULL) == len && spawners_count() == 1);
    /* it survives the world's record: the count is in the bytes after the tiles */
    save_world();
    spawners_clear();
    CHECK(load_world(2));
    CHECK(spawners_save(NULL) == len);
    /* by night it makes nothing more of them (an animal's light rules) */
    memset(ents, 0, sizeof ents);
    game_time = 18000;   /* (midnight) */
    for (int t = 0; t < 600; t++) pl.health = 20, pl.dead = false, ents_tick(), game_time++;
    int night = 0;
    for (int i = 0; i < N_ENT; i++)
      if (ents[i].type == E_COW) night++;
    CHECK(night <= 8);
    /* set again with the creeper egg, and by night it makes creepers (eight of each kind at most) */
    memset(ents, 0, sizeof ents);
    opt.difficulty = 2;   /* (spawners make monsters only on a world with monsters: an earlier test made it peaceful) */
    pl.inv[0] = (Stack){I_CREEPER_SPAWN_EGG, 5};
    player_look(pl.x, pl.y + 1.62f, pl.z, 0, 70);
    CHECK(pl.hit_face >= 0 && world_get(pl.hit_x, pl.hit_y, pl.hit_z) == B_MOB_SPAWNER);
    player_tick(K_USE, K_USE);
    CHECK(pl.inv[0].aux == 4 && spawners_save(NULL) == len);   /* (the same one, set to another mob) */
    game_time = 18000;
    /* (they wander off as the ticks go, so the most there have been is what counts) */
    int near = 0, mobs = 0;
    for (int t = 0; t < 4000; t++) {
      pl.health = 20, pl.dead = false, ents_tick(), game_time++;
      if (t % 20) continue;
      int c = 0, m = 0;
      for (int i = 0; i < N_ENT; i++) {
        if (is_mob(ents[i].type)) m++;
        if (ents[i].type == E_CREEPER && fabsf(ents[i].x - pl.x) < 7 && fabsf(ents[i].z - pl.z) < 7) c++;
      }
      if (c > near) near = c;
      if (m > mobs) mobs = m;
    }
    CHECK(near >= 1 && mobs <= 32);
    /* an unset spawner makes nothing, and a set one's mobs appear in the air about it and drop,
     * whatever the light (natural spawning keeps 14 blocks off the player and puts a mob on the
     * ground, and wants darkness for a monster; a zombie up in the air beside the spawner, in the
     * middle of the day, can only be a spawner's) */
    memset(ents, 0, sizeof ents);
    int sx = px + 3, sy = y + 4, sz = pz + 3;
    world_set(sx, sy, sz, B_MOB_SPAWNER);
    spawners_note(sx, sy, sz);
    game_time = 6000;   /* (the middle of the day: a lit spawner still makes its monsters) */
    int airborne = 0, alive = 0;
    for (int t = 0; t < 1200; t++) {
      pl.health = 20, pl.dead = false, ents_tick(), game_time++;
      for (int i = 0; i < N_PART; i++)
        if (parts[i].age < (parts[i].life & P_LIFE)) alive++;
      for (int i = 0; i < N_ENT; i++) {
        const Entity *e = &ents[i];
        if (e->type == E_ZOMBIE && e->y > (float)y && fabsf(e->x - (sx + 0.5f)) < 3.5f && fabsf(e->z - (sz + 0.5f)) < 3.5f)
          airborne++;
      }
    }
    CHECK(airborne == 0);   /* (nothing comes out of it while it is empty) */
    spawners_set(sx, sy, sz, E_ZOMBIE);
    airborne = alive = 0;
    for (int t = 0; t < 1200; t++) {
      pl.health = 20, pl.dead = false, ents_tick(), game_time++;
      for (int i = 0; i < N_PART; i++)
        if (parts[i].age < (parts[i].life & P_LIFE)) alive++;
      for (int i = 0; i < N_ENT; i++) {
        const Entity *e = &ents[i];
        if (e->type == E_ZOMBIE && e->y > (float)y && fabsf(e->x - (sx + 0.5f)) < 3.5f && fabsf(e->z - (sz + 0.5f)) < 3.5f)
          airborne++;
      }
    }
    CHECK(airborne > 0);   /* (they drop out of the air onto the grass below) */
    CHECK(alive > 4);      /* (and the puff that goes with each: smoke about the mob, flames about the spawner) */
    /* and no mob spawning at all with the rule off */
    game_rules &= ~(uint8_t)GR_MOB_SPAWNING;
    memset(ents, 0, sizeof ents);
    for (int t = 0; t < 600; t++) pl.health = 20, pl.dead = false, ents_tick(), game_time++;
    int any = 0;
    for (int i = 0; i < N_ENT; i++)
      if (ents[i].type != E_NONE) any++;
    CHECK(any == 0);
    game_rules = GR_DEFAULT;
    delete_world(2);
  }
  {
    /* a dungeon's spawner as world generation leaves it: placed, and set to the mob its own
     * draw made of it (spawners_made, WorldGenDungeons.a). Its first mob is out well within
     * a minute, and about three to the minute after it, where the one try in sixteen a
     * second it had made the first take a minute or two to come */
    extern uint32_t game_time;
    new_world(2, 5, 0, false, WT_DEFAULT, "DungeonSpawner", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int y = (int)floorf(pl.y), px = (int)floorf(pl.x), pz = (int)floorf(pl.z);
    for (int z = pz - 4; z <= pz + 8; z++)
      for (int x = px - 4; x <= px + 8; x++) {
        world_set(x, y - 1, z, B_GRASS);
        for (int k = 0; k < 6; k++) world_set(x, y + k, z, B_AIR);
      }
    int sx = px + 3, sy = y + 4, sz = pz + 3;
    world_set(sx, sy, sz, B_MOB_SPAWNER);
    spawners_clear();
    spawners_made(sx, sy, sz, 3);   /* a spider: the draw that makes one of the four */
    opt.difficulty = 2;             /* (spawners make monsters only on a world with monsters) */
    memset(ents, 0, sizeof ents);
    game_time = 6000;   /* (daylight on the grass: only a spawner's monsters, natural ones want the dark) */
    /* What the spawner itself makes (a slot that holds a spider and held none a tick
     * ago), rather than the spiders about it: a wanderer out of the window and back
     * moved that count by itself, and the world keeps six of a kind about one, so the
     * spiders are cleared away every couple of seconds and the count is the tries the
     * spawner won. A run of the world's shared random stream of the test's own makes the
     * count its own - any change about the world moves where that stream stands - and
     * the stream goes back afterwards. */
    uint32_t rng0 = rnd_seed(0x2545F491);
    uint8_t was[N_ENT];
    memset(was, 0, sizeof was);
    int made = 0, first_at = -1;
    for (int t = 0; t < 2400; t++) {
      pl.health = 20, pl.dead = false, ents_tick(), game_time++;
      for (int i = 0; i < N_ENT; i++) {
        bool now = ents[i].type == E_SPIDER;
        if (now && !was[i]) {
          made++;
          if (first_at < 0) first_at = t;
        }
        was[i] = (uint8_t)now;
      }
      if (t % 40 == 39)
        for (int i = 0; i < N_ENT; i++)
          if (ents[i].type == E_SPIDER) ents[i].type = E_NONE, was[i] = 0;
    }
    CHECK(first_at >= 0 && first_at < 900);   /* the first within three quarters of a minute of ticks */
    CHECK(made >= 12);                        /* seventeen in the two minutes: about one try in six a
                                               * second won, where the one try in sixteen a second it had
                                               * won six, and a wanderer in and out is not counted now */
    rnd_seed(rng0);
    delete_world(2);
  }
  {
    /* the nether: netherrack, a lava sea at y <= 31, a rough bedrock floor and ceiling,
     * one biome (1.8's Hell, 8) and no light of the sky's */
    world_new(7, "utn");
    world_switch_dim(DIM_NETHER);
    world_follow(8, 40, 8);   /* (the window covers the lava sea and its shore) */
    while (world_pending()) world_follow(8, 40, 8);
    CHECK(world_dim == DIM_NETHER);
    int nb0 = gen_biome(8, 8), nb1 = gen_biome(-500, 900);
    CHECK(nb0 == BI_NETHER_WASTES || nb0 == BI_CRIMSON_FOREST || nb0 == BI_WARPED_FOREST ||
          nb0 == BI_SOUL_SAND_VALLEY);
    CHECK(nb1 == BI_NETHER_WASTES || nb1 == BI_CRIMSON_FOREST || nb1 == BI_WARPED_FOREST ||
          nb1 == BI_SOUL_SAND_VALLEY);
    int nrack = 0, nlava = 0, nair = 0, nbed = 0, nsoul = 0, ngrav = 0, nother = 0;
    int nquartz = 0, nglow = 0, nnew = 0;
    int ylo = vc_y0 > 0 ? vc_y0 : 0, yhi = vc_y0 + VCY < 32 ? vc_y0 + VCY : 32;   /* the rows it holds */
    CHECK(yhi > ylo);
    for (int z = 0; z < VCZ; z++)
      for (int x = 0; x < VCX; x++)
        for (int y = ylo; y < yhi; y++) {
          int b = world_get(vc_x0 + x, y, vc_z0 + z);
          if (b == B_NETHERRACK) nrack++;
          else if (b == B_LAVA) nlava++;
          else if (b == B_AIR) nair++;
          else if (b == B_BEDROCK) nbed++;
          else if (b == B_SOUL_SAND) nsoul++;
          else if (b == B_GRAVEL) ngrav++;
          else if (b == B_NETHER_QUARTZ_ORE) nquartz++;
          else if (b == B_GLOWSTONE) nglow++;
          else if (b >= B_CRIMSON_NYLIUM && b <= B_SOUL_FIRE) nnew++;   /* the biomes' (1.16) */
          else nother++;
        }
    CHECK(nother == 0 && nrack > 1000);   /* netherrack, and the patches and lava near the sea */
    CHECK(nlava > 100);
    CHECK(nair == 0);   /* the sea fills everything the density leaves at y <= 31 */
    /* the nether's light runs eight brighter: no block light below 8 anywhere */
    {
      int minlb = 15;
      for (int z = 0; z < VCZ; z++)
        for (int x = 0; x < VCX; x++)
          for (int y = 0; y < VCY; y++) {
            int lb = vl[VC_I(x, y, z)] >> 4;
            if (lb < minlb) minlb = lb;
          }
      CHECK(minlb >= 8);
    }
    /* glowstone hangs off the ceiling: some in a high window */
    world_follow(8, 108, 8);
    while (world_pending()) world_follow(8, 108, 8);
    {
      int nglow = 0;
      for (int z = 0; z < VCZ; z++)
        for (int x = 0; x < VCX; x++)
          for (int y = vc_y0; y < vc_y0 + VCY; y++)
            if (world_get(vc_x0 + x, y, vc_z0 + z) == B_GLOWSTONE) nglow++;
      CHECK(nglow > 0);
    }
    /* the ceiling: bedrock within a few blocks of 127 */
    {
      uint8_t gs[GEN_SCRATCH];   /* (gen_top away from the last chunk's runs the generator) */
      memset(gs, 0, sizeof gs);
      gen_scratch(gs);
      for (int k = 0; k < 40; k++) {
        int t = gen_top(k * 7 - 100, k * 13 - 60);
        CHECK(t >= 123 && t <= 127);
      }
      gen_scratch(NULL);
    }
    /* and back: the overworld's own terrain again (the same seed, the same world) */
    world_switch_dim(DIM_OVER);
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    int obio = vbiome[(8 - vc_z0) * VCX + 8 - vc_x0];
    CHECK(obio != 8);   /* (the overworld's biome, as the cache has it) */
    int over = 0, back = 0;
    for (int z = 0; z < VCZ; z++)
      for (int x = 0; x < VCX; x++)
        for (int y = vc_y0; y < vc_y0 + VCY; y++) {
          int b = world_get(vc_x0 + x, y, vc_z0 + z);
          if (b == B_STONE || b == B_WATER || b == B_GRASS || b == B_DIRT || b == B_SAND || b == B_GRAVEL) over++;
          if (b == B_NETHERRACK) back++;
        }
    CHECK(over > 100 && back == 0);   /* (a 1.8 world has ground in it; no netherrack) */
    plat_remove_prefix("utn");
  }
  {
    /* ---- the nether's biomes (todo [22]): the crimson and warped forests and the
     * soul sand valley beside the wastes --------------------------------------------- */
    world_new(22, "utnb");
    world_switch_dim(DIM_NETHER);
    uint8_t bs[GEN_SCRATCH];
    memset(bs, 0, sizeof bs);
    gen_scratch(bs);
    /* the four of them in a 49 x 49 chunk window, and the split is a pure function
     * of the column (the same answer for the same place every time) */
    int saw = 0;
    for (int cz = -24; cz <= 24; cz++)
      for (int cx = -24; cx <= 24; cx++) {
        int b = gen_biome(cx * 16 + 8, cz * 16 + 8);
      CHECK(b == BI_NETHER_WASTES || b == BI_CRIMSON_FOREST || b == BI_WARPED_FOREST ||
            b == BI_SOUL_SAND_VALLEY);
      saw |= 1 << (b == BI_NETHER_WASTES ? 0 : b == BI_CRIMSON_FOREST ? 1 : b == BI_WARPED_FOREST ? 2 : 3);
    }
    CHECK(saw == 15);
    /* the mix over a wide area: the wastes a little over half, the other three a
     * sixth each (three cells of each in every twenty of the split's macros) */
    {
      int n[4] = {0, 0, 0, 0};
      for (int cz = -200; cz <= 200; cz++)
        for (int cx = -200; cx <= 200; cx++) {
          int b = gen_biome(cx * 64 + 32, cz * 64 + 32);
          n[b == BI_NETHER_WASTES ? 0 : b == BI_CRIMSON_FOREST ? 1 : b == BI_WARPED_FOREST ? 2 : 3]++;
        }
      int tot = n[0] + n[1] + n[2] + n[3];
      CHECK(n[0] * 100 > tot * 50 && n[0] * 100 < tot * 60);
      for (int k = 1; k < 4; k++) CHECK(n[k] * 100 > tot * 12 && n[k] * 100 < tot * 18);
    }
    /* the sky and the fog fade across a biome's edge instead of stepping: the
     * colour thirty blocks out is the biome's own, on the edge half of each, and
     * it moves monotonically between them. The walk crosses one clean edge (both
     * cells' other neighbours the same biome, so no second edge leans in). */
    {
      int ex = 0, ez = 0, found = 0;
      for (int cz = -80; cz <= 80 && !found; cz++)
        for (int cx = -80; cx <= 80 && !found; cx++) {
          int w0 = gen_biome(cx * 64 + 32, cz * 64 + 32);
          if (w0 == gen_biome((cx + 1) * 64 + 32, cz * 64 + 32)) continue;
          if (gen_biome(cx * 64 + 32, (cz - 1) * 64 + 32) != w0) continue;
          if (gen_biome(cx * 64 + 32, (cz + 1) * 64 + 32) != w0) continue;
          int w1 = gen_biome((cx + 1) * 64 + 32, cz * 64 + 32);
          if (gen_biome((cx + 1) * 64 + 32, (cz - 1) * 64 + 32) != w1) continue;
          if (gen_biome((cx + 1) * 64 + 32, (cz + 1) * 64 + 32) != w1) continue;
          ex = (cx + 1) * 64, ez = cz * 64 + 32, found = 1;
        }
      CHECK(found);
      int r0, g0, b0, r1, g1, b1, re, ge, be;
      gen_nether_sky_colour((float)(ex - 30), (float)ez, &r0, &g0, &b0);
      gen_nether_sky_colour((float)(ex + 30), (float)ez, &r1, &g1, &b1);
      gen_nether_sky_colour((float)ex, (float)ez, &re, &ge, &be);
      CHECK(re == (r0 + r1 + 1) / 2 && ge == (g0 + g1 + 1) / 2 && be == (b0 + b1 + 1) / 2);
      int pr = r0, pg = g0, pb = b0, seen = 0;
      for (int x = ex - 30; x <= ex + 30; x++) {
        int r, g, b;
        gen_nether_sky_colour((float)x, (float)ez, &r, &g, &b);
        if (r != pr || g != pg || b != pb) seen = 1;   /* (it moves at all) */
        /* no step back away from this side's colour (a snap would jump) */
        CHECK(abs(r - r1) + abs(g - g1) + abs(b - b1) <= abs(pr - r1) + abs(pg - g1) + abs(pb - b1) + 3);
        pr = r, pg = g, pb = b;
      }
      CHECK(seen);
      CHECK(pr == r1 && pg == g1 && pb == b1);
    }
    int stable_biome = gen_biome(1234, -5678);
    {
      static uint8_t other_window[24 * 288];
      gen_slab(3, -7, 72, 24, other_window);
    }
    CHECK(gen_biome(1234, -5678) == stable_biome);
    CHECK(gen_biome_known(BI_CRIMSON_FOREST) && gen_biome_known(BI_WARPED_FOREST) &&
          gen_biome_known(BI_SOUL_SAND_VALLEY));
    CHECK(!strcmp(gen_biome_name(BI_CRIMSON_FOREST), "Crimson Forest") &&
          !strcmp(gen_biome_name(BI_WARPED_FOREST), "Warped Forest") &&
          !strcmp(gen_biome_name(BI_SOUL_SAND_VALLEY), "Soul Sand Valley"));
    CHECK(gen_biome_id("crimson_forest") == BI_CRIMSON_FOREST &&
          gen_biome_id("warped_forest") == BI_WARPED_FOREST &&
          gen_biome_id("soul_sand_valley") == BI_SOUL_SAND_VALLEY);
    CHECK(gen_biome_id("nether_wastes") == BI_NETHER_WASTES && gen_biome_id("hell") == BI_NETHER_WASTES &&
          !strcmp(gen_biome_name(BI_NETHER_WASTES), "Nether Wastes"));
    /* the same chunk out of two sliding windows (40..63 and 52..75): the twelve
     * rows they share are the same, block for block (9 bits a cell: 288 bytes a row) */
    {
      static uint8_t wa[24 * 288], wb[24 * 288];
      gen_slab(0, 0, 40, 24, wa);
      gen_slab(0, 0, 52, 24, wb);
      CHECK(!memcmp(wa + 12 * 288, wb, 12 * 288));
    }
    /* each biome's surface and growth, out of its own chunk's window: a forest has
     * nylium and something growing on it, the valley soul ground (and its bone
     * blocks where the ground has them), the wastes their red and brown mushrooms.
     * Three cache windows cover the ground. */
    {
      int f_found = 0, v_found = 0, m_found = 0, f_ok = 0, v_ok = 0, m_ok = 0;
      for (int cx = -24; cx <= 24 && !(f_found && v_found && m_found); cx++)
        for (int cz = -24; cz <= 24 && !(f_found && v_found && m_found); cz++) {
          int bio = gen_biome(cx * 16 + 8, cz * 16 + 8);
          if (bio != BI_NETHER_WASTES && bio != BI_CRIMSON_FOREST && bio != BI_WARPED_FOREST &&
              bio != BI_SOUL_SAND_VALLEY)
            continue;
          int nyl = 0, growth = 0, soul = 0, shroom = 0;
          for (int wy = 32; wy <= 120; wy += 24) {
            world_follow((float)(cx * 16 + 8), (float)wy, (float)(cz * 16 + 8));
            while (world_pending()) world_follow((float)(cx * 16 + 8), (float)wy, (float)(cz * 16 + 8));
            for (int z = 0; z < VCZ; z++)
              for (int x = 0; x < VCX; x++)
                for (int y = 0; y < VCY; y++) {
                  int b = world_get(vc_x0 + x, vc_y0 + y, vc_z0 + z);
                  if (b == B_CRIMSON_NYLIUM || b == B_WARPED_NYLIUM) nyl++;
                  if (b == B_CRIMSON_FUNGUS || b == B_WARPED_FUNGUS || b == B_CRIMSON_ROOTS ||
                      b == B_WARPED_ROOTS || b == B_NETHER_SPROUTS || b == B_WEEPING_VINES ||
                      b == B_TWISTING_VINES || b == B_NETHER_WART_BLOCK || b == B_WARPED_WART_BLOCK ||
                      b == B_SHROOMLIGHT || b == B_SOUL_FIRE)
                    growth++;
                  if (b == B_SOUL_SAND || b == B_SOUL_SOIL) soul++;
                  if (b == B_RED_MUSHROOM || b == B_BROWN_MUSHROOM) shroom++;
                }
          }
          if ((bio == BI_CRIMSON_FOREST || bio == BI_WARPED_FOREST) && !f_found) {
            f_found = 1;
            CHECK(nyl > 0 && growth > 0);
            f_ok = 1;
          } else if (bio == BI_SOUL_SAND_VALLEY && !v_found) {
            v_found = 1;
            CHECK(soul > 0);
            v_ok = 1;
          } else if (bio == BI_NETHER_WASTES && !m_found) {
            m_found = 1;
            CHECK(shroom > 0);
            m_ok = 1;
          }
        }
      CHECK(f_found && v_found && m_found && f_ok && v_ok && m_ok);
    }
    /* a biome is that biome all the way through: every netherrack the terrain left
     * open above the lava sea is the biome's own block - nylium in the two forests,
     * soul sand in the valley - instead of pockets of it in netherrack (the ledge
     * the highest open face happened to sit on was the trap: the floor the player
     * walks on stayed netherrack) */
    {
      static uint8_t slab[128 * 288];
      int chunks = 0, rack = 0, waste_chunks = 0, waste_rack = 0, soulshroom = 0;
      for (int cz = -24; cz <= 24; cz++)
        for (int cx = -24; cx <= 24; cx++) {
          int bio = gen_biome(cx * 16 + 8, cz * 16 + 8);
          if (bio != BI_CRIMSON_FOREST && bio != BI_WARPED_FOREST && bio != BI_SOUL_SAND_VALLEY) continue;
          chunks++;
          gen_scratch(bs);   /* (the world's follow released the scratch it borrowed) */
          gen_slab(cx, cz, 0, 128, slab);
          for (int z = 0; z < 16; z++)
            for (int x = 0; x < 16; x++)
              for (int y = 121; y >= 32; y--) {
                int b = b9_get(slab, y * 256 + z * 16 + x);
                if (b == B_NETHERRACK && b9_get(slab, (y + 1) * 256 + z * 16 + x) == B_AIR) rack++;
                /* mushrooms never stand on the valley's ground: they grow in
                 * the forests and the wastes, on nylium and netherrack */
                if (b == B_RED_MUSHROOM || b == B_BROWN_MUSHROOM) {
                  int below = b9_get(slab, (y - 1) * 256 + z * 16 + x);
                  if (below == B_SOUL_SAND || below == B_SOUL_SOIL || below == B_BONE_BLOCK) soulshroom++;
                }
              }
        }
      CHECK(chunks > 0);
      CHECK(rack == 0);
      CHECK(soulshroom == 0);
      /* the wastes keep theirs: no biome, no conversion */
      for (int cz = -24; cz <= 24 && !waste_chunks; cz++)
        for (int cx = -24; cx <= 24 && !waste_chunks; cx++) {
          if (gen_biome(cx * 16 + 8, cz * 16 + 8) != BI_NETHER_WASTES) continue;
          waste_chunks = 1;
          gen_scratch(bs);
          gen_slab(cx, cz, 0, 128, slab);
          for (int z = 0; z < 16; z++)
            for (int x = 0; x < 16; x++)
              for (int y = 121; y >= 32; y--) {
                int b = b9_get(slab, y * 256 + z * 16 + x);
                if (b == B_NETHERRACK && b9_get(slab, (y + 1) * 256 + z * 16 + x) == B_AIR) waste_rack++;
                if (b == B_RED_MUSHROOM || b == B_BROWN_MUSHROOM) {
                  int below = b9_get(slab, (y - 1) * 256 + z * 16 + x);
                  if (below == B_SOUL_SAND || below == B_SOUL_SOIL || below == B_BONE_BLOCK) soulshroom++;
                }
              }
        }
      CHECK(waste_chunks == 1 && waste_rack > 0);
      CHECK(soulshroom == 0);
    }
    /* the same growth whichever window asked: the chunk's whole column against a
     * higher window, the rows they share identical (the glowstone lesson of 1.5.4) */
    {
      int ox = -1000, oz = -1000;
      for (int cx = -24; cx <= 24 && ox < -500; cx++)
        for (int cz = -24; cz <= 24 && ox < -500; cz++)
          if (gen_biome(cx * 16 + 8, cz * 16 + 8) == BI_WARPED_FOREST) ox = cx, oz = cz;
      CHECK(ox >= -24 && ox <= 24);   /* (the window above found one) */
      static uint8_t wa[128 * 288], wb[64 * 288];
      gen_scratch(bs);   /* (world_follow released the scratch it borrowed) */
      gen_slab(ox, oz, 0, 128, wa);
      gen_slab(ox, oz, 64, 64, wb);
      CHECK(!memcmp(wa + 64 * 288, wb, 64 * 288));
    }
    gen_scratch(NULL);
    plat_remove_prefix("utnb");
  }
  {
    /* soul fire (1.16): fire on soul sand or soul soil is the blue kind, nowhere
     * else, and it goes out when its soul ground is gone; the new blocks place,
     * break and drop as their own */
    world_new(24, "utsoul");
    world_switch_dim(DIM_NETHER);
    world_follow(8, 60, 8);
    while (world_pending()) world_follow(8, 60, 8);
    memset(&weather, 0, sizeof weather);
    rain_str = 0;
    game_rules = GR_DEFAULT;
    memset(&pl, 0, sizeof pl);
    for (int z = 0; z < 16; z++)
      for (int x = 0; x < 16; x++)
        for (int y = 60; y < 70; y++) world_set(x, y, z, y == 60 ? B_NETHERRACK : B_AIR);
    world_set(4, 61, 4, B_SOUL_SAND);
    world_set(6, 61, 4, B_SOUL_SOIL);
    fire_set(4, 62, 4, 0);
    CHECK(world_get(4, 62, 4) == B_SOUL_FIRE);
    fire_set(6, 62, 4, 0);
    CHECK(world_get(6, 62, 4) == B_SOUL_FIRE);
    fire_set(8, 61, 4, 0);
    CHECK(world_get(8, 61, 4) == B_FIRE);
    for (int t = 0; t < 120; t++) ticks_run++, world_tick();
    CHECK(world_get(4, 62, 4) == B_SOUL_FIRE && world_get(6, 62, 4) == B_SOUL_FIRE);
    /* the soul ground removed, the soul fire goes out (the next update) */
    world_set(4, 61, 4, B_AIR);
    world_set(6, 61, 4, B_AIR);
    for (int t = 0; t < 120 && (world_get(4, 62, 4) != B_AIR || world_get(6, 62, 4) != B_AIR); t++)
      ticks_run++, world_tick();
    CHECK(world_get(4, 62, 4) == B_AIR && world_get(6, 62, 4) == B_AIR);
    /* place, break, drop: the states are their own items and the vines need shears */
    memset(pl.inv, 0, sizeof pl.inv);
    CHECK(blk_item[B_CRIMSON_PLANKS] == B_CRIMSON_PLANKS && blk_item[B_SOUL_SOIL] == B_SOUL_SOIL);
    CHECK(blk_item[B_CRIMSON_STEM_X] == B_CRIMSON_STEM && blk_item[B_STRIPPED_WARPED_STEM_Z] == B_STRIPPED_WARPED_STEM);
    CHECK(blk_model[B_SOUL_FIRE] == M_CROSS && blk_light[B_SHROOMLIGHT] == 15 && blk_light[B_SOUL_FIRE] == 10);
    CHECK(blk_hard[B_SOUL_SOIL] > 0 && blk_hard[B_WEEPING_VINES] > 0);
    CHECK(blk_fuel[B_CRIMSON_PLANKS] == 0 && blk_fuel[B_WARPED_STEM] == 0 && smelt_of(B_CRIMSON_STEM) == 0);
    plat_remove_prefix("utsoul");
  }
  {
    /* ---- the nether's mobs (todo [15]): piglins and zombified piglins ----------------
     * Spawning, models, skins, drops, spawn eggs, /summon; the enderman is the stretch
     * past them and is not in (no ender pearl item, no teleport). Bartering, the
     * golden-armour truce and any piglin anger beyond being hit are out of scope. */
    extern uint32_t game_time;
    /* models: the 1.16 piglin's own head (10 wide, snout, tusks, ears) on the player's
     * body: 11 boxes for both; MAX_PARTS is 12. Skins from minecraft-textures/. */
    {
      Entity e;
      memset(&e, 0, sizeof e);
      Part parts[MAX_PARTS];
      e.type = E_ZPIGLIN, e.limb_amt = 1;
      CHECK(mob_parts(&e, 0, parts) == 11);
      e.type = E_PIGLIN;
      CHECK(mob_parts(&e, 0, parts) == 11);
      CHECK(mob_width(E_PIGLIN) == 0.6f && mob_height(E_PIGLIN) == 1.95f);
      CHECK(mob_width(E_ZPIGLIN) == 0.6f && mob_height(E_ZPIGLIN) == 1.95f);
      CHECK(is_mob(E_PIGLIN) && is_mob(E_ZPIGLIN) && is_hostile_mob(E_PIGLIN) && is_hostile_mob(E_ZPIGLIN));
      CHECK(!is_mob(E_ARROW) && !is_mob(0));
      /* the ids: an item's id is one the item tables have a row for; the block states
       * past the reserved range (396's chiseled quartz and on) are ids of their own,
       * and the eggs sit in the state space's unused tail past every block (447, 448) */
      CHECK(is_item_id(I_PIGLIN_SPAWN_EGG) && is_item_id(I_ZOMBIFIED_PIGLIN_SPAWN_EGG));
      CHECK(is_item_id(I_WOODEN_SWORD) && is_item_id(I_QUARTZ));
      CHECK(!is_item_id(B_CHISELED_QUARTZ_BLOCK) && !is_item_id(B_QUARTZ_PILLAR) && !is_item_id(B_DEEPSLATE));
      CHECK(!is_item_id(I_END) && !is_item_id(0) && !is_item_id(B_STONE));
      CHECK(!strcmp(item_label(I_PIGLIN_SPAWN_EGG), "Piglin Spawn Egg"));
      CHECK(!strcmp(item_label(B_CHISELED_QUARTZ_BLOCK), "Chiseled Quartz Block"));
    }
    /* spawning: in the nether only, on solid ground of the wastes, around the player,
     * in small groups, with the hostile cap; no sky light gate (vanilla nether mobs
     * ignore light). A spawn never drops them in lava (mob_can_stand refuses it). */
    {
      world_new(99, "utnmob");
      world_switch_dim(DIM_NETHER);
      world_follow(8, 40, 8);
      while (world_pending()) world_follow(8, 40, 8);
      int y = 40, px = 8, pz = 8;
      for (int z = pz - 20; z <= pz + 20; z++)
        for (int x = px - 20; x <= px + 20; x++) {
          world_set(x, y - 1, z, B_NETHERRACK);
          for (int k = 0; k < 4; k++) world_set(x, y + k, z, B_AIR);
        }
      /* The synthetic floor is a controlled biome patch too: this checks the
       * biome-specific list rather than whichever noise patch seed 99 made here. */
      memset(vbiome, BI_NETHER_WASTES, sizeof vbiome);
      memset(ents, 0, sizeof ents);
      opt.difficulty = 2, pl.mode = 0, pl.dead = false, pl.health = 20;
      pl.x = px + 0.5f, pl.y = (float)y, pl.z = pz + 0.5f;
      game_rules = GR_DEFAULT;
      uint32_t rng0 = rnd_seed(123);
      for (int t = 0; t < 200; t++) mobs_spawn();
      int pig = 0, zpig = 0, other = 0;
      for (int i = 0; i < N_ENT; i++) {
        if (ents[i].type == E_PIGLIN) pig++;
        else if (ents[i].type == E_ZPIGLIN) zpig++;
        else if (is_mob(ents[i].type)) other++;
      }
      CHECK(pig + zpig > 0 && other == 0);   /* only the nether's two */
      CHECK(pig + zpig <= 8);                /* the hostile cap */
      for (int i = 0; i < N_ENT; i++)
        if (ents[i].type == E_PIGLIN || ents[i].type == E_ZPIGLIN) {
          int bx = (int)floorf(ents[i].x), by = (int)floorf(ents[i].y), bz = (int)floorf(ents[i].z);
          CHECK(mob_can_stand(bx, by, bz) && world_get(bx, by - 1, bz) != B_LAVA);
        }
      /* the soul sand valley uses the existing skeleton; the warped forest has
       * no naturally spawning mob the game models (enderman/strider are absent). */
      memset(ents, 0, sizeof ents);
      memset(vbiome, BI_SOUL_SAND_VALLEY, sizeof vbiome);
      rnd_seed(123);
      for (int t = 0; t < 200; t++) mobs_spawn();
      int skel = 0;
      for (int i = 0; i < N_ENT; i++) {
        if (ents[i].type == E_SKELETON) skel++;
        else if (is_mob(ents[i].type)) other++;
      }
      CHECK(skel > 0 && other == 0);
      memset(ents, 0, sizeof ents);
      memset(vbiome, BI_WARPED_FOREST, sizeof vbiome);
      for (int t = 0; t < 200; t++) mobs_spawn();
      int warped_mobs = 0;
      for (int i = 0; i < N_ENT; i++) warped_mobs += is_mob(ents[i].type);
      CHECK(warped_mobs == 0);
      /* peaceful: nothing (as overworld hostiles make nothing) */
      memset(ents, 0, sizeof ents);
      opt.difficulty = 0;
      for (int t = 0; t < 100; t++) mobs_spawn();
      int any = 0;
      for (int i = 0; i < N_ENT; i++) any += is_mob(ents[i].type);
      CHECK(any == 0);
      opt.difficulty = 2;
      /* the overworld still makes its own, never the nether's */
      world_switch_dim(DIM_OVER);
      world_follow(8, 70, 8);
      while (world_pending()) world_follow(8, 70, 8);
      for (int z = -12; z <= 28; z++)
        for (int x = -12; x <= 28; x++) {
          world_set(x, 69, z, B_GRASS);
          for (int k = 0; k < 4; k++) world_set(x, 70 + k, z, B_AIR);
        }
      memset(ents, 0, sizeof ents);
      game_time = 18000;   /* midnight: hostiles want the dark */
      pl.x = 8.5f, pl.y = 70, pl.z = 8.5f;
      for (int t = 0; t < 200; t++) mobs_spawn(), game_time++;
      int nether_in_over = 0, over = 0;
      for (int i = 0; i < N_ENT; i++) {
        if (ents[i].type == E_PIGLIN || ents[i].type == E_ZPIGLIN) nether_in_over++;
        else if (is_mob(ents[i].type)) over++;
      }
      CHECK(nether_in_over == 0 && over > 0);
      rnd_seed(rng0);
      plat_remove_prefix("utnmob");
    }
    /* behaviour: the piglin is hostile (the zombie's chase and bite); the zombified is
     * neutral (wanders, and when hit it fights back and its neighbours join: 1.8's
     * zombie pigman trick). No burning (the daylight burn is the zombie's and the
     * skeleton's only). */
    {
      world_new(100, "utmobai");
      world_follow(8, 70, 8);
      int y = 70;
      for (int z = 0; z <= 20; z++)
        for (int x = 0; x <= 20; x++) {
          world_set(x, y - 1, z, B_STONE);
          for (int k = 0; k < 4; k++) world_set(x, y + k, z, B_AIR);
        }
      memset(ents, 0, sizeof ents);
      opt.difficulty = 2, pl.mode = 0, pl.dead = false, pl.health = 20;
      pl.x = 8.5f, pl.y = (float)y, pl.z = 8.5f;
      game_time = 18000;
      /* a piglin eight blocks off, seeing the player: it comes and bites */
      Entity *pg = ent_new(E_PIGLIN, 8.5f, (float)y, 16.5f);
      pg->health = 16;
      float d0 = fabsf(pg->z - pl.z);
      for (int t = 0; t < 200; t++) ticks_run++, ents_tick(), game_time++;
      CHECK(pg->type == E_PIGLIN && fabsf(pg->z - pl.z) < d0);
      CHECK(pl.health < 20);   /* (it bit) */
      /* a zombified left alone wanders (no chase, no bite) */
      memset(ents, 0, sizeof ents);
      pl.health = 20;
      Entity *zg = ent_new(E_ZPIGLIN, 8.5f, (float)y, 16.5f);
      zg->health = 20;
      for (int t = 0; t < 200; t++) ticks_run++, ents_tick(), game_time++;
      CHECK(zg->type == E_ZPIGLIN && pl.health == 20 && zg->panic == 0);
      /* hit it: it fights back, and its neighbours join */
      Entity *n1 = ent_new(E_ZPIGLIN, 10.5f, (float)y, 16.5f);
      Entity *n2 = ent_new(E_ZPIGLIN, 6.5f, (float)y, 16.5f);
      n1->health = n2->health = 20;
      pl.slot = 0, pl.inv[0].id = 0;
      player_look(pl.x, pl.y + 1.62f, pl.z, 0, 60);
      mob_attack(zg);
      CHECK(zg->panic > 0 && n1->panic > 0 && n2->panic > 0);
      /* (the player does not tick here, so the hit grace is cleared by hand: several
       * bites have to land for the check to read one) */
      for (int t = 0; t < 200; t++) pl.invuln = 0, ticks_run++, ents_tick(), game_time++;
      CHECK(pl.health < 20);   /* (they bit back) */
      /* daylight does not burn them (only the zombie and the skeleton burn) */
      memset(ents, 0, sizeof ents);
      game_time = 6000;   /* noon */
      Entity *pz = ent_new(E_ZPIGLIN, 8.5f, (float)y, 8.5f);
      Entity *pp = ent_new(E_PIGLIN, 9.5f, (float)y, 8.5f);
      pz->health = 20, pp->health = 16;
      for (int t = 0; t < 100; t++) ticks_run++, ents_tick(), game_time++;
      CHECK(pz->type == E_ZPIGLIN && pp->type == E_PIGLIN && pz->fire == 0 && pp->fire == 0);
      /* far away: gone (hostiles despawn past 32, as overworld hostiles do) */
      pz->x = pl.x + 40;
      for (int t = 0; t < 5; t++) ticks_run++, ents_tick(), game_time++;
      CHECK(pz->type == E_NONE);
      plat_remove_prefix("utmobai");
    }
    /* drops: the piglin's is empty (modern's piglin.json: nothing but XP); the
     * zombified's is rotten flesh 0-1 and gold nuggets 0-1 (its gold-ingot 2.5% with
     * looting is left out: no enchanting here). */
    {
      world_new(101, "utmobloot");
      world_follow(8, 70, 8);
      int y = 70;
      for (int z = 0; z <= 8; z++)
        for (int x = 0; x <= 8; x++) {
          world_set(x, y - 1, z, B_STONE);
          for (int k = 0; k < 3; k++) world_set(x, y + k, z, B_AIR);
        }
      pl.x = 8.5f, pl.y = (float)y, pl.z = 8.5f, pl.mode = 0, pl.dead = false;
      opt.difficulty = 2;
      game_rules = GR_DEFAULT;
      uint32_t rng0 = rnd_seed(7);
      /* a piglin killed drops nothing (but XP: the level moves). It dies three blocks
       * off so the drops are not picked up as they land (the pickup box is one). */
      memset(ents, 0, sizeof ents);
      memset(pl.inv, 0, sizeof pl.inv);
      int xp0 = pl.xp_total;
      Entity *pg = ent_new(E_PIGLIN, 8.5f, (float)y, 11.5f);
      pg->health = 1;
      mob_attack(pg);
      for (int t = 0; t < 30; t++) ticks_run++, ents_tick();
      int pig_items = 0;
      for (int i = 0; i < N_ENT; i++) pig_items += ents[i].type == E_ITEM;
      CHECK(pg->type == E_NONE && pig_items == 0 && pl.xp_total > xp0);
      /* a zombified drops only flesh and nuggets, 0-1 each */
      int saw_flesh = 0, saw_nugget = 0;
      for (int k = 0; k < 20; k++) {
        memset(ents, 0, sizeof ents);
        Entity *zg = ent_new(E_ZPIGLIN, 8.5f, (float)y, 11.5f);
        zg->health = 1;
        mob_attack(zg);
        for (int t = 0; t < 30; t++) ticks_run++, ents_tick();
        for (int i = 0; i < N_ENT; i++)
          if (ents[i].type == E_ITEM) {
            int id = ents[i].item.id, n = ents[i].item.aux;
            CHECK((id == I_ROTTEN_FLESH || id == I_GOLD_NUGGET) && n >= 0 && n <= 1);
            if (id == I_ROTTEN_FLESH && n) saw_flesh = 1;
            if (id == I_GOLD_NUGGET && n) saw_nugget = 1;
          }
      }
      CHECK(saw_flesh && saw_nugget);   /* (both drop over twenty kills) */
      rnd_seed(rng0);
      plat_remove_prefix("utmobloot");
    }
    /* spawn eggs, /summon and spawners: the eggs are in the Spawn Eggs tab and set
     * into a spawner; /summon piglin works and completes. */
    {
      world_new(102, "utmobegg");
      world_follow(8, 70, 8);
      int y = 70;
      for (int z = 0; z <= 8; z++)
        for (int x = 0; x <= 8; x++) {
          world_set(x, y - 1, z, B_GRASS);
          for (int k = 0; k < 3; k++) world_set(x, y + k, z, B_AIR);
        }
      CHECK(item_kind(I_PIGLIN_SPAWN_EGG) == IK_SPAWN_EGG && item_kind(I_ZOMBIFIED_PIGLIN_SPAWN_EGG) == IK_SPAWN_EGG);
      /* Spawn Eggs tab holds both (Search holds everything once) */
      {
        int lo = tab_start[11], hi = tab_start[12], has_pig = 0, has_zpig = 0;
        for (int i = lo; i < hi; i++) {
          if (tab_items[i] == I_PIGLIN_SPAWN_EGG) has_pig = 1;
          if (tab_items[i] == I_ZOMBIFIED_PIGLIN_SPAWN_EGG) has_zpig = 1;
        }
        CHECK(has_pig && has_zpig);
      }
      /* /summon (and its completion) */
      world_cheats = true;
      memset(ents, 0, sizeof ents);
      pl.x = 8.5f, pl.y = (float)y, pl.z = 8.5f;
      command_run("/summon Piglin");
      {
        int n = 0;
        for (int i = 0; i < N_ENT; i++) n += ents[i].type == E_PIGLIN;
        CHECK(n == 1 && !strcmp(chat_last_line(), "Object successfully summoned"));
      }
      command_run("/summon ZombifiedPiglin");
      {
        int n = 0;
        for (int i = 0; i < N_ENT; i++) n += ents[i].type == E_ZPIGLIN;
        CHECK(n == 1);
      }
      {
        char o[64];
        int at = 0;
        /* ("Pi" matches Pig first: the list's own order, as /summon's does) */
        CHECK(command_complete("/summon Pi", 0, &at, o, sizeof o) && !strcmp(o, "Pig"));
        CHECK(command_complete("/summon Pi", 1, &at, o, sizeof o) && !strcmp(o, "Piglin"));
        CHECK(command_complete("/summon Zombified", 0, &at, o, sizeof o) && !strcmp(o, "ZombifiedPiglin"));
      }
      /* /give names (modern's piglin_spawn_egg, zombified_piglin_spawn_egg) */
      memset(pl.inv, 0, sizeof pl.inv);
      command_run("/give @p piglin_spawn_egg 1");
      CHECK(pl.inv[0].id == I_PIGLIN_SPAWN_EGG);
      command_run("/give @p zombified_piglin_spawn_egg 1");
      {
        int found = 0;
        for (int i = 0; i < 36; i++) found += pl.inv[i].id == I_ZOMBIFIED_PIGLIN_SPAWN_EGG;
        CHECK(found == 1);
      }
      /* an egg on a spawner sets it, and it makes that mob (in the nether too). The
       * spawner sits at the player's own level one ahead, the crosshair down on it. */
      world_switch_dim(DIM_NETHER);
      world_follow(8, 40, 8);
      while (world_pending()) world_follow(8, 40, 8);
      for (int z = -4; z <= 8; z++)
        for (int x = -4; x <= 12; x++) {
          world_set(x, 40, z, B_NETHERRACK);
          for (int k = 1; k <= 3; k++) world_set(x, 40 + k, z, B_AIR);
        }
      world_set(8, 41, 9, B_MOB_SPAWNER);
      memset(ents, 0, sizeof ents);
      pl.x = 8.5f, pl.y = 41, pl.z = 8.5f, pl.mode = 0, pl.slot = 0;
      opt.difficulty = 2;
      spawners_clear();
      pl.inv[0] = (Stack){I_PIGLIN_SPAWN_EGG, 2};
      player_look(pl.x, pl.y + 1.62f, pl.z, 0, 70);
      CHECK(pl.hit_face >= 0 && world_get(pl.hit_x, pl.hit_y, pl.hit_z) == B_MOB_SPAWNER);
      player_tick(K_USE, K_USE);
      CHECK(spawner_mob_at(8, 41, 9) == E_PIGLIN);
      {
        uint8_t buf[512];
        int len = spawners_save(buf);
        spawners_clear();
        spawners_load(buf, len);
        CHECK(spawner_mob_at(8, 41, 9) == E_PIGLIN);
      }
      /* it draws (a frame with the egg turning in it) */
      {
        Camera c = {8.5f, 42.62f, 6.5f, 0, 12};
        render_frame(&c, 6000);
      }
      world_cheats = false;
      plat_remove_prefix("utmobegg");
    }
  }
  {
    /* the nether's build height: blocks above its ceiling (y 128..199) place, break
     * and come back from the records - through the cache's own window, and through a
     * save and a load */
    new_world(3, 12, 1, false, WT_DEFAULT, "Roof", NULL);
    world_switch_dim(DIM_NETHER);
    int px = 66, pz = 8;
    pl.x = px + 0.5f, pl.z = pz + 0.5f;
    pl.y = 130.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(vc_y0 + VCY > 128);   /* (the window is up in the open air) */
    world_set(px, 128, pz, B_DIAMOND_BLOCK);   /* just over the ceiling */
    CHECK(world_get(px, 128, pz) == B_DIAMOND_BLOCK);
    pl.y = 150.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    world_set(px, 150, pz, B_GOLD_BLOCK);
    CHECK(world_get(px, 150, pz) == B_GOLD_BLOCK);
    pl.y = 195.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(vc_y0 + VCY == NETHER_H);   /* (the cache stops at the build height) */
    int nj = edits_count();
    world_set(px, 199, pz, B_EMERALD_BLOCK);   /* the top block there is */
    CHECK(world_get(px, 199, pz) == B_EMERALD_BLOCK && edits_count() == nj + 1);
    world_set(px, 200, pz, B_EMERALD_BLOCK);   /* one past it: nothing to write */
    CHECK(edits_count() == nj + 1 && world_get(px, 200, pz) == B_AIR);
    CHECK(edits_flush());
    /* the cache gone and back: the records put them back */
    world_switch_dim(DIM_OVER);
    world_switch_dim(DIM_NETHER);
    pl.y = 130.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(px, 128, pz) == B_DIAMOND_BLOCK);
    pl.y = 195.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(px, 199, pz) == B_EMERALD_BLOCK && world_get(px, 150, pz) == B_AIR);
    pl.y = 150.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(px, 150, pz) == B_GOLD_BLOCK);
    /* and through the world's own save and load */
    save_world();
    CHECK(load_world(3));
    world_switch_dim(DIM_NETHER);
    pl.y = 195.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(px, 199, pz) == B_EMERALD_BLOCK);
    pl.y = 128.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(px, 128, pz) == B_DIAMOND_BLOCK);
    delete_world(3);
  }
  {
    /* concrete: the powder falls like sand and gravel and stays powder while it is dry; the
     * water that touches it - a source or a flow - sets it into the concrete of its colour,
     * which then sits where it is; powder that falls into water sets where it lands */
    new_world(5, 15, 0, false, WT_DEFAULT, "Concrete", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int bx = (int)floorf(pl.x), by = (int)floorf(pl.y), bz = (int)floorf(pl.z);
    /* a column of red powder on a stone shelf, a three-deep pit dug under it (the stone
     * below the pit is the ground the fall stops on) */
    world_set(bx, by - 1, bz, B_STONE);
    for (int y = by; y <= by + 2; y++) world_set(bx, y, bz, B_AIR);
    world_set(bx, by + 3, bz, B_STONE);
    world_set(bx, by + 4, bz, B_CONCRETE_POWDER_RED);
    world_set(bx, by + 5, bz, B_CONCRETE_POWDER_RED);
    /* the shelf goes: the column falls into the pit and stacks there, still powder */
    break_block_at(bx, by + 3, bz, false);
    CHECK(world_get(bx, by + 3, bz) == B_AIR && world_get(bx, by + 4, bz) == B_AIR &&
          world_get(bx, by + 5, bz) == B_AIR);
    CHECK(world_get(bx, by, bz) == B_CONCRETE_POWDER_RED);
    CHECK(world_get(bx, by + 1, bz) == B_CONCRETE_POWDER_RED);
    /* a source beside the lower one sets it; a flow arriving beside the upper one sets that
     * too (the same pair of calls a flowing liquid makes: the set and its neighbours) */
    world_set(bx + 1, by, bz, B_WATER);
    neighbours_changed(bx + 1, by, bz);
    CHECK(world_get(bx, by, bz) == B_CONCRETE_RED);
    world_set(bx + 1, by + 1, bz, B_FLOWING_WATER_2);
    neighbours_changed(bx + 1, by + 1, bz);
    CHECK(world_get(bx, by + 1, bz) == B_CONCRETE_RED);
    /* set, it does not fall: the pit's floor goes and the two stay */
    break_block_at(bx, by - 1, bz, false);
    CHECK(world_get(bx, by - 1, bz) == B_AIR);
    CHECK(world_get(bx, by, bz) == B_CONCRETE_RED && world_get(bx, by + 1, bz) == B_CONCRETE_RED);
    /* powder that lands in water sets where it lands: a pool with powder above it */
    world_set(bx + 3, by - 1, bz, B_STONE);
    world_set(bx + 3, by, bz, B_WATER);
    world_set(bx + 4, by, bz, B_WATER);
    world_set(bx + 3, by + 2, bz, B_CONCRETE_POWDER_CYAN);
    world_set(bx + 3, by + 1, bz, B_AIR);   /* (the change that starts the fall) */
    neighbours_changed(bx + 3, by + 1, bz);
    CHECK(world_get(bx + 3, by, bz) == B_CONCRETE_CYAN && world_get(bx + 3, by + 2, bz) == B_AIR);
    /* and they draw (a frame with the set pair in view) */
    {
      Camera c = {(float)bx + 2.5f, (float)by + 2.62f, (float)bz - 2.5f, 0, 20};
      render_frame(&c, 6000);
    }
    delete_world(5);
  }
  {
    /* the portal: the frame lights with flint and steel, standing in it travels to the
     * nether at the 1:8 mapping, the far side's portal is made where there is none,
     * and coming back goes through the portal the player left by */
    world_new(9, "utp");
    pl.x = 8.5f, pl.y = 70.0f, pl.z = 8.5f;
    pl.vx = pl.vy = pl.vz = 0.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int fx = (int)floorf(pl.x) - 2, fy = (int)floorf(pl.y), fz = (int)floorf(pl.z) + 3;
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 5; j++) {
        bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
        world_set(fx + i, fy + j, fz, inner ? B_AIR : B_OBSIDIAN);
      }
    /* in its air, looking at the frame's side column: flint and steel lights it */
    pl.x = fx + 1.5f, pl.z = fz + 0.5f, pl.y = (float)fy + 1;
    pl.inv[0] = (Stack){I_FLINT_AND_STEEL, 1}, pl.slot = 0, pl.mode = 0, pl.dead = false, pl.health = 20;
    pl.yaw = 90, pl.pitch = 0, pl.vx = pl.vy = pl.vz = 0;
    player_look(pl.x, pl.y + 1.62f, pl.z, 90, 0);
    CHECK(pl.hit_face >= 0 && world_get(pl.hit_x, pl.hit_y, pl.hit_z) == B_OBSIDIAN);
    player_tick(K_USE, K_USE);
    CHECK(world_get(fx + 1, fy + 1, fz) == B_PORTAL_NS && world_get(fx + 2, fy + 3, fz) == B_PORTAL_NS);
    /* standing in it: a few seconds, and the nether */
    memset(ents, 0, sizeof ents);
    pl.yaw = 0;
    for (int t = 0; t < 100 && world_dim == DIM_OVER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_NETHER);
    int mx = (fx + 1) / 8, mz = fz / 8;
    CHECK(mx != 0 || mz != 0);
    CHECK(abs((int)floorf(pl.x) - mx) <= 2 && abs((int)floorf(pl.z) - mz) <= 2);
    /* the portal is made on the far side, and standing in it comes back */
    int found = 0;
    for (int dz = -3; dz <= 3 && !found; dz++)
      for (int dx = -3; dx <= 3 && !found; dx++)
        for (int y = 20; y < 100 && !found; y++)
          if (world_get(mx + dx, y, mz + dz) == B_PORTAL_NS) {
            pl.x = mx + dx + 0.5f, pl.z = mz + dz + 0.5f, pl.y = (float)y;
            pl.vx = pl.vy = pl.vz = 0;
            found = 1;
          }
    CHECK(found);
    /* the far side's portal spawned whole: a full obsidian ring stands around it */
    {
      int nobs = 0;
      int fx2 = (int)floorf(pl.x), fy2 = (int)floorf(pl.y), fz2 = (int)floorf(pl.z);
      for (int dy = -2; dy <= 3; dy++)
        for (int dz = -2; dz <= 2; dz++)
          for (int dx = -2; dx <= 2; dx++)
            if (world_get(fx2 + dx, fy2 + dy, fz2 + dz) == B_OBSIDIAN) nobs++;
      CHECK(nobs >= 10);
    }
    for (int t = 0; t < 160 && world_dim == DIM_NETHER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_OVER);
    /* back through the portal the player left by (or at its place) */
    CHECK(fabsf(pl.x - (fx + 1.5f)) < 6.0f && fabsf(pl.z - (fz + 0.5f)) < 6.0f && abs((int)floorf(pl.y) - (fy + 1)) < 6);
    /* breaking the frame breaks the whole interior: one obsidian out, no portal left */
    {
      int nportal = 0;
      for (int j = 1; j <= 3; j++)
        for (int i = 1; i <= 2; i++)
          if (world_get(fx + i, fy + j, fz) == B_PORTAL_NS) nportal++;
      CHECK(nportal == 6);
      CHECK(world_get(fx, fy + 1, fz) == B_OBSIDIAN);
      break_block_at(fx, fy + 1, fz, false);
      nportal = 0;
      for (int j = 1; j <= 3; j++)
        for (int i = 1; i <= 2; i++)
          if (world_get(fx + i, fy + j, fz) == B_PORTAL_NS) nportal++;
      CHECK(nportal == 0);
    }
    /* blowing one up breaks it too: a second frame, lit, TNT next to it */
    {
      int gx = fx + 12;
      for (int i = 0; i < 4; i++)
        for (int j = 0; j < 5; j++) {
          bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
          world_set(gx + i, fy + j, fz, inner ? B_AIR : B_OBSIDIAN);
        }
      CHECK(portal_light_here(gx + 1, fy + 1, fz));
      CHECK(world_get(gx + 1, fy + 2, fz) == B_PORTAL_NS);
      explode((float)gx + 1.5f, (float)fy + 2.0f, (float)fz, 4.0f, true);
      int left = 0;
      for (int j = 1; j <= 3; j++)
        for (int i = 1; i <= 2; i++)
          if (world_get(gx + i, fy + j, fz) == B_PORTAL_NS) left++;
      CHECK(left == 0);
      CHECK(world_get(gx, fy + 1, fz) == B_OBSIDIAN);   /* (the frame stands: only the interior goes) */
    }
    /* a frame running along Z lights thin in X: the EW state, not the NS one */
    {
      int hx = fx - 6, hz = fz;   /* (inside the cache: the scan reads past it as air) */
      for (int i = 0; i < 4; i++)
        for (int j = 0; j < 5; j++) {
          bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
          world_set(hx, fy + j, hz + i, inner ? B_AIR : B_OBSIDIAN);
        }
      CHECK(portal_light_here(hx, fy + 1, hz + 1));
      CHECK(world_get(hx, fy + 1, hz + 1) == B_PORTAL_EW);
      CHECK(world_get(hx, fy + 3, hz + 2) == B_PORTAL_EW);
      CHECK(world_get(hx, fy + 2, hz + 1) != B_PORTAL_NS);
    }
    plat_remove_prefix("utp");
  }
  {
    /* portals link in pairs at 1:8 both ways: a second nether portal at 0,10
     * arrives at 0,80 in the overworld (not back at the first), and a second
     * overworld portal gets its own nether portal that leads back to it */
    world_new(12, "utlink");
    pl.mode = 1, pl.dead = false, pl.health = 20, pl.flying = true;
    pl.vx = pl.vy = pl.vz = 0.0f;
    /* high air out of any portal before idling (a lit portal left standing in
     * would travel again once the cooldown runs out) */
    pl.x = 0.5f, pl.y = 90.0f, pl.z = 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    for (int t = 0; t < 100; t++) player_tick(0, 0);   /* (the last test's cooldown) */
    pl.x = 0.5f, pl.y = 70.0f, pl.z = 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    /* overworld A at (0,70,0), along X */
    int A_x = 0, A_y = 70, A_z = 0;
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 5; j++) {
        bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
        world_set(A_x + i, A_y + j, A_z, inner ? B_AIR : B_OBSIDIAN);
      }
    CHECK(portal_light_here(A_x + 1, A_y + 1, A_z));
    pl.x = A_x + 1.5f, pl.y = (float)A_y + 1, pl.z = A_z + 0.5f;
    pl.vx = pl.vy = pl.vz = 0.0f;
    for (int t = 0; t < 10 && world_dim == DIM_OVER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_NETHER);
    /* nether A' near (0,70,0) */
    int NA_x = 0, NA_z = 0, NA_found = 0;
    {
      int best = 0;
      for (int lz = 0; lz < VCZ; lz++)
        for (int lx = 0; lx < VCX; lx++)
          for (int ly = 0; ly < VCY; ly++) {
            int bx = vc_x0 + lx, by = vc_y0 + ly, bz = vc_z0 + lz;
            if (!is_portal(world_get(bx, by, bz))) continue;
            int dx = bx - 0, dy = by - 70, dz = bz - 0, d = dx * dx + dy * dy + dz * dz;
            if (!NA_found || d < best) NA_found = 1, best = d, NA_x = bx, NA_z = bz;
          }
    }
    CHECK(NA_found && abs(NA_x - 0) <= 6 && abs(NA_z - 0) <= 6);
    for (int t = 0; t < 80; t++) player_tick(0, 0);   /* (the arrival cooldown) */
    /* nether B' at (-1,70,10), along X: ten nether blocks south of A', its
     * inside at x 0 (0*8) so the overworld side lands at x 0 */
    int B2_x = -1, B2_y = 70, B2_z = 10;
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 5; j++) {
        bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
        world_set(B2_x + i, B2_y + j, B2_z, inner ? B_AIR : B_OBSIDIAN);
      }
    CHECK(portal_light_here(B2_x + 1, B2_y + 1, B2_z));
    pl.x = B2_x + 1.5f, pl.y = (float)B2_y + 1, pl.z = B2_z + 0.5f;
    pl.vx = pl.vy = pl.vz = 0.0f;
    for (int t = 0; t < 10 && world_dim == DIM_NETHER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_OVER);
    /* at 0,80 (ten times eight): linked to B', not back at A (0,0) */
    {
      int ox = (int)floorf(pl.x), oz = (int)floorf(pl.z);
      CHECK(abs(ox - 0) <= 6 && abs(oz - 80) <= 6);
      CHECK(abs(oz - A_z) > 40);
    }
    for (int t = 0; t < 80; t++) player_tick(0, 0);
    /* overworld B at (200,70,0): far enough for its own nether portal */
    pl.x = 200.5f, pl.y = 70.0f, pl.z = 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int B_x = 200, B_y = 70, B_z = 0;
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 5; j++) {
        bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
        world_set(B_x + i, B_y + j, B_z, inner ? B_AIR : B_OBSIDIAN);
      }
    CHECK(portal_light_here(B_x + 1, B_y + 1, B_z));
    pl.x = B_x + 1.5f, pl.y = (float)B_y + 1, pl.z = B_z + 0.5f;
    pl.vx = pl.vy = pl.vz = 0.0f;
    for (int t = 0; t < 10 && world_dim == DIM_OVER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_NETHER);
    /* its nether portal near (25,70,0), far from A' (0,70,0) */
    {
      int nx = (int)floorf(pl.x), nz = (int)floorf(pl.z);
      CHECK(abs(nx - 25) <= 6 && abs(nz - 0) <= 6);
      CHECK(abs(nx - NA_x) > 15);
    }
    for (int t = 0; t < 80; t++) player_tick(0, 0);
    /* into its portal, and back: to B (200,0), not to A (0,0) */
    {
      int found = 0, fx2 = 0, fy2 = 0, fz2 = 0, best = 0;
      for (int lz = 0; lz < VCZ; lz++)
        for (int lx = 0; lx < VCX; lx++)
          for (int ly = 0; ly < VCY; ly++) {
            int bx = vc_x0 + lx, by = vc_y0 + ly, bz = vc_z0 + lz;
            if (!is_portal(world_get(bx, by, bz))) continue;
            int dx = bx - 25, dy = by - 70, dz = bz - 0, d = dx * dx + dy * dy + dz * dz;
            if (!found || d < best) found = 1, best = d, fx2 = bx, fy2 = by, fz2 = bz;
          }
      CHECK(found);
      if (found) {
        pl.x = fx2 + 0.5f, pl.y = (float)fy2, pl.z = fz2 + 0.5f;
        pl.vx = pl.vy = pl.vz = 0.0f;
      }
    }
    for (int t = 0; t < 10 && world_dim == DIM_NETHER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_OVER);
    {
      int ox = (int)floorf(pl.x), oz = (int)floorf(pl.z);
      CHECK(abs(ox - 200) <= 8 && abs(oz - 0) <= 8);
    }
    plat_remove_prefix("utlink");
  }
  {
    /* portals never spawn underwater: aimed at a pool, the new portal lands
     * above the water on its obsidian floor instead of submerged in it */
    world_new(15, "utpool");
    pl.mode = 1, pl.dead = false, pl.health = 20, pl.flying = true;
    pl.vx = pl.vy = pl.vz = 0.0f;
    /* high air out of any portal: the last test ends inside its own portal, and
     * idling there would travel again once the cooldown runs out */
    pl.x = 8.5f, pl.y = 90.0f, pl.z = 8.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    for (int t = 0; t < 100; t++) player_tick(0, 0);   /* (the last test's cooldown) */
    pl.x = 8.5f, pl.y = 70.0f, pl.z = 8.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    /* a pool: stone at 60, water 61..64, air above (no ticks run, so it stays) */
    for (int z = 0; z < 16; z++)
      for (int x = 0; x < 16; x++) {
        world_set(x, 60, z, B_STONE);
        for (int y = 61; y <= 64; y++) world_set(x, y, z, B_WATER);
        for (int y = 65; y <= 70; y++) world_set(x, y, z, B_AIR);
      }
    /* overworld portal far away (so the return must build, not find) */
    int W_x = 200, W_y = 70, W_z = 200;
    pl.x = W_x + 0.5f, pl.y = 70.0f, pl.z = W_z + 0.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 5; j++) {
        bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
        world_set(W_x + i, W_y + j, W_z, inner ? B_AIR : B_OBSIDIAN);
      }
    CHECK(portal_light_here(W_x + 1, W_y + 1, W_z));
    pl.x = W_x + 1.5f, pl.y = (float)W_y + 1, pl.z = W_z + 0.5f;
    pl.vx = pl.vy = pl.vz = 0.0f;
    for (int t = 0; t < 10 && world_dim == DIM_OVER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_NETHER);
    for (int t = 0; t < 80; t++) player_tick(0, 0);
    /* nether portal aimed so the overworld side lands in the pool at y 61 */
    pl.x = 1.5f, pl.y = 61.0f, pl.z = 1.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 5; j++) {
        bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
        world_set(0 + i, 61 + j, 1, inner ? B_AIR : B_OBSIDIAN);
      }
    CHECK(portal_light_here(0 + 1, 61 + 1, 1));
    pl.x = 1.5f, pl.y = 62.0f, pl.z = 1.5f;
    pl.vx = pl.vy = pl.vz = 0.0f;
    for (int t = 0; t < 10 && world_dim == DIM_NETHER; t++) player_tick(0, 0);
    CHECK(world_dim == DIM_OVER);
    /* above the water (pool top 64): dry feet on the obsidian floor with the
     * portal right ahead (arrivals land in front of a new portal, facing it) */
    {
      int ox = (int)floorf(pl.x), oy = (int)floorf(pl.y), oz = (int)floorf(pl.z);
      CHECK(!is_water(world_get(ox, oy, oz)));
      CHECK(is_portal(world_get(ox, oy, oz + 1)) || is_portal(world_get(ox, oy, oz - 1)));
      CHECK(oy > 64);
      CHECK(world_get(ox, oy - 1, oz) == B_OBSIDIAN);
    }
    plat_remove_prefix("utpool");
  }
  {
    /* glowstone grows as solid clumps hanging off the ceiling: face-touching
     * blocks, several neighbours deep (not a diagonal staircase where nothing
     * face-touches) */
    world_new(13, "utglow");
    world_switch_dim(DIM_NETHER);
    world_follow(8, 108, 8);
    while (world_pending()) world_follow(8, 108, 8);
    int total = 0, with_nb = 0, max_nb = 0;
    for (int lz = 0; lz < VCZ; lz++)
      for (int lx = 0; lx < VCX; lx++)
        for (int ly = 0; ly < VCY; ly++) {
          int bx = vc_x0 + lx, by = vc_y0 + ly, bz = vc_z0 + lz;
          if (world_get(bx, by, bz) != B_GLOWSTONE) continue;
          total++;
          int nbs = 0;
          if (world_get(bx + 1, by, bz) == B_GLOWSTONE) nbs++;
          if (world_get(bx - 1, by, bz) == B_GLOWSTONE) nbs++;
          if (world_get(bx, by + 1, bz) == B_GLOWSTONE) nbs++;
          if (world_get(bx, by - 1, bz) == B_GLOWSTONE) nbs++;
          if (world_get(bx, by, bz + 1) == B_GLOWSTONE) nbs++;
          if (world_get(bx, by, bz - 1) == B_GLOWSTONE) nbs++;
          if (nbs >= 1) with_nb++;
          if (nbs > max_nb) max_nb = nbs;
        }
    CHECK(total > 20);
    CHECK(with_nb * 2 >= total);   /* at least half face-touch another */
    CHECK(max_nb >= 2);            /* branching, not a lone trail */
    plat_remove_prefix("utglow");
  }
  {
    /* the quartz family's cut blocks place, drop themselves and mine with a
     * pickaxe, with their modern names - and they draw (a frame in view) */
    world_new(14, "utq");
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int qx = (int)floorf(pl.x), qy = (int)floorf(pl.y), qz = (int)floorf(pl.z);
    static const int qb[6] = {B_CHISELED_QUARTZ_BLOCK, B_QUARTZ_PILLAR, B_QUARTZ_PILLAR_X, B_QUARTZ_PILLAR_Z,
                              B_SMOOTH_QUARTZ, B_QUARTZ_BRICKS};
    for (int k = 0; k < 6; k++) world_set(qx + k, qy + 2, qz + 4, qb[k]);
    for (int k = 0; k < 6; k++) CHECK(world_get(qx + k, qy + 2, qz + 4) == qb[k]);
    static const int qi[4] = {B_CHISELED_QUARTZ_BLOCK, B_QUARTZ_PILLAR, B_SMOOTH_QUARTZ, B_QUARTZ_BRICKS};
    for (int k = 0; k < 4; k++) {
      CHECK(can_harvest(qi[k], I_WOODEN_PICKAXE));
      Stack o[2];
      CHECK(block_drops(qi[k], I_WOODEN_PICKAXE, o) == 1 && o[0].id == qi[k]);
    }
    CHECK(!strcmp(item_label(B_CHISELED_QUARTZ_BLOCK), "Chiseled Quartz Block"));
    CHECK(!strcmp(item_label(B_QUARTZ_PILLAR), "Quartz Pillar"));
    CHECK(!strcmp(item_label(B_SMOOTH_QUARTZ), "Smooth Quartz"));
    CHECK(!strcmp(item_label(B_QUARTZ_BRICKS), "Quartz Bricks"));
    {
      Camera c = {(float)qx + 2.5f, (float)qy + 3.62f, (float)qz - 2.5f, 0, 20};
      render_frame(&c, 6000);
    }
    plat_remove_prefix("utq");
  }
  {
    /* a world saved before the states were 9 bits: its records, written back in the old
     * shape, are re-encoded 9-bit (edits_convert) and read the same afterwards */
    new_world(2, 11, 0, false, WT_DEFAULT, "Old", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int bx = (int)floorf(pl.x), by = (int)floorf(pl.y), bz = (int)floorf(pl.z);
    world_set(bx, by, bz, B_GOLD_BLOCK);
    world_set(bx + 1, by, bz, B_DIAMOND_BLOCK);
    save_world();
    /* the region's record, written back in the old 8-bit shape - the whole record: the
     * settle pass (world.c) may have left entries of its own beside the two changes */
    uint32_t len = 0;
    char rec[40];
    const uint8_t *r = NULL;
    for (int i = 0; plat_record(i, rec, sizeof rec); i++)
      if (!strncmp(rec, "nb2r", 4) && plat_load(rec, &len) && len >= 8 && len % 4 == 0) {
        r = plat_load(rec, &len);
        break;
      }
    CHECK(r != NULL);
    uint8_t *old = r ? (uint8_t *)edits_scratch(len) : NULL;
    CHECK(old != NULL);
    for (uint32_t i = 0; old && i < len / 4; i++) {
      uint32_t e;
      memcpy(&e, r + 4 * i, 4);
      /* the current entry, y and all: undo the deepslate floor base (the overworld's
       * stored y is y + 64), then the old shape - the 7-bit-y position, the block on top */
      uint32_t key = e & 0xFFFFF, blk = (e >> 20) & 511;
      int y = (int)(key & 0xFF) - 64;
      key = (key & ~0xFFu) | (uint32_t)(y & 0xFF);
      e = (((key >> 8) << 7) | (key & 0x7F)) | blk << 24;
      memcpy(old + 4 * i, &e, 4);
    }
    if (old) plat_save(rec, old, len);
    /* the world again: the slab is made from the records, so the old shape reads wrong ... */
    load_world(2);
    world_switch_dim(DIM_OVER);   /* (the whole volume again, from the records) */
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(bx, by, bz) != B_GOLD_BLOCK);
    /* ... and the conversion puts it back */
    edits_convert("nb2", 0);
    world_switch_dim(DIM_OVER);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(bx, by, bz) == B_GOLD_BLOCK && world_get(bx + 1, by, bz) == B_DIAMOND_BLOCK);
    delete_world(2);
  }
  {
    /* a world saved while the position packed y in 7 bits (an NBW5 head): the load
     * re-encodes its records once and every position comes back - the y low and the
     * x, z and chunk of a column moved up one bit - and the head says NBW7 at once,
     * so a game killed before its next save does not convert them a second time */
    new_world(4, 13, 0, false, WT_DEFAULT, "Old5", NULL);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int bx = (int)floorf(pl.x), by = (int)floorf(pl.y), bz = (int)floorf(pl.z);
    world_set(bx, by, bz, B_GOLD_BLOCK);
    world_set(bx + 1, by + 1, bz + 2, B_DIAMOND_BLOCK);
    save_world();
    /* the records back in the 19-bit shape, and the head as NBW5 */
    char rec[40];
    for (int i = 0; plat_record(i, rec, sizeof rec); i++) {
      uint32_t len = 0;
      const uint8_t *d = plat_load(rec, &len);
      uint8_t *tmp = d && len >= 4 ? (uint8_t *)edits_scratch(len) : NULL;
      if (!strcmp(rec, "nb4.nbw")) {
        CHECK(tmp != NULL);
        if (!tmp) continue;
        memcpy(tmp, d, len);   /* (the whole record, its magic patched) */
        memcpy(tmp, "NBW5", 4);
        plat_save(rec, tmp, len);
        continue;
      }
      if (strncmp(rec, "nb4", 3) || (rec[3] != 'r' && rec[3] != 'n')) continue;
      if (!tmp || !len || len % 4) continue;
      memcpy(tmp, d, len);
      for (uint32_t k = 0; k < len / 4; k++) {
        uint32_t e;
        memcpy(&e, tmp + 4 * k, 4);
        uint32_t key = e & 0xFFFFF, blk = (e >> 20) & 511;
        int y = (int)(key & 0xFF) - 64;   /* undo the deepslate floor base, then the 19-bit shape */
        key = (key & ~0xFFu) | (uint32_t)(y & 0xFF);
        e = (((key >> 8) << 7) | (key & 0x7F)) | blk << 19;
        memcpy(tmp + 4 * k, &e, 4);
      }
      plat_save(rec, tmp, len);
    }
    load_world(4);   /* (the load re-encodes the records: NBW5 -> the current shape) */
    world_switch_dim(DIM_OVER);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(bx, by, bz) == B_GOLD_BLOCK);
    CHECK(world_get(bx + 1, by + 1, bz + 2) == B_DIAMOND_BLOCK);
    uint32_t hl4 = 0;
    const uint8_t *h4 = plat_load("nb4.nbw", &hl4);
    CHECK(h4 && hl4 >= 4 && !memcmp(h4, "NBW7", 4));   /* (the head says so at once) */
    delete_world(4);
  }
  {
    /* an edit in the deep layer (y -40, below the 1.8 world) survives a save and reload:
     * the record's y is based on the deepslate floor (NBW7), and both a plain block and a
     * deepslate ore come back */
    new_world(8, 21, 0, false, WT_DEFAULT, "Deep", NULL);
    pl.x = 8.5f, pl.z = 8.5f, pl.y = -40.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    world_set(8, -40, 8, B_GOLD_BLOCK);
    world_set(9, -39, 9, B_DEEPSLATE_DIAMOND_ORE);
    save_world();
    load_world(8);
    world_switch_dim(DIM_OVER);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    CHECK(world_get(8, -40, 8) == B_GOLD_BLOCK);
    CHECK(world_get(9, -39, 9) == B_DEEPSLATE_DIAMOND_ORE);
    delete_world(8);
  }
  {
    /* the deep layer is carved: the caves are a pure function of a block's world position,
     * so a chunk comes out the same whichever slab window asks for it, and a cavern chunk
     * holds rooms and tunnels with rock still dominant (no floating islands) */
    world_new(12345, "utcave");
    uint8_t gs[GEN_SCRATCH];
    static uint8_t full[64 * 288 + 4], part[64 * 288 + 4];
    gen_scratch(gs);
    /* the same in any slab window, on a nearly-solid chunk and a cavern one */
    int diff = 0;
    for (int t = 0; t < 2; t++) {
      int cx = 1, cz = t ? 2 : 0;
      gen_slab(cx, cz, -64, 64, full);
      gen_slab(cx, cz, -64, 24, part);
      gen_slab(cx, cz, -40, 24, part + 24 * 288);
      gen_slab(cx, cz, -16, 16, part + 48 * 288);
      for (int r = 0; r < 64; r++)
        for (int i = 0; i < 256; i++)
          if (b9_get(full, r * 256 + i) != b9_get(part, r * 256 + i)) diff++;
    }
    /* chunk (1, 2) holds a cavern system */
    gen_slab(1, 2, -64, 64, full);
    gen_scratch(NULL);
    int air = 0, rock = 0, bad = 0;
    for (int r = 0; r < 64; r++)
      for (int i = 0; i < 256; i++) {
        int a = b9_get(full, r * 256 + i);
        if (a == B_AIR) air++; else if (a != B_WATER && a != B_LAVA) rock++;
        if (a == B_WATER && r > 0 && b9_get(full, (r - 1) * 256 + i) == B_LAVA) bad++;
      }
    CHECK(diff == 0);       /* the same in any slab window */
    CHECK(air > 1500);      /* rooms and tunnels to walk */
    CHECK(rock > air);      /* with rock dominant between them */
    CHECK(bad == 0);        /* no water sits directly on lava (obsidian crusts the contact) */
  }
  {
    /* a column whose chunk is still to be made: it is reported pending - so the pick ray
     * passes it over instead of mining a block that would then stay - but reads as stone,
     * so nothing falls through it */
    world_new(0, "utpend");
    world_follow(8, 70, 8);
    world_follow(24, 70, 8);   /* a move of 16: the far chunks are queued, not made at once */
    int pend = 0, stone = 0;
    for (int z = 0; z < VCZ && !pend; z++)
      for (int x = 0; x < VCX; x++)
        if (world_pending_at(vc_x0 + x, 70, vc_z0 + z)) {
          pend = 1;
          stone = world_get(vc_x0 + x, 70, vc_z0 + z) == B_STONE;
          break;
        }
    CHECK(pend && stone);
  }
  {
    /* a dug shaft keeps its sky light as the cache follows the player below the surface: the
     * sky top is the shaft's own bottom (a signed world y below 0 now), not the surface that
     * has left the cache, so the deep part of the shaft is lit, not black */
    world_new(0, "utlight");
    pl.x = 8.5f, pl.z = 8.5f;
    for (int y = 90; y >= -30; y--) {   /* from the open air above the surface, straight down */
      pl.y = (float)y;
      world_follow(pl.x, pl.y, pl.z);
      while (world_pending()) world_follow(pl.x, pl.y, pl.z);
      world_set(8, y, 8, B_AIR);
    }
    /* leave and come back, so the column is rebuilt from the records (edits_top), not kept */
    pl.x = 300.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    pl.x = 8.5f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    world_follow(pl.x, pl.y, pl.z);   /* flush the last change's light */
    int n = 0, lit = 0;
    for (int y = -30; y <= -19; y++)
      if (world_loaded(8, y, 8)) {
        n++;
        if (light_at(VC_I(8 - vc_x0, y - vc_y0, 8 - vc_z0)) > 0) lit++;
      }
    CHECK(n > 0 && lit == n);
  }
  {
    /* keys saved as they first were (Back mining, OK placing) become OK mining, Back placing; set ones stay */
    keys_reset();
    CHECK(opt.keys[A_ATTACK] == 4 && opt.keys[A_USE] == 5);
    opt.keys[A_ATTACK] = 5, opt.keys[A_USE] = 4;
    keys_update(3);
    CHECK(opt.keys[A_ATTACK] == 4 && opt.keys[A_USE] == 5);
    opt.keys[A_ATTACK] = 12, opt.keys[A_USE] = 4;
    keys_update(3);
    CHECK(opt.keys[A_ATTACK] == 12 && opt.keys[A_USE] == 4);
    opt.keys[A_ATTACK] = 5, opt.keys[A_USE] = 4;
    keys_update(KEY_SHEET);   /* (chosen with this version: kept) */
    CHECK(opt.keys[A_ATTACK] == 5 && opt.keys[A_USE] == 4);
    keys_reset();
  }
  {
    /* the sprint key: i, next to forward; backspace (which deletes while typing) no longer sprints,
     * and a key set by someone on backspace moves over */
    keys_reset();
    CHECK(opt.keys[A_SPRINT] == 21);
    CHECK(keys_of(1ull << 21) == K_SPRINT);       /* (i pressed: sprint, and nothing else) */
    CHECK(keys_of(1ull << 17) == 0);
    opt.keys[A_SPRINT] = 17;
    keys_update(5);
    CHECK(opt.keys[A_SPRINT] == 21);              /* (backspace: it moves to i) */
    opt.keys[A_SPRINT] = 12;                      /* (set to shift: it stays) */
    keys_update(5);
    CHECK(opt.keys[A_SPRINT] == 12);
    keys_reset();
  }
  {
    /* Brightness: the options come in at 8, just over the middle of Moody to Bright. One
     * byte is appended at the end of the options record: a record written before the field
     * existed is shorter, so what the field had stays; a value the four bits cannot hold
     * is never set (Moody). */
    CHECK(opt.brightness == 8);
    static uint8_t rec[sizeof opt];
    save_options();
    uint32_t len = 0;
    const uint8_t *d = plat_load("numblocks.cfg", &len);   /* (save.c's OPTIONS_REC) */
    CHECK(d && len == sizeof opt);
    memcpy(rec, d, sizeof rec);
    /* the record as an older version wrote it: it ends before the field */
    opt.brightness = 7;
    plat_save("numblocks.cfg", rec, offsetof(Options, brightness));
    load_options();
    CHECK(opt.brightness == 7);                     /* (what the field had stays) */
    /* a value the field cannot hold: never set */
    rec[offsetof(Options, brightness)] = 200;
    plat_save("numblocks.cfg", rec, sizeof rec);
    opt.brightness = 7;
    load_options();
    CHECK(opt.brightness == 0);
    opt.brightness = 0;
    remove("build/unit-saves/numblocks.cfg");
  }
  {
    /* ---- the desert pyramid (todo [23]) ------------------------------------------
     * Placed by the jar's RandomSpreadStructurePlacement (spacing 32, separation 8,
     * salt 14357617, LINEAR) over deserts (1.21.5-26.3's has_structure tag), built
     * by DesertPyramidPiece block for block, its plate the trap over nine TNT, its
     * four chests filled from the jar's desert_pyramid loot table when first opened.
     * The reference for every number here: the decompiled classes, replayed side by
     * side with the generator's own box (tools-check, not in this file). */
    static uint8_t s1[4][64 * 288 + 4], s2[4][64 * 288 + 4];
    uint8_t gs[GEN_SCRATCH];
    gen_scratch(gs);
    gen_init(12345);
    /* the first temple out from the origin grid cell: the placement's own formula
     * (the grid cell, Java's Random, nextInt(spacing - separation) twice), deserts
     * only - the biome the generator itself asks - then the chunk itself for the
     * piece's plate, which nothing else in a desert makes */
    int ox = 0, oz = 0, found = 0;
    for (int gr = 0; gr <= 6 && !found; gr++)
      for (int gz = -gr; gz <= gr && !found; gz++)
        for (int gx = -gr; gx <= gr && !found; gx++) {
          if (gr && gx > -gr && gx < gr && gz > -gr && gz < gr) continue;
          int64_t s = (int64_t)gx * 341873128712LL + (int64_t)gz * 132897987541LL + 12345 + 14357617;
          uint64_t r = ((uint64_t)s ^ 0x5DEECE66DLL) & ((1ULL << 48) - 1);
          int sx = 0, sz = 0;
          for (int k = 0; k < 2; k++) {
            int bits, val;
            do {
              r = (r * 0x5DEECE66DLL + 0xBLL) & ((1ULL << 48) - 1);
              bits = (int)(r >> 17);
              val = bits % 24;
            } while ((int)((unsigned)bits - (unsigned)val + 23u) < 0);
            if (k) sz = val; else sx = val;
          }
          int cx = gx * 32 + sx, cz = gz * 32 + sz;
          if (gen_biome(cx * 16 + 8, cz * 16 + 8) != 2) continue;   /* (BI_DESERT, gen.c's own enum) */
          gen_slab(cx, cz, 40, 64, s1[0]);
          for (int row = 0; row < 64 && !found; row++)
            for (int i = 0; i < 256; i++)
              if (b9_get(s1[0], row * 256 + i) == B_STONE_PRESSURE_PLATE) {
                ox = cx, oz = cz, found = 1;
                break;
              }
        }
    CHECK(found);
    CHECK(ox == 86 && oz == -14);   /* (the placement's answer for seed 12345: the nearest desert cell) */
    /* the same temple from any direction: the 2x2's four chunks, made in two
     * different orders, come out the same - the memo keeps the outcome and every
     * write lands in the chunk it belongs to */
    static const int dx4[4] = {0, 1, 0, 1}, dz4[4] = {0, 0, 1, 1};
    for (int k = 0; k < 4; k++) gen_slab(ox + dx4[k], oz + dz4[k], 40, 64, s1[k]);
    for (int k = 3; k >= 0; k--) gen_slab(ox + dx4[k], oz + dz4[k], 40, 64, s2[k]);
    int diff = 0;
    for (int k = 0; k < 4; k++)
      for (int row = 0; row < 64; row++)
        for (int i = 0; i < 256; i++)
          diff += b9_get(s1[k], row * 256 + i) != b9_get(s2[k], row * 256 + i);
    CHECK(diff == 0);
    /* the piece: the base's top row (local y = 0) sits 11 above the plate */
    int pr = -1;
    for (int row = 0; row < 64 && pr < 0; row++)
      if (b9_get(s1[0], row * 256 + 10 * 16 + 10) == B_STONE_PRESSURE_PLATE) pr = 40 + row;
    CHECK(pr > 0);
    int base = pr + 11;
    int cut = 0, chi = 0, orn = 0, blu = 0, slb = 0, plt = 0, tnt = 0, cst = 0, st[4] = {0, 0, 0, 0};
    for (int ly = -14; ly <= 10; ly++)
      for (int lx = 0; lx < 21; lx++)
        for (int lz = 0; lz < 21; lz++) {
          int y = base + ly;
          if (y < 40 || y > 103) continue;
          int b = b9_get(s1[(lz >= 16) * 2 + (lx >= 16)], (y - 40) * 256 + (lz % 16) * 16 + (lx % 16));
          switch (b) {
            case B_SANDSTONE_SMOOTH: cut++; break;
            case B_SANDSTONE_CHISELED: chi++; break;
            case B_STAINED_CLAY_ORANGE: orn++; break;
            case B_STAINED_CLAY_BLUE: blu++; break;
            case B_SANDSTONE_SLAB: slb++; break;
            case B_STONE_PRESSURE_PLATE: plt++; break;
            case B_TNT: tnt++; break;
            case B_CHEST: cst++; break;
            case B_SANDSTONE_STAIRS: st[0]++; break;      /* (BlockStairs: 0 east, 1 west, 2 south, 3 north) */
            case B_SANDSTONE_STAIRS_W: st[1]++; break;
            case B_SANDSTONE_STAIRS_S: st[2]++; break;
            case B_SANDSTONE_STAIRS_N: st[3]++; break;
          }
        }
    CHECK(cut == 191 && chi == 39 && orn == 54 && blu == 1);
    CHECK(st[0] == 3 && st[1] == 3 && st[2] == 2 && st[3] == 6);
    CHECK(slb == 2 && plt == 1 && tnt == 9 && cst == 4);
    /* and the cells it is known by: the base, the entrance, the shafts, the chamber,
     * the pattern, the stairs' four ways and the slab pair */
    static const int spot[][4] = {
      {10, -11, 10, B_STONE_PRESSURE_PLATE}, {9, -13, 9, B_TNT}, {10, -13, 10, B_TNT}, {11, -13, 11, B_TNT},
      {10, -11, 8, B_CHEST}, {12, -11, 10, B_CHEST}, {10, -11, 12, B_CHEST}, {8, -11, 10, B_CHEST},
      {9, -11, 9, B_AIR}, {10, -1, 10, B_AIR}, {10, -4, 10, B_AIR},
      {10, 0, 10, B_STAINED_CLAY_BLUE}, {10, 0, 7, B_STAINED_CLAY_ORANGE}, {0, 2, 2, B_STAINED_CLAY_ORANGE},
      {20, 7, 3, B_STAINED_CLAY_ORANGE},
      {4, 1, 5, B_SANDSTONE_SMOOTH}, {4, 2, 5, B_SANDSTONE_CHISELED}, {10, 5, 0, B_SANDSTONE_CHISELED},
      {9, 3, 1, B_SANDSTONE_SMOOTH},
      {2, 10, 0, B_SANDSTONE_STAIRS_N}, {2, 10, 4, B_SANDSTONE_STAIRS_S}, {0, 10, 2, B_SANDSTONE_STAIRS},
      {4, 10, 2, B_SANDSTONE_STAIRS_W}, {2, 1, 2, B_SANDSTONE_STAIRS_W}, {18, 1, 2, B_SANDSTONE_STAIRS},
      {2, 3, 4, B_SANDSTONE_STAIRS_N}, {2, 4, 5, B_SANDSTONE_STAIRS_N},
      {1, 2, 2, B_SANDSTONE_SLAB}, {19, 2, 2, B_SANDSTONE_SLAB},
      {0, -4, 0, B_SANDSTONE}, {20, 0, 20, B_SANDSTONE}, {5, 4, 10, B_SANDSTONE}, {5, 0, 5, B_SANDSTONE},
      {15, 4, 15, B_SANDSTONE}, {6, 7, 10, B_SANDSTONE}, {14, 7, 10, B_SANDSTONE}, {0, 0, 0, B_SANDSTONE},
      {4, 9, 4, B_SANDSTONE},
    };
    for (unsigned k = 0; k < sizeof spot / sizeof spot[0]; k++) {
      int lx = spot[k][0], ly = spot[k][1], lz = spot[k][2];
      int b = b9_get(s1[(lz >= 16) * 2 + (lx >= 16)], (base + ly - 40) * 256 + (lz % 16) * 16 + (lx % 16));
      CHECK(b == spot[k][3]);
    }
    gen_scratch(NULL);
    /* the world itself: the same temple comes out of the cache there (the summaries
     * and the memo make the population decide the same), the plate lights the nine
     * TNT under it, and a chest draws the loot table's items on its first open */
    world_new(12345, "uttemple");
    int wx = ox * 16 + 10, wz = oz * 16 + 10;
    world_follow((float)wx + 0.5f, (float)pr, (float)wz + 0.5f);
    while (world_pending()) world_follow((float)wx + 0.5f, (float)pr, (float)wz + 0.5f);
     CHECK(world_get(wx, pr, wz) == B_STONE_PRESSURE_PLATE);
    /* (the chests sit at the plate's own row: one and two out of the chamber's centre) */
     CHECK(world_get(wx, pr, wz - 2) == B_CHEST && world_get(wx + 2, pr, wz) == B_CHEST &&
           world_get(wx, pr, wz + 2) == B_CHEST && world_get(wx - 2, pr, wz) == B_CHEST);
     memset(ents, 0, sizeof ents);
     pl.x = (float)wx + 0.5f, pl.y = (float)pr + 1.0625f, pl.z = (float)wz + 0.5f;
     pressure_plate_tick(pl.x, pl.y, pl.z);   /* actual feet position atop the plate */
     CHECK(world_get(wx, pr, wz) == B_STONE_PRESSURE_PLATE_DOWN);
     pressure_plates_update();   /* occupied: stay pressed */
     CHECK(world_get(wx, pr, wz) == B_STONE_PRESSURE_PLATE_DOWN);
     int moved = 0, lit = 0, stays = 0;
    for (int dx = -1; dx <= 1; dx++)
      for (int dz = -1; dz <= 1; dz++)
        if (world_get(wx + dx, pr - 2, wz + dz) != B_TNT) moved++;
     for (int i = 0; i < N_ENT; i++) lit += ents[i].type == E_TNT;
     CHECK(moved == 9 && lit == 9);
     pl.x = (float)wx - 4.5f;
     pressure_plates_update();   /* unoccupied: release */
     CHECK(world_get(wx, pr, wz) == B_STONE_PRESSURE_PLATE);
     /* and only the plate: five west there is no plate at that row */
    pressure_plate_tick((float)wx - 4.5f, (float)pr, (float)wz + 0.5f);
    for (int i = 0; i < N_ENT; i++) stays += ents[i].type == E_TNT;
    CHECK(stays == 9);
    /* the loot: the main pool's 2-4 rolls and the bone pool's four, each item in the
     * table's own range (the four horse armours, the enchanted apple and the trim
     * template are dropped: this game has no item for them) */
    int got[64][2], n1 = 0;
    memset(ents, 0, sizeof ents);
    gui_open(GUI_CHEST, wx, pr, wz - 2);
    CHECK(gui == GUI_CHEST);
    gui_close();
    tiles_removed(wx, pr, wz - 2);
    for (int i = 0; i < N_ENT && n1 < 64; i++)
      if (ents[i].type == E_ITEM)
        got[n1][0] = ents[i].item.id, got[n1][1] = ents[i].item.aux, n1++;
    CHECK(n1 >= 4 && n1 <= 8);
    for (int i = 0; i < n1; i++) {
      int id = got[i][0], n = got[i][1];
      CHECK(id == I_DIAMOND || id == I_IRON_INGOT || id == I_GOLD_INGOT || id == I_EMERALD || id == I_BONE ||
            id == I_SPIDER_EYE || id == I_ROTTEN_FLESH || id == I_LEATHER || id == I_BOOK ||
            id == I_GOLDEN_APPLE || id == I_GUNPOWDER || id == I_STRING || id == B_SAND);
      if (id == I_DIAMOND || id == I_EMERALD) CHECK(n >= 1 && n <= 3);
      else if (id == I_IRON_INGOT) CHECK(n >= 1 && n <= 5);
      else if (id == I_GOLD_INGOT) CHECK(n >= 2 && n <= 7);
      else if (id == I_LEATHER) CHECK(n >= 1 && n <= 5);
      else if (id == I_BOOK || id == I_GOLDEN_APPLE) CHECK(n == 1);
      else CHECK(n >= 1 && n <= 8);   /* bone, spider eye, rotten flesh, gunpowder, string, sand */
    }
    /* the same chest again: the same items (the seed is the world seed and the place,
     * nothing of the loot is stored) */
    memset(ents, 0, sizeof ents);
    gui_open(GUI_CHEST, wx, pr, wz - 2);
    gui_close();
    tiles_removed(wx, pr, wz - 2);
    int n2 = 0, same = 1;
    for (int i = 0; i < N_ENT; i++)
      if (ents[i].type == E_ITEM) {
        if (n2 >= n1 || ents[i].item.id != got[n2][0] || ents[i].item.aux != got[n2][1]) same = 0;
        n2++;
      }
    CHECK(same && n2 == n1);
    /* the sandstone stairs place the four ways a player looks, as the oak ones do
     * (BlockStairs: 0 east, 1 west, 2 south, 3 north; the sandstone states sit
     * together, unlike the oak and cobblestone ones) */
    {
      int tx = 3000, tz = 3000, ty = 70;
      world_follow((float)tx + 0.5f, (float)ty, (float)tz + 0.5f);
      while (world_pending()) world_follow((float)tx + 0.5f, (float)ty, (float)tz + 0.5f);
      for (int dz = -3; dz <= 3; dz++)
        for (int dx = -3; dx <= 3; dx++)
          for (int dy = -1; dy <= 3; dy++) world_set(tx + dx, ty + dy, tz + dz, dy == -1 ? B_STONE : B_AIR);
      pl.mode = 0, pl.flying = false, pl.dead = 0;
      pl.x = (float)tx + 0.5f, pl.z = (float)tz + 0.5f, pl.y = (float)ty, pl.vy = 0;
      static const int st4[4] = {B_SANDSTONE_STAIRS_S, B_SANDSTONE_STAIRS_W, B_SANDSTONE_STAIRS_N, B_SANDSTONE_STAIRS};
      static const float yaw4[4] = {0.0f, 90.0f, 180.0f, 270.0f};   /* south, west, north, east */
      static const int wx4[4] = {0, -1, 0, 1}, wz4[4] = {1, 0, -1, 0};
      for (int w = 0; w < 4; w++) {
        int ax = tx + wx4[w], az = tz + wz4[w];
        world_set(ax, ty, az, B_AIR);   /* (the cell the stair goes into) */
        memset(pl.inv, 0, sizeof pl.inv);
        pl.inv[0].id = B_SANDSTONE_STAIRS, pl.inv[0].aux = 1, pl.slot = 0;
        pl.yaw = yaw4[w], pl.pitch = 60;   /* (the floor a block ahead: the cell above it) */
        player_look(pl.x, pl.y + 1.62f, pl.z, pl.yaw, pl.pitch);   /* (as main.c looks each frame) */
        player_tick(K_USE, K_USE);
        CHECK(world_get(ax, ty, az) == st4[w]);
      }
    }
    /* ---- /locate (todo: the user's ask): the structure by the placement grid's own
     * cells, a biome by sampled chunks, in Minecraft's own words ("~" for a structure's
     * y, which is not known without making the chunk) */
    {
      bool was_cheats = world_cheats;   /* (as the command tests above: /locate needs them) */
      world_cheats = true;
      pl.x = 1100.5f, pl.z = -100.5f, pl.y = 70.0f;   /* (in the temple's own grid cell: its chunk is the nearest) */
      command_run("/locate structure desert_pyramid");
      CHECK(!strcmp(chat_last_line(), "The nearest desert_pyramid is at [1376, ~, -224] (302 blocks away)"));
      command_run("/locate structure temple");
      CHECK(!strcmp(chat_last_line(), "There is no structure with type \"temple\""));
      command_run("/locate biome desert");
      {
        int x = 0, y = 0, z = 0, d = 0;
        CHECK(sscanf(chat_last_line(), "The nearest Desert is at [%d, %d, %d] (%d blocks away)", &x, &y, &z, &d) == 4);
        uint8_t lgs[GEN_SCRATCH];
        gen_scratch(lgs);
        CHECK(gen_biome(x, z) == 2 && gen_biome_known(2));
        CHECK(y == gen_top(x, z) + 1);
        long long dx = x - 1100, dz = z - (-101), dy = y - 70;
        CHECK(d == (int)sqrtf((float)(dx * dx + dy * dy + dz * dz)));
        gen_scratch(NULL);
      }
      command_run("/locate biome 2");   /* (an id answers too) */
      CHECK(!strncmp(chat_last_line(), "The nearest Desert is at [", 26));
      command_run("/locate biome swamp");   /* (a modern name for the 1.8 ground) */
      CHECK(!strncmp(chat_last_line(), "The nearest Swampland is at [", 29));
      command_run("/locate biome bogus");
      CHECK(!strcmp(chat_last_line(), "There is no such biome with name bogus"));
      command_run("/locate biome");
      CHECK(!strcmp(chat_last_line(), "Usage: /locate <biome|structure> <biome or structure>"));
      char o[64];
      int at = 0;
      CHECK(command_complete("/locate b", 0, &at, o, sizeof o) && !strcmp(o, "biome"));
      CHECK(command_complete("/locate structure de", 0, &at, o, sizeof o) && !strcmp(o, "minecraft:desert_pyramid"));
      CHECK(command_complete("/locate biome dese", 0, &at, o, sizeof o) && !strcmp(o, "minecraft:desert"));
      world_cheats = was_cheats;
    }
  }
  {
    /* A stone plate powers an adjacent oak door while occupied and closes it
     * again on release. */
    world_new(25, "utplate");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    for (int z = 6; z <= 10; z++)
      for (int x = 6; x <= 10; x++) world_set(x, 69, z, B_STONE);
    world_set(8, 70, 8, B_STONE_PRESSURE_PLATE);
    world_set(9, 70, 8, B_DOOR_OAK_LOWER);
    world_set(9, 71, 8, B_DOOR_OAK_UPPER);
    memset(ents, 0, sizeof ents);
    pl.x = 16.5f, pl.y = 70, pl.z = 16.5f;
    pressure_plate_tick(8.5f, 70, 8.5f);
    CHECK(world_get(8, 70, 8) == B_STONE_PRESSURE_PLATE_DOWN);
    CHECK(world_get(9, 70, 8) == B_DOOR_OAK_LOWER_E_OPEN);
    pressure_plates_update();
    CHECK(world_get(8, 70, 8) == B_STONE_PRESSURE_PLATE);
    CHECK(world_get(9, 70, 8) == B_DOOR_OAK_LOWER);
    plat_remove_prefix("utplate");
  }
  {
    /* An axe strips a Nether stem without changing its axis. */
    world_new(26, "utstrip");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    world_set(8, 70, 9, B_CRIMSON_STEM_X);
    memset(pl.inv, 0, sizeof pl.inv);
    pl.mode = 0, pl.dead = false, pl.x = 8.5f, pl.y = 70, pl.z = 8.5f, pl.health = 20;
    pl.inv[0] = (Stack){I_IRON_AXE, 0}, pl.slot = 0;
    player_look(pl.x, pl.y + 1.62f, pl.z, 0, 45);
    player_tick(K_USE, K_USE);
    CHECK(world_get(8, 70, 9) == B_STRIPPED_CRIMSON_STEM_X);
    plat_remove_prefix("utstrip");
  }
  {
    /* A torch stands on a barrier, as on any whole block (Block.canPlaceTorchOnTop):
       an invisible floor is still a floor. */
    world_new(28, "utbar");
    world_follow(8, 70, 8);
    while (world_pending()) world_follow(8, 70, 8);
    for (int x = 7; x <= 9; x++)
      for (int z = 7; z <= 9; z++) {
        world_set(x, 69, z, B_GRASS);
        for (int k = 1; k < 4; k++) world_set(x, 69 + k, z, B_AIR);
      }
    world_set(8, 69, 9, B_BARRIER);   /* one ahead of the player, its top at y = 70 */
    memset(pl.inv, 0, sizeof pl.inv);
    pl.mode = 0, pl.dead = false, pl.x = 8.5f, pl.y = 70, pl.z = 8.5f, pl.health = 20;
    pl.inv[0] = (Stack){B_TORCH, 4}, pl.slot = 0;
    player_look(pl.x, pl.y + 1.62f, pl.z, 0, 58);   /* down at the barrier's top, one block ahead */
    player_tick(K_USE, K_USE);
    CHECK(world_get(8, 70, 9) == B_TORCH && item_count(&pl.inv[0]) == 3);
    /* and a wall of one still stands in the way of a player walking into it */
    for (int y = 70; y < 72; y++) world_set(8, y, 10, B_BARRIER);
    world_set(8, 69, 10, B_AIR);
    {
      float p[3] = {8.5f, 70, 9.5f}, v[3] = {0, 0, 0.5f};
      phys_move(p, v, 0.6f, 1.8f);
      CHECK(p[2] < 10.0f);
    }
    plat_remove_prefix("utbar");
  }
  printf("%s (%d recipes)\n", fails ? "FAILED" : "all good", N_RECIPES);
  return fails != 0;
}
