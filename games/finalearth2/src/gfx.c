/* The renderer: a draw list rasterised in bands of GFX_BAND rows.
 *
 * Sprites are drawn straight from data.bin (tools/pack.py says how they are
 * coded): each band walks the rows of the commands that cross it. Alpha has 33
 * levels (the colour's own alpha times the command's), blended in RGB565. */
#include "gfx.h"
#include <string.h>
#include "data.h"

enum { K_RECT, K_SPRITE, K_TEXT, K_CALL };
typedef struct {
  int16_t x, y;            /* top-left on screen */
  int16_t sx, sy, w, h;    /* sprite: its part; rect: w, h; text: font, top, width, height */
  uint16_t ref;            /* sprite, text (offset in the pool), band function (slot) */
  uint16_t color;          /* rect and text colour, sprite tint */
  uint8_t kind;            /* K_* | zoom << 4 */
  uint8_t flags, alpha, clip;
} Item;

#define MAX_CLIPS 48
#define MAX_CALLS 64
typedef struct { int16_t x0, y0, x1, y1; } Clip;

#ifdef HOST
static uint16_t host_band[SCREEN_W * GFX_BAND];
static uint16_t *band = host_band;
#else
static uint16_t *band;
#endif
void gfx_set_band_memory(uint16_t *px) {
#ifdef HOST
  band = px ? px : host_band;
#else
  band = px;
#endif
}
static Item items[GFX_MAX_ITEMS];
static char text_pool[GFX_TEXT_POOL];
static Clip clips[MAX_CLIPS];
static struct { GfxBandFn fn; void *ctx; } calls[MAX_CALLS];
static int nitems, ntext, nclips, ncalls, zoom = 1, dropped, last_items, last_dropped;
static bool clip_used;     /* a command uses the last clip */
static bool clips_full;    /* no clip left: commands are dropped until the next frame */

/* ---------------------------------------------------------------- data.bin */
static inline uint32_t rd16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static inline uint32_t rd32(const uint8_t *p) { return rd16(p) | rd16(p + 2) << 16; }
static inline uint32_t nsprites(void) { return rd16(fe_data + 4); }
static inline const uint8_t *spr_rec(uint16_t spr) { return fe_data + rd32(fe_data + 8) + 8u * spr; }
int spr_w(uint16_t spr) { return spr < nsprites() ? (int)rd16(spr_rec(spr) + 4) : 0; }
int spr_h(uint16_t spr) { return spr < nsprites() ? (int)rd16(spr_rec(spr) + 6) : 0; }

typedef struct {
  const uint8_t *d;        /* its data */
  const uint16_t *pal;     /* RGB565 of each colour */
  const uint8_t *alpha;    /* alpha (0..255) of each colour */
  const uint8_t *slabs;    /* per slab: u16 x0, u16 x1, u32 offset of its rows */
  uint8_t mode;            /* 0 4-bit, 1 8-bit, 2 direct */
  bool big;                /* u32 row offsets */
  int ncol, nslab, w, h;
} Spr;

static void spr_open(uint16_t spr, Spr *s) {
  const uint8_t *r = spr_rec(spr);
  const uint8_t *d = fe_data + rd32(r);
  s->d = d;
  s->w = (int)rd16(r + 4);
  s->h = (int)rd16(r + 6);
  s->ncol = d[0];
  s->mode = d[1] & 3;
  s->big = d[1] & 4;
  s->nslab = d[2];
  s->pal = (const uint16_t *)(const void *)(d + 4);
  s->alpha = d + 4 + 2 * s->ncol;
  s->slabs = d + ((4 + 3 * (uint32_t)s->ncol + 3) & ~3u);
}

/* row r of slab k: its tokens, from *p to *end */
static inline void slab_row(const Spr *s, int k, int r, const uint8_t **p, const uint8_t **end) {
  const uint8_t *sl = s->d + rd32(s->slabs + 8 * k + 4);
  uint32_t a, b;
  if (s->big) {
    a = ((const uint32_t *)(const void *)sl)[r];
    b = ((const uint32_t *)(const void *)sl)[r + 1];
    sl += 4 * (s->h + 1);
  } else {
    a = ((const uint16_t *)(const void *)sl)[r];
    b = ((const uint16_t *)(const void *)sl)[r + 1];
    sl += 2 * (s->h + 1);
  }
  *p = sl + a;
  *end = sl + b;
}

/* ---------------------------------------------------------------- pixels */
typedef uint32_t __attribute__((may_alias)) u32a;

/* a colour times a tint, channel by channel (PIXI's tint) */
static inline uint16_t tinted(uint16_t c, uint16_t t) {
  uint32_t r = ((c >> 11) * (t >> 11) + 15) / 31, g = ((c >> 5 & 63) * (t >> 5 & 63) + 31) / 63,
           b = ((c & 31) * (t & 31) + 15) / 31;
  return (uint16_t)(r << 11 | g << 5 | b);
}

