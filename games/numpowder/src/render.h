/* NumPowder: The Powder Toy's renderer (Renderer.cpp, ElementGraphics.h, RendererSettings.h), for the calculator.
 *
 * The original draws the simulation one pixel per particle into a frame, then the interface over it. There is no
 * frame in RAM here: the sim area is drawn in bands of 4 particle rows (the fire glow's cells), each into a small
 * buffer of 24-bit pixels as the original's (so its blending and glows add up the same), then shown 3 x 3 pixels
 * a particle in bands of 6 screen rows, the walls' patterns and the interface's overlay (render_ovl.c) drawn over
 * them, and pushed to the screen. */
#ifndef NP_RENDER_H
#define NP_RENDER_H
#include "font.h"
#include <stddef.h>
#include "plat.h"
#include "simpriv.h"   /* (restrict_flt, clamp_flt, the elements' shared functions) */

/* ---------------------------------------------------------------- ElementGraphics.h */
#define PMODE           0x00000FFF
#define PMODE_NONE      0x00000000
#define PMODE_FLAT      0x00000001
#define PMODE_BLOB      0x00000002
#define PMODE_BLUR      0x00000004
#define PMODE_GLOW      0x00000008
#define PMODE_SPARK     0x00000010
#define PMODE_FLARE     0x00000020
#define PMODE_LFLARE    0x00000040
#define PMODE_ADD       0x00000080
#define PMODE_BLEND     0x00000100
#define PSPEC_STICKMAN  0x00000200
#define OPTIONS         0x0000F000
#define NO_DECO         0x00001000
#define DECO_FIRE       0x00002000
#define FIREMODE        0x00FF0000
#define FIRE_ADD        0x00010000
#define FIRE_BLEND      0x00020000
#define FIRE_SPARK      0x00040000
#define EFFECT          0xFF000000u
#define EFFECT_GRAVIN   0x01000000
#define EFFECT_GRAVOUT  0x02000000
#define EFFECT_LINES    0x04000000
#define EFFECT_DBGLINES 0x08000000
#define RENDER_EFFE (OPTIONS | PSPEC_STICKMAN | EFFECT | PMODE_SPARK | PMODE_FLARE | PMODE_LFLARE)
#define RENDER_FIRE (OPTIONS | PSPEC_STICKMAN | PMODE_ADD | PMODE_BLEND | FIRE_ADD | FIRE_BLEND)
#define RENDER_SPRK (OPTIONS | PSPEC_STICKMAN | PMODE_ADD | PMODE_BLEND | FIRE_SPARK)
#define RENDER_GLOW (OPTIONS | PSPEC_STICKMAN | PMODE_GLOW | PMODE_ADD | PMODE_BLEND)
#define RENDER_BLUR (OPTIONS | PSPEC_STICKMAN | PMODE_BLUR | PMODE_ADD | PMODE_BLEND)
#define RENDER_BLOB (OPTIONS | PSPEC_STICKMAN | PMODE_BLOB | PMODE_ADD | PMODE_BLEND)
#define RENDER_BASC (OPTIONS | PSPEC_STICKMAN | PMODE_FLAT | PMODE_ADD | PMODE_BLEND | EFFECT_LINES)
#define RENDER_NONE (OPTIONS | PSPEC_STICKMAN | PMODE_FLAT)
#define COLOUR_HEAT 0x00000001
#define COLOUR_LIFE 0x00000002
#define COLOUR_GRAD 0x00000004
#define COLOUR_BASC 0x00000008
#define COLOUR_DEST 0x00000010
#define COLOUR_DEFAULT 0x00000000
#define DISPLAY_AIRC 0x00000001
#define DISPLAY_AIRP 0x00000002
#define DISPLAY_AIRV 0x00000004
#define DISPLAY_AIRH 0x00000008
#define DISPLAY_AIRW 0x00000080
#define DISPLAY_AIR  0x0000008F
#define DISPLAY_WARP 0x00000010
#define DISPLAY_PERS 0x00000020
#define DISPLAY_EFFE 0x00000040

