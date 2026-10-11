/* Drawing tests: renders scenes with gfx.c and bg.c and saves them as images.
 *
 *   render [DIR]    (default build/shots)
 *
 * Writes DIR/a_title_night, b_title_midday, c_city, d_fonts and e_city_zoom2, each as .ppm and
 * .png, and prints what each frame took on this computer. Scenes follow the
 * original's stories/displayCity.json (the title screen's city: theLostShip's
 * island, seed 1000) with a few buildings added for the city. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "../src/bg.h"
#include "../src/data.h"
#include "../src/gfx.h"

extern uint16_t host_fb[SCREEN_W * SCREEN_H];
void host_shot(const char *path);

/* ---------------------------------------------------------------- PNG (stored, no compression) */
static uint32_t crc_table[256];
static uint32_t crc(uint32_t c, const uint8_t *p, size_t n) {
  if (!crc_table[1])
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t v = i;
      for (int k = 0; k < 8; k++) v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
      crc_table[i] = v;
    }
  c = ~c;
  while (n--) c = crc_table[(c ^ *p++) & 255] ^ (c >> 8);
  return ~c;
}
static void be32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24), p[1] = (uint8_t)(v >> 16), p[2] = (uint8_t)(v >> 8), p[3] = (uint8_t)v; }
static void chunk(FILE *f, const char *type, const uint8_t *data, uint32_t n) {
  uint8_t h[8];
  be32(h, n);
  memcpy(h + 4, type, 4);
  fwrite(h, 1, 8, f);
  if (n) fwrite(data, 1, n, f);
  uint32_t c = crc(crc(0, (const uint8_t *)type, 4), data, n);
  be32(h, c);
  fwrite(h, 1, 4, f);
}
static void save_png(const char *path) {
  enum { ROW = 1 + SCREEN_W * 3, RAW = ROW * SCREEN_H };
  static uint8_t raw[RAW], z[RAW + RAW / 65535 * 5 + 64];
  for (int y = 0; y < SCREEN_H; y++) {
    uint8_t *r = raw + y * ROW;
    r[0] = 0;
    for (int x = 0; x < SCREEN_W; x++) {
      uint16_t c = host_fb[y * SCREEN_W + x];
      r[1 + 3 * x] = (uint8_t)((c >> 11) * 255 / 31);
      r[2 + 3 * x] = (uint8_t)(((c >> 5) & 63) * 255 / 63);
      r[3 + 3 * x] = (uint8_t)((c & 31) * 255 / 31);
    }
  }
  /* zlib with stored blocks */
  size_t n = 0;
  z[n++] = 0x78, z[n++] = 1;
  uint32_t a = 1, b = 0;
  for (size_t at = 0; at < RAW;) {
    size_t len = RAW - at > 65535 ? 65535 : RAW - at;
    z[n++] = at + len == RAW;
    z[n++] = (uint8_t)len, z[n++] = (uint8_t)(len >> 8), z[n++] = (uint8_t)~len, z[n++] = (uint8_t)(~len >> 8);
    memcpy(z + n, raw + at, len);
    for (size_t i = 0; i < len; i++) a = (a + raw[at + i]) % 65521, b = (b + a) % 65521;
    n += len;
    at += len;
  }
  be32(z + n, b << 16 | a);
  n += 4;
  FILE *f = fopen(path, "wb");
  if (!f) return;
  fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
  uint8_t ihdr[13];
  be32(ihdr, SCREEN_W);
  be32(ihdr + 4, SCREEN_H);
  ihdr[8] = 8, ihdr[9] = 2, ihdr[10] = ihdr[11] = ihdr[12] = 0;
  chunk(f, "IHDR", ihdr, 13);
  chunk(f, "IDAT", z, (uint32_t)n);
  chunk(f, "IEND", NULL, 0);
  fclose(f);
}

