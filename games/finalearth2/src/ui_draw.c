/* The Final Earth 2: drawing the user interface (the original's
 * gui.NinePatch, graphics.BitmapText, gui.MaterialsCostDisplay and
 * gui.IconListDisplay), as a list of primitives drawn by one band function,
 * and the texts' numbers (common.Localize.lo, MathExtensions). */
#include <math.h>
#include "uii.h"

UiFrame *uf;

/* the original's 9-patches: each is a 1-pixel border round a fill (colours and alphas from its sprite) */
/* rgb565_near, as a constant */
#define NEAR565(c) ((uint16_t)(((((c) >> 16 & 255) * 31 + 127) / 255) << 11 | ((((c) >> 8 & 255) * 63 + 127) / 255) << 5 | \
                               (((c) & 255) * 31 + 127) / 255))
#define BOX(b, f, ba, fa) {NEAR565(b), NEAR565(f), ba, fa}
const Box boxes[BX_COUNT] = {
  [BX_WINDOW] = BOX(0x000000, 0xFFFFFF, 255, 180),
  [BX_WINDOW_OPAQUE] = BOX(0x000000, 0xF4F4F4, 255, 250),
  [BX_TOOLTIP] = BOX(0x969696, 0xF2F2F2, 230, 230),
  [BX_INFO] = BOX(0x000000, 0xFFFFFF, 255, 180),
  [BX_HINTINFO] = BOX(0x4D9600, 0xC3FF7F, 230, 230),
  [BX_ACTIVE] = BOX(0x94BA8D, 0xFFFFFF, 255, 255),
  [BX_DISABLED] = BOX(0xDBDBDB, 0xFFFFFF, 255, 255),
  [BX_BUTTON] = BOX(0x000000, 0xFFFFFF, 255, 255),
  [BX_BUTTON_HOVER] = BOX(0x7F7F7F, 0xFFFFFF, 255, 255),
  [BX_BUTTON_DOWN] = BOX(0xBCBCBC, 0xFFFFFF, 255, 255),
  [BX_TRANS] = BOX(0x000000, 0xFFFFFF, 255, 180),
  [BX_TRANS_HOVER] = BOX(0x969696, 0xFFFFFF, 255, 180),
  [BX_TRANS_DOWN] = BOX(0xFFFFFF, 0xFFFFFF, 255, 180),
  [BX_BLD] = BOX(0x000000, 0xFFFFFF, 255, 128),
  [BX_BLD_HOVER] = BOX(0x7F7F7F, 0xFFFFFF, 255, 128),
  [BX_BLD_DOWN] = BOX(0xFFFFFF, 0xFFFFFF, 255, 128),
  [BX_TRANSB] = BOX(0x000000, 0xFFFFFF, 255, 128),
  [BX_TRANSB_HOVER] = BOX(0x969696, 0xFFFFFF, 255, 128),
  [BX_TRANSB_DOWN] = BOX(0xFFFFFF, 0xFFFFFF, 255, 128),
  [BX_CHECKED] = BOX(0x94BA8D, 0xFFFFFF, 255, 255),
  [BX_CHECKED_HOVER] = BOX(0xB6CCB1, 0xFFFFFF, 255, 255),
  [BX_CHECKED_DOWN] = BOX(0xD8E2D5, 0xFFFFFF, 255, 255),
  [BX_ACTIVABLE] = BOX(0x000000, 0xFFFFFF, 255, 255),
  [BX_ACTIVABLE_HOVER] = BOX(0x7F7F7F, 0xFFFFFF, 255, 255),
  [BX_ACTIVABLE_DOWN] = BOX(0x94BA8D, 0xFFFFFF, 255, 255),
  [BX_GRAY] = BOX(0x404040, 0xE0E0E0, 255, 255),
  [BX_GRAY_HOVER] = BOX(0x7F7F7F, 0xE0E0E0, 255, 255),
  [BX_GRAY_DOWN] = BOX(0xBCBCBC, 0xE0E0E0, 255, 255),
  [BX_SCROLL] = BOX(0x000000, 0x000000, 255, 0),
  [BX_HANDLE] = BOX(0x000000, 0xFFFFFF, 255, 255),
};