/* ---------------------------------------------------------------- RendererSettings */
typedef struct {
  uint32_t render_mode, display_mode, colour_mode;
  bool grav_field;     /* gravityFieldEnabled: Newtonian gravity's field lines ("G") */
  bool decorations;    /* decorationLevel: decorationEnabled or decorationDisabled ("D") */
  bool hdisp_auto;     /* wantHdispLimitMin and Max HdispLimitAuto (Dynamic Heat Display), else MIN_TEMP, MAX_TEMP */
} RenderSettings;
extern RenderSettings ren;

/* the original's display presets (Renderer::renderModePresets), in the order of its render options' buttons */
typedef struct {
  const char *name;
  uint32_t render_mode, display_mode, colour_mode;
  bool hdisp_auto;
} RenderPreset;
enum { PRESET_ALTAIR, PRESET_VELOCITY, PRESET_PRESSURE, PRESET_PERSISTENT, PRESET_FIRE, PRESET_BLOB, PRESET_HEAT,
       PRESET_FANCY, PRESET_NOTHING, PRESET_HEATGRAD, PRESET_LIFEGRAD, PRESET_DYNHEAT, PRESET_VORTICITY, PRESET_DEST,
       PRESET_COUNT };
extern const RenderPreset render_presets[PRESET_COUNT];
int render_preset_now(void);           /* the preset the settings match, -1 none */
void render_apply(uint32_t render_mode, uint32_t display_mode, uint32_t colour_mode);   /* ApplySettings */
void render_apply_preset(int preset);   /* LoadRenderPreset */
void render_clear_accumulation(void);  /* ClearAccumulation: the fire glow and the persistent trails */
void render_init(void);

/* ---------------------------------------------------------------- the elements' graphics (render_el*.c) */
/* random numbers for the graphics functions (the original's gfctx.rng): the same for a particle all frame */
uint32_t ren_rng_gen(void);
int ren_rng_between(int lower, int upper);
bool ren_rng_chance(int numerator, unsigned denominator);
/* the original's colour tables (Renderer::PopulateTables), as 0xRRGGBB */
uint32_t flame_table_at(int i);
uint32_t plasma_table_at(int i);
uint32_t heat_table_at(int i);
uint32_t clfm_table_at(int i);
uint32_t firw_table_at(int i);
/* a particle's decoration colour (0xAARRGGBB, 0 none): decorations aren't kept, unless the core keeps some */
uint32_t part_dcolour(const Particle *p);
/* the pixel modes an element's graphics function may give, besides PMODE_FLAT (for which bands it reaches) */
extern const uint32_t el_graphics_modes[PT_COUNT];
/* an element's graphics function as the original has it: its own, else Element::defaultGraphics (the elements
 * whose files set none, DUST, SAWD and DRAY, have none) */
typedef int (*GraphicsFn)(GRAPHICS_FUNC_ARGS);
int default_graphics(GRAPHICS_FUNC_ARGS);
static inline GraphicsFn element_graphics(int t) {
  if (elements[t].Graphics) return elements[t].Graphics;
  return t == PT_DUST || t == PT_SAWD || t == PT_DRAY ? NULL : default_graphics;
}
/* the shared ones (elements_data.c names them) */
int Element_BIZR_graphics(GRAPHICS_FUNC_ARGS);
int Element_COAL_graphics(GRAPHICS_FUNC_ARGS);
int Element_PIPE_graphics(GRAPHICS_FUNC_ARGS);
int Element_QRTZ_graphics(GRAPHICS_FUNC_ARGS);
int Element_STKM_graphics(GRAPHICS_FUNC_ARGS);
int Element_VIBR_graphics(GRAPHICS_FUNC_ARGS);

