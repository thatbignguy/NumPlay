/* NumPowder: The Powder Toy's Simulation (Simulation.cpp): the particles' store, creating and killing them,
 * the frame's steps around the particles' update (BeforeSim, AfterSim, RecalcFreeParticles, CheckStacking),
 * the flood fills, the random numbers (common/tpt-rand.cpp) and the settings. The particles' update and
 * movement are in sim_move.c. */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O3")   /* (a hot path: a few KB of flash for speed) */
#endif
#include "simpriv.h"

/* ---------------------------------------------------------------- the state */
Particle *parts;
int npart_max;
int parts_active, parts_lastActiveIndex = -1;
int pfree = -1;
int NUM_PARTS;
int elementCount[PT_NUM];
uint16_t (*pmap_id)[XRES];
uint16_t (*photons_id)[XRES];
uint8_t *sim_scratch;
uint32_t temp_dither = 0x2545F491;

unsigned char bmap[YWCELLS][XWCELLS], emap[YWCELLS][XWCELLS];
float fvx[YACELLS][XACELLS], fvy[YACELLS][XACELLS];

SimSettings settings;
int legacy_enable, aheat_enable, water_equal_test, edgeMode = EDGE_VOID, gravityMode;
float customGravityX, customGravityY;
bool ngrav_enable;
int currentTick;
uint32_t frameCount;
bool sys_pause;
int emp_decor;

int ISWIRE;
bool emap_any12;
bool force_stacking_check;
int emp_trigger_count;
bool etrd_count_valid;
int etrd_life0_count;
int lightningRecreate;
bool gravWallChanged;
bool elementRecount;
uint8_t wireless[CHANNELS][2];
int CGOL, GSPEED = 1;
int Element_PPIP_ppip_changed;
int16_t Element_PSTN_tempParts[XRES > YRES ? XRES : YRES];
uint8_t Element_LOLZ_lolz[XRES / 9][YRES / 9];
uint8_t Element_LOVE_love[XRES / 9][YRES / 9];
playerst player, player2, fighters[MAX_FIGHTERS];
unsigned char fighcount;
PortalSlot portalp[PORTAL_SLOTS];
int portal_count;

static inline void set_active(int n) {
  parts_active = n;
  parts_lastActiveIndex = n - 1;
}

