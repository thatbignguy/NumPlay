/* The Final Earth 2: the backdrop and the worlds' islands.
 *
 * Ported from the original: the sky's colour over the day (CityTime), the
 * stars (graphics.Stars), the planets and moons (graphics.BackgroundImages),
 * the clouds (graphics.Cloud), each world's island (graphics.WorldImage, made
 * from its seed with the original's random.SeedeableRandom, so a seed gives
 * the same island) and the grass and other decorations on the ground
 * (graphics.GroundDecoration, World.setDecoration).
 *
 * Layers, back to front, as the original's City stacks them: bg_draw_sky
 * (sky, stars, planets, the three cloud layers), then the city: buildings'
 * backs, citizens inside, buildings, then bg_draw_world for each world (its
 * island and decorations, in the same stage as the buildings' fronts; they
 * don't overlap), then citizens outside. */
#ifndef FE_BG_H
#define FE_BG_H
#include <stdbool.h>
#include <stdint.h>
#include "gfx.h"

/* the camera: the city position of the screen's top-left, and the zoom (1 or 2).
 * The original's viewPos (the screen's centre) is (x + SCREEN_W / 2 / zoom, y + SCREEN_H / 2 / zoom). */
typedef struct {
  float x, y;
  int zoom;
} BgCamera;
/* screen position of a city position (the original's moving view: its offset floored, then the
 * nearest pixel); for buildings, citizens and everything else in the city */
static inline int bg_floor(float v) { int i = (int)v; return i - (v < (float)i); }
static inline int bg_screen_x(const BgCamera *c, float x) { return bg_floor(x * c->zoom + 0.5f) + bg_floor(-c->x * c->zoom); }
static inline int bg_screen_y(const BgCamera *c, float y) { return bg_floor(y * c->zoom + 0.5f) + bg_floor(-c->y * c->zoom); }

/* City.getCityEdges() (the worlds and what is built on them), which the clouds spread over */
typedef struct { int minX, maxX, minY, maxY; } BgEdges;

/* ---------------------------------------------------------------- the sky */
void bg_init(uint32_t seed);              /* seeds the stars and clouds (the original uses Math.random) */
uint32_t bg_sky_color(float hour);        /* CityTime.getCurrentColor: 0xRRGGBB at this hour (0 to 24) */
float bg_stars_alpha(float hour);         /* Stars.update: the stars' stage alpha */
float bg_clouds_alpha(float hour);        /* Stars.update: cloudsAlpha */

/* a story's planets and moons (its backgroundSprites) */
typedef struct {
  uint16_t spr;            /* SPR_PLANETS_SMALL... */
  uint16_t width;          /* its "width" (frames are cut as getTexturesByWidth does) */
  uint8_t image;           /* the frame shown */
  int16_t x, y;            /* place at time 0 */
  float velocity;
} BgPlanet;
#define BG_MAX_PLANETS 8
void bg_set_planets(const BgPlanet *p, int n);

/* the clouds: bg_clouds_start once the worlds are made (BackgroundImages.postCreate: the sky
 * filled with clouds as if they had always been there), bg_clouds_update each tick with the
 * original's timeMod times the game speed (BackgroundImages.update: they drift right, new ones
 * come from the left) */
#define BG_MAX_CLOUDS 128
void bg_clouds_start(const BgEdges *e);
void bg_clouds_update(float timeMod, const BgEdges *e);
int bg_cloud_count(void);

/* sky, stars, planets and clouds; time: CityTime.timeSinceStart (minutes) */
void bg_draw_sky(const BgCamera *cam, double timeSinceStart);

/* ---------------------------------------------------------------- the worlds */
enum { BG_LOOK_NORMAL, BG_LOOK_KEY, BG_LOOK_KEY2, BG_LOOK_ROCK, BG_LOOK_ROCK_LESS_JAGGED, BG_LOOK_SNOW };
int bg_look(const char *appearance);      /* the original's appearance string ("", "key", "rock:lessJagged"...) */

/* A world's look. Its island is made from the rect, the seed and the look when it
 * is first drawn (or its mask asked for), and kept in a cache of BG_SHAPE_POOL
 * bytes: about 3 bytes for each pixel column of island (theLostShip's 440 take
 * 1314 bytes, FreePlayManyWorlds' 21 worlds 12.5 KB in all). Worlds not drawn for
 * a while give their room to others and are made again when seen (some ms for a
 * wide one); the islands one frame draws must fit together, or the last ones
 * aren't drawn. A world must stay at the same address while its island is cached
 * (bg_forget_shapes() after moving or reloading worlds). */
typedef struct {
  int16_t x, y, w, h;      /* rect in city pixels; h 0: an invisible world (no island, no mask) */
  int32_t seed;
  uint8_t look;            /* BG_LOOK_* */
  uint8_t shape;           /* bg.c's: the cache slot of its island + 1, 0 if none */
} BgWorld;
#define BG_SHAPE_POOL 6144
#define BG_SHAPE_SLOTS 32
void bg_forget_shapes(void);

/* ground decorations: a byte per 20-pixel column, 0 for none, else kind << 4 | frame */
enum { DECO_GRASS = 1, DECO_FLOWERS, DECO_FLOWERS_BLUE, DECO_ROCKS, DECO_KEYCITY, DECO_FLOWERS_KEYCITY,
       DECO_PURPLEGRASS, DECO_REDGRASS, DECO_KINDS };
int bg_deco_kind(const char *textureName);   /* "spr_grass"... (0 if none) */
uint16_t bg_deco_sprite(int kind);
int bg_deco_frames(int kind);
/* World.setDecoration: a decoration at column col (frame < 0: a random one); does nothing if
 * the column has a permanent at the bottom (has_ground_permanent), as the original */
void bg_set_decoration(uint8_t *decos, int col, int kind, int frame, bool has_ground_permanent);
/* the new World's grass on every column (none for the snow look) */
void bg_world_decorate(const BgWorld *w, uint8_t *decos);

/* the island and the decorations (decos: one byte per column, or NULL) */
void bg_draw_world(BgWorld *w, const uint8_t *decos, const BgCamera *cam);

/* WorldImage's collision mask: whether the 20x20 cell (cx, cy) of the world's rect holds any of
 * its island (Builder.canBuildBuilding); false for invisible worlds. Makes the island if it isn't
 * cached. */
bool bg_world_mask(BgWorld *w, int cx, int cy);
#endif
