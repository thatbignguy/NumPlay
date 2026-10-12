/* NumPowder: The Powder Toy's font and text drawing (FontReader.cpp, RasterDrawMethodsImpl.h's BlendText).
 *
 * Text is one byte per character: ASCII from 0x20, FONT_DEG (0x7F) the degree sign, 0x80 + n the icon glyph
 * U+E000 + n (FONT_ICON(0x41) is the walls' section icon, U+E041), FONT_DOT the middle dot and the keypad's symbols
 * FONT_LEFT, FONT_RIGHT, FONT_UP, FONT_DOWN (the arrow keys) and FONT_DEL (the delete key). As in the original:
 * - '\n' starts a new line FONT_H pixels lower;
 * - "\x0F" then three bytes r, g, b sets the colour (a 0 byte ends the string: write 1 for it);
 * - "\x0E" goes back to the colour given; "\x01" inverts the colour;
 * - '\b' then a letter picks a colour: w white, g grey, o orange, r red, l light red, b blue, t teal,
 *   u purple; "\bU" underlines.
 * A glyph covers FONT_H rows from 2 pixels above the y given (the original's offset of -2). */
#ifndef NP_FONT_H
#define NP_FONT_H
#include <stdbool.h>
#include <stdint.h>

#define FONT_H 12
#define FONT_GLYPHS 214
#define FONT_DEG "\x7F"
#define FONT_ICON(n) (0x80 + (n))
#define FONT_DOT "\xEF"
#define FONT_LEFT "\xF0"
#define FONT_RIGHT "\xF1"
#define FONT_UP "\xF2"
#define FONT_DOWN "\xF3"
#define FONT_DEL "\xF4"
extern const uint8_t font_widths[FONT_GLYPHS];
extern const uint16_t font_offsets[FONT_GLYPHS];
extern const uint8_t font_bits[];

/* a screen band: w x h RGB565 pixels covering the screen from (x0, y0); drawing is clipped to it */
typedef struct {
  uint16_t *px;
  int x0, y0, w, h;
} Band;

static inline int font_glyph(uint8_t c) {
  return c >= 0x20 && c <= 0xF4 ? c - 0x20 : FONT_GLYPHS - 1;
}
static inline int font_char_width(uint8_t c) { return font_widths[font_glyph(c)]; }
/* the size of a text as the original's TextSize: width of the widest line, height FONT_H - 2 per line */
int font_text_width(const char *s);
int font_text_lines(const char *s);
/* how many bytes of s fit in width (TextFit: a character fits if half of it does) */
int font_text_fit(const char *s, int width);

/* the colour and blending of a text */
enum { FONT_BLEND, FONT_ADD };
/* BlendText (or AddChar for FONT_ADD): returns the width drawn on the last line */
int font_draw(Band *b, int x, int y, const char *s, uint32_t rgb, int alpha, int mode);
/* BlendTextOutline: the text over a dark outline */
void font_draw_outline(Band *b, int x, int y, const char *s, uint32_t rgb, int alpha);
/* one glyph */
int font_draw_char(Band *b, int x, int y, uint8_t c, uint32_t rgb, int alpha, int mode);
/* a text at twice the size (titles), without colour codes: returns its width */
int font_draw_big(Band *b, int x, int y, const char *s, uint32_t rgb);

/* colours: 0xRRGGBB to RGB565 and back */
static inline uint16_t rgb565(uint32_t c) {
  return (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F));
}
static inline uint32_t rgb888(uint16_t c) {
  uint32_t r = c >> 11, g = (c >> 5) & 63, b = c & 31;
  return (r << 3 | r >> 2) << 16 | (g << 2 | g >> 4) << 8 | (b << 3 | b >> 2);
}
/* a pixel of a band: blended (the original's RGB::Blend), added (RGB::Add) */
static inline uint16_t px_blend(uint16_t d, uint32_t c, int a) {
  if (a >= 255) return rgb565(c);
  if (a <= 0) return d;
  /* (in RGB565, 32 levels of alpha: the three channels at once, spread over 32 bits) */
  uint32_t a5 = (uint32_t)(a + 4) >> 3, s = rgb565(c);
  uint32_t dd = (d | (uint32_t)d << 16) & 0x07E0F81Fu, ss = (s | s << 16) & 0x07E0F81Fu;
  uint32_t r = ((ss * a5 + dd * (32 - a5)) >> 5) & 0x07E0F81Fu;
  return (uint16_t)(r | r >> 16);
}
static inline uint16_t px_add(uint16_t d, uint32_t c, int a) {
  uint32_t o = rgb888(d);
  uint32_t r = (o >> 16 & 255) + a * (c >> 16 & 255) / 255, g = (o >> 8 & 255) + a * (c >> 8 & 255) / 255;
  uint32_t bl = (o & 255) + a * (c & 255) / 255;
  return rgb565((r > 255 ? 255 : r) << 16 | (g > 255 ? 255 : g) << 8 | (bl > 255 ? 255 : bl));
}
#endif
