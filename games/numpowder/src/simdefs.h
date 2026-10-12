/* NumPowder: The Powder Toy's simulation definitions, for the calculator.
 *
 * The original's grid is CELLS x CELL = 153 x 96 cells of 4 x 4 particles (612 x 384); its code is written
 * against those constants. Here the particle grid is 100 x 64, each drawn 3 x 3 pixels (300 x 192, the original's
 * layout at half its size). Walls (bmap, emap) have cells of 2 x 2 particles (WCELL) for finer building; air,
 * gravity and fans keep the original's cells of 4 x 4 particles (ACELL).
 *
 * A particle keeps every field the original's has, packed into 24 bytes (56 there): positions, velocities and
 * temperatures are fixed point, read and written through the accessors below (p_x, p_set_vx, p_temp...). */
#ifndef NP_SIMDEFS_H
#define NP_SIMDEFS_H
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/* ---------------------------------------------------------------- the grid */
#define XRES 100
#define YRES 64
#define NCELLS_P (XRES * YRES)        /* particle cells */
#define WCELL 2                       /* walls: bmap, emap (the original's CELL) */
#define XWCELLS (XRES / WCELL)
#define YWCELLS (YRES / WCELL)
#define ACELL 4                       /* air (pv, vx, vy, hv), gravity, fans (the original's CELL) */
#define XACELLS (XRES / ACELL)
#define YACELLS (YRES / ACELL)
#define CFDS (4.0f / ACELL)
#define ISTP (ACELL / 2)
#define BORDER ACELL                  /* the edge where particles die: the original's one CELL (4 particles) */
#define XCNTR (XRES / 2)
#define YCNTR (YRES / 2)
#define MAX_VELOCITY 1e4f
#define PX_SCALE 3                    /* screen pixels per particle */

/* ---------------------------------------------------------------- the original's constants */
#define MAX_TEMP 9999.0f
#define MIN_TEMP 0.0f
#define O_MAX_TEMP 3500.0f
#define O_MIN_TEMP (-273.0f)
#define MAX_PRESSURE 256.0f
#define MIN_PRESSURE (-256.0f)
#define R_TEMP 22.0f
#define IPL (MIN_PRESSURE - 1)
#define IPH (MAX_PRESSURE + 1)
#define ITL (MIN_TEMP - 1)
#define ITH (MAX_TEMP + 1)
#define NT (-1)                       /* no transition (PT_NONE kills) */
#define ST PT_NUM                     /* special transition */
#define M_GRAV 6.67300e-1f

#define TYPE_PART 0x00000001u
#define TYPE_LIQUID 0x00000002u
#define TYPE_SOLID 0x00000004u
#define TYPE_GAS 0x00000008u
#define TYPE_ENERGY 0x00000010u
#define STATE_FLAGS 0x0000001Fu
#define PROP_CONDUCTS 0x00000020u
#define PROP_PHOTPASS 0x00000040u
#define PROP_NEUTPENETRATE 0x00000080u
#define PROP_NEUTABSORB 0x00000100u
#define PROP_NEUTPASS 0x00000200u
#define PROP_DEADLY 0x00000400u
#define PROP_HOT_GLOW 0x00000800u
#define PROP_LIFE 0x00001000u
#define PROP_RADIOACTIVE 0x00002000u
#define PROP_LIFE_DEC 0x00004000u
#define PROP_LIFE_KILL 0x00008000u
#define PROP_LIFE_KILL_DEC 0x00010000u
#define PROP_SPARKSETTLE 0x00020000u
#define PROP_NOAMBHEAT 0x00040000u
#define PROP_NOCTYPEDRAW 0x00100000u

#define FLAG_STAGNANT 0x1
#define FLAG_SKIPMOVE 0x2
#define FLAG_MOVABLE 0x8
#define FLAG_PHOTDECO 0x8

/* the fields of a particle, as the original numbers them (CarriesTypeIn, the property tool) */
enum { FIELD_TYPE = 0, FIELD_LIFE = 1, FIELD_CTYPE = 2, FIELD_TMP = 9, FIELD_TMP2 = 10, FIELD_TMP3 = 11, FIELD_TMP4 = 12 };

/* the menu sections (MenuSection.h) */
enum { SC_WALL, SC_ELEC, SC_POWERED, SC_SENSOR, SC_FORCE, SC_EXPLOSIVE, SC_GAS, SC_LIQUID, SC_POWDERS, SC_SOLIDS,
       SC_NUCLEAR, SC_SPECIAL, SC_LIFE, SC_TOOL, SC_FAVORITES, SC_DECO, SC_TOTAL };

/* pmap values: an id and a type, as the original packs them (its PMAPBITS is 9; the types here fit 8 bits) */
#define PMAPBITS 8
#define PMAPMASK ((1 << PMAPBITS) - 1)
#define PT_NUM (1 << PMAPBITS)
static inline int ID(int r) { return r >> PMAPBITS; }
static inline int TYP(int r) { return r & PMAPMASK; }
static inline int PMAP(int id, int typ) { return (id << PMAPBITS) | (typ & PMAPMASK); }
static inline int PMAPID(int id) { return id << PMAPBITS; }
static inline bool InBounds(int x, int y) { return x >= 0 && y >= 0 && x < XRES && y < YRES; }

/* ---------------------------------------------------------------- a particle */
typedef struct {
  int32_t ctype;          /* 32 bits: photons' wavelengths */
  int16_t life, tmp, tmp2, tmp3;   /* (the original's are int; tmp4 is not kept: see p_tmp4) */
  int16_t x, y;           /* position, 1/64 particle */
  int16_t vx, vy;         /* velocity, 1/256 particle a frame */
  uint16_t temp;          /* Kelvin, 1/4 */
  uint8_t type, flags;
} Particle;
_Static_assert(sizeof(Particle) == 24, "a particle is 24 bytes");

