/* NumPowder: the simulation (The Powder Toy's Simulation, Air and gravity), in C.
 *
 * Ported element code reads like the original's with these changes:
 * - `sim->` goes: the simulation's state and functions are globals (create_part, kill_part, pv, bmap...);
 * - `pmap[y][x]` and `photons[y][x]` are read with PMAPAT(x, y) and PHOTAT(x, y), which give the original's
 *   values (PMAP(id, type), 0 for none), and written with PMAPSET(x, y, r) and PHOTSET(x, y, r) (the
 *   simulation's code only: the interface reads them);
 * - `parts[i].x`, `.y`, `.vx`, `.vy` and `.temp` are fixed point: read them with p_x(&parts[i])...,
 *   write them with p_set_x(&parts[i], v)... (simdefs.h); the other fields are plain ints;
 * - air, gravity and fan cells are ACELL wide, wall cells WCELL: `sim->pv[y/CELL][x/CELL]` becomes
 *   pv[y/ACELL][x/ACELL], `sim->bmap[y/CELL][x/CELL]` becomes bmap[y/WCELL][x/WCELL]; the edge where
 *   particles die (the original's `x < CELL`...) is BORDER wide;
 * - `sim->rng.between(a, b)` is rng_between(a, b), `.chance(n, d)` rng_chance(n, d), `.gen()` rng_gen(),
 *   `()` rng_u32(), `.uniform01()` rng_uniform01();
 * - the element table is `elements[t]` (elements.h), its fields named as the original's;
 * - NPART is npart_max, `parts.active` parts_active. */
#ifndef NP_SIM_H
#define NP_SIM_H
#include "simdefs.h"
#include "elements.h"

/* ---------------------------------------------------------------- the particles */
extern Particle *parts;              /* npart_max of them (the RAM the calculator gives decides how many) */
extern int npart_max;
extern int parts_active;             /* the original's parts.active: ids from 0 to parts_active - 1 are in use */
extern int parts_lastActiveIndex;    /* the highest id in use (parts_active - 1) */
extern int pfree;                    /* the free list (as the original's) */
extern int NUM_PARTS;
extern int elementCount[PT_NUM];
/* the maps: an id + 1 per particle cell (0: none); pmap for everything but energy, photons for energy.
 * main() may point pmap_id at a YRES x XRES array on its stack before sim_init (or sim_init takes it from
 * the heap) */
extern uint16_t (*pmap_id)[XRES];
extern uint16_t (*photons_id)[XRES];
static inline int PMAPAT(int x, int y) {
  unsigned id = pmap_id[y][x];
  return id ? PMAP((int)id - 1, parts[id - 1].type) : 0;
}
static inline int PHOTAT(int x, int y) {
  unsigned id = photons_id[y][x];
  return id ? PMAP((int)id - 1, parts[id - 1].type) : 0;
}
/* the original's `pmap[y][x] = r` (r a PMAP value, or 0) */
static inline void PMAPSET(int x, int y, int r) { pmap_id[y][x] = r ? (uint16_t)(ID(r) + 1) : 0; }
static inline void PHOTSET(int x, int y, int r) { photons_id[y][x] = r ? (uint16_t)(ID(r) + 1) : 0; }

/* ---------------------------------------------------------------- air, heat, walls, gravity */
extern float pv[YACELLS][XACELLS], vx[YACELLS][XACELLS], vy[YACELLS][XACELLS];   /* pressure and air velocity */
extern float hv[YACELLS][XACELLS];   /* ambient heat */
extern float fvx[YACELLS][XACELLS], fvy[YACELLS][XACELLS];   /* fans */
extern unsigned char bmap[YWCELLS][XWCELLS];   /* walls (WL_*) */
extern unsigned char emap[YWCELLS][XWCELLS];   /* walls' electricity */
extern float gravx[YACELLS][XACELLS], gravy[YACELLS][XACELLS];   /* Newtonian gravity's field (gravOut) */
extern float gravmass[YACELLS][XACELLS];   /* its masses, this frame (gravIn.mass) */
extern uint8_t gravmask[YACELLS][XACELLS];   /* 0 inside a box of gravity walls (gravIn.mask) */
extern uint8_t bmap_blockair[YACELLS][XACELLS], bmap_blockairh[YACELLS][XACELLS];   /* Air's */

