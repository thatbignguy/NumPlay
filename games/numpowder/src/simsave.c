/* NumPowder: saving the simulation to a compact byte stream and loading it, as the original's Simulation::Save,
 * GameSave's OPS format and Simulation::Load (with GameModel::SaveToSimParameters) do.
 *
 * As in OPS, the particles go by place, row by row and in id order within a place, and come back with new ids in
 * that order in the middle of their cells; velocities are kept to 1/16 (a byte each, 127 for none), the air's
 * pressure and velocity to 1/128, the ambient heat to a kelvin, the fans to 1/64; flags aren't kept (but for
 * PIPE's and PLNT's, which hold the original's tmp and tmp4 here); SOAP keeps its forward links and gets its
 * back links from them; stickmen keep their rocket boots and fans and get new legs. Unlike OPS, temperatures keep
 * their 1/4 K (OPS rounds them to a kelvin), and the frame count, the random numbers and the temperature dither
 * are always kept and put back after loading, so a save loaded twice goes on the same way.
 *
 * The stream, little-endian:
 *   "NPWD", version, the particles' count (varint), the settings, the frame count, random numbers and dither;
 *   the walls (bmap, emap: run-length); the fans (a bitmap of the cells that have one, then a byte each way); the
 *   air's pressure, velocities and ambient heat (a bitmap of the cells not at rest, then 16 bits each); the air
 *   blocking maps (run-length); the stickmen's rocket boots and fans; Newtonian gravity's maps when it's on;
 *   the particles, each a byte saying what's as the last one (the next cell or the same one, the element, the
 *   temperature, the velocity) or the element's defaults, then only the rest. Sand, drawn or settled, is about a
 *   byte a grain. */
#include "simpriv.h"
#include "simsave.h"

#define MAGIC "NPWD"
#define VERSION 3   /* (2: temperatures kept as T * 4, read and converted) */

/* what a particle's first byte says */
#define H_NEXTCELL 0x01   /* it's in the cell after the last particle's */
#define H_SAMECELL 0x02   /* it's in the last particle's cell (neither: a varint of the cells skipped, less 2) */
#define H_TYPE 0x04       /* the last particle's element (else a byte) */
#define H_TEMP 0x08       /* the last particle's temperature if the same element, else its default (else the difference) */
#define H_VEL 0x10        /* the last particle's velocity if the same element, else none (else two bytes) */
#define H_FIELDS 0x20     /* life, ctype, tmp, tmp2, tmp3 are the defaults (else a byte saying which aren't, then those) */
#define H_FLAGS 0x40      /* a byte of flags follows */
#define H_EXTRA 0x80      /* a byte of extras follows, then theirs: */
#define X_PIPEHI 0x01     /* PIPE's tmp4's top byte */
#define X_SOAPLINK 0x02   /* a SOAP's forward link: the place in the stream of the one it's linked to, less its own */
#define X_BOOTS 0x08      /* a fighter's rocket boots */
#define X_FAN 0x10        /* a fighter's fan */

/* ---------------------------------------------------------------- the particles' order */
/* the cell a particle is saved in (Save's int(x + 0.5f)), -1 if outside */
static inline int cell_of(int i) {
  int x = (parts[i].x + 32) / 64, y = (parts[i].y + 32) / 64;
  return InBounds(x, y) ? y * XRES + x : -1;
}
/* its place in the stream: its cell, then its id (plus 1; 0: not saved) */
static inline uint32_t key_of(int i) {
  if (!parts[i].type) return 0;
  int c = cell_of(i);
  return c < 0 ? 0 : ((uint32_t)c << 16 | (uint32_t)i) + 1;
}

/* the scratch RAM while saving: a bit for each cell with a particle that isn't on pmap or photons there, then the
 * keys of the next particles to write, as a heap */
#define EXTRA_BYTES ((XRES * YRES + 7) / 8)
#define HEAP_AT ((EXTRA_BYTES + 3) & ~3)
#define HEAP_CAP ((SCRATCH - HEAP_AT) / 4)
_Static_assert(HEAP_CAP >= 256, "the scratch RAM holds a good part of the particles' keys");
static inline uint32_t *heap(void) { return (uint32_t *)(void *)(sim_scratch + HEAP_AT); }
static inline bool has_extras(int c) { return sim_scratch[c >> 3] >> (c & 7) & 1; }