#define P_POS 64.0f
#define P_VEL 256.0f
#define P_TEMP 4.0f
static inline float p_x(const Particle *p) { return p->x * (1 / P_POS); }
static inline float p_y(const Particle *p) { return p->y * (1 / P_POS); }
static inline float p_vx(const Particle *p) { return p->vx * (1 / P_VEL); }
static inline float p_vy(const Particle *p) { return p->vy * (1 / P_VEL); }
/* temperatures are kept as (T + 0.1 K) * 4, so that the whole degrees Celsius (n + 273.15 K) the original starts
 * elements at are exact; 0 is 0 K, and T_MAXQ is MAX_TEMP exactly (WARP's 10000 K is MAX_TEMP here: a code above
 * it would cost every temperature read a test) */
#define T_OFFSET 0.1f
#define T_MAXQ 39997
static inline float p_temp(const Particle *p) {
  return fminf(fmaxf(p->temp * (1 / P_TEMP) - T_OFFSET, MIN_TEMP), MAX_TEMP);
}
/* to the nearest (ties to even), within int16_t, NaN as 0: on the calculator two instructions do it all */
static inline int16_t np_q16(float v, float scale) {
  float s = v * scale;
#if defined(__ARM_FP) && defined(__thumb2__)
  int32_t r;
  __asm__("vcvtr.s32.f32 %0, %0" : "+t"(s));   /* (saturating; NaN gives 0) */
  __asm__("vmov %0, %1" : "=r"(r) : "t"(s));
  __asm__("ssat %0, #16, %0" : "+r"(r));
  return (int16_t)r;
#else
  if (!(s == s)) return 0;   /* NaN */
  s = s < -32768 ? -32768 : s > 32767 ? 32767 : s;
  return (int16_t)rintf(s);
#endif
}
/* velocities: to their 1/256 with dithering. Rounded to the nearest, a slow particle's v *= Loss would stop under
 * 10/256 (half a step off rounds back up) and it would drift for ever; dithered, the fraction is kept on average.
 * Stochastic rounding: the value in 1/256 of a step (towards 0), a random byte added, then the step it's in (so a
 * value already on a step stays on it). The dither has its own generator (temp_dither, the temperatures' too, here
 * a step of an LCG: its top byte), so the original's random numbers stay the same. NaN is 0, as np_q16 */
extern uint32_t temp_dither;
static inline int16_t np_q16_d(float v, float scale) {
  float s = v * (scale * 256.0f);
  if (s > 8388352.0f) s = 8388352.0f;   /* (32767 steps at most, the byte added or not; NaN stays NaN) */
  int32_t t;
#if defined(__ARM_FP) && defined(__thumb2__)
  __asm__("vcvt.s32.f32 %0, %0" : "+t"(s));   /* (towards 0, saturating; NaN gives 0) */
  __asm__("vmov %0, %1" : "=r"(t) : "t"(s));
  __asm__("ssat %0, #24, %0" : "+r"(t));
#else
  if (!(s == s)) s = 0;
  t = s < -8388608.0f ? -8388608 : (int32_t)s;
#endif
  temp_dither = temp_dither * 1664525u + 1013904223u;
  return (int16_t)((t + (int32_t)(temp_dither >> 24)) >> 8);
}
static inline void p_set_x(Particle *p, float v) { p->x = np_q16(v, P_POS); }
static inline void p_set_y(Particle *p, float v) { p->y = np_q16(v, P_POS); }
static inline void p_set_vx(Particle *p, float v) { p->vx = np_q16_d(v, P_VEL); }
static inline void p_set_vy(Particle *p, float v) { p->vy = np_q16_d(v, P_VEL); }
/* a temperature kept to the nearest (within MIN_TEMP and MAX_TEMP; NaN: MIN_TEMP, as restrict_flt) */
static inline uint16_t temp_q(float t) {
  t = !(t >= MIN_TEMP) ? MIN_TEMP : t > MAX_TEMP ? MAX_TEMP : t;
  return t >= MAX_TEMP ? T_MAXQ : (uint16_t)((t + T_OFFSET) * P_TEMP + 0.5f);
}
static inline void p_set_temp(Particle *p, float t) { p->temp = temp_q(t); }
/* the cell a particle is in (the original's (int)(x + 0.5f)) */
static inline int p_cx(const Particle *p) { return (p->x + 32) >> 6; }
static inline int p_cy(const Particle *p) { return (p->y + 32) >> 6; }

/* the functions' arguments, as the original's (the simulation, parts and pmap are globals here: sim.h) */
#define UPDATE_FUNC_ARGS int i, int x, int y, int surround_space, int nt
#define UPDATE_FUNC_SUBCALL_ARGS i, x, y, surround_space, nt
#define GRAPHICS_FUNC_ARGS const Particle *cpart, int nx, int ny, int *pixel_mode, int *cola, int *colr, int *colg, int *colb, int *firea, int *firer, int *fireg, int *fireb
#define GRAPHICS_FUNC_SUBCALL_ARGS cpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb
#define ELEMENT_CREATE_FUNC_ARGS int i, int x, int y, int t, int v
#define ELEMENT_CREATE_ALLOWED_FUNC_ARGS int i, int x, int y, int t
#define ELEMENT_CHANGETYPE_FUNC_ARGS int i, int x, int y, int from, int to
#define CTYPEDRAW_FUNC_ARGS int i, int t, int v
#define CTYPEDRAW_FUNC_SUBCALL_ARGS i, t, v
#endif