static const char *out_dir = "build/shots";
static void save(const char *name) {
  char p[512];
  snprintf(p, sizeof p, "%s/%s.ppm", out_dir, name);
  host_shot(p);
  snprintf(p, sizeof p, "%s/%s.png", out_dir, name);
  save_png(p);
  printf("render: %s/%s.ppm, .png (%d commands, %d dropped)\n", out_dir, name, gfx_items(), gfx_dropped());
}

/* ---------------------------------------------------------------- the scene */
/* displayCity.json's planets and moons */
static const BgPlanet planets[] = {
  {SPR_PLANETS_SMALL, 40, 2, 120, -80, 0.5f}, {SPR_PLANETS_MEDIUM, 80, 1, 300, -200, 1},
  {SPR_PLANETS_SMALL, 40, 0, 40, -500, 0.2f}, {SPR_PLANETS_BIG, 100, 1, 900, 0, 1.3f},
  {SPR_PLANETS_BIG, 100, 3, -900, 400, 1.5f}, {SPR_PLANETS_MEDIUM, 80, 3, -400, 100, 0.1f}};

static BgWorld world = {0, 160, 440, 100, 1000, BG_LOOK_NORMAL, 0};
#define COLS 22
static uint8_t decos[COLS];

/* what stands on each column: world resources (displayCity.json) and buildings, bottom first */
enum { NONE_, FOREST, ROCK, RUINS, RUINS2, SHIP, NORMALHOUSE, WOODCUTTING, STONEMINE, INDOORFARM };
static uint8_t stack[COLS][4];
static const uint16_t building_spr[] = {[SHIP] = SPR_EXPLORATIONSHIP, [NORMALHOUSE] = SPR_NORMALHOUSE,
                                        [WOODCUTTING] = SPR_WOODCUTTINGCENTRE, [STONEMINE] = SPR_STONEMINE,
                                        [INDOORFARM] = SPR_INDOORFARM};

static void set_title_world(bool city) {
  memset(stack, 0, sizeof stack);
  static const struct { int col, what; } res[] = {{0, FOREST}, {6, ROCK}, {7, FOREST}, {9, ROCK}, {12, RUINS},
                                                  {14, FOREST}, {17, RUINS2}, {18, ROCK}, {19, FOREST},
                                                  {20, ROCK}, {21, FOREST}};
  bg_world_decorate(&world, decos);
  for (size_t i = 0; i < sizeof res / sizeof res[0]; i++) {
    stack[res[i].col][0] = (uint8_t)res[i].what;
    decos[res[i].col] = 0;   /* World.createWorldResource removes the decoration */
  }
  stack[3][0] = SHIP;
  decos[3] = 0;
  if (city) {
    static const struct { int col; uint8_t s[4]; } b[] = {
      {4, {NORMALHOUSE, NORMALHOUSE}}, {5, {WOODCUTTING, NORMALHOUSE, NORMALHOUSE}}, {8, {STONEMINE}},
      {10, {INDOORFARM, INDOORFARM, NORMALHOUSE}}, {11, {NORMALHOUSE, WOODCUTTING}}};
    for (size_t i = 0; i < sizeof b / sizeof b[0]; i++) {
      memcpy(stack[b[i].col], b[i].s, 4);
      decos[b[i].col] = 0;
    }
  }
}

/* a citizen: spr_humans frame (2 x 5, anchor at its bottom) */
typedef struct { float x, y; int sprite; } Walker;

static void draw_citizen(const BgCamera *cam, const Walker *w) {
  int z = cam->zoom;
  gfx_zoom(z);
  gfx_frame(SPR_HUMANS, 2, w->sprite, bg_screen_x(cam, w->x), bg_screen_y(cam, w->y - 5), 0, 255);
}