/* ---------------------------------------------------------------- the list */
static UiPrim *prim(uint8_t kind, int x, int y, int w, int h) {
  if (!uf) return NULL;
  if (kind != PR_CLIP) {
    /* outside the clip: nothing to draw (text: its line, give or take) */
    int y0 = y, y1 = y + h;
    if (kind == PR_TEXT) y0 -= 4, y1 += 4;
    if (y1 <= uf->cy0 || y0 >= uf->cy1 || x >= uf->cx1 || x + w <= uf->cx0) return NULL;
  }
  if (uf->n >= UI_PRIMS) {
    uf->dropped++;
    return NULL;
  }
  UiPrim *p = &uf->p[uf->n++];
  memset(p, 0, sizeof *p);
  p->kind = kind;
  p->x = (int16_t)x;
  p->y = (int16_t)y;
  p->w = (int16_t)w;
  p->h = (int16_t)h;
  p->a1 = p->a2 = 255;
  return p;
}

void ui_clip(int x, int y, int w, int h) {
  if (!uf) return;
  int x1 = x + w, y1 = y + h;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x1 > SCREEN_W) x1 = SCREEN_W;
  if (y1 > SCREEN_H) y1 = SCREEN_H;
  if (uf->n && uf->p[uf->n - 1].kind == PR_CLIP) uf->n--;   /* a clip nothing used */
  uf->cx0 = (int16_t)x, uf->cy0 = (int16_t)y, uf->cx1 = (int16_t)x1, uf->cy1 = (int16_t)y1;
  prim(PR_CLIP, x, y, x1 - x, y1 - y);
}
void ui_noclip(void) { ui_clip(0, 0, SCREEN_W, SCREEN_H); }

void ui_rect(int x, int y, int w, int h, uint16_t c, uint8_t a) {
  if (w <= 0 || h <= 0 || !a) return;
  UiPrim *p = prim(PR_RECT, x, y, w, h);
  if (p) p->c1 = c, p->a1 = a;
}

void ui_box(int x, int y, int w, int h, int box) {
  if (w <= 0 || h <= 0) return;
  UiPrim *p = prim(PR_BOX, x, y, w, h);
  if (!p) return;
  const Box *b = &boxes[box];
  p->c1 = b->border, p->c2 = b->fill, p->a1 = b->ba, p->a2 = b->fa;
}

/* a box added now (so it is under what follows) whose height comes later; never culled */
UiPrim *ui_box_later(int x, int y, int w, int box) {
  if (!uf || uf->n >= UI_PRIMS) {
    if (uf) uf->dropped++;
    return NULL;
  }
  UiPrim *p = &uf->p[uf->n++];
  memset(p, 0, sizeof *p);
  p->kind = PR_BOX;
  p->x = (int16_t)x, p->y = (int16_t)y, p->w = (int16_t)w, p->h = 0;
  const Box *b = &boxes[box];
  p->c1 = b->border, p->c2 = b->fill, p->a1 = b->ba, p->a2 = b->fa;
  return p;
}

