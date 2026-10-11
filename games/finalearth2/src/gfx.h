/* The Final Earth 2: drawing.
 *
 * A frame is a draw list, rasterised band by band (the calculator has no
 * frame buffer) and pushed to the screen. Commands are in screen pixels and
 * painted in the order given. */
#ifndef FE_GFX_H
#define FE_GFX_H
#include <stdbool.h>
#include <stdint.h>
#include "plat.h"

static inline uint16_t rgb565(uint32_t rgb) {   /* 0xRRGGBB */
  return (uint16_t)(((rgb >> 8) & 0xF800) | ((rgb >> 5) & 0x07E0) | ((rgb >> 3) & 0x001F));
}
/* the nearest RGB565 colour (the sprites' colours are packed this way) */
static inline uint16_t rgb565_near(uint32_t rgb) {
  uint32_t r = rgb >> 16 & 255, g = rgb >> 8 & 255, b = rgb & 255;
  return (uint16_t)(((r * 31 + 127) / 255) << 11 | ((g * 63 + 127) / 255) << 5 | ((b * 31 + 127) / 255));
}

enum { GFX_FLIPX = 1 };
#define GFX_NO_TINT 0xFFFF

/* the memory the renderer keeps (gfx.c): the draw list, its text, clips and band functions
 * (7.1 KB on the calculator); the band is the caller's (gfx_set_band_memory) */
#define GFX_BAND 16           /* screen rows rasterised at a time */
#define GFX_MAX_ITEMS 256     /* commands in a frame (more are dropped and counted) */
#define GFX_TEXT_POOL 1024    /* bytes of text in a frame */

/* where gfx_end rasterises: SCREEN_W * GFX_BAND pixels, only used while gfx_end runs (on the
 * calculator gfx_end draws nothing until it is given; the computer's build has its own) */
void gfx_set_band_memory(uint16_t *px);
void gfx_begin(void);                    /* a new frame */
void gfx_end(void);                      /* rasterise it and push it */
/* the zoom of the commands that follow (1 or 2): a sprite pixel becomes z x z screen pixels;
 * positions stay in screen pixels */
void gfx_zoom(int z);
/* later commands draw only inside this screen rectangle (gfx_clip(0, 0, SCREEN_W, SCREEN_H): all) */
void gfx_clip(int x, int y, int w, int h);

void gfx_fill(uint16_t color);           /* the whole clip rectangle */
void gfx_rect(int x, int y, int w, int h, uint16_t color, uint8_t alpha);   /* alpha 0 (none) to 255 */
/* a sprite of the atlas (SPR_* in data.h) with its top-left at (x, y) */
void gfx_sprite(uint16_t spr, int x, int y, uint8_t flags, uint8_t alpha);
/* the part (sx, sy, w, h) of a sprite (a strip of frames, a building's variant...) */
void gfx_sprite_part(uint16_t spr, int sx, int sy, int w, int h, int x, int y, uint8_t flags, uint8_t alpha);
/* the same sprite at many places, tinted (citizens, grass, stars...) */
void gfx_sprite_tint(uint16_t spr, int x, int y, uint8_t flags, uint16_t tint);
/* all of it at once: a part, flipped, see-through and tinted (GFX_NO_TINT: none); the tint
 * multiplies each colour channel, as PIXI's tint does */
void gfx_sprite_ex(uint16_t spr, int sx, int sy, int w, int h, int x, int y, uint8_t flags, uint8_t alpha,
                   uint16_t tint);
/* frame i of a strip of frames fw pixels wide and 2 pixels apart (the original's
 * Resources.getTexturesByWidth(name, fw)), full height */
void gfx_frame(uint16_t spr, int fw, int i, int x, int y, uint8_t flags, uint8_t alpha);
int spr_w(uint16_t spr);
int spr_h(uint16_t spr);

/* text in one of the original's fonts (FONT_* in data.h), its top-left at (x, y); returns its width.
 * UTF-8; '\n' starts a new line. Letters are laid out as PIXI's BitmapText does (offsets,
 * advances, kerning), and scaled by the zoom. */
int gfx_text(uint8_t font, int x, int y, const char *s, uint16_t color);
int gfx_text_ex(uint8_t font, int x, int y, const char *s, uint16_t color, uint8_t alpha);
int gfx_text_width(uint8_t font, const char *s);     /* of its widest line, at zoom 1 */
int gfx_text_width_n(uint8_t font, const char *s, int n);   /* of its first n bytes (one line) */
/* the first line of s wrapped as PIXI's BitmapText wraps at max_w (0: no wrapping): returns its bytes,
 * sets its width and where the next line starts */
int gfx_text_wrap(uint8_t font, const char *s, int max_w, int *line_w, const char **next);
int gfx_font_height(uint8_t font);                   /* the line height */

/* Drawing of one's own (the sky's stars, the islands, the clouds): fn is called
 * for each band that rows y to y + h - 1 cross, in its place in the list, and
 * draws into the band. px[(sy - y0) * SCREEN_W + sx] is screen pixel (sx, sy),
 * for rows y0 to y1 - 1; draw only inside cx0..cx1 - 1, cy0..cy1 - 1 (the
 * clip rectangle in the band). zoom: gfx_zoom's when fn was added. Up to 64 a frame. */
typedef struct {
  uint16_t *px;
  int y0, y1;
  int cx0, cy0, cx1, cy1;
  int zoom;
} GfxBand;
typedef void (*GfxBandFn)(const GfxBand *b, void *ctx);
void gfx_call(GfxBandFn fn, void *ctx, int y, int h);
/* for band functions: a sprite (at b's zoom) or a row's span, drawn into the band now */
void gfx_band_sprite(const GfxBand *b, uint16_t spr, int sx, int sy, int w, int h, int x, int y, uint8_t flags,
                     uint8_t alpha, uint16_t tint);
void gfx_band_span(const GfxBand *b, int y, int x0, int x1, uint16_t color, uint8_t alpha);
/* n bytes of text drawn into the band now (rot: turned a quarter clockwise, its top-left at (x, y)) */
void gfx_band_text(const GfxBand *b, uint8_t font, int x, int y, const char *s, int n, uint16_t color, uint8_t alpha,
                   bool rot);

/* fg over bg with alpha a (0 to 32) */
static inline uint16_t gfx_blend(uint16_t fg, uint16_t bg, uint32_t a) {
  uint32_t f = (fg | (uint32_t)fg << 16) & 0x07E0F81Fu, g = (bg | (uint32_t)bg << 16) & 0x07E0F81Fu;
  uint32_t r = ((f * a + g * (32 - a) + 0x02008010u) >> 5) & 0x07E0F81Fu;
  return (uint16_t)(r | r >> 16);
}
/* alpha 0..255 as gfx_blend's 0..32 */
static inline uint32_t gfx_alpha32(uint32_t a) { return (a * 32 + 127) / 255; }

/* the last frame: commands drawn, and commands lost because the list or the text pool was full */
int gfx_items(void);
int gfx_dropped(void);
#endif