static void draw_city(const BgCamera *cam, double time, bool city) {
  int z = cam->zoom;
  bg_draw_sky(cam, time);
  gfx_zoom(z);
  /* buildings' backs (cityBgStage), then the farms' crops in front of them */
  for (int c = 0; c < COLS; c++)
    for (int r = 0; r < 4 && stack[c][r]; r++) {
      int k = stack[c][r];
      if (k < SHIP) continue;
      int x = bg_screen_x(cam, (float)(world.x + 20 * c)), y = bg_screen_y(cam, (float)(world.y - 20 * (1 + r)));
      gfx_sprite_part(building_spr[k], 44, 0, 20, 20, x, y, 0, 255);
    }
  for (int c = 0; c < COLS; c++)
    for (int r = 0; r < 4 && stack[c][r]; r++)
      if (stack[c][r] == INDOORFARM)
        gfx_frame(SPR_INDOORFARM_CROPS, 20, r ? 7 : 4, bg_screen_x(cam, (float)(world.x + 20 * c)),
                  bg_screen_y(cam, (float)(world.y - 20 * (1 + r))), 0, 255);
  /* citizens inside buildings */
  if (city) {
    /* (a citizen inside stands 1 pixel above its floor's bottom: Citizen.updateDisplay) */
    static const Walker inside[] = {{4 * 20 + 6, 159, 3}, {5 * 20 + 12, 139, 17}, {10 * 20 + 9, 159, 25},
                                    {11 * 20 + 14, 139, 40}, {5 * 20 + 4, 119, 8}};
    for (size_t i = 0; i < sizeof inside / sizeof inside[0]; i++) draw_citizen(cam, &inside[i]);
  }
  /* the island and its decorations, buildings' fronts, world resources (cityStage) */
  bg_draw_world(&world, decos, cam);
  gfx_zoom(z);
  for (int c = 0; c < COLS; c++)
    for (int r = 0; r < 4 && stack[c][r]; r++) {
      int k = stack[c][r];
      int x = bg_screen_x(cam, (float)(world.x + 20 * c)), y = bg_screen_y(cam, (float)(world.y - 20 * (1 + r)));
      switch (k) {
      case FOREST: gfx_frame(SPR_FOREST, 20, 0, x, y, 0, 255); break;
      case ROCK: gfx_frame(SPR_ROCK, 20, 0, x, y, 0, 255); break;
      case RUINS: gfx_sprite(SPR_ALIENRUINS, x, y, 0, 255); break;
      case RUINS2: gfx_sprite(SPR_ALIENRUINS_2, x, y, 0, 255); break;
      default:
        /* NormalBuildingDrawer: the ground floor's frame is the second */
        gfx_sprite_part(building_spr[k], r == 0 ? 22 : 0, 0, 20, 20, x, y, 0, 255);
        if (k == NORMALHOUSE && r == 0) gfx_frame(SPR_COLOREDDOORS, 20, c % 6, x, y, 0, 255);
      }
    }
  /* citizens outside (displayCity: five, between x 60 and 80) */
  static const Walker outside[] = {{61, 160, 0}, {66, 160, 12}, {70, 160, 30}, {74, 160, 7}, {79, 160, 21},
                                   {130, 160, 44}, {150, 160, 2}, {205, 160, 15}};
  for (size_t i = 0; i < (city ? 8u : 5u); i++) draw_citizen(cam, &outside[i]);
}

static double frame_ms(void (*fn)(void *), void *arg) {
  struct timespec a, b;
  clock_gettime(CLOCK_MONOTONIC, &a);
  int n = 20;
  for (int i = 0; i < n; i++) fn(arg);
  clock_gettime(CLOCK_MONOTONIC, &b);
  return ((double)(b.tv_sec - a.tv_sec) * 1e3 + (double)(b.tv_nsec - a.tv_nsec) / 1e6) / n;
}

typedef struct { BgCamera cam; double time; bool city, title; } Scene;

static void scene(void *p) {
  const Scene *s = p;
  gfx_begin();
  draw_city(&s->cam, s->time, s->city);
  if (s->title) {   /* MainMenu's logo, centred 10 pixels from the top */
    gfx_zoom(1);
    gfx_sprite(SPR_TITLE, SCREEN_W / 2 - (SPR_TITLE_W + 1) / 2, 10, 0, 255);
  }
  gfx_end();
}