void ui_spr_ex(uint16_t spr, int sx, int sy, int w, int h, int x, int y, uint8_t flags, uint8_t alpha, uint16_t tint) {
  if (!spr || w <= 0 || h <= 0 || !alpha) return;
  UiPrim *p = prim(PR_SPR, x, y, w, h);
  if (!p) return;
  p->c1 = spr, p->c2 = tint, p->a1 = alpha, p->f = flags;
  p->u.part.sx = (int16_t)sx, p->u.part.sy = (int16_t)sy;
}
void ui_spr(uint16_t spr, int x, int y) { ui_spr_ex(spr, 0, 0, spr_w(spr), spr_h(spr), x, y, 0, 255, GFX_NO_TINT); }
void ui_keys(int x, int y, int cw, int h, const char *chars, int n, int focused, bool down) {
  if (!uf || uf->ntext + n > UI_TEXT) {
    if (uf) uf->dropped++;
    return;
  }
  UiPrim *p = prim(PR_KEYS, x, y, cw * n, h);
  if (!p) return;
  memcpy(uf->text + uf->ntext, chars, (size_t)n);
  p->u.s = uf->text + uf->ntext;
  uf->ntext += n;
  p->f = (uint8_t)n, p->a1 = (uint8_t)cw, p->a2 = (uint8_t)focused, p->c1 = down;
}
void ui_ring(int x, int y, int w, int h, int t, uint16_t c) {
  ui_rect(x - t, y - t, w + 2 * t, t, c, 255);
  ui_rect(x - t, y + h, w + 2 * t, t, c, 255);
  ui_rect(x - t, y, t, h, c, 255);
  ui_rect(x + w, y, t, h, c, 255);
}
void ui_spr_small(uint16_t spr, int x, int y) {
  UiPrim *p = prim(PR_SPR_SMALL, x, y, (spr_w(spr) * 3 + 4) / 5, (spr_h(spr) * 3 + 4) / 5);
  if (p) p->c1 = spr;
}
void ui_frame(uint16_t spr, int fw, int i, int x, int y, uint8_t flags, uint8_t alpha) {
  ui_spr_ex(spr, (fw + 2) * i, 0, fw, spr_h(spr), x, y, flags, alpha, GFX_NO_TINT);
}

int ui_text_n(uint8_t font, int x, int y, const char *s, int n, uint16_t color, uint8_t alpha, bool keep) {
  if (!s) return 0;
  if (n < 0) n = (int)strlen(s);
  int w = gfx_text_width_n(font, s, n);
  if (!n || !uf) return w;
  UiPrim *p = prim(PR_TEXT, x, y, w, ui_line_h(font));
  if (!p) return w;
  if (!keep) {
    if (uf->ntext + n > UI_TEXT) {
      uf->dropped++;
      uf->n--;
      return w;
    }
    memcpy(uf->text + uf->ntext, s, (size_t)n);
    s = uf->text + uf->ntext;
    uf->ntext += n;
  }
  p->u.s = s;
  p->w = (int16_t)n;
  p->f = font;
  p->c1 = color;
  p->a1 = alpha;
  return w;
}

void ui_text_rot(uint8_t font, int x, int y, const char *s, uint16_t color) {
  int n = (int)strlen(s), w = gfx_text_width(font, s);
  UiPrim *p = prim(PR_TEXTROT, x - ui_line_h(font) - 4, y, ui_line_h(font) + 8, w);
  if (!p || uf->ntext + n > UI_TEXT) {
    if (p) uf->n--, uf->dropped++;
    return;
  }
  memcpy(uf->text + uf->ntext, s, (size_t)n);
  p->u.s = uf->text + uf->ntext;
  uf->ntext += n;
  p->x = (int16_t)x, p->w = (int16_t)n, p->f = font, p->c1 = color;
}

int ui_text_w(uint8_t font, const char *s) { return s ? gfx_text_width(font, s) : 0; }

int ui_para(uint8_t font, int x, int y, int max_w, const char *s, uint16_t color, bool keep, int *width) {
  int lh = ui_line_h(font), h = 0, widest = 0;
  if (!s) s = "";
  const char *p = s;
  do {
    int lw;
    const char *next;
    int n = gfx_text_wrap(font, p, max_w, &lw, &next);
    if (y != INT16_MIN) ui_text_n(font, x, y + h, p, n, color, 255, keep);
    if (lw > widest) widest = lw;
    h += lh;
    p = next;
  } while (*p);
  if (width) *width = widest;
  return h;
}

