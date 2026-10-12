/* NumPowder: what's drawn over the sim and below it: what's under the brush (small, or the original's debug HUD:
 * GameView::OnDraw's FPS box and sample), the brush's outline (Brush::RenderPoint, RenderLine, RenderRect), the panel
 * below the sim (what OK and Back hold, the brush, the keys) and the welcome card. */
#include "ui.h"
#include <string.h>

/* ---------------------------------------------------------------- numbers as text (no printf on the calculator) */
#define cat text_cat
#define cat_fixed text_cat_fixed
int fmt_int(char *out, int v) {
  char tmp[12];
  int n = 0, k = 0;
  unsigned u = v < 0 ? (unsigned)-(long)v : (unsigned)v;
  do tmp[n++] = (char)('0' + u % 10), u /= 10;
  while (u);
  if (v < 0) out[k++] = '-';
  while (n) out[k++] = tmp[--n];
  out[k] = 0;
  return k;
}
/* Format::Precision: a fixed number of decimals */
int fmt_fixed(char *out, float v, int decimals) {
  if (!isfinite(v)) return (int)(strcpy(out, "nan"), 3);
  int k = 0;
  if (v < 0) out[k++] = '-', v = -v;
  long scale = 1;
  for (int i = 0; i < decimals; i++) scale *= 10;
  long q = (long)(v * scale + 0.5f);
  k += fmt_int(out + k, (int)(q / scale));
  if (decimals) {
    out[k++] = '.';
    long f = q % scale;
    for (long d = scale / 10; d; d /= 10) out[k++] = (char)('0' + (f / d) % 10);
  }
  out[k] = 0;
  return k;
}
char *text_cat(char *p, const char *end, const char *s) {
  while (*s && p < end - 1) *p++ = *s++;
  *p = 0;
  return p;
}
static char *cat_int(char *p, const char *end, int v) {
  char b[12];
  fmt_int(b, v);
  return cat(p, end, b);
}
char *text_cat_fixed(char *p, const char *end, float v) {
  char b[24];
  fmt_fixed(b, v, 2);
  return cat(p, end, b);
}

/* breaks a text into lines no wider than `width` at its spaces */
void text_wrap(char *out, int n, const char *s, int width) {
  int len = 0, line_start = 0, last_space = -1;
  for (; *s && len < n - 1; s++) {
    out[len] = *s;
    if (*s == '\n') line_start = len + 1, last_space = -1;
    else if (*s == ' ') last_space = len;
    len++;
    out[len] = 0;
    if (font_text_width(out + line_start) > width && last_space > line_start) {
      out[last_space] = '\n';
      line_start = last_space + 1;
      last_space = -1;
    }
  }
  out[len] = 0;
}

/* ---------------------------------------------------------------- what's under the cursor */
/* SimulationData::ElementResolve */
static const char *element_resolve(int type, int ctype) {
  if (type == PT_LIFE) return ctype >= 0 && ctype < NGOL ? builtinGol[ctype].name : "LIFE";
  if (type >= 0 && type < PT_NUM) return elements[type].Name;
  return "Empty";
}

static const Particle *particle_at(int x, int y, int *id) {
  int r = PHOTAT(x, y);
  if (!r) r = PMAPAT(x, y);
  if (!r) return NULL;
  *id = ID(r);
  return &parts[ID(r)];
}

