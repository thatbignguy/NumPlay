/* NumPowder: The Powder Toy's particle update and movement (Simulation.cpp): eval_move, try_move, do_move,
 * move, the surfaces' normals for reflection and refraction, PlanMove, and UpdateParticles with its phases
 * (GetNeighbourhood, TransitionPhase, MovementPhase). */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O3")   /* (a hot path: a few KB of flash for speed) */
#endif
#include "simpriv.h"

static inline float remainder_p(float x, float y) { return fmodf(x, y) + (x >= 0 ? 0 : y); }
/* (int)(v + 0.5f) of a particle's x or y (truncated, as the original's) */
static inline int ipos(int16_t v) { return (v + 32) / 64; }
/* the type of the particle with this pmap id + 1 (0: none) */
static inline int type_of_id(unsigned id) { return id ? parts[id - 1].type : 0; }

/* ---------------------------------------------------------------- moving into a place
 * eval_move: 0 can't (bounce), 1 swaps, 2 both occupy the same place */
/* eval_move's less common parts: can_move's 3 (it depends on the particle there), and walls */
static __attribute__((noinline)) int eval_move_3(int pt, int nx, int ny, unsigned r) {
  int result = 3;
  const Particle *q = &parts[ID(r)];
  switch (TYP(r)) {
  case PT_LCRY:
    if (pt == PT_PHOT) result = (q->life > 5) ? 2 : 0;
    break;
  case PT_GPMP:
    if (pt == PT_PHOT) result = (q->life < 10) ? 2 : 0;
    break;
  case PT_INVIS: {
    float pressureResistance = q->tmp > 0 ? (float)q->tmp : 4.0f;
    float pres = pv[ny / ACELL][nx / ACELL];
    result = (pres < -pressureResistance || pres > pressureResistance) ? 2 : 0;
    break;
  }
  case PT_PVOD:
    if (q->life == 10) result = (!q->ctype || (q->ctype == pt) != (q->tmp & 1)) ? 1 : 0;
    else result = 0;
    break;
  case PT_VOID:
    result = (!q->ctype || (q->ctype == pt) != (q->tmp & 1)) ? 1 : 0;
    break;
  case PT_SWCH:
    if (pt == PT_TRON) return q->life >= 10 ? -2 : -1;   /* (the answer, walls or not: -2 for 2, -1 for 0) */
    break;
  default:
    result = 1;   /* "this should never happen" */
  }
  return result;
}
static __attribute__((noinline)) int eval_move_wall(int pt, int nx, int ny, unsigned r, int result) {
  int wall = bmap[ny / WCELL][nx / WCELL];
  if (IsWallBlocking(nx, ny, pt)) return 0;
  if (wall == WL_EHOLE && !emap[ny / WCELL][nx / WCELL] && !(elements[pt].Properties & TYPE_SOLID) &&
      !(elements[TYP(r)].Properties & TYPE_SOLID))
    return 2;
  return result;
}
static inline __attribute__((always_inline)) int eval_move_i(int pt, int nx, int ny, unsigned *rr) {
  if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES) return 0;
  unsigned r = (unsigned)PMAPAT(nx, ny);
  if (rr) *rr = r;
  if (pt >= PT_COUNT || TYP(r) >= PT_COUNT) return 0;
  int result = can_move(pt, TYP(r));
  if (result == 3) {
    result = eval_move_3(pt, nx, ny, r);
    if (result < 0) return result == -2 ? 2 : 0;
  }
  if (bmap[ny / WCELL][nx / WCELL]) return eval_move_wall(pt, nx, ny, r, result);
  return result;
}

int eval_move(int pt, int nx, int ny, unsigned *rr) { return eval_move_i(pt, nx, ny, rr); }

/* try_move's less common parts, apart so that its usual way stays short: */
/* a move refused (r is there): WOOD hit hard, and the energy particles' */
static __attribute__((noinline)) int try_move_refused(int i, int x, int y, int nx, int ny, unsigned r) {
  Particle *p = &parts[i];
  int rt = TYP(r);
  if (rt == PT_WOOD) {
    /* WOOD -> SAWD */
    float vel = sqrtf(p_vx(p) * p_vx(p) + p_vy(p) * p_vy(p));
    if (vel > 5) part_change_type(ID(r), nx, ny, PT_SAWD);
  }
  if (!(elements[p->type].Properties & TYPE_ENERGY)) return 0;
  Particle *q = &parts[ID(r)];
  if (!legacy_enable && p->type == PT_PHOT && r) {   /* PHOT heat conduction */
    if (rt == PT_COAL || rt == PT_BCOL) q->temp = p->temp;
    if (rt < PT_NUM && !IsHeatInsulator(q) && rt != PT_FILT) {
      p_set_temp(p, restrict_flt((p_temp(q) + p_temp(p)) / 2, MIN_TEMP, MAX_TEMP));
      q->temp = p->temp;
    }
  } else if ((p->type == PT_NEUT || p->type == PT_ELEC) && (rt == PT_CLNE || rt == PT_PCLN || rt == PT_BCLN || rt == PT_PBCN)) {
    if (!q->ctype) q->ctype = p->type;
  }
  if (rt == PT_PRTI && (elements[p->type].Properties & TYPE_ENERGY)) {
    int count;
    for (count = 0; count < 8; count++)
      if (isign(x - nx) == isign(portal_rx[count]) && isign(y - ny) == isign(portal_ry[count])) break;
    count = count % 8;
    q->tmp = (int16_t)(int)((p_temp(q) - 73.15f) / 100 + 1);
    if (q->tmp >= CHANNELS) q->tmp = CHANNELS - 1;
    else if (q->tmp < 0) q->tmp = 0;
    if (portal_put(q->tmp, count, p)) kill_part(i);
  }
  return 0;
}
/* both in the same place (r is there): what photons, neutrons, electrons, protons and BIZR do */
static __attribute__((noinline)) int try_move_overlap(int i, int x, int y, int nx, int ny, unsigned r) {
  Particle *p = &parts[i];
  Particle *q = &parts[ID(r)];
  switch (p->type) {
  case PT_PHOT:
    switch (TYP(r)) {
    case PT_GLOW:
      if (!q->life && rng_chance(1, 30)) {
        q->life = 120;
        create_gain_photon(i);
      }
      break;
    case PT_FILT:
      p->ctype = Element_FILT_interactWavelengths(q, p->ctype);
      break;
    case PT_C5:
      /* C5's tmp and tmp3 keep the photon's velocity (the original's tmp, 16 bits each), tmp2 its place */
      if (q->life > 0 && (q->ctype & p->ctype & 0xFFFFFFC0)) {
        float cvx = q->tmp / 255.0f, cvy = q->tmp3 / 255.0f;
        float pvx = p_vx(p), pvy = p_vy(p);
        float vn = pvx * pvx + pvy * pvy;
        /* a velocity of 0 would divide by 0: shoot the photon off at 90 degrees instead */
        if (pvx + cvx == 0 && pvy + cvy == 0) {
          p_set_vx(p, cvy);
          p_set_vy(p, -cvx);
        } else {
          p->ctype = (q->ctype & p->ctype) >> 6;
          pvx += cvx;   /* add momentum of photons to each other */
          pvy += cvy;
          vn /= pvx * pvx + pvy * pvy;   /* normalize velocity to original value */
          vn = sqrtf(vn);
          p_set_vx(p, pvx * vn);
          p_set_vy(p, pvy * vn);
        }
        q->life = 0;
        q->ctype = 0;
      } else if (!q->ctype && (p->ctype & 0xFFFFFFC0)) {
        q->life = 1;
        q->ctype = p->ctype;
        q->tmp = (int16_t)(int)(p_vx(p) * 255.0f);
        q->tmp3 = (int16_t)((int)(p_vy(p) * 16711680.0f) >> 16);
        int dx = (int)((p_x(p) - x) * 255.0f), dy = (int)((p_y(p) - y) * 16711680.0f) >> 16;
        q->tmp2 = (int16_t)((dx & 0xFF) | (dy << 8));
        kill_part(i);
      }
      break;
    case PT_INVIS: {
      float pressureResistance = (q->tmp > 0) ? (float)q->tmp : 4.0f;
      float pres = pv[ny / ACELL][nx / ACELL];
      if (pres >= -pressureResistance && pres <= pressureResistance) {
        /* PHOT + INVIS -> NEUT + INVIS */
        part_change_type(i, x, y, PT_NEUT);
        p->ctype = 0;
      }
      break;
    }
    case PT_BIZR:
    case PT_BIZRG:
    case PT_BIZRS:
      /* PHOT + BIZR -> ELEC + BIZR */
      part_change_type(i, x, y, PT_ELEC);
      p->ctype = 0;
      break;
    case PT_H2:
      if (!(p->tmp & 0x1)) {
        /* PHOT + H2 -> PROT + ELEC */
        part_change_type(i, x, y, PT_PROT);
        p->ctype = 0;
        p->tmp2 = 0x1;
        create_part(ID(r), x, y, PT_ELEC, -1);
        return 1;
      }
      break;
    case PT_GPMP:
      if (q->life == 0) {
        /* PHOT + GPMP -> GRVT + GPMP */
        part_change_type(i, x, y, PT_GRVT);
        p->tmp = (int16_t)(int)(p_temp(q) - 273.15f);
      }
      break;
    }
    break;
  case PT_NEUT:
    /* NEUT + GLAS/BGLA -> NEUT + GLAS/BGLA + PHOT */
    if (TYP(r) == PT_GLAS || TYP(r) == PT_BGLA)
      if (rng_chance(1, 10)) create_cherenkov_photon(i);
    break;
  case PT_ELEC:
    if (TYP(r) == PT_GLOW) {
      /* ELEC + GLOW -> PHOT + GLOW */
      part_change_type(i, x, y, PT_PHOT);
      p->ctype = 0x3FFFFFFF;
    }
    break;
  case PT_PROT:
    /* PROT + INVIS -> NEUT + INVIS */
    if (TYP(r) == PT_INVIS) part_change_type(i, x, y, PT_NEUT);
    break;
  case PT_BIZR:
  case PT_BIZRG:
    if (TYP(r) == PT_FILT) p->ctype = Element_FILT_interactWavelengths(q, p->ctype);
    break;
  }
  return 1;
}
/* a swap with r, which eats some particles or is held back by some: 0 no move, 1 on to the swap, -1 eaten */
static __attribute__((noinline)) int try_move_swap_special(int i, int x, int y, int nx, int ny, unsigned r) {
  Particle *p = &parts[i];
  switch (TYP(r)) {
  case PT_VOID:
  case PT_PVOD:
    /* void eats particles (its ctype was checked in eval_move) */
    kill_part(i);
    return -1;
  case PT_BHOL:
  case PT_NBHL:
    /* black holes eat particles */
    if (!legacy_enable) p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)]) + p_temp(p) / 2, MIN_TEMP, MAX_TEMP));
    kill_part(i);
    return -1;
  case PT_WHOL:
  case PT_NWHL:
    /* white holes eat ANAR */
    if (p->type == PT_ANAR) {
      if (!legacy_enable)
        p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)]) - (MAX_TEMP - p_temp(p)) / 2, MIN_TEMP, MAX_TEMP));
      kill_part(i);
      return -1;
    }
    break;
  case PT_DEUT:
    if (p->type == PT_ELEC) {
      if (parts[ID(r)].life < 6000) parts[ID(r)].life += 1;
      p_set_temp(&parts[ID(r)], 0);
      kill_part(i);
      return -1;
    }
    break;
  case PT_VIBR:
  case PT_BVBR:
    if (elements[p->type].Properties & TYPE_ENERGY) {
      parts[ID(r)].tmp += 20;
      kill_part(i);
      return -1;
    }
    break;
  }

  switch (p->type) {
  case PT_NEUT:
    if (elements[TYP(r)].Properties & PROP_NEUTABSORB) {
      kill_part(i);
      return -1;
    }
    break;
  case PT_CNCT: {
    float cnctGravX, cnctGravY;   /* offset from gravity */
    GetGravityField(x, y, elements[PT_CNCT].Gravity, elements[PT_CNCT].Gravity, &cnctGravX, &cnctGravY);
    int offsetX = 0, offsetY = 0;
    if (cnctGravX > 0.0f) offsetX++;
    else if (cnctGravX < 0.0f) offsetX--;
    if (cnctGravY > 0.0f) offsetY++;
    else if (cnctGravY < 0.0f) offsetY--;
    if ((offsetX != 0) != (offsetY != 0) &&   /* not a diagonal */
        ((nx - x) * offsetX > 0 || (ny - y) * offsetY > 0) &&   /* the destination is below */
        (TYP(PMAPAT(x + offsetX, y + offsetY)) == PT_CNCT || TYP(PMAPAT(x + offsetX, y + offsetY)) == PT_ROCK))
      return 0;
    break;
  }
  case PT_GBMB:
    if (p->life > 0) return 0;
    break;
  }

  return 1;
}
/* the elements try_move_overlap and try_move_swap_special have something to do for */
static const uint8_t move_special[PT_NUM] = {
  /* (bit 0: as the one moving into the same place; 1: as the one moving to a swap; 2: as the one swapped with) */
  [PT_PHOT] = 1, [PT_NEUT] = 3, [PT_ELEC] = 1, [PT_PROT] = 1, [PT_BIZR] = 1, [PT_BIZRG] = 1,
  [PT_CNCT] = 2, [PT_GBMB] = 2,
  [PT_VOID] = 4, [PT_PVOD] = 4, [PT_BHOL] = 4, [PT_NBHL] = 4, [PT_WHOL] = 4, [PT_NWHL] = 4, [PT_DEUT] = 4,
  [PT_VIBR] = 4, [PT_BVBR] = 4,
};

