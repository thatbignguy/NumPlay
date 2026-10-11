/* The backdrop and the worlds' islands (bg.h), ported from the original's
 * CityTime, graphics.Stars, graphics.BackgroundImages, graphics.Cloud,
 * graphics.WorldImage, graphics.GroundDecoration and World. */
#include "bg.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"

#define W2 (SCREEN_W / 2)
#define H2 (SCREEN_H / 2)

static int floordiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }
static int jsround(double v) { return (int)floor(v + 0.5); }   /* Math.round */
/* n / d as a double: the calculator's build reads constants such as 0.2 as floats
 * (-fsingle-precision-constant), so the island's numbers are written as exact ratios */
#define DBL(n, d) ((double)(n) / (double)(d))

/* ---------------------------------------------------------------- random numbers */
/* random.SeedeableRandom: xorshift128+ on Haxe's Int64 (its right shifts keep the sign) */
typedef struct { uint64_t s0, s1; } SRand;

static void srand_init(SRand *r, int32_t seed) {
  r->s0 = (uint64_t)(int64_t)seed;
  r->s1 = 89432;
}

static uint64_t srand_next(SRand *r) {
  uint64_t s0 = r->s0, s1 = r->s1;
  r->s0 = s1;
  s1 ^= s1 << 23;
  s1 ^= (uint64_t)((int64_t)s1 >> 18);
  s1 ^= s0;
  s1 ^= (uint64_t)((int64_t)s0 >> 5);
  r->s1 = s1;
  return r->s0 + r->s1;
}

/* getInt(min, max): min to max - 1 */
static int srand_int(SRand *r, int min, int max) {
  uint64_t v = srand_next(r) >> 1;
  int64_t n = (int64_t)max - min;
  if (n < 0) n = -n;
  if (!n) return min;   /* (the original throws) */
  return min + (int)(v % (uint64_t)n);
}

/* getFloat(min, max) */
static double srand_float(SRand *r, double min, double max) {
  int32_t low = (int32_t)(uint32_t)srand_next(r);
  return fabs(low / (double)2147483647) * (max - min) + min;
}

/* where the original uses Math.random (random.Random): the stars, the clouds, decorations */
static SRand rng;
static double rnd_float(double min, double max) { return srand_float(&rng, min, max); }
static int rnd_int(int n) { return srand_int(&rng, 0, n); }

/* ---------------------------------------------------------------- the sky's colour */
/* CityTime.skyColors */
static const struct { uint32_t color; float time; } sky_colors[] = {
  {2626656, 0}, {2626656, 4}, {12541833, 6}, {12541833, 6.5f}, {11389183, 8.5f}, {11389183, 11},
  {12247804, 13}, {12247804, 17}, {8872356, 20.5f}, {8872356, 21}, {2626656, 23}, {2626656, 24}};

uint32_t bg_sky_color(float hour) {
  int n = (int)(sizeof sky_colors / sizeof sky_colors[0]);
  for (int i = 0; i + 1 < n; i++) {
    if (hour < sky_colors[i + 1].time) {
      uint32_t a = sky_colors[i].color, b = sky_colors[i + 1].color;
      float t = (hour - sky_colors[i].time) / (sky_colors[i + 1].time - sky_colors[i].time);
      uint32_t c = 0;
      for (int sh = 16; sh >= 0; sh -= 8) {
        float va = (float)(a >> sh & 255), vb = (float)(b >> sh & 255);
        c |= (uint32_t)(int)(va + t * (vb - va)) << sh;
      }
      return c;
    }
  }
  return sky_colors[0].color;
}

float bg_stars_alpha(float t) {
  if (t >= 23 || t < 4) return 1;
  if (t >= 4 && t < 6) return 1 - (t - 4) / 2;
  if (t >= 21 && t < 23) return 1 - (23 - t) / 2;
  return 0;
}

float bg_clouds_alpha(float t) {
  float a;
  if (t >= 21 || t < 4) a = 1;
  else if (t >= 4 && t < 6) a = 1 - (t - 4) / 2;
  else if (t >= 19.5f && t < 21.5f) a = 1 - (23 - t - 1.5f) / 2;
  else a = 0;
  a = (0.76923076923076916f - a) * 1.3f;
  return a < 0 ? 0 : a > 1 ? 1 : a;
}

/* ---------------------------------------------------------------- stars */
/* graphics.Stars: maxStarX * maxStarY / 1000 stars over an 800 x 800 field centred on the
 * screen, which doesn't move with the view. Kept: the ones the screen can show at zoom 1
 * (field x 240..559, y 280..519; zoom 2 shows a part of that). */
#define STAR_FIELD 800
#define STAR_X0 ((STAR_FIELD - SCREEN_W) / 2)
#define STAR_Y0 ((STAR_FIELD - SCREEN_H) / 2)
#define MAX_STARS 128
typedef struct { uint16_t x; uint8_t y, alpha; uint16_t color; } Star;
static Star stars[MAX_STARS];
static int nstars;

/* thx.color.Hsl._c */
static double hsl_c(double d, double s, double l) {
  double m2 = l <= 0.5 ? l * (1 + s) : l + s - l * s, m1 = 2 * l - m2;
  d = fmod(d, 360);
  if (d < 0) d += 360;
  if (d < 60) return m1 + (m2 - m1) * d / 60;
  if (d < 180) return m2;
  if (d < 240) return m1 + (m2 - m1) * (240 - d) / 60;
  return m1;
}