/* GameView::OnDraw's sample: the particle (with the debug HUD's details), the wall, the air */
void particle_info(char *out, int n, int x, int y, bool line2) {
  char *p = out, *end = out + n;
  *p = 0;
  int id = -1;
  const Particle *s = particle_at(x, y, &id);
  if (!line2) {
    if (s) {
      /* (BasicParticleInfo; with the debug HUD, GameView's details) */
      int type = s->type, ctype = s->ctype;
      int pipeType = TYP(ctype), stored = (type == PT_PIPE || type == PT_PPIP) ? tmp4_get(id) : 0;   /* (PIPE: its tmp4) */
      if (type == PT_LAVA && IsElement(ctype)) {
        p = cat(p, end, "Molten "), p = cat(p, end, element_resolve(ctype, -1));
      } else if ((type == PT_PIPE || type == PT_PPIP) && IsElement(pipeType)) {
        p = cat(p, end, element_resolve(type, -1));
        if (pipeType == PT_LAVA && IsElement(stored)) p = cat(p, end, " with molten "), p = cat(p, end, element_resolve(stored, -1));
        else p = cat(p, end, " with "), p = cat(p, end, element_resolve(pipeType, stored));
      } else if (ui.debug && type == PT_FILT) {
        static const char *const filtModes[] = {"set colour", "AND", "OR", "AND-NOT", "red shift", "blue shift", "no effect",
                                                "XOR", "NOT", "old QRTZ scattering", "variable red shift", "variable blue shift"};
        p = cat(p, end, element_resolve(type, ctype)), p = cat(p, end, " (");
        p = cat(p, end, s->tmp >= 0 && s->tmp <= 11 ? filtModes[s->tmp] : "unknown mode"), p = cat(p, end, ")");
      } else if (ui.debug && (type == PT_SEED || (type == PT_PLNT && (ctype & PLNT_CTYPE)))) {
        /* the genes: water, colour (cyan, magenta, yellow), the direction, active */
        static const char *const directions[8] = {"N", "NW", "W", "SW", "S", "SE", "E", "NE"};
        static const char *const colours[3][4] = {{"cc", "cC", "Cc", "CC"}, {"mm", "mM", "Mm", "MM"}, {"yy", "yY", "Yy", "YY"}};
        int colour = (ctype >> PLNT_COLOUR) & 0x3F;
        p = cat(p, end, element_resolve(type, ctype)), p = cat(p, end, " (");
        p = cat_int(p, end, (ctype >> PLNT_LIFE) & 0xFF), p = cat(p, end, " ");
        p = cat(p, end, colours[0][(colour >> 4) & 3]), p = cat(p, end, colours[1][(colour >> 2) & 3]);
        p = cat(p, end, colours[2][colour & 3]), p = cat(p, end, " "), p = cat(p, end, directions[(ctype >> PLNT_DIR) & 7]);
        p = cat(p, end, " "), p = cat_int(p, end, ctype & 1), p = cat(p, end, ")");
      } else {
        p = cat(p, end, element_resolve(type, ctype));
        bool wavelength = type == PT_PHOT || type == PT_BIZR || type == PT_BIZRG || type == PT_BIZRS || type == PT_FILT ||
                          type == PT_BRAY || type == PT_C5;
        if (ui.debug && !wavelength && type != PT_EMBR && type != PT_PRTI && type != PT_PRTO && type != PT_LIFE) {
          if (type == PT_CRAY || type == PT_DRAY || type == PT_CONV || type == PT_LDTC)
            p = cat(p, end, " ("), p = cat(p, end, element_resolve(TYP(ctype), ID(ctype))), p = cat(p, end, ")");
          else if (type == PT_CLNE || type == PT_BCLN || type == PT_PCLN || type == PT_PBCN || type == PT_DTEC)
            p = cat(p, end, " ("), p = cat(p, end, element_resolve(ctype, s->tmp)), p = cat(p, end, ")");
          else if (IsElement(ctype) && type != PT_GLOW && type != PT_WIRE && type != PT_SOAP && type != PT_LITH)
            p = cat(p, end, " ("), p = cat(p, end, element_resolve(ctype, 0)), p = cat(p, end, ")");
          else if (ctype)
            p = cat(p, end, " ("), p = cat_int(p, end, ctype), p = cat(p, end, ")");
        }
      }
      p = cat(p, end, ", Temp: "), p = cat_fixed(p, end, p_temp(s) - 273.15f), p = cat(p, end, "C");
    } else if (bmap[y / WCELL][x / WCELL] && bmap[y / WCELL][x / WCELL] < UI_WALLCOUNT) {
      p = cat(p, end, walltypes[bmap[y / WCELL][x / WCELL]].name);
    } else {
      p = cat(p, end, "Empty");
    }
    p = cat(p, end, ", Pressure: "), p = cat_fixed(p, end, pv[y / ACELL][x / ACELL]);
  } else {
    /* the debug HUD's: life, tmp (tmp2 where it means something), the particle's number and place */
    if (s) {
      int type = s->type;
      p = cat(p, end, "Life: "), p = cat_int(p, end, s->life);
      if (type == PT_CONV && IsElementOrNone(TYP(s->tmp)) && TYP(s->tmp))
        p = cat(p, end, ", Tmp: "), p = cat(p, end, element_resolve(TYP(s->tmp), ID(s->tmp)));
      else if (type != PT_RFRG && type != PT_RFGL && type != PT_LIFE) p = cat(p, end, ", Tmp: "), p = cat_int(p, end, s->tmp);
      if (type == PT_CRAY || type == PT_DRAY || type == PT_EXOT || type == PT_LIGH || type == PT_SOAP || type == PT_TRON ||
          type == PT_VIBR || type == PT_VIRS || type == PT_WARP || type == PT_LCRY || type == PT_CBNW || type == PT_TSNS ||
          type == PT_DTEC || type == PT_LSNS || type == PT_PSTN || type == PT_LDTC || type == PT_VSNS || type == PT_LITH ||
          type == PT_CONV || type == PT_ETRD)
        p = cat(p, end, ", Tmp2: "), p = cat_int(p, end, s->tmp2);
      p = cat(p, end, ", #"), p = cat_int(p, end, id), p = cat(p, end, ", ");
    }
    p = cat(p, end, "X:"), p = cat_int(p, end, x), p = cat(p, end, " Y:"), p = cat_int(p, end, y);
    if (aheat_enable) p = cat(p, end, ", AHeat: "), p = cat_fixed(p, end, hv[y / ACELL][x / ACELL] - 273.15f), p = cat(p, end, "C");
  }
}