/* try_move when it's a swap: 0 no swap or move, 1 still overlapping or moving (the swap happens later) */
static __attribute__((noinline)) int try_move_swap(int i, int x, int y, int nx, int ny, unsigned r) {
  Particle *p = &parts[i];
  int pt = p->type;
  if ((move_special[pt] & 2) || (move_special[TYP(r)] & 4)) {
    int k = try_move_swap_special(i, x, y, nx, ny, r);
    if (k <= 0) return 0;
  }

  if ((bmap[y / WCELL][x / WCELL] == WL_EHOLE && !emap[y / WCELL][x / WCELL]) &&
      !(bmap[ny / WCELL][nx / WCELL] == WL_EHOLE && !emap[ny / WCELL][nx / WCELL]))
    return 0;

  int ri = ID(r);
  if (r) {   /* the swap */
    if (p->type == PT_NEUT) {
      /* the target is NEUTPENETRATE: it's moved around when a neutron passes */
      int s = PMAPAT(x, y);
      if (s && !(elements[TYP(s)].Properties & PROP_NEUTPENETRATE)) return 1;   /* only the neutron moves */
      if (bmap[y / WCELL][x / WCELL] == WL_ALLOWENERGY) return 1;   /* not into an energy-only wall */
      if (s) {
        PMAPSET(nx, ny, s);
        parts[ID(s)].x = (int16_t)(nx * 64);
        parts[ID(s)].y = (int16_t)(ny * 64);
      } else {
        PMAPSET(nx, ny, 0);
      }
      parts[ri].x = (int16_t)(x * 64);
      parts[ri].y = (int16_t)(y * 64);
      PMAPSET(x, y, PMAP(ri, parts[ri].type));
      return 1;
    }
    if (pmap_id[ny][nx] == ri + 1) pmap_id[ny][nx] = 0;
    parts[ri].x = p->x;
    parts[ri].y = p->y;
    int rx = ipos(parts[ri].x), ry = ipos(parts[ri].y);
    pmap_id[ry][rx] = (uint16_t)(ri + 1);
  }
  return 1;
}

/* try_move_i's 0 when nothing was done but look (no WOOD hit, not an energy particle): the same move tried again
 * before anything changes gets the same answer */
#define MOVE_REFUSED (-2)
static inline __attribute__((always_inline)) int try_move_i(int i, int x, int y, int nx, int ny) {
  unsigned r = 0;
  if (x == nx && y == ny) return 1;
  if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES) return 1;
  Particle *p = &parts[i];
  int pt = p->type;
  int e = eval_move_i(pt, nx, ny, &r);

  /* half-silvered mirror */
  if (!e && pt == PT_PHOT && ((TYP(r) == PT_BMTL && rng_chance(1, 2)) || TYP(PMAPAT(x, y)) == PT_BMTL)) e = 2;

  if (!e) {   /* no movement */
    if (TYP(r) != PT_WOOD && !(elements[pt].Properties & TYPE_ENERGY)) return MOVE_REFUSED;
    return try_move_refused(i, x, y, nx, ny, r);
  }

  if (e == 2) {   /* both occupy the same place */
    if (move_special[pt] & 1) return try_move_overlap(i, x, y, nx, ny, r);
    return 1;
  }

  /* else e == 1: swapping */
  return try_move_swap(i, x, y, nx, ny, r);
}

int try_move(int i, int x, int y, int nx, int ny) {
  int r = try_move_i(i, x, y, nx, ny);
  return r == MOVE_REFUSED ? 0 : r;
}

/* the original's move, the place given as a particle keeps it (1/64 particle) */
static inline __attribute__((always_inline)) bool sim_move_q(int i, int x, int y, int16_t qx, int16_t qy) {
  Particle *p = &parts[i];
  int t = p->type;
  p->x = qx;
  p->y = qy;
  int nx = ipos(qx), ny = ipos(qy);   /* (the place the stored position rounds to) */
  if (ny != y || nx != x) {
    if (pmap_id[y][x] == i + 1) pmap_id[y][x] = 0;
    if (photons_id[y][x] == i + 1) photons_id[y][x] = 0;
    /* out of bounds: killed */
    if (nx < BORDER || nx >= XRES - BORDER || ny < BORDER || ny >= YRES - BORDER) {
      kill_part(i);
      return false;
    }
    if (elements[t].Properties & TYPE_ENERGY) photons_id[ny][nx] = (uint16_t)(i + 1);
    else if (t) pmap_id[ny][nx] = (uint16_t)(i + 1);
  }
  return true;
}

/* tries to move a particle, and if it can, moves it: updates pmap and its position. The place is rounded to the
 * particle's 1/64 once: the cell it's in is worked out from that, so it's the one checked (q64) */
static inline void edge_wrap(float *nxf, float *nyf) {
  if (edgeMode == EDGE_LOOP) {
    int nx = (int)(*nxf + 0.5f), ny = (int)(*nyf + 0.5f);
    bool x_ok = (nx >= BORDER && nx < XRES - BORDER);
    bool y_ok = (ny >= BORDER && ny < YRES - BORDER);
    if (!x_ok) *nxf = remainder_p(*nxf - BORDER + .5f, XRES - BORDER * 2.0f) + BORDER - .5f;
    if (!y_ok) *nyf = remainder_p(*nyf - BORDER + .5f, YRES - BORDER * 2.0f) + BORDER - .5f;
  }
}
/* (MOVE_REFUSED for try_move's plain refusal) */
static int do_move_q(int i, int x, int y, int16_t qx, int16_t qy, int nx, int ny) {
  if (parts[i].type == PT_NONE) return 0;
  int result = try_move_i(i, x, y, nx, ny);
  if (result > 0) {
    if (!sim_move_q(i, x, y, qx, qy)) return -1;
  }
  return result;
}
/* the usual refusal, seen without going through try_move: a particle there it can't move into (can_move 0, or
 * either type out of the table), no wall, not WOOD, not an energy particle. Then try_move would only have looked
 * and said MOVE_REFUSED; false when it's anything else (try_move tells) */