/* ---------------------------------------------------------------- random numbers: xoroshiro128+ (tpt-rand.cpp) */
uint64_t rng_state[2] = {0, 614};
static inline uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
static inline uint64_t rng_next(void) {
  uint64_t s0 = rng_state[0], s1 = rng_state[1], result = s0 + s1;
  s1 ^= s0;
  rng_state[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
  rng_state[1] = rotl(s1, 36);
  return result;
}
unsigned int rng_gen(void) { return (unsigned int)(rng_next() & 0x7FFFFFFF); }
unsigned int rng_u32(void) { return (unsigned int)rng_next(); }
int rng_between(int lower, int upper) {
  unsigned int r = (unsigned int)rng_next();
  return (int)(r % ((unsigned int)upper - (unsigned int)lower + 1u)) + lower;
}
/* the original takes the 64-bit number modulo the denominator; below 2^16 that's done with 32-bit divisions */
bool rng_chance(int numerator, unsigned int denominator) {
  if (numerator < 0) return false;
  uint64_t n = rng_next();
  uint32_t m;
  if (denominator <= 0x10000u) {
    uint32_t hi = (uint32_t)(n >> 32), lo = (uint32_t)n, k = (0u - denominator) % denominator;   /* 2^32 mod d */
    m = ((hi % denominator) * k + lo % denominator) % denominator;
  } else {
    m = (uint32_t)(n % denominator);
  }
  return m < (unsigned int)numerator;
}
float rng_uniform01(void) { return (float)(uint32_t)rng_next() / (float)0xFFFFFFFFu; }
void rng_seed(unsigned int sd) { rng_state[0] = sd, rng_state[1] = sd; }

/* ---------------------------------------------------------------- the particles' store (Parts) */
static void parts_reset(void) {
  memset(parts, 0, sizeof(Particle) * (size_t)npart_max);
  set_active(0);
  pfree = -1;
}
int parts_alloc(void) {
  if (pfree != -1) {
    int i = pfree;
    pfree = parts[i].life;
    return i;
  }
  if (parts_active < NPART) {
    int i = parts_active;
    set_active(parts_active + 1);
    return i;
  }
  return -1;
}
static void parts_free(int i) {
  parts[i].type = PT_NONE;
  parts[i].life = (int16_t)pfree;
  pfree = i;
}
static bool max_parts_reached(void) { return pfree == -1 && parts_active >= NPART; }
/* Parts::Flatten: the free list in order, the active ids up to the last particle */
void sim_flatten_parts(void) {
  int newActive = 0, prev = -1;
  pfree = -1;
  for (int i = 0; i < parts_active; i++) {
    if (parts[i].type) {
      for (int j = newActive; j < i; ++j) {
        if (prev < 0) pfree = j;
        else parts[prev].life = (int16_t)j;
        prev = j;
      }
      newActive = i + 1;
    }
  }
  if (prev < 0) pfree = -1;
  else parts[prev].life = -1;
  set_active(newActive);
}

/* ---------------------------------------------------------------- tmp4, which the particle doesn't keep
 * VIRS, VRSS, VRSG: tmp3 holds the cure countdown when it's above 0 (as the original's tmp3), or minus tmp4 (the
 *   countdown to dying) otherwise: the original only uses tmp4 while tmp3 is 0, and clears it when cured.
 * PLNT, SEED: the 15 bits of genes in ctype's bits 20 to 31 (the original's ctype uses 20) and flags' bits 5 to 7.
 * STOR: the stored particle's ctype in ctype's bits 8 to 31 (the original's ctype there is an element) and
 *   tmp's bits 8 to 15 (its tmp is the stored element).
 * PIPE, PPIP: the stored ctype in ctype's bits 8 to 31 (bits 0 to 7 are the stored element), sign-extended;
 *   one that doesn't fit 24 bits (photons' wavelengths) keeps its top bits in a small table by particle id. */
#define PIPE_OVF 64
static uint16_t pipe_ovf_id[PIPE_OVF];   /* id + 1, 0 free */
static int8_t pipe_ovf_hi[PIPE_OVF];
static int pipe_ovf_find(int i) {
  for (int k = 0; k < PIPE_OVF; k++)
    if (pipe_ovf_id[k] == i + 1) return k;
  return -1;
}
void pipe_ovf_clear(int i) {
  int k = pipe_ovf_find(i);
  if (k >= 0) pipe_ovf_id[k] = 0;
}
void pipe_ovf_move(int from, int to) {
  pipe_ovf_clear(to);
  int k = pipe_ovf_find(from);
  if (k >= 0) pipe_ovf_id[k] = (uint16_t)(to + 1);
}
int tmp4_get(int i) {
  const Particle *p = &parts[i];
  switch (p->type) {
  case PT_VIRS: case PT_VRSS: case PT_VRSG:
    return p->tmp3 < 0 ? -p->tmp3 : 0;
  case PT_PLNT: case PT_SEED:
    return (int)(((uint32_t)p->ctype >> 20) | (uint32_t)((p->flags >> 5) & 7) << 12);
  case PT_STOR:
    return (int)(((uint32_t)p->ctype >> 8) | ((uint32_t)(uint16_t)p->tmp >> 8) << 24);
  case PT_PIPE: case PT_PPIP: {
    int v = p->ctype >> 8;   /* (arithmetic) */
    int k = pipe_ovf_find(i);
    if (k >= 0) v = (int)(((uint32_t)v & 0xFFFFFF) | (uint32_t)(uint8_t)pipe_ovf_hi[k] << 24);
    return v;
  }
  default:
    return 0;
  }
}
void tmp4_set(int i, int v) {
  Particle *p = &parts[i];
  switch (p->type) {
  case PT_VIRS: case PT_VRSS: case PT_VRSG:
    if (p->tmp3 <= 0) p->tmp3 = (int16_t)-iclamp(v, 0, 32767);
    break;
  case PT_PLNT: case PT_SEED:
    p->ctype = (int32_t)(((uint32_t)p->ctype & 0xFFFFF) | ((uint32_t)v & 0xFFF) << 20);
    p->flags = (uint8_t)((p->flags & 0x1F) | (((uint32_t)v >> 12) & 7) << 5);
    break;
  case PT_STOR:
    p->ctype = (int32_t)(((uint32_t)p->ctype & 0xFF) | ((uint32_t)v & 0xFFFFFF) << 8);
    p->tmp = (int16_t)(((uint16_t)p->tmp & 0xFF) | (((uint32_t)v >> 24) & 0xFF) << 8);
    break;
  case PT_PIPE: case PT_PPIP: {
    int k = pipe_ovf_find(i);
    if (v >= -0x800000 && v < 0x800000) {
      if (k >= 0) pipe_ovf_id[k] = 0;
    } else {
      if (k < 0)
        for (k = 0; k < PIPE_OVF && pipe_ovf_id[k]; k++) {}
      if (k < PIPE_OVF) pipe_ovf_id[k] = (uint16_t)(i + 1), pipe_ovf_hi[k] = (int8_t)(v >> 24);
      /* (the table full: what fits, the low 23 bits, positive, rather than a value read back sign-extended from
       * bit 23. A photon keeps its blue and green wavelengths) */
      else v &= 0x7FFFFF;
    }
    p->ctype = (int32_t)(((uint32_t)p->ctype & 0xFF) | ((uint32_t)v & 0xFFFFFF) << 8);
    break;
  }
  default:
    break;
  }
}
void tmp4_clear(int i) { tmp4_set(i, 0); }

/* ---------------------------------------------------------------- the portals' particles (portalp) */
const int portal_rx[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
const int portal_ry[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
static uint64_t portal_used(int channel, int dir, uint32_t *hi) {
  uint64_t lo = 0;
  *hi = 0;
  for (int k = 0; k < portal_count; k++)
    if (portalp[k].channel == channel && portalp[k].dir == dir) {
      int s = portalp[k].slot;
      if (s < 64) lo |= (uint64_t)1 << s;
      else *hi |= 1u << (s - 64);
    }
  return lo;
}
static int portal_first_free(int channel, int dir) {
  uint32_t hi;
  uint64_t lo = portal_used(channel, dir, &hi);
  if (~lo) return __builtin_ctzll(~lo);
  if ((~hi) & 0xFFFF) return 64 + __builtin_ctz(~hi);
  return -1;   /* the 80 places taken */
}
bool portal_free(int channel, int dir) { return portal_count < PORTAL_SLOTS && portal_first_free(channel, dir) >= 0; }
bool portal_put(int channel, int dir, const Particle *p) {
  if (portal_count >= PORTAL_SLOTS) return false;
  int s = portal_first_free(channel, dir);
  if (s < 0) return false;
  portalp[portal_count].channel = (uint8_t)channel;
  portalp[portal_count].dir = (uint8_t)dir;
  portalp[portal_count].slot = (uint8_t)s;
  portalp[portal_count].p = *p;
  portal_count++;
  return true;
}
int portal_find(int channel, int dir, int slot) {
  for (int k = 0; k < portal_count; k++)
    if (portalp[k].slot == slot && portalp[k].channel == channel && portalp[k].dir == dir) return k;
  return -1;
}
void portal_take(int entry) { portalp[entry] = portalp[--portal_count]; }
bool portal_any(int channel) {
  for (int k = 0; k < portal_count; k++)
    if (portalp[k].channel == channel) return true;
  return false;
}

/* ---------------------------------------------------------------- SimulationData's */
float HeatCapacityOf(const Particle *p) {
  if ((p->type == PT_PIPE || p->type == PT_PPIP) && IsElement(TYP(p->ctype)))
    return elements[p->type].HeatCapacity + elements[TYP(p->ctype)].HeatCapacity;
  return elements[p->type].HeatCapacity;
}

/* ---------------------------------------------------------------- killing, changing and creating particles */
void kill_part(int i) {
  if (i < 0 || i >= NPART) return;
  Particle *p = &parts[i];
  int x = (p->x + 32) / 64, y = (p->y + 32) / 64;   /* (int)(x + 0.5f) */
  int t = p->type;
  if (t && elements[t].ChangeType) elements[t].ChangeType(i, x, y, t, PT_NONE);
  if (x >= 0 && y >= 0 && x < XRES && y < YRES) {
    if (pmap_id[y][x] == i + 1) pmap_id[y][x] = 0;
    else if (photons_id[y][x] == i + 1) photons_id[y][x] = 0;
  }
  if (t == PT_NONE) return;
  if (t == PT_PIPE || t == PT_PPIP) pipe_ovf_clear(i);
  elementCount[t]--;
  parts_free(i);
  NUM_PARTS -= 1;
}

bool part_change_type(int i, int x, int y, int t) {
  if (x < 0 || y < 0 || x >= XRES || y >= YRES || i >= NPART || t < 0 || t >= PT_NUM || !parts[i].type) return false;
  if (!elements[t].Enabled || t == PT_NONE) {
    kill_part(i);
    return true;
  }
  if (elements[t].CreateAllowed && !elements[t].CreateAllowed(i, x, y, t)) return false;
  int from = parts[i].type;
  if (elements[from].ChangeType) elements[from].ChangeType(i, x, y, from, t);
  if (elements[t].ChangeType) elements[t].ChangeType(i, x, y, parts[i].type, t);
  from = parts[i].type;
  if (from > 0 && from < PT_NUM && elementCount[from]) elementCount[from]--;
  elementCount[t]++;
  if ((from == PT_PIPE || from == PT_PPIP) && t != PT_PIPE && t != PT_PPIP) pipe_ovf_clear(i);
  parts[i].type = (uint8_t)t;
  if (elements[t].Properties & TYPE_ENERGY) {
    photons_id[y][x] = (uint16_t)(i + 1);
    if (pmap_id[y][x] == i + 1) pmap_id[y][x] = 0;
  } else {
    pmap_id[y][x] = (uint16_t)(i + 1);
    if (photons_id[y][x] == i + 1) photons_id[y][x] = 0;
  }
  return false;
}

/* p: -1 a new particle, -2 from the brush, -3 without the map's checks, or the particle to replace */
int create_part(int p, int x, int y, int t, int v) {
  if (x < 0 || y < 0 || x >= XRES || y >= YRES || t <= 0 || t >= PT_NUM || !elements[t].Enabled) return -1;

  if (t == PT_SPRK && p != -3 && !(p == -2 && elements[TYP(PMAPAT(x, y))].CtypeDraw)) {
    int r = PMAPAT(x, y), type = TYP(r), index = ID(r);
    if (type == PT_WIRE) {
      parts[index].ctype = PT_DUST;
      return index;
    }
    if (!(type == PT_INST || (elements[type].Properties & PROP_CONDUCTS)) || parts[index].life != 0) return -1;
    if (p == -2 && type == PT_INST) {
      FloodINST(x, y);
      return index;
    }
    parts[index].type = PT_SPRK;
    parts[index].life = 4;
    parts[index].ctype = type;
    if (p_temp(&parts[index]) + 10.0f < 673.0f && !legacy_enable &&
        (type == PT_METL || type == PT_BMTL || type == PT_BRMT || type == PT_PSCN || type == PT_NSCN ||
         type == PT_ETRD || type == PT_NBLE || type == PT_IRON))
      p_set_temp(&parts[index], p_temp(&parts[index]) + 10.0f);
    return index;
  }

  if (p == -2) {
    int r = PMAPAT(x, y);
    if (r) {
      int drawOn = TYP(r);
      if (elements[drawOn].CtypeDraw) elements[drawOn].CtypeDraw(ID(r), t, v);
      return -1;
    } else if (IsWallBlocking(x, y, t)) {
      return -1;
    } else if (photons_id[y][x] && (elements[t].Properties & TYPE_ENERGY)) {
      return -1;
    }
  }

  if (elements[t].CreateAllowed && !elements[t].CreateAllowed(p, x, y, t)) return -1;

  int i, oldType = PT_NONE;
  if (p == -1 || p == -2 || p == -3) {
    if (p == -1) {
      /* a particle there: only one that can be in the same place; none but a wall: only one the wall allows */
      if (pmap_id[y][x] ? (eval_move(t, x, y, NULL) != 2) : (bmap[y / WCELL][x / WCELL] && eval_move(t, x, y, NULL) == 0))
        return -1;
    }
    i = parts_alloc();
    if (i == -1) return -1;
    NUM_PARTS += 1;
  } else {
    int oldX = (parts[p].x + 32) / 64, oldY = (parts[p].y + 32) / 64;
    if (InBounds(oldX, oldY)) {
      if (pmap_id[oldY][oldX] == p + 1) pmap_id[oldY][oldX] = 0;
      if (photons_id[oldY][oldX] == p + 1) photons_id[oldY][oldX] = 0;
    }
    oldType = parts[p].type;
    if (elements[oldType].ChangeType) elements[oldType].ChangeType(p, oldX, oldY, oldType, t);
    if (oldType) elementCount[oldType]--;
    if (oldType == PT_PIPE || oldType == PT_PPIP) pipe_ovf_clear(p);
    i = p;
  }

  /* parts[i] = elements[t].DefaultProperties */
  const Element *e = &elements[t];
  Particle *q = &parts[i];
  q->type = (uint8_t)t;
  q->flags = 0;
  q->life = (int16_t)e->DefaultProperties.life;
  q->ctype = e->DefaultProperties.ctype;
  q->tmp = (int16_t)e->DefaultProperties.tmp;
  q->tmp2 = (int16_t)e->DefaultProperties.tmp2;
  q->tmp3 = (int16_t)e->DefaultProperties.tmp3;
  p_set_temp(q, e->DefaultProperties.temp);
  q->vx = q->vy = 0;
  if (t == PT_MORT) q->vx = (int16_t)(2 * P_VEL);   /* (its DefaultProperties.vx, the only element with one: CHOO CHOO) */
  q->x = (int16_t)(x * 64);
  q->y = (int16_t)(y * 64);
  if (e->DefaultProperties.tmp4) tmp4_set(i, e->DefaultProperties.tmp4);

  if (e->Properties & TYPE_ENERGY) photons_id[y][x] = (uint16_t)(i + 1);
  else if (t != PT_STKM && t != PT_STKM2 && t != PT_FIGH) pmap_id[y][x] = (uint16_t)(i + 1);

  /* (pretty powders' colours are decoration, which isn't kept) */
  if (e->Create) e->Create(i, x, y, t, v);
  if (e->ChangeType) e->ChangeType(i, x, y, oldType, t);
  elementCount[t]++;
  return i;
}

/* changes a particle's type, keeping its temperature and velocity */
int createPartTempVel(int i, int x, int y, int t) {
  uint16_t temp = parts[i].temp;
  int16_t pvx = parts[i].vx, pvy = parts[i].vy;
  int np = create_part(i, x, y, t, -1);
  if (np >= 0) {
    parts[np].temp = temp;
    parts[np].vx = pvx;
    parts[np].vy = pvy;
  }
  return np;
}

void delete_part(int x, int y) {
  if (x < 0 || y < 0 || x >= XRES || y >= YRES) return;
  int i = photons_id[y][x] ? photons_id[y][x] : pmap_id[y][x];
  if (!i) return;
  kill_part(i - 1);
}

int parts_avg(int ci, int ni, int t) {
  if (t == PT_INSL) {   /* to keep electronics working */
    int pmr = PMAPAT(((int)(p_x(&parts[ci]) + 0.5f) + (int)(p_x(&parts[ni]) + 0.5f)) / 2,
                     ((int)(p_y(&parts[ci]) + 0.5f) + (int)(p_y(&parts[ni]) + 0.5f)) / 2);
    return pmr ? parts[ID(pmr)].type : PT_NONE;
  }
  int pmr2 = PMAPAT((int)((p_x(&parts[ci]) + p_x(&parts[ni])) / 2 + 0.5f),
                    (int)((p_y(&parts[ci]) + p_y(&parts[ni])) / 2 + 0.5f));
  if (pmr2) {
    if (parts[ID(pmr2)].type == t) return t;
  } else {
    return PT_NONE;
  }
  return PT_NONE;
}

/* photons from PHOT going through GLOW */
void create_gain_photon(int pp) {
  if (max_parts_reached()) return;
  int lr = 2 * rng_between(0, 1) - 1;
  float xx = q64(p_x(&parts[pp]) - lr * 0.3f * p_vy(&parts[pp]));
  float yy = q64(p_y(&parts[pp]) + lr * 0.3f * p_vx(&parts[pp]));
  int nx = (int)(xx + 0.5f), ny = (int)(yy + 0.5f);
  if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES) return;
  int g = PMAPAT(nx, ny);
  if (TYP(g) != PT_GLOW) return;
  uint16_t oldTemp = parts[ID(g)].temp;
  int i = create_part(-3, nx, ny, PT_PHOT, -1);
  if (i == -1) return;
  p_set_x(&parts[i], xx);
  p_set_y(&parts[i], yy);
  parts[i].vx = parts[pp].vx;
  parts[i].vy = parts[pp].vy;
  parts[i].temp = oldTemp;
  int temp_bin = (int)((p_temp(&parts[i]) - 273.0f) * 0.25f);
  if (temp_bin < 0) temp_bin = 0;
  if (temp_bin > 25) temp_bin = 25;
  parts[i].ctype = 0x1F << temp_bin;
}

/* photons from NEUT going through GLAS */
void create_cherenkov_photon(int pp) {
  if (max_parts_reached()) return;
  int nx = (int)(p_x(&parts[pp]) + 0.5f), ny = (int)(p_y(&parts[pp]) + 0.5f);
  int g = PMAPAT(nx, ny);
  if (TYP(g) != PT_GLAS && TYP(g) != PT_BGLA) return;
  if (hypotf(p_vx(&parts[pp]), p_vy(&parts[pp])) < 1.44f) return;
  uint16_t oldTemp = parts[ID(g)].temp;
  int i = create_part(-3, nx, ny, PT_PHOT, -1);
  if (i == -1) return;
  int lr = 2 * rng_between(0, 1) - 1;
  parts[i].ctype = 0x00000F80;
  parts[i].x = parts[pp].x;
  parts[i].y = parts[pp].y;
  parts[i].temp = oldTemp;
  float nvx = p_vx(&parts[pp]) - lr * 2.5f * p_vy(&parts[pp]);
  float nvy = p_vy(&parts[pp]) + lr * 2.5f * p_vx(&parts[pp]);
  float r = 1.269f / hypotf(nvx, nvy);   /* photons have speed of light. no discussion. */
  p_set_vx(&parts[i], nvx * r);
  p_set_vy(&parts[i], nvy * r);
}

/* ---------------------------------------------------------------- gravity's field at a particle */
void GetGravityField(int x, int y, float particleGrav, float newtonGrav, float *pGravX, float *pGravY) {
  switch (gravityMode) {
  default:
  case GRAV_VERTICAL:
    *pGravX = 0;
    *pGravY = particleGrav;
    break;
  case GRAV_OFF:
    *pGravX = 0;
    *pGravY = 0;
    break;
  case GRAV_RADIAL: {
    *pGravX = 0;
    *pGravY = 0;
    float dx = (float)(x - XCNTR), dy = (float)(y - YCNTR);
    if (dx || dy) {
      float pGravD = 0.01f - hypotf(dx, dy);
      *pGravX = particleGrav * (dx / pGravD);
      *pGravY = particleGrav * (dy / pGravD);
    }
    break;
  }
  case GRAV_CUSTOM:
    *pGravX = particleGrav * customGravityX;
    *pGravY = particleGrav * customGravityY;
    break;
  }
  if (newtonGrav) {
    *pGravX += newtonGrav * gravx[y / ACELL][x / ACELL];
    *pGravY += newtonGrav * gravy[y / ACELL][x / ACELL];
  }
}

/* ---------------------------------------------------------------- walls */
bool IsWallBlocking(int x, int y, int type) {
  int wall = bmap[y / WCELL][x / WCELL];
  if (wall) {
    uint32_t props = elements[type].Properties;
    if (wall == WL_ALLOWGAS && !(props & TYPE_GAS)) return true;
    else if (wall == WL_ALLOWENERGY && !(props & TYPE_ENERGY)) return true;
    else if (wall == WL_ALLOWLIQUID && !(props & TYPE_LIQUID)) return true;
    else if (wall == WL_ALLOWPOWDER && !(props & TYPE_PART)) return true;
    else if (wall == WL_ALLOWAIR || wall == WL_WALL || wall == WL_WALLELEC) return true;
    else if (wall == WL_EWALL && !emap[y / WCELL][x / WCELL]) return true;
    else if (wall == WL_DETECT && (props & TYPE_SOLID)) return true;
  }
  return false;
}

static inline bool is_wire(int x, int y) {
  if ((unsigned)x >= XWCELLS || (unsigned)y >= YWCELLS) return false;   /* (off the grid: no wall) */
  int b = bmap[y][x];
  return b == WL_DETECT || b == WL_EWALL || b == WL_ALLOWLIQUID || b == WL_WALLELEC || b == WL_ALLOWALLELEC ||
         b == WL_EHOLE || b == WL_STASIS;
}
static inline bool is_wire_off(int x, int y) { return is_wire(x, y) && emap[y][x] < 8; }

/* powers the connected wall cells (in wall cells) */
void set_emap(int x, int y) {
  if (!is_wire_off(x, y)) return;
  int x1 = x, x2 = x;
  while (x1 > 0 && is_wire_off(x1 - 1, y)) x1--;
  while (x2 < XWCELLS - 1 && is_wire_off(x2 + 1, y)) x2++;
  for (x = x1; x <= x2; x++) emap[y][x] = 16;

  if (y > 1 && x1 == x2 && is_wire(x1 - 1, y - 1) && is_wire(x1, y - 1) && is_wire(x1 + 1, y - 1) &&
      !is_wire(x1 - 1, y - 2) && is_wire(x1, y - 2) && !is_wire(x1 + 1, y - 2))
    set_emap(x1, y - 2);
  else if (y > 0)
    for (x = x1; x <= x2; x++)
      if (is_wire_off(x, y - 1)) {
        if (x == x1 || x == x2 || y >= YWCELLS - 1 || is_wire(x - 1, y - 1) || is_wire(x + 1, y - 1) ||
            is_wire(x - 1, y + 1) || !is_wire(x, y + 1) || is_wire(x + 1, y + 1))
          set_emap(x, y - 1);
      }

  if (y < YWCELLS - 2 && x1 == x2 && is_wire(x1 - 1, y + 1) && is_wire(x1, y + 1) && is_wire(x1 + 1, y + 1) &&
      !is_wire(x1 - 1, y + 2) && is_wire(x1, y + 2) && !is_wire(x1 + 1, y + 2))
    set_emap(x1, y + 2);
  else if (y < YWCELLS - 1)
    for (x = x1; x <= x2; x++)
      if (is_wire_off(x, y + 1)) {
        if (x == x1 || x == x2 || y < 0 || is_wire(x - 1, y + 1) || is_wire(x + 1, y + 1) ||
            is_wire(x - 1, y - 1) || !is_wire(x, y - 1) || is_wire(x + 1, y - 1))
          set_emap(x, y + 1);
      }
}

void SetEdgeMode(int newEdgeMode) {
  edgeMode = newEdgeMode;
  int b = BORDER / WCELL;   /* the edge's wall cells */
  switch (edgeMode) {
  case EDGE_VOID:
  case EDGE_LOOP:
  case EDGE_SOLID:
    for (int y = 0; y < YWCELLS; y++)
      for (int x = 0; x < XWCELLS; x++)
        if (y < b || x < b || y >= YWCELLS - b || x >= XWCELLS - b) bmap[y][x] = edgeMode == EDGE_SOLID ? WL_WALL : 0;
    break;
  default:
    SetEdgeMode(EDGE_VOID);
  }
}

/* the wavelengths a photon keeps going through glass, and the bin of its refraction index */
int get_wavelength_bin(int *wm) {
  if (!(*wm & 0x3FFFFFFF)) return -1;
  int w0 = __builtin_ctz((unsigned)*wm | 0xC0000000u);
  int wM = 31 - __builtin_clz((unsigned)*wm & 0x3FFFFFFFu);
  if (wM - w0 < 5) return wM + w0;
  int r = (int)rng_gen();
  int i = (r >> 1) % (wM - w0 - 4);
  i += w0;
  if (r & 1) {
    *wm &= 0x1F << i;
    return (i + 2) * 2;
  } else {
    *wm &= 0xF << i;
    return (i + 2) * 2 - 1;
  }
}

/* a line of particles, 0 particles thick (SPRK's) */
void CreateLine(int x1, int y1, int x2, int y2, int c) {
  bool reverseXY = abs(y2 - y1) > abs(x2 - x1);
  int x, y, dx, dy, sy, v = ID(c);
  float e, de;
  c = TYP(c);
  if (reverseXY) {
    y = x1, x1 = y1, y1 = y;
    y = x2, x2 = y2, y2 = y;
  }
  if (x1 > x2) {
    y = x1, x1 = x2, x2 = y;
    y = y1, y1 = y2, y2 = y;
  }
  dx = x2 - x1;
  dy = abs(y2 - y1);
  e = 0.0f;
  de = dx ? dy / (float)dx : 0.0f;
  y = y1;
  sy = (y1 < y2) ? 1 : -1;
  for (x = x1; x <= x2; x++) {
    if (reverseXY) create_part(-1, y, x, c, v);
    else create_part(-1, x, y, c, v);
    e += de;
    if (e >= 0.5f) {
      y += sy;
      if ((y1 < y2) ? (y <= y2) : (y >= y2)) {
        if (reverseXY) create_part(-1, y, x, c, v);
        else create_part(-1, x, y, c, v);
      }
      e -= 1.0f;
    }
  }
}

/* ---------------------------------------------------------------- flood fills' stack (CoordStack): the cells
 * pushed, as runs along a row (pushing the next x of the top run extends it), so the order is the original's
 * and a row pushed whole is one entry; in sim_scratch after `from` bytes */
static uint32_t *cs_runs;
static int cs_n, cs_cap;
void flood_stack_init(int from) {
  cs_runs = (uint32_t *)(void *)(sim_scratch + from);
  cs_cap = (SCRATCH - from) / 4;
  cs_n = 0;
}
bool flood_stack_push(int x, int y) {
  if (cs_n) {
    uint32_t t = cs_runs[cs_n - 1];
    if ((int)(t >> 16) == y && (int)(t & 0xFF) + 1 == x) {
      cs_runs[cs_n - 1] = t + 1;
      return true;
    }
  }
  if (cs_n >= cs_cap) return false;   /* (the original's CoordStackOverflowException) */
  cs_runs[cs_n++] = (uint32_t)y << 16 | (uint32_t)x << 8 | (uint32_t)x;
  return true;
}
void flood_stack_pop(int *x, int *y) {
  uint32_t t = cs_runs[cs_n - 1];
  *y = (int)(t >> 16);
  *x = (int)(t & 0xFF);
  if ((int)(t & 0xFF) == (int)((t >> 8) & 0xFF)) cs_n--;
  else cs_runs[cs_n - 1] = t - 1;
}
int flood_stack_size(void) { return cs_n; }
/* a bitmap of the cells looked at, in sim_scratch's first FLOOD_BITMAP bytes */
#define FLOOD_BITMAP ((XRES * YRES + 7) / 8)
static inline bool bm_get(int x, int y) {
  int k = y * XRES + x;
  return sim_scratch[k >> 3] >> (k & 7) & 1;
}
static inline void bm_set(int x, int y) {
  int k = y * XRES + x;
  sim_scratch[k >> 3] |= (uint8_t)(1 << (k & 7));
}

bool FloodFillPmapCheck(int x, int y, int type) {
  if (type == 0) return !pmap_id[y][x] && !photons_id[y][x];
  if (elements[type].Properties & TYPE_ENERGY) return TYP(PHOTAT(x, y)) == type;
  return TYP(PMAPAT(x, y)) == type;
}

/* the original's flood_prop, setting an int property (FIELD_LIFE, FIELD_CTYPE, FIELD_TMP...) */
static void set_field(int i, int field, int value) {
  switch (field) {
  case FIELD_LIFE: parts[i].life = (int16_t)value; break;
  case FIELD_CTYPE: parts[i].ctype = value; break;
  case FIELD_TMP: parts[i].tmp = (int16_t)value; break;
  case FIELD_TMP2: parts[i].tmp2 = (int16_t)value; break;
  case FIELD_TMP3: parts[i].tmp3 = (int16_t)value; break;
  case FIELD_TMP4: tmp4_set(i, value); break;
  default: break;
  }
}
int flood_prop_int(int x, int y, int field, int value) {
  int i, x1, x2, dy = 1, did_something = 0;
  int r = PMAPAT(x, y);
  if (!r) r = PHOTAT(x, y);
  if (!r) return 0;
  int parttype = TYP(r);
  memset(sim_scratch, 0, FLOOD_BITMAP);
  flood_stack_init(FLOOD_BITMAP);
  flood_stack_push(x, y);
  do {
    flood_stack_pop(&x, &y);
    x1 = x2 = x;
    while (x1 >= BORDER) {
      if (!FloodFillPmapCheck(x1 - 1, y, parttype) || bm_get(x1 - 1, y)) break;
      x1--;
    }
    while (x2 < XRES - BORDER) {
      if (!FloodFillPmapCheck(x2 + 1, y, parttype) || bm_get(x2 + 1, y)) break;
      x2++;
    }
    for (x = x1; x <= x2; x++) {
      i = pmap_id[y][x];
      if (!i) i = photons_id[y][x];
      if (!i) continue;
      set_field(i - 1, field, value);
      bm_set(x, y);
      did_something = 1;
    }
    if (y >= BORDER + dy)
      for (x = x1; x <= x2; x++)
        if (FloodFillPmapCheck(x, y - dy, parttype) && !bm_get(x, y - dy))
          if (!flood_stack_push(x, y - dy)) return -1;
    if (y < YRES - BORDER - dy)
      for (x = x1; x <= x2; x++)
        if (FloodFillPmapCheck(x, y + dy, parttype) && !bm_get(x, y + dy))
          if (!flood_stack_push(x, y + dy)) return -1;
  } while (cs_n > 0);
  return did_something;
}

static inline bool isSparkableInst(int x, int y) {
  int r = PMAPAT(x, y);
  return TYP(r) == PT_INST && parts[ID(r)].life == 0;
}
static inline bool isInst(int x, int y) {
  int r = PMAPAT(x, y);
  return TYP(r) == PT_INST || (TYP(r) == PT_SPRK && parts[ID(r)].ctype == PT_INST);
}
/* sparks a whole piece of INST at once */
int FloodINST(int x, int y) {
  int x1, x2, created_something = 0;
  if (!isSparkableInst(x, y)) return 1;
  flood_stack_init(0);
  flood_stack_push(x, y);
  do {
    flood_stack_pop(&x, &y);
    x1 = x2 = x;
    while (x1 >= BORDER && isSparkableInst(x1 - 1, y)) x1--;
    while (x2 < XRES - BORDER && isSparkableInst(x2 + 1, y)) x2++;
    for (x = x1; x <= x2; x++)
      if (create_part(-1, x, y, PT_SPRK, -1) >= 0) created_something = 1;

    /* add vertically adjacent pixels to stack (wire crossing for INST) */
    if (y >= BORDER + 1 && x1 == x2 && isInst(x1 - 1, y - 1) && isInst(x1, y - 1) && isInst(x1 + 1, y - 1) &&
        !isInst(x1 - 1, y - 2) && isInst(x1, y - 2) && !isInst(x1 + 1, y - 2)) {
      /* travelling vertically up, skipping a horizontal line */
      if (isSparkableInst(x1, y - 2) && !flood_stack_push(x1, y - 2)) return -1;
    } else if (y >= BORDER + 1) {
      for (x = x1; x <= x2; x++)
        if (isSparkableInst(x, y - 1)) {
          /* at the end of a horizontal section, or a T junction or not a 1px wire crossing */
          if (x == x1 || x == x2 || y >= YRES - BORDER - 1 || !isInst(x, y + 1) || isInst(x + 1, y + 1) || isInst(x - 1, y + 1))
            if (!flood_stack_push(x, y - 1)) return -1;
        }
    }
    if (y < YRES - BORDER - 1 && x1 == x2 && isInst(x1 - 1, y + 1) && isInst(x1, y + 1) && isInst(x1 + 1, y + 1) &&
        !isInst(x1 - 1, y + 2) && isInst(x1, y + 2) && !isInst(x1 + 1, y + 2)) {
      /* travelling vertically down, skipping a horizontal line */
      if (isSparkableInst(x1, y + 2) && !flood_stack_push(x1, y + 2)) return -1;
    } else if (y < YRES - BORDER - 1) {
      for (x = x1; x <= x2; x++)
        if (isSparkableInst(x, y + 1)) {
          if (x == x1 || x == x2 || y < 0 || !isInst(x, y - 1) || isInst(x + 1, y - 1) || isInst(x - 1, y - 1))
            if (!flood_stack_push(x, y + 1)) return -1;
        }
    }
  } while (cs_n > 0);
  return created_something;
}

/* water equalization: moves water to a free place at the top of the body of water it's in */
bool flood_water(int x, int y, int i) {
  int x1, x2, originalX = x, originalY = y;
  if (!pmap_id[y][x]) return false;
  memset(sim_scratch, 0, FLOOD_BITMAP);
  flood_stack_init(FLOOD_BITMAP);
  flood_stack_push(x, y);
  do {
    flood_stack_pop(&x, &y);
    x1 = x2 = x;
    while (x1 >= BORDER) {
      if (elements[TYP(PMAPAT(x1 - 1, y))].Falldown != 2 || bm_get(x1 - 1, y)) break;
      x1--;
    }
    while (x2 < XRES - BORDER) {
      if (elements[TYP(PMAPAT(x2 + 1, y))].Falldown != 2 || bm_get(x2 + 1, y)) break;
      x2++;
    }
    for (x = x1; x <= x2; x++) {
      if ((y - 1) > originalY && !pmap_id[y - 1][x]) {
        /* try a random place on this line, there's probably a free one somewhere */
        int randPos = rng_between(x, x2);
        if (!pmap_id[y - 1][randPos] && eval_move(parts[i].type, randPos, y - 1, NULL)) x = randPos;
        /* or the original place on the left */
        else if (!eval_move(parts[i].type, x, y - 1, NULL)) continue;
        sim_move(i, originalX, originalY, (float)x, (float)(y - 1));
        return true;
      }
      bm_set(x, y);
    }
    if (y >= BORDER + 1)
      for (x = x1; x <= x2; x++)
        if (elements[TYP(PMAPAT(x, y - 1))].Falldown == 2 && !bm_get(x, y - 1))
          if (!flood_stack_push(x, y - 1)) return false;
    if (y < YRES - BORDER - 1)
      for (x = x1; x <= x2; x++)
        if (elements[TYP(PMAPAT(x, y + 1))].Falldown == 2 && !bm_get(x, y + 1))
          if (!flood_stack_push(x, y + 1)) return false;
  } while (cs_n > 0);
  return false;
}

/* ---------------------------------------------------------------- the pmap, the free list and life (RecalcFreeParticles),
 * counting the particles stacked in a cell (the original's pmap_count) on the way: a bit for the cells with one
 * counted, and the cells with more in a small table (in sim_scratch, until CheckStacking reads it) */
#define STACK_BITMAP ((XRES * YRES + 7) / 8)
#define STACK_SLOTS 256
typedef struct { uint16_t cell, count; } StackCount;
static StackCount *stack_tab;
static int stack_n;
static bool stack_valid;
static void stack_count(int cell) {
  uint8_t *bm = sim_scratch;
  if (!(bm[cell >> 3] >> (cell & 7) & 1)) {
    bm[cell >> 3] |= (uint8_t)(1 << (cell & 7));
    return;
  }
  for (int k = 0; k < stack_n; k++)
    if (stack_tab[k].cell == cell) {
      if (stack_tab[k].count < 0xFFFF) stack_tab[k].count++;
      return;
    }
  if (stack_n < STACK_SLOTS) stack_tab[stack_n++] = (StackCount){(uint16_t)cell, 2};
}

void RecalcFreeParticles(bool do_life_dec) {
  clear_words(pmap_id, sizeof(uint16_t) * XRES * YRES);
  clear_words(photons_id, sizeof(uint16_t) * XRES * YRES);
  clear_words(sim_scratch, (STACK_BITMAP + 3) & ~3);
  stack_tab = (StackCount *)(void *)(sim_scratch + ((STACK_BITMAP + 3) & ~3));
  stack_n = 0;
  stack_valid = true;
  NUM_PARTS = 0;
  for (int i = 0; i < parts_active; i++) {
    Particle *p = &parts[i];
    int t = p->type;
    if (!t) continue;
    int x = (p->x + 32) / 64, y = (p->y + 32) / 64;
    bool inBounds = false;
    if ((unsigned)x < XRES && (unsigned)y < YRES) {
      if (elements[t].Properties & TYPE_ENERGY) {
        photons_id[y][x] = (uint16_t)(i + 1);
      } else {
        /* particles can be inside INVS and FILT: those don't take the place of one already there */
        if (!pmap_id[y][x] || (t != PT_INVIS && t != PT_FILT)) pmap_id[y][x] = (uint16_t)(i + 1);
        if (t != PT_THDR && t != PT_EMBR && t != PT_FIGH && t != PT_PLSM) stack_count(y * XRES + x);
      }
      inBounds = true;
    }
    NUM_PARTS++;
    if (elementRecount && t >= 0 && t < PT_NUM && elements[t].Enabled) elementCount[t]++;

    if (do_life_dec) {
      if (t < 0 || t >= PT_NUM || !elements[t].Enabled) {
        kill_part(i);
        continue;
      }
      uint32_t props = elements[t].Properties;
      /* (stasis looked at only when it matters) */
#define stasis (inBounds && bmap[y / WCELL][x / WCELL] == WL_STASIS && emap[y / WCELL][x / WCELL] < 8)
      if (p->life > 0 && (props & PROP_LIFE_DEC) && !stasis) {
        p->life--;
        if (p->life <= 0 && (props & (PROP_LIFE_KILL_DEC | PROP_LIFE_KILL))) {
          kill_part(i);
          continue;
        }
      } else if (p->life <= 0 && (props & PROP_LIFE_KILL) && !stasis) {
        kill_part(i);
        continue;
      }
#undef stasis
    }
  }
  sim_flatten_parts();
  if (elementRecount) elementRecount = false;
}

static int stack_cmp(const void *a, const void *b) {
  return (int)((const StackCount *)a)->cell - (int)((const StackCount *)b)->cell;
}
static StackCount *stack_at(int cell) {
  for (int k = 0; k < stack_n; k++)
    if (stack_tab[k].cell == cell) return &stack_tab[k];
  return NULL;
}
/* too many particles in one place make a black hole (CheckStacking); the counts can't reach NPART here, so a
 * count over 5 marked is kept with its top bit */
#define MARKED 0x8000
void CheckStacking(void) {
  bool excessive_stacking_found = false;
  force_stacking_check = false;
  if (!stack_valid) return;
  stack_valid = false;
  qsort(stack_tab, (size_t)stack_n, sizeof(StackCount), stack_cmp);   /* the cells in the original's order */
  for (int k = 0; k < stack_n; k++) {
    int count = stack_tab[k].count, cell = stack_tab[k].cell, x = cell % XRES, y = cell / XRES;
    if (count > 5) {
      if (bmap[y / WCELL][x / WCELL] == WL_EHOLE) {
        if (count > 1500) {   /* more stacking allowed in an E-hole */
          stack_tab[k].count |= MARKED;
          excessive_stacking_found = true;
        }
      } else if (count > 1500 || (unsigned int)rng_between(0, 1599) <= (unsigned int)(count + 100)) {
        stack_tab[k].count |= MARKED;
        excessive_stacking_found = true;
      }
    }
  }
  if (excessive_stacking_found) {
    for (int i = 0; i < parts_active; i++) {
      if (!parts[i].type) continue;
      int t = parts[i].type, x = (parts[i].x + 32) / 64, y = (parts[i].y + 32) / 64;
      if (x >= 0 && y >= 0 && x < XRES && y < YRES && !(elements[t].Properties & TYPE_ENERGY)) {
        StackCount *s = stack_at(y * XRES + x);
        if (s && (s->count & MARKED)) {
          if ((s->count & ~MARKED) != 0) {
            /* stacking -> NBHL */
            int strength = s->count & ~MARKED;
            create_part(i, x, y, PT_NBHL, -1);
            p_set_temp(&parts[i], MAX_TEMP);
            parts[i].tmp = (int16_t)imin(strength, 51200);   /* strength of the gravity field */
            s->count = MARKED;   /* (the original's pmap_count = NPART: the rest are killed) */
          } else {
            kill_part(i);
          }
        }
      }
    }
  }
}

/* ---------------------------------------------------------------- the area cleared (clear_area) */
void clear_area(int area_x, int area_y, int area_w, int area_h) {
  int x0 = imax(area_x, 0), y0 = imax(area_y, 0);
  int x1 = imin(area_x + area_w, XRES), y1 = imin(area_y + area_h, YRES);
  if (x1 <= x0 || y1 <= y0) return;
  area_x = x0, area_y = y0, area_w = x1 - x0, area_h = y1 - y0;
  float fx = area_x - .5f, fy = area_y - .5f;
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type) {
      float px = p_x(&parts[i]), py = p_y(&parts[i]);
      if (px >= fx && px <= fx + area_w + 1 && py >= fy && py <= fy + area_h + 1) kill_part(i);
    }
  int cx1 = area_x / WCELL, cy1 = area_y / WCELL, cx2 = (area_x + area_w) / WCELL, cy2 = (area_y + area_h) / WCELL;
  for (int y = cy1; y <= cy2 && y < YWCELLS; y++)
    for (int x = cx1; x <= cx2 && x < XWCELLS; x++) {
      if (bmap[y][x] == WL_GRAV) gravWallChanged = true;
      bmap[y][x] = 0;
      emap[y][x] = 0;
    }
}

/* ---------------------------------------------------------------- the settings */
void sim_default_settings(void) {
  settings = (SimSettings){
    .heat = true, .ambient_heat = false, .newtonian_gravity = false, .water_equalization = false,
    .air_mode = AIR_ON, .gravity_mode = GRAV_VERTICAL, .edge_mode = EDGE_VOID,
    .custom_gravity_x = 0, .custom_gravity_y = 0, .ambient_air_temp = R_TEMP + 273.15f,
    .edge_pressure = 0, .edge_velocity_x = 0, .edge_velocity_y = 0, .vorticity = 0.1f,
    .convection_mode = AIRC_BOUSSINESQ,
  };
}
void sim_settings_changed(void) {
  legacy_enable = !settings.heat;
  aheat_enable = settings.ambient_heat;
  water_equal_test = settings.water_equalization;
  gravityMode = settings.gravity_mode < NUM_GRAVMODES ? settings.gravity_mode : GRAV_VERTICAL;
  customGravityX = settings.custom_gravity_x;
  customGravityY = settings.custom_gravity_y;
  airMode = settings.air_mode < NUM_AIRMODES ? settings.air_mode : AIR_ON;
  ambientAirTemp = restrict_flt(settings.ambient_air_temp, MIN_TEMP, MAX_TEMP);
  edgePressure = settings.edge_pressure;
  edgeVelocityX = settings.edge_velocity_x;
  edgeVelocityY = settings.edge_velocity_y;
  vorticityCoeff = settings.vorticity;
  convectionMode = settings.convection_mode < NUM_CONVMODES ? settings.convection_mode : AIRC_BOUSSINESQ;
  if (settings.edge_mode != edgeMode) SetEdgeMode(settings.edge_mode < NUM_EDGEMODES ? settings.edge_mode : EDGE_VOID);
  if (settings.newtonian_gravity != ngrav_enable) grav_enable(settings.newtonian_gravity);
}

/* ---------------------------------------------------------------- starting and clearing */
#ifdef HOST
__attribute__((weak)) int host_npart = 0;   /* tests: the particles to make room for (0: as many as the calculator) */
#endif
#if defined(SIM_SCENE) || defined(HOST)
/* speed tests of the simulation (make EXTRA=-DSIM_SCENE=n, with tools/emu.py): a scene from the start.
 * 1: the lower half full (water, sand, oil, a stone floor); 2: wood burning, with gas; 3: as many particles as
 * fit, powders and liquids; 4: the lower half with pressure, Newtonian gravity and ambient heat; 5: the Game of
 * Life over half the grid; 6: electronics (metal, PSCN/NSCN, batteries, sparks) */
static void scene_fill(int x0, int y0, int x1, int y1, int t) {
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) create_part(-1, x, y, t, -1);
}
void sim_scene(int n) {
  int x0 = BORDER, x1 = XRES - BORDER - 1, y1 = YRES - BORDER - 1;
  CreateWallBox(x0, y1 - 1, x1, y1, WL_WALL);
  CreateWallBox(x0, BORDER, x0 + 1, y1, WL_WALL), CreateWallBox(x1 - 1, BORDER, x1, y1, WL_WALL);
  int top = (YRES - 2 * BORDER) / 2 + BORDER, lo = y1 - 2;
  switch (n) {
  case 1: case 4:
    scene_fill(x0 + 2, top, 40, lo, PT_WATR);
    scene_fill(41, top, 70, lo, PT_SAND);
    scene_fill(71, top, x1 - 2, lo, PT_OIL);
    if (n == 4) {
      settings.newtonian_gravity = true, settings.ambient_heat = true;
      sim_settings_changed();
      scene_fill(45, 20, 47, 22, PT_SING);
      scene_fill(20, 15, 23, 18, PT_PLEX);
      scene_fill(20, 19, 23, 19, PT_FIRE);
    }
    break;
  case 2:
    scene_fill(10, 25, 85, lo, PT_WOOD);
    scene_fill(10, 15, 85, 24, PT_GAS);
    scene_fill(10, lo - 1, 85, lo, PT_FIRE);
    break;
  case 3:
    for (int y = BORDER; y <= lo; y++)
      for (int x = x0 + 2; x <= x1 - 2; x++) {
        static const uint8_t mix[] = {PT_SAND, PT_WATR, PT_DUST, PT_OIL, PT_SALT, PT_STNE};
        create_part(-1, x, y, mix[(x / 8 + y / 8) % 6], -1);
      }
    break;
  case 5:
    for (int y = top; y <= lo; y++)
      for (int x = x0 + 2; x <= x1 - 2; x++)
        if (rng_chance(1, 3)) create_part(-1, x, y, PT_LIFE, 0);
    break;
  case 6:
    for (int y = 12; y <= lo; y += 4) {
      scene_fill(10, y, 85, y, (y / 4) % 3 == 0 ? PT_METL : (y / 4) % 3 == 1 ? PT_PSCN : PT_NSCN);
      create_part(-1, 9, y, PT_BTRY, -1);
      scene_fill(10, y + 1, 85, y + 1, PT_WATR);
    }
    break;
  default:
    break;
  }
}
#endif
void sim_init(void) {
  /* the particles take the RAM left after the game's own: on the calculator from _heap_start to _heap_end */
  static bool done;
  if (done) return;
  done = true;
  size_t maps = sizeof(uint16_t) * XRES * YRES;
#ifdef HOST
  size_t room = maps * 2 + SCRATCH + sizeof(Particle) * (size_t)(host_npart ? host_npart : 4000);
  uint8_t *from = (uint8_t *)malloc(room), *to = from + room;
#else
  extern char _heap_start[], _heap_end[];
  uint8_t *from = (uint8_t *)(((uintptr_t)_heap_start + 3) & ~(uintptr_t)3), *to = (uint8_t *)_heap_end;
#endif
  photons_id = (uint16_t (*)[XRES])(void *)from;
  from += maps;
  sim_scratch = from;
  from += SCRATCH;
  if (!pmap_id) {
    pmap_id = (uint16_t (*)[XRES])(void *)from;
    from += maps;
  }
  parts = (Particle *)(void *)from;
  long n = (long)(to - from) / (long)sizeof(Particle);
  npart_max = n > 32767 ? 32767 : n < 0 ? 0 : (int)n;   /* (life holds the free list: 16 bits) */
  sim_default_settings();
  air_make_kernel();
  rng_state[0] = 0x9E3779B97F4A7C15ull, rng_state[1] = 614;
  sim_clear();
  sim_settings_changed();
  UpdateGravityMask();
#ifdef SIM_SCENE
  sim_scene(SIM_SCENE);
#endif
}