/* sampling (GameView's middle click): the left tool becomes what's under the cursor */
void sample_at(int x, int y) {
  int id;
  const Particle *s = particle_at(x, y, &id);
  int tool = -1;
  if (s) {
    if (s->type == PT_LIFE) tool = PMAP(s->ctype >= 0 && s->ctype < NGOL ? s->ctype : 0, PT_LIFE);
    else if (elements[s->type].MenuVisible) tool = s->type;
  } else if (bmap[y / WCELL][x / WCELL] && bmap[y / WCELL][x / WCELL] < UI_WALLCOUNT) {
    tool = TOOL_WALLS + bmap[y / WCELL][x / WCELL];
  }
  if (tool >= 0) {
    ui.tool[0] = tool;
    ui.section = (uint8_t)tool_section(tool);
    menu_show_tool(tool);
  }
}

/* ---------------------------------------------------------------- the HUD */
/* what's under the brush, small: its name and temperature, the pressure when there is some (in the corner the
 * original's HUD has it); with "Details in the HUD", the original's: its FPS and particles' count, all the sample */
void hud_draw(Overlay *o) {
  char buf[96];
  char *p = buf, *end = buf + sizeof buf;
  int alpha = 255 - ui.intro * 5;
  if (ui.debug) {
    /* FPS and the particles' count (top left) */
    char f[16];
    fmt_fixed(f, ui.fps100 / 100.0f, 2);
    p = cat(p, end, "FPS: "), p = cat(p, end, f);
    p = cat(p, end, " Parts: "), p = cat_int(p, end, NUM_PARTS);
    if (replaceModeFlags & REPLACE_MODE) p = cat(p, end, " [REPLACE MODE]");
    if (replaceModeFlags & SPECIFIC_DELETE) p = cat(p, end, " [SPECIFIC DELETE]");
    int tw = font_text_width(buf) - 1, fps_right = SIM_X + 8 + tw + 4;
    ov_fill(o, SIM_X + 4, 4, tw + 8, FONT_H - 2 + 5, 0x000000, 127);
    ov_text(o, SIM_X + 8, 8, buf, 0x20D8FF, 191);
    if (alpha <= 0 || ui.mode == MODE_STKM || ui.mode == MODE_MENU) return;
    particle_info(buf, sizeof buf, ui.cx, ui.cy, false);
    tw = font_text_width(buf) - 1;
    int x = SIM_X + SIM_W - 8 - tw, y = 8;
    if (x - 4 < fps_right + 2) y += 15;
    if (x < SIM_X + 4) x = SIM_X + 4;
    ov_fill(o, x - 4, y - 4, tw + 8, 15, 0x000000, alpha / 2);
    ov_text(o, x, y, buf, 0xFFFFFF, alpha * 3 / 4);
    particle_info(buf, sizeof buf, ui.cx, ui.cy, true);
    tw = font_text_width(buf) - 1;
    x = SIM_X + SIM_W - 8 - tw;
    if (x < SIM_X + 4) x = SIM_X + 4;
    ov_fill(o, x - 4, y + 11, tw + 8, 14, 0x000000, alpha / 2);
    ov_text(o, x, y + 14, buf, 0xFFFFFF, alpha * 3 / 4);
    return;
  }
  /* (only while the brush is in use: not over the windows and menus) */
  if (alpha <= 0 || (ui.mode != MODE_SIM && ui.mode != MODE_SHAPE)) return;
  int id;
  const Particle *s = particle_at(ui.cx, ui.cy, &id);
  int wall = bmap[ui.cy / WCELL][ui.cx / WCELL];
  if (s) {
    if (s->type == PT_LAVA && IsElement(s->ctype)) p = cat(p, end, "molten "), p = cat(p, end, element_resolve(s->ctype, -1));
    else p = cat(p, end, element_resolve(s->type, s->ctype));
    char t[16];
    fmt_fixed(t, p_temp(s) - 273.15f, 0);
    p = cat(p, end, "  "), p = cat(p, end, t), p = cat(p, end, FONT_DEG "C");
  } else if (wall && wall < UI_WALLCOUNT) {
    p = cat(p, end, walltypes[wall].name);
  } else {
    return;   /* (nothing there) */
  }
  float pres = pv[ui.cy / ACELL][ui.cx / ACELL];
  if (fabsf(pres) >= 0.5f) {
    char t[16];
    fmt_fixed(t, pres, 1);
    p = cat(p, end, "  pressure "), p = cat(p, end, t);
  }
  int tw = font_text_width(buf) - 1, x = SIM_X + SIM_W - 6 - tw;
  ov_fill(o, x - 3, 3, tw + 6, 14, 0x000000, alpha / 2);
  ov_text(o, x, 7, buf, 0xFFFFFF, alpha * 7 / 8);
}

