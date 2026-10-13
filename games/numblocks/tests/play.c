/* Plays NumBlocks on a computer with scripted keys and saves screenshots.
 *
 *   play [--frames N] [--ms-per-frame M] [--keys "10-40:fwd,50:use,..."] [--shots 30,60] [--out DIR]
 *        [--showcase]   (the hotbar of the README's GIF: TNT, flint and steel, water and lava, blocks)
 *        [--animals]    (a sheep, a cow, a pig and a chicken ahead, on the ground where they stand)
 *        [--mobs]       (a row of every mob, four blocks ahead, facing the player, on a cleared field)
 *        [--skeleton]   (one skeleton ahead but looking the other way, on a cleared field, Survival
 *                         and Normal: it sees the player within a few ticks and shoots - the frames
 *                         show it turn to face - or keep its head on - the player as it fires)
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
 *        [--nether]     (the world is the nether's: netherrack, lava sea, red sky)
 *        [--roof]       (above the nether's ceiling: a floor laid over the roof, the player
 *                         standing on it in Creative with netherrack in hand, so keys can
 *                         break and place blocks up there - the nether is 200 tall)
 *        [--concrete]   (a powder column on a shelf over a pit, and a water stream reaching a
 *                         powder block: OK mines the shelf and the column falls, the stream
 *                         sets its block into concrete - the player holds white powder)
 *        [--settle]     (sand hanging in the air and a water source standing beside it,
 *                         with no block update behind either - as generation and a save
 *                         leave blocks: the cache slides a little and back, and when the
 *                         chunks are made again the settle pass drops the sand and starts
 *                         the water - the frames show the sand down and the stream growing)
 *        [--place]      (sand in hand before a stone pillar over the air: OK at frame 25
 *                         places it against the side, and the block's own update - the cell
 *                         it was placed into is checked for what is under it - drops it to
 *                         the ground at once, where a build that only checked its
 *                         neighbours leaves it hanging)
 *        [--lava]       (a lava source standing in the air over a cleared field: the
 *                         stream runs down and spreads - the flowing layers' picture,
 *                         with the still source's over it; Creative, so nothing burns.
 *                         With --nether it is the lava sea, and holding sneak sinks the
 *                         eye into it - --keys "10-40:sneak" and a shot past frame 30
 *                         show the eye in lava: the dense orange fog and screen wash)
 *        [--portal]     (a lit obsidian frame in front of the player, flint and steel in hand:
 *                         the player stands in it and travels when the wait is over)
 *        [--deep]       (the deepslate family in a row - deepslate and its seven ores - on a
 *                         deepslate floor, four blocks ahead: the deep layer's blocks)
 *        [--caves]      (the deep layer's 1.17-1.19 caves: the camera dropped into a deep
 *                         cavern, lit by glowstone, looking along it - the noise caves under
 *                         the surface world; Creative, Peaceful)
 *        [--xray]       (the spectator's water and lava holes: the head inside the stone
 *                         block over a pool of water, a lava block against the block's
 *                         side - looking down, the water under the block and the lava
 *                         beside it are the faces the rock hides; 1.5.9 draws the sand
 *                         under the water and the stone behind the lava instead)
 *        [--contrast]   (Personalisation: High Contrast Block Outlines on)
 *        [--panorama N] (Personalisation: the title's backdrop; 0 Random, 1 World,
 *                         2 Overworld, 3 Nether)
 *
 * Keys: left right up down fwd back sleft sright jump use attack inv sneak sprint pause 1..9.
 * Raws: --raws "10-14:24,20:25" holds the calculator's own key numbers (24 sin, 25 cos...). */