void sim_clear(void) {
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type) kill_part(i);
  frameCount = 0;
  emp_decor = 0;
  emp_trigger_count = 0;
  memset(bmap, 0, sizeof bmap);
  memset(emap, 0, sizeof emap);
  parts_reset();
  NUM_PARTS = 0;
  memset(pmap_id, 0, sizeof(uint16_t) * XRES * YRES);
  memset(photons_id, 0, sizeof(uint16_t) * XRES * YRES);
  memset(fvx, 0, sizeof fvx);
  memset(fvy, 0, sizeof fvy);
  memset(wireless, 0, sizeof wireless);
  portal_count = 0;
  memset(pipe_ovf_id, 0, sizeof pipe_ovf_id);
  memset(fighters, 0, sizeof fighters);
  memset(&player, 0, sizeof player);
  memset(&player2, 0, sizeof player2);
  memset(Element_LOLZ_lolz, 0, sizeof Element_LOLZ_lolz);
  memset(Element_LOVE_love, 0, sizeof Element_LOVE_love);
  memset(Element_PSTN_tempParts, 0, sizeof Element_PSTN_tempParts);
  Element_PPIP_ppip_changed = 0;
  memset(elementCount, 0, sizeof elementCount);
  elementRecount = true;
  fighcount = 0;
  player.spwn = 0;
  player.spawnID = -1;
  player.rocketBoots = false;
  player.fan = false;
  player2.spwn = 0;
  player2.spawnID = -1;
  player2.rocketBoots = false;
  player2.fan = false;
  stack_valid = false;
  grav_reset();
  air_clear();
  air_clear_heat();
  SetEdgeMode(edgeMode);
}