static void mark_extras(void) {
  const uint16_t *pm = &pmap_id[0][0], *ph = &photons_id[0][0];
  memset(sim_scratch, 0, EXTRA_BYTES);
  for (int i = 0; i < parts_active; i++) {
    if (!parts[i].type) continue;
    int c = cell_of(i);
    if (c >= 0 && pm[c] != i + 1 && ph[c] != i + 1) sim_scratch[c >> 3] |= (uint8_t)(1 << (c & 7));
  }
}

static void sift_down(uint32_t *h, int n, uint32_t k) {
  int j = 0;
  for (;;) {
    int c = 2 * j + 1;
    if (c >= n) break;
    if (c + 1 < n && h[c + 1] > h[c]) c++;
    if (h[c] <= k) break;
    h[j] = h[c], j = c;
  }
  h[j] = k;
}
/* the next keys after `after`, as many as the heap holds, in order; how many */
static int next_keys(uint32_t after) {
  uint32_t *h = heap();
  int n = 0;
  for (int i = 0; i < parts_active; i++) {
    uint32_t k = key_of(i);
    if (k <= after) continue;
    if (n < HEAP_CAP) {
      int j = n++;
      while (j > 0 && h[(j - 1) / 2] < k) h[j] = h[(j - 1) / 2], j = (j - 1) / 2;
      h[j] = k;
    } else if (k < h[0]) {
      sift_down(h, n, k);
    }
  }
  for (int end = n - 1; end > 0; end--) {
    uint32_t top = h[0];
    sift_down(h, end, h[end]);
    h[end] = top;
  }
  return n;
}

/* the particles with keys from ka up to (not with) kb: from the maps when every one there is on them */
static int count_keys(uint32_t ka, uint32_t kb) {
  int ca = (int)((ka - 1) >> 16), cb = (int)((kb - 1) >> 16), n = 0;
  bool scan = cb - ca > 2 * XRES;
  for (int c = ca; c <= cb && !scan; c++) scan = has_extras(c);
  if (scan) {
    for (int i = 0; i < parts_active; i++) {
      uint32_t k = key_of(i);
      n += k >= ka && k < kb;
    }
    return n;
  }
  const uint16_t *pm = &pmap_id[0][0], *ph = &photons_id[0][0];
  for (int c = ca; c <= cb; c++) {
    int a = pm[c] - 1, b = ph[c] - 1;
    if (a >= 0 && cell_of(a) == c) {
      uint32_t k = key_of(a);
      n += k >= ka && k < kb;
    }
    if (b >= 0 && b != a && cell_of(b) == c) {
      uint32_t k = key_of(b);
      n += k >= ka && k < kb;
    }
  }
  return n;
}

/* ---------------------------------------------------------------- the values as saved */
static uint32_t air16(float v) {   /* pressure and air velocity: [-255, 255] + 256, times 128 */
  v = fmaxf(-255.0f, fminf(255.0f, v)) + 256.0f;
  return (uint32_t)(int)(v * 128);
}
static float unair16(uint32_t v) { return (float)v / 128.0f - 256; }
static uint32_t heat16(float v) {   /* ambient heat: to a kelvin */
  return !(v >= 0) ? 0 : v >= 65535 ? 65535 : (uint32_t)(int)(v + 0.5f);
}
static float unheat16(uint32_t v) { return (float)v; }
static uint32_t fan8(float v) {   /* fans: 1/64 */
  int t = (int)(v * 64.0f + 127.5f);
  return (uint32_t)(t < 0 ? 0 : t > 255 ? 255 : t);
}
static float unfan8(uint32_t v) { return ((float)v - 127.0f) / 64.0f; }
static uint32_t vel8(int16_t v) {   /* a particle's velocity: 1/16, none if under 0.001 */
  float f = v * (1 / P_VEL);
  if (!(fabsf(f) > 0.001f)) return 127;
  int t = (int)(f * 16.0f + 127.5f);
  return (uint32_t)(t < 0 ? 0 : t > 255 ? 255 : t);
}
static int16_t unvel8(uint32_t v) { return (int16_t)(((int)v - 127) * (int)(P_VEL / 16)); }
static uint16_t default_temp(int t, int version) {
  float d = elements[t].DefaultProperties.temp;
  if (version == 2) return (uint16_t)(restrict_flt(d, MIN_TEMP, MAX_TEMP) * P_TEMP + 0.5f);
  return temp_q(d);
}
/* the flags kept: PIPE's and PPIP's hold the original's tmp bits, PLNT's and SEED's the top of tmp4 */
static uint32_t kept_flags(int t, uint32_t f) {
  if (t == PT_PIPE || t == PT_PPIP) return f & 0xFE;
  if (t == PT_PLNT || t == PT_SEED) return f & 0xE0;
  return 0;
}

