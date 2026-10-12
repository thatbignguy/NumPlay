/* NumPowder: what the simulation's files share and the interface doesn't need: the original's helpers (Misc.h),
 * the can_move table, the elements' shared state and the functions elements call in each other. */
#ifndef NP_SIMPRIV_H
#define NP_SIMPRIV_H
#include <stdlib.h>
#include <string.h>
#include "sim.h"

/* ---------------------------------------------------------------- Misc.h */
static inline float restrict_flt(float f, float min, float max) {
  if (!isfinite(f)) return min;
  if (f < min) return min;
  if (f > max) return max;
  return f;
}
static inline int isign_i(int i) { return i < 0 ? -1 : i > 0 ? 1 : 0; }
static inline int isign_f(float i) { return i < 0 ? -1 : i > 0 ? 1 : 0; }
#define isign(v) _Generic((v), float: isign_f, double: isign_f, default: isign_i)(v)
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline int iclamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline unsigned clamp_flt(float f, float min, float max) {
  if (f < min) return 0;
  if (f > max) return 255;
  return (unsigned)(int)(255.0f * (f - min) / (max - min));
}
#define NP_PI 3.14159265358979323846f
/* a position as a particle keeps it (1/64 particle): the cell a moved particle ends up in is worked out from
 * this, so it's the one that was checked */
static inline float q64(float v) { return np_q16(v, P_POS) * (1 / P_POS); }

/* a temperature kept to its 1/4 K with dithering, where the original adds small amounts every frame (heat
 * conduction, ambient heat, URAN, POLO...): rounded to the nearest, a change under 1/8 K would be lost; dithered,
 * it's kept on average. The dither has its own generator, so the original's random numbers stay the same. */
extern uint32_t temp_dither;
static inline void p_set_temp_d(Particle *p, float t) {
  t = !(t >= MIN_TEMP) ? MIN_TEMP : t > MAX_TEMP ? MAX_TEMP : t;
  if (t >= MAX_TEMP) {
    p->temp = T_MAXQ;
    return;
  }
  float s = (t + T_OFFSET) * P_TEMP;
  uint32_t q = (uint32_t)s;
  temp_dither ^= temp_dither << 13;
  temp_dither ^= temp_dither >> 17;
  temp_dither ^= temp_dither << 5;
  if ((float)(temp_dither >> 8) * (1.0f / 16777216.0f) < s - (float)q) q++;
  p->temp = (uint16_t)q;
}

/* clears n bytes (a multiple of 4, word aligned) a word at a time: the library's memset goes a byte at a time */
static inline void clear_words(void *p, size_t n) {
  uint32_t *d = (uint32_t *)p, *e = d + n / 4;
  while (e - d >= 4) d[0] = d[1] = d[2] = d[3] = 0, d += 4;
  while (d < e) *d++ = 0;
}

/* ---------------------------------------------------------------- SimulationConfig.h */
#define NPART npart_max
#define MAXSIGNS 16
#define AIR_TSTEPP 0.3f
#define AIR_TSTEPV 0.4f
#define AIR_VADV 0.3f
#define AIR_VLOSS 0.999f
#define AIR_PLOSS 0.9999f
#define SURF_RANGE 10
#define NORMAL_MIN_EST 3
#define NORMAL_INTERP 20
#define NORMAL_FRAC 16
#define REFRACT 0x80000000u
#define GLASS_IOR 1.9f
#define GLASS_DISP 0.07f
#define CHANNELS 101                  /* the original's int(MAX_TEMP - 73) / 100 + 2 */
#define OLD_SPC_AIR 236
#define SPC_AIR 256

/* the original's GT_* (old ids of the Game of Life types) */
enum { GT_GOL = 78, GT_HLIF, GT_ASIM, GT_2x2, GT_DANI, GT_AMOE, GT_MOVE, GT_PGOL, GT_DMOE, GT_34, GT_LLIF, GT_STAN,
       GT_SEED = 134, GT_MAZE, GT_COAG, GT_WALL, GT_GNAR, GT_REPL, GT_MYST, GT_LOTE = 142, GT_FRG2, GT_STAR, GT_FROG,
       GT_BRAN };

/* ---------------------------------------------------------------- can_move (SimulationData::init_can_move),
 * 2 bits a pair in flash (canmove_data.c, made by tools/gen_elements.py) */
#define CANMOVE_STRIDE ((PT_COUNT + 3) / 4)
extern const uint8_t can_move_table[PT_COUNT][CANMOVE_STRIDE];
static inline int can_move(int moving, int dest) {
  return (can_move_table[moving][dest >> 2] >> ((dest & 3) * 2)) & 3;
}

