/* NumPowder: the overlay, the interface drawn over the sim and around it (the original's Graphics calls in
 * GameView::OnDraw and its components' Draw): a list made once a frame, drawn into every band it crosses. */
#include "render.h"
#include <string.h>

void ov_reset(Overlay *o) { o->n = o->ntext = o->ncalls = 0, o->full = false; }

bool ov_overflowed;   /* (tests: a frame had more to draw than the list holds) */
static OvOp *ov_add(Overlay *o, int kind, int x, int y, int w, int h, uint32_t rgb, int alpha) {
  if (o->n >= OV_MAX) {
    o->full = ov_overflowed = true;
    return NULL;
  }
  OvOp *op = &o->op[o->n++];
  op->kind = (uint8_t)kind, op->alpha = (uint8_t)(alpha < 0 ? 0 : alpha > 255 ? 255 : alpha);
  op->x = (int16_t)x, op->y = (int16_t)y, op->w = (int16_t)w, op->h = (int16_t)h, op->rgb = rgb, op->text = 0;
  return op;
}

void ov_fill(Overlay *o, int x, int y, int w, int h, uint32_t rgb, int alpha) {
  if (w > 0 && h > 0) ov_add(o, OV_FILL, x, y, w, h, rgb, alpha);
}
void ov_rect(Overlay *o, int x, int y, int w, int h, uint32_t rgb, int alpha) {
  if (w > 0 && h > 0) ov_add(o, OV_RECT, x, y, w, h, rgb, alpha);
}
void ov_frame(Overlay *o, int x, int y, int w, int h, int thick, uint32_t rgb) {
  for (int k = 1; k <= thick; k++) ov_rect(o, x - k, y - k, w + 2 * k, h + 2 * k, rgb, 255);
}
static void ov_text_kind(Overlay *o, int kind, int x, int y, const char *s, uint32_t rgb, int alpha) {
  int n = (int)strlen(s) + 1;
  if (o->ntext + n > OV_POOL) {
    o->full = ov_overflowed = true;
    return;
  }
  OvOp *op = ov_add(o, kind, x, y, 0, 0, rgb, alpha);
  if (!op) return;
  memcpy(o->pool + o->ntext, s, (size_t)n);
  op->text = (uint16_t)o->ntext;
  /* (its size, to skip the bands it misses) */
  op->w = (int16_t)(font_text_width(s) + 2);
  op->h = (int16_t)(font_text_lines(s) * FONT_H);
  o->ntext += n;
}
void ov_text(Overlay *o, int x, int y, const char *s, uint32_t rgb, int alpha) { ov_text_kind(o, OV_TEXT, x, y, s, rgb, alpha); }
void ov_text_outline(Overlay *o, int x, int y, const char *s, uint32_t rgb, int alpha) {
  ov_text_kind(o, OV_TEXT_OUTLINE, x, y, s, rgb, alpha);
}
void ov_text_add(Overlay *o, int x, int y, const char *s, uint32_t rgb, int alpha) { ov_text_kind(o, OV_TEXT_ADD, x, y, s, rgb, alpha); }
void ov_text_big(Overlay *o, int x, int y, const char *s, uint32_t rgb) {
  int n = o->n;
  ov_text_kind(o, OV_TEXT_BIG, x, y - 2, s, rgb, 255);
  if (o->n > n) o->op[n].y = (int16_t)y, o->op[n].w *= 2, o->op[n].h *= 2;
}
void ov_line(Overlay *o, int x1, int y1, int x2, int y2, uint32_t rgb, int alpha) { ov_add(o, OV_LINE, x1, y1, x2, y2, rgb, alpha); }
void ov_xor_line(Overlay *o, int x1, int y1, int x2, int y2) { ov_add(o, OV_XOR_LINE, x1, y1, x2, y2, 0, 255); }
void ov_xor_rect(Overlay *o, int x, int y, int w, int h) { ov_add(o, OV_XOR_RECT, x, y, w, h, 0, 255); }
void ov_call(Overlay *o, OvFn fn, const void *ctx) {
  if (o->ncalls >= OV_CALLS) {
    o->full = true;
    return;
  }
  OvOp *op = ov_add(o, OV_CALL, 0, 0, 0, 0, (uint32_t)o->ncalls, 255);
  if (!op) return;
  o->fn[o->ncalls] = fn, o->ctx[o->ncalls] = ctx;
  o->ncalls++;
}