/* ---------------------------------------------------------------- writing (out NULL: counting) */
typedef struct {
  uint8_t *out;
  uint32_t n, room;
  bool over;
  int index, lastCell, lastType, lastTemp, lastVx, lastVy;   /* (the particles') */
} W;
static void w8(W *w, uint32_t v) {
  if (w->out) {
    if (w->n < w->room) w->out[w->n] = (uint8_t)v;
    else w->over = true;
  }
  w->n++;
}
static void w32(W *w, uint32_t v) { w8(w, v), w8(w, v >> 8), w8(w, v >> 16), w8(w, v >> 24); }
static void wf(W *w, float f) {
  union { float f; uint32_t u; } c = {f};
  w32(w, c.u);
}
static void wvar(W *w, uint32_t v) {
  while (v >= 0x80) w8(w, (v & 0x7F) | 0x80), v >>= 7;
  w8(w, v);
}
static void wzig(W *w, int32_t v) { wvar(w, ((uint32_t)v << 1) ^ (uint32_t)(v >> 31)); }
static void wrle(W *w, const uint8_t *a, int n) {
  for (int k = 0; k < n;) {
    int run = 1;
    while (k + run < n && run < 255 && a[k + run] == a[k]) run++;
    w8(w, (uint32_t)run), w8(w, a[k]);
    k += run;
  }
}
/* a map's values as q makes them: a bitmap of those not rest, then those, `bytes` each */
static void wmap(W *w, const float *a, int n, uint32_t (*q)(float), uint32_t rest, int bytes) {
  for (int k = 0; k < n; k += 8) {
    uint32_t b = 0;
    for (int j = 0; j < 8 && k + j < n; j++) b |= (uint32_t)(q(a[k + j]) != rest) << j;
    w8(w, b);
  }
  for (int k = 0; k < n; k++) {
    uint32_t v = q(a[k]);
    if (v == rest) continue;
    w8(w, v);
    if (bytes == 2) w8(w, v >> 8);
  }
}
static void wfloats(W *w, const float *a, int n) {   /* floats as they are, the zero ones left out */
  for (int k = 0; k < n; k += 8) {
    uint32_t b = 0;
    for (int j = 0; j < 8 && k + j < n; j++) b |= (uint32_t)(a[k + j] != 0) << j;
    w8(w, b);
  }
  for (int k = 0; k < n; k++)
    if (a[k] != 0) wf(w, a[k]);
}