/* a colour's alpha (0..255) times the command's (ia: its alpha times 129) as 0..32 */
static inline uint32_t mix_alpha(uint32_t a8, uint32_t ia) { return (a8 * ia + 131072) >> 18; }

/* screen pixels [d0, d1) of a row in one colour */
static inline void span(uint16_t *row, int d0, int d1, uint16_t c, uint32_t a5) {
  uint16_t *p = row + d0, *e = row + d1;
  if (a5 >= 32) {
    if (((uintptr_t)p & 2) && p < e) *p++ = c;
    uint32_t two = c | (uint32_t)c << 16;
    u32a *w = (u32a *)(void *)p;
    for (; p + 8 <= e; p += 8, w += 4) w[0] = w[1] = w[2] = w[3] = two;
    for (; p + 2 <= e; p += 2) *w++ = two;
    if (p < e) *p = c;
  } else if (a5) {
    uint32_t f = ((c | (uint32_t)c << 16) & 0x07E0F81Fu) * a5 + 0x02008010u, inv = 32 - a5;
    for (; p < e; p++) {
      uint32_t g = (*p | (uint32_t)*p << 16) & 0x07E0F81Fu;
      uint32_t r = ((f + g * inv) >> 5) & 0x07E0F81Fu;
      *p = (uint16_t)(r | r >> 16);
    }
  }
}

/* how the rows of a sprite's part are drawn */
typedef struct {
  uint16_t *row;           /* the band row (indexed by screen x) */
  int lo, hi;              /* the source columns that show */
  int base, step;          /* screen x of source column p's left: base + p * step (step: the zoom, negative flipped) */
  int z, cx0, cx1;
  uint32_t ia;             /* the command's alpha times 129 */
  bool solid;              /* the command's alpha is 255 */
  bool plain;              /* zoom 1, not flipped, opaque, not tinted: the fast way */
  uint16_t tint;
  const uint16_t *pal;
  const uint8_t *alpha;
  uint16_t col[16];        /* 4-bit sprites: each colour, tinted */
  uint8_t a5[16];          /* and its alpha with the command's, 0..32 */
} Blit;

static inline void put_px(const Blit *c, uint16_t *d, uint16_t col, uint32_t a5) {
  if (c->z == 1) {
    *d = a5 >= 32 ? col : gfx_blend(col, *d, a5);
    return;
  }
  for (int k = 0; k < 2; k++) {   /* zoom 2: the pair can be half outside the clip */
    int x = (int)(d + k - c->row);
    if (x >= c->cx0 && x < c->cx1) d[k] = a5 >= 32 ? col : gfx_blend(col, d[k], a5);
  }
}

/* source columns [a, b) in one colour */
static inline void put_fill(const Blit *c, int a, int b, uint16_t col, uint32_t a5) {
  if (a < c->lo) a = c->lo;
  if (b > c->hi) b = c->hi;
  if (a >= b || !a5) return;
  int d0, d1;
  if (c->step > 0) {
    d0 = c->base + a * c->step;
    d1 = c->base + b * c->step;
  } else {
    d0 = c->base + (b - 1) * c->step;
    d1 = c->base + a * c->step + c->z;
  }
  if (d0 < c->cx0) d0 = c->cx0;
  if (d1 > c->cx1) d1 = c->cx1;
  if (d0 < d1) span(c->row, d0, d1, col, a5);
}

/* the length of a token (4-bit sprites) */
static inline int tok4(const uint8_t *p, int *size) {
  uint32_t t = *p;
  if (t < 0x40) return *size = 1, (int)t + 1;
  if (t < 0x80) return *size = 1 + (int)((t & 63) + 2) / 2, (int)(t & 63) + 1;
  if (t < 0xF0) return *size = 1, (int)(t >> 4) - 7;
  return *size = 2, (int)p[1] + 8;
}

/* one pixel of a plain command (zoom 1, not flipped, opaque, not tinted) */
static inline void plain_px(uint16_t *d, uint16_t col, uint32_t a8) {
  if (a8 == 255) *d = col;
  else if (a8) *d = gfx_blend(col, *d, mix_alpha(a8, 255 * 129));
}

/* rows of a plain command (zoom 1, not flipped, opaque, not tinted), 4-bit, u16 row offsets:
 * rows r to r + nrows - 1 of a slab starting at column x0; row[x] is source column x on the
 * first screen row */
