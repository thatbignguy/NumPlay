/* Runs the buildings without a screen. First a city with every class of
 * building and world resource: many days, every window and button, every
 * upgrade, mode, research and policy, every drawer at both zooms, then a save
 * and a load. Then small cities, one per kind of production, whose output is
 * measured against the original's formulas.
 *
 * Built like the other tests but without the user interface (it has its own
 * windows and stand-ins here): every src file but plat_eadk.c, game.c and
 * ui*.c, with -DHOST and the address and undefined behaviour sanitizers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../src/bldi.h"
#include "../src/save.h"
#include "../src/scratch.h"
#include "../src/view.h"

void story_set_name(const char *n);
void game_reset(void);
void host_save_dir(const char *d);
extern int bld_bad_draws;

/* ---------------------------------------------------------------- windows: counted, and their buttons kept */
struct WinOut {
  int lines, buttons;
  int actions[24], nactions;
  bool print;
};
static void show(WinOut *w, const char *kind, const char *s) {
  w->lines++;
  if (w->print) printf("    %s %s\n", kind, s);
}
void win_title(WinOut *w, const char *s) { show(w, "#", s); }
void win_text(WinOut *w, const char *s) { show(w, "|", s); }
void win_info(WinOut *w, uint16_t icon, const char *s) { (void)icon, show(w, "i", s); }
void win_button(WinOut *w, uint16_t icon, const char *label, int action, bool enabled) {
  (void)icon;
  w->buttons++;
  if (w->print) printf("    [%s]%s\n", label, enabled ? "" : " (off)");
  if (enabled && w->nactions < 24) w->actions[w->nactions++] = action;
}
void win_cost(WinOut *w, const float cost[M_COUNT]) {
  w->lines++;
  if (w->print) {
    printf("    $");
    for (int m = 0; m < M_COUNT; m++)
      if (cost[m]) printf(" %d:%g", m, cost[m]);
    printf("\n");
  }
}

void win_para(WinOut *w, uint8_t font, const char *s, int max_w, uint16_t color) {
  (void)font, (void)max_w, (void)color;
  show(w, "|", s);
}
void win_setting(WinOut *w, bool checked, int action, const char *text) {
  w->buttons++;
  if (w->print) printf("    [%s] %s\n", checked ? "x" : " ", text);
  if (w->nactions < 24) w->actions[w->nactions++] = action;
}
void win_later(WinOut *w) { (void)w; }
void win_space(WinOut *w, int h) { (void)w, (void)h; }
void win_heading(WinOut *w, const char *title, int one_action, int all_action, bool all, bool max, const char *one_tip,
                 const char *all_tip) {
  (void)all, (void)max, (void)one_tip, (void)all_tip;
  show(w, "##", title);
  if (one_action >= 0 && w->nactions + 2 <= 24) w->actions[w->nactions++] = one_action, w->actions[w->nactions++] = all_action;
}
void win_bottom_button(WinOut *w, const char *label, int action, const char *tip_header, const char *tip) {
  (void)tip_header, (void)tip;
  w->buttons++;
  if (w->print) printf("    [%s] (bottom)\n", label);
  if (w->nactions < 24) w->actions[w->nactions++] = action;
}

/* the interface's part the city calls (the interface itself is not linked here) */
void ui_show_goal_text(int goal) { (void)goal; }
void ui_show_tutorial(int tutorial) { (void)tutorial; }
void ui_show_message(const char *text) { (void)text; }
void ui_show_simple(const char *text) { (void)text; }
bool ui_story_window_open(void) { return false; }
void meta_win_scenario(int s, float time) { (void)s, (void)time; }
void ui_pause_for_window(void) {}

static int failures;
#define CHECK(c, ...) do { if (!(c)) { failures++; printf("FAIL: " __VA_ARGS__); printf("\n"); } } while (0)

