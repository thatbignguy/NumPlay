/* NumBlocks world generator: a port of Minecraft 1.8.8's default overworld
 * generator (ChunkProviderGenerate and everything it calls), sized for the
 * NumWorks calculator (all state static, no malloc, about 28 KB of RAM).
 * See the comment at the top of gen.c for what differs from Minecraft. */
#ifndef NB_GEN_H
#define NB_GEN_H
#include <stdint.h>

/* Starts a world: seed as a Java long (what /seed prints). Call before anything else. */
void gen_init(int64_t seed);
#define GEN_CACHE_BYTES (25 * 528)   /* the summary cache: on main's stack on the calculator (gen_set_cache) */
void gen_set_cache(void *mem);

/* The world type: 0 Minecraft's default, 1 superflat (FlatGeneratorInfo's default preset
 * "2;7,2x3,2;1": bedrock, two dirt, grass, all plains, nothing else). Kept across gen_init. */
void gen_set_flat(int flat);

/* Superflat with a preset: either a simple "block,block,..." list (one layer per
 * block, bottom to top, e.g. "bedrock,dirt,dirt,grass") or a 1.8 FlatGeneratorInfo
 * "version;layers;biome;structures" (e.g. "2;7,2x3,2;1", kept for older saves):
 * version ignored, layers comma list of [count 'x'] block, biome decimal id
 * (default 1 plains), structures ignored. A block is a decimal blk_id[:meta] or a
 * simple name like bedrock/dirt/grass/stone/sand/glass. Invalid/NULL falls back
 * to the default preset. Max 16 layer entries, max height 128. */
void gen_set_flat_preset(const char *preset);

/* Blocks of chunk (cx, cz) for y in [y0, y0 + h), after terrain, caves, ravines
 * and the decoration (population) that Minecraft would put in this chunk:
 * out[(y - y0) * 256 + z * 16 + x], x and z in 0..15, values from blocks.h (B_*).
 * Only y < 128 is ever written (y0 + h must be <= 128).
 * Cost: work around the chunk is cached, so neighbouring chunks asked one after the
 * other are much cheaper than scattered ones (Cortex-M7 at 216 MHz, h = 24, rows of
 * 3-4 neighbours: about 60-80 ms per call; the first call after gen_init or after a
 * jump to a new place about 190 ms). */
void gen_slab(int cx, int cz, int y0, int h, uint8_t *out);

/* Minecraft biome id at block (x, z) (0 ocean, 1 plains, ... 129+ mutated).
 * Cheap for the chunk of the last gen_slab call, a few ms elsewhere. */
int gen_biome(int x, int z);

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