__attribute__((noinline)) static void plain4(uint16_t *row, const uint16_t *pal, const uint8_t *al,
                                             const uint16_t *offs, const uint8_t *data, int r, int nrows, int x0,
                                             int lo, int hi) {
  for (; nrows--; r++, row += SCREEN_W) {
    const uint8_t *p = data + offs[r], *end = data + offs[r + 1];
    int size, n, x = x0;
    while (p < end && x + (n = tok4(p, &size)) <= lo) x += n, p += size;   /* left of what shows */
    while (p < end && x < hi) {
      uint32_t t = *p++;
      if (t < 0x40) {
        x += (int)t + 1;
      } else if (t < 0x80) {
        n = (int)(t & 63) + 1;
        const uint8_t *q = p;
        p += (n + 1) >> 1;
        int k = x < lo ? lo - x : 0, k1 = x + n > hi ? hi - x : n;
        uint16_t *d = row + x + k;
        if (k & 1) {
          uint32_t i = q[k >> 1] >> 4;
          plain_px(d++, pal[i], al[i]);
          k++;
        }
        for (q += k >> 1; k + 1 < k1; k += 2, d += 2) {
          uint32_t v = *q++, i0 = v & 15, i1 = v >> 4, a0 = al[i0], a1 = al[i1];
          if (a0 == 255) d[0] = pal[i0];
          else plain_px(d, pal[i0], a0);
          if (a1 == 255) d[1] = pal[i1];
          else plain_px(d + 1, pal[i1], a1);
        }
        if (k < k1) plain_px(d, pal[*q & 15], al[*q & 15]);
        x += n;
      } else {
        uint32_t i = t & 15, a8 = al[i];
        n = t < 0xF0 ? (int)(t >> 4) - 7 : *p++ + 8;
        int a = x > lo ? x : lo, b = x + n < hi ? x + n : hi;
        if (a8 == 255 && b - a <= 8) {
          uint16_t col = pal[i], *d = row + a, *e = row + b;
          while (d < e) *d++ = col;
        } else {
          span(row, a, b, pal[i], a8 == 255 ? 32 : mix_alpha(a8, 255 * 129));
        }
        x += n;
      }
    }
  }
}

static void row4(const Blit *c, const uint8_t *p, const uint8_t *end, int x) {
  int size, n;
  while (p < end && x + (n = tok4(p, &size)) <= c->lo) x += n, p += size;
  while (p < end && x < c->hi) {
    uint32_t t = *p;
    n = tok4(p, &size);
    if (t >= 0x80) {
      uint32_t i = t & 15;
      put_fill(c, x, x + n, c->col[i], c->a5[i]);
    } else if (t >= 0x40) {
      const uint8_t *lit = p + 1;
      int a = x > c->lo ? x : c->lo, b = x + n < c->hi ? x + n : c->hi;
      uint16_t *d = c->row + c->base + a * c->step;
      for (int k = a - x; k < b - x; k++, d += c->step) {
        uint32_t i = lit[k >> 1] >> ((k & 1) << 2) & 15;
        if (c->a5[i]) put_px(c, d, c->col[i], c->a5[i]);
      }
    }
    x += n;
    p += size;
  }
}

static inline int tok8(const uint8_t *p, int *size) {
  uint32_t t = *p;
  if (t < 0x40) return *size = 1, (int)t + 1;
  if (t < 0x80) return *size = 1 + (int)(t & 63) + 1, (int)(t & 63) + 1;
  return *size = 2, (int)(t & 127) + 1;
}

static void row8(const Blit *c, const uint8_t *p, const uint8_t *end, int x) {
  int size, n;
  while (p < end && x + (n = tok8(p, &size)) <= c->lo) x += n, p += size;
  while (p < end && x < c->hi) {
    uint32_t t = *p;
    n = tok8(p, &size);
    if (t >= 0x80) {
      uint32_t i = p[1], a8 = c->alpha[i];
      uint16_t col = c->tint != GFX_NO_TINT ? tinted(c->pal[i], c->tint) : c->pal[i];
      put_fill(c, x, x + n, col, c->solid && a8 == 255 ? 32 : mix_alpha(a8, c->ia));
    } else if (t >= 0x40) {
      const uint8_t *lit = p + 1;
      int a = x > c->lo ? x : c->lo, b = x + n < c->hi ? x + n : c->hi;
      uint16_t *d = c->row + c->base + a * c->step;
      if (c->plain) {
        for (int k = a - x; k < b - x; k++, d++) plain_px(d, c->pal[lit[k]], c->alpha[lit[k]]);
      } else {
        for (int k = a - x; k < b - x; k++, d += c->step) {
          uint32_t i = lit[k], a8 = c->alpha[i], a5 = c->solid && a8 == 255 ? 32 : mix_alpha(a8, c->ia);
          if (!a5) continue;
          put_px(c, d, c->tint != GFX_NO_TINT ? tinted(c->pal[i], c->tint) : c->pal[i], a5);
        }
      }
    }
    x += n;
    p += size;
  }
}

