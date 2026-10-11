/* Plays NumBlocks on a computer with scripted keys and saves screenshots.
 *
 *   play [--frames N] [--ms-per-frame M] [--keys "10-40:fwd,50:use,..."] [--shots 30,60] [--out DIR]
 *        [--showcase]   (the hotbar of the README's GIF: TNT, flint and steel, water and lava, blocks)
 *        [--spawner]    (a monster spawner hanging in the air in front of the player, unset, and the eggs to set it with)
 *        [--distance 1..2]   (render distance, default: the saved option, 2 chunks)
 *        [--fancy]           (graphics: Fancy, see-through leaves; the default is what the options say, Fast)
 *        [--bright 0..15]    (the Brightness setting: 0 Moody, 15 Bright; default: the saved option)
 *        [--trace]       (a line a frame: the screen, the position, health, whether it is dead)
 *        [--flat]       (a superflat world)
 *        [--spectator]  (spectator mode: flying through the blocks, nothing to touch)
 *        [--spectate N] (with --mobs: the spectator's camera is the N-th mob's, 0..7)
 *        [--tree]       (an oak in front of the player, with the spectator's head in its canopy)
 *        [--cave]       (a sealed stone room with no light in it, for what the dark is)
 *        [--void]       (a shaft dug through the bedrock under the player, who hangs in the void below it)
 *
 * Keys: left right up down fwd back sleft sright jump use attack inv sneak sprint pause 1..9.
 * Raws: --raws "10-14:24,20:25" holds the calculator's own key numbers (24 sin, 25 cos...). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "../src/nb.h"

extern uint32_t host_keys, host_time;
extern uint64_t host_raw;   /* the calculator's own key numbers (plat_scan) */
void host_shot(const char *path);
void game_init(void);
bool game_frame(void);

typedef struct { int a, b; uint32_t k; } Hold;
typedef struct { int a, b, k; } RHold;   /* the calculator's own key numbers (plat_scan) */
static Hold holds[512];
static int nholds;
static RHold rholds[64];
static int nrholds;

static uint32_t key_bit(const char *s) {
  static const struct { const char *n; uint32_t k; } t[] = {
    {"left", K_LEFT}, {"right", K_RIGHT}, {"up", K_UP}, {"down", K_DOWN}, {"fwd", K_FWD}, {"back", K_BACKW},
    {"sleft", K_STRAFE_L}, {"sright", K_STRAFE_R}, {"jump", K_JUMP}, {"use", K_USE}, {"attack", K_ATTACK},
    {"inv", K_INV}, {"sneak", K_SNEAK}, {"sprint", K_SPRINT}, {"pause", K_PAUSE}, {"home", K_HOME},
    {"ok", K_OK | K_USE}, {"exe", K_EXE | K_USE}, {"shift", K_SHIFT}, {"esc", K_BACK | K_ATTACK}, {"drop", K_DROP}};
  for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++)
    if (!strcmp(s, t[i].n)) return t[i].k;
  if (s[0] >= '1' && s[0] <= '9' && !s[1]) return K_SLOT1 << (s[0] - '1');
  return 0;
}