static void wpart(W *w, int i) {
  const Particle *p = &parts[i];
  int t = p->type, cell = cell_of(i);
  const Element *e = &elements[t];
  int life = p->life, ctype = p->ctype, tmp = p->tmp, tmp2 = p->tmp2, tmp3 = p->tmp3, extra = 0, link = 0;
  uint32_t flags = kept_flags(t, p->flags);
  if ((t == PT_PIPE || t == PT_PPIP) && tmp4_get(i) >> 23 != 0 && tmp4_get(i) >> 23 != -1) extra |= X_PIPEHI;
  if (t == PT_SOAP) {
    /* only the forward link, to a SOAP that's saved, as the place in the stream (as OPS: the back links come
     * from the forward ones) */
    int j = tmp;
    if ((ctype & 2) && j >= 0 && j < parts_active && parts[j].type == PT_SOAP && key_of(j)) {
      uint32_t k = key_of(i), kj = key_of(j);
      link = kj > k ? count_keys(k, kj) : -count_keys(kj, k);
      extra |= X_SOAPLINK;
    }
    if (ctype & 2) tmp = 0;
    if (ctype & 4) tmp2 = 0;
    ctype &= ~6;
  }
  if (t == PT_FIGH) {
    if (tmp >= 0 && tmp < MAX_FIGHTERS) extra |= (fighters[tmp].rocketBoots ? X_BOOTS : 0) | (fighters[tmp].fan ? X_FAN : 0);
    tmp = 0;   /* (its fighter: a new one when loaded) */
  }
  bool same = t == w->lastType;
  int refTemp = same ? w->lastTemp : default_temp(t, VERSION);
  uint32_t qx = vel8(p->vx), qy = vel8(p->vy), refx = same ? (uint32_t)w->lastVx : 127, refy = same ? (uint32_t)w->lastVy : 127;
  uint32_t m = (uint32_t)(life != e->DefaultProperties.life) | (uint32_t)(ctype != e->DefaultProperties.ctype) << 1 |
               (uint32_t)(tmp != (int16_t)e->DefaultProperties.tmp) << 2 | (uint32_t)(tmp2 != (int16_t)e->DefaultProperties.tmp2) << 3 |
               (uint32_t)(tmp3 != (int16_t)e->DefaultProperties.tmp3) << 4;
  uint32_t h = (cell == w->lastCell + 1 ? H_NEXTCELL : cell == w->lastCell ? H_SAMECELL : 0) | (same ? H_TYPE : 0) |
               (p->temp == refTemp ? H_TEMP : 0) | (qx == refx && qy == refy ? H_VEL : 0) | (m ? 0 : H_FIELDS) |
               (flags ? H_FLAGS : 0) | (extra ? H_EXTRA : 0);
  w8(w, h);
  if (!(h & (H_NEXTCELL | H_SAMECELL))) wvar(w, (uint32_t)(cell - w->lastCell - 2));
  if (!(h & H_TYPE)) w8(w, (uint32_t)t);
  if (!(h & H_TEMP)) wzig(w, (int)p->temp - refTemp);
  if (!(h & H_VEL)) w8(w, qx), w8(w, qy);
  if (m) {
    w8(w, m);
    if (m & 1) wzig(w, life);
    if (m & 2) wzig(w, ctype);
    if (m & 4) wzig(w, tmp);
    if (m & 8) wzig(w, tmp2);
    if (m & 16) wzig(w, tmp3);
  }
  if (flags) w8(w, flags);
  if (extra) {
    w8(w, (uint32_t)extra);
    if (extra & X_PIPEHI) w8(w, (uint32_t)(tmp4_get(i) >> 24));
    if (extra & X_SOAPLINK) wzig(w, link);
  }
  w->index++, w->lastCell = cell, w->lastType = t, w->lastTemp = p->temp, w->lastVx = (int)qx, w->lastVy = (int)qy;
}