static void make_stars(void) {
  nstars = 0;
  for (int i = 0; i < STAR_FIELD * STAR_FIELD / 1000; i++) {
    rnd_int(STAR_FIELD);    /* posX, posY: drawn, then not used */
    rnd_int(STAR_FIELD);
    double h = rnd_float(0, 360), s = rnd_float(0, 1), l = rnd_float(DBL(9, 10), 1);
    uint32_t r = (uint32_t)jsround(hsl_c(h + 120, s, l) * 255) & 255, g = (uint32_t)jsround(hsl_c(h, s, l) * 255) & 255,
             b = (uint32_t)jsround(hsl_c(h - 120, s, l) * 255) & 255;
    int x = rnd_int(STAR_FIELD), y = rnd_int(STAR_FIELD);
    double a = rnd_float(DBL(2, 10), DBL(6, 10));
    x -= STAR_X0;
    y -= STAR_Y0;
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H || nstars >= MAX_STARS) continue;
    stars[nstars++] = (Star){(uint16_t)x, (uint8_t)y, (uint8_t)jsround(a * 255), rgb565_near(r << 16 | g << 8 | b)};
  }
}

static float stars_now;   /* the stars' stage alpha this frame */

static void draw_stars(const GfxBand *b, void *ctx) {
  (void)ctx;
  int z = b->zoom, ox = (SCREEN_W - STAR_FIELD * z) / 2 + STAR_X0 * z, oy = (SCREEN_H - STAR_FIELD * z) / 2 + STAR_Y0 * z;
  for (int i = 0; i < nstars; i++) {
    const Star *s = &stars[i];
    int x = ox + s->x * z, y = oy + s->y * z;
    if (y + z <= b->cy0 || y >= b->cy1 || x + z <= b->cx0 || x >= b->cx1) continue;
    uint32_t a5 = gfx_alpha32((uint32_t)(s->alpha * stars_now + 0.5f));
    if (!a5) continue;
    for (int yy = y; yy < y + z; yy++) {
      if (yy < b->cy0 || yy >= b->cy1) continue;
      uint16_t *row = b->px + (yy - b->y0) * SCREEN_W;
      for (int xx = x; xx < x + z; xx++)
        if (xx >= b->cx0 && xx < b->cx1) row[xx] = gfx_blend(s->color, row[xx], a5);
    }
  }
}

/* ---------------------------------------------------------------- planets */
static BgPlanet planets[BG_MAX_PLANETS];
static int nplanets;

void bg_set_planets(const BgPlanet *p, int n) {
  if (n > BG_MAX_PLANETS) n = BG_MAX_PLANETS;
  nplanets = n < 0 ? 0 : n;
  if (nplanets) memcpy(planets, p, sizeof *p * (size_t)nplanets);
}

/* ---------------------------------------------------------------- clouds */
/* graphics.Cloud: a cloud is a few 60 x 60 tiles of spr_clouds (16 x 2 of them, 2 pixels
 * apart) chosen to fit each other by their meta (Cloud.texturesMeta). The tiles are made
 * again from the cloud's own seed whenever needed instead of being kept. */
static const uint8_t cloud_meta[31] = {0, 0, 0, 1, 2, 3, 1, 2, 3, 1, 4, 3, 1, 4, 5, 6, 3, 7, 7, 8, 8, 9, 10, 11, 11, 12, 13, 14, 14, 5, 6};
enum { TL_CORNER, TL_SECOND, TL_SECOND_NOUP, TR_CORNER, TR_CORNER_NOUP, BL_CORNER, BL_SECOND, BR_CORNER,
       TL_SIMPLE, C_LEFT, C_RIGHT, C_TOP, C_LEFTRIGHT, C_LEFTRIGHT_NOTOP, CLOUD_LISTS };
static uint8_t cloud_list[CLOUD_LISTS][8], cloud_list_n[CLOUD_LISTS];

static void make_cloud_lists(void) {
  memset(cloud_list_n, 0, sizeof cloud_list_n);
  for (int i = 0; i < 31; i++) {
    int m = cloud_meta[i];
#define TO(l) cloud_list[l][cloud_list_n[l]++] = (uint8_t)i
    if (m == 7 || m == 9 || m == 10) TO(TL_CORNER);
    if (m == 9 || m == 10) TO(TL_SECOND);
    if (m == 10) TO(TL_SECOND_NOUP);
    if (m == 5 || m == 12 || m == 13) TO(TR_CORNER);
    if (m == 5 || m == 13) TO(TR_CORNER_NOUP);
    if (m == 8 || m == 11) TO(BL_CORNER);
    if (m == 11) TO(BL_SECOND);
    if (m == 6 || m == 14) TO(BR_CORNER);
    if (m == 7) TO(TL_SIMPLE);
    if (m == 1) TO(C_LEFT);
    if (m == 3) TO(C_RIGHT);
    if (m == 0) TO(C_TOP);
    if (m == 2 || m == 4) TO(C_LEFTRIGHT);
    if (m == 4) TO(C_LEFTRIGHT_NOTOP);
#undef TO
  }
}

typedef struct {
  float x, y;              /* position */
  uint32_t seed;           /* of its tiles */
  uint8_t layer;           /* 0 (far) to 2 */
  uint8_t alpha;           /* alph, 0..255 */
  int8_t x0, y0, x1, y1;   /* its rect, in tiles */
} Cloud;
static Cloud clouds[BG_MAX_CLOUDS];
static int nclouds;

#define CLOUD_TILES 32
typedef struct {
  struct { int8_t x, y; uint8_t t; } t[CLOUD_TILES];
  int n, x0, y0, x1, y1;
  uint32_t r;              /* the tiles' random numbers (xorshift32) */
} CloudShape;

static float cs_float(CloudShape *c) {
  c->r ^= c->r << 13;
  c->r ^= c->r >> 17;
  c->r ^= c->r << 5;
  return (float)(c->r >> 8) * (1.0f / 16777216.0f);
}
static int cs_pick(CloudShape *c, int list) { return cloud_list[list][(int)(cs_float(c) * cloud_list_n[list])]; }

static void cs_add(CloudShape *c, int x, int y, int t) {
  if (c->n >= CLOUD_TILES) return;
  c->t[c->n].x = (int8_t)x;
  c->t[c->n].y = (int8_t)y;
  c->t[c->n].t = (uint8_t)t;
  c->n++;
  if (x < c->x0) c->x0 = x;
  if (y < c->y0) c->y0 = y;
  if (x + 1 > c->x1) c->x1 = x + 1;
  if (y + 1 > c->y1) c->y1 = y + 1;
}