/* direct colours (the colour picker's circle) */
static void row16(const Blit *c, const uint8_t *p, const uint8_t *end, int x) {
  while (p < end && x < c->hi) {
    uint32_t t = *p++;
    int n = (int)(t & 63) + 1;
    if (t < 0x40) {
      x += n;
    } else if (t < 0x80 || t >= 0xC0) {
      int step = t < 0x80 ? 2 : 3, a = x > c->lo ? x : c->lo, b = x + n < c->hi ? x + n : c->hi;
      uint16_t *d = c->row + c->base + a * c->step;
      for (int k = a - x; k < b - x; k++, d += c->step) {
        uint32_t a8 = step == 2 ? 255 : p[step * k + 2], a5 = c->solid && a8 == 255 ? 32 : mix_alpha(a8, c->ia);
        uint16_t col = (uint16_t)rd16(p + step * k);
        if (a5) put_px(c, d, c->tint != GFX_NO_TINT ? tinted(col, c->tint) : col, a5);
      }
      p += step * n;
      x += n;
    } else {
      uint16_t col = (uint16_t)rd16(p);
      put_fill(c, x, x + n, c->tint != GFX_NO_TINT ? tinted(col, c->tint) : col, c->solid ? 32 : mix_alpha(255, c->ia));
      p += 2;
      x += n;
    }
  }
}

/* a sprite's part drawn into rows y0..y1 - 1 of the band (screen rows by0..), clipped to cx0..cx1 */
static void draw_sprite(uint16_t *buf, int by0, int y0, int y1, int cx0, int cx1, uint16_t spr, int sx, int sy,
                        int w, int h, int x, int y, int z, uint8_t flags, uint8_t alpha, uint16_t tint) {
  if (spr >= nsprites() || !alpha) return;
  Spr s;
  spr_open(spr, &s);
  bool flip = flags & GFX_FLIPX;
  /* the part, inside the sprite */
  if (sx < 0) { x += flip ? 0 : -sx * z; w += sx; sx = 0; }
  if (sy < 0) { y -= sy * z; h += sy; sy = 0; }
  if (sx + w > s.w) { if (flip) x += (sx + w - s.w) * z; w = s.w - sx; }
  if (sy + h > s.h) h = s.h - sy;
  if (w <= 0 || h <= 0) return;
  if (y0 < y) y0 = y;
  if (y1 > y + h * z) y1 = y + h * z;
  if (cx0 < x) cx0 = x;
  if (cx1 > x + w * z) cx1 = x + w * z;
  if (y0 >= y1 || cx0 >= cx1) return;
  Blit c;
  c.z = z;
  c.cx0 = cx0;
  c.cx1 = cx1;
  c.ia = alpha * 129u;
  c.solid = alpha == 255;
  c.tint = tint;
  c.pal = s.pal;
  c.alpha = s.alpha;
  int q0 = (cx0 - x) / z, q1 = (cx1 - x + z - 1) / z;   /* the part's columns that show, from its left on screen */
  if (flip) {
    c.lo = sx + w - q1;
    c.hi = sx + w - q0;
    c.base = x + (sx + w - 1) * z;
    c.step = -z;
  } else {
    c.lo = sx + q0;
    c.hi = sx + q1;
    c.base = x - sx * z;
    c.step = z;
  }
  c.plain = z == 1 && !flip && c.solid && tint == GFX_NO_TINT;
  if (s.mode == 0 && !c.plain) {
    for (int i = 0; i < s.ncol; i++) {
      uint32_t a8 = s.alpha[i];
      c.a5[i] = (uint8_t)(c.solid && a8 == 255 ? 32 : mix_alpha(a8, c.ia));
      c.col[i] = tint != GFX_NO_TINT ? tinted(s.pal[i], tint) : s.pal[i];
    }
  }
  /* the slabs that show */
  int k0 = 0, k1 = s.nslab;
  for (int hi = k1; k0 < hi;) {   /* the first that ends right of lo */
    int m = (k0 + hi) >> 1;
    if ((int)rd16(s.slabs + 8 * m + 2) <= c.lo) k0 = m + 1;
    else hi = m;
  }
  while (k1 > k0 && (int)rd16(s.slabs + 8 * (k1 - 1)) >= c.hi) k1--;
  if (c.plain && s.mode == 0 && !s.big) {
    for (int k = k0; k < k1; k++) {
      const uint8_t *sl = s.d + rd32(s.slabs + 8 * k + 4);
      plain4(buf + (y0 - by0) * SCREEN_W + c.base, c.pal, c.alpha, (const uint16_t *)(const void *)sl,
             sl + 2 * (s.h + 1), sy + (y0 - y), y1 - y0, (int)rd16(s.slabs + 8 * k), c.lo, c.hi);
    }
    return;
  }
  for (int yy = y0; yy < y1; yy++) {
    c.row = buf + (yy - by0) * SCREEN_W;
    int r = sy + (yy - y) / z;
    for (int k = k0; k < k1; k++) {
      const uint8_t *p, *end;
      slab_row(&s, k, r, &p, &end);
      int x0 = (int)rd16(s.slabs + 8 * k);
      if (s.mode == 0) row4(&c, p, end, x0);
      else if (s.mode == 1) row8(&c, p, end, x0);
      else row16(&c, p, end, x0);
    }
  }
}