/* ---------------------------------------------------------------- drawing into a band */
void band_fill(Band *b, int x, int y, int w, int h, uint32_t rgb, int alpha) {
  int x0 = x < b->x0 ? b->x0 : x, y0 = y < b->y0 ? b->y0 : y;
  int x1 = x + w > b->x0 + b->w ? b->x0 + b->w : x + w, y1 = y + h > b->y0 + b->h ? b->y0 + b->h : y + h;
  if (x0 >= x1 || y0 >= y1) return;
  uint16_t c = rgb565(rgb);
  /* black over at half or a quarter of what's there (px_blend's 32 levels: 16 or 24 of black): each channel shifted,
   * two pixels a word */
  int keep = 32 - ((alpha + 4) >> 3);
  if (!(rgb & 0xFFFFFF) && alpha > 0 && alpha < 255 && (keep == 16 || keep == 8)) {
    int sh = keep == 16 ? 1 : 2;
    uint32_t mask = sh == 1 ? 0x7BEF7BEFu : 0x39E739E7u;
    for (int yy = y0; yy < y1; yy++) {
      uint16_t *d = b->px + (yy - b->y0) * b->w + (x0 - b->x0), *e = d + (x1 - x0);
      if (((uintptr_t)d & 2) && d < e) *d = (uint16_t)((*d >> sh) & mask), d++;
      for (; e - d >= 2; d += 2) *(uint32_t *)(void *)d = (*(uint32_t *)(void *)d >> sh) & mask;
      if (d < e) *d = (uint16_t)((*d >> sh) & mask);
    }
    return;
  }
  for (int yy = y0; yy < y1; yy++) {
    uint16_t *d = b->px + (yy - b->y0) * b->w + (x0 - b->x0);
    if (alpha >= 255)
      for (int xx = x0; xx < x1; xx++) *d++ = c;
    else
      for (int xx = x0; xx < x1; xx++, d++) *d = px_blend(*d, rgb, alpha);
  }
}

static inline void band_px(Band *b, int x, int y, uint32_t rgb, int alpha, bool xor) {
  int sx = x - b->x0, sy = y - b->y0;
  if ((unsigned)sx >= (unsigned)b->w || (unsigned)sy >= (unsigned)b->h) return;
  uint16_t *d = &b->px[sy * b->w + sx];
  *d = xor ? px_xor(*d) : px_blend(*d, rgb, alpha);
}

void band_line(Band *b, int x1, int y1, int x2, int y2, uint32_t rgb, int alpha, bool xor) {
  if ((y1 < b->y0 && y2 < b->y0) || (y1 >= b->y0 + b->h && y2 >= b->y0 + b->h)) return;
  int dx = abs(x2 - x1), dy = abs(y2 - y1), sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1, err = dx - dy;
  for (;;) {
    band_px(b, x1, y1, rgb, alpha, xor);
    if (x1 == x2 && y1 == y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) err -= dy, x1 += sx;
    if (e2 < dx) err += dx, y1 += sy;
  }
}

void ov_draw(const Overlay *o, Band *b) {
  for (int k = 0; k < o->n; k++) {
    const OvOp *op = &o->op[k];
    if (op->kind == OV_CALL) {
      o->fn[op->rgb](b, o->ctx[op->rgb]);
      continue;
    }
    if (op->kind == OV_LINE || op->kind == OV_XOR_LINE) {
      band_line(b, op->x, op->y, op->w, op->h, op->rgb, op->alpha, op->kind == OV_XOR_LINE);
      continue;
    }
    /* (boxes and texts that miss the band) */
    int top = op->kind == OV_TEXT_BIG ? op->y - 5 : op->kind >= OV_TEXT && op->kind <= OV_TEXT_ADD ? op->y - 3 : op->y;
    if (top >= b->y0 + b->h || op->y + op->h + 1 <= b->y0) continue;
    switch (op->kind) {
      case OV_FILL: band_fill(b, op->x, op->y, op->w, op->h, op->rgb, op->alpha); break;
      case OV_RECT:
        band_fill(b, op->x, op->y, op->w, 1, op->rgb, op->alpha);
        if (op->h > 1) band_fill(b, op->x, op->y + op->h - 1, op->w, 1, op->rgb, op->alpha);
        if (op->h > 2) {
          band_fill(b, op->x, op->y + 1, 1, op->h - 2, op->rgb, op->alpha);
          if (op->w > 1) band_fill(b, op->x + op->w - 1, op->y + 1, 1, op->h - 2, op->rgb, op->alpha);
        }
        break;
      case OV_XOR_RECT:
        /* RasterizeDottedRect: every other pixel of the edges */
        for (int x = op->x; x < op->x + op->w; x += 2) {
          band_px(b, x, op->y, 0, 255, true);
          band_px(b, x, op->y + op->h - 1, 0, 255, true);
        }
        for (int y = op->y + 2; y < op->y + op->h - 1; y += 2) {
          band_px(b, op->x, y, 0, 255, true);
          band_px(b, op->x + op->w - 1, y, 0, 255, true);
        }
        break;
      case OV_TEXT: font_draw(b, op->x, op->y, o->pool + op->text, op->rgb, op->alpha, FONT_BLEND); break;
      case OV_TEXT_ADD: font_draw(b, op->x, op->y, o->pool + op->text, op->rgb, op->alpha, FONT_ADD); break;
      case OV_TEXT_OUTLINE: font_draw_outline(b, op->x, op->y, o->pool + op->text, op->rgb, op->alpha); break;
      case OV_TEXT_BIG: font_draw_big(b, op->x, op->y, o->pool + op->text, op->rgb); break;
    }
  }
}