static void write_all(W *w) {
  int count = 0;
  for (int i = 0; i < parts_active; i++) count += key_of(i) != 0;
  for (int k = 0; k < 4; k++) w8(w, (uint8_t)MAGIC[k]);
  w8(w, VERSION);
  wvar(w, (uint32_t)count);
  /* the settings (SaveToSimParameters's) */
  w8(w, settings.heat), w8(w, settings.ambient_heat), w8(w, settings.newtonian_gravity), w8(w, settings.water_equalization);
  w8(w, settings.air_mode), w8(w, settings.gravity_mode), w8(w, settings.edge_mode), w8(w, settings.convection_mode);
  wf(w, settings.custom_gravity_x), wf(w, settings.custom_gravity_y), wf(w, settings.ambient_air_temp);
  wf(w, settings.edge_pressure), wf(w, settings.edge_velocity_x), wf(w, settings.edge_velocity_y), wf(w, settings.vorticity);
  w32(w, frameCount);
  w32(w, (uint32_t)rng_state[0]), w32(w, (uint32_t)(rng_state[0] >> 32));
  w32(w, (uint32_t)rng_state[1]), w32(w, (uint32_t)(rng_state[1] >> 32));
  w32(w, temp_dither);
  /* walls, fans, air */
  wrle(w, &bmap[0][0], XWCELLS * YWCELLS);
  wrle(w, &emap[0][0], XWCELLS * YWCELLS);
  wmap(w, &fvx[0][0], XACELLS * YACELLS, fan8, 127, 1), wmap(w, &fvy[0][0], XACELLS * YACELLS, fan8, 127, 1);
  wmap(w, &pv[0][0], XACELLS * YACELLS, air16, air16(0), 2), wmap(w, &vx[0][0], XACELLS * YACELLS, air16, air16(0), 2);
  wmap(w, &vy[0][0], XACELLS * YACELLS, air16, air16(0), 2);
  wmap(w, &hv[0][0], XACELLS * YACELLS, heat16, heat16(ambientAirTemp), 2);
  wrle(w, &bmap_blockair[0][0], XACELLS * YACELLS);
  wrle(w, &bmap_blockairh[0][0], XACELLS * YACELLS);
  w8(w, (uint32_t)player.rocketBoots | (uint32_t)player.fan << 1 | (uint32_t)player2.rocketBoots << 2 | (uint32_t)player2.fan << 3);
  /* Newtonian gravity's maps (gravIn's and gravOut's) */
  w8(w, ngrav_enable);
  if (ngrav_enable) {
    wfloats(w, &gravmass[0][0], XACELLS * YACELLS);
    wrle(w, &gravmask[0][0], XACELLS * YACELLS);
    wfloats(w, &gravx[0][0], XACELLS * YACELLS), wfloats(w, &gravy[0][0], XACELLS * YACELLS);
  }
  /* the particles, by place */
  mark_extras();
  w->index = 0, w->lastCell = -1, w->lastType = 0;
  uint32_t after = 0;
  for (;;) {
    int n = next_keys(after);
    if (!n) break;
    const uint32_t *h = heap();
    for (int k = 0; k < n; k++) wpart(w, (int)((h[k] - 1) & 0xFFFF));
    after = heap()[n - 1];
  }
}

uint32_t simsave_size(void) {
  W w = {0};
  write_all(&w);
  return w.n;
}

uint32_t simsave_write(uint8_t *out, uint32_t room) {
  W w = {0};
  w.out = out, w.room = room;
  write_all(&w);
  return w.over ? 0 : w.n;
}

/* ---------------------------------------------------------------- reading */
typedef struct { const uint8_t *d; uint32_t n, len; bool bad; } R;
static uint32_t r8(R *r) {
  if (r->n >= r->len) {
    r->bad = true;
    return 0;
  }
  return r->d[r->n++];
}
static uint32_t r32(R *r) {
  uint32_t v = r8(r);
  v |= r8(r) << 8;
  v |= r8(r) << 16;
  return v | r8(r) << 24;
}
static float rf(R *r) {
  union { uint32_t u; float f; } c;
  c.u = r32(r);
  return c.f;
}
static uint32_t rvar(R *r) {
  uint32_t v = 0;
  for (int s = 0; s < 35; s += 7) {
    uint32_t b = r8(r);
    v |= (b & 0x7F) << s;
    if (!(b & 0x80)) return v;
  }
  r->bad = true;
  return v;
}
static int32_t rzig(R *r) {
  uint32_t v = rvar(r);
  return (int32_t)(v >> 1) ^ -(int32_t)(v & 1);
}
/* into a (NULL: only read) */
static void rrle(R *r, uint8_t *a, int n) {
  for (int k = 0; k < n;) {
    int run = (int)r8(r);
    uint8_t v = (uint8_t)r8(r);
    if (!run || r->bad) {
      r->bad = true;
      return;
    }
    for (int j = 0; j < run && k < n; j++, k++)
      if (a) a[k] = v;
  }
}
static void rmap(R *r, float *a, int n, float (*f)(uint32_t), float rest, int bytes) {
  uint32_t bits = r->n;
  r->n += (uint32_t)(n + 7) / 8;
  if (r->n > r->len) {
    r->bad = true;
    return;
  }
  for (int k = 0; k < n; k++) {
    float v = rest;
    if (r->d[bits + k / 8] >> (k & 7) & 1) {
      uint32_t q = r8(r);
      if (bytes == 2) q |= r8(r) << 8;
      v = f(q);
    }
    if (a) a[k] = v;
  }
}
static void rfloats(R *r, float *a, int n) {
  uint32_t bits = r->n;
  r->n += (uint32_t)(n + 7) / 8;
  if (r->n > r->len) {
    r->bad = true;
    return;
  }
  for (int k = 0; k < n; k++) {
    float v = (r->d[bits + k / 8] >> (k & 7) & 1) ? rf(r) : 0;
    if (a) a[k] = v;
  }
}