/* ---------------------------------------------------------------- fonts */
typedef struct {
  uint8_t line_h, nglyphs;
  uint16_t nkern;
  const uint8_t *glyphs, *kern, *bits;
} Font;

static bool font_open(uint8_t f, Font *ft) {
  if (f >= rd16(fe_data + 6)) return false;
  const uint8_t *d = fe_data + rd32(fe_data + rd32(fe_data + 12) + 4u * f);
  ft->line_h = d[0];
  ft->nglyphs = d[1];
  ft->nkern = (uint16_t)rd16(d + 2);
  ft->glyphs = d + 8;
  ft->kern = ft->glyphs + 8u * ft->nglyphs;
  ft->bits = d + rd32(d + 4);
  return true;
}

/* the next character of a UTF-8 string as a glyph index (-1: not in the fonts, -2: a new line) */
static int next_glyph(const char **sp) {
  const uint8_t *s = (const uint8_t *)*sp;
  uint32_t c = *s++;
  if (c >= 0xC0) {
    int more = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : 1;
    c &= 0x3F >> more;
    while (more-- && (*s & 0xC0) == 0x80) c = c << 6 | (*s++ & 0x3F);
  }
  *sp = (const char *)s;
  if (c == '\n') return -2;
  if (c >= 32 && c < 127) return (int)c - 32;
  switch (c) {
  case 0xDF: return 95;    /* the order of pack.py's EXTRA_CHARS */
  case 0xE9: return 96;
  case 0xEB: return 97;
  case 0xFC: return 98;
  }
  return -1;
}

static int kerning(const Font *f, int prev, int g) {
  int lo = 0, hi = f->nkern - 1;
  while (lo <= hi) {   /* pairs sorted by second, then first */
    int m = (lo + hi) >> 1;
    const uint8_t *k = f->kern + 4 * m;
    int d = k[1] != g ? k[1] - g : k[0] - prev;
    if (!d) return (int8_t)k[2];
    if (d < 0) lo = m + 1;
    else hi = m - 1;
  }
  return 0;
}

/* lays out a string; draw: also draws its glyphs that cross band rows y0..y1 - 1 */
typedef struct {
  uint16_t *buf;
  int by0, y0, y1, cx0, cx1;
  uint16_t color;
  uint32_t a5;
  bool rot;                /* turned a quarter clockwise: text x goes down the screen, text y left */
} TextDraw;

/* one pixel of a glyph, at text position (tx, ty) from the origin (x, y) */
static inline void text_px(const TextDraw *td, int x, int y, int tx, int ty) {
  int sx = td->rot ? x - ty : x + tx, sy = td->rot ? y + tx : y + ty;
  if (sy < td->y0 || sy >= td->y1 || sx < td->cx0 || sx >= td->cx1) return;
  uint16_t *d = td->buf + (sy - td->by0) * SCREEN_W + sx;
  *d = td->a5 >= 32 ? td->color : gfx_blend(td->color, *d, td->a5);
}

/* end: where the string stops (NULL: at its 0) */
static int text_layout(const Font *f, int x, int y, int z, const char *s, const char *end, const TextDraw *td, int *top,
                       int *bottom) {
  int pen = 0, line = 0, widest = 0, prev = -1, t = 1 << 30, b = -(1 << 30);
  while (*s && (!end || s < end)) {
    int g = next_glyph(&s);
    if (g == -2) {
      if (pen > widest) widest = pen;
      pen = 0;
      line++;
      prev = -1;
      continue;
    }
    if (g < 0 || g >= f->nglyphs) continue;
    const uint8_t *gl = f->glyphs + 8 * g;
    if (prev >= 0 && f->nkern) pen += kerning(f, prev, g);
    int gx = pen + (int8_t)gl[2], gy = line * f->line_h + (int8_t)gl[3], gw = gl[4], gh = gl[5];
    if (gw && gh) {
      if (gy < t) t = gy;
      if (gy + gh > b) b = gy + gh;
    }
    if (td && td->rot && gw && gh) {
      int first = (int)rd16(gl);
      for (int r = 0; r < gh; r++)
        for (int k = 0, bit = first + r * gw; k < gw; k++, bit++)
          if (f->bits[bit >> 3] >> (bit & 7) & 1) text_px(td, x, y, gx + k, gy + r);
    } else if (td && gw && gh) {
      int sy0 = y + gy * z, first = (int)rd16(gl);
      int r0 = td->y0 > sy0 ? (td->y0 - sy0) / z : 0, r1 = (td->y1 - sy0 + z - 1) / z;
      if (r1 > gh) r1 = gh;
      for (int r = r0; r < r1; r++) {
        int bit = first + r * gw;
        for (int k = 0; k < gw; k++, bit++) {
          if (!(f->bits[bit >> 3] >> (bit & 7) & 1)) continue;
          for (int dy = 0; dy < z; dy++) {
            int sy = sy0 + r * z + dy;
            if (sy < td->y0 || sy >= td->y1) continue;
            uint16_t *row = td->buf + (sy - td->by0) * SCREEN_W;
            for (int dx = 0; dx < z; dx++) {
              int sx = x + (gx + k) * z + dx;
              if (sx < td->cx0 || sx >= td->cx1) continue;
              row[sx] = td->a5 >= 32 ? td->color : gfx_blend(td->color, row[sx], td->a5);
            }
          }
        }
      }
    }
    pen += gl[6];
    prev = g;
  }
  if (pen > widest) widest = pen;
  if (top) *top = t > b ? 0 : t;
  if (bottom) *bottom = t > b ? 0 : b;
  return widest;
}