static inline __attribute__((always_inline)) bool plainly_refused(int i, int x, int y, int nx, int ny) {
  if ((x == nx && y == ny) || (unsigned)nx >= XRES || (unsigned)ny >= YRES) return false;
  int pt = parts[i].type;
  if (!pt || (elements[pt].Properties & TYPE_ENERGY)) return false;
  unsigned id = pmap_id[ny][nx];
  if (!id || bmap[ny / WCELL][nx / WCELL]) return false;
  int tr = parts[id - 1].type;
  return tr != PT_WOOD && (pt >= PT_COUNT || tr >= PT_COUNT || can_move(pt, tr) == 0);
}
int do_move(int i, int x, int y, float nxf, float nyf) {
  edge_wrap(&nxf, &nyf);
  int16_t qx = np_q16(nxf, P_POS), qy = np_q16(nyf, P_POS);
  int nx = ipos(qx), ny = ipos(qy);
  if (plainly_refused(i, x, y, nx, ny)) return 0;
  int result = do_move_q(i, x, y, qx, qy, nx, ny);
  return result == MOVE_REFUSED ? 0 : result;
}
/* do_move for MovementPhase's tries one after the other, where nothing changes between them but the moves: a
 * place refused (MOVE_REFUSED) is refused again without looking. The memo is the refused move's cells, packed (0:
 * none); `loop` is edgeMode == EDGE_LOOP */
typedef uint32_t MoveMemo;
static inline uint32_t memo_key(int x, int y, int nx, int ny) {
  return 1u | (uint32_t)nx << 1 | (uint32_t)ny << 8 | (uint32_t)x << 16 | (uint32_t)y << 24;
}
static inline int do_move_m(MoveMemo *m, bool loop, int i, int x, int y, float nxf, float nyf) {
  if (loop) edge_wrap(&nxf, &nyf);
  int16_t qx = np_q16(nxf, P_POS), qy = np_q16(nyf, P_POS);
  int nx = ipos(qx), ny = ipos(qy);
  /* (a refused place is on the grid, the particle too: their cells fit the key) */
  if ((unsigned)nx < XRES && (unsigned)ny < YRES) {
    uint32_t key = memo_key(x, y, nx, ny);
    if (*m == key) return 0;
    if (plainly_refused(i, x, y, nx, ny)) {
      *m = key;
      return 0;
    }
  }
  int result = do_move_q(i, x, y, qx, qy, nx, ny);
  if (result == MOVE_REFUSED) {
    *m = memo_key(x, y, nx, ny);
    return 0;
  }
  *m = 0;
  return result;
}
/* plainly_refused for a particle of type pt and a cell holding a particle of type tr, on the grid, without a
 * wall, not the particle's own */
static inline bool refused_by(int pt, int tr) {
  return pt && !(elements[pt].Properties & TYPE_ENERGY) && tr != PT_WOOD && (pt >= PT_COUNT || tr >= PT_COUNT || can_move(pt, tr) == 0);
}

/* the original's move */
bool sim_move(int i, int x, int y, float nxf, float nyf) { return sim_move_q(i, x, y, np_q16(nxf, P_POS), np_q16(nyf, P_POS)); }

/* sparks from PHOT hitting PSCN next to NSCN */
void photoelectric_effect(int nx, int ny) {
  int r = PMAPAT(nx, ny);
  if (TYP(r) == PT_PSCN && !parts[ID(r)].life) {
    if (TYP(PMAPAT(nx - 1, ny)) == PT_NSCN || TYP(PMAPAT(nx + 1, ny)) == PT_NSCN ||
        TYP(PMAPAT(nx, ny - 1)) == PT_NSCN || TYP(PMAPAT(nx, ny + 1)) == PT_NSCN) {
      parts[ID(r)].ctype = PT_PSCN;
      part_change_type(ID(r), nx, ny, PT_SPRK);
      parts[ID(r)].life = 4;
    }
  }
}

/* ---------------------------------------------------------------- surfaces' normals (reflection, refraction) */
static unsigned direction_to_map(float dx, float dy) {
  return (unsigned)(dx >= 0) | (unsigned)((dx + dy) >= 0) << 1 | (unsigned)(dy >= 0) << 2 |
         (unsigned)((dy - dx) >= 0) << 3 | (unsigned)(dx <= 0) << 4 | (unsigned)((dx + dy) <= 0) << 5 |
         (unsigned)(dy <= 0) << 6 | (unsigned)((dy - dx) <= 0) << 7;
}
static int is_blocking(unsigned t, int x, int y) {
  if (t & REFRACT) {
    if (x < 0 || y < 0 || x >= XRES || y >= YRES) return 0;
    int rt = TYP(PMAPAT(x, y));
    return rt == PT_GLAS || rt == PT_BGLA;
  }
  return !eval_move((int)t, x, y, NULL);
}
static int is_boundary(unsigned pt, int x, int y) {
  if (!is_blocking(pt, x, y)) return 0;
  if (is_blocking(pt, x, y - 1) && is_blocking(pt, x, y + 1) && is_blocking(pt, x - 1, y) && is_blocking(pt, x + 1, y)) return 0;
  return 1;
}
static int find_next_boundary(unsigned pt, int *x, int *y, int dm, int *em, bool reverse) {
  static const int dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
  static const int dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int de[8] = {0x83, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xC1};
  if (*x <= 0 || *x >= XRES - 1 || *y <= 0 || *y >= YRES - 1) return 0;
  if (*em != -1) dm &= de[*em];
  unsigned mask = 0;
  for (int i = 0; i < 8; ++i)
    if ((dm & (1 << i)) && is_blocking(pt, *x + dx[i], *y + dy[i])) mask |= 1u << i;
  for (int i = 0; i < 8; ++i) {
    int n = (i + (reverse ? 1 : -1)) & 7;
    if ((mask & (1u << i)) && !(mask & (1u << n))) {
      *x += dx[i];
      *y += dy[i];
      *em = i;
      return 1;
    }
  }
  return 0;
}
static bool get_normal(unsigned pt, int x, int y, float dx, float dy, float *pnx, float *pny) {
  int ldm, rdm, lm, rm, lx, ly, lv, rx, ry, rv, i, j;
  if (!dx && !dy) return false;
  if (!is_boundary(pt, x, y)) return false;
  ldm = (pt & REFRACT) ? 0xFF : (int)direction_to_map(-dy, dx);
  rdm = (pt & REFRACT) ? 0xFF : (int)direction_to_map(dy, -dx);
  lx = rx = x;
  ly = ry = y;
  lv = rv = 1;
  lm = rm = -1;
  j = 0;
  for (i = 0; i < SURF_RANGE; i++) {
    if (lv) lv = find_next_boundary(pt, &lx, &ly, ldm, &lm, true);
    if (rv) rv = find_next_boundary(pt, &rx, &ry, rdm, &rm, false);
    j += lv + rv;
    if (!lv && !rv) break;
  }
  if (j < NORMAL_MIN_EST) return false;
  if ((lx == rx) && (ly == ry)) return false;
  float ex = (float)(rx - lx), ey = (float)(ry - ly);
  float r = 1.0f / hypotf(ex, ey);
  *pnx = ey * r;
  *pny = -ex * r;
  return true;
}
/* get_normal_interp<true>: finds the boundary along the way (sparking PSCN under PHOT), then its normal */
static bool get_normal_interp(unsigned pt, float x0, float y0, float dx, float dy, float *nx, float *ny) {
  int x, y, i;
  dx /= NORMAL_FRAC;
  dy /= NORMAL_FRAC;
  for (i = 0; i < NORMAL_INTERP; i++) {
    x = (int)(x0 + 0.5f);
    y = (int)(y0 + 0.5f);
    if (x < 0 || y < 0 || x >= XRES || y >= YRES) return false;
    if (is_boundary(pt, x, y)) break;
    x0 += dx;
    y0 += dy;
  }
  if (i >= NORMAL_INTERP) return false;
  if (pt == PT_PHOT) photoelectric_effect(x, y);
  return get_normal(pt, x, y, dx, dy, nx, ny);
}
int get_normal_interp_ok(int pt, float x0, float y0, float dx, float dy, float *nx, float *ny) {
  return get_normal_interp((unsigned)pt, x0, y0, dx, dy, nx, ny);
}

/* ---------------------------------------------------------------- planning a move (PlanMove<true>) */
typedef struct {
  int fin_x, fin_y, clear_x, clear_y;
  float fin_xf, fin_yf, clear_xf, clear_yf;
  float vx, vy;
  bool cut;   /* the velocity was cut to MAX_VELOCITY (else it's the particle's) */
} PlanMoveResult;