/* ---------------------------------------------------------------- the frame around the particles' update */
static const char love_table[9][9] = {
  {0, 0, 1, 1, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0, 0},
  {1, 0, 0, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 1, 0},
  {1, 0, 0, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 1, 1, 0, 0, 0, 0, 0},
};
static const char lolz_table[9][9] = {
  {0, 0, 0, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0, 0},
  {1, 0, 0, 1, 1, 0, 0, 1, 0}, {1, 0, 1, 0, 0, 1, 0, 1, 0}, {1, 0, 1, 0, 0, 1, 0, 1, 0},
  {0, 1, 0, 1, 1, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 0, 1, 0},
};
static void love_lolz(void) {
  int nx, nnx, ny, nny, r, rt;
  for (ny = 0; ny < YRES - 4; ny++)
    for (nx = 0; nx < XRES - 4; nx++) {
      r = PMAPAT(nx, ny);
      if (!r) continue;
      else if ((ny < 9 || nx < 9 || ny > YRES - 7 || nx > XRES - 10) && (parts[ID(r)].type == PT_LOVE || parts[ID(r)].type == PT_LOLZ))
        kill_part(ID(r));
      else if (parts[ID(r)].type == PT_LOVE)
        Element_LOVE_love[nx / 9][ny / 9] = 1;
      else if (parts[ID(r)].type == PT_LOLZ)
        Element_LOLZ_lolz[nx / 9][ny / 9] = 1;
    }
  for (nx = 9; nx <= XRES - 18; nx++)
    for (ny = 9; ny <= YRES - 7; ny++) {
      if (Element_LOVE_love[nx / 9][ny / 9] == 1) {
        for (nnx = 0; nnx < 9; nnx++)
          for (nny = 0; nny < 9; nny++)
            if (ny + nny > 0 && ny + nny < YRES && nx + nnx >= 0 && nx + nnx < XRES) {
              rt = PMAPAT(nx + nnx, ny + nny);
              if (!rt && love_table[nnx][nny] == 1) create_part(-1, nx + nnx, ny + nny, PT_LOVE, -1);
              else if (!rt) continue;
              else if (parts[ID(rt)].type == PT_LOVE && love_table[nnx][nny] == 0) kill_part(ID(rt));
            }
      }
      Element_LOVE_love[nx / 9][ny / 9] = 0;
      if (Element_LOLZ_lolz[nx / 9][ny / 9] == 1) {
        for (nnx = 0; nnx < 9; nnx++)
          for (nny = 0; nny < 9; nny++)
            if (ny + nny > 0 && ny + nny < YRES && nx + nnx >= 0 && nx + nnx < XRES) {
              rt = PMAPAT(nx + nnx, ny + nny);
              if (!rt && lolz_table[nny][nnx] == 1) create_part(-1, nx + nnx, ny + nny, PT_LOLZ, -1);
              else if (!rt) continue;
              else if (parts[ID(rt)].type == PT_LOLZ && lolz_table[nny][nnx] == 0) kill_part(ID(rt));
            }
      }
      Element_LOLZ_lolz[nx / 9][ny / 9] = 0;
    }
}