int gfx_font_height(uint8_t font) {
  Font f;
  return font_open(font, &f) ? f.line_h : 0;
}

int gfx_text_width(uint8_t font, const char *s) {
  Font f;
  return font_open(font, &f) && s ? text_layout(&f, 0, 0, 1, s, NULL, NULL, NULL, NULL) : 0;
}

int gfx_text_width_n(uint8_t font, const char *s, int n) {
  Font f;
  return font_open(font, &f) && s ? text_layout(&f, 0, 0, 1, s, s + n, NULL, NULL, NULL) : 0;
}

/* PIXI's BitmapText wrapping (maxWidth): a line breaks at its last white space once the pen passes
 * max_w; a word longer than max_w stays whole */
int gfx_text_wrap(uint8_t font, const char *s, int max_w, int *line_w, const char **next) {
  Font f;
  const char *p = s;
  int pen = 0, prev = -1, brk = -1, brk_w = 0;
  if (!font_open(font, &f)) {
    *line_w = 0;
    *next = s + strlen(s);
    return (int)(*next - s);
  }
  while (*p) {
    const char *c = p;
    if (*c == ' ' || *c == '\t') brk = (int)(c - s), brk_w = pen;
    int g = next_glyph(&p);
    if (g == -2) {
      *line_w = pen;
      *next = p;
      return (int)(c - s);
    }
    if (g < 0 || g >= f.nglyphs) continue;
    if (prev >= 0 && f.nkern) pen += kerning(&f, prev, g);
    pen += f.glyphs[8 * g + 6];
    prev = g;
    if (brk >= 0 && max_w > 0 && pen > max_w) {
      *line_w = brk_w;
      *next = s + brk + 1;
      return brk;
    }
  }
  *line_w = p > s && (p[-1] == ' ' || p[-1] == '\t') ? brk_w : pen;
  *next = p;
  return (int)(p - s);
}

void gfx_band_text(const GfxBand *b, uint8_t font, int x, int y, const char *s, int n, uint16_t color, uint8_t alpha,
                   bool rot) {
  Font f;
  if (!s || !alpha || b->cy0 >= b->cy1 || !font_open(font, &f)) return;
  TextDraw td = {b->px, b->y0, b->cy0, b->cy1, b->cx0, b->cx1, color, gfx_alpha32(alpha), rot};
  text_layout(&f, x, y, rot ? 1 : b->zoom, s, s + n, &td, NULL, NULL);
}

/* ---------------------------------------------------------------- the list */
static Item *add(uint8_t kind) {
  if (nitems >= GFX_MAX_ITEMS || clips_full) {
    dropped++;
    return NULL;
  }
  Item *it = &items[nitems++];
  memset(it, 0, sizeof *it);
  it->kind = (uint8_t)(kind | zoom << 4);
  it->clip = (uint8_t)(nclips - 1);
  it->alpha = 255;
  it->color = GFX_NO_TINT;
  clip_used = true;
  return it;
}

void gfx_begin(void) {
  last_items = nitems;
  last_dropped = dropped;
  nitems = ntext = ncalls = dropped = 0;
  zoom = 1;
  nclips = 1;
  clips[0] = (Clip){0, 0, SCREEN_W, SCREEN_H};
  clip_used = clips_full = false;
}

int gfx_items(void) { return last_items; }
int gfx_dropped(void) { return last_dropped; }

void gfx_zoom(int z) { zoom = z == 2 ? 2 : 1; }