const char *ui_color_prefix(const char *s, uint16_t *color) {
  if (!s) return "";
  if (!strncmp(s, "[red]", 5)) return *color = C_RED, s + 5;
  if (!strncmp(s, "[blue]", 6)) return *color = C_BLUE, s + 6;
  if (!strncmp(s, "[green]", 7)) return *color = C_GREEN, s + 7;
  if (!strncmp(s, "[white]", 7)) return *color = C_WHITE, s + 7;
  return s;
}

/* ---------------------------------------------------------------- the band function */
static void draw_box(const GfxBand *b, const UiPrim *p) {
  int x0 = p->x, x1 = p->x + p->w, y0 = p->y, y1 = p->y + p->h;
  for (int y = y0 > b->cy0 ? y0 : b->cy0; y < y1 && y < b->cy1; y++) {
    if (y == y0 || y == y1 - 1) {
      gfx_band_span(b, y, x0, x1, p->c1, p->a1);
    } else {
      gfx_band_span(b, y, x0, x0 + 1, p->c1, p->a1);
      gfx_band_span(b, y, x0 + 1, x1 - 1, p->c2, p->a2);
      gfx_band_span(b, y, x1 - 1, x1, p->c1, p->a1);
    }
  }
}

/* PR_SPR_SMALL: row by row, the sprite's row (5/3 as far down) is drawn over what is under its pixels, then each
 * screen pixel takes the nearest of them */
static void draw_small(const GfxBand *b, const UiPrim *p) {
  int sw = spr_w(p->c1), sh = spr_h(p->c1);
  if (sw > SCREEN_W) return;
  uint16_t line[SCREEN_W];
  GfxBand one = {line, 0, 1, 0, 0, sw, 1, 1};
  int x0 = p->x > b->cx0 ? p->x : b->cx0, x1 = p->x + p->w < b->cx1 ? p->x + p->w : b->cx1;
  for (int y = p->y > b->cy0 ? p->y : b->cy0; y < p->y + p->h && y < b->cy1; y++) {
    int sy = (y - p->y) * 5 / 3;
    if (sy >= sh || x0 >= x1) continue;
    uint16_t *row = b->px + (y - b->y0) * SCREEN_W;
    for (int sx = 0; sx < sw; sx++) {
      int dx = p->x + sx * 3 / 5;
      line[sx] = dx >= 0 && dx < SCREEN_W ? row[dx] : 0;
    }
    gfx_band_sprite(&one, p->c1, 0, sy, sw, 1, 0, 0, 0, 255, GFX_NO_TINT);
    for (int x = x0; x < x1; x++) row[x] = line[(x - p->x) * 5 / 3 < sw ? (x - p->x) * 5 / 3 : sw - 1];
  }
}

/* PR_KEYS: each key a button (spr_button's normal, hover or down 9-patch) round its character */
static void draw_keys(const GfxBand *b, const UiPrim *p) {
  for (int i = 0; i < p->f; i++) {
    UiPrim k = *p;
    int box = i == p->a2 ? (p->c1 ? BX_BUTTON_DOWN : BX_BUTTON_HOVER) : BX_BUTTON;
    k.x = (int16_t)(p->x + i * p->a1), k.w = p->a1;
    k.c1 = boxes[box].border, k.c2 = boxes[box].fill, k.a1 = boxes[box].ba, k.a2 = boxes[box].fa;
    draw_box(b, &k);
    int tw = gfx_text_width_n(FONT_STANDARD, p->u.s + i, 1);
    gfx_band_text(b, FONT_STANDARD, k.x + (p->a1 - tw) / 2, p->y + 1, p->u.s + i, 1, C_BLACK, 255, false);
  }
}