/* ---------------------------------------------------------------- a stickman's life */
/* with the cursor on a stickman's head, its life by the cursor (the original's renderer, under the mouse) */
void stickman_hp(Overlay *o) {
  if (!elementCount[PT_STKM] && !elementCount[PT_STKM2] && !elementCount[PT_FIGH]) return;
  for (int i = 0; i < parts_active; i++) {
    int t = parts[i].type;
    if (t != PT_STKM && t != PT_STKM2 && t != PT_FIGH) continue;
    int nx = p_cx(&parts[i]), ny = p_cy(&parts[i]), life = parts[i].life;
    if (!(ui.cx > nx - 3 && ui.cx < nx + 3 && ui.cy < ny + 3 && ui.cy > ny - 3)) continue;
    char hp[12], txt[16];
    fmt_int(hp, life);
    int pad = 3 - (int)strlen(hp), k = 0;   /* (Format::Width(life, 3)) */
    while (pad-- > 0) txt[k++] = ' ';
    strcpy(txt + k, hp);
    int mx = SIM_X + ui.cx * PX_SCALE + 1, my = ui.cy * PX_SCALE + 1;
    ov_text(o, mx - 8 - 2 * (life < 100) - 2 * (life < 10), my - 12, txt, 0xFFFFFF, 255);
    return;
  }
}