void gfx_clip(int x, int y, int w, int h) {
  int x1 = x + w, y1 = y + h;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x1 > SCREEN_W) x1 = SCREEN_W;
  if (y1 > SCREEN_H) y1 = SCREEN_H;
  if (x1 < x) x1 = x;
  if (y1 < y) y1 = y;
  Clip c = {(int16_t)x, (int16_t)y, (int16_t)x1, (int16_t)y1};
  if (!clip_used) {
    clips[nclips - 1] = c;    /* no command uses the last one yet */
    return;
  }
  if (nclips >= MAX_CLIPS) {
    clips_full = true;
    return;
  }
  clips[nclips++] = c;
  clip_used = false;
}

void gfx_fill(uint16_t color) {
  const Clip *c = &clips[nclips - 1];
  gfx_rect(c->x0, c->y0, c->x1 - c->x0, c->y1 - c->y0, color, 255);
}

void gfx_rect(int x, int y, int w, int h, uint16_t color, uint8_t alpha) {
  if (w <= 0 || h <= 0 || !alpha) return;
  Item *it = add(K_RECT);
  if (!it) return;
  it->kind = K_RECT;   /* (rects are in screen pixels: no zoom) */
  it->x = (int16_t)x;
  it->y = (int16_t)y;
  it->w = (int16_t)w;
  it->h = (int16_t)h;
  it->color = color;
  it->alpha = alpha;
}

void gfx_sprite_ex(uint16_t spr, int sx, int sy, int w, int h, int x, int y, uint8_t flags, uint8_t alpha,
                   uint16_t tint) {
  if (spr >= nsprites() || !alpha || w <= 0 || h <= 0) return;
  if (x >= SCREEN_W || y >= SCREEN_H || x + w * zoom <= 0 || y + h * zoom <= 0) return;
  Item *it = add(K_SPRITE);
  if (!it) return;
  it->ref = spr;
  it->x = (int16_t)x;
  it->y = (int16_t)y;
  it->sx = (int16_t)sx;
  it->sy = (int16_t)sy;
  it->w = (int16_t)w;
  it->h = (int16_t)h;
  it->flags = flags;
  it->alpha = alpha;
  it->color = tint;
}

void gfx_sprite(uint16_t spr, int x, int y, uint8_t flags, uint8_t alpha) {
  gfx_sprite_ex(spr, 0, 0, spr_w(spr), spr_h(spr), x, y, flags, alpha, GFX_NO_TINT);
}

void gfx_sprite_part(uint16_t spr, int sx, int sy, int w, int h, int x, int y, uint8_t flags, uint8_t alpha) {
  gfx_sprite_ex(spr, sx, sy, w, h, x, y, flags, alpha, GFX_NO_TINT);
}

void gfx_sprite_tint(uint16_t spr, int x, int y, uint8_t flags, uint16_t tint) {
  gfx_sprite_ex(spr, 0, 0, spr_w(spr), spr_h(spr), x, y, flags, 255, tint);
}

void gfx_frame(uint16_t spr, int fw, int i, int x, int y, uint8_t flags, uint8_t alpha) {
  gfx_sprite_ex(spr, (fw + 2) * i, 0, fw, spr_h(spr), x, y, flags, alpha, GFX_NO_TINT);
}

int gfx_text_ex(uint8_t font, int x, int y, const char *s, uint16_t color, uint8_t alpha) {
  Font f;
  if (!s || !font_open(font, &f)) return 0;
  int top, bottom;
  int w = text_layout(&f, 0, 0, 1, s, NULL, NULL, &top, &bottom);
  size_t n = strlen(s) + 1;
  if (!alpha || bottom <= top) return w;
  if (ntext + n > GFX_TEXT_POOL) {
    dropped++;
    return w;
  }
  Item *it = add(K_TEXT);
  if (!it) return w;
  memcpy(text_pool + ntext, s, n);
  it->ref = (uint16_t)ntext;
  ntext += (int)n;
  it->x = (int16_t)x;
  it->y = (int16_t)y;
  it->sx = font;
  it->sy = (int16_t)top;
  it->w = (int16_t)w;
  it->h = (int16_t)(bottom - top);
  it->color = color;
  it->alpha = alpha;
  return w;
}

int gfx_text(uint8_t font, int x, int y, const char *s, uint16_t color) { return gfx_text_ex(font, x, y, s, color, 255); }

void gfx_call(GfxBandFn fn, void *ctx, int y, int h) {
  if (!fn || h <= 0) return;
  if (ncalls >= MAX_CALLS) {
    dropped++;
    return;
  }
  Item *it = add(K_CALL);
  if (!it) return;
  calls[ncalls].fn = fn;
  calls[ncalls].ctx = ctx;
  it->ref = (uint16_t)ncalls++;
  it->y = (int16_t)y;
  it->h = (int16_t)h;
}