/* Cloud.makeANiceCloudForMe; layer 0 to 2 (its parallaxLayer is layer + 1) */
static void make_cloud(CloudShape *c, uint32_t seed, int layer) {
  c->n = c->x0 = c->y0 = c->x1 = c->y1 = 0;
  c->r = seed ? seed : 1;
  int pl = 4 - (layer + 1);
  struct { int8_t x, y; } left[16], right[16];
  int nl = 0, nr = 0, second = -1;
  const uint8_t *meta = cloud_meta;
  if (pl == 3 && cs_float(c) < 0.8f) {
    cs_add(c, 0, 0, cs_pick(c, C_LEFT));
    right[nr].x = 1, right[nr++].y = 0;
  } else {
    int tl = cs_pick(c, TL_CORNER), tr = cs_pick(c, TR_CORNER), bl = cs_pick(c, BL_CORNER), br = cs_pick(c, BR_CORNER);
    cs_add(c, 0, 0, tl);
    cs_add(c, 1, 0, tr);
    cs_add(c, 0, 1, bl);
    cs_add(c, 1, 1, br);
    if (meta[tl] == 9 || meta[tl] == 10) left[nl].x = -1, left[nl++].y = 0;
    if (meta[bl] == 11) left[nl].x = -1, left[nl++].y = 1;
    if (cs_float(c) < (pl == 2 ? 0.1f : 0.8f) && pl < 3) {
      int desire = (int)(cs_float(c) * 4);
      bool trs = meta[tr] == 12 || meta[tr] == 13;
      if (desire < 2 && trs && meta[br] == 14) second = 0;
      if (desire == 2 && trs) second = 1;
      if (desire == 3 && meta[br] == 14) second = 2;
    }
    if ((second == -1 || second == 2) && (meta[tr] == 12 || meta[tr] == 13)) right[nr].x = 2, right[nr++].y = 0;
    if ((second == -1 || second == 1) && meta[br] == 14) right[nr].x = 2, right[nr++].y = 1;
    if (meta[tl] == 9) cs_add(c, 0, -1, cs_pick(c, C_TOP));
    if (meta[tr] == 12) cs_add(c, 1, -1, cs_pick(c, C_TOP));
    if (second != -1) {
      int sx = 2, sy = second == 0 ? 0 : second == 1 ? -1 : 1;
      int tl2 = cs_pick(c, second == 1 ? TL_SIMPLE : second == 2 ? TL_SECOND_NOUP : TL_SECOND);
      int tr2 = cs_pick(c, second == 2 ? TR_CORNER_NOUP : TR_CORNER);
      int bl2 = cs_pick(c, second == 2 ? BL_CORNER : BL_SECOND);
      int br2 = cs_pick(c, BR_CORNER);
      cs_add(c, sx, sy, tl2);
      cs_add(c, sx + 1, sy, tr2);
      cs_add(c, sx, sy + 1, bl2);
      cs_add(c, sx + 1, sy + 1, br2);
      if (second == 2 && meta[bl2] == 11) left[nl].x = (int8_t)(sx - 1), left[nl++].y = (int8_t)(sy + 1);
      if (meta[tr2] == 12 || meta[tr2] == 13) right[nr].x = (int8_t)(sx + 2), right[nr++].y = (int8_t)sy;
      if (meta[br2] == 14) right[nr].x = (int8_t)(sx + 2), right[nr++].y = (int8_t)(sy + 1);
      if (meta[tl2] == 9) cs_add(c, sx, sy - 1, cs_pick(c, C_TOP));
      if (meta[tr2] == 12) cs_add(c, sx + 1, sy - 1, cs_pick(c, C_TOP));
    }
  }
  while (nl > 0) {
    int x = left[nl - 1].x, y = left[--nl].y;
    if (x > -1 && cs_float(c) < 0.6f - 0.1f * pl) {
      int t = cs_pick(c, y <= 0 ? C_LEFTRIGHT : C_LEFTRIGHT_NOTOP);
      cs_add(c, x, y, t);
      if (meta[t] == 2) cs_add(c, x, y - 1, cs_pick(c, C_TOP));
      if (nl < 16) left[nl].x = (int8_t)(x - 1), left[nl++].y = (int8_t)y;
    } else {
      cs_add(c, x, y, cs_pick(c, C_LEFT));
    }
  }
  while (nr > 0) {
    int x = right[nr - 1].x, y = right[--nr].y;
    if (x < 3 && cs_float(c) < 0.6f - 0.1f * pl) {
      int t = cs_pick(c, y <= 0 && (second != 1 || y <= -1) ? C_LEFTRIGHT : C_LEFTRIGHT_NOTOP);
      cs_add(c, x, y, t);
      if (meta[t] == 2) cs_add(c, x, y - 1, cs_pick(c, C_TOP));
      if (nr < 16) right[nr].x = (int8_t)(x + 1), right[nr++].y = (int8_t)y;
    } else {
      cs_add(c, x, y, cs_pick(c, C_RIGHT));
    }
  }
}

/* common.Rectangle.intersects */
static bool intersects(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  bool x = (ax >= bx && ax < bx + bw) || (bx >= ax && bx < ax + aw);
  return x && ((by >= ay && by < ay + ah) || (ay >= by && ay < by + bh));
}

#define CLOUD_BASE 0.12500000125f   /* BackgroundImages.cloudSpeedBase, cloudSpeedParallaxMult */
#define PARALLAX 0.2f