/* ---------------------------------------------------------------- the simulation's settings */
enum { EDGE_VOID, EDGE_SOLID, EDGE_LOOP, NUM_EDGEMODES };
enum { GRAV_VERTICAL, GRAV_OFF, GRAV_RADIAL, GRAV_CUSTOM, NUM_GRAVMODES };
enum { AIR_ON, AIR_PRESSUREOFF, AIR_VELOCITYOFF, AIR_OFF, AIR_NOUPDATE, NUM_AIRMODES };
enum { AIRC_NONE, AIRC_LEGACY, AIRC_BOUSSINESQ, NUM_CONVMODES };
typedef struct {
  bool heat;               /* legacy_enable is !heat */
  bool ambient_heat;       /* aheat_enable */
  bool newtonian_gravity;
  bool water_equalization;
  uint8_t air_mode, gravity_mode, edge_mode;
  float custom_gravity_x, custom_gravity_y;
  float ambient_air_temp;  /* Kelvin */
  /* the original's newer air options (its defaults: 0, 0, 0, 0.1, AIRC_BOUSSINESQ) */
  float edge_pressure, edge_velocity_x, edge_velocity_y, vorticity;
  uint8_t convection_mode;
} SimSettings;
/* the interface changes `settings`, then calls sim_settings_changed() (sim_step also applies them) */
extern SimSettings settings;
void sim_settings_changed(void);
void sim_default_settings(void);
/* the original's variables, kept from `settings` */
extern int legacy_enable, aheat_enable, water_equal_test, edgeMode, gravityMode;
extern float customGravityX, customGravityY;
extern bool ngrav_enable;            /* Newtonian gravity on (the original's bool(grav)) */
extern int currentTick;
extern uint32_t frameCount;
extern bool sys_pause;
extern int emp_decor;                /* EMP's flash, for the renderer */

/* ---------------------------------------------------------------- random numbers (the original's RNG) */
unsigned int rng_gen(void);          /* .gen(): 31 bits */
unsigned int rng_u32(void);          /* (): 32 bits */
int rng_between(int lower, int upper);
bool rng_chance(int numerator, unsigned int denominator);
float rng_uniform01(void);
void rng_seed(unsigned int seed);
extern uint64_t rng_state[2];

/* ---------------------------------------------------------------- the original's functions */
int create_part(int p, int x, int y, int t, int v);   /* v: -1 when the original's caller gives none */
int createPartTempVel(int i, int x, int y, int t);
void kill_part(int i);
bool part_change_type(int i, int x, int y, int t);
void delete_part(int x, int y);
int eval_move(int pt, int nx, int ny, unsigned *rr);
int try_move(int i, int x, int y, int nx, int ny);
int do_move(int i, int x, int y, float nxf, float nyf);
bool sim_move(int i, int x, int y, float nxf, float nyf);
int parts_avg(int ci, int ni, int t);
void create_gain_photon(int pp);
void create_cherenkov_photon(int pp);
void GetGravityField(int x, int y, float particleGrav, float newtonGrav, float *pGravX, float *pGravY);
bool IsWallBlocking(int x, int y, int type);
void set_emap(int x, int y);
int flood_prop_int(int x, int y, int field, int value);   /* flood_prop with an int field (FIELD_*) */
int FloodINST(int x, int y);
bool flood_water(int x, int y, int i);
void CreateLine(int x1, int y1, int x2, int y2, int c);
int get_wavelength_bin(int *wm);
void photoelectric_effect(int nx, int ny);
void SetEdgeMode(int newEdgeMode);
void RecalcFreeParticles(bool do_life_dec);
void clear_area(int area_x, int area_y, int area_w, int area_h);
static inline bool IsElement(int type) { return type > 0 && type < PT_NUM && elements[type].Enabled; }
static inline bool IsElementOrNone(int type) { return type >= 0 && type < PT_NUM && elements[type].Enabled; }

/* ---------------------------------------------------------------- running it */
void sim_init(void);                 /* the particles' room: before anything else */
void sim_clear(void);                /* clear_sim */
void sim_step(void);                 /* a frame: BeforeSim, UpdateParticles, AfterSim */
void sim_frame(bool update);         /* a frame shown: sim_step, or (paused) only BeforeSim(false) */

/* ---------------------------------------------------------------- walls (WallType.h, SimulationData.cpp) */
enum { WL_ERASE, WL_WALLELEC, WL_EWALL, WL_DETECT, WL_STREAM, WL_FAN, WL_ALLOWLIQUID, WL_DESTROYALL, WL_WALL,
       WL_ALLOWAIR, WL_ALLOWPOWDER, WL_ALLOWALLELEC, WL_EHOLE, WL_ALLOWGAS, WL_GRAV, WL_ALLOWENERGY, WL_BLOCKAIR,
       WL_ERASEALL, WL_STASIS, UI_WALLCOUNT, WL_FLOODHELPER = 255 };
typedef struct {
  uint32_t colour, eglow;   /* 0xRRGGBB; eglow is added to the fire glow when the wall is powered */
  uint8_t drawstyle;
  const char *name, *identifier, *descs;
} WallType;
extern const WallType walltypes[UI_WALLCOUNT];

