/* NumPowder: text in The Powder Toy's font (FontReader.cpp; RasterDrawMethodsImpl.h's BlendChar, AddChar,
 * BlendText, TextSize, TextFit), drawn into a band of the screen. */
#include "font.h"

static uint32_t code_colour(uint8_t c, uint32_t cur, bool *underline) {
  switch (c) {
    case 'U': *underline = !*underline; return cur;
    case 'w': return 0xFFFFFF;
    case 'g': return 0xC0C0C0;
    case 'o': return 0xFFD820;
    case 'r': return 0xFF0000;
    case 'l': return 0xFF4B4B;
    case 'b': return 0x0000FF;
    case 't': return 0x20AAFF;
    case 'u': return 0x9353D3;
  }
  return cur;
}

int font_draw_char(Band *b, int x, int y, uint8_t c, uint32_t rgb, int alpha, int mode) {
  int g = font_glyph(c), w = font_widths[g];
  const uint8_t *p = font_bits + font_offsets[g];
  int top = y - 2;
  /* (the whole glyph is skipped when it misses the band) */
  if (top >= b->y0 + b->h || top + FONT_H <= b->y0 || x >= b->x0 + b->w || x + w <= b->x0) return w;
  /* (only the glyph's rows in the band) */
  int r0 = b->y0 - top, r1 = b->y0 + b->h - top;
  if (r0 < 0) r0 = 0;
  if (r1 > FONT_H) r1 = FONT_H;
  int levels[4] = {0, alpha / 3, 2 * alpha / 3, alpha};
  for (int r = r0; r < r1; r++) {
    int sy = top + r - b->y0, bit = r * w;
    uint16_t *row = b->px + sy * b->w;
    for (int k = 0; k < w; k++, bit++) {
      int a = (p[bit >> 2] >> ((bit & 3) * 2)) & 3;
      if (!a) continue;
      int sx = x + k - b->x0;
      if ((unsigned)sx >= (unsigned)b->w) continue;
      row[sx] = mode == FONT_ADD ? px_add(row[sx], rgb, levels[a]) : px_blend(row[sx], rgb, levels[a]);
    }
  }
  return w;
}

int font_draw(Band *b, int x, int y, const char *s, uint32_t rgb, int alpha, int mode) {
  const uint8_t *t = (const uint8_t *)s;
  uint32_t col = rgb;
  bool underline = false;
  int px = x;
  for (; *t; t++) {
    uint8_t c = *t;
    /* (a line below the band: nothing more to draw in it; above it, its underline too: only its colour codes to
     * follow. A glyph covers FONT_H rows from y - 2, the underline is row y + FONT_H) */
    if (y - 2 >= b->y0 + b->h) break;
    bool above = y + FONT_H < b->y0;
    if (c == '\n') {
      px = x, y += FONT_H;
    } else if (above && c >= 0x20) {
      px += font_char_width(c);
    } else if (c == 0x0F) {
      if (!t[1] || !t[2] || !t[3]) break;
      col = (uint32_t)t[1] << 16 | (uint32_t)t[2] << 8 | t[3];
      t += 3;
    } else if (c == 0x0E) {
      col = rgb;
    } else if (c == 0x01) {
      col ^= 0xFFFFFF;
    } else if (c == '\b') {
      if (!t[1]) break;
      col = code_colour(t[1], col, &underline);
      t++;
    } else {
      int dx = font_draw_char(b, px, y, c, col, alpha, mode);
      if (underline) {
        int sy = y + FONT_H - b->y0;
        for (int k = 0; k < dx; k++) {
          int sx = px + k - b->x0;
          if (sy >= 0 && sy < b->h && sx >= 0 && sx < b->w) b->px[sy * b->w + sx] = px_blend(b->px[sy * b->w + sx], col, alpha);
        }
      }
      px += dx;
    }
  }
  return px - x;
}

void font_draw_outline(Band *b, int x, int y, const char *s, uint32_t rgb, int alpha) {
  font_draw(b, x - 1, y - 1, s, 0, 0x78, FONT_BLEND);
  font_draw(b, x - 1, y + 1, s, 0, 0x78, FONT_BLEND);
  font_draw(b, x + 1, y - 1, s, 0, 0x78, FONT_BLEND);
  font_draw(b, x + 1, y + 1, s, 0, 0x78, FONT_BLEND);
  font_draw(b, x, y, s, rgb, alpha, FONT_BLEND);
}

int font_text_width(const char *s) {
  const uint8_t *t = (const uint8_t *)s;
  int w = 0, cur = 0;
  for (; *t; t++) {
    if (*t == '\n') {
      if (cur > w) w = cur;
      cur = 0;
    } else if (*t == 0x0F) {
      if (!t[1] || !t[2] || !t[3]) break;
      t += 3;
    } else if (*t == 0x0E || *t == 0x01) {
    } else if (*t == '\b') {
      if (!t[1]) break;
      t++;
    } else {
      cur += font_char_width(*t);
    }
  }
  return cur > w ? cur : w;
}

int font_text_lines(const char *s) {
  int n = 1;
  for (; *s; s++) n += *s == '\n';
  return n;
}

int font_text_fit(const char *s, int width) {
  const uint8_t *t = (const uint8_t *)s;
  int cur = 0, i = 0;
  for (; t[i]; i++) {
    if (t[i] == '\n') {
      cur = 0;
    } else if (t[i] == 0x0F) {
      if (!t[i + 1] || !t[i + 2] || !t[i + 3]) break;
      i += 3;
    } else if (t[i] == 0x0E || t[i] == 0x01) {
    } else if (t[i] == '\b') {
      if (!t[i + 1]) break;
      i++;
    } else {
      int dx = font_char_width(t[i]);
      if (cur + dx / 2 >= width) return i;
      cur += dx;
    }
  }
  return i;
}

int font_draw_big(Band *b, int x, int y, const char *s, uint32_t rgb) {
  int px = x, top = y - 4;
  uint16_t c565 = rgb565(rgb);
  for (const uint8_t *t = (const uint8_t *)s; *t; t++) {
    int g = font_glyph(*t), w = font_widths[g];
    const uint8_t *p = font_bits + font_offsets[g];
    if (top < b->y0 + b->h && top + 2 * FONT_H > b->y0)
      for (int r = 0; r < FONT_H; r++)
        for (int k = 0; k < w; k++) {
          int bit = r * w + k, a = (p[bit >> 2] >> ((bit & 3) * 2)) & 3;
          if (!a) continue;
          for (int dy = 0; dy < 2; dy++)
            for (int dx = 0; dx < 2; dx++) {
              int sx = px + 2 * k + dx - b->x0, sy = top + 2 * r + dy - b->y0;
              if ((unsigned)sx >= (unsigned)b->w || (unsigned)sy >= (unsigned)b->h) continue;
              uint16_t *d = &b->px[sy * b->w + sx];
              *d = a == 3 ? c565 : px_blend(*d, rgb, a * 85);
            }
        }
    px += 2 * w;
  }
  return px - x;
}