/* ---------------------------------------------------------------- band functions' helpers */
void gfx_band_sprite(const GfxBand *b, uint16_t spr, int sx, int sy, int w, int h, int x, int y, uint8_t flags,
                     uint8_t alpha, uint16_t tint) {
  int y0 = b->cy0, y1 = b->cy1;
  if (y0 >= y1) return;
  draw_sprite(b->px, b->y0, y0, y1, b->cx0, b->cx1, spr, sx, sy, w, h, x, y, b->zoom, flags, alpha, tint);
}

void gfx_band_span(const GfxBand *b, int y, int x0, int x1, uint16_t color, uint8_t alpha) {
  if (y < b->cy0 || y >= b->cy1 || !alpha) return;
  if (x0 < b->cx0) x0 = b->cx0;
  if (x1 > b->cx1) x1 = b->cx1;
  if (x0 < x1) span(b->px + (y - b->y0) * SCREEN_W, x0, x1, color, gfx_alpha32(alpha));
}

/* ---------------------------------------------------------------- rasterising */
static void draw_item(const Item *it, int by0, int by1) {
  const Clip *c = &clips[it->clip];
  int z = it->kind >> 4, kind = it->kind & 15;
  int y0 = by0 > c->y0 ? by0 : c->y0, y1 = by1 < c->y1 ? by1 : c->y1;
  if (y0 >= y1 || c->x0 >= c->x1) return;
  switch (kind) {
  case K_RECT: {
    int ry0 = it->y > y0 ? it->y : y0, ry1 = it->y + it->h < y1 ? it->y + it->h : y1;
    int x0 = it->x > c->x0 ? it->x : c->x0, x1 = it->x + it->w < c->x1 ? it->x + it->w : c->x1;
    if (x0 >= x1) return;
    uint32_t a5 = gfx_alpha32(it->alpha);
    for (int y = ry0; y < ry1; y++) span(band + (y - by0) * SCREEN_W, x0, x1, it->color, a5);
    break;
  }
  case K_SPRITE:
    draw_sprite(band, by0, y0, y1, c->x0, c->x1, it->ref, it->sx, it->sy, it->w, it->h, it->x, it->y, z, it->flags,
                it->alpha, it->color);
    break;
  case K_TEXT: {
    Font f;
    if (!font_open((uint8_t)it->sx, &f)) return;
    TextDraw td = {band, by0, y0, y1, c->x0, c->x1, it->color, gfx_alpha32(it->alpha), false};
    text_layout(&f, it->x, it->y, z, text_pool + it->ref, NULL, &td, NULL, NULL);
    break;
  }
  case K_CALL: {
    int ry0 = it->y > y0 ? it->y : y0, ry1 = it->y + it->h < y1 ? it->y + it->h : y1;
    if (ry0 >= ry1) return;
    GfxBand b = {band, by0, by1, c->x0, ry0, c->x1, ry1, z};
    calls[it->ref].fn(&b, calls[it->ref].ctx);
    break;
  }
  }
}

/* the screen rows an item may touch */
static void item_rows(const Item *it, int *y0, int *y1) {
  int z = it->kind >> 4;
  switch (it->kind & 15) {
  case K_SPRITE:
    *y0 = it->y;
    *y1 = it->y + it->h * z;
    break;
  case K_TEXT: {
    /* (each line's own top and bottom are inside the first's and the last's) */
    *y0 = it->y + it->sy * z;
    *y1 = it->y + (it->sy + it->h) * z;
    break;
  }
  default:
    *y0 = it->y;
    *y1 = it->y + it->h;
  }
}

void gfx_end(void) {
  if (!band) return;
  /* a first command that covers the whole screen needs no clearing first */
  bool covered = nitems && (items[0].kind & 15) == K_RECT && items[0].alpha == 255 && items[0].x <= 0 &&
                 items[0].y <= 0 && items[0].x + items[0].w >= SCREEN_W && items[0].y + items[0].h >= SCREEN_H &&
                 clips[items[0].clip].x0 == 0 && clips[items[0].clip].y0 == 0 &&
                 clips[items[0].clip].x1 == SCREEN_W && clips[items[0].clip].y1 == SCREEN_H;
  for (int by0 = 0; by0 < SCREEN_H; by0 += GFX_BAND) {
    int by1 = by0 + GFX_BAND < SCREEN_H ? by0 + GFX_BAND : SCREEN_H;
    if (!covered) memset(band, 0, sizeof(uint16_t) * SCREEN_W * GFX_BAND);
    for (int i = 0; i < nitems; i++) {
      int y0, y1;
      item_rows(&items[i], &y0, &y1);
      if (y1 <= by0 || y0 >= by1) continue;
      draw_item(&items[i], by0, by1);
    }
    plat_push(0, by0, SCREEN_W, by1 - by0, band);
  }
}