/* ---------------------------------------------------------------- SimulationData's */
static inline bool IsHeatInsulator(const Particle *p);
float HeatCapacityOf(const Particle *p);

/* ---------------------------------------------------------------- the simulation's own state */
extern int ISWIRE;
extern bool emap_any12;   /* a wall cell's electricity is at 12 this frame (BeforeSim): it sparks conductors */
extern bool force_stacking_check;
extern int emp_trigger_count;
extern bool etrd_count_valid;
extern int etrd_life0_count;
extern int lightningRecreate;
extern bool gravWallChanged;
extern bool elementRecount;
extern uint8_t wireless[CHANNELS][2];
extern int CGOL, GSPEED;
extern int Element_PPIP_ppip_changed;
extern int16_t Element_PSTN_tempParts[XRES > YRES ? XRES : YRES];
extern uint8_t Element_LOLZ_lolz[XRES / 9][YRES / 9];
extern uint8_t Element_LOVE_love[XRES / 9][YRES / 9];
/* scratch RAM shared by the passes that need some for a moment (flood fills, the air's next state, the stacking
 * check's counts, the Game of Life): SCRATCH bytes, word aligned */
#define SCRATCH (2 * XACELLS * YACELLS * 4 + 2 * XACELLS * 4)
extern uint8_t *sim_scratch;

/* the flood fills' stack (CoordStack), in sim_scratch after its first `from` bytes */
void flood_stack_init(int from);
bool flood_stack_push(int x, int y);   /* false when full */
void flood_stack_pop(int *x, int *y);
int flood_stack_size(void);

/* the portals' particles (the original's portalp[CHANNELS][8][80]: 80 places for each channel and direction): a pool of
 * PORTAL_SLOTS shared by all, each knowing its channel, direction and place */
#define PORTAL_SLOTS 64
typedef struct { uint8_t channel, dir, slot; Particle p; } PortalSlot;
extern PortalSlot portalp[PORTAL_SLOTS];
extern int portal_count;
extern const int portal_rx[8], portal_ry[8];
bool portal_free(int channel, int dir);                     /* is there a free place */
bool portal_put(int channel, int dir, const Particle *p);   /* in the first free place (PRTI); false if none */
int portal_find(int channel, int dir, int slot);            /* the pool's entry at that place, -1 none */
void portal_take(int entry);
bool portal_any(int channel);

/* tmp4, which the 24-byte particle doesn't keep, for the few elements that use it: a small table by particle id */
int tmp4_get(int i);
void tmp4_set(int i, int v);
void tmp4_clear(int i);

/* ---------------------------------------------------------------- core functions elements use */
int get_normal_interp_ok(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny);
void detach(int i);
bool FloodFillPmapCheck(int x, int y, int type);
void UpdateParticles(int start, int end);
void BeforeSim(bool willUpdate);
void AfterSim(void);
void CheckStacking(void);
void SimulateGoL(void);
void sim_flatten_parts(void);
int parts_alloc(void);
static inline bool parts_full(void) { return pfree == -1 && parts_active >= NPART; }   /* Parts::MaxPartsReached */
void UpdateGravityMask(void);
#if defined(SIM_SCENE) || defined(HOST)
void sim_scene(int n);   /* the speed tests' scenes (sim.c), also the tests' */
#endif
extern float grav_work_mass[YACELLS][XACELLS];   /* the masses handed to the gravity's worker, and its state */
extern uint8_t grav_work_mask[YACELLS][XACELLS];
extern bool grav_work_pending, grav_force_recalc;
void grav_exchange(void);
void grav_reset(void);
void grav_enable(bool enable);
void air_update(void);
void air_update_heat(void);
void air_clear(void);
void air_clear_heat(void);
void air_make_kernel(void);
void air_invert(void);
void air_approximate_block_maps(void);
void air_walls_block(void);
extern float ambientAirTemp, edgePressure, edgeVelocityX, edgeVelocityY, vorticityCoeff;
extern int airMode, convectionMode;

