/* Plays NumPowder on a computer, with scripted keys, and saves frames.
 *
 *   play [--frames N] [--keys "10-40:right,50:ok,..."] [--shots 30,60] [--out DIR] [--saves DIR]
 *        [--scene NAME]... [--rect TYPE:x0,y0,x1,y1]... [--wall WALL:x0,y0,x1,y1]... [--display N]
 *        [--tools LEFT,RIGHT] [--cursor X,Y] [--brush R] [--paused] [--intro] [--npart N] [--debug]
 *        [--seed N] [--time]
 *
 * Keys: left right up down ok back home toolbox var shift alpha plus minus backspace, and the typing keys as
 * tkN. "a-b:key" holds the key from frame a to b, "a:key" for one frame. Screenshots are DIR/shot_<frame>.ppm
 * (and the last frame's). Scenes: sand, water, fire, lava, walls, elec, gases, glow, stkm, explode, air, all (one
 * of every element, in rows). --rect fills with an element (its name, or its number), --wall with a wall's
 * number. --display picks a render preset (render.h's PRESET_*). --intro starts with the intro text, as the
 * calculator does. --time prints how long the drawing took. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include "../src/ui.h"

extern uint32_t host_keys, host_time;
extern int host_text_key;
void host_shot(const char *path);
void host_save_dir(const char *d);
bool game_frame(void);
void game_init(void);
int host_npart;

typedef struct { int a, b; uint32_t k; int tk; } Hold;
static Hold holds[1024];
static int nholds;

static uint32_t key_bit(const char *s) {
  if (!strncmp(s, "tk", 2)) return 0;
  static const struct { const char *n; uint32_t k; } keys[] = {
    {"left", K_LEFT}, {"right", K_RIGHT}, {"up", K_UP}, {"down", K_DOWN}, {"ok", K_OK}, {"back", K_BACK},
    {"home", K_HOME}, {"toolbox", K_TOOLBOX}, {"var", K_VAR}, {"shift", K_SHIFT}, {"alpha", K_ALPHA},
    {"plus", K_PLUS}, {"minus", K_MINUS}, {"backspace", K_BACKSPACE}};
  for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
    if (!strcmp(s, keys[i].n)) return keys[i].k;
  fprintf(stderr, "unknown key %s\n", s);
  return 0;
}

static int element_by_name(const char *s) {
  if (s[0] >= '0' && s[0] <= '9') return atoi(s);
  for (int t = 0; t < PT_COUNT; t++)
    if (elements[t].Name && !strcasecmp(elements[t].Name, s)) return t;
  fprintf(stderr, "unknown element %s\n", s);
  return 0;
}

static void fill(int t, int x0, int y0, int x1, int y1) {
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++)
      if (x >= 0 && y >= 0 && x < XRES && y < YRES) create_part(-1, x, y, t, -1);
}
static void wall(int w, int x0, int y0, int x1, int y1) {
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++)
      if (x >= 0 && y >= 0 && x < XRES && y < YRES) bmap[y / WCELL][x / WCELL] = (unsigned char)w;
}

static void scene(const char *name) {
  if (!strcmp(name, "sand")) {
    fill(PT_SAND, 30, 5, 45, 20);
    fill(PT_METL, 10, 50, 89, 52);
    fill(PT_SALT, 60, 10, 70, 18);
  } else if (!strcmp(name, "water")) {
    wall(WL_WALL, 4, 40, 95, 43);
    fill(PT_WATR, 10, 10, 40, 30);
    fill(PT_OIL, 60, 15, 80, 25);
  } else if (!strcmp(name, "fire")) {
    fill(PT_WOOD, 20, 40, 79, 50);
    fill(PT_FIRE, 30, 35, 70, 39);
    fill(PT_GAS, 40, 10, 60, 20);
  } else if (!strcmp(name, "lava")) {
    fill(PT_LAVA, 30, 20, 60, 30);
    fill(PT_WATR, 10, 45, 89, 55);
    fill(PT_GLAS, 5, 56, 94, 58);
  } else if (!strcmp(name, "walls")) {
    for (int w = 1; w < UI_WALLCOUNT; w++) {
      int x = 4 + ((w - 1) % 9) * 10, y = 6 + ((w - 1) / 9) * 14;
      wall(w, x, y, x + 7, y + 7);
    }
    fill(PT_WATR, 10, 40, 30, 50);
  } else if (!strcmp(name, "elec")) {
    fill(PT_METL, 10, 30, 89, 31);
    fill(PT_BTRY, 8, 28, 9, 33);
    fill(PT_LCRY, 40, 34, 60, 40);
    fill(PT_SPRK, 11, 30, 12, 30);
  } else if (!strcmp(name, "gases")) {
    fill(PT_PLSM, 10, 10, 30, 20);
    fill(PT_SMKE, 40, 10, 60, 20);
    fill(PT_NEUT, 70, 10, 90, 20);
    fill(PT_PHOT, 10, 40, 30, 50);
    fill(PT_CFLM, 40, 40, 60, 50);
    fill(PT_THDR, 75, 45, 76, 46);
  } else if (!strcmp(name, "glow")) {
    fill(PT_GLOW, 10, 10, 30, 30);
    fill(PT_DEUT, 40, 10, 60, 30);
    fill(PT_ACID, 70, 10, 90, 30);
    fill(PT_LAVA, 10, 40, 30, 55);
    fill(PT_GEL, 40, 40, 60, 55);
    fill(PT_BRAY, 70, 40, 90, 41);
    fill(PT_EMBR, 70, 50, 90, 55);
  } else if (!strcmp(name, "stkm")) {
    wall(WL_WALL, 4, 54, 95, 57);
    fill(PT_DMND, 70, 44, 75, 53);
    create_part(-2, 30, 45, PT_STKM, 0);
  } else if (!strcmp(name, "explode")) {
    fill(PT_PLEX, 40, 40, 60, 50);
    fill(PT_FIRE, 50, 38, 51, 39);
  } else if (!strcmp(name, "air")) {
    /* air to show: pressure up at the left and down at the right, a swirl in the middle */
    for (int y = 0; y < YACELLS; y++)
      for (int x = 0; x < XACELLS; x++) {
        float dx = x - XACELLS / 2.0f, dy = y - YACELLS / 2.0f;
        pv[y][x] = (x < XACELLS / 2 ? 1 : -1) * 8.0f * (1 - fabsf(dy) / YACELLS);
        vx[y][x] = -dy * 0.6f, vy[y][x] = dx * 0.6f;
        hv[y][x] = 273.15f + 22 + x * 60;
      }
  } else if (!strcmp(name, "all")) {
    /* every element in the menus, one block each, in rows */
    int k = 0;
    for (int t = 1; t < PT_COUNT; t++) {
      if (!elements[t].Enabled || !elements[t].MenuVisible) continue;
      int x = (k % 25) * 4, y = (k / 25) * 8 + 2;
      fill(t, x, y, x + 2, y + 2);
      k++;
    }
  } else {
    fprintf(stderr, "unknown scene %s\n", name);
  }
}

