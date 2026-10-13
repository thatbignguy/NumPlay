/* Checks of the item rules: crafting, stacking, smelting, drops. */
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include "nb.h"
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
    CHECK(command_complete("/setblock ~ ~ ~ wa", 0, &at, o, sizeof o) && at == 16 && !strcmp(o, "minecraft:water"));
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
    CHECK(pl.y < (float)VOID_Y && pl.health == 20 && !pl.dead);
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
    static const int eggs[8] = {I_ZOMBIE_SPAWN_EGG, I_SKELETON_SPAWN_EGG, I_CREEPER_SPAWN_EGG,
                               I_SPIDER_SPAWN_EGG, I_PIG_SPAWN_EGG, I_COW_SPAWN_EGG, I_SHEEP_SPAWN_EGG,
                               I_CHICKEN_SPAWN_EGG};
    static const int mobs[8] = {E_ZOMBIE, E_SKELETON, E_CREEPER, E_SPIDER, E_PIG, E_COW, E_SHEEP, E_CHICKEN};
    for (int i = 0; i < 8; i++) {
      CHECK(mob_of_egg(eggs[i]) == mobs[i]);
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
    /* every mob survives the save and load: the last two are the ones the old record
     * read back as unset (E_SHEEP and E_CHICKEN are past the old "under 8" bound) */
    {
      uint8_t b2[512];
      spawners_clear();
      for (int i = 0; i < 8; i++) spawners_set(3000 + i * 2, 10, 3000, E_ZOMBIE + i);
      int l2 = spawners_save(b2);
      spawners_clear();
      spawners_load(b2, l2);
      for (int i = 0; i < 8; i++) CHECK(spawner_mob_at(3000 + i * 2, 10, 3000) == E_ZOMBIE + i);
      CHECK(spawners_count() == 8 && spawners_save(NULL) == l2);
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
        if (ents[i].type >= E_ZOMBIE && ents[i].type <= E_CHICKEN) m++;
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
    CHECK(gen_biome(8, 8) == 8 && gen_biome(-500, 900) == 8);
    int nrack = 0, nlava = 0, nair = 0, nbed = 0, nsoul = 0, ngrav = 0, nother = 0;
    int nquartz = 0, nglow = 0;
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
      /* the old shape: the 7-bit-y position, the block on top */
      uint32_t key = e & 0xFFFFF, blk = (e >> 20) & 511;
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
     * x, z and chunk of a column moved up one bit - and the head says NBW6 at once,
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
        e = (((key >> 8) << 7) | (key & 0x7F)) | blk << 19;   /* the 19-bit shape */
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
    CHECK(h4 && hl4 >= 4 && !memcmp(h4, "NBW6", 4));   /* (the head says so at once) */
    delete_world(4);
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
  printf("%s (%d recipes)\n", fails ? "FAILED" : "all good", N_RECIPES);
  return fails != 0;
}
