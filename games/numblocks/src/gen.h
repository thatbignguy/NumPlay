/* NumBlocks world generator: a port of Minecraft 1.8.8's default overworld
 * generator (ChunkProviderGenerate and everything it calls), sized for the
 * NumWorks calculator (all state static, no malloc, about 28 KB of RAM).
 * See the comment at the top of gen.c for what differs from Minecraft. */
#ifndef NB_GEN_H
#define NB_GEN_H
#include <stdint.h>

/* Starts a world: seed as a Java long (what /seed prints). Call before anything else. */
void gen_init(int64_t seed);
/* The dimension the generator makes: 0 the overworld, 1 the nether (nb.h's DIM_*).
 * The octaves are reseeded (from a derived seed in the nether), and what the other
 * dimension cached is dropped, so the two never mix. */
void gen_set_dim(int dim);
#define GEN_CACHE_BYTES (25 * 528)   /* the summary cache: on main's stack on the calculator (gen_set_cache) */
void gen_set_cache(void *mem);

/* The world type: 0 Minecraft's default, 1 superflat (FlatGeneratorInfo's default preset
 * "2;7,2x3,2;1": bedrock, two dirt, grass, all plains, nothing else). Kept across gen_init. */
void gen_set_flat(int flat);

/* The test world (nb.h's WT_TEST): the same empty world whatever the seed, holding
 * everything the game has once - every block state in a grid, every mob in a cage of
 * its own and the structures the world makes, each on its own platform. Off is the
 * two above. Kept across gen_init, as the flat mode is. */
void gen_set_test(int test);

/* The test world's layout (nb.h's WT_TEST): where the showcase puts its sections, so
 * the tests can read the world through them. Everything stands on one row,
 * TEST_SECTION_Y, three blocks above the spawn platform's top, and the sections run in
 * one row along +z, in the order of the game's own: the grid of every block state first
 * (starting a bit before the spawn platform and running past it, so the platform is in
 * the middle of the blocks, which are three blocks over its top), then the chests of
 * every item that has no block of its own, then the mobs in their cages, then the
 * structures the world makes. A player on the platform faces the whole row. */
#define TEST_PLATFORM_Y 40           /* the spawn platform: the row its blocks sit on (the player on 41) */
#define TEST_SECTION_Y 43            /* the row the showcase's platforms and the grid's barriers sit on */
#define TEST_BLOCK_Y 44              /* the row the grid's blocks and the item chests float at (their barriers at TEST_SECTION_Y) */
/* the spawn platform: 16 x 16, in the middle of the grid of blocks */
#define TEST_SPAWN_X0 0
#define TEST_SPAWN_Z0 (-31)
#define TEST_SPAWN_X1 (TEST_SPAWN_X0 + 15)
#define TEST_SPAWN_Z1 (TEST_SPAWN_Z0 + 15)
/* the grid of every block state: 15 columns along x, 23 rows along z (the row the
 * showcase runs in) - the 345 block states there are (the ids the block and the item
 * number spaces share - 258 to 395, and the two reserved slots - hold no block of their
 * own), one air gap between each */
#define TEST_GRID_X0 (-7)
#define TEST_GRID_Z0 (-46)
#define TEST_GRID_COLS 15
#define TEST_GRID_ROWS 23
#define TEST_GRID_X1 (TEST_GRID_X0 + (TEST_GRID_COLS - 1) * 2)
#define TEST_GRID_Z1 (TEST_GRID_Z0 + (TEST_GRID_ROWS - 1) * 2)
/* the chests of every item that has no block of its own (the ones that place one are
 * shown in the grid of blocks): TEST_ITEM_CHESTS of them in a row, floating like the
 * blocks, one stack of each of the 136 of them (27 a chest) */
#define TEST_ITEM_X0 5
#define TEST_ITEM_Z 8
#define TEST_ITEM_CHESTS 6
/* the cages: five along x, two along z, 5 x 5 of grass each with one block between
 * them. Each mob stands on the middle block of its own, with a barrier on each of its
 * four sides and one over it */