/* BackgroundImages.doCloudsStep for one layer */
static void clouds_step(float timeMod, const BgEdges *e, int layer) {
  float p = PARALLAX * (float)(layer + 1);
  float left = (float)e->minX * p - 300 - 300 - W2, right = (float)e->maxX * p + 120 + 300 + W2;
  float miny = (float)e->minY * p - 300 - H2, maxy = (float)e->maxY * p + 300 + H2;
  float chance = (7 - layer * 1.8f) * (maxy - miny) * 0.0000009f;
  if (chance > 0.15f) chance = 0.15f;
  if (rnd_float(0, 1) < timeMod * chance && nclouds < BG_MAX_CLOUDS) {
    Cloud n;
    n.y = (float)rnd_float(miny, maxy);
    n.x = left - 300;
    n.layer = (uint8_t)layer;
    n.alpha = (uint8_t)jsround((DBL(1, 10) + (layer + 1) * DBL(15, 100) + rnd_float(0, DBL(1, 10))) * 255);
    n.seed = (uint32_t)srand_next(&rng);
    CloudShape s;
    make_cloud(&s, n.seed, layer);
    n.x0 = (int8_t)s.x0, n.y0 = (int8_t)s.y0, n.x1 = (int8_t)s.x1, n.y1 = (int8_t)s.y1;
    bool hit = false;
    for (int i = 0; i < nclouds && !hit; i++) {
      const Cloud *c = &clouds[i];
      if (c->layer != layer) continue;
      int dx = jsround(n.x - c->x), dy = jsround(n.y - c->y);
      hit = intersects(c->x0 * 60, c->y0 * 60, (c->x1 - c->x0) * 60, (c->y1 - c->y0) * 60, n.x0 * 60 + dx,
                       n.y0 * 60 + dy, (n.x1 - n.x0) * 60, (n.y1 - n.y0) * 60);
    }
    if (!hit) clouds[nclouds++] = n;
  }
  float speed = CLOUD_BASE * timeMod + CLOUD_BASE * timeMod * (float)(layer + 1);
  int k = 0;
  for (int i = 0; i < nclouds; i++) {
    Cloud c = clouds[i];
    if (c.layer == layer) {
      c.x += speed;
      if (c.x > right) continue;
    }
    clouds[k++] = c;
  }
  nclouds = k;
}

void bg_clouds_start(const BgEdges *e) {
  nclouds = 0;
  for (int layer = 0; layer < 3; layer++)
    for (int i = 0; i < 500 + (e->maxX - e->minX) / 40; i++) clouds_step(40, e, layer);
}

void bg_clouds_update(float timeMod, const BgEdges *e) {
  for (int layer = 0; layer < 3; layer++) clouds_step(timeMod, e, layer);
}

int bg_cloud_count(void) { return nclouds; }

void bg_init(uint32_t seed) {
  srand_init(&rng, (int32_t)seed);
  make_cloud_lists();
  make_stars();
  nclouds = 0;
}

/* ---------------------------------------------------------------- drawing the sky */
static uint32_t frame_no = 1;   /* bg's frames (bg_draw_sky starts one), for the shape cache */
static uint16_t sky565;

void bg_draw_sky(const BgCamera *cam, double time) {
  frame_no++;
  int z = cam->zoom == 2 ? 2 : 1;
  double hour = fmod(time / 60, 24);
  sky565 = rgb565_near(bg_sky_color((float)hour));
  gfx_zoom(z);
  gfx_fill(sky565);
  /* stars */
  stars_now = bg_stars_alpha((float)hour);
  if (stars_now > 0) gfx_call(draw_stars, NULL, 0, SCREEN_H);
  /* the original's viewPos: the screen's centre */
  double vx = cam->x + (double)W2 / z, vy = cam->y + (double)H2 / z;
  /* planets and moons (BackgroundImages.update), their stage moving at 0.15 of the view */
  double ppar = DBL(15000000015LL, 100000000000LL);
  int px = (int)floor(-vx * ppar * z + W2), py = (int)floor(-vy * ppar * z + H2);
  for (int i = 0; i < nplanets; i++) {
    const BgPlanet *p = &planets[i];
    int sw = spr_w(p->spr), sh = spr_h(p->spr);
    int n = (sw + 2) / (p->width + 2);
    if (n < 3) continue;
    int fw = (sw + 2) / n - 2;
    double len = sqrt((double)p->x * p->x + (double)p->y * p->y);
    double dir = atan2(p->y, p->x) + time * DBL(175, 10000000) * p->velocity;
    int x = jsround(px + z * (len * cos(dir) - fw * 0.5)), y = jsround(py + z * (len * sin(dir) - sh * 0.5));
    gfx_sprite_ex(p->spr, (fw + 2) * (n - 2), 0, fw, sh, x, y, 0, 255, sky565);
    gfx_sprite_ex(p->spr, (fw + 2) * p->image, 0, fw, sh, x, y, 0, 102, GFX_NO_TINT);
    gfx_sprite_ex(p->spr, (fw + 2) * (n - 1), 0, fw, sh, x, y, 0, 10, GFX_NO_TINT);
  }
  /* clouds, far layer first */
  float ca = bg_clouds_alpha((float)hour);
  if (ca <= 0) return;
  for (int layer = 0; layer < 3; layer++) {
    float par = PARALLAX + PARALLAX * layer;
    int ox = (int)floor(-vx * par * z + W2), oy = (int)floor(-vy * par * z + H2);
    for (int i = 0; i < nclouds; i++) {
      const Cloud *c = &clouds[i];
      if (c->layer != layer) continue;
      /* the cloud's rect on screen */
      float sx = (float)ox + z * c->x, sy = (float)oy + z * c->y;
      if (sx + z * 60 * c->x1 < 0 || sx + z * 60 * c->x0 >= SCREEN_W || sy + z * 60 * c->y1 < 0 ||
          sy + z * 60 * c->y0 >= SCREEN_H)
        continue;
      int a = (int)(c->alpha * ca + 0.5f);
      if (a <= 0) continue;
      CloudShape s;
      make_cloud(&s, c->seed, layer);
      for (int k = 0; k < s.n; k++) {
        int t = s.t[k].t;
        int x = bg_floor(sx + z * 60.0f * s.t[k].x + 0.5f), y = bg_floor(sy + z * 60.0f * s.t[k].y + 0.5f);
        gfx_sprite_ex(SPR_CLOUDS, 62 * (t % 16), 62 * (t / 16), 60, 60, x, y, 0, (uint8_t)a, GFX_NO_TINT);
      }
    }
  }
}