static Scratch scr;
static void new_city(const char *story) {
  game_reset();
  unlocks_init();
  story_set_name(story);
}
static uint16_t build(int w, int col, int row, const char *name) {
  int cls = pcls_find(name);
  if (cls < 0) {
    printf("no class %s\n", name);
    failures++;
    return 0;
  }
  return bld_build_at(w, col, row, cls, false);
}
static void people(int w, int n, float amin, float amax) {
  for (int i = 0; i < n; i++) cit_create(w, rnd_range(amin, amax), NONE, -1);
}
static void plenty(void) {
  for (int m = 0; m < M_COUNT; m++) city.materials.m[m] = 1e7;
}
/* production of material m, summed over the days (the stats' today, which starts again at midnight) */
typedef struct { double last, total; } Meter;
static void meter(Meter *mt, int m) {
  double cur = stats.materialProduction[m][0];
  mt->total += cur >= mt->last ? cur - mt->last : cur;
  mt->last = cur;
}
#define TICK_TIMEMOD (city.simulationSpeed * 0.6666667f * 2)   /* timeMod of one sim_tick */
#define TICKS_PER_DAY ((int)(1440 / (city.minutesPerTick * TICK_TIMEMOD)))

/* ---------------------------------------------------------------- drawing: every permanent, every layer, both zooms */
static uint16_t band_px[SCREEN_W * GFX_BAND];
static uint8_t bad_shown[256];
static int draw_all(void) {
  /* nothing a drawer reaches may touch the search memory (the band lives there while a frame is drawn) */
  memset(&scr, 0xA5, sizeof scr);
  int n = 0;
  for (int z = 1; z <= 2; z++) {
    GfxBand b = {band_px, 0, GFX_BAND, 0, 0, SCREEN_W, GFX_BAND, z};
    camera.zoom = z;
    for (int layer = 0; layer < LAYER_COUNT; layer++) {
      for (int i = 1; i < MAX_PERMS; i++)
        if (perms[i].flags & PF_USED) {
          int bad = bld_bad_draws;
          for (int dy = -20 * z; dy < GFX_BAND; dy += 7) bld_draw(&b, &perms[i], layer, 100, dy);
          if (bld_bad_draws != bad && !bad_shown[perms[i].cls]) {
            bad_shown[perms[i].cls] = 1;
            printf("  bad drawing: %s layer %d\n", pcls[perms[i].cls].name, layer);
          }
          n++;
        }
      int bad = bld_bad_draws;
      bld_draw_free(&b, layer);
      if (bld_bad_draws != bad) printf("  bad free drawing on layer %d\n", layer);
    }
  }
  camera.zoom = 1;
  const uint8_t *s = (const uint8_t *)&scr;
  for (size_t k = 0; k < sizeof scr; k++)
    if (s[k] != 0xA5) {
      failures++;
      printf("FAIL: drawing wrote into the search memory at byte %zu\n", k);
      break;
    }
  return n;
}

/* ---------------------------------------------------------------- windows, buttons and upgrades of one permanent */
static int windows_shown, buttons_pressed, upgrades_bought, options_seen;
static void exercise(uint16_t id, bool press) {
  Perm *p = P(id);
  if (!(p->flags & PF_USED)) return;
  const PermOps *o = PC(p)->ops;
  WinOut w = {0};
  if (o && o->window) {
    o->window(p, &w);
    windows_shown++;
  }
  if (press && o && o->window_action)
    for (int a = 0; a < w.nactions && (p->flags & PF_USED); a++)
      if (w.actions[a] < WA_UI_BIRTH_CONTROL) {
        o->window_action(p, w.actions[a]);
        buttons_pressed++;
      }
  if (!(p->flags & PF_USED) || !is_building(p)) return;
  UpgradeOption opt[16];
  int n = upgrade_options(p, opt, 16);
  options_seen += n;
  if (!press) return;
  for (int k = 0; k < n; k++) {
    if (opt[k].kind == UO_POLICY) continue;
    if (!opt[k].owned && upgrade_press(p, opt[k].upgrade, k & 1)) upgrades_bought++;
    if (opt[k].canChangeLook) upgrade_change_look(p, opt[k].upgrade);
  }
}