int simsave_count(const uint8_t *data, uint32_t len) {
  R r = {data, 0, len, false};
  bool magic = true;
  for (int k = 0; k < 4; k++) magic &= r8(&r) == (uint8_t)MAGIC[k];
  int version = r8(&r);
  if (r.bad || !magic || (version != VERSION && version != 2)) return -1;
  uint32_t n = rvar(&r);
  return r.bad || n > 0xFFFF ? -1 : (int)n;
}

/* the new ids of the particles in the stream: as their places, but from where one wasn't made (or another was
 * made with it, STKM's SPAWN) the difference changes; those changes are kept, a few */
#define ID_CHANGES 64
typedef struct {
  struct { uint16_t from; int16_t delta; } at[ID_CHANGES];
  int n, delta;
  bool lost;
  uint16_t lostFrom;
} IdMap;
#define NOT_MADE INT16_MIN
static void idmap_note(IdMap *m, int index, int id) {
  int delta = id < 0 ? NOT_MADE : id - index;
  if (delta == m->delta || m->lost) return;
  if (m->n == ID_CHANGES) {
    m->lost = true, m->lostFrom = (uint16_t)index;
    return;
  }
  m->at[m->n].from = (uint16_t)index, m->at[m->n].delta = (int16_t)delta;
  m->n++, m->delta = delta;
}
static int idmap_get(const IdMap *m, int index) {
  if (m->lost && index >= m->lostFrom) return -1;
  int delta = 0;
  for (int k = 0; k < m->n && m->at[k].from <= index; k++) delta = m->at[k].delta;
  return delta == NOT_MADE ? -1 : index + delta;
}

/* a particle from the stream, made as Simulation::Load makes it; its id or -1 */
static int load_particle(const Particle *p, int extra, int pipeHi, uint32_t boots) {
  int t = p->type, x = p->x / 64, y = p->y / 64;
  if ((player.spwn == 1 && t == PT_STKM) || (player2.spwn == 1 && t == PT_STKM2)) return -1;
  if ((t == PT_SPAWN || t == PT_SPAWN2) && elementCount[t]) return -1;
  if (t == PT_FIGH && !Element_FIGH_CanAlloc()) return -1;
  if (elements[t].CreateAllowed && !elements[t].CreateAllowed(-3, x, y, t)) return -1;
  int i = create_part(-3, x, y, t, -1);
  if (i < 0) return -1;
  int fighter = parts[i].tmp;   /* (FIGH: the fighter its ChangeType just took) */
  parts[i] = *p;
  if (t == PT_PIPE || t == PT_PPIP) {
    int v = p->ctype >> 8;   /* (24 bits, signed) */
    if (extra & X_PIPEHI) v = (int)(((uint32_t)v & 0xFFFFFF) | (uint32_t)(uint8_t)pipeHi << 24);
    tmp4_set(i, v);
  }
  switch (t) {
  case PT_STKM:
    Element_STKM_init_legs(&player, i);
    player.spwn = 1;
    player.elem = PT_DUST;
    if (boots & 1) player.rocketBoots = true;
    if (boots & 2) player.fan = true;
    break;
  case PT_STKM2:
    Element_STKM_init_legs(&player2, i);
    player2.spwn = 1;
    player2.elem = PT_DUST;
    if (boots & 4) player2.rocketBoots = true;
    if (boots & 8) player2.fan = true;
    break;
  case PT_SPAWN: player.spawnID = i; break;
  case PT_SPAWN2: player2.spawnID = i; break;
  case PT_FIGH:
    /* (the original takes another fighter here and leaves the first one taken; this one keeps it) */
    parts[i].tmp = (int16_t)fighter;
    if (fighter >= 0 && fighter < MAX_FIGHTERS) {
      fighters[fighter].elem = PT_DUST;
      Element_FIGH_NewFighter(fighter, i, parts[i].ctype);
      if (extra & X_BOOTS) fighters[fighter].rocketBoots = true;
      if (extra & X_FAN) fighters[fighter].fan = true;
    } else {
      kill_part(i);
      return -1;
    }
    break;
  default:
    break;
  }
  return i;
}