/* ---------------------------------------------------------------- islands */
/* graphics.WorldImage.makeWorldSprite, kept down each column: its core (a byte: if not 0, row 0
 * is outline and rows 1 to core - 1 fill), then runs from the core down, a byte each
 * kind << 6 | (length - 1), kind 0 clear, 1 fill, 2 outline, 3 outline that ends the column;
 * 0xFF: no runs. Before the columns, for every 32 columns: the offset of the first (u16), and
 * the smallest core of those that aren't the island's two edges (u8, then u8 0). */
#define MAXH 256
#define HW (MAXH / 32)
#define RING 64

typedef struct {
  const BgWorld *owner;
  int32_t seed;
  int16_t w, h;
  uint8_t look;
  uint16_t off, size;      /* in the pool */
  uint32_t used;           /* the frame it was last needed */
} Slot;
_Static_assert(BG_SHAPE_POOL <= 65536, "shape offsets are 16-bit");
static uint8_t pool[BG_SHAPE_POOL];
static Slot slots[BG_SHAPE_SLOTS];
static uint32_t pool_top;

void bg_forget_shapes(void) {
  memset(slots, 0, sizeof slots);
  pool_top = 0;
}

typedef struct {
  uint8_t *p;
  uint32_t n, cap;
} Out;
static void out_byte(Out *o, uint32_t b) {
  if (o->n < o->cap) o->p[o->n] = (uint8_t)b;
  o->n++;
}

/* one column (F its pixels, L and R its neighbours'): its core byte, then its runs from the core
 * down; returns the core */
static int put_column(Out *o, const uint32_t *F, const uint32_t *L, const uint32_t *R, int h, bool edge) {
  uint32_t O[HW];
  int last = -1;
  for (int k = 0; k < HW; k++) {
    uint32_t up = F[k] << 1 | (k ? F[k - 1] >> 31 : 0), down = F[k] >> 1 | (k + 1 < HW ? F[k + 1] << 31 : 0);
    O[k] = edge ? F[k] : F[k] & ~(L[k] & R[k] & up & down);
    if (F[k]) last = k * 32 + 31 - __builtin_clz(F[k]);
  }
  if (last >= h) last = h - 1;
  /* the core: row 0 (outline, as the top always is) and the fill under it */
  int core = 0;
  if (F[0] & 1) {
    core = 1;
    while (core <= last && (F[core >> 5] >> (core & 31) & 1) && !(O[core >> 5] >> (core & 31) & 1)) core++;
  }
  out_byte(o, (uint32_t)core);
  int y = core;
  if (y > last) {
    out_byte(o, 0xFF);
    return core;
  }
  while (y <= last) {
    int kind = (F[y >> 5] >> (y & 31) & 1) ? ((O[y >> 5] >> (y & 31) & 1) ? 2 : 1) : 0;
    int n = 1;
    while (y + n <= last) {
      int yy = y + n;
      int k2 = (F[yy >> 5] >> (yy & 31) & 1) ? ((O[yy >> 5] >> (yy & 31) & 1) ? 2 : 1) : 0;
      if (k2 != kind) break;
      n++;
    }
    y += n;
    if (y > last) {   /* the column's last run (outline, as its lowest pixel always is) ends it */
      while (n > 63) {
        int m = n - 63 < 64 ? n - 63 : 64;
        out_byte(o, (uint32_t)(kind << 6 | (m - 1)));
        n -= m;
      }
      out_byte(o, (uint32_t)(3 << 6 | (n - 1)));
    } else {
      while (n > 0) {
        int m = n < 64 ? n : 64;
        out_byte(o, (uint32_t)(kind << 6 | (m - 1)));
        n -= m;
      }
    }
  }
  return core;
}