/* ---------------------------------------------------------------- 1: every class */
static unsigned long city_sum(void) {
  unsigned long h = (unsigned long)ncits * 31 + (unsigned long)perm_count() * 7;
  for (int i = 1; i < MAX_CITS; i++)
    if (cits[i].flags & CF_ALIVE) h = h * 131 + cits[i].home * 3 + cits[i].job * 5 + cits[i].inPermanent + cits[i].hobby;
  for (int i = 1; i < MAX_PERMS; i++)
    if (perms[i].flags & PF_USED) h = h * 7 + perms[i].cls + perms[i].upgrades * 3 + perms[i].mode;
  return h;
}
static void every_class(int days) {
  printf("== every class\n");
  rnd_seed(1234);
  new_city("FreePlayEmptyCanvas");
  int ncls = 0;
  for (int c = 0; c < npcls; c++)
    if (pcls[c].kind == KIND_BUILDING) ncls++;
  int cols = ncls + 24;
  int wa = world_add(0, 400, cols * 20, 120, 11, 0);
  int wb = world_add(cols * 20 + 200, 300, 400, 100, 12, 0);
  world_add(-20, 600, cols * 20 + 640, 0, 0, 0);   /* (the bottom world) */
  plenty();
  for (int u = 0; u < U_COUNT; u++) unlocks_unlock_uid(npcls + u);
  int col = 0, built = 0;
  for (int c = 0; c < npcls; c++) {
    if (pcls[c].kind != KIND_BUILDING) continue;
    if (pcls[c].base & B_ROOFTOP) {
      bld_build_at(wa, col, 0, pcls_find("NormalHouse"), false);
      built += bld_build_at(wa, col, 1, c, false) != 0;
    } else {
      built += bld_build_at(wa, col, 0, c, false) != 0;
    }
    col++;
  }
  /* neighbours that merge, stacks, trees, flying and teleporting */
  for (int k = 0; k < 3; k++) build(wa, col + k, 0, "BotanicalGardens"), build(wa, col + k, 1, "BotanicalGardens");
  col += 3;
  for (int k = 0; k < 3; k++) build(wa, col, k, "RedwoodTree");
  col++;
  build(wa, col, 0, "HyperElevator"), build(wa, col, 1, "HyperElevator"), build(wa, col, 2, "Library");
  col++;
  build(wa, col++, 0, "Teleporter");
  build(wa, col, 0, "NormalHouse"), build(wa, col++, 1, "LandingSite");
  build(wa, col++, 0, "ScrapyardNightClub"), build(wa, col++, 0, "ScrapyardNightClub");
  build(wa, col++, 0, "HoloGameHall");
  build(wa, col++, 0, "Pub");
  const char *res[] = {"Forest", "PineForest", "BranchyForest", "RedForest", "PurpleForest", "BirchForest", "Rock", "Boulder",
                       "AlienRuins", "AlienRuins", "ComputerAlienRuins"};
  for (int k = 0; k < (int)(sizeof res / sizeof res[0]) && col < cols; k++) perm_new(pcls_find(res[k]), wa, col++, 0);
  for (int k = 0; k < 6; k++) build(wb, k, 0, "NormalHouse");
  build(wb, 6, 0, "Teleporter");
  build(wb, 7, 0, "NormalHouse"), build(wb, 7, 1, "LandingSite");
  for (int k = 8; k < 12; k++) perm_new(pcls_find("Forest"), wb, k, 0);
  city_update_connections();
  sim_update_pathfinder(true, 0);
  people(wa, 260, 0, 70);
  people(wb, 30, 18, 50);
  saucer_spawn(wb, 4, 20, 30, -1);
  printf("  %d of %d building classes built, %d permanents, %d citizens\n", built, ncls, perm_count(), ncits);
  CHECK(built == ncls, "not every class was built");

  /* the decorations' looks, as the story makes them */
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *p = &perms[i];
    if (!(p->flags & PF_USED)) continue;
    if (p->cls == CLS_RooftopDecoration) bld_customize(p, 9, true), bld_customize_color(p, 0x3080FF);
    if (p->cls == CLS_ColoredBlock) bld_customize(p, 19, false), bld_customize_color(p, 0xFF0000);
    if (p->cls == CLS_Loft || p->cls == CLS_LandedExplorationShip) bld_customize(p, -1, true);
  }
  CHECK(bld_door_x(P(perm_at(wa, 0, 0))) >= 0, "door");

  int ticks = days * TICKS_PER_DAY, drawn = 0, k = 0;
  for (int t = 0; t < ticks; t++) {
    sim_tick(1.0f);
    if (t % 37 == 0) {
      /* a few windows and their buttons */
      for (int j = 0; j < 4; j++) {
        k = k % (MAX_PERMS - 1) + 1;
        if (perms[k].flags & PF_USED) exercise((uint16_t)k, (t / 37) % 3 != 0);
      }
    }
    if (t % 211 == 0) drawn += draw_all();
    if (t == ticks / 3) {
      /* research every city upgrade, turn every policy on, start a festival */
      for (int u = 0; u < U_COUNT; u++)
        if (upgrade_info[u].kind >= UK_CITY_UPGRADE) upgrade_press(NULL, u, false);
      upgrade_smart_labs_buy();
      for (int i = 1; i < MAX_PERMS; i++)
        if ((perms[i].flags & PF_USED) && perms[i].cls == CLS_FestivalHQ) PC(&perms[i])->ops->window_action(&perms[i], WA_SPECIAL);
      printf("  a third of the way: festival %s\n", festival_running() ? "running" : "not running");
    }
    if (t == 2 * ticks / 3)
      for (int u = 0; u < U_COUNT; u++)
        if (upgrade_info[u].kind == UK_POLICY && policy_on(u)) upgrade_press(NULL, u, false);
    plenty();
  }
  printf("  %d days: %d citizens, %d permanents; %d windows, %d buttons pressed, %d upgrade options seen, %d bought\n", days,
         ncits, perm_count(), windows_shown, buttons_pressed, options_seen, upgrades_bought);
  printf("  %d permanent drawings, %d parts outside their sprites\n", drawn, bld_bad_draws);
  CHECK(bld_bad_draws == 0, "drawings outside their sprites");
  int with_hobby = 0;
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].hobby) with_hobby++;
  printf("  citizens with a hobby: %d; story questions: ruins %.2f, research craft %.0f, hackers %.0f, decryptor %.0f\n",
         with_hobby, bld_alien_ruins_explored(), bld_research_bot_time(), bld_hacker_mission(), bld_decryptor_time());
  {
    int jobs = 0, workers = 0;
    for (int i = 1; i < MAX_PERMS; i++)
      if ((perms[i].flags & PF_USED) && is_a(&perms[i], B_WORK)) jobs += bld_jobs(&perms[i]), workers += perms[i].nworkers;
    printf("  jobs %d, workers %d, unemployed adults %d\n", jobs, workers, stats.laborForce - stats.peopleWithAJob);
    for (int i = 1; i < MAX_PERMS; i++)
      if ((perms[i].flags & PF_USED) && perms[i].cls == CLS_ResearchBot) {
        Cit *c = worker_at(&perms[i], 0);
        printf("  research craft %d (row %d): %d workers", i, perms[i].row, perms[i].nworkers);
        if (c) printf(", its pilot: action %d in %d at %.1f,%.1f flags %x", c->currentAction, c->inPermanent, c->relativeX, c->relativeY, c->flags);
        printf("\n");
      }
  }
  printf("  upgrades: SmartLights on %d buildings; BuildingRecycling bought %d; recycling %.2f\n",
         upgrades_buildings_with("SmartLights"), upgrades_city_bought("BuildingRecycling"), recycling_amount);
  CHECK(upgrades_city_bought("BirthControl") && upgrades.hasBirthControl, "research");

  /* a save and a load: the same city */
  mkdir("build", 0755);
  mkdir("build/bldtest-saves", 0755);
  host_save_dir("build/bldtest-saves");
  unsigned long before = city_sum();
  float speed = upgrades.computerChipFactorySpeed;
  CHECK(save_game(), "save");
  new_city("theLostShip");
  CHECK(load_game(), "load");
  unsigned long after = city_sum();
  printf("  save and load: %s (chip factory speed %.3f, then %.3f)\n", before == after ? "same city" : "DIFFERENT", speed,
         upgrades.computerChipFactorySpeed);
  CHECK(before == after && fabsf(speed - upgrades.computerChipFactorySpeed) < 1e-4f, "save and load");
  for (int t = 0; t < TICKS_PER_DAY / 2; t++) sim_tick(1.0f);
  draw_all();

  /* the windows' words, for a few */
  const char *show_classes[] = {"Laboratory", "Forest", "AlienRuins", "HackerHQ", "TheMachine", "FestivalHQ", "MachinePartsFactory"};
  for (int s = 0; s < (int)(sizeof show_classes / sizeof show_classes[0]); s++) {
    int cls = pcls_find(show_classes[s]);
    for (int i = 1; i < MAX_PERMS; i++)
      if ((perms[i].flags & PF_USED) && perms[i].cls == cls) {
        printf("  window of %s:\n", show_classes[s]);
        WinOut w = {.print = true};
        PC(&perms[i])->ops->window(&perms[i], &w);
        break;
      }
  }
}