static void fonts(void *p) {
  (void)p;
  static const char *names[FONT_COUNT] = {"standard", "standard10", "standard15", "standard16", "standard18"};
  gfx_begin();
  gfx_fill(rgb565_near(0x281460));
  gfx_zoom(1);
  /* a window as the original's GUI draws its back (spr_9p_window's middle, see-through) */
  gfx_rect(4, 4, SCREEN_W - 8, SCREEN_H - 8, rgb565_near(0x000000), 96);
  int y = 8;
  char line[128];
  for (int f = 0; f < FONT_COUNT; f++) {
    snprintf(line, sizeof line, "%s: The Final Earth 2", names[f]);
    gfx_text(f, 8, y, line, 0xFFFF);
    y += gfx_font_height(f) + 1;
    snprintf(line, sizeof line, "0123456789 %d px, ABC xyz +-/%%() Straße é ë ü", gfx_text_width(f, line));
    gfx_text(f, 8, y, line, rgb565_near(0xFFE08A));
    y += gfx_font_height(f) + 3;
  }
  /* kerning (standard16 has pairs such as " A"), alpha and a clipped line */
  gfx_text(FONT_16, 8, y, "A A AV To kerning", rgb565_near(0xA0FFA0));
  gfx_text_ex(FONT_STANDARD, 180, y, "half see-through", 0xFFFF, 128);
  y += gfx_font_height(FONT_16) + 2;
  gfx_clip(8, y, 100, 6);
  gfx_text(FONT_STANDARD, 8, y, "clipped to a 100 x 6 box: cut", rgb565_near(0xFF8080));
  gfx_clip(0, 0, SCREEN_W, SCREEN_H);
  gfx_zoom(2);
  gfx_text(FONT_10, 120, y, "zoom 2", 0xFFFF);
  gfx_end();
}

int main(int argc, char **argv) {
  if (argc > 1) out_dir = argv[1];
  mkdir("build", 0777);
  mkdir(out_dir, 0777);
  bg_init(1);
  bg_set_planets(planets, (int)(sizeof planets / sizeof planets[0]));
  /* City.getCityEdges for displayCity: its world (the stacks on it are lower than y 0) */
  BgEdges edges = {0, 440, 0, 260};
  bg_clouds_start(&edges);
  for (int i = 0; i < 120; i++) bg_clouds_update(2, &edges);
  printf("render: %d clouds\n", bg_cloud_count());

  /* the title screen's view: viewPos (225, 150) (displayCity's viewYFromBottom 270), zoom 1 */
  Scene s = {{225 - SCREEN_W / 2, 150 - SCREEN_H / 2, 1}, 0, false, true};
  set_title_world(false);
  double ms = frame_ms(scene, &s);
  save("a_title_night");
  printf("render: %.2f ms a frame (this computer, sanitizers on)\n", ms);
  s.time = 12 * 60;
  ms = frame_ms(scene, &s);
  save("b_title_midday");
  printf("render: %.2f ms a frame\n", ms);
  set_title_world(true);
  s = (Scene){{170 - SCREEN_W / 2, 120 - SCREEN_H / 2, 1}, 16.5 * 60, true, false};
  ms = frame_ms(scene, &s);
  save("c_city");
  printf("render: %.2f ms a frame\n", ms);
  fonts(NULL);
  save("d_fonts");
  s.cam = (BgCamera){170 - SCREEN_W / 4, 120 - SCREEN_H / 4, 2};
  ms = frame_ms(scene, &s);
  save("e_city_zoom2");
  printf("render: %.2f ms a frame\n", ms);

  /* the islands' collision mask, as WorldImage's */
  printf("render: mask of the island (20 x 20 cells):\n");
  for (int cy = 0; cy < world.h / 20; cy++) {
    printf("  ");
    for (int cx = 0; cx < world.w / 20; cx++) putchar(bg_world_mask(&world, cx, cy) ? '#' : '.');
    putchar('\n');
  }
  return 0;
}