/* updates the pmap, the Game of Life and the rest that isn't particles, before they're updated */
void BeforeSim(bool willUpdate) {
  if (willUpdate) {
    air_update();
    if (aheat_enable) air_update_heat();
    grav_exchange();
    memset(gravmass, 0, sizeof(float) * XACELLS * YACELLS);   /* (cleared for the frame) */
    if (emp_decor > 0) emp_decor -= emp_decor / 25 + 2;
    if (emp_decor < 0) emp_decor = 0;
    etrd_count_valid = false;
    etrd_life0_count = 0;
    currentTick++;
    elementRecount |= !(currentTick % 180);
    if (elementRecount) memset(elementCount, 0, sizeof elementCount);
  }
  if (gravWallChanged) {
    UpdateGravityMask();
    gravWallChanged = false;
  }
  RecalcFreeParticles(willUpdate);
  if (willUpdate) {
    /* walls' electricity fades; walls block air and ambient heat. (Whether a wall cell is at 12 now, when it
     * sparks conductors touching it: it can't come back to 12 before the next frame's fading) */
    bool at12 = false;
    for (int y = 0; y < YWCELLS; y++)
      for (int x = 0; x < XWCELLS; x++)
        if (emap[y][x]) at12 |= --emap[y][x] == 12;
    emap_any12 = at12;
    air_walls_block();
    /* stacking makes black holes */
    if (force_stacking_check || rng_chance(1, 10)) CheckStacking();
    if (elementCount[PT_LOVE] > 0 || elementCount[PT_LOLZ] > 0) love_lolz();
    /* make WIRE work */
    if (elementCount[PT_WIRE] > 0)
      for (int nx = 0; nx < XRES; nx++)
        for (int ny = 0; ny < YRES; ny++) {
          int r = PMAPAT(nx, ny);
          if (r && parts[ID(r)].type == PT_WIRE) parts[ID(r)].tmp = (int16_t)parts[ID(r)].ctype;
        }
    /* PPIP's triggers from the last frame (kept in flags) become this frame's */
    if (Element_PPIP_ppip_changed) {
      for (int i = 0; i < parts_active; i++)
        if (parts[i].type == PT_PPIP) {
          parts[i].flags |= (uint8_t)((parts[i].flags >> PPIP_LATE_SHIFT) & PPIP_TMPFLAG_TRIGGERS);
          parts[i].flags &= (uint8_t)~(PPIP_TMPFLAG_TRIGGERS << PPIP_LATE_SHIFT);
        }
      Element_PPIP_ppip_changed = 0;
    }
    /* the Game of Life; GSPEED is frames per generation */
    if (elementCount[PT_LIFE] > 0 && ++CGOL >= GSPEED) SimulateGoL();
    /* wifi channels */
    if (ISWIRE > 0) {
      for (int q = 0; q < (int)(MAX_TEMP - 73.15f) / 100 + 2; q++) {
        wireless[q][0] = wireless[q][1];
        wireless[q][1] = 0;
      }
      ISWIRE--;
    }
    /* spawn STKM and STK2 */
    if (!player.spwn && player.spawnID >= 0)
      create_part(-1, (int)p_x(&parts[player.spawnID]), (int)p_y(&parts[player.spawnID]), PT_STKM, -1);
    if (!player2.spwn && player2.spawnID >= 0)
      create_part(-1, (int)p_x(&parts[player2.spawnID]), (int)p_y(&parts[player2.spawnID]), PT_STKM2, -1);
  }
}

void AfterSim(void) {
  if (emp_trigger_count) {
    Element_EMP_Trigger(emp_trigger_count);
    emp_trigger_count = 0;
  }
  frameCount += 1;
}

void sim_step(void) {
  sim_settings_changed();
  BeforeSim(true);
  UpdateParticles(0, NPART);
  AfterSim();
}

void sim_frame(bool update) {
  if (update) {
    sim_step();
  } else {
    sim_settings_changed();
    BeforeSim(false);
  }
}