/* ---------------------------------------------------------------- 2: production rates */
/* a small city on one world: houses for n workers, the buildings given, and n adults */
static int small_city(int cols) {
  new_city("FreePlayEmptyCanvas");
  rnd_seed(77);
  int w = world_add(0, 300, cols * 20, 100, 5, 0);
  world_add(-20, 500, cols * 20 + 40, 0, 0, 0);
  return w;
}
static void settle(int w, int n) {
  plenty();
  city_update_connections();
  sim_update_pathfinder(true, 0);
  people(w, n, 25, 35);
}
/* workers of class cls at work inside a permanent passing test, this tick */
typedef bool (*Where)(const Perm *in, const Cit *c);
static bool any_path;   /* gatherers and farmers work along scripted paths: count them too */
static int at_work(int cls, Where where, float *edu_sum) {
  int n = 0;
  if (edu_sum) *edu_sum = 0;
  for (int i = 1; i < MAX_CITS; i++) {
    const Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE) || !c->job || P(c->job)->cls != cls || c->currentAction != 0) continue;
    if (!any_path && (c->flags & (CF_HAS_PATH | CF_REQUESTING_PATH))) continue;
    const Perm *in = P(c->inPermanent);
    if (!in || !where(in, c)) continue;
    n++;
    if (edu_sum) *edu_sum += cit_education_speed_modifier(c);
  }
  return n;
}
static bool in_lab(const Perm *in, const Cit *c) { return in->cls == CLS_Laboratory && !cit_is_at_ground_level(c); }
static bool in_observatory(const Perm *in, const Cit *c) { (void)c; return in->cls == CLS_Observatory; }
static bool in_forest(const Perm *in, const Cit *c) { (void)c; return is_a(in, B_FOREST); }
static bool in_rock(const Perm *in, const Cit *c) { (void)c; return is_a(in, B_ROCK); }
static bool in_farm(const Perm *in, const Cit *c) { (void)c; return in->cls == CLS_IndoorFarm; }
static bool in_mpf(const Perm *in, const Cit *c) { (void)c; return in->cls == CLS_MachinePartsFactory; }