/* ---------------------------------------------------------------- tools (simtools/), in the original's order */
enum { TOOL_HEAT, TOOL_COOL, TOOL_AIR, TOOL_VAC, TOOL_PGRV, TOOL_NGRV, TOOL_MIX, TOOL_CYCL, TOOL_AMBM, TOOL_AMBP,
       TOOL_WIND, TOOL_COUNT };
typedef struct { const char *identifier, *name; uint32_t colour; const char *description; } SimToolInfo;
extern const SimToolInfo simtools[TOOL_COUNT];

/* ---------------------------------------------------------------- Game of Life (BuiltinGOL.h, GOLString.cpp) */
#define NGOL 24
typedef struct { const char *name; int oldtype, ruleset; uint32_t colour, colour2; const char *description; } BuiltinGOL;
extern const BuiltinGOL builtinGol[NGOL];
int ParseGOLString(const char *value);          /* "B3/S23" (or "B3/S23/2"): a rule, -1 if invalid */
void SerialiseGOLRule(int rule, char *out);     /* (out: 24 chars) */

/* ---------------------------------------------------------------- drawing with the brush (Editing.cpp, the tools)
 * A tool is an element (the original's ToolID: PMAP(v, type), so PMAP(n, PT_LIFE) is the nth Game of Life
 * type and PT_NONE erases), TOOL_WALLS + WL_* or TOOL_SIMTOOLS + TOOL_*. */
#define TOOL_WALLS 0x40000000
#define TOOL_SIMTOOLS 0x20000000
enum { BRUSH_CIRCLE, BRUSH_SQUARE, BRUSH_TRIANGLE, NUM_BRUSHES };
typedef struct { int rx, ry; uint8_t shape; } Brush;
extern Brush brush;                  /* radii 0 to 200, as the original's */
bool brush_has(int dx, int dy);      /* is (dx, dy) in the brush (-rx..rx, -ry..ry) */
extern float tool_strength;          /* the original's: 1, shift 10, ctrl 0.1 */
/* replace mode (REPLACE_MODE) and specific delete (SPECIFIC_DELETE) of replaceModeSelected */
enum { REPLACE_MODE = 1, SPECIFIC_DELETE = 2 };
extern int replaceModeSelected, replaceModeFlags;
/* GameController::DrawPoints: a press (held false: Tool::Draw) or the cursor held (Tool::DrawLine dragging),
 * every frame while the button is down */
void sim_tool_points(int tool, int x1, int y1, int x2, int y2, bool held);
void sim_tool_line(int tool, int x1, int y1, int x2, int y2);   /* a line drawn (shift-drag) */
void sim_tool_box(int tool, int x1, int y1, int x2, int y2);    /* a box drawn (ctrl-drag) */
void sim_tool_fill(int tool, int x, int y);                     /* flood fill */
/* the element tools themselves (the original's CreateParts, CreateLine, CreateBox, FloodParts) */
int CreateParts(int p, int x, int y, int c, int flags);         /* with the brush; flags -1: replaceModeFlags */
int CreatePartsR(int p, int x, int y, int rx, int ry, int c, int flags);
void CreateLineBrush(int x1, int y1, int x2, int y2, int c, int flags);
void CreateBox(int p, int x1, int y1, int x2, int y2, int c, int flags);
int FloodParts(int x, int y, int c, int cm, int flags);
int CreateWalls(int x, int y, int rx, int ry, int wall, bool withBrush);
void CreateWallLine(int x1, int y1, int x2, int y2, int rx, int ry, int wall, bool withBrush);
void CreateWallBox(int x1, int y1, int x2, int y2, int wall);
int FloodWalls(int x, int y, int wall, int bm);

/* ---------------------------------------------------------------- stickmen (Stickman.h) */
#define MAX_FIGHTERS 16              /* (the original's is 100) */
typedef struct {
  char comm, pcomm;        /* commands: 1 left, 2 right, 4 up (jump), 8 down (use the element) */
  int elem;                /* the element it uses */
  float legs[16];          /* the legs' positions */
  float accs[8];
  char spwn;               /* spawned */
  unsigned int frames;     /* since the last LIGH it made */
  bool rocketBoots, fan;
  int spawnID;             /* its SPWN */
} playerst;
extern playerst player, player2, fighters[MAX_FIGHTERS];
extern unsigned char fighcount;
/* the arrow keys for STKM (who 0) or STK2 (who 1): GameController::KeyPress and KeyRelease */
enum { STKM_LEFT = 1, STKM_RIGHT = 2, STKM_UP = 4, STKM_DOWN = 8 };
void stkm_key(int who, int key, bool down);
/* every frame (GameController::Update): a stickman not out uses the right tool's element when it comes */
void stkm_frame(int rightElement);
#endif