int main(int argc, char **argv) {
  const char *out = "build/play", *shots = "";
  int give = 0, showcase = 0, title_screen = 0, mobs = 0, start_time = -1, torches = 0, water = 0, creative = 0, build = 0, rain = 0, spawner = 0;
  int distance = -1;
  int spectator = 0, ride = -1, flat = 0, tree = 0, cave = 0, cave_by = 0, bright = -1, trace = 0, void_world = 0;
  float hurt = 0;
  int frames = 60, mspf = 50;
  int cheats = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--ms-per-frame") && i + 1 < argc) mspf = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
    else if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
    else if (!strcmp(argv[i], "--give")) give = 1;
    else if (!strcmp(argv[i], "--showcase")) showcase = 1;
    else if (!strcmp(argv[i], "--title")) title_screen = 1;
    else if (!strcmp(argv[i], "--mobs")) mobs = 1;
    else if (!strcmp(argv[i], "--torches")) torches = 1;
    else if (!strcmp(argv[i], "--water")) water = 1;
    else if (!strcmp(argv[i], "--creative")) creative = 1;
    else if (!strcmp(argv[i], "--flat")) flat = 1;
    else if (!strcmp(argv[i], "--tree")) tree = 1;
    else if (!strcmp(argv[i], "--cave")) cave = 1;
    else if (!strcmp(argv[i], "--void")) void_world = 1;
    else if (!strcmp(argv[i], "--spectator")) spectator = 1;
    else if (!strcmp(argv[i], "--spectate") && i + 1 < argc) ride = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--cheats")) cheats = 1;
    else if (!strcmp(argv[i], "--build")) build = 1;
    else if (!strcmp(argv[i], "--spawner")) spawner = 1;
    else if (!strcmp(argv[i], "--rain")) rain = 1;
    else if (!strcmp(argv[i], "--storm")) rain = 2;
    else if (!strcmp(argv[i], "--distance") && i + 1 < argc) distance = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--fancy")) opt.fancy = 1;
    else if (!strcmp(argv[i], "--bright") && i + 1 < argc) {
      bright = atoi(argv[++i]);
      if (bright >= 0 && bright <= 15) opt.brightness = (uint8_t)bright;
    }
    else if (!strcmp(argv[i], "--time") && i + 1 < argc) start_time = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--trace")) trace = 1;
    else if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
      extern int64_t start_seed;
      start_seed = strtoll(argv[++i], NULL, 10);
    }
    else if (!strcmp(argv[i], "--hurt") && i + 1 < argc) hurt = (float)atof(argv[++i]);
    else if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
      char *s = strdup(argv[++i]);
      for (char *t = strtok(s, ","); t && nholds < 512; t = strtok(NULL, ",")) {
        int a, b;
        char k[16];
        if (sscanf(t, "%d-%d:%15s", &a, &b, k) == 3) holds[nholds++] = (Hold){a, b, key_bit(k)};
        else if (sscanf(t, "%d:%15s", &a, k) == 2) holds[nholds++] = (Hold){a, a, key_bit(k)};
      }
    }
    else if (!strcmp(argv[i], "--raws") && i + 1 < argc) {
      char *s = strdup(argv[++i]);
      for (char *t = strtok(s, ","); t && nrholds < 64; t = strtok(NULL, ",")) {
        int a, b, k;
        if (sscanf(t, "%d-%d:%d", &a, &b, &k) == 3) rholds[nrholds++] = (RHold){a, b, k};
        else if (sscanf(t, "%d:%d", &a, &k) == 2) rholds[nrholds++] = (RHold){a, a, k};
      }
    }
  }
  mkdir(out, 0755);
  clock_t c0 = clock();
  extern bool start_in_world;
  extern int start_type;
  start_in_world = !title_screen;
  start_type = flat ? WT_FLAT : WT_DEFAULT;
  game_init();
  if (give) {
    /* a test kit: tools, blocks, food, armour */
    static const int kit[][2] = {{I_IRON_PICKAXE, 1}, {I_DIAMOND_SWORD, 1}, {B_COBBLESTONE, 64}, {B_PLANKS_OAK, 23},
                                 {B_TORCH, 16}, {I_BREAD, 5}, {B_CRAFTING_TABLE, 1}, {B_FURNACE, 1}, {B_CHEST, 1},
                                 {B_LOG_OAK, 12}, {I_COAL, 9}, {B_IRON_ORE, 7}, {I_IRON_HELMET, 1},
                                 {I_IRON_CHESTPLATE, 1}, {I_STICK, 10}, {I_WHEAT_SEEDS, 3}, {I_WOODEN_HOE, 1},
                                 {I_BUCKET, 1}, {B_SAND, 20}, {I_APPLE, 2}};
    for (unsigned i = 0; i < sizeof kit / sizeof kit[0]; i++) inv_add(pl.inv, 36, kit[i][0], kit[i][1], 0);
    pl.inv[0].aux = 100;   /* a worn pickaxe */
  }
  if (showcase) {
    /* the README's GIF: the hotbar first, then a full inventory to look at */
    static const int kit[][2] = {{B_TNT, 16}, {I_FLINT_AND_STEEL, 1}, {I_WATER_BUCKET, 1}, {I_LAVA_BUCKET, 1},
                                 {B_COBBLESTONE, 64}, {B_PLANKS_OAK, 48}, {B_GLASS, 32}, {I_DIAMOND_PICKAXE, 1},
                                 {B_TORCH, 32}, {I_DIAMOND_SWORD, 1}, {I_BOW, 1}, {I_ARROW, 48}, {I_DIAMOND, 12},
                                 {I_GOLD_INGOT, 21}, {I_IRON_INGOT, 30}, {I_COOKED_BEEF, 9}, {I_BREAD, 7},
                                 {I_APPLE, 4}, {B_OBSIDIAN, 10}, {B_DIAMOND_BLOCK, 2}, {B_LOG_OAK, 24},
                                 {B_CRAFTING_TABLE, 1}, {B_FURNACE, 2}, {B_CHEST, 3}, {I_IRON_HELMET, 1},
                                 {I_IRON_CHESTPLATE, 1}, {B_SAND, 40}};
    for (unsigned i = 0; i < sizeof kit / sizeof kit[0]; i++) inv_add(pl.inv, 36, kit[i][0], kit[i][1], 0);
  }
  if (start_time >= 0) {
    extern uint32_t game_time;
    game_time = (uint32_t)start_time;
  }
  if (creative) pl.mode = 1;
  if (spectator) pl.mode = 2, pl.flying = true;
  if (tree) {
    /* an oak in front of the player, with the spectator's head in its canopy:
     * flying into leaves (Minecraft's spectator goes through them, and sees the
     * world through them: the canopy is see-through, not a wall of leaf) */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    int cx = px, cz = pz + 6;
    for (int k = 0; k < 4; k++) world_set(cx, fy + k, cz, B_LOG_OAK);
    for (int dy = 2; dy <= 4; dy++)
      for (int dx = -2; dx <= 2; dx++)
        for (int dz = -2; dz <= 2; dz++)
          if (dx * dx + dz * dz + (dy - 4) * (dy - 4) * 3 <= 7) world_set(cx + dx, fy + dy, cz + dz, B_LEAVES_OAK);
    world_set(cx, fy + 5, cz, B_LEAVES_OAK);
    pl.mode = 2, pl.flying = true;
    pl.x = cx + 0.5f, pl.y = fy + 4.05f, pl.z = cz + 0.5f;
    pl.yaw = 0, pl.pitch = 0;
  }
  if (cheats) world_cheats = true;   /* the new world starts without them; the switcher needs them */
  if (distance >= 1 && distance <= 2) opt.distance = (uint8_t)distance;
  if (rain) {
    /* raining (and storming) from the start, for long enough */
    weather.raining = 1, weather.rain_time = 24000, rain_str = 1;
    if (rain == 2) weather.thundering = 1, weather.thunder_time = 24000, thunder_str = 1;
  }
  if (build) {
    /* a cleared field with the shaped blocks in a row: stairs, door, bed, chest, furnace, fence, pane, ladder */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 3; z < pz + 12; z++)
      for (int x = px - 9; x < px + 10; x++)
        for (int y = fy - 1; y < fy + 8; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    int z = pz + 4;
    world_set(px + 5, fy, z, B_OAK_STAIRS);
    world_set(px + 4, fy, z, B_COBBLESTONE_STAIRS_S);
    world_set(px + 2, fy, z, B_DOOR_OAK_LOWER_N), world_set(px + 2, fy + 1, z, B_DOOR_OAK_UPPER);
    world_set(px + 1, fy, z, B_DOOR_OAK_LOWER_N_OPEN), world_set(px + 1, fy + 1, z, B_DOOR_OAK_UPPER);
    world_set(px - 1, fy, z, B_BED_FOOT), world_set(px - 1, fy, z + 1, B_BED_HEAD);
    world_set(px - 3, fy, z, B_CHEST);
    world_set(px - 4, fy, z, B_FURNACE);
    for (int k = 0; k < 3; k++) world_set(px - 6 - k, fy, z, B_FENCE_OAK);
    world_set(px - 6, fy, z + 1, B_FENCE_OAK);
    for (int k = 0; k < 2; k++) world_set(px + 7 + k, fy, z, B_GLASS_PANE), world_set(px + 7 + k, fy + 1, z, B_GLASS_PANE);
    world_set(px + 3, fy, z + 2, B_COBBLESTONE), world_set(px + 3, fy + 1, z + 2, B_COBBLESTONE);
    world_set(px + 3, fy, z + 1, B_LADDER), world_set(px + 3, fy + 1, z + 1, B_LADDER);
    world_set(px - 2, fy + 1, z + 3, B_TORCH_N);
    world_set(px - 2, fy + 1, z + 4, B_COBBLESTONE);
  }
  if (water) {
    /* a cleared field with a water source on a step */
    int fy = (int)pl.y;
    for (int z = (int)pl.z - 3; z < (int)pl.z + 12; z++)
      for (int x = (int)pl.x - 9; x < (int)pl.x + 10; x++)
        for (int y = fy - 1; y < fy + 8; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    world_set((int)pl.x, fy, (int)pl.z + 6, B_COBBLESTONE);
    world_set((int)pl.x, fy + 1, (int)pl.z + 6, B_WATER);
    neighbours_changed((int)pl.x, fy + 1, (int)pl.z + 6);
  }
  if (mobs) {
    /* a row of every mob, 4 blocks ahead, facing the player, on a cleared grass field */
    int fy = (int)pl.y;
    for (int z = (int)pl.z - 3; z < (int)pl.z + 9; z++)
      for (int x = (int)pl.x - 9; x < (int)pl.x + 10; x++)
        for (int y = fy - 1; y < fy + 8; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    for (int k = 0; k < 8; k++) {
      float x = pl.x - 5.6f + k * 1.6f, z = pl.z + 4.5f;
      Entity *e = ent_new(E_ZOMBIE + k, x, (float)fy, z);
      if (e) e->health = 20, e->yaw = 180, e->gx = e->x, e->gz = e->z;
    }
    if (ride >= 0 && ride < N_ENT) spectate = (int8_t)ride;   /* the spectator's camera on the k-th mob */
  }
  if (cave) {
    /* a sealed stone room, the player standing inside it, for what the dark is: a cave with
     * no light in it (pitch black at Moody, readable at Bright). Built rather than dug out
     * of the rock, so nothing of the sky light around it reaches the air inside, and
     * Peaceful, so nothing comes out of the dark to spoil the picture while it is taken */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 9; z <= pz + 9; z++)
      for (int x = px - 9; x <= px + 9; x++)
        for (int y = fy - 1; y <= fy + 7; y++)
          world_set(x, y, z, y == fy - 1 ? B_GRASS
                        : y == fy + 7 || x <= px - 8 || x >= px + 8 || z <= pz - 8 || z >= pz + 8 ? B_STONE
                                                                                                : B_AIR);
    cave_by = fy + 1;
    opt.difficulty = 0;
  }
  if (void_world) {
    /* the hole a player digs: a 3 x 3 shaft through the bedrock under them (the bedrock
     * floor of the world included), with the player hanging in the void below it, where
     * the world above has to show through and around the hole */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 1; z <= pz + 1; z++)
      for (int x = px - 1; x <= px + 1; x++)
        for (int y = 0; y < fy; y++) world_set(x, y, z, B_AIR);
    pl.x = px + 0.5f, pl.z = pz + 0.5f, pl.y = (float)fy - 8.5f, pl.vy = 0;
    if (pl.mode) pl.flying = true;   /* (creative and spectator hang there; survival falls) */
  }
  if (torches) {
    /* torches around, for the night (on the cave's floor when there is one, in front of
     * the player, so a lit room can be seen against an unlit one at any brightness) */
    if (cave) {
      world_set((int)pl.x - 4, cave_by, (int)pl.z + 6, B_TORCH);
      world_set((int)pl.x + 4, cave_by, (int)pl.z + 6, B_TORCH);
    } else {
      int fy = (int)pl.y;
      for (int k = 0; k < 3; k++) world_set((int)pl.x - 3 + k * 3, fy, (int)pl.z + 3 + k, B_TORCH);
      world_set((int)pl.x + 2, fy, (int)pl.z + 6, B_GLOWSTONE);
    }
  }
  if (hurt > 0) pl.health -= hurt, pl.food -= 7, pl.xp_level = 7, pl.xp = 0.4f;
  if (spawner) {
    /* a cleared grass field with a monster spawner hanging in the air in front of the player,
     * empty as it comes from the inventory, and the spawn eggs to set it with */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 3; z < pz + 12; z++)
      for (int x = px - 9; x < px + 10; x++)
        for (int y = fy - 1; y < fy + 10; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    world_set(px, fy + 1, pz + 3, B_MOB_SPAWNER);
    spawners_note(px, fy + 1, pz + 3);
    opt.difficulty = 2;   /* (no monsters at all on a peaceful world: the fresh options have 0) */
    static const int kit[][2] = {{I_ZOMBIE_SPAWN_EGG, 8}, {I_CREEPER_SPAWN_EGG, 8}, {I_PIG_SPAWN_EGG, 8}};
    for (unsigned i = 0; i < sizeof kit / sizeof kit[0]; i++) inv_add(pl.inv, 36, kit[i][0], kit[i][1], 0);
  }
  double init_ms = (clock() - c0) * 1000.0 / CLOCKS_PER_SEC;
  c0 = clock();
  double slowest = 0;
  int slowest_at = 0;
  for (int f = 0; f < frames; f++) {
    host_keys = 0;
    host_raw = 0;
    for (int i = 0; i < nholds; i++)
      if (f >= holds[i].a && f <= holds[i].b) host_keys |= holds[i].k;
    for (int i = 0; i < nrholds; i++)
      if (f >= rholds[i].a && f <= rholds[i].b) host_raw |= 1LL << rholds[i].k;
    host_time += mspf;
    clock_t f0 = clock();
    if (!game_frame()) break;
    double fms = (clock() - f0) * 1000.0 / CLOCKS_PER_SEC;
    if (fms > slowest) slowest = fms, slowest_at = f;
    if (trace) printf("%4d gui %2d mode %d %8.2f %8.2f %8.2f health %5.1f%s\n", f, gui, pl.mode, pl.x, pl.y,
                      pl.z, pl.health, pl.dead ? " DEAD" : "");
    for (const char *s = shots; *s;) {
      if (atoi(s) == f) {
        char p[512];
        snprintf(p, sizeof p, "%s/shot_%d.ppm", out, f);
        host_shot(p);
      }
      const char *c = strchr(s, ',');
      if (!c) break;
      s = c + 1;
    }
  }
  extern unsigned long st_steps, st_texels, st_jumps, st_pixels;
  printf("per frame: %.0f traces, %.0f steps\n", (double)st_pixels / frames, (double)st_steps / frames);
  extern unsigned long st_dis, st_k0, st_fpfail, st_fpok, st_sky;
  printf("between per frame: %.0f disagree, %.0f kind0, %.0f face fail, %.0f face ok, %.0f sky\n", (double)st_dis / frames, (double)st_k0 / frames, (double)st_fpfail / frames, (double)st_fpok / frames, (double)st_sky / frames);
  printf("per pixel: %.1f steps, %.2f texels, %.2f jumps\n", (double)st_steps / st_pixels, (double)st_texels / st_pixels, (double)st_jumps / st_pixels);
  printf("player: mode %d, %.1f %.1f %.1f, health %.1f, dead %d\n", pl.mode, pl.x, pl.y, pl.z, pl.health, pl.dead);
  printf("init %.1f ms, %.2f ms a frame (host), the slowest %.1f ms (frame %d)\n", init_ms,
         (clock() - c0) * 1000.0 / CLOCKS_PER_SEC / frames, slowest, slowest_at);
  return 0;
}