/* ---------------------------------------------------------------- elements' functions other elements call */
int Element_FILT_interactWavelengths(Particle *cpart, int origWl);
int Element_FILT_getWavelengths(const Particle *cpart);
void Element_PIPE_transfer_part_to_pipe(Particle *part, Particle *pipe);
void Element_PIPE_transfer_pipe_to_part(Particle *pipe, Particle *part, bool STOR);
void Element_PIPE_transformPatchOffsets(Particle *part, const int offsets[8]);
void Element_PPIP_flood_trigger(int x, int y, int sparkedBy);
void Element_EMP_Trigger(int triggerCount);
void Element_STKM_init_legs(playerst *playerp, int i);
void Element_STKM_set_element(playerst *playerp, int element);
int Element_STKM_run_stickman(playerst *playerp, UPDATE_FUNC_ARGS);
void Element_STKM_interact(playerst *playerp, int i, int x, int y);
bool Element_FIGH_CanAlloc(void);
int Element_FIGH_Alloc(void);
void Element_FIGH_NewFighter(int fighterID, int i, int elem);
void Element_SOAP_detach(int i);
void SOAP_neighbourLoop(float *dx, float *dy);   /* Element_SOAP_neighourLoop (the renderer's too) */
int Element_LIFE_createAllowed_ok(void);
bool Element_PCLN_ctypeDraw(CTYPEDRAW_FUNC_ARGS);
int Element_COAL_update(UPDATE_FUNC_ARGS);
int Element_NTCT_update(UPDATE_FUNC_ARGS);
int Element_ETRD_nearestSparkablePart(int targetId);
int Element_FIRE_update(UPDATE_FUNC_ARGS);
int Element_PIPE_update(UPDATE_FUNC_ARGS);
int Element_VIRS_update(UPDATE_FUNC_ARGS);
bool basicCtypeDraw(CTYPEDRAW_FUNC_ARGS);
bool ctypeDrawVInTmp(CTYPEDRAW_FUNC_ARGS);
bool ctypeDrawVInCtype(CTYPEDRAW_FUNC_ARGS);
int Element_legacyUpdate(UPDATE_FUNC_ARGS);

/* PLNT's and SEED's genes (PLNT.h): ctype's bits 0 to 19 (bits 20 to 31 keep part of tmp4: see tmp4_get) */
#define PLNT_PHASE 1
#define PLNT_DIR 3
#define PLNT_COLOUR 6
#define PLNT_LIFE 12
#define PLNT_TOTAL_CTP 20
#define PLNT_TOTAL_TMP 15
#define PLNT_CTYPE 0xFFFFF
/* directions counter-clockwise from the top: 107 / 2.6 / 345 */
static const struct { int8_t X, Y; } dir3x3[8] = {{0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}};
int Element_PLNT_detectDown(int x, int y);

/* PIPE's and PPIP's flags (PIPE.h), moved to fit 16 bits of tmp, and 8 of flags */
#define PFLAG_CAN_CONDUCT 0x0001          /* (the original's 0x00000001) */
#define PIPE_SINGLE 0x0002                /* a single pixel pipe (0x00000100) */
#define PIPE_FWD_SINGLE 0x0004            /* transfers like a single pixel pipe going forward (0x00000200) */
#define PIPE_FWD_SHIFT 3                  /* and its direction (0x00001C00) */
#define PIPE_REV_SINGLE 0x0040            /* the same in reverse (0x00002000, 0x0001C000) */
#define PIPE_REV_SHIFT 7
#define PIPE_FWD_GROUP 0x003C             /* the forward and reverse groups, swapped when a PPIP reverses */
#define PIPE_REV_GROUP 0x03C0
#define PFLAG_INITIALIZING 0x0400         /* colours not set yet (0x00020000) */
#define PFLAG_COLOR_RED 0x0800            /* (0x00040000...) */
#define PFLAG_COLOR_GREEN 0x1000
#define PFLAG_COLOR_BLUE 0x1800
#define PFLAG_COLORS 0x1800
#define PPIP_TMPFLAG_REVERSED 0x2000      /* (0x01000000) */
#define PPIP_TMPFLAG_PAUSED 0x4000        /* (0x02000000) */
/* in flags (bit 0 is FLAG_STAGNANT): this frame's triggers, the next frame's (the original's tmp << 3), and
 * PFLAG_NORMALSPEED (the original's flags 0x00010000) */
#define PPIP_TMPFLAG_TRIGGER_REVERSE 0x02
#define PPIP_TMPFLAG_TRIGGER_OFF 0x04
#define PPIP_TMPFLAG_TRIGGER_ON 0x08
#define PPIP_TMPFLAG_TRIGGERS 0x0E
#define PPIP_LATE_SHIFT 3
#define PFLAG_NORMALSPEED 0x80
void pipe_ovf_clear(int i);
void pipe_ovf_move(int from, int to);

static inline bool IsHeatInsulator(const Particle *p) {
  return elements[p->type].HeatConduct == 0 || (p->type == PT_HSWC && p->life != 10) ||
         ((p->type == PT_PIPE || p->type == PT_PPIP) && (p->tmp & PFLAG_CAN_CONDUCT) == 0);
}
#endif