static void ui_band(const GfxBand *band, void *ctx) {
  UiFrame *f = ctx;
  GfxBand b = *band;
  for (int i = 0; i < f->n; i++) {
    const UiPrim *p = &f->p[i];
    if (p->kind == PR_CLIP) {
      b.cx0 = p->x > band->cx0 ? p->x : band->cx0;
      b.cx1 = p->x + p->w < band->cx1 ? p->x + p->w : band->cx1;
      b.cy0 = p->y > band->cy0 ? p->y : band->cy0;
      b.cy1 = p->y + p->h < band->cy1 ? p->y + p->h : band->cy1;
      continue;
    }
    if (b.cy0 >= b.cy1 || b.cx0 >= b.cx1) continue;
    switch (p->kind) {
    case PR_RECT:
      if (p->y + p->h <= b.cy0 || p->y >= b.cy1) break;
      for (int y = p->y > b.cy0 ? p->y : b.cy0; y < p->y + p->h && y < b.cy1; y++)
        gfx_band_span(&b, y, p->x, p->x + p->w, p->c1, p->a1);
      break;
    case PR_BOX:
      if (p->y + p->h <= b.cy0 || p->y >= b.cy1) break;
      draw_box(&b, p);
      break;
    case PR_SPR:
      if (p->y + p->h <= b.cy0 || p->y >= b.cy1) break;
      gfx_band_sprite(&b, p->c1, p->u.part.sx, p->u.part.sy, p->w, p->h, p->x, p->y, p->f, p->a1, p->c2);
      break;
    case PR_TEXT:
      if (p->y + p->h + 6 <= b.cy0 || p->y - 6 >= b.cy1) break;
      gfx_band_text(&b, p->f, p->x, p->y, p->u.s, p->w, p->c1, p->a1, false);
      break;
    case PR_TEXTROT:
      if (p->y + p->h <= b.cy0 || p->y >= b.cy1) break;
      gfx_band_text(&b, p->f, p->x, p->y, p->u.s, p->w, p->c1, 255, true);
      break;
    case PR_SPR_SMALL:
      if (p->y + p->h <= b.cy0 || p->y >= b.cy1) break;
      draw_small(&b, p);
      break;
    case PR_KEYS:
      if (p->y + p->h + 6 <= b.cy0 || p->y - 6 >= b.cy1) break;
      draw_keys(&b, p);
      break;
    }
  }
}

void ui_frame_begin(UiFrame *f) {
  f->n = f->ntext = f->dropped = 0;
  f->tip.on = false;
  f->cx0 = f->cy0 = 0;
  f->cx1 = SCREEN_W, f->cy1 = SCREEN_H;
  uf = f;
}

int ui_peak_prims, ui_peak_text, ui_dropped_total;   /* for the tests */
int ui_redraws;   /* windows laid out again in the frame they changed size (tests) */

void ui_frame_end(void) {
  if (!uf) return;
  if (uf->n > ui_peak_prims) ui_peak_prims = uf->n;
  if (uf->ntext > ui_peak_text) ui_peak_text = uf->ntext;
  ui_dropped_total += uf->dropped;
  if (uf->n) {
    gfx_zoom(1);
    gfx_clip(0, 0, SCREEN_W, SCREEN_H);
    gfx_call(ui_band, uf, 0, SCREEN_H);
  }
}

/* ---------------------------------------------------------------- numbers and texts */
int fmt_int(char *out, int size, long v) {
  char tmp[24];
  int n = 0;
  unsigned long u = v < 0 ? (unsigned long)-v : (unsigned long)v;
  do tmp[n++] = (char)('0' + u % 10), u /= 10;
  while (u);
  if (v < 0) tmp[n++] = '-';
  int k = 0;
  while (n && k < size - 1) out[k++] = tmp[--n];
  out[k] = 0;
  return k;
}