/* reads it all; real false: only checks it */
static bool read_all(R *r, bool real, int version) {
  int count = (int)rvar(r);
  SimSettings s;
  s.heat = r8(r), s.ambient_heat = r8(r), s.newtonian_gravity = r8(r), s.water_equalization = r8(r);
  s.air_mode = (uint8_t)r8(r), s.gravity_mode = (uint8_t)r8(r), s.edge_mode = (uint8_t)r8(r), s.convection_mode = (uint8_t)r8(r);
  s.custom_gravity_x = rf(r), s.custom_gravity_y = rf(r), s.ambient_air_temp = rf(r);
  s.edge_pressure = rf(r), s.edge_velocity_x = rf(r), s.edge_velocity_y = rf(r), s.vorticity = rf(r);
  uint32_t fc = r32(r);
  uint64_t s0 = r32(r);
  s0 |= (uint64_t)r32(r) << 32;
  uint64_t s1 = r32(r);
  s1 |= (uint64_t)r32(r) << 32;
  uint32_t dither = r32(r);
  if (r->bad) return false;
  if (real) {
    /* SaveToSimParameters, then clear_sim */
    settings = s;
    sim_settings_changed();
    sim_clear();
  }
  rrle(r, real ? &bmap[0][0] : NULL, XWCELLS * YWCELLS);
  rrle(r, real ? &emap[0][0] : NULL, XWCELLS * YWCELLS);
  rmap(r, real ? &fvx[0][0] : NULL, XACELLS * YACELLS, unfan8, 0, 1);
  rmap(r, real ? &fvy[0][0] : NULL, XACELLS * YACELLS, unfan8, 0, 1);
  rmap(r, real ? &pv[0][0] : NULL, XACELLS * YACELLS, unair16, 0, 2);
  rmap(r, real ? &vx[0][0] : NULL, XACELLS * YACELLS, unair16, 0, 2);
  rmap(r, real ? &vy[0][0] : NULL, XACELLS * YACELLS, unair16, 0, 2);
  rmap(r, real ? &hv[0][0] : NULL, XACELLS * YACELLS, unheat16, real ? unheat16(heat16(ambientAirTemp)) : 0, 2);
  rrle(r, real ? &bmap_blockair[0][0] : NULL, XACELLS * YACELLS);
  rrle(r, real ? &bmap_blockairh[0][0] : NULL, XACELLS * YACELLS);
  uint32_t boots = r8(r);
  bool gravMaps = r8(r) != 0;
  if (gravMaps) {
    bool use = real && ngrav_enable;   /* (useGravityMaps) */
    rfloats(r, use ? &gravmass[0][0] : NULL, XACELLS * YACELLS);
    rrle(r, use ? &gravmask[0][0] : NULL, XACELLS * YACELLS);
    if (use) grav_exchange();   /* ResetNewtonianGravity: the worker takes these masses */
    rfloats(r, use ? &gravx[0][0] : NULL, XACELLS * YACELLS);
    rfloats(r, use ? &gravy[0][0] : NULL, XACELLS * YACELLS);
    if (use) grav_force_recalc = true;
  }
  if (r->bad) return false;

  /* the particles */
  IdMap ids;
  ids.n = 0, ids.delta = 0, ids.lost = false, ids.lostFrom = 0;
  int cell = -1, type = 0, temp = 0, qx = 127, qy = 127;
  for (int k = 0; k < count; k++) {
    uint32_t h = r8(r);
    if ((h & H_NEXTCELL) && (h & H_SAMECELL)) return false;
    cell += (h & H_NEXTCELL) ? 1 : (h & H_SAMECELL) ? 0 : (int)rvar(r) + 2;
    bool same = (h & H_TYPE) != 0;
    if (!same) type = (int)r8(r);
    if (r->bad || cell < 0 || cell >= XRES * YRES || type <= 0 || type >= PT_NUM || !elements[type].Enabled) return false;
    const Element *e = &elements[type];
    int refTemp = same ? temp : default_temp(type, version);
    temp = (h & H_TEMP) ? refTemp : refTemp + rzig(r);
    if (!same) qx = qy = 127;
    if (!(h & H_VEL)) qx = (int)r8(r), qy = (int)r8(r);
    Particle p;
    memset(&p, 0, sizeof p);
    p.type = (uint8_t)type;
    p.x = (int16_t)(cell % XRES * 64), p.y = (int16_t)(cell / XRES * 64);
    p.vx = unvel8((uint32_t)qx), p.vy = unvel8((uint32_t)qy);
    p.life = (int16_t)e->DefaultProperties.life, p.ctype = e->DefaultProperties.ctype;
    p.tmp = (int16_t)e->DefaultProperties.tmp, p.tmp2 = (int16_t)e->DefaultProperties.tmp2, p.tmp3 = (int16_t)e->DefaultProperties.tmp3;
    if (!(h & H_FIELDS)) {
      uint32_t m = r8(r);
      if (!m || m > 31) return false;
      if (m & 1) p.life = (int16_t)rzig(r);
      if (m & 2) p.ctype = rzig(r);
      if (m & 4) p.tmp = (int16_t)rzig(r);
      if (m & 8) p.tmp2 = (int16_t)rzig(r);
      if (m & 16) p.tmp3 = (int16_t)rzig(r);
    }
    if (h & H_FLAGS) p.flags = (uint8_t)kept_flags(type, r8(r));
    int extra = 0, pipeHi = 0, link = -1;
    if (h & H_EXTRA) {
      extra = (int)r8(r);
      if (extra & X_PIPEHI) pipeHi = (int)r8(r);
      if (extra & X_SOAPLINK) link = k + rzig(r);
    }
    if (r->bad || temp < 0 || temp > (version == 2 ? (int)(MAX_TEMP * P_TEMP + 0.5f) : T_MAXQ) ||
        (extra & X_SOAPLINK && (link < 0 || link >= count)))
      return false;
    /* (a version 2 save's temperatures, as T * 4: the nearest now, whole degrees Celsius exact) */
    p.temp = version == 2 ? temp_q(temp * (1 / P_TEMP)) : (uint16_t)temp;
    if (!real) continue;
    if (type == PT_SOAP) {
      p.ctype &= ~6;
      if (link >= 0) p.ctype |= 2, p.tmp = (int16_t)(uint16_t)link;   /* (the place in the stream, for now) */
    }
    idmap_note(&ids, k, load_particle(&p, extra, pipeHi, boots));
  }
  if (r->bad) return false;
  if (!real) return true;

  /* as Simulation::Load ends */
  force_stacking_check = true;
  Element_PPIP_ppip_changed = 1;
  /* (clear_sim asked for a recount: the original's RecalcFreeParticles then counts the particles create_part
   * already counted, twice until the next recount; here they're counted once) */
  if (elementRecount) memset(elementCount, 0, sizeof elementCount);
  RecalcFreeParticles(false);
  /* SOAP's links: the forward ones to the new ids, and the back ones from them */
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type == PT_SOAP && (parts[i].ctype & 2)) {
      int j = idmap_get(&ids, (uint16_t)parts[i].tmp);
      if (j >= 0 && j < parts_active && parts[j].type == PT_SOAP) {
        parts[i].tmp = (int16_t)j;
        parts[j].ctype |= 4;
        parts[j].tmp2 = (int16_t)i;
      } else {
        parts[i].ctype &= ~2;
        parts[i].tmp = 0;
      }
    }
  gravWallChanged = true;
  /* the frame count and random numbers as saved (the original sets them before clear_sim, which zeroes the
   * frame count, and the elements' Create functions use the random numbers while loading) */
  frameCount = fc, rng_state[0] = s0, rng_state[1] = s1, temp_dither = dither;
  return true;
}

bool simsave_read(const uint8_t *data, uint32_t len) {
  if (simsave_count(data, len) < 0) return false;
  int version = data[4];
  R check = {data, 5, len, false};
  if (!read_all(&check, false, version)) return false;   /* (all of it checked before anything changes) */
  R r = {data, 5, len, false};
  return read_all(&r, true, version);
}