#include <math.h>
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
  int give = 0, showcase = 0, title_screen = 0, mobs = 0, animals = 0, start_time = -1, torches = 0, water = 0, creative = 0, build = 0, rain = 0, spawner = 0;
  int skeleton = 0;
  int distance = -1;
  int spectator = 0, ride = -1, flat = 0, tree = 0, cave = 0, cave_by = 0, bright = -1, trace = 0, void_world = 0;
  int nether = 0, portal = 0, roof = 0, concrete = 0, settle = 0, placing = 0, xray = 0, lava = 0, deep = 0, caves = 0;
  float hurt = 0;
  int frames = 60, mspf = 50;
  int cheats = 0, contrast = 0, panorama = -1;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--ms-per-frame") && i + 1 < argc) mspf = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
    else if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
    else if (!strcmp(argv[i], "--give")) give = 1;
    else if (!strcmp(argv[i], "--showcase")) showcase = 1;
    else if (!strcmp(argv[i], "--title")) title_screen = 1;
    else if (!strcmp(argv[i], "--mobs")) mobs = 1;
    else if (!strcmp(argv[i], "--skeleton")) skeleton = 1;
    else if (!strcmp(argv[i], "--animals")) animals = 1;
    else if (!strcmp(argv[i], "--torches")) torches = 1;
    else if (!strcmp(argv[i], "--water")) water = 1;
    else if (!strcmp(argv[i], "--lava")) lava = 1;
    else if (!strcmp(argv[i], "--creative")) creative = 1;
    else if (!strcmp(argv[i], "--flat")) flat = 1;
    else if (!strcmp(argv[i], "--tree")) tree = 1;
    else if (!strcmp(argv[i], "--cave")) cave = 1;
    else if (!strcmp(argv[i], "--void")) void_world = 1;
    else if (!strcmp(argv[i], "--spectator")) spectator = 1;
    else if (!strcmp(argv[i], "--nether")) nether = 1;
    else if (!strcmp(argv[i], "--roof")) roof = 1;
    else if (!strcmp(argv[i], "--concrete")) concrete = 1;
    else if (!strcmp(argv[i], "--settle")) settle = 1;
    else if (!strcmp(argv[i], "--place")) placing = 1;
    else if (!strcmp(argv[i], "--portal")) portal = 1;
    else if (!strcmp(argv[i], "--xray")) xray = 1;
    else if (!strcmp(argv[i], "--deep")) deep = 1;
    else if (!strcmp(argv[i], "--caves")) caves = 1;
    else if (!strcmp(argv[i], "--spectate") && i + 1 < argc) ride = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--cheats")) cheats = 1;
    else if (!strcmp(argv[i], "--contrast")) contrast = 1;
    else if (!strcmp(argv[i], "--panorama") && i + 1 < argc) panorama = atoi(argv[++i]);
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
  if (nether) {
    /* the nether: the player stands where there is room (the lava and the glowstone
     * are what lights it, so a glowstone goes over the spot), looking down at the
     * terrain */
    world_switch_dim(DIM_NETHER);
    pl.y = 40;   /* (the cache is 24 blocks tall: this one covers the lava sea's shore) */
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int px = (int)floorf(pl.x), pz = (int)floorf(pl.z), best = -1, bx = px, bz = pz;
    for (int dz = -14; dz <= 14 && best < 0; dz++)
      for (int dx = -14; dx <= 14 && best < 0; dx++) {
        int x = px + dx, z = pz + dz;
        for (int y = 45; y >= 33; y--)
          if (world_get(x, y, z) != B_AIR && world_get(x, y + 1, z) == B_AIR && world_get(x, y + 2, z) == B_AIR) {
            best = y, bx = x, bz = z;
            break;
          }
      }
    if (best >= 0) {
      pl.x = bx + 0.5f, pl.z = bz + 0.5f, pl.y = (float)(best + 1), pl.pitch = -24, pl.yaw = 35;
      for (int k = 0; k < 4; k++) world_set(bx + (k & 1) * 2 - 1, best + 3, bz + (k >> 1) * 2 - 1, B_GLOWSTONE);
    }
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    if (best >= 0) {
      int i = VC_I(bx - vc_x0, best + 3 - vc_y0, bz - vc_z0);
      fprintf(stderr, "nether: spot %d %d %d glowstone light %d (vl %d)\n", bx, best + 1, bz, vl[i] >> 4, vl[i]);
    }
    camera_reset();
  }
  if (roof) {
    /* Above the nether's ceiling: the open air over the roof is buildable (the
     * dimension's build limit is 200, so y 128..199 is there). The player stands on
     * a netherrack floor laid just over the ceiling, in Creative with netherrack in
     * hand: scripted keys break a block of it and place another, against the red sky. */
    if (!nether) {
      world_switch_dim(DIM_NETHER);
      pl.x = 8.5f, pl.z = 8.5f;
    }
    pl.y = 126.0f;   /* (up where the cache covers the ceiling, then lay the floor) */
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int px = (int)floorf(pl.x), pz = (int)floorf(pl.z), top = 127;
    for (int y = 127; y >= 120; y--)
      if (world_get(px, y, pz) == B_BEDROCK) {
        top = y;
        break;
      }
    for (int dz = -3; dz <= 3; dz++)
      for (int dx = -3; dx <= 3; dx++) world_set(px + dx, top + 1, pz + dz, B_NETHERRACK);
    pl.x = px + 0.5f, pl.y = (float)(top + 2), pl.z = pz + 0.5f;
    pl.yaw = 0, pl.pitch = 30;   /* (down the floor, a couple of blocks ahead) */
    pl.mode = 1, pl.flying = false;
    pl.inv[0] = (Stack){B_NETHERRACK, 64}, pl.slot = 0;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    camera_reset();
    fprintf(stderr, "roof: standing %d %d %d on netherrack at %d\n", px, top + 2, pz, top + 1);
  }
  if (portal) {
    /* a portal frame in front of the player (an obsidian rectangle 4 x 5 with a
     * 2 x 3 air inside), the player standing in its air with flint and steel,
     * looking at the frame's side column */
    int fx = (int)floorf(pl.x) - 2, fy = (int)floorf(pl.y), fz = (int)floorf(pl.z) + 3;
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 5; j++) {
        bool inner = i >= 1 && i <= 2 && j >= 1 && j <= 3;
        world_set(fx + i, fy + j, fz, inner ? B_AIR : B_OBSIDIAN);
      }
    pl.x = fx + 1.5f, pl.z = fz + 0.5f, pl.y = (float)fy + 1;   /* in the frame's air */
    pl.inv[0] = (Stack){I_FLINT_AND_STEEL, 1};
    pl.slot = 0;
    pl.yaw = 90, pl.pitch = 0;
  }
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
  if (contrast) opt.contrast = 1;
  /* (after game_init: the title's backdrop re-picks when the option changed under it) */
  if (panorama >= PAN_RANDOM && panorama <= PAN_NETHER) opt.panorama = (uint8_t)panorama;
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
  if (deep) {
    /* the deepslate family (1.17): a deepslate floor with a 3-high row of deepslate and
     * its seven ores four blocks ahead, so the deep layer's blocks are in the frame */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 2; z < pz + 8; z++)
      for (int x = px - 6; x < px + 7; x++)
        for (int y = fy - 1; y < fy + 6; y++) world_set(x, y, z, y == fy - 1 ? B_DEEPSLATE : B_AIR);
    static const uint16_t fam[8] = {B_DEEPSLATE, B_DEEPSLATE_COAL_ORE, B_DEEPSLATE_IRON_ORE,
                                    B_DEEPSLATE_GOLD_ORE, B_DEEPSLATE_REDSTONE_ORE, B_DEEPSLATE_DIAMOND_ORE,
                                    B_DEEPSLATE_LAPIS_ORE, B_DEEPSLATE_EMERALD_ORE};
    for (int k = 0; k < 8; k++)
      for (int h = 0; h < 3; h++) world_set(px - 3 + k, fy + h, pz + 4, fam[k]);
    pl.yaw = 0, pl.pitch = 0;
  }
  if (caves) {
    /* the deep layer's 1.17-1.19 noise caves: drop the camera into the middle of the
     * biggest deep cavern the cache holds, light it with a little glowstone and face the
     * most open way, so the frames show a cavern and its tunnels under the surface world
     * (Creative, Peaceful, so nothing spoils the view) */
    pl.x = 8.5f, pl.z = 8.5f, pl.y = -40.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    int bx = VCX / 2, by = VCY / 2, bz = VCZ / 2, best = -1;
    for (int ly = 3; ly < VCY - 3; ly++)
      for (int lz = 3; lz < VCZ - 3; lz++)
        for (int lx = 3; lx < VCX - 3; lx++) {
          if (vc_y0 + ly >= 0 || vc_get(VC_I(lx, ly, lz)) != B_AIR) continue;
          int fl = vc_get(VC_I(lx, ly - 1, lz));
          if (fl == B_AIR || fl == B_WATER || fl == B_LAVA) continue;   /* (a floor to stand on) */
          int s = 0;
          for (int dy = -3; dy <= 3; dy++)
            for (int dz = -3; dz <= 3; dz++)
              for (int dx = -3; dx <= 3; dx++) {
                int b = vc_get(VC_I(lx + dx, ly + dy, lz + dz));
                s += b == B_AIR ? 1 : (b == B_WATER || b == B_LAVA) ? -4 : 0;
              }
          if (s > best) best = s, bx = lx, by = ly, bz = lz;
        }
    pl.x = vc_x0 + bx + 0.5f, pl.y = (float)(vc_y0 + by), pl.z = vc_z0 + bz + 0.5f;
    /* face the most open way (the longest air run of the four) */
    int run[4] = {0, 0, 0, 0};
    for (int d = 1; d <= 20 && bz + d < VCZ && vc_get(VC_I(bx, by, bz + d)) == B_AIR; d++) run[0]++;
    for (int d = 1; d <= 20 && bz - d >= 0 && vc_get(VC_I(bx, by, bz - d)) == B_AIR; d++) run[1]++;
    for (int d = 1; d <= 20 && bx + d < VCX && vc_get(VC_I(bx + d, by, bz)) == B_AIR; d++) run[2]++;
    for (int d = 1; d <= 20 && bx - d >= 0 && vc_get(VC_I(bx - d, by, bz)) == B_AIR; d++) run[3]++;
    int dir = 0;
    for (int k = 1; k < 4; k++) if (run[k] > run[dir]) dir = k;
    static const float yaw_of[4] = {0, 180, 270, 90};   /* +z, -z, +x, -x */
    pl.yaw = yaw_of[dir], pl.pitch = 10.0f;
    /* a little glowstone in the ceiling about the camera and along the way (the deep layer
     * is dark; lights high in the ceiling stay out of the way and read as a player's
     * torches, and the dark past them is the cave going on) */
    int fx = dir == 2 ? 1 : dir == 3 ? -1 : 0, fz = dir == 0 ? 1 : dir == 1 ? -1 : 0;
    world_set(vc_x0 + bx, vc_y0 + by + 3, vc_z0 + bz, B_GLOWSTONE);
    for (int k = 1; k <= 4; k++) {
      int gx = bx + fx * k * 3, gz = bz + fz * k * 3;
      if (gx >= 0 && gx < VCX && gz >= 0 && gz < VCZ) world_set(vc_x0 + gx, vc_y0 + by + 3, vc_z0 + gz, B_GLOWSTONE);
    }
    pl.mode = 1, pl.flying = true;   /* (hover in the cavern) */
    opt.difficulty = 0;
    fprintf(stderr, "caves: the camera at %d %d %d (open score %d), runs %d %d %d %d\n",
            vc_x0 + bx, vc_y0 + by, vc_z0 + bz, best, run[0], run[1], run[2], run[3]);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    camera_reset();
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
  if (lava) {
    if (nether) {
      /* the nether's lava sea: down to the shore (the cache then holds the sea),
       * then over the nearest lava, looking down at it - the still sea's picture */
      pl.y = 38;
      world_follow(pl.x, pl.y, pl.z);
      while (world_pending()) world_follow(pl.x, pl.y, pl.z);
      int bx = (int)floorf(pl.x), bz = (int)floorf(pl.z), lx = 0, ly = 0, lz = 0, bscore = -1;
      for (int z = 1; z < VCZ - 1; z++)
        for (int x = 1; x < VCX - 1; x++)
          for (int y = 0; y < VCY - 1; y++) {
            if (vc_get(VC_I(x, y, z)) != B_LAVA) continue;
            bool open = true;
            for (int k = 1; k <= 5; k++)
              if (vc_get(VC_I(x, y + k, z)) != B_AIR) open = false;
            if (!open) continue;   /* (a rock ceiling over it: stay out of the rock) */
            /* the open surface of the sea: the more lava around it the better */
            int n = (vc_get(VC_I(x - 1, y, z)) == B_LAVA) + (vc_get(VC_I(x + 1, y, z)) == B_LAVA) +
                    (vc_get(VC_I(x, y, z - 1)) == B_LAVA) + (vc_get(VC_I(x, y, z + 1)) == B_LAVA);
            if (n > bscore) bscore = n, lx = vc_x0 + x, ly = vc_y0 + y, lz = vc_z0 + z;
          }
      if (bscore >= 0) {
        pl.x = lx + 0.5f, pl.z = lz + 0.5f, pl.y = (float)(ly + 4);
        pl.yaw = 0, pl.pitch = 78;   /* (pitch runs positive downwards) */
        pl.flying = true;   /* (hover over the sea: it is not solid) */
        world_follow(pl.x, pl.y, pl.z);
        while (world_pending()) world_follow(pl.x, pl.y, pl.z);
        fprintf(stderr, "lava: the sea over %d %d %d (%d sides lava, shore %d %d)\n", lx, ly, lz, bscore, bx, bz);
      }
    } else {
      /* a fall of lava: a source standing in the air over a cleared field - the
       * stream runs down and spreads, its flowing layers' picture over the still
       * source's */
      int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
      for (int z = pz - 3; z < pz + 12; z++)
        for (int x = px - 9; x < px + 10; x++)
          for (int y = fy - 1; y < fy + 8; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
      world_set(px, fy + 4, pz + 6, B_LAVA);
      neighbours_changed(px, fy + 4, pz + 6);
    }
    pl.mode = 1;   /* (Creative: the frames are of the lava and not of a burning player) */
    camera_reset();
  }
  if (concrete) {
    /* the concrete pair on a cleared field: a white powder column on a stone shelf over a
     * pit (Back places a powder block at frame 10, OK mines it at 25, OK mines the shelf at
     * 50 and the column falls into the pit), and a red powder block at the foot of a stone
     * spout the water runs down (the stream sets it on its own). Creative, white powder in
     * hand; the field is cleared wide enough that the water's seven-block spread stops well
     * short of the pit, which only opens when the shelf goes. */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 3; z < pz + 12; z++)
      for (int x = px - 9; x < px + 10; x++)
        for (int y = fy - 1; y < fy + 8; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    /* the column: the shelf, white powder on it, a three-deep pit under the shelf */
    int cx = px - 1, cz = pz + 3;
    world_set(cx, fy - 3, cz, B_STONE);   /* the pit's floor */
    for (int y = fy - 2; y < fy; y++) world_set(cx, y, cz, B_AIR);
    world_set(cx, fy, cz, B_STONE);       /* the shelf */
    for (int y = fy + 1; y <= fy + 3; y++) world_set(cx, y, cz, B_CONCRETE_POWDER_WHITE);
    /* the spout: a stone pillar with the source on top, the red powder two blocks off at its
     * foot (the base flow reaches it two steps in; its top face stands above the water, so
     * the frames can show the powder's texture become the concrete's) */
    int wx = px - 7, wz = pz + 8;
    for (int y = fy; y <= fy + 2; y++) world_set(wx, y, wz, B_STONE);
    world_set(wx, fy + 3, wz, B_WATER);
    world_set(wx, fy, wz - 2, B_CONCRETE_POWDER_RED);
    neighbours_changed(wx, fy + 3, wz);
    pl.mode = 1, pl.flying = false;
    pl.yaw = 18, pl.pitch = 22;   /* (the shelf at the crosshair, the spout off to the right) */
    pl.inv[0] = (Stack){B_CONCRETE_POWDER_WHITE, 64}, pl.slot = 0;
    camera_reset();
    fprintf(stderr, "concrete: shelf %d %d %d (pit under), spout %d %d\n", cx, fy, cz, wx, wz);
  }
  if (settle) {
    /* The settle pass (world.c): a chunk the generator (or a save) put blocks into has
     * never given them their first update - sand hanging over the air a cave carved,
     * water with nothing under it standing still. This builds the same pair with world_set
     * (nothing here runs neighbours_changed, as generation runs nothing), then slides the
     * cache a little and back: the pass runs once the move's chunks are whole, and the
     * sand drops and the water starts running with nothing touching either. The camera
     * stands on the ground between them; the stream grows over the frames. */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    /* a cleared patch of ground: grass at fy-1, air above */
    for (int z = pz - 2; z <= pz + 10; z++)
      for (int x = px - 6; x <= px + 8; x++)
        for (int y = fy - 1; y <= fy + 6; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    /* the sand: two blocks hanging in the air, three blocks over the grass */
    int sx = px - 3, sz = pz + 8;
    world_set(sx, fy + 3, sz, B_SAND);
    world_set(sx, fy + 4, sz, B_SAND);
    /* the water: a source standing in the air with a clear run under it */
    int wx = px + 3, wz = pz + 9;
    world_set(wx, fy + 2, wz, B_WATER);
    /* the cache a little and back: the chunks are made again (a move completes with the
     * settle pass), and the two have not been ticked by anything in between */
    world_follow(pl.x + 8, pl.y, pl.z + 8);
    while (world_pending()) world_follow(pl.x + 8, pl.y, pl.z + 8);
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    pl.yaw = 0, pl.pitch = 6;   /* (the sand ahead and left, the water ahead and right) */
    camera_reset();
    fprintf(stderr, "settle: the sand sits at %d %d %d (%s) and %d (%s); the water source is at %d %d %d (%s) with %d under it (%s)\n",
            sx, fy, sz, world_get(sx, fy, sz) == B_SAND ? "sand" : "NOT SAND", fy + 1,
            world_get(sx, fy + 1, sz) == B_SAND ? "sand" : "NOT SAND", wx, fy + 2, wz,
            world_get(wx, fy + 2, wz) == B_WATER ? "a source" : "NOT A SOURCE", fy + 1,
            world_get(wx, fy + 1, wz) == B_FALLING_WATER ? "falling" : "not yet");
  }
  if (placing) {
    /* A block placed in the air falls the moment it is placed: a placement runs
     * neighbours_changed on the cell it placed into, and that cell is checked for what is
     * under it too - not only its neighbours, which is all that was checked before, so the
     * sand hung where it was put until something beside it changed. A cleared field with a
     * stone pillar one block ahead; the player looks at the pillar's top block, the
     * crosshair on its side over the air, the sand in hand, and OK at frame 25 places it:
     * the frame after the placement has it on the grass under the crosshair. */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 3; z <= pz + 9; z++)
      for (int x = px - 6; x <= px + 6; x++)
        for (int y = fy - 1; y < fy + 6; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    for (int y = fy; y <= fy + 2; y++) world_set(px, y, pz + 3, B_STONE);
    /* back a block, so the crosshair's block and the ground it drops onto are both in
     * the picture (and the pillar stays inside the Creative reach, 5) */
    pl.x = (float)px + 0.5f, pl.y = (float)fy, pl.z = (float)pz - 1.0f;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    pl.mode = 1, pl.flying = false;
    pl.inv[0] = (Stack){B_SAND, 64}, pl.slot = 0;
    pl.yaw = 0, pl.pitch = -12;   /* (the pillar's top block ahead, its side over the air) */
    camera_reset();
    fprintf(stderr, "place: pillar %d %d %d..%d; OK at 25 puts the sand at %d %d %d, over the air\n", px, pz + 3, fy,
            fy + 2, px, fy + 2, pz + 2);
  }
  if (xray) {
    /* The spectator's water and lava holes (render.c's xray rule): with the head inside
     * a stone block the world is seen through the rock, but a liquid face the rock hides
     * - the water directly under the block, the lava beside it - was left out with it,
     * so the view showed what stood behind the liquid instead (the sand under the water,
     * the stone behind the lava). A pool of water with a stone block hanging over its
     * middle and a lava block against the block's side; the spectator's head goes into
     * the block, looking down at the water below and the lava beside it. */
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);   /* (the world whole: no settle pass after the build) */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    /* a cleared field with a sand floor and a stone-rimmed pool of water in it */
    for (int z = pz - 6; z <= pz + 6; z++)
      for (int x = px - 6; x <= px + 6; x++)
        for (int y = fy - 1; y <= fy + 6; y++) world_set(x, y, z, y == fy - 1 ? B_SAND : B_AIR);
    int r = 3;
    for (int z = pz - r; z <= pz + r; z++)
      for (int x = px - r; x <= px + r; x++) world_set(x, fy, z, B_WATER);
    for (int k = -r; k <= r; k++) {
      world_set(px - r - 1, fy, pz + k, B_STONE);
      world_set(px + r + 1, fy, pz + k, B_STONE);
      world_set(px + k, fy, pz - r - 1, B_STONE);
      world_set(px + k, fy, pz + r + 1, B_STONE);
    }
    /* the block over the middle water, and the lava against its side - in a stone
     * pocket, so a settle pass can never start it flowing */
    world_set(px, fy + 1, pz, B_STONE);
    world_set(px + 1, fy + 1, pz, B_LAVA);
    world_set(px + 2, fy + 1, pz, B_STONE);
    world_set(px + 1, fy + 1, pz - 1, B_STONE);
    world_set(px + 1, fy + 1, pz + 1, B_STONE);
    /* the head in the block, the eye half a block over its floor, looking down and
     * towards +z: the water it stands over below, the lava at its side to the left */
    pl.mode = 2, pl.flying = true;
    pl.x = px + 0.5f, pl.y = fy - 0.12f, pl.z = pz + 0.5f;
    pl.yaw = 0, pl.pitch = 60;
    opt.difficulty = 0;
    camera_reset();
    fprintf(stderr, "xray: the block over the water at %d %d %d, the lava against it at %d %d %d, the head in the block\n",
            px, fy + 1, pz, px + 1, fy + 1, pz);
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
  if (skeleton) {
    /* One skeleton on a cleared field, seven blocks ahead of the player but looking the
     * other way (its body's yaw points away from the player). Survival on Normal: within
     * a few ticks it sees the player, draws its bow and shoots. The frames show it turn
     * to face the player as it fires, instead of shooting over its shoulder. */
    int fy = (int)pl.y, px = (int)pl.x, pz = (int)pl.z;
    for (int z = pz - 4; z <= pz + 12; z++)
      for (int x = px - 9; x <= px + 9; x++)
        for (int y = fy - 1; y < fy + 8; y++) world_set(x, y, z, y == fy - 1 ? B_GRASS : B_AIR);
    /* a slab of stone over it, so the daylight does not set it alight: it is its aim the
     * frames are of, not the burn */
    for (int dz = -1; dz <= 1; dz++)
      for (int dx = -1; dx <= 1; dx++) world_set(px + dx, fy + 3, pz + 7 + dz, B_STONE);
    opt.difficulty = 2;
    Entity *e = ent_new(E_SKELETON, px + 0.5f, (float)fy, pz + 7.5f);
    if (e) e->health = 20, e->yaw = 0, e->gx = e->x, e->gz = e->z;   /* (facing +z: away from the player) */
    pl.yaw = 0, pl.pitch = -4;
    world_follow(pl.x, pl.y, pl.z);
    while (world_pending()) world_follow(pl.x, pl.y, pl.z);
    camera_reset();
    fprintf(stderr, "skeleton: it stands at %d %d facing +z (away), the player at %d %d facing it\n", px, pz + 7, px, pz);
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
        for (int y = WORLD_MIN_Y; y < fy; y++) world_set(x, y, z, B_AIR);   /* through the deep layer too */
    pl.x = px + 0.5f, pl.z = pz + 0.5f, pl.y = (float)fy - 8.5f, pl.vy = 0;
    if (pl.mode) pl.flying = true;   /* (creative and spectator hang there; survival falls) */
  }
  if (animals) {
    /* a few animals ahead of the player (forward f, right r blocks), each on the first solid ground under it, facing
     * the player */
    float a = pl.yaw * 3.14159265f / 180, fx = -sinf(a), fz = cosf(a), rx = -cosf(a), rz = -sinf(a);
    static const struct { int type; float f, r; } herd[] = {
      {E_SHEEP, 5.2f, -1.7f}, {E_COW, 7.5f, 1.2f}, {E_PIG, 4.6f, 1.9f}, {E_CHICKEN, 3.6f, -0.6f}};
    for (unsigned k = 0; k < sizeof herd / sizeof herd[0]; k++) {
      float x = pl.x + fx * herd[k].f + rx * herd[k].r, z = pl.z + fz * herd[k].f + rz * herd[k].r;
      for (int y = (int)pl.y + 8; y > (int)pl.y - 12; y--) {
        float box[6] = {x - 0.3f, y - 0.5f, z - 0.3f, x + 0.3f, y - 0.01f, z + 0.3f};
        if (phys_free(box)) continue;
        Entity *e = ent_new(herd[k].type, x, (float)y, z);
        if (e) e->health = 10, e->yaw = pl.yaw + 180 + (float)(k * 37 % 60) - 30, e->gx = e->x, e->gz = e->z;
        break;
      }
    }
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
    if (trace)
      printf("%4d gui %2d mode %d dim %d %8.2f %8.2f %8.2f health %5.1f%s\n", f, gui, pl.mode, world_dim, pl.x, pl.y,
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