/* String(v) for the numbers the interface shows: integers, or up to 6 decimals (trailing zeros dropped) */
int fmt_num(char *out, int size, double v) {
  if (v != v) v = 0;
  bool neg = v < 0;
  if (neg) v = -v;
  double ip = floor(v);
  long frac = (long)((v - ip) * 1e6 + 0.5);
  if (frac >= 1000000) ip += 1, frac = 0;
  char tmp[40];
  int n = 0;
  if (neg && (ip > 0 || frac > 0)) tmp[n++] = '-';
  n += fmt_int(tmp + n, (int)sizeof tmp - n, ip > 2e9 ? 2000000000L : (long)ip);
  if (frac > 0) {
    char fd[7];
    for (int i = 5; i >= 0; i--) fd[i] = (char)('0' + frac % 10), frac /= 10;
    int last = 5;
    while (last >= 0 && fd[last] == '0') last--;
    tmp[n++] = '.';
    for (int i = 0; i <= last; i++) tmp[n++] = fd[i];
  }
  tmp[n] = 0;
  int k = 0;
  for (; k < n && k < size - 1; k++) out[k] = tmp[k];
  out[k] = 0;
  return k;
}

/* MathExtensions.floatFormat: v with at most that many decimals, as JavaScript shows it */
int ui_float_fmt(char *out, int size, double v, int decimals) {
  double m = pow(10, decimals);
  return fmt_num(out, size, round(v * m) / m);
}

/* MathExtensions.largeNumberFormat(Alt): thousands and millions shortened */
int fmt_large(char *out, int size, double v, bool alt) {
  char num[24];
  const char *suffix = "";
  if (v >= 1e6) v = (alt ? floor(v / 1e5) : floor(v / 1e5)) / 10, suffix = "M";
  else if (v >= 1e4) v = floor(v / 100) / 10, suffix = "k";
  fmt_num(num, sizeof num, v);
  out[0] = 0;
  str_cat(out, size, num);
  return (int)strlen(str_cat(out, size, suffix));
}

char *str_cat(char *dst, int size, const char *src) {
  int n = (int)strlen(dst);
  while (src && *src && n < size - 1) dst[n++] = *src++;
  dst[n] = 0;
  return dst;
}

const char *lo(char *buf, int size, const char *text, int n, const LoArg *args) {
  int o = 0;
  if (!text) text = "";
  for (const char *s = text; *s && o < size - 1;) {
    if (s[0] == '[' && s[1] >= '0' && s[1] <= '9' && s[2] == ']' && s[1] - '0' < n) {
      const LoArg *a = &args[s[1] - '0'];
      if (a->s) {
        for (const char *t = a->s; *t && o < size - 1;) buf[o++] = *t++;
      } else {
        o += fmt_num(buf + o, size - o, a->v);
      }
      s += 3;
    } else {
      buf[o++] = *s++;
    }
  }
  buf[o] = 0;
  return buf;
}
const char *lo_1(char *buf, int size, const char *text, LoArg a) { return lo(buf, size, text, 1, &a); }
const char *lo_2(char *buf, int size, const char *text, LoArg a, LoArg b) {
  LoArg v[2] = {a, b};
  return lo(buf, size, text, 2, v);
}
const char *lo_3(char *buf, int size, const char *text, LoArg a, LoArg b, LoArg c) {
  LoArg v[3] = {a, b, c};
  return lo(buf, size, text, 3, v);
}