static PlanMoveResult PlanMove(int i, int x, int y) {
  const Particle *p = &parts[i];
  int t = p->type;
  int fin_x, fin_y, clear_x, clear_y;
  float fin_xf, fin_yf, clear_xf, clear_yf;
  float pvx = p_vx(p), pvy = p_vy(p), px = p_x(p), py = p_y(p);
  float mv = fmaxf(fabsf(pvx), fabsf(pvy));
  bool cut = false;
  if (mv < ISTP || isnan(mv)) {
    clear_x = x;
    clear_y = y;
    clear_xf = px;
    clear_yf = py;
    fin_xf = clear_xf + pvx;
    fin_yf = clear_yf + pvy;
    fin_x = (int)(fin_xf + 0.5f);
    fin_y = (int)(fin_yf + 0.5f);
  } else {
    if (mv > MAX_VELOCITY) {
      pvx *= MAX_VELOCITY / mv;
      pvy *= MAX_VELOCITY / mv;
      mv = MAX_VELOCITY;
      cut = true;
    }
    /* interpolate to see if there is anything in the way */
    float dx = pvx * ISTP / mv, dy = pvy * ISTP / mv;
    fin_xf = px;
    fin_yf = py;
    fin_x = (int)(fin_xf + 0.5f);
    fin_y = (int)(fin_yf + 0.5f);
    bool closedEholeStart = InBounds(fin_x, fin_y) && (bmap[fin_y / WCELL][fin_x / WCELL] == WL_EHOLE && !emap[fin_y / WCELL][fin_x / WCELL]);
    while (1) {
      mv -= ISTP;
      fin_xf += dx;
      fin_yf += dy;
      fin_x = (int)(fin_xf + 0.5f);
      fin_y = (int)(fin_yf + 0.5f);
      if (edgeMode == EDGE_LOOP) {
        bool x_ok = (fin_xf >= BORDER - .5f && fin_xf < XRES - BORDER - .5f);
        bool y_ok = (fin_yf >= BORDER - .5f && fin_yf < YRES - BORDER - .5f);
        if (!x_ok) fin_xf = remainder_p(fin_xf - BORDER + .5f, XRES - BORDER * 2.0f) + BORDER - .5f;
        if (!y_ok) fin_yf = remainder_p(fin_yf - BORDER + .5f, YRES - BORDER * 2.0f) + BORDER - .5f;
        fin_x = (int)(fin_xf + 0.5f);
        fin_y = (int)(fin_yf + 0.5f);
      }
      if (mv <= 0.0f) {
        /* nothing found */
        fin_xf = px + pvx;
        fin_yf = py + pvy;
        if (edgeMode == EDGE_LOOP) {
          bool x_ok = (fin_xf >= BORDER - .5f && fin_xf < XRES - BORDER - .5f);
          bool y_ok = (fin_yf >= BORDER - .5f && fin_yf < YRES - BORDER - .5f);
          if (!x_ok) fin_xf = remainder_p(fin_xf - BORDER + .5f, XRES - BORDER * 2.0f) + BORDER - .5f;
          if (!y_ok) fin_yf = remainder_p(fin_yf - BORDER + .5f, YRES - BORDER * 2.0f) + BORDER - .5f;
        }
        fin_x = (int)(fin_xf + 0.5f);
        fin_y = (int)(fin_yf + 0.5f);
        clear_xf = fin_xf - dx;
        clear_yf = fin_yf - dy;
        clear_x = (int)(clear_xf + 0.5f);
        clear_y = (int)(clear_yf + 0.5f);
        break;
      }
      /* blocked if it can't move (0), or it would be eaten (can_move 3, eval_move 1); photons are slowed by
       * any particle, and the absorb wall blocks particles */
      int eval = eval_move(t, fin_x, fin_y, NULL);
      bool inside = InBounds(fin_x, fin_y);
      if (!eval || (inside && can_move(t, TYP(PMAPAT(fin_x, fin_y))) == 3 && eval == 1) ||
          (t == PT_PHOT && inside && pmap_id[fin_y][fin_x]) ||
          (inside && bmap[fin_y / WCELL][fin_x / WCELL] == WL_DESTROYALL) ||
          (inside && closedEholeStart != (bmap[fin_y / WCELL][fin_x / WCELL] == WL_EHOLE && !emap[fin_y / WCELL][fin_x / WCELL]))) {
        /* found an obstacle */
        clear_xf = fin_xf - dx;
        clear_yf = fin_yf - dy;
        clear_x = (int)(clear_xf + 0.5f);
        clear_y = (int)(clear_yf + 0.5f);
        break;
      }
      if (bmap[fin_y / WCELL][fin_x / WCELL] == WL_DETECT && emap[fin_y / WCELL][fin_x / WCELL] < 8)
        set_emap(fin_x / WCELL, fin_y / WCELL);
    }
  }
  return (PlanMoveResult){fin_x, fin_y, clear_x, clear_y, fin_xf, fin_yf, clear_xf, clear_yf, pvx, pvy, cut};
}

/* ---------------------------------------------------------------- the particles' update */
typedef struct {
  uint16_t id[8];   /* the pmap ids (id + 1, 0: none) around, in the original's order (its surround[]) */
  int surround_space;
  int nt;   /* over 1: something around that isn't the particle's type (for water's movement) */
  float pGravX, pGravY;
} Neighbourhood;

/* GetGravityField where it's only the vertical gravity: without Newtonian gravity its field is 0 everywhere, so it
 * adds nothing (a particle's gravity of -0 is left to it, as adding 0 would make it +0) */
static inline void gravity_field(int x, int y, float particleGrav, float newtonGrav, float *pGravX, float *pGravY) {
  if (gravityMode == GRAV_VERTICAL && !ngrav_enable && !(particleGrav == 0 && signbit(particleGrav))) {
    *pGravX = 0;
    *pGravY = particleGrav;
    return;
  }
  GetGravityField(x, y, particleGrav, newtonGrav, pGravX, pGravY);
}

/* the bytes of w that are 0 (UQSUB8 makes them 1, the others 0; USAD8 adds them up) */
static inline int zero_bytes(uint32_t w) {
#if defined(__ARM_FEATURE_SIMD32) && __ARM_FEATURE_SIMD32
  uint32_t m, r;
  __asm__("uqsub8 %0, %1, %2" : "=r"(m) : "r"(0x01010101u), "r"(w));
  __asm__("usad8 %0, %1, %2" : "=r"(r) : "r"(m), "r"(0u));
  return (int)r;
#else
  return !(w & 0xFFu) + !(w & 0xFF00u) + !(w & 0xFF0000u) + !(w & 0xFF000000u);
#endif
}

static inline void GetNeighbourhood(int t, int x, int y, Neighbourhood *n) {
  const Particle *P = parts;
  const uint16_t *row = &pmap_id[y][x];   /* (x and y are inside the border) */
  /* (the original's order: x - 1, then x, then x + 1, each from y - 1 down) */
  static const int16_t at[8] = {-XRES - 1, -1, XRES - 1, -XRES, XRES, -XRES + 1, 1, XRES + 1};
  /* (their types a byte each in two words: empty space is the bytes at 0, the other types the bytes not at t) */
  uint32_t w[2] = {0, 0};
  n->pGravX = n->pGravY = 0;
  for (int j = 0; j < 8; j++) {
    unsigned id = row[at[j]];
    uint32_t rt = id ? P[id - 1].type : 0;
    n->id[j] = (uint16_t)id;
    w[j >> 2] |= rt << ((j & 3) * 8);
  }
  uint32_t tt = (uint32_t)t * 0x01010101u;
  n->surround_space = zero_bytes(w[0]) + zero_bytes(w[1]);   /* empty space */
  n->nt = 8 - zero_bytes(w[0] ^ tt) - zero_bytes(w[1] ^ tt);   /* empty space and particles of another type */
  if (!(elements[t].Properties & TYPE_SOLID) && (elements[t].Gravity || elements[t].NewtonianGravity))
    gravity_field(x, y, elements[t].Gravity, elements[t].NewtonianGravity, &n->pGravX, &n->pGravY);
}

/* the wall cells a conductor at x, y touches, for sparks from walls: the original checks one, the cell next to
 * it when it's at the edge of its 4-particle cell, its own when in the middle; with 2-particle cells every
 * particle is at an edge, so its own cell and the ones next to it it touches */
static int touched_cells(int x, int y, int *cx, int *cy) {
  int ox = x / WCELL, oy = y / WCELL;
  int dx = x % WCELL == 0 ? -1 : x % WCELL == WCELL - 1 ? 1 : 0;
  int dy = y % WCELL == 0 ? -1 : y % WCELL == WCELL - 1 ? 1 : 0;
  int n = 0;
  const int cand[4][2] = {{ox, oy}, {ox + dx, oy}, {ox, oy + dy}, {ox + dx, oy + dy}};
  for (int k = 0; k < 4; k++) {
    if (k && ((k == 1 && !dx) || (k == 2 && !dy) || (k == 3 && (!dx || !dy)))) continue;
    if (cand[k][0] >= 0 && cand[k][1] >= 0 && cand[k][0] < XWCELLS && cand[k][1] < YWCELLS)
      cx[n] = cand[k][0], cy[n] = cand[k][1], n++;
  }
  return n;
}

/* the elements the heat transfer's exceptions name (IsHeatInsulator's and TransitionPhase's): the others conduct
 * when their HeatConduct isn't 0, with their element's HeatCapacity */
static const uint8_t heat_special[PT_NUM] = {
  [PT_FILT] = 1, [PT_BRAY] = 1, [PT_BIZR] = 1, [PT_BIZRG] = 1, [PT_PHOT] = 1, [PT_ELEC] = 1, [PT_DEUT] = 1,
  [PT_HSWC] = 1, [PT_PIPE] = 1, [PT_PPIP] = 1,
};