int main(int argc, char **argv) {
  const char *out = "build/play", *saves = "build/play/saves", *shots = "";
  int frames = 120, display = -1, brush_r = -1, seed = 1;
  bool paused = false, intro = false, debug = false, timing = false;
  const char *scenes[16], *rects[32], *walls_[32], *tools = NULL, *cursor = NULL;
  int nscenes = 0, nrects = 0, nwalls = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
    else if (!strcmp(argv[i], "--saves") && i + 1 < argc) saves = argv[++i];
    else if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
    else if (!strcmp(argv[i], "--scene") && i + 1 < argc && nscenes < 16) scenes[nscenes++] = argv[++i];
    else if (!strcmp(argv[i], "--rect") && i + 1 < argc && nrects < 32) rects[nrects++] = argv[++i];
    else if (!strcmp(argv[i], "--wall") && i + 1 < argc && nwalls < 32) walls_[nwalls++] = argv[++i];
    else if (!strcmp(argv[i], "--display") && i + 1 < argc) display = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--tools") && i + 1 < argc) tools = argv[++i];
    else if (!strcmp(argv[i], "--cursor") && i + 1 < argc) cursor = argv[++i];
    else if (!strcmp(argv[i], "--brush") && i + 1 < argc) brush_r = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--npart") && i + 1 < argc) host_npart = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--seed") && i + 1 < argc) seed = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--paused")) paused = true;
    else if (!strcmp(argv[i], "--intro")) intro = true;
    else if (!strcmp(argv[i], "--debug")) debug = true;
    else if (!strcmp(argv[i], "--time")) timing = true;
    else if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
      char *s = strdup(argv[++i]);
      for (char *t = strtok(s, ","); t && nholds < 1024; t = strtok(NULL, ",")) {
        int a, b;
        char k[16];
        if (sscanf(t, "%d-%d:%15s", &a, &b, k) == 3) holds[nholds++] = (Hold){a, b, key_bit(k), -1};
        else if (sscanf(t, "%d:%15s", &a, k) == 2) holds[nholds++] = (Hold){a, a, key_bit(k), -1};
        else continue;
        if (!strncmp(k, "tk", 2)) holds[nholds - 1].tk = atoi(k + 2);
      }
    } else {
      fprintf(stderr, "unknown option %s\n", argv[i]);
      return 2;
    }
  }
  mkdir(out, 0755);
  mkdir(saves, 0755);
  host_save_dir(saves);
  game_init();
  rng_seed((unsigned)seed);
  if (!intro) ui.intro = 0;
  if (paused) sys_pause = true;
  if (debug) ui.debug = true;
  if (display >= 0 && display < PRESET_COUNT)
    render_apply_preset(display);
  if (brush_r >= 0) brush.rx = brush.ry = brush_r;
  if (cursor) sscanf(cursor, "%d,%d", &ui.cx, &ui.cy);
  if (tools) {
    char l[32], r[32];
    if (sscanf(tools, "%31[^,],%31s", l, r) == 2) ui.tool[0] = element_by_name(l), ui.tool[1] = element_by_name(r);
  }
  for (int k = 0; k < nscenes; k++) scene(scenes[k]);
  for (int k = 0; k < nrects; k++) {
    char name[32];
    int x0, y0, x1, y1;
    if (sscanf(rects[k], "%31[^:]:%d,%d,%d,%d", name, &x0, &y0, &x1, &y1) == 5) fill(element_by_name(name), x0, y0, x1, y1);
  }
  for (int k = 0; k < nwalls; k++) {
    int w, x0, y0, x1, y1;
    if (sscanf(walls_[k], "%d:%d,%d,%d,%d", &w, &x0, &y0, &x1, &y1) == 5) wall(w, x0, y0, x1, y1);
  }
  double draw_total = 0;
  for (int f = 0; f < frames; f++) {
    host_keys = 0;
    host_text_key = -1;
    for (int i = 0; i < nholds; i++)
      if (f >= holds[i].a && f <= holds[i].b) {
        host_keys |= holds[i].k;
        if (holds[i].tk >= 0) host_text_key = holds[i].tk;
      }
    clock_t c0 = clock();
    bool go = game_frame();
    draw_total += (double)(clock() - c0) / CLOCKS_PER_SEC;
    host_time += 33;
    char tag[16], list[1024];
    snprintf(tag, sizeof tag, ",%d,", f);
    snprintf(list, sizeof list, ",%s,", shots);
    if (strstr(list, tag) || f == frames - 1 || !go) {
      char path[512];
      snprintf(path, sizeof path, "%s/shot_%d.ppm", out, f);
      host_shot(path);
    }
    if (!go) {
      printf("quit at frame %d\n", f);
      break;
    }
  }
  if (timing) {
    float g = 0;
    for (int y = 0; y < YACELLS; y++)
      for (int x = 0; x < XACELLS; x++) g = fmaxf(g, fabsf(gravx[y][x]) + fabsf(gravy[y][x]));
    printf("Newtonian gravity %s, field lines %s, strongest field %.3f\n", ngrav_enable ? "on" : "off", ren.grav_field ? "on" : "off", g);
  }
  extern bool ov_overflowed;
  if (ov_overflowed) printf("the overlay's list overflowed\n");
  if (timing) printf("%d frames, %.2f ms a frame on this computer (sanitizers on), %d particles\n", frames, draw_total * 1000 / frames, NUM_PARTS);
  return 0;
}