/* makes the world's island into o (o->n: the bytes it needs, even past o->cap) */
static void make_island(const BgWorld *w, Out *o) {
  int width = w->w, height = w->h > MAXH ? MAXH : w->h;
  SRand r;
  srand_init(&r, w->seed);
  int below = height - 10 < 0 ? 0 : height - 10 > 12 ? 12 : height - 10, wh = height - below;
  double minYMax = floor(wh - wh / 4.0), maxYMin = floor(wh - wh / 3.0), minYMin = floor(wh - wh / 2.0);
  if (w->look == BG_LOOK_ROCK_LESS_JAGGED) {
    minYMax = floor(wh - wh / 8.0);
    maxYMin = floor(wh - wh / 6.0);
    minYMin = floor(wh - wh / 4.0);
  }
  /* the outline's points: (0, 0), then deep and shallow in turn, then (width - 1, 0) */
  enum { MAXP = 100 };
  int ptx[MAXP], pty[MAXP], np = 0;
  ptx[np] = 0, pty[np++] = 0;
  double a = wh / 4.0 + srand_int(&r, -5, 6), b = width / 3.0;
  int side = (int)(a < b ? a : b);
  double d = floor((width - side * 2) / (srand_int(&r, 25, 35) + width / 40.0));
  int n = (int)(d > 1 ? d : 1);
  if (n > (MAXP - 2) / 2) n = (MAXP - 2) / 2;
  double xchange = floor((width - side * 2) / (double)n);
  int xx = side + (int)((width - side * 2 - xchange * (n - 0.5)) / 2);
  for (int i = 0; i < n; i++) {
    int x = xx + srand_int(&r, -6, 7);
    ptx[np] = x, pty[np++] = srand_int(&r, (int)minYMax, wh);
    if (i != n - 1) {
      x = xx + (int)(xchange / 2) + srand_int(&r, -6, 7);
      ptx[np] = x, pty[np++] = srand_int(&r, (int)minYMin, (int)maxYMin);
    }
    xx += (int)xchange;
  }
  ptx[np] = width - 1, pty[np++] = 0;

  double pf1 = DBL(16, 10), pf2 = DBL(12, 10), pchange = 8;
  if (w->look == BG_LOOK_KEY) pf1 = 1, pf2 = DBL(8, 10), pchange = 3;

  /* the columns' pixels, in a ring: a column is done 20 columns later, when no earth part
   * (which reaches 18 columns back) can touch it or its neighbours any more */
  uint32_t ring[RING][HW];
  memset(ring, 0, sizeof ring);
  static const uint32_t none[HW];
#define PIX(x, y) (ring[(x) & (RING - 1)][(y) >> 5] |= 1u << ((y) & 31))
  /* every 32nd column: u16 offset of its runs, u8 its chunk's core (the fill rows 1 to core - 1
   * that all its columns but the island's two edges have), u8 0 */
  int nchk = (width + 31) / 32, done = 0, core = 255;
  uint32_t base = (uint32_t)nchk * 4;
  for (uint32_t i = 0; i < base; i++) out_byte(o, 0);
#define FINISH(c)                                                                                         \
  do {                                                                                                    \
    int c_ = (c), k_ = 4 * (c_ >> 5);                                                                     \
    if (!(c_ & 31)) {                                                                                     \
      uint32_t at = o->n - base;                                                                          \
      if ((uint32_t)k_ + 1 < o->cap) o->p[k_] = (uint8_t)at, o->p[k_ + 1] = (uint8_t)(at >> 8);           \
      core = 255;                                                                                         \
    }                                                                                                     \
    int cc_ = put_column(o, ring[c_ & (RING - 1)], c_ > 0 ? ring[(c_ - 1) & (RING - 1)] : none,           \
                         c_ + 1 < width ? ring[(c_ + 1) & (RING - 1)] : none, height, c_ == 0 || c_ == width - 1); \
    if (c_ > 0 && c_ < width - 1 && cc_ < core) core = cc_;                                               \
    if (((c_ & 31) == 31 || c_ == width - 1) && (uint32_t)k_ + 2 < o->cap) o->p[k_ + 2] = (uint8_t)(core > 255 ? 255 : core); \
    if (c_ > 0) memset(ring[(c_ - 1) & (RING - 1)], 0, sizeof ring[0]);                                   \
  } while (0)

  int cur = 0;
  double add = 0;
  for (int i = 0; i < width; i++) {
    double maxAdd = pchange / (abs(pty[cur + 1] - pty[cur]) + 5);
    double v = add + srand_float(&r, -maxAdd, maxAdd);
    add = v < -DBL(2, 10) ? -DBL(2, 10) : v > DBL(2, 10) ? DBL(2, 10) : v;
    if (i > ptx[cur + 1]) {
      if (pty[cur + 1] > pty[cur]) {
        /* bits of earth floating under the point */
        int parts = srand_int(&r, 1, 3), dist = srand_int(&r, 5, 8);
        for (int op = 0; op < parts; op++) {
          int bx = srand_int(&r, -2, 3) + i - (parts - 1) * dist + dist * 2 * op;
          int by = pty[cur + 1] + srand_int(&r, 0, 3);
          int fromX = bx - 1, toX = bx;
          for (int y = by + 2; y < by + 11; y++) {
            if (y <= by + 5) {
              fromX -= srand_int(&r, 0, y < by + 5 ? 3 : 2);
              toX += srand_int(&r, 0, y < by + 5 ? 3 : 2);
            } else {
              int v1 = fromX + srand_int(&r, 0, y > by + 6 ? 3 : 2);
              fromX = toX < v1 ? toX : v1;
              int v2 = toX - srand_int(&r, 0, y > by + 6 ? 3 : 2);
              toX = fromX > v2 ? fromX : v2;
            }
            if (y == by + 2 && toX - fromX == 1) ++toX;
            if (y == by + 3 && toX - fromX == 2) --fromX;
            for (int x = fromX; x < toX; x++)
              if (x >= 0 && x < width && y + 2 < height && x >= i - 40 && x < i + 20) PIX(x, y);
          }
        }
      }
      ++cur;
    }
    int x0 = ptx[cur], y0 = pty[cur], x1 = ptx[cur + 1], y1 = pty[cur + 1];
    double t = x1 != x0 ? (double)(i - x0) / (x1 - x0) : i > x0 ? INFINITY : i < x0 ? -INFINITY : NAN;
    if (y1 < y0) t = pow(t, pf1 - t * pf2 + add);
    else t = 1 - pow(1 - t, pf1 - (1 - t) * pf2 - add);
    double v12 = floor(t * (y1 - y0) + y0), v2 = wh - 1;
    double bottom = v2 < v12 ? v2 : v12;
    if (bottom >= 0) {
      int bt = (int)bottom;
      for (int j = 0; j <= bt && j < height; j++) PIX(i, j);
    }
    if (i - 20 >= 0) FINISH(i - 20);
    done = i - 19;
  }
  for (int c = done < 0 ? 0 : done; c < width; c++) FINISH(c);
#undef PIX
#undef FINISH
}

static void compact(void) {
  /* slots by offset, slid down */
  uint32_t at = 0;
  for (;;) {
    int best = -1;
    for (int i = 0; i < BG_SHAPE_SLOTS; i++)
      if (slots[i].owner && slots[i].off >= at && (best < 0 || slots[i].off < slots[best].off)) best = i;
    if (best < 0) break;
    if (slots[best].off != at) memmove(pool + at, pool + slots[best].off, slots[best].size);
    slots[best].off = (uint16_t)at;
    at += (slots[best].size + 1) & ~1u;
  }
  pool_top = at;
}