#define TEST_CAGE_X0 (-7)
#define TEST_CAGE_Z0 18
#define TEST_CAGE_COLS 5
#define TEST_CAGE_ROWS 2
#define TEST_CAGE_STEP 6
#define TEST_CAGE_X1 (TEST_CAGE_X0 + (TEST_CAGE_COLS - 1) * TEST_CAGE_STEP + 4)
#define TEST_CAGE_Z1 (TEST_CAGE_Z0 + (TEST_CAGE_ROWS - 1) * TEST_CAGE_STEP + 4)
#define TEST_CAGE_Y 44      /* the row the mobs' feet stand on (the grass is at TEST_SECTION_Y) */
/* the desert pyramid: its origin chunk (its 21 x 21 starts there, on a chunk boundary
 * as the world's own placement puts it) and the row its base is on */
#define TEST_TEMPLE_OX 0
#define TEST_TEMPLE_OZ 2
#define TEST_TEMPLE_BASE 60
/* the dungeon: the middle of its box, and the row its floor is on */
#define TEST_DUNG_X 3
#define TEST_DUNG_Z 63
#define TEST_DUNG_Y 45
#define TEST_DUNG_SEED 20250614   /* its own stream: the same dungeon every time */

/* The test world's dungeon box: its half sizes, the two draws WorldGenDungeons makes of
 * its own stream first (2..3 each way, so its room is 9 x 9 to 11 x 11). */
void test_dung_box(int *i, int *l);

/* The test world's mob cages (gen.c's test world section): where the k-th of the
 * game's ten mobs (E_ZOMBIE .. E_ZPIGLIN) stands, as the block its feet are on, in
 * the middle of its own 2 x 2 room. k outside 0..9 gives (0, 0, 0). */
void test_cage(int k, int *x, int *y, int *z);

/* Superflat with a preset: either a simple "block,block,..." list (one layer per
 * block, bottom to top, e.g. "bedrock,dirt,dirt,grass") or a 1.8 FlatGeneratorInfo
 * "version;layers;biome;structures" (e.g. "2;7,2x3,2;1", kept for older saves):
 * version ignored, layers comma list of [count 'x'] block, biome decimal id
 * (default 1 plains), structures ignored. A block is a decimal blk_id[:meta] or a
 * simple name like bedrock/dirt/grass/stone/sand/glass. Invalid/NULL falls back
 * to the default preset. Max 16 layer entries, max height 128. */
void gen_set_flat_preset(const char *preset);

/* The world's sea level: the row the water of the default worlds sits at, and the row the rock
 * under the generator's own world ends (below it, a column of the test world's vget is stone).
 * And the desert pyramid's footprint: TEMPLE_SIZE x TEMPLE_SIZE (gen.c's temple_build). */
#define SEA 63
#define TEMPLE_SIZE 21

/* The nether's build height: the top of gen_slab's windows there (one past the highest
 * block the player can put, y 199). Its terrain is the same 128-block column, ceiling
 * and all; everything above the ceiling is the open air the world lets build in, as on
 * Minecraft's nether roof (nb.h's dim_height is this per dimension). */
#define NETHER_H 200

/* The top of the terrain column: y 0..127 in both dimensions (block level 128 is air).
 * The overworld's surface world is unchanged by the deep layer below. */
#define WORLD_TOP 128

/* The overworld's lowest block (1.18's min build height): deepslate runs from y = 0 down
 * to it and bedrock sits on it (nb.h's WORLD_MIN_Y is this). The nether has no deep layer:
 * its floor stays at y = 0, and below 0 there is air. */
#define WORLD_MIN_Y (-64)
/* The deepest row bedrock reaches (1.18: bedrock about -64..-60). */
#define BEDROCK_TOP (-60)

/* The nether's other biomes (1.16's own ids; the wastes are 1.8's Hell, 8):
 * their temperature and rainfall are nominal (the dimension has no weather). */
#define BI_CRIMSON_FOREST 49
#define BI_WARPED_FOREST 50
#define BI_SOUL_SAND_VALLEY 51
#define BI_NETHER_WASTES 8   /* 1.8's Hell (gen.c's BI_HELL): the ground the others sit in */

/* Blocks of chunk (cx, cz) for y in [y0, y0 + h), after terrain, caves, ravines
 * and the decoration (population) that Minecraft would put in this chunk:
 * out[(y - y0) * 256 + z * 16 + x], x and z in 0..15, values from blocks.h (B_*).
 * The overworld and flat generators write y < 128 only; the nether's terrain stops at
 * its ceiling too (y 127), and y0 + h may reach its build height NETHER_H (200) there:
 * everything above the ceiling comes out as air.
 * Cost: work around the chunk is cached, so neighbouring chunks asked one after the
 * other are much cheaper than scattered ones (Cortex-M7 at 216 MHz, h = 24, rows of
 * 3-4 neighbours: about 60-80 ms per call; the first call after gen_init or after a
 * jump to a new place about 190 ms). */