/* ---------------------------------------------------------------- the brush's outline */
typedef struct { int cx, cy; } BrushCtx;
static BrushCtx brush_ctx;
static inline void xor_px(Band *b, int x, int y) {
  if (x < SIM_X || y < 0 || x >= SIM_X + SIM_W || y >= SIM_H) return;
  int sx = x - b->x0, sy = y - b->y0;
  if ((unsigned)sx < (unsigned)b->w && (unsigned)sy < (unsigned)b->h) b->px[sy * b->w + sx] = px_xor(b->px[sy * b->w + sx]);
}
/* Brush::RenderPoint: the brush's edge, drawn on the outer pixels of its edge particles (not their middles) */
static void brush_outline_fn(Band *band, const void *ctx) {
  const BrushCtx *c = ctx;
  int rx = brush.rx, ry = brush.ry;
  int py0 = band->y0 / PX_SCALE - 1, py1 = (band->y0 + band->h) / PX_SCALE + 1;
  for (int dy = -ry; dy <= ry; dy++) {
    int y = c->cy + dy;
    if (y < py0 || y > py1) continue;
    for (int dx = -rx; dx <= rx; dx++) {
      if (!brush_has(dx, dy)) continue;
      bool top = dy > -ry && brush_has(dx, dy - 1), bottom = dy < ry && brush_has(dx, dy + 1);
      bool left = dx > -rx && brush_has(dx - 1, dy), right = dx < rx && brush_has(dx + 1, dy);
      if (top && bottom && left && right) continue;
      int bx = SIM_X + (c->cx + dx) * PX_SCALE, by = y * PX_SCALE;
      for (int j = 0; j < PX_SCALE; j++)
        for (int i = 0; i < PX_SCALE; i++) {
          bool on = (j == 0 && !top) || (j == PX_SCALE - 1 && !bottom) || (i == 0 && !left) || (i == PX_SCALE - 1 && !right);
          if (on && !(i == 1 && j == 1)) xor_px(band, bx + i, by + j);
        }
    }
  }
}
/* a particle's middle on the screen */
static int screen_x(int v) { return SIM_X + v * PX_SCALE + 1; }
static int screen_y(int v) { return v * PX_SCALE + 1; }
static void xor_box(Overlay *o, int X0, int Y0, int X1, int Y1) {
  ov_xor_line(o, X0, Y0, X1, Y0);
  ov_xor_line(o, X0, Y1, X1, Y1);
  ov_xor_line(o, X0, Y0 + 1, X0, Y1 - 1);
  ov_xor_line(o, X1, Y0 + 1, X1, Y1 - 1);
}
void brush_draw(Overlay *o) {
  int tool = ui.tool[ui.mode == MODE_SHAPE ? ui.shape_side : 0];
  if (ui.mode == MODE_SHAPE) {
    if (ui.shape == SHAPE_LINE) {
      ov_xor_line(o, screen_x(ui.sx), screen_y(ui.sy), screen_x(ui.cx), screen_y(ui.cy));
    } else if (ui.shape == SHAPE_BOX) {
      int x0 = ui.sx < ui.cx ? ui.sx : ui.cx, x1 = ui.sx < ui.cx ? ui.cx : ui.sx;
      int y0 = ui.sy < ui.cy ? ui.sy : ui.cy, y1 = ui.sy < ui.cy ? ui.cy : ui.sy;
      if (tool & TOOL_WALLS) x0 -= x0 % WCELL, y0 -= y0 % WCELL, x1 += WCELL - 1 - x1 % WCELL, y1 += WCELL - 1 - y1 % WCELL;
      xor_box(o, SIM_X + x0 * PX_SCALE, y0 * PX_SCALE, SIM_X + x1 * PX_SCALE + PX_SCALE - 1, y1 * PX_SCALE + PX_SCALE - 1);
      return;
    } else {
      /* Brush::RenderFill: a small cross */
      int x = screen_x(ui.cx), y = screen_y(ui.cy);
      ov_xor_line(o, x - 7, y, x - 2, y);
      ov_xor_line(o, x + 7, y, x + 2, y);
      ov_xor_line(o, x, y - 7, x, y - 2);
      ov_xor_line(o, x, y + 7, x, y + 2);
      return;
    }
  }
  if (tool & TOOL_WALLS) {
    /* a wall brush covers whole wall cells (GameView: NormaliseBlockCoord) */
    int wx = ui.cx / WCELL, wy = ui.cy / WCELL, rx = brush.rx / WCELL, ry = brush.ry / WCELL;
    xor_box(o, SIM_X + (wx - rx) * WCELL * PX_SCALE, (wy - ry) * WCELL * PX_SCALE,
            SIM_X + (wx + rx + 1) * WCELL * PX_SCALE - 1, (wy + ry + 1) * WCELL * PX_SCALE - 1);
    return;
  }
  brush_ctx = (BrushCtx){ui.cx, ui.cy};
  ov_call(o, brush_outline_fn, &brush_ctx);
}