/* the island's slot, made if needed (-1: it can't be) */
static int shape_of(BgWorld *w) {
  if (w->h <= 0 || w->w <= 0) return -1;
  if (w->shape) {
    Slot *s = &slots[w->shape - 1];
    if (s->owner == w && s->seed == w->seed && s->w == w->w && s->h == w->h && s->look == w->look) {
      s->used = frame_no;
      return w->shape - 1;
    }
  }
  w->shape = 0;
  int slot = -1;
  for (int i = 0; i < BG_SHAPE_SLOTS && slot < 0; i++)
    if (!slots[i].owner) slot = i;
  Out o = {pool + pool_top, 0, BG_SHAPE_POOL - pool_top};
  make_island(w, &o);
  if (o.n > o.cap || slot < 0) {
    /* room: let go of the islands seen longest ago (not this frame's) */
    uint32_t need = o.n;
    for (;;) {
      uint32_t live = 0;
      for (int i = 0; i < BG_SHAPE_SLOTS; i++)
        if (slots[i].owner) live += (slots[i].size + 1) & ~1u;
      if (BG_SHAPE_POOL - live >= need && slot >= 0) break;
      int old = -1;
      for (int i = 0; i < BG_SHAPE_SLOTS; i++)
        if (slots[i].owner && slots[i].used != frame_no && (old < 0 || slots[i].used < slots[old].used)) old = i;
      if (old < 0) return -1;
      slots[old].owner = NULL;
      if (slot < 0) slot = old;
    }
    compact();
    o = (Out){pool + pool_top, 0, BG_SHAPE_POOL - pool_top};
    make_island(w, &o);
    if (o.n > o.cap) return -1;
  }
  slots[slot] = (Slot){w, w->seed, w->w, w->h, w->look, (uint16_t)pool_top, (uint16_t)o.n, frame_no};
  pool_top += (o.n + 1) & ~1u;
  w->shape = (uint8_t)(slot + 1);
  return slot;
}

/* the runs of column c of a shape */
static const uint8_t *column(const Slot *s, int c) {
  const uint8_t *base = pool + s->off, *p = base + ((s->w + 31) / 32) * 4;
  p += base[4 * (c >> 5)] | base[4 * (c >> 5) + 1] << 8;
  for (int k = c & ~31; k < c; k++) {
    p++;   /* the core */
    while (*p++ < 0xC0) {}
  }
  return p;
}

typedef struct {
  uint8_t slot, z;
  int16_t x, y;            /* screen position of the rect's top-left */
  uint16_t fill, outline;
} IslandDraw;
#define MAX_ISLAND_DRAWS 48
static IslandDraw island_draws[MAX_ISLAND_DRAWS];   /* a ring: a frame draws fewer than this */
static unsigned nisland_draws;

/* rows y0..y1 - 1 of an island column at screen x0..x1 - 1, in one colour */
static inline void island_rows(const GfxBand *b, const IslandDraw *d, int x0, int x1, int y0, int y1, uint16_t col) {
  int sy0 = d->y + y0 * d->z, sy1 = d->y + y1 * d->z;
  if (sy0 < b->cy0) sy0 = b->cy0;
  if (sy1 > b->cy1) sy1 = b->cy1;
  uint16_t *q = b->px + (sy0 - b->y0) * SCREEN_W + x0;
  if (x1 - x0 == 1)
    for (int sy = sy0; sy < sy1; sy++, q += SCREEN_W) q[0] = col;
  else if (x1 - x0 == 2)
    for (int sy = sy0; sy < sy1; sy++, q += SCREEN_W) q[0] = q[1] = col;
}

static void draw_island(const GfxBand *b, void *ctx) {
  const IslandDraw *d = ctx;
  const Slot *s = &slots[d->slot];
  if (!s->owner) return;
  int z = d->z, w = s->w;
  int c0 = floordiv(b->cx0 - d->x, z), c1 = floordiv(b->cx1 - d->x + z - 1, z);
  int r0 = floordiv(b->cy0 - d->y, z), r1 = floordiv(b->cy1 - d->y + z - 1, z);
  if (c0 < 0) c0 = 0;
  if (c1 > w) c1 = w;
  if (r0 < 0) r0 = 0;
  if (c0 >= c1 || r1 <= 0) return;
  const uint8_t *base = pool + s->off, *cols = base + 4 * ((w + 31) / 32);
  for (int k = c0 >> 5; k <= (c1 - 1) >> 5; k++) {
    const uint8_t *chk = base + 4 * k;
    int ka = k * 32 > c0 ? k * 32 : c0, kb = k * 32 + 32 < c1 ? k * 32 + 32 : c1, core = chk[2];
    /* the chunk's core (but for the island's edges), as rows: the outline on top, the fill under it */
    int fa = ka > 0 ? ka : 1, fb = kb < w - 1 ? kb : w - 1;
    if (core && fa < fb) {
      int sx0 = d->x + fa * z, sx1 = d->x + fb * z;
      for (int ry = r0; ry < r1 && ry < core; ry++) {
        int sy0 = d->y + ry * z, sy1 = sy0 + z;
        if (sy0 < b->cy0) sy0 = b->cy0;
        if (sy1 > b->cy1) sy1 = b->cy1;
        for (int sy = sy0; sy < sy1; sy++) gfx_band_span(b, sy, sx0, sx1, ry ? d->fill : d->outline, 255);
      }
    }
    bool inside = r1 <= core;   /* the band shows only the core here */
    const uint8_t *p = cols + (chk[0] | chk[1] << 8);
    for (int c = k * 32; c < kb; c++) {
      bool edge = c == 0 || c == w - 1;
      int cc = *p++;   /* the column's core */
      if (c < ka || (inside && !edge)) {
        while (*p++ < 0xC0) {}
        continue;
      }
      int x0 = d->x + c * z, x1 = x0 + z;
      if (x0 < b->cx0) x0 = b->cx0;
      if (x1 > b->cx1) x1 = b->cx1;
      if (cc) {   /* the part of its core the chunk's doesn't draw */
        int from = edge || !core ? 0 : core;
        if (from == 0 && r0 == 0) island_rows(b, d, x0, x1, 0, 1, d->outline);
        if (from < 1) from = 1;
        if (from < r0) from = r0;
        if (from < cc && from < r1) island_rows(b, d, x0, x1, from, cc < r1 ? cc : r1, d->fill);
      }
      int y = cc;
      for (;;) {
        uint8_t v = *p++;
        if (v == 0xFF) break;
        if (y >= r1) {   /* the rest is below the band */
          if (v < 0xC0)
            while (*p++ < 0xC0) {}
          break;
        }
        int kind = v >> 6, n = (v & 63) + 1;
        if (kind && y + n > r0) island_rows(b, d, x0, x1, y > r0 ? y : r0, y + n < r1 ? y + n : r1,
                                            kind == 1 ? d->fill : d->outline);
        y += n;
        if (kind == 3) break;
      }
    }
  }
}