/* (x, y: the particle's place, as UpdateParticles worked it out) */
static __attribute__((noinline)) bool TransitionPhase(int i, int x, int y, const Neighbourhood *nb) {
  Particle *p = &parts[i];
  int t = p->type;
  int ax = x / ACELL, ay = y / ACELL;
  bool transitionOccurred = false;
  if (!legacy_enable) {
    float gel_scale = 1.0f;
    if (t == PT_GEL) gel_scale = p->tmp * 2.55f;

    if ((elements[t].Properties & TYPE_LIQUID) && (t != PT_GEL || gel_scale > (1 + rng_between(0, 254)))) {
      float convGravX, convGravY;
      GetGravityField(x, y, -2.0f, -2.0f, &convGravX, &convGravY);
      int offsetX = iclamp((int)roundf(convGravX + x), x - 1, x + 1);
      int offsetY = iclamp((int)roundf(convGravY + y), y - 1, y + 1);
      /* some heat convection for liquids */
      if (offsetX != x || offsetY != y) {
        int r = PMAPAT(offsetX, offsetY);
        if (r && p->type == TYP(r)) {
          if (p->temp > parts[ID(r)].temp) {
            uint16_t swappage = p->temp;
            p->temp = parts[ID(r)].temp;
            parts[ID(r)].temp = swappage;
          }
        }
      }
    }

    /* heat transfer */
    if (t && !IsHeatInsulator(p) && rng_chance((int)(elements[t].HeatConduct * gel_scale), 250)) {
      /* with the air */
      if (aheat_enable && !(elements[t].Properties & PROP_NOAMBHEAT)) {
        float dtemp = hv[ay][ax] - p_temp(p);
        float hc = HeatCapacityOf(p);
        float alpha = fminf(0.04f, 0.4f * hc);   /* alpha / heat_capacity must be < 1 */
        p_set_temp_d(p, restrict_flt(p_temp(p) + alpha * dtemp / hc, MIN_TEMP, MAX_TEMP));
        hv[ay][ax] = restrict_flt(hv[ay][ax] - alpha * dtemp, MIN_TEMP, MAX_TEMP);
      }
      /* with other particles (the original writes the result to all 8, the particle itself for the ones that
       * don't conduct: here only to the ones that do) */
      float hc_total = 0.0f, c_heat = 0.0f;
      uint16_t conduct[8];
      int nc = 0;
      bool tspecial = heat_special[t];
      for (int j = 0; j < 8; j++) {
        unsigned id = nb->id[j];
        if (!id) continue;
        const Particle *q = &parts[id - 1];
        int rt = q->type;
        if (!rt || !elements[rt].HeatConduct) continue;
        if ((tspecial || heat_special[rt]) &&
            (IsHeatInsulator(q) || (t == PT_FILT && (rt == PT_BRAY || rt == PT_BIZR || rt == PT_BIZRG)) ||
             (rt == PT_FILT && (t == PT_BRAY || t == PT_PHOT || t == PT_BIZR || t == PT_BIZRG)) ||
             (t == PT_ELEC && rt == PT_DEUT) || (t == PT_DEUT && rt == PT_ELEC) ||
             (t == PT_HSWC && rt == PT_FILT && p->tmp == 1) || (t == PT_FILT && rt == PT_HSWC && q->tmp == 1)))
          continue;
        conduct[nc++] = (uint16_t)(id - 1);
        float hc = heat_special[rt] ? HeatCapacityOf(q) : elements[rt].HeatCapacity;
        c_heat += p_temp(q) * hc;
        hc_total += hc;
      }
      float hc = HeatCapacityOf(p);
      c_heat += p_temp(p) * hc;
      hc_total += hc;
      /* equilibrium temperature */
      float pt = restrict_flt(c_heat / hc_total, MIN_TEMP, MAX_TEMP);
      p_set_temp_d(p, pt);   /* (one dithered value for all of them, as the original's one) */
      for (int k = 0; k < nc; k++) parts[conduct[k]].temp = p->temp;

      float ctemph = pt, ctempl = pt;
      /* change boiling point with pressure */
      if (((elements[t].Properties & TYPE_LIQUID) && IsElementOrNone(elements[t].HighTemperatureTransition) &&
           (elements[elements[t].HighTemperatureTransition].Properties & TYPE_GAS)) || t == PT_LNTG || t == PT_SLTW)
        ctemph -= 2.0f * pv[ay][ax];
      else if (((elements[t].Properties & TYPE_GAS) && IsElementOrNone(elements[t].LowTemperatureTransition) &&
                (elements[elements[t].LowTemperatureTransition].Properties & TYPE_LIQUID)) || t == PT_WTRV)
        ctempl -= 2.0f * pv[ay][ax];
      int s = 1;

      /* a fix for ice with ctype = 0 */
      if ((t == PT_ICEI || t == PT_SNOW) && (!IsElement(p->ctype) || p->ctype == PT_ICEI || p->ctype == PT_SNOW))
        p->ctype = PT_WATR;

      if (elements[t].HighTemperatureTransition != NT && ctemph >= elements[t].HighTemperature) {
        /* particle type change due to high temperature */
        if (elements[t].HighTemperatureTransition != ST) {
          if (t == PT_FOG) p->ctype = 0;   /* clear unnecessary ctype */
          t = elements[t].HighTemperatureTransition;
        } else if (t == PT_ICEI || t == PT_SNOW) {
          if (p->ctype > 0 && p->ctype < PT_NUM && p->ctype != t) {
            if (elements[p->ctype].LowTemperatureTransition == PT_ICEI || elements[p->ctype].LowTemperatureTransition == PT_SNOW) {
              if (pt < elements[p->ctype].LowTemperature) s = 0;
            } else if (pt < 273.15f) {
              s = 0;
            }
            if (s) {
              t = p->ctype;
              p->ctype = PT_NONE;
              p->life = 0;
            }
          } else {
            s = 0;
          }
        } else if (t == PT_SLTW) {
          /* SLTW -> SALT/WTRV */
          t = rng_chance(1, 4) ? PT_SALT : PT_WTRV;
        } else if (t == PT_BRMT) {
          /* BRMT(TUNG) -> LAVA(TUNG) */
          if (p->ctype == PT_TUNG) {
            if (ctemph < elements[p->ctype].HighTemperature) {
              s = 0;
            } else {
              t = PT_LAVA;
              p->type = PT_TUNG;
            }
          } else if (ctemph >= elements[t].HighTemperature) {
            t = PT_LAVA;
          } else {
            s = 0;
          }
        } else if (t == PT_CRMC) {
          float pres = fmaxf((pv[ay][ax] + pv[(y - 2) / ACELL][ax] + pv[(y + 2) / ACELL][ax] + pv[ay][(x - 2) / ACELL] +
                              pv[ay][(x + 2) / ACELL]) * 2.0f, 0.0f);
          if (ctemph < pres + elements[PT_CRMC].HighTemperature) s = 0;
          else t = PT_LAVA;
        } else if (t == PT_RIME) {
          if (p->tmp > 5) {
            /* RIME -> ACID */
            t = PT_ACID;
            p->life = (int16_t)(25 + 5 * p->tmp);
            p->tmp = 0;
          } else {
            /* RIME -> WATR */
            t = p->ctype == PT_DSTW ? PT_DSTW : PT_WATR;
            p->ctype = 0;
          }
        } else {
          s = 0;
        }
      } else if (elements[t].LowTemperatureTransition != NT && ctempl < elements[t].LowTemperature) {
        /* particle type change due to low temperature */
        if (elements[t].LowTemperatureTransition != ST) {
          t = elements[t].LowTemperatureTransition;
        } else if (t == PT_WTRV) {
          /* WTRV -> RIME/DSTW */
          t = (pt < 273.0f) ? PT_RIME : PT_DSTW;
        } else if (t == PT_LAVA) {
          if (p->ctype > 0 && p->ctype < PT_NUM && p->ctype != PT_LAVA && elements[p->ctype].Enabled) {
            if (p->ctype == PT_THRM && pt >= elements[PT_BMTL].HighTemperature) {
              s = 0;
            } else if ((p->ctype == PT_VIBR || p->ctype == PT_BVBR) && pt >= 273.15f) {
              s = 0;
            } else if (p->ctype == PT_TUNG) {
              /* TUNG melts in its own update; the threshold is its HighTemperature */
              if (pt >= elements[p->ctype].HighTemperature) s = 0;
            } else if (p->ctype == PT_CRMC) {
              float pres = fmaxf((pv[ay][ax] + pv[(y - 2) / ACELL][ax] + pv[(y + 2) / ACELL][ax] + pv[ay][(x - 2) / ACELL] +
                                  pv[ay][(x + 2) / ACELL]) * 2.0f, 0.0f);
              if (ctemph >= pres + elements[PT_CRMC].HighTemperature) s = 0;
            } else if (elements[p->ctype].HighTemperatureTransition == PT_LAVA || p->ctype == PT_HEAC) {
              if (pt >= elements[p->ctype].HighTemperature) s = 0;
            } else if (pt >= 973.0f) {
              s = 0;   /* freezing point for lava with any other ctype */
            }
            if (s) {
              t = p->ctype;
              p->ctype = PT_NONE;
              if (t == PT_THRM) {
                /* LAVA(THRM) -> BMTL */
                p->tmp = 0;
                t = PT_BMTL;
              }
              if (t == PT_PLUT) {
                /* LAVA(PLUT) -> LAVA */
                p->tmp = 0;
                t = PT_LAVA;
              }
            }
          } else if (pt < 973.0f) {
            t = PT_STNE;   /* LAVA -> STNE */
          } else {
            s = 0;
          }
        } else {
          s = 0;
        }
      } else {
        s = 0;
      }

      if (s) {   /* a type change */
        if (t == PT_ICEI || t == PT_LAVA || t == PT_SNOW) p->ctype = p->type;
        if (t == PT_RIME) p->ctype = PT_DSTW;
        if (!(t == PT_ICEI && p->ctype == PT_FRZW) && t != PT_ACID) p->life = 0;
        if (t == PT_FIRE) {
          /* if tmp isn't 0 the FIRE might turn into DSTW later */
          p->tmp = 0;
          if (p->type == PT_SEED) {
            p->ctype = 0;
            p->tmp2 = 0;
            p->tmp3 = 0;
            tmp4_set(i, 0);
          }
        }
        if ((elements[t].Properties & TYPE_GAS) && !(elements[p->type].Properties & TYPE_GAS)) pv[ay][ax] += 0.50f;
        if (t == PT_NONE) {
          kill_part(i);
          return true;
        }
        /* part_change_type can refuse and kill the particle (STKM when one exists...) */
        if (part_change_type(i, x, y, t)) return true;
        if (t == PT_FIRE || t == PT_PLSM || t == PT_CFLM) p->life = (int16_t)rng_between(120, 169);
        if (t == PT_LAVA) {
          if (p->ctype == PT_BRMT) p->ctype = PT_BMTL;
          else if (p->ctype == PT_SAND) p->ctype = PT_GLAS;
          else if (p->ctype == PT_BGLA) p->ctype = PT_GLAS;
          else if (p->ctype == PT_PQRT) p->ctype = PT_QRTZ;
          else if (p->ctype == PT_LITH && p->tmp2 > 3) p->ctype = PT_GLAS;
          p->life = (int16_t)rng_between(240, 359);
        }
        transitionOccurred = true;
      }

      /* (temperatures are kept within MIN_TEMP and MAX_TEMP) */
      if (t == PT_LAVA) {
        p->life = (int16_t)(int)restrict_flt((p_temp(p) - 700) / 7, 0, 400);
        if (p->ctype == PT_THRM && p->tmp > 0) {
          p->tmp--;
          p_set_temp(p, 3500);
        }
        if (p->ctype == PT_PLUT && p->tmp > 0) {
          p->tmp--;
          p_set_temp(p, MAX_TEMP);
        }
      }
    } else {
      if (!(bmap_blockairh[ay][ax] & 0x8)) bmap_blockairh[ay][ax]++;
    }
  }

  if (t == PT_LIFE) p_set_temp(p, restrict_flt(p_temp(p) - 50.0f, MIN_TEMP, MAX_TEMP));

  /* sparks from walls (conductors only from a wall cell at 12) */
  if (((elements[t].Properties & PROP_CONDUCTS) && emap_any12) || t == PT_SPRK) {
    int cx[4], cy[4], n = touched_cells(x, y, cx, cy);
    for (int k = 0; k < n; k++) {
      int nx = cx[k], ny = cy[k];
      if (t != PT_SPRK) {
        if (emap[ny][nx] == 12 && !p->life && bmap[ny][nx] != WL_STASIS) {
          part_change_type(i, x, y, PT_SPRK);
          p->life = 4;
          p->ctype = t;
          t = PT_SPRK;
        }
      } else {
        int b = bmap[ny][nx];
        if (b == WL_DETECT || b == WL_EWALL || b == WL_ALLOWLIQUID || b == WL_WALLELEC || b == WL_ALLOWALLELEC || b == WL_EHOLE)
          set_emap(nx, ny);
      }
    }
  }

  /* the basic explosion, from Explosive */
  if ((elements[t].Explosive & 2) && pv[ay][ax] > 2.5f) {
    p->life = (int16_t)rng_between(180, 259);
    p_set_temp(p, restrict_flt(elements[PT_FIRE].DefaultProperties.temp + (elements[t].Flammable / 2), MIN_TEMP, MAX_TEMP));
    t = PT_FIRE;
    part_change_type(i, x, y, t);
    pv[ay][ax] += 0.25f * CFDS;
  }

  {
    int s = 1;
    /* (gravtot, worked out only where it's needed) */
#define gravtot (fabsf(gravx[ay][ax]) + fabsf(gravy[ay][ax]))
    if (elements[t].HighPressureTransition != NT && pv[ay][ax] > elements[t].HighPressure) {
      /* particle type change due to high pressure */
      if (elements[t].HighPressureTransition != ST) {
        t = elements[t].HighPressureTransition;
      } else if (t == PT_BMTL) {
        if (pv[ay][ax] > 2.5f) t = PT_BRMT;
        else if (pv[ay][ax] > 1.0f && p->tmp == 1) t = PT_BRMT;
        else s = 0;
      } else {
        s = 0;
      }
    } else if (elements[t].LowPressureTransition != NT && pv[ay][ax] < elements[t].LowPressure && gravtot <= (elements[t].LowPressure / 4.0f)) {
      /* particle type change due to low pressure */
      if (elements[t].LowPressureTransition != ST) t = elements[t].LowPressureTransition;
      else s = 0;
    } else if (elements[t].HighPressureTransition != NT && gravtot > (elements[t].HighPressure / 4.0f)) {
      /* particle type change due to high gravity */
      if (elements[t].HighPressureTransition != ST) {
        t = elements[t].HighPressureTransition;
      } else if (t == PT_BMTL) {
        if (gravtot > 0.625f) t = PT_BRMT;
        else if (gravtot > 0.25f && p->tmp == 1) t = PT_BRMT;
        else s = 0;
      } else {
        s = 0;
      }
    } else {
      s = 0;
    }
#undef gravtot
    if (s) {
      if (t == PT_NONE) {
        kill_part(i);
        return true;
      }
      p->life = 0;
      /* so that PIPE -> BRMT doesn't set BRMT's ctype */
      if (t == PT_BRMT) {
        tmp4_set(i, 0);
        p->ctype = 0;
        p->tmp = 0;
        p->tmp2 = 0;
        p->tmp3 = 0;
      }
      if (part_change_type(i, x, y, t)) return true;
      if (t == PT_FIRE) p->life = (int16_t)rng_between(120, 169);
      transitionOccurred = true;
    }
  }
  return transitionOccurred;
}