/* ---------------------------------------------------------------- materials */
const uint16_t mat_icon[M_COUNT] = {
  [M_FOOD] = SPR_RESOURCE_FOOD, [M_WOOD] = SPR_RESOURCE_WOOD, [M_STONE] = SPR_RESOURCE_STONE,
  [M_MACHINEPARTS] = SPR_RESOURCE_MACHINEPARTS, [M_REFINEDMETAL] = SPR_RESOURCE_REFINEDMETAL,
  [M_COMPUTERCHIPS] = SPR_RESOURCE_COMPUTERCHIPS, [M_GRAPHENE] = SPR_RESOURCE_GRAPHENE,
  [M_ROCKETFUEL] = SPR_RESOURCE_ROCKETFUEL, [M_KNOWLEDGE] = SPR_RESOURCE_KNOWLEDGE,
};
/* lo(name): as the original names them in its texts */
const char *const mat_lname[M_COUNT] = {
  [M_FOOD] = L_food, [M_WOOD] = L_wood, [M_STONE] = L_stone, [M_MACHINEPARTS] = L_machine_parts,
  [M_REFINEDMETAL] = L_refined_metals, [M_COMPUTERCHIPS] = L_computer_chips, [M_CACAO] = "cacao",
  [M_CHOCOLATE] = "chocolate", [M_GRAPHENE] = L_graphene, [M_ROCKETFUEL] = L_rocketFuel, [M_KNOWLEDGE] = L_knowledge,
};
/* firstToUpper(lo(name)) */
const char *mat_title(int m, char *buf, int size) {
  buf[0] = 0;
  str_cat(buf, size, mat_lname[m]);
  if (buf[0] >= 'a' && buf[0] <= 'z') buf[0] = (char)(buf[0] - 32);
  return buf;
}

/* MaterialsCostDisplay.updateCostDisplay: food, wood, stone, machine parts, refined metal, computer chips,
 * graphene, rocket fuel, then knowledge (with the texts around it) */
int ui_cost(int x, int y, const float cost[M_COUNT], const char *before_k, const char *after_k, bool city_red) {
  static const uint8_t order[] = {M_FOOD, M_WOOD, M_STONE, M_MACHINEPARTS, M_REFINEDMETAL, M_COMPUTERCHIPS, M_CACAO,
                                  M_CHOCOLATE, M_GRAPHENE, M_ROCKETFUEL, M_KNOWLEDGE};
  bool draw = y != INT16_MIN;
  int xx = 0;
  bool any_before = false, shown_before_k = false;
  char num[48];
  if (!before_k) before_k = "";
  for (unsigned k = 0; k < sizeof order; k++) {
    int m = order[k];
    if (cost[m] <= 0) continue;
    if (m == M_KNOWLEDGE && *before_k && any_before) {
      if (draw) ui_text(FONT_STANDARD, x + xx, y - 1, before_k, C_BLACK);
      xx += ui_text_w(FONT_STANDARD, before_k) + 3;
      shown_before_k = true;
    }
    if (m == M_FOOD) xx -= 2;
    any_before = true;
    if (draw) ui_spr(mat_icon[m], x + xx, y);
    xx += 10 + 1;
    if (m == M_FOOD) xx -= 2;
    fmt_num(num, sizeof num, cost[m]);
    if (m == M_KNOWLEDGE && after_k) str_cat(num, sizeof num, after_k);
    bool red = city_red && city.materials.m[m] < cost[m];
    if (draw) ui_text(FONT_STANDARD, x + xx, y - 1, num, red ? C_RED : C_BLACK);
    xx += ui_text_w(FONT_STANDARD, num) + 3;
  }
  if (!shown_before_k && *before_k) {
    if (draw) ui_text(FONT_STANDARD, x + xx, y - 1, before_k, C_BLACK);
    xx += ui_text_w(FONT_STANDARD, before_k) + 3;
  }
  return xx;
}

int ui_icon_list(int x, int y, const IconText *it, int n) {
  int xx = 0;
  bool draw = y != INT16_MIN;
  for (int i = 0; i < n; i++) {
    if (draw) ui_spr(it[i].spr, x + xx, y);
    xx += spr_w(it[i].spr) + 2;
    uint16_t col = C_BLACK;
    const char *t = ui_color_prefix(it[i].text, &col);
    bool one = !strcmp(t, "1");
    if (one) xx--;
    if (draw) ui_text(FONT_STANDARD, x + xx, y - 1, t, col);
    if (one) xx--;
    xx += ui_text_w(FONT_STANDARD, t) + 3;
  }
  return xx;
}