/* decorations */
static const struct { const char *name; uint16_t spr; } decos[DECO_KINDS] = {
  {"", 0}, {"spr_grass", SPR_GRASS}, {"spr_flowers", SPR_FLOWERS}, {"spr_flowers_blue", SPR_FLOWERS_BLUE},
  {"spr_decoration_rocks", SPR_DECORATION_ROCKS}, {"spr_decoration_keycity", SPR_DECORATION_KEYCITY},
  {"spr_flowers_keycity", SPR_FLOWERS_KEYCITY}, {"spr_purplegrass", SPR_PURPLEGRASS}, {"spr_redgrass", SPR_REDGRASS}};

int bg_deco_kind(const char *name) {
  for (int k = 1; k < DECO_KINDS; k++)
    if (name && !strcmp(name, decos[k].name)) return k;
  return 0;
}
uint16_t bg_deco_sprite(int kind) { return kind > 0 && kind < DECO_KINDS ? decos[kind].spr : 0; }
int bg_deco_frames(int kind) { return kind > 0 && kind < DECO_KINDS ? (spr_w(decos[kind].spr) + 2) / 22 : 0; }

void bg_set_decoration(uint8_t *d, int col, int kind, int frame, bool has_ground_permanent) {
  d[col] = 0;
  if (has_ground_permanent || kind <= 0 || kind >= DECO_KINDS) return;
  int n = bg_deco_frames(kind);
  if (frame < 0) frame = rnd_int(n);
  if (frame >= n || frame > 15) frame = 0;
  d[col] = (uint8_t)(kind << 4 | frame);
}

void bg_world_decorate(const BgWorld *w, uint8_t *d) {
  if (w->h == 0 || w->look == BG_LOOK_SNOW) return;
  for (int c = 0; c < (w->w + 19) / 20; c++) bg_set_decoration(d, c, DECO_GRASS, -1, false);
}

int bg_look(const char *a) {
  if (!a) return BG_LOOK_NORMAL;
  if (!strcmp(a, "key")) return BG_LOOK_KEY;
  if (!strcmp(a, "key2")) return BG_LOOK_KEY2;
  if (!strcmp(a, "rock")) return BG_LOOK_ROCK;
  if (!strcmp(a, "rock:lessJagged")) return BG_LOOK_ROCK_LESS_JAGGED;
  if (!strcmp(a, "snow")) return BG_LOOK_SNOW;
  return BG_LOOK_NORMAL;
}

void bg_draw_world(BgWorld *w, const uint8_t *d, const BgCamera *cam) {
  int z = cam->zoom == 2 ? 2 : 1;
  gfx_zoom(z);
  int ox = bg_floor(-cam->x * z), oy = bg_floor(-cam->y * z);
  int x = ox + w->x * z, y = oy + w->y * z;
  if (w->h > 0 && x < SCREEN_W && x + w->w * z > 0 && y < SCREEN_H && y + w->h * z > 0) {
    int slot = shape_of(w);
    if (slot >= 0) {
      uint32_t fill = 5259312, outline = 2103312;
      if (w->look == BG_LOOK_KEY || w->look == BG_LOOK_KEY2) fill = 5195330, outline = 1973275;
      if (w->look == BG_LOOK_ROCK || w->look == BG_LOOK_ROCK_LESS_JAGGED) fill = 5197647, outline = 1973790;
      IslandDraw *id = &island_draws[nisland_draws++ % MAX_ISLAND_DRAWS];
      *id = (IslandDraw){(uint8_t)slot, (uint8_t)z, (int16_t)x, (int16_t)y, rgb565_near(fill), rgb565_near(outline)};
      gfx_call(draw_island, id, y, w->h * z);
    }
  }
  /* the decorations, 20 pixels above the ground */
  if (!d) return;
  for (int c = 0; c < (w->w + 19) / 20; c++) {
    if (!d[c]) continue;
    int kind = d[c] >> 4, f = d[c] & 15;
    if (kind >= DECO_KINDS) continue;
    gfx_sprite_ex(decos[kind].spr, 22 * f, 0, 20, 20, x + 20 * c * z, y - 20 * z, 0, 255, GFX_NO_TINT);
  }
}

bool bg_world_mask(BgWorld *w, int cx, int cy) {
  if (cx < 0 || cy < 0 || cx * 20 >= w->w || cy * 20 >= w->h) return false;
  int slot = shape_of(w);
  if (slot < 0) return false;
  const Slot *s = &slots[slot];
  const uint8_t *p = column(s, cx * 20);
  int r0 = cy * 20, r1 = r0 + 20;
  bool hit = false;
  for (int c = cx * 20; c < cx * 20 + 20 && c < s->w && !hit; c++) {
    int y = *p++;
    if (y && r0 < y) hit = true;   /* its core: rows 0 to core - 1 */
    for (;;) {
      uint8_t v = *p++;
      if (v == 0xFF) break;
      int kind = v >> 6, n = (v & 63) + 1;
      if (kind && y < r1 && y + n > r0) hit = true;
      y += n;
      if (kind == 3) break;
    }
  }
  return hit;
}