/* the velocity times the element's Collision: at 0 (most liquids), 0 at once */
static inline void vel_scale(Particle *p, float coll) {
  if (coll == 0) {
    p->vx = p->vy = 0;
    return;
  }
  p_set_vx(p, p_vx(p) * coll);
  p_set_vy(p, p_vy(p) * coll);
}

static void MovementPhase(int i, const Neighbourhood *nb) {
  Particle *p = &parts[i];
  int t = p->type;
  int x = ipos(p->x), y = ipos(p->y);
  PlanMoveResult mr = PlanMove(i, x, y);
  int fin_x = mr.fin_x, fin_y = mr.fin_y, clear_x = mr.clear_x, clear_y = mr.clear_y;
  float fin_xf = mr.fin_xf, fin_yf = mr.fin_yf, clear_xf = mr.clear_xf, clear_yf = mr.clear_yf;
  /* (PlanMove's velocity is the particle's unless it cut it to MAX_VELOCITY: then it's kept) */
  if (mr.cut) {
    p_set_vx(p, mr.vx);
    p_set_vy(p, mr.vy);
  }

  int stagnant = p->flags & FLAG_STAGNANT;
  p->flags &= (uint8_t)~FLAG_STAGNANT;

  if (t == PT_STKM || t == PT_STKM2 || t == PT_FIGH) {
    /* head movement: the head passes through anything */
    p_set_x(p, p_x(p) + p_vx(p));
    p_set_y(p, p_y(p) + p_vy(p));
    int nx = ipos(p->x), ny = ipos(p->y);
    if (edgeMode == EDGE_LOOP) {
      bool x_ok = (nx >= BORDER && nx < XRES - BORDER);
      bool y_ok = (ny >= BORDER && ny < YRES - BORDER);
      int oldnx = nx, oldny = ny;
      if (!x_ok) {
        p_set_x(p, remainder_p(p_x(p) - BORDER + .5f, XRES - BORDER * 2.0f) + BORDER - .5f);
        nx = ipos(p->x);
      }
      if (!y_ok) {
        p_set_y(p, remainder_p(p_y(p) - BORDER + .5f, YRES - BORDER * 2.0f) + BORDER - .5f);
        ny = ipos(p->y);
      }
      if (!x_ok || !y_ok) {   /* adjust the stickman's legs */
        playerst *stickman = NULL;
        if (t == PT_STKM) stickman = &player;
        else if (t == PT_STKM2) stickman = &player2;
        else if (t == PT_FIGH && p->tmp >= 0 && p->tmp < MAX_FIGHTERS) stickman = &fighters[p->tmp];
        if (stickman)
          for (int k = 0; k < 16; k += 2) {
            stickman->legs[k] += (float)(nx - oldnx);
            stickman->legs[k + 1] += (float)(ny - oldny);
            stickman->accs[k / 2] *= .95f;
          }
        p_set_vy(p, p_vy(p) * .95f);
        p_set_vx(p, p_vx(p) * .95f);
      }
    }
    if (ny != y || nx != x) {
      if (pmap_id[y][x] == i + 1) pmap_id[y][x] = 0;
      else if (photons_id[y][x] == i + 1) photons_id[y][x] = 0;
      if (nx < BORDER || nx >= XRES - BORDER || ny < BORDER || ny >= YRES - BORDER) {
        kill_part(i);
        return;
      }
      if (elements[t].Properties & TYPE_ENERGY) photons_id[ny][nx] = (uint16_t)(i + 1);
      else if (t) pmap_id[ny][nx] = (uint16_t)(i + 1);
    }
  } else if (elements[t].Properties & TYPE_ENERGY) {
    if (t == PT_PHOT) {
      if (p->flags & FLAG_SKIPMOVE) {
        p->flags &= (uint8_t)~FLAG_SKIPMOVE;
        return;
      }
      if (eval_move(PT_PHOT, fin_x, fin_y, NULL)) {
        int rt = TYP(PMAPAT(fin_x, fin_y)), lt = TYP(PMAPAT(x, y));
        int rt_glas = (rt == PT_GLAS) || (rt == PT_BGLA), lt_glas = (lt == PT_GLAS) || (lt == PT_BGLA);
        if ((rt_glas && !lt_glas) || (lt_glas && !rt_glas)) {
          float nrx, nry;
          if (!get_normal_interp(REFRACT | (unsigned)t, p_x(p), p_y(p), p_vx(p), p_vy(p), &nrx, &nry)) {
            kill_part(i);
            return;
          }
          int ctype = p->ctype;
          int r = get_wavelength_bin(&ctype);
          p->ctype = ctype;
          if (r == -1 || !(p->ctype & 0x3FFFFFFF)) {
            kill_part(i);
            return;
          }
          float nn = GLASS_IOR - GLASS_DISP * (r - 30) / 30.0f;
          nn *= nn;
          bool enter = rt_glas && !lt_glas;
          nrx = enter ? -nrx : nrx;
          nry = enter ? -nry : nry;
          nn = enter ? 1.0f / nn : nn;
          float pvx = p_vx(p), pvy = p_vy(p);
          float ct1 = pvx * nrx + pvy * nry;
          float ct2 = 1.0f - (nn * nn) * (1.0f - (ct1 * ct1));
          if (ct2 < 0.0f) {
            /* total internal reflection */
            p_set_vx(p, pvx - 2.0f * ct1 * nrx);
            p_set_vy(p, pvy - 2.0f * ct1 * nry);
            fin_xf = p_x(p);
            fin_yf = p_y(p);
            fin_x = x;
            fin_y = y;
          } else {
            /* refraction */
            ct2 = sqrtf(ct2);
            ct2 = ct2 - nn * ct1;
            p_set_vx(p, nn * pvx + ct2 * nrx);
            p_set_vy(p, nn * pvy + ct2 * nry);
          }
        }
      }
    }
    if (stagnant) {   /* reflected on the previous frame */
      /* whole coordinates, for compatibility with existing saves */
      if (!do_move(i, x, y, (float)fin_x, (float)fin_y) && p->type) {
        kill_part(i);
        return;
      }
    } else if (!do_move(i, x, y, fin_xf, fin_yf)) {
      if (p->type == PT_NONE) return;
      /* reflection */
      p->flags |= FLAG_STAGNANT;
      if (t == PT_NEUT && rng_chance(1, 10)) {
        kill_part(i);
        return;
      }
      int r = PMAPAT(fin_x, fin_y);
      if ((TYP(r) == PT_PIPE || TYP(r) == PT_PPIP) && !TYP(parts[ID(r)].ctype)) {
        Element_PIPE_transfer_part_to_pipe(p, &parts[ID(r)]);   /* (the original doesn't kill it) */
        return;
      }
      if (t == PT_PHOT) {
        int mask = (int)elements[TYP(r)].PhotonReflectWavelengths;
        if (TYP(r) == PT_LITH) {
          int wl_bin = parts[ID(r)].ctype / 4;
          if (wl_bin < 0) wl_bin = 0;
          if (wl_bin > 25) wl_bin = 25;
          mask = (0x1F << wl_bin);
        } else if (TYP(r) == PT_SEED) {
          /* SEED's colour genes reflect different wavelengths */
          int colour = (parts[ID(r)].ctype >> 6) & 0x3f;   /* PLNT_COLOUR */
          mask |= ((colour & 0x30) != 0) ? 0 : (1 << 25);   /* red */
          mask |= ((colour & 0x0C) != 0) ? 0 : (1 << 15);   /* green */
          mask |= ((colour & 0x03) != 0) ? 0 : (1 << 5);    /* blue */
        }
        p->ctype &= mask;
      }
      float nrx, nry;
      if (get_normal_interp((unsigned)t, p_x(p), p_y(p), p_vx(p), p_vy(p), &nrx, &nry)) {
        float pvx = p_vx(p), pvy = p_vy(p);
        if (TYP(r) == PT_CRMC) {
          float rr = rng_between(-50, 50) * 0.01f;
          rr = rr * rr * rr;
          float rx = cosf(rr), ry = sinf(rr);
          float anrx = rx * nrx + ry * nry, anry = rx * nry - ry * nrx;
          float dp = anrx * pvx + anry * pvy;
          p_set_vx(p, pvx - 2.0f * dp * anrx);
          p_set_vy(p, pvy - 2.0f * dp * anry);
        } else {
          float dp = nrx * pvx + nry * pvy;
          p_set_vx(p, pvx - 2.0f * dp * nrx);
          p_set_vy(p, pvy - 2.0f * dp * nry);
        }
        /* the actual movement waits for the next frame, so that fast particles reflect and refract right */
      } else {
        if (t != PT_NEUT) kill_part(i);
        return;
      }
      if (!(p->ctype & 0x3FFFFFFF) && t == PT_PHOT) {
        kill_part(i);
        return;
      }
    }
  } else if (elements[t].Falldown == 0) {
    /* gases and solids (not powders) */
    if (!do_move(i, x, y, fin_xf, fin_yf)) {
      if (p->type == PT_NONE) return;
      /* can't move there: bounce off */
      if (fin_x > x + ISTP) fin_x = x + ISTP;
      if (fin_x < x - ISTP) fin_x = x - ISTP;
      if (fin_y > y + ISTP) fin_y = y + ISTP;
      if (fin_y < y - ISTP) fin_y = y - ISTP;
      float coll = elements[t].Collision;
      if (do_move(i, x, y, (float)(2 * x - fin_x), (float)fin_y)) {
        p_set_vx(p, p_vx(p) * coll);
      } else if (do_move(i, x, y, (float)fin_x, (float)(2 * y - fin_y))) {
        p_set_vy(p, p_vy(p) * coll);
      } else {
        vel_scale(p, coll);
      }
    }
  } else {
    float coll = elements[t].Collision;
    MoveMemo memo = 0;
    const bool loop = edgeMode == EDGE_LOOP;
    /* water equalization (it doesn't update when changed later, if checking stagnant) */
    if (water_equal_test && elements[t].Falldown == 2 && rng_chance(1, 200))
      if (flood_water(x, y, i)) return;
    /* liquids and powders */
    if (!do_move_m(&memo, loop, i, x, y, fin_xf, fin_yf)) {
      if (p->type == PT_NONE) return;
      if (fin_x != x && do_move_m(&memo, loop, i, x, y, fin_xf, clear_yf)) {
        vel_scale(p, coll);
      } else if (fin_y != y && do_move_m(&memo, loop, i, x, y, clear_xf, fin_yf)) {
        vel_scale(p, coll);
      } else {
        float pGravX = nb->pGravX, pGravY = nb->pGravY;
        int r = (int)(rng_u32() & 1) * 2 - 1;   /* the side searched first (rng_between(0, 1): the low bit) */
        if ((clear_x != x || clear_y != y || nb->nt || nb->surround_space) && (fabsf(p_vx(p)) > 0.01f || fabsf(p_vy(p)) > 0.01f)) {
          /* diagonal movement if the target is blocked, unless stuck among identical particles */
          float dx = p_vx(p) - p_vy(p) * r, dy = p_vy(p) + p_vx(p) * r;
          float mv = fmaxf(fabsf(dx), fabsf(dy));
          dx /= mv;
          dy /= mv;
          if (do_move_m(&memo, loop, i, x, y, clear_xf + dx, clear_yf + dy)) {
            vel_scale(p, coll);
            return;
          }
          {
            float swappage = dx;
            dx = dy * r;
            dy = -swappage * r;
          }
          if (do_move_m(&memo, loop, i, x, y, clear_xf + dx, clear_yf + dy)) {
            vel_scale(p, coll);
            return;
          }
        }
        if (elements[t].Falldown > 1 && !ngrav_enable && gravityMode == GRAV_VERTICAL && p_vy(p) > fabsf(p_vx(p))) {
          int s = 0, rt;
          /* stagnant: FLAG_STAGNANT was set on the previous frame */
          if (!stagnant || nb->nt) rt = 30;   /* slightly less water lag, though it changes how it moves a lot */
          else rt = 10;
          if (t == PT_GEL) rt = (int)(p->tmp * 0.20f + 5.0f);
          int nx = -1, ny = -1;
          /* (the rows read once: fin and clear are on the grid here, as do_move didn't take the particle off it;
           * a type is read again after a do_move that didn't move it, as the original reads pmap each time) */
          const uint16_t *rowF = pmap_id[fin_y], *rowC = pmap_id[clear_y];
          const uint8_t *wallF = bmap[fin_y / WCELL], *wallC = bmap[clear_y / WCELL];
          /* (j from clear_x + r while 0 <= j < XRES and clear_x - rt <= j < clear_x + rt) */
          int lo = imax(clear_x - rt, 0), hi = imin(clear_x + rt, XRES) - 1;
          /* a move to (j, fin_yf) or (j, clear_yf) is to that row when it rounds to it (without the loop edges'
           * wrapping): then one plainly refused is seen from the row already read (do_move would say 0) */
          bool rowF_exact = !loop && ipos(np_q16(fin_yf, P_POS)) == fin_y;
          bool rowC_exact = !loop && ipos(np_q16(clear_yf, P_POS)) == clear_y;
          for (int j = clear_x + r; j >= lo && j <= hi; j += r) {
            /* (where this row holds this element, and the row it falls to holds it too or something it's plainly
             * refused by, without walls, nothing happens but going on: the cells passed over the way the code below
             * would) */
            int pt = p->type, ok = -1;
            while (j >= lo && j <= hi) {
              unsigned a = rowF[j], b = rowC[j];
              if (!a || !b || parts[b - 1].type != t || (wallF[(unsigned)j / WCELL] | wallC[(unsigned)j / WCELL])) break;
              int tf = parts[a - 1].type;
              if (tf != t) {
                if (!rowF_exact || (j == x && fin_y == y)) break;
                if (tf != ok) {
                  if (!refused_by(pt, tf)) break;
                  ok = tf;
                }
              }
              j += r;
            }
            if (j < lo || j > hi) break;
            unsigned idF = rowF[j];
            int tf = type_of_id(idF), wF = wallF[(unsigned)j / WCELL];
            if (tf != t || wF) {
              if (rowF_exact && idF && !wF && !(j == x && fin_y == y) && refused_by(p->type, tf)) {
                s = 0;
              } else if ((s = do_move_m(&memo, loop, i, x, y, (float)j, fin_yf))) {
                nx = ipos(p->x);
                ny = ipos(p->y);
                break;
              }
            }
            unsigned idC = rowC[j];
            int tc = type_of_id(idC), wC = wallC[(unsigned)j / WCELL];
            if (fin_y != clear_y && (tc != t || wC)) {
              if (rowC_exact && idC && !wC && !(j == x && clear_y == y) && refused_by(p->type, tc)) {
                s = 0;
              } else {
                if ((s = do_move_m(&memo, loop, i, x, y, (float)j, clear_yf))) {
                  nx = ipos(p->x);
                  ny = ipos(p->y);
                  break;
                }
                tc = type_of_id(rowC[j]);
              }
            }
            if (tc != t || (wC && wC != WL_STREAM)) break;
          }
          r = (p_vy(p) > 0) ? 1 : -1;
          if (s == 1) {
            for (int j = ny + r; j >= 0 && j < YRES && j >= ny - rt && j < ny + rt; j += r) {
              if ((TYP(PMAPAT(nx, j)) != t || bmap[j / WCELL][nx / WCELL]) && do_move_m(&memo, loop, i, nx, ny, (float)nx, (float)j)) break;
              if (TYP(PMAPAT(nx, j)) != t || (bmap[j / WCELL][nx / WCELL] && bmap[j / WCELL][nx / WCELL] != WL_STREAM)) break;
            }
          } else if (s == -1) {
            /* out of bounds */
          } else if ((clear_x != x || clear_y != y) && do_move_m(&memo, loop, i, x, y, clear_xf, clear_yf)) {
          } else {
            p->flags |= FLAG_STAGNANT;
          }
          vel_scale(p, coll);
        } else if (elements[t].Falldown > 1 && fabsf(pGravX * p_vx(p) + pGravY * p_vy(p)) > fabsf(pGravY * p_vx(p) - pGravX * p_vy(p))) {
          float nxf, nyf, prev_pGravX = 0, prev_pGravY = 0, ptGrav = elements[t].Gravity;
          int s = 0;
          int rt = (!stagnant || nb->nt) ? 30 : 10;
          /* clear_xf, clear_yf: the last place it can almost certainly move to */
          nxf = clear_xf;
          nyf = clear_yf;
          int nx = clear_x, ny = clear_y;
          /* look for places to move to perpendicular to gravity, up to rt of them */
          for (int j = 0; j < rt; j++) {
            GetGravityField(nx, ny, ptGrav, 1.0f, &pGravX, &pGravY);
            float mv = fmaxf(fabsf(pGravX), fabsf(pGravY));   /* the largest component made 1 particle */
            if (mv < 0.0001f) break;
            pGravX /= mv;
            pGravY /= mv;
            /* 1 particle perpendicular to gravity, r is +1/-1 at random */
            if (j) {
              /* gravity's direction plus its last change: less frothy, following radial gravity's curve */
              nxf += r * (pGravY * 2.0f - prev_pGravY);
              nyf += -r * (pGravX * 2.0f - prev_pGravX);
            } else {
              nxf += r * pGravY;
              nyf += -r * pGravX;
            }
            prev_pGravX = pGravX;
            prev_pGravY = pGravY;
            nx = (int)(nxf + 0.5f);
            ny = (int)(nyf + 0.5f);
            if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES) break;
            if (TYP(PMAPAT(nx, ny)) != t || bmap[ny / WCELL][nx / WCELL]) {
              s = do_move_m(&memo, loop, i, x, y, nxf, nyf);
              if (s) {
                nx = ipos(p->x);
                ny = ipos(p->y);
                break;
              }
              /* something else, or a wall: stop unless the wall is invisible to particles */
              if (TYP(PMAPAT(nx, ny)) != t || bmap[ny / WCELL][nx / WCELL] != WL_STREAM) break;
            }
          }
          if (s == 1) {
            /* moved perpendicular to gravity: now along it, until blocked by something else, up to rt */
            clear_x = nx;
            clear_y = ny;
            for (int j = 0; j < rt; j++) {
              GetGravityField(nx, ny, ptGrav, 1.0f, &pGravX, &pGravY);
              float mv = fmaxf(fabsf(pGravX), fabsf(pGravY));
              if (mv < 0.0001f) break;
              pGravX /= mv;
              pGravY /= mv;
              nxf += pGravX;
              nyf += pGravY;
              nx = (int)(nxf + 0.5f);
              ny = (int)(nyf + 0.5f);
              if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES) break;
              if (TYP(PMAPAT(nx, ny)) != t || bmap[ny / WCELL][nx / WCELL]) {
                s = do_move_m(&memo, loop, i, clear_x, clear_y, nxf, nyf);
                if (s || TYP(PMAPAT(nx, ny)) != t || bmap[ny / WCELL][nx / WCELL] != WL_STREAM) break;
              }
            }
          } else if (s == -1) {
            /* out of bounds */
          } else if ((clear_x != x || clear_y != y) && do_move_m(&memo, loop, i, x, y, clear_xf, clear_yf)) {
          } else {
            p->flags |= FLAG_STAGNANT;
          }
          vel_scale(p, coll);
        } else {
          /* after interpolation, the last clear place */
          if ((clear_x != x || clear_y != y) && do_move_m(&memo, loop, i, x, y, clear_xf, clear_yf)) {
          } else {
            p->flags |= FLAG_STAGNANT;
          }
          vel_scale(p, coll);
        }
      }
    }
  }
}