void gen_slab(int cx, int cz, int y0, int h, uint8_t *out);

/* Minecraft biome id at block (x, z) (0 ocean, 1 plains, ... 129+ mutated).
 * Cheap for the chunk of the last gen_slab call, a few ms elsewhere.
 * In the nether the answer is one of 1.16's 49-51 or BI_HELL (8, the wastes). */
int gen_biome(int x, int z);

/* The nether's sky and fog colour at (x, z): 1.16's own colour for the biome the
 * camera is in, faded toward the biome across the edge you are walking to (the fade
 * runs 16 blocks either side of it). Pure function of (x, z). The wastes' red, the
 * forests' and the valley's own; give (r, g, b) it fills. */
void gen_nether_sky_colour(float x, float z, int *r, int *g, int *b);

/* y (0..127) of the highest non-air block of column (x, z) after generation
 * (including trees and other decoration, whatever the y range of the slab asked).
 * Cheap for the chunk of the last gen_slab call; elsewhere it costs a gen_slab. */
int gen_top(int x, int z);

/* Temperature noise at block (x, z): Minecraft's BiomeBase.ae.a(x / 8.0, z / 8.0), the raw
 * simplex value. BiomeBase.a(BlockPosition) is then, for y > 64:
 *   temperature - (gen_temp_noise(x, z) * 4 + y - 64) * 0.05 / 30
 * (and the biome temperature for y <= 64); below 0.15 it snows and water freezes.
 * Cheap (one simplex sample), usable any time after gen_init. */
float gen_temp_noise(int x, int z);

/* The world spawn point, as Minecraft chooses it (WorldChunkManager.findBiomePosition
 * then the random walk until the top block is grass); y is the first air block above it.
 * About 0.4 s on the calculator. */
void gen_spawn(int *x, int *y, int *z);

/* Statistics for tests: bytes of static RAM the generator uses (27108 on the M7). */
unsigned gen_ram_bytes(void);

/* Where the mob of a monster spawner that world generation places is handed out. The generator
 * has no world of its own to remember it in (as Minecraft keeps it in the spawner's tile entity),
 * so it gives it here when it places one, as the world would: (x, y, z) where, and mob what
 * WorldGenDungeons.a's draw made of it (0-1 skeleton, 2 zombie, 3 spider). Nobody has to set it:
 * the tests leave it unset, and nothing in them asks a spawner for its mob. */
extern void (*gen_spawner_hook)(int x, int y, int z, int mob);

/* The desert pyramid's placement grid (gen.c's temple_origin): is the chunk holding
 * (x, z) a cell the modern random spread picks? Flat worlds and the nether: no. */
int temple_cell(int x, int z);

/* The biomes' names and the keys /locate biome takes (gen.c's own tables): the F3
 * line's name for an id, the id of a key (-1: none), and the i-th key (0: past the
 * last), for the chat's finish. */
const char *gen_biome_name(int id);
int gen_biome_id(const char *key);
const char *gen_biome_key(int i);
int gen_biome_known(int id);   /* an id the world may hold (0: no such biome) */
/* /locate's searches: the nearest block with a biome (fy: the ground there) and the
 * nearest desert pyramid, to (x, z); 1 and the place when one is found. */
int gen_find_biome(int id, int x, int z, int *fx, int *fz, int *fy);
int gen_find_temple(int x, int z, int *fx, int *fz);

/* The generator's scratch: its passes never overlap (layers, then terrain, then caves and
 * populations), so they share one buffer, which the caller gives it -- a local of a function
 * that makes the world is what it is for, and where the game would otherwise keep 7.7 KB of
 * it in the app's RAM. Call gen_scratch(buf), buf at least GEN_SCRATCH bytes, before any other
 * gen_* call below, and keep it alive for the whole of the calls that follow (the world keeps
 * it in world_follow's frame for a move, and in world_spawn's for the spawn search, and gives
 * NULL when it is done: a call without a live scratch faults instead of writing nowhere). */
#define GEN_SCRATCH 7684
void gen_scratch(void *buf);

#endif