/* ---------------------------------------------------------------- the panel below the sim */
/* a small brush: its shape at one pixel a particle (radius up to 6) */
typedef struct { int x, y; } BrushIconCtx;
static BrushIconCtx brush_icon;
static void brush_icon_fn(Band *b, const void *ctx) {
  const BrushIconCtx *c = ctx;
  int rx = brush.rx < 6 ? brush.rx : 6, ry = brush.ry < 6 ? brush.ry : 6;
  for (int dy = -ry; dy <= ry; dy++)
    for (int dx = -rx; dx <= rx; dx++) {
      /* (a bigger brush is shown by its shape at radius 6) */
      int sx = brush.rx > 6 ? dx * brush.rx / 6 : dx, sy = brush.ry > 6 ? dy * brush.ry / 6 : dy;
      if (brush_has(sx, sy)) band_fill(b, c->x + dx, c->y + dy, 1, 1, 0xFFFFFF, 255);
    }
}

/* a tool as the original's ToolButton (its colour, its name in black or white as it's light or dark), here as wide
 * as its name needs; NONE is "ERASE" (the walls' eraser "WALL ERASE"). Returns its width */
static int held_button(Overlay *o, int x, int y, int tool, uint32_t border) {
  uint32_t c = tool_colour(tool);
  const char *name = tool == PT_NONE ? "ERASE" : tool == (TOOL_WALLS | WL_ERASE) ? "WALL ERASE" : tool_name(tool);
  /* (a wall's name without its " WALL": LIQUID, GAS...) */
  char t[16];
  int n = (int)strlen(name), k = 0;
  if ((tool & TOOL_WALLS) && n > 5 && !strcmp(name + n - 5, " WALL")) n -= 5;
  while (k < n && k < 12) t[k] = name[k], k++;
  t[k] = 0;
  int tw = font_text_width(t) - 1, w = tw + 10 > 30 ? tw + 10 : 30, h = 18;
  ov_fill(o, x + 2, y + 2, w - 4, h - 4, c, 255);
  ov_rect(o, x, y, w, h, border, 255);
  ov_rect(o, x + 1, y + 1, w - 2, h - 2, 0x000000, 255);
  int total = (c & 255) + 3 * (c >> 8 & 255) + 2 * (c >> 16 & 255);
  ov_text(o, x + (w - tw) / 2, y + 5, t, total < 544 ? 0xFFFFFF : 0x000000, 255);
  return w;
}