static void report(const char *what, double measured, double expected, int days, int units, const char *unit, double tol) {
  double per = measured / days / units, exp_per = expected / days / units;
  double ratio = expected > 0 ? measured / expected : 0;
  printf("  %-34s %8.3f %s/day each; formula %8.3f (ratio %.3f)\n", what, per, unit, exp_per, ratio);
  if (tol > 0) CHECK(fabs(ratio - 1) <= tol, "%s off by more than %.0f%%", what, tol * 100);
}

static void rates(int days) {
  printf("== production over %d days (formula: the original's, over the time the workers were at it)\n", days);
  /* Laboratory: 0.00125 x timeMod x speed x labSpeed x education per tick, upstairs */
  {
    int w = small_city(8);
    build(w, 0, 0, "NormalHouse"), build(w, 1, 0, "NormalHouse");
    build(w, 3, 0, "Laboratory"), build(w, 4, 0, "Laboratory");
    settle(w, 4);
    Meter m = {0};
    double exp = 0;
    for (int t = 0; t < days * TICKS_PER_DAY; t++) {
      float e;
      at_work(CLS_Laboratory, in_lab, &e);
      exp += 0.00125 * TICK_TIMEMOD * happiness.actionSpeedModifier * bonuses.labSpeed * e;
      sim_tick(1.0f);
      meter(&m, M_KNOWLEDGE);
    }
    report("knowledge per lab", m.total, exp, days, 2, "", 0.05);
  }
  /* Observatory: 0.002 x timeMod x speed x labSpeed x education per tick */
  {
    int w = small_city(8);
    build(w, 0, 0, "NormalHouse"), build(w, 1, 0, "NormalHouse");
    build(w, 3, 0, "Observatory");
    settle(w, 4);
    Meter m = {0};
    double exp = 0;
    for (int t = 0; t < days * TICKS_PER_DAY; t++) {
      float e;
      at_work(CLS_Observatory, in_observatory, &e);
      exp += 0.002 * TICK_TIMEMOD * happiness.actionSpeedModifier * bonuses.labSpeed * e;
      sim_tick(1.0f);
      meter(&m, M_KNOWLEDGE);
    }
    report("knowledge per observatory", m.total, exp, days, 1, "", 0.05);
  }
  /* MachinePartsFactory: 0.0015 x timeMod x workers x speed x bonus per tick, 3 stone each */
  {
    int w = small_city(8);
    build(w, 0, 0, "NormalHouse"), build(w, 1, 0, "NormalHouse");
    uint16_t f = build(w, 3, 0, "MachinePartsFactory");
    settle(w, 4);
    city.materials.m[M_STONE] = 1e6;
    Meter m = {0};
    double exp = 0, stone0 = city.materials.m[M_STONE];
    for (int t = 0; t < days * TICKS_PER_DAY; t++) {
      int n = at_work(CLS_MachinePartsFactory, in_mpf, NULL);
      exp += 0.0015 * TICK_TIMEMOD * n * happiness.actionSpeedModifier * bonuses.machinePartsFactorySpeed;
      sim_tick(1.0f);
      meter(&m, M_MACHINEPARTS);
    }
    report("machine parts per factory", m.total, exp, days, 1, "", 0.05);
    double used = stone0 - city.materials.m[M_STONE];
    printf("  %-34s %8.3f (formula 3)\n", "stone per machine part", m.total > 0 ? used / m.total : 0);
    CHECK(m.total > 0 && fabs(used / m.total - 3) < 0.01, "stone per part");
    /* its upgrade: two stone per part */
    upgrade_press(P(f), U_MachinePartsFactoryEfficiency, false);
    stone0 = city.materials.m[M_STONE];
    Meter m2 = {stats.materialProduction[M_MACHINEPARTS][0], 0};
    for (int t = 0; t < TICKS_PER_DAY; t++) sim_tick(1.0f), meter(&m2, M_MACHINEPARTS);
    used = stone0 - city.materials.m[M_STONE];
    printf("  %-34s %8.3f (formula 2)\n", "  with Scrap Metal Recycling", m2.total > 0 ? used / m2.total : 0);
    CHECK(m2.total > 0 && fabs(used / m2.total - 2) < 0.01, "upgrade efficiency");
  }
  any_path = true;
  /* WoodcuttingCentre: 0.2 x earlyFix wood per cut, a cut every getInt(60, 90) / speed ticks at the forest
   * (the bound counts no walking inside the forest) */
  {
    int w = small_city(12);
    build(w, 0, 0, "NormalHouse"), build(w, 1, 0, "NormalHouse");
    build(w, 2, 0, "WoodcuttingCentre");
    for (int k = 4; k < 10; k++) perm_new(pcls_find("Forest"), w, k, 0);
    settle(w, 4);
    Meter m = {0};
    double exp = 0, ticks_at = 0;
    for (int t = 0; t < days * TICKS_PER_DAY; t++) {
      int n = at_work(CLS_WoodcuttingCentre, in_forest, NULL);
      ticks_at += n * TICK_TIMEMOD;
      exp += n * TICK_TIMEMOD * 0.2 * early_fix() * happiness.actionSpeedModifier / 74.5;
      sim_tick(1.0f);
      meter(&m, M_WOOD);
    }
    report("wood per woodcutter", m.total, exp, days, 2, "", 0);
    printf("  %-34s %8.1f ticks a day at the forest (of %d working)\n", "", ticks_at / days / 2, (int)(13 * 60 / city.minutesPerTick));
    CHECK(m.total > 0 && m.total <= exp * 1.02, "wood above the bound");
  }
  /* StoneMine: 0.2 x stoneMiningSpeed x earlyFix stone per blow, a blow every getInt(90, 120) / speed ticks */
  {
    int w = small_city(12);
    build(w, 0, 0, "NormalHouse"), build(w, 1, 0, "NormalHouse");
    uint16_t mine = build(w, 2, 0, "StoneMine");
    for (int k = 4; k < 8; k++) perm_new(pcls_find("Rock"), w, k, 0);
    settle(w, 4);
    upgrade_press(P(mine), U_BetterPickaxes, false);
    Meter m = {0};
    double exp = 0;
    for (int t = 0; t < days * TICKS_PER_DAY; t++) {
      int n = at_work(CLS_StoneMine, in_rock, NULL);
      exp += n * TICK_TIMEMOD * 0.2 * 1.2 * upgrades.stoneMiningSpeed * early_fix() * happiness.actionSpeedModifier / 104.5;
      sim_tick(1.0f);
      meter(&m, M_STONE);
    }
    report("stone per miner (Better Pickaxes)", m.total, exp, days, 2, "", 0);
    CHECK(m.total > 0 && m.total <= exp * 1.02, "stone above the bound");
  }
  /* IndoorFarm: 25 food a harvest; a cycle is about 74 growing (45 ticks), 100 harvesting (22.5) and 20 cleaning
   * actions (45), plus the crops' own growth in day light */
  {
    int w = small_city(8);
    build(w, 0, 0, "NormalHouse"), build(w, 1, 0, "NormalHouse");
    build(w, 3, 0, "IndoorFarm"), build(w, 4, 0, "IndoorFarm");
    settle(w, 4);
    Meter m = {0};
    double exp = 0;
    for (int t = 0; t < days * TICKS_PER_DAY; t++) {
      int n = at_work(CLS_IndoorFarm, in_farm, NULL);
      exp += n * TICK_TIMEMOD * happiness.actionSpeedModifier * 25.0 / (74 * 45 + 100 * 22.5 + 20 * 45);
      sim_tick(1.0f);
      meter(&m, M_FOOD);
    }
    report("food per indoor farm", m.total, exp, days, 2, "", 0);
  }
}

int main(int argc, char **argv) {
  scratch = &scr;
  int days = argc > 1 ? atoi(argv[1]) : 6;
  every_class(days);
  rates(days);
  printf("%s (%d failures)\n", failures ? "FAIL" : "all good", failures);
  return failures != 0;
}
