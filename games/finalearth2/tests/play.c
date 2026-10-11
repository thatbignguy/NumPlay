/* Plays The Final Earth 2 on a computer, with scripted keys, and saves frames.
 *
 *   play [--story N] [--frames N] [--keys "10-40:right,50:ok,..."] [--shots 30,60] [--out DIR] [--saves DIR]
 *        [--speed X] [--zoom Z] [--at X,Y] [--title] [--draw-all] [--cursor X,Y] [--materials N]
 *        [--building Class[:arg2]] [--move Class] [--open kind,arg,arg2] [--open-frame N] [--tutorial N] [--trace]
 *        [--add Class:world:col[:rgb]]... [--boost] [--explore N] [--storage-full]
 *
 * --title starts at the title screen (as the calculator does); otherwise the story's city is made and
 * played. --draw-all draws every frame (to find the frames' peak use of the draw list). --cursor puts the
 * city's cursor on a city position; --materials gives the city N of each material. --add builds a class on top
 * of a world's column (a decoration tinted 0xRRGGBB); --boost starts a music festival's happiness boost;
 * --explore explores every alien ruins N percent. --storage-full leaves no room for saves.
 *
 * Keys: left right up down ok back home toolbox var shift alpha plus minus backspace, and the typing keys as
 * tkN (N: the key's number on the NumWorks keyboard, plat.h's TK_*: tk18 is exp, "a" with alpha). "a-b:key"
 * holds the key from frame a to b, "a:key" for one frame. Screenshots are DIR/shot_<frame>.ppm. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../src/bld.h"
#include "../src/game.h"
#include "../src/scratch.h"
#include "../src/story.h"
#include "../src/view.h"
#include "../src/ui.h"
#include "../src/uii.h"
#include "../src/classes.h"
extern int ui_peak_prims, ui_peak_text, ui_dropped_total, ui_redraws;
uint16_t bld_build_at(int w, int col, int row, int cls, bool replace);
void ruins_explore(Perm *p, float amount);

extern uint32_t host_keys, host_time;
extern int host_text_key;
extern bool host_storage_full;
void host_shot(const char *path);
void host_save_dir(const char *d);

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

int main(int argc, char **argv) {
  static Scratch scr;
  scratch = &scr;
  const char *out = "build/play", *saves = "build/play/saves", *shots = "";
  int frames = 300, story_index = 0, zoom = 1;
  float speed = 1, at_x = 0, at_y = 0;
  bool at = false, title = false, draw_all = false, cursor = false, trace = false;
  float cur_x = 0, cur_y = 0, materials = -1;
  int open_kind = -1, open_arg = 0, open_arg2 = 0, open_frame = 1;
  const char *building = NULL, *move = NULL;
  int tutorial = 0, nadds = 0;
  const char *adds[16];
  bool boost = false;
  float explore = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--story") && i + 1 < argc) story_index = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
    else if (!strcmp(argv[i], "--saves") && i + 1 < argc) saves = argv[++i];
    else if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
    else if (!strcmp(argv[i], "--speed") && i + 1 < argc) speed = (float)atof(argv[++i]);
    else if (!strcmp(argv[i], "--zoom") && i + 1 < argc) zoom = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--at") && i + 1 < argc) at = sscanf(argv[++i], "%f,%f", &at_x, &at_y) == 2;
    else if (!strcmp(argv[i], "--cursor") && i + 1 < argc) cursor = sscanf(argv[++i], "%f,%f", &cur_x, &cur_y) == 2;
    else if (!strcmp(argv[i], "--materials") && i + 1 < argc) materials = (float)atof(argv[++i]);
    else if (!strcmp(argv[i], "--title")) title = true;
    else if (!strcmp(argv[i], "--draw-all")) draw_all = true;
    else if (!strcmp(argv[i], "--trace")) trace = true;
    else if (!strcmp(argv[i], "--open") && i + 1 < argc) sscanf(argv[++i], "%d,%d,%d", &open_kind, &open_arg, &open_arg2);
    else if (!strcmp(argv[i], "--open-frame") && i + 1 < argc) open_frame = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--building") && i + 1 < argc) building = argv[++i];
    else if (!strcmp(argv[i], "--move") && i + 1 < argc) move = argv[++i];
    else if (!strcmp(argv[i], "--tutorial") && i + 1 < argc) tutorial = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--add") && i + 1 < argc && nadds < 16) adds[nadds++] = argv[++i];
    else if (!strcmp(argv[i], "--boost")) boost = true;
    else if (!strcmp(argv[i], "--storage-full")) host_storage_full = true;
    else if (!strcmp(argv[i], "--explore") && i + 1 < argc) explore = (float)atof(argv[++i]);
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
    }
  }
  mkdir(out, 0755);
  mkdir(saves, 0755);
  host_save_dir(saves);
  rnd_seed(12345);
  bg_init(777);
  if (title) {
    ui_init();
  } else {
    ui_before_city();
    game_start(story_index, NULL);
    ui_after_load();
    city.simulationSpeed = speed;
    if (zoom == 2) camera.zoom = 2;
    if (at) camera.x = at_x, camera.y = at_y;
    if (cursor) cursor_to((int)cur_x, (int)cur_y);
    if (materials >= 0)
      for (int m = 0; m < M_COUNT; m++) city.materials.m[m] = materials;
    for (int a = 0; a < nadds; a++) {
      char name[48];
      int w = 0, col = 0;
      unsigned rgb = 0;
      snprintf(name, sizeof name, "%s", adds[a]);
      char *c1 = strchr(name, ':');
      if (!c1) continue;
      *c1 = 0;
      int n = sscanf(c1 + 1, "%d:%d:%x", &w, &col, &rgb);
      int cls = pcls_find(name);
      if (cls < 0 || n < 2 || w >= nworlds) continue;
      uint16_t id = bld_build_at(w, col, col_stack_height(w, col), cls, false);
      if (id && n == 3) bld_customize_color(P(id), (int32_t)rgb);
    }
    if (boost) happiness_add_boost(city.timeSinceStart + 10080, 25, true, HB_MUSIC_FESTIVAL);
    for (int k = 1; explore > 0 && k < MAX_PERMS; k++)
      if ((perms[k].flags & PF_USED) && is_a(&perms[k], B_RUINS)) ruins_explore(&perms[k], explore);
  }
  int peak_items = 0, peak_dropped = 0;
  for (int f = 0; f < frames && game_running; f++) {
    host_keys = 0;
    host_text_key = -1;
    for (int i = 0; i < nholds; i++)
      if (f >= holds[i].a && f <= holds[i].b) {
        host_keys |= holds[i].k;
        if (holds[i].tk >= 0) host_text_key = holds[i].tk;
      }
    if (f == open_frame) {
      /* a window opened straight away (for screenshots): --open kind,arg,arg2 or --building Class */
      if (building) {
        /* "Class" or "Class:arg2" */
        char name[48];
        int arg2 = 0;
        snprintf(name, sizeof name, "%s", building);
        char *colon = strchr(name, ':');
        if (colon) *colon = 0, arg2 = atoi(colon + 1);
        int c = pcls_find(name);
        for (int k = 1; k < MAX_PERMS; k++)
          if (c >= 0 && (perms[k].flags & PF_USED) && perms[k].cls == c) {
            win_close_all();
            cu.selected = (uint16_t)k;
            win_open(WK_BUILDING, (uint16_t)k, (uint16_t)arg2);
            break;
          }
      }
      if (tutorial) {
        /* a tutorial's arrows (as the story's goal starts them), its window already closed */
        win_close_all();
        ui_show_tutorial(tutorial);
        tut_flags |= TF_WINDOW_CLOSED;
      }
      if (move) {
        /* a world resource being moved (as explored alien ruins' Move button does) */
        int c = pcls_find(move);
        for (int k = 1; k < MAX_PERMS; k++)
          if (c >= 0 && (perms[k].flags & PF_USED) && perms[k].cls == c) {
            moving_resource = (uint16_t)k;
            cu.selected = (uint16_t)k;
            builder_start(BK_MOVE, 0, 0);
            break;
          }
      }
      if (open_kind >= 0) {
        win_close_all();
        win_open(open_kind, (uint16_t)open_arg, (uint16_t)open_arg2);
      }
    }
    game_tick();
    host_time += 33;
    if (trace)
      printf("%d: keys %x zone %d windows %d (top %d focus %d) category %d focus %d builder %d cursor %d,%d camera %.0f,%.0f panel %d\n", f,
             host_keys, cu.zone, nwins, nwins ? wins[nwins - 1].kind : -1, nwins ? wins[nwins - 1].focus : -1, cu.category, cu.bfocus,
             cu.build_kind, cu.cx, cu.cy, camera.x, camera.y, cu.pfocus);
    char tag[16];
    snprintf(tag, sizeof tag, ",%d,", f);
    char list[1024];
    snprintf(list, sizeof list, ",%s,", shots);
    bool shot = strstr(list, tag) || f == frames - 1;
    if (shot || draw_all) {
      game_draw();
      if (gfx_items() > peak_items) peak_items = gfx_items();
      if (gfx_dropped() > peak_dropped) peak_dropped = gfx_dropped();
    }
    if (shot) {
      char p[512];
      snprintf(p, sizeof p, "%s/shot_%d.ppm", out, f);
      host_shot(p);
    }
  }
  printf("story %d (goal %d): day %d %.1fh citizens %d permanents %d food %.1f wood %.1f stone %.1f happiness %.1f gfx items %d dropped %d\n",
         story.index, story.goal, city_day(), city_hour(), ncits, perm_count(), city.materials.m[M_FOOD], city.materials.m[M_WOOD],
         city.materials.m[M_STONE], happiness.happiness, gfx_items(), gfx_dropped());
  if (trace) {
    int counts[CLS_COUNT] = {0};
    for (int k = 1; k < MAX_PERMS; k++)
      if (perms[k].flags & PF_USED) counts[perms[k].cls]++;
    for (int c = 0; c < CLS_COUNT; c++)
      if (counts[c]) printf("%s x%d\n", pcls[c].name, counts[c]);
  }
  printf("peak: gfx items %d, gfx dropped %d, ui prims %d/%d, ui text %d/%d, ui dropped %d, windows laid out again %d\n",
         peak_items, peak_dropped, ui_peak_prims, UI_PRIMS, ui_peak_text, UI_TEXT, ui_dropped_total, ui_redraws);
  return 0;
}