/* the line of keys at the bottom: centred, the keys' names in the original's orange (\bo), the rest grey */
void keys_line(Overlay *o, const char *text) {
  ov_fill(o, 0, KEYS_Y - 4, SCREEN_W, 1, 0x404040, 255);
  ov_text(o, (SCREEN_W - font_text_width(text)) / 2, KEYS_Y, text, 0xC0C0C0, 255);
}

void panel_draw(Overlay *o) {
  if (ui.mode == MODE_MENU) {
    menu_row(o);
  } else {
    /* what OK and Back draw with */
    int y = PANEL_Y + 3, x = 6;
    ov_text(o, x, y + 5, "OK", 0xFFD820, 255);
    x += font_text_width("OK") + 3;
    x += held_button(o, x, y, ui.tool[0], 0xFF0000) + 10;
    ov_text(o, x, y + 5, "Back", 0xFFD820, 255);
    x += font_text_width("Back") + 3;
    x += held_button(o, x, y, ui.tool[1], 0x0000FF) + 12;
    /* the brush: its shape and size; while a shape is placed, which */
    if (ui.mode == MODE_SHAPE) {
      static const char *const names[NUM_SHAPES] = {"line", "box", "fill"};
      ov_text(o, x, y + 5, names[ui.shape], 0xFFFFFF, 255);
    } else {
      brush_icon = (BrushIconCtx){x + 6, y + 9};
      ov_call(o, brush_icon_fn, &brush_icon);
      char r[12] = "size ";
      fmt_int(r + 5, brush.rx);
      ov_text(o, x + 16, y + 5, r, 0xC0C0C0, 255);
    }
    /* paused: the original's pause button, pressed */
    if (sys_pause) {
      int w = font_text_width("paused") + 18, px = SCREEN_W - 4 - w;
      ov_fill(o, px, y + 1, w, 16, 0xFFFFFF, 255);
      ov_text(o, px + 3, y + 4, "\x90", 0x000000, 255);
      ov_text(o, px + 15, y + 5, "paused", 0x000000, 255);
    }
  }
  keys_line(o, mode_keys());
}

/* ---------------------------------------------------------------- the welcome card */
/* on a dark backdrop, a window as the original's: the name, where it comes from, the four keys to start */
void intro_draw(Overlay *o) {
  int a = ui.intro > 50 ? 255 : ui.intro * 5;
  ov_fill(o, 0, 0, SCREEN_W, SCREEN_H, 0x000000, a * 3 / 4);
  if (a < 128) return;
  int w = 236, h = 146, x = (SCREEN_W - w) / 2, y = (SCREEN_H - h) / 2 - 6;
  window_draw(o, x, y, w, h, NULL);
  ov_text_big(o, x + (w - 2 * font_text_width("NumPowder")) / 2, y + 14, "NumPowder", 0xFF4B4B);
  ov_text(o, x + (w - font_text_width("Inspired by The Powder Toy")) / 2, y + 40, "Inspired by The Powder Toy", 0xC0C0C0, 255);
  static const char *const keys[4][2] = {{"OK", "draws"}, {"Back", "erases"}, {"toolbox", "picks a material"},
                                         {"var", "opens the menu: help, saves"}};
  for (int k = 0; k < 4; k++) {
    int ky = y + 62 + k * 15;
    ov_text(o, x + 70 - font_text_width(keys[k][0]), ky, keys[k][0], 0xFFD820, 255);
    ov_text(o, x + 78, ky, keys[k][1], 0xFFFFFF, 255);
  }
  ov_text(o, x + (w - font_text_width("Press any key")) / 2, y + h - 18, "Press any key", 0x20AAFF, 255);
}