/* the main particle loop: every particle, once a frame (parts_active can grow on the way: new particles past
 * the current one are updated in the same frame, as in the original) */
void UpdateParticles(int start, int end) {
  for (int i = start; i < end && i < parts_active; i++) {
    Particle *p = &parts[i];
    int t = p->type;
    if (!t) continue;
    int x = ipos(p->x), y = ipos(p->y);

    /* off the screen: killed */
    if (x < BORDER || y < BORDER || x >= XRES - BORDER || y >= YRES - BORDER) {
      kill_part(i);
      continue;
    }
    const Element *e = &elements[t];
    /* in a wall it isn't allowed in: killed */
    int wall = bmap[y / WCELL][x / WCELL];
    if (wall &&
        (wall == WL_WALL || wall == WL_WALLELEC || wall == WL_ALLOWAIR || wall == WL_DESTROYALL ||
         (wall == WL_ALLOWLIQUID && !(e->Properties & TYPE_LIQUID)) || (wall == WL_ALLOWPOWDER && !(e->Properties & TYPE_PART)) ||
         (wall == WL_ALLOWGAS && !(e->Properties & TYPE_GAS)) || (wall == WL_ALLOWENERGY && !(e->Properties & TYPE_ENERGY)) ||
         (wall == WL_EWALL && !emap[y / WCELL][x / WCELL])) &&
        t != PT_STKM && t != PT_STKM2 && t != PT_FIGH) {
      kill_part(i);
      continue;
    }
    /* stasis: no update */
    if (wall == WL_STASIS && emap[y / WCELL][x / WCELL] < 8) continue;
    if (wall == WL_DETECT && emap[y / WCELL][x / WCELL] < 8) set_emap(x / WCELL, y / WCELL);

    /* the air gets the particle's velocity */
    int ax = x / ACELL, ay = y / ACELL;
    vx[ay][ax] = vx[ay][ax] * e->AirLoss + e->AirDrag * p_vx(p);
    vy[ay][ax] = vy[ay][ax] * e->AirLoss + e->AirDrag * p_vy(p);
    if (e->HotAir) {
      if (t == PT_GAS || t == PT_NBLE) {
        if (pv[ay][ax] < 3.5f) pv[ay][ax] += 4.0f * e->HotAir * (3.5f - pv[ay][ax]);
      } else {
        /* the hot air into the pressure (black hole, white hole) */
        pv[ay][ax] += 4.0f * e->HotAir;
      }
    }

    Neighbourhood nb;
    GetNeighbourhood(t, x, y, &nb);

    /* the particle's velocity: loss, air, gravity, diffusion (one rounding at the end) */
    float pvx = p_vx(p), pvy = p_vy(p);
    if (t != PT_SPNG || !(p->flags & FLAG_MOVABLE)) {
      pvx *= e->Loss;
      pvy *= e->Loss;
    }
    pvx += e->Advection * vx[ay][ax] + nb.pGravX;
    pvy += e->Advection * vy[ay][ax] + nb.pGravY;
    if (e->Diffusion) {   /* gases' random diffusion */
      pvx += e->Diffusion * (2.0f * rng_uniform01() - 1.0f);
      pvy += e->Diffusion * (2.0f * rng_uniform01() - 1.0f);
    }
    p_set_vx(p, pvx);
    p_set_vy(p, pvy);

    bool transitionOccurred = TransitionPhase(i, x, y, &nb);
    if (!p->type) continue;
    if (transitionOccurred) t = p->type;

    /* the element's own update */
    if (elements[t].Update) {
      if (elements[t].Update(i, x, y, nb.surround_space, nb.nt)) continue;
      x = ipos(p->x);
      y = ipos(p->y);
    }
    if (legacy_enable) Element_legacyUpdate(i, x, y, nb.surround_space, nb.nt);   /* heat simulation off */
    if (p->type == PT_NONE) continue;
    if (transitionOccurred) continue;
    if (!p->vx && !p->vy) continue;   /* not moving */
    MovementPhase(i, &nb);
  }
}