/* ---------------------------------------------------------------- the overlay: the interface over the sim */
/* a list of things to draw, made once a frame and drawn into each band they cross */
enum { OV_FILL, OV_RECT, OV_TEXT, OV_TEXT_OUTLINE, OV_TEXT_ADD, OV_TEXT_BIG, OV_XOR_LINE, OV_LINE, OV_CALL, OV_XOR_RECT };
typedef void (*OvFn)(Band *b, const void *ctx);
typedef struct {
  uint8_t kind, alpha;
  int16_t x, y, w, h;      /* (lines: x, y to w, h) */
  uint32_t rgb;            /* (text: its offset in the pool; call: the context's index) */
  uint16_t text;
} OvOp;
#define OV_MAX 128
#define OV_POOL 1024
#define OV_CALLS 12
typedef struct {
  int n, ntext, ncalls;
  bool full;
  OvOp op[OV_MAX];
  char pool[OV_POOL];
  OvFn fn[OV_CALLS];
  const void *ctx[OV_CALLS];
} Overlay;
void ov_reset(Overlay *o);
void ov_fill(Overlay *o, int x, int y, int w, int h, uint32_t rgb, int alpha);      /* BlendFilledRect */
void ov_rect(Overlay *o, int x, int y, int w, int h, uint32_t rgb, int alpha);      /* BlendRect: 1 pixel */
void ov_frame(Overlay *o, int x, int y, int w, int h, int thick, uint32_t rgb);     /* a frame `thick` wide, outside */
void ov_text(Overlay *o, int x, int y, const char *s, uint32_t rgb, int alpha);     /* BlendText */
void ov_text_outline(Overlay *o, int x, int y, const char *s, uint32_t rgb, int alpha);
void ov_text_add(Overlay *o, int x, int y, const char *s, uint32_t rgb, int alpha); /* AddChar */
void ov_text_big(Overlay *o, int x, int y, const char *s, uint32_t rgb);             /* twice the size */
void ov_line(Overlay *o, int x1, int y1, int x2, int y2, uint32_t rgb, int alpha);  /* BlendLine */
void ov_xor_line(Overlay *o, int x1, int y1, int x2, int y2);                       /* XorLine */
void ov_xor_rect(Overlay *o, int x, int y, int w, int h);                            /* (dotted) XorDottedRect */
void ov_call(Overlay *o, OvFn fn, const void *ctx);
void ov_draw(const Overlay *o, Band *b);
/* a pixel as the original's XorPixel: light grey over dark, dark grey over light */
static inline uint16_t px_xor(uint16_t d) {
  uint32_t c = rgb888(d);
  return 2 * (c >> 16 & 255) + 3 * (c >> 8 & 255) + (c & 255) < 512 ? rgb565(0xC0C0C0) : rgb565(0x404040);
}
void band_fill(Band *b, int x, int y, int w, int h, uint32_t rgb, int alpha);
void band_line(Band *b, int x1, int y1, int x2, int y2, uint32_t rgb, int alpha, bool xor);

/* ---------------------------------------------------------------- a frame */
/* the sim's area (in the middle of the screen's width, at its top) and the bottom panel below it */
#define SIM_W (XRES * PX_SCALE)
#define SIM_H (YRES * PX_SCALE)
#define SIM_X ((SCREEN_W - SIM_W) / 2)
_Static_assert(SIM_X % 2 == 0, "the sim's rows are written a word at a time");
/* draws a frame: the sim's rows always (the overlay over them; up to sim_push_w, unless `panel`: an unchanging window
 * at their right isn't sent again), the bottom panel when `panel` (it doesn't change often) */
void render_frame(const Overlay *o, bool panel, int sim_push_w);   /* (sim_push_w even) */
/* the time the last frame's parts took, ms (for the speed tests) */
extern uint32_t render_ms_parts, render_ms_total;
extern int render_found;   /* particles drawn last frame */
#endif
