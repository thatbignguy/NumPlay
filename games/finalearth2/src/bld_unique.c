/* The Final Earth 2: unique buildings (buildings.md 11): the workshop, the
 * research craft, observatory, art colony, city hall, research centers, the
 * hackers', the Secret Society's and the hippies' missions, the escape room,
 * the misdirector, the machine, the festival HQ and its music festivals
 * (simulation.festival.*), the supercomputer, the fossil museum, the beacon. */
#include <stdio.h>
#include "bldi.h"
#include "lang_data.h"
#include "save.h"

static inline float speed(void) { return happiness.actionSpeedModifier; }
static inline float edu(const Cit *c) { return cit_education_speed_modifier(c); }
static void walk_wait(Cit *c, int xmin, int xmax, int tmin, int tmax, bool slow) {
  int x = rnd_between(xmin, xmax);
  cit_move_and_wait(c, x, rnd_between(tmin, tmax), THEN_NONE, 0, false, slow);
}
static void plain_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  draw_upgrades(b, p, layer, x, y, true);
}
static int count_class(int cls) {
  int n = 0;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == cls) n++;
  return n;
}
static Perm *first_class(int cls) {
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == cls) return &perms[i];
  return NULL;
}
static bool any_day_at_least(int m, double v) {
  for (int d = 0; d < STATS_DAYS; d++)
    if (stats.materialProduction[m][d] >= v) return true;
  return false;
}
/* the knowledge it makes, also added to its total (TotalState, the start of its state) */
typedef struct PACKED { float total; } TotalState;
STATE_FITS(TotalState);
static float knowledge(Perm *p, float k) {
  produce(M_KNOWLEDGE, k);
  if (p) ST(TotalState, p)->total += k;
  return k;
}
uint8_t modern_artworks_unlocked = 4;   /* unlocks.numberOfModernArtMuseumArtworksUnlocked */

/* ---------------------------------------------------------------- Workshop */
typedef struct PACKED { float totalKnowledge, totalMaterialUsed, materialMade; } WorkshopState;
STATE_FITS(WorkshopState);
static void workshop_init(Perm *p) { upgrade_first_mode(p); }
static void workshop_on_build(Perm *p) { (void)p; bonuses.machinePartsFactorySpeed *= 1.25f; }
static void workshop_destroy(Perm *p) { (void)p; bonuses.machinePartsFactorySpeed /= 1.25f; }
static void workshop_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  if (worker_index(c) == 0) {
    if (rnd_int(5) < 4) walk_wait(c, 4, 8, 60, 120, true);
    else walk_wait(c, 11, 14, 60, 120, true);
  } else if (c->relativeY < 2) {
    cit_change_floor(c, THEN_NONE, 0);
  } else if (rnd_int(2) == 0) {
    walk_wait(c, 3, 6, 60, 120, true);
  } else {
    walk_wait(c, 13, 14, 60, 120, true);
  }
  WorkshopState *s = ST(WorkshopState, p);
  if (p->mode == MODE_Workshop_WorkshopKnowledge) {
    s->totalKnowledge += knowledge(NULL, 0.15f * edu(c) * boost());
  } else if (city.materials.m[M_WOOD] >= 1) {
    float made = 0.075f * edu(c) * boost();
    produce(M_MACHINEPARTS, made);
    s->materialMade += made;
    float used = made * 2;
    city.materials.m[M_WOOD] -= used;
    stats_add_used(M_WOOD, used);
    s->totalMaterialUsed += used;
  }
}
static void workshop_window(Perm *p, WinOut *w) {
  const WorkshopState *s = STC(WorkshopState, p);
  char a[64], b[96], t[170];
  lo1(a, sizeof a, "[0] knowledge gathered.", (int)s->totalKnowledge);
  lo2(b, sizeof b, "[0] wood used to make [1] machine parts.", (int)s->totalMaterialUsed, (int)s->materialMade);
  int k = 0;
  for (const char *q = a; *q; q++) t[k++] = *q;
  t[k++] = ' ';
  for (const char *q = b; *q && k < 168; q++) t[k++] = *q;
  t[k] = 0;
  win_line(w, t);
}
const PermOps ops_Workshop = {.init = workshop_init, .on_build = workshop_on_build, .destroy = workshop_destroy,
                              .work = workshop_work, .draw = plain_draw, .window = workshop_window};

/* ---------------------------------------------------------------- ResearchBot (the research craft) */
typedef struct PACKED {
  uint16_t timeWorking;        /* ticks */
  int16_t y4;                  /* flyingSaucerY x 4 */
  int8_t xprog4;               /* flyingSaucerXProg x 4 */
  int16_t xi, target;          /* flyingSaucerXI and flyingSaucerTargetX (indexes of highestYPositions) */
  uint8_t waiting;
  uint8_t flags;               /* RB_FLYING, RB_RETURNING; above them timeWorking's 64ths */
} BotState;
STATE_FITS(BotState);
enum { RB_FLYING = 1, RB_RETURNING = 2 };
static float bot_time(const BotState *s) { return s->timeWorking + (s->flags >> 2) / 64.0f; }
float research_bot_time_working(const Perm *p) { return bot_time(STC(BotState, p)); }
static int bot_home_index(const Perm *p) {
  (void)flying_columns();   /* (the grid of columns as now) */
  int x;
  flying_highest_y(0, &x);
  return perm_city_x(p) / 20 - x / 20;   /* (position.x / 20 | 0) - gridIStart - 1 */
}
/* the highest obstruction of a column, kept for the column the craft is over and the next */
static struct { uint16_t bot; int16_t xi; int32_t y; } bot_cache[2];
static int bot_highest(const Perm *p, int xi) {
  for (int k = 0; k < 2; k++)
    if (bot_cache[k].bot == pid(p) && bot_cache[k].xi == xi) return bot_cache[k].y;
  int x, n = flying_columns();
  int y = xi >= 0 && xi < n ? flying_highest_y(xi, &x) : -1000001;
  bot_cache[1] = bot_cache[0];
  bot_cache[0].bot = pid(p), bot_cache[0].xi = (int16_t)xi, bot_cache[0].y = y;
  return y;
}
static void bot_init(Perm *p) {
  BotState *s = ST(BotState, p);
  s->y4 = (int16_t)(perm_city_y(p) * 4);
}
static void bot_city_change(Perm *p) {
  BotState *s = ST(BotState, p);
  memset(bot_cache, 0, sizeof bot_cache);
  if (!(s->flags & RB_FLYING)) {
    s->xi = (int16_t)bot_home_index(p);
    s->y4 = (int16_t)(perm_city_y(p) * 4);
  }
}
static void bot_update(Perm *p, float timeMod) {
  BotState *s = ST(BotState, p);
  if (!(s->flags & RB_FLYING)) return;
  float t = bot_time(s) + timeMod;
  if (t > 65535) t = 65535;
  s->timeWorking = (uint16_t)t;
  s->flags = (uint8_t)((s->flags & 3) | (int)((t - s->timeWorking) * 64) << 2);
  if (s->waiting > 0) {
    s->waiting = tick_down(s->waiting, timeMod);
  } else {
    float y = s->y4 / 4.0f, xprog = s->xprog4 / 4.0f;
    int dir = s->target > s->xi ? 1 : s->target < s->xi ? -1 : 0;
    if (dir == 0) {
      float next = (float)bot_highest(p, s->xi) - 40;
      if (s->flags & RB_RETURNING) next += 40;
      if (y < next && next >= -1000000) {
        y += 3 * timeMod;
        if (y > next) y = next;
      } else {
        if (next >= -1000000) y = next;
        if (s->flags & RB_RETURNING) {
          s->flags &= (uint8_t)~(RB_FLYING | RB_RETURNING);
          s->waiting = 60;
        } else {
          s->target = (int16_t)bot_home_index(p);
          s->flags |= RB_RETURNING;
          s->waiting = 60;
        }
      }
    } else {
      float next = (float)bot_highest(p, s->xi + dir) - 40;
      if (y > next && next >= -100000) {
        y -= 3 * timeMod;
        if (y < next) y = next;
      } else {
        xprog += 3 * dir * timeMod;
        if (dir * xprog > 20) xprog = 0, s->xi = (int16_t)(s->xi + dir);
      }
    }
    if (y < -8000) y = -8000;
    if (y > 8000) y = 8000;
    s->y4 = (int16_t)(y * 4);
    s->xprog4 = (int8_t)(xprog * 4);
  }
  /* the pilot rides in the craft */
  Cit *pilot = worker_at(p, 0);
  if (pilot && pilot->inPermanent == pid(p)) {
    int x;
    flying_highest_y(s->xi, &x);
    float sx = x + s->xprog4 / 4.0f, sy = s->y4 / 4.0f;
    set_rel(pilot, sx - perm_city_x(p) + 9, -(sy - perm_city_y(p)) + 3);
  }
}
static void bot_work(Perm *p, Cit *c, float timeMod, bool stop) {
  BotState *s = ST(BotState, p);
  if (stop && !(s->flags & RB_FLYING)) {
    c->currentAction = 2;
    return;
  }
  if (s->flags & RB_FLYING) {
    produce(M_KNOWLEDGE, 0.00125f * timeMod * speed() * edu(c) * boost());
    return;
  }
  float spd = c->pathWalkSpeed / 100.0f * timeMod;
  if (fabsf(9 - c->relativeX) >= spd) {
    c->relativeX += (9 > c->relativeX ? 1 : -1) * spd;
    return;
  }
  c->relativeX = 9;
  c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
  s->flags |= RB_FLYING;
  s->target = (int16_t)rnd_int(flying_columns());
  s->xi = (int16_t)bot_home_index(p);
  if (story_is("hackersandaliens") && s->timeWorking < 60) {
    int x;
    flying_highest_y(0, &x);
    s->target = (int16_t)(-36 / 20 - x / 20);   /* ((-36 | 0) / 20...) as written: the column of x -36 */
  }
  s->y4 = (int16_t)(perm_city_y(p) * 4);
}
static void bot_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer == LAYER_MAIN && !(STC(BotState, p)->flags & RB_FLYING)) dr_sprite(b, SPR_RESEARCHBOT, x, y, 0, 0, 0, 255);
}
const PermOps ops_ResearchBot = {.init = bot_init, .on_city_change = bot_city_change, .update = bot_update, .work = bot_work,
                                 .draw = bot_draw};
#include "view.h"
/* the craft in the air (drawn wherever it is) */
void research_bot_draw_free(const GfxBand *b, int layer) {
  if (layer != LAYER_MAIN) return;
  for (int i = 1; i < MAX_PERMS; i++) {
    const Perm *p = &perms[i];
    if (!(p->flags & PF_USED) || p->cls != CLS_ResearchBot || !(STC(BotState, p)->flags & RB_FLYING)) continue;
    const BotState *s = STC(BotState, p);
    int x;
    flying_highest_y(s->xi, &x);
    float cx = x + s->xprog4 / 4.0f, cy = s->y4 / 4.0f;
    gfx_band_sprite(b, SPR_RESEARCHBOT, 0, 0, 20, 20, bg_screen_x(&camera, cx), bg_screen_y(&camera, cy), 0, 255, GFX_NO_TINT);
  }
}

/* ---------------------------------------------------------------- Observatory */
static void observatory_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop) {
    c->currentAction = 2;
    return;
  }
  switch (worker_index(c)) {
  case 0: step_to_x(c, 5, timeMod); break;
  case 1: step_to_x(c, 13, timeMod); break;
  case 2: step_to_x(c, 9, timeMod); break;
  }
  float per = 0.002f * (has_upgrade_bit(p, UB_Observatory_StarDatabase) ? upgrades.starDatabaseIsMoreEffective ? 6 : 1.5f : 1);
  knowledge(p, per * timeMod * speed() * bonuses.labSpeed * edu(c) * boost());
}
static void total_window(Perm *p, WinOut *w) { win_line1(w, "[0] knowledge gathered.", (int)STC(TotalState, p)->total); }
const PermOps ops_Observatory = {.work = observatory_work, .draw = plain_draw, .window = total_window};

/* ---------------------------------------------------------------- ArtColony */
typedef struct PACKED { float progress; uint8_t texture; } ArtColonyState;
static const uint8_t unlock_new_work_after[6] = {4, 7, 8, 10, 13, 13};
static float art_colony_capacity(const Perm *p) { return p->nworkers * 50.0f; }
static bool art_colony_open(const Perm *p) {
  float h = hour_now();
  return p->nworkers > 0 && first_worker_working(p) && day_number() % 3 == 0 && h < 21.5f && h > 15.0f;
}
static uint8_t TH_ART_MOVE;
static void th_art_move(Cit *c, uint16_t arg) { (void)arg; walk_wait(c, 3, 16, 50, 100, true); }
static void art_colony_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (c->relativeY < 5) cit_change_floor(c, TH_ART_MOVE, 0);
  else th_art_move(c, 0);
}
static void art_colony_walk(Perm *p, Cit *c, float steps) {
  (void)p;
  int r = rnd_int(3);
  if (r == 0 && steps > 120) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  else if (r == 1) walk_wait(c, 3, 15, 30, 60, false);
  else wait_ticks(c, 90, 120);
}
static void art_colony_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop) c->currentAction = 2;   /* (as written: it carries on) */
  int place = (worker_index(c) + day_number()) % 5;
  bool redo = rnd_float() < 0.015f * timeMod;
  if (place == 0 || place == 3) {
    if (c->relativeY > 5) cit_change_floor(c, THEN_NONE, 0);
    else if (place == 0) {
      if (c->relativeX < 3 || c->relativeX > 6 || redo) walk_wait(c, 3, 7, 5, 10, false);
    } else if (c->relativeX < 12 || c->relativeX > 15 || redo) walk_wait(c, 12, 16, 5, 10, false);
  } else if (c->relativeY < 5) {
    cit_change_floor(c, THEN_NONE, 0);
  } else if (place == 1) {
    if (c->relativeX < 3 || c->relativeX > 6 || redo) walk_wait(c, 3, 7, 5, 10, false);
  } else if (place == 2) {
    if (c->relativeX < 12 || c->relativeX > 15 || redo) walk_wait(c, 12, 16, 5, 10, false);
  } else if (place == 4) {
    if (c->relativeX < 8 || c->relativeX > 10 || redo) walk_wait(c, 8, 11, 5, 10, false);
  }
  ArtColonyState *s = ST(ArtColonyState, p);
  if (s->progress >= 1.0f) {
    s->texture++;
    int n = 0;
    for (int i = 0; i < 6; i++)
      if (s->texture > unlock_new_work_after[i]) n++;
    if (4 + n > modern_artworks_unlocked) modern_artworks_unlocked = (uint8_t)(4 + n);
    s->texture %= (uint8_t)frames_of(SPR_ARTCOLONYWORK, 20);
    s->progress = 0;
  }
  s->progress += timeMod * edu(c) * speed() * 0.00002f;
}
static void art_colony_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer == LAYER_BG) dr_frame(b, SPR_ARTCOLONYWORK, 20, STC(ArtColonyState, p)->texture, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
const PermOps ops_ArtColony = {.work = art_colony_work, .walk_around = art_colony_walk, .is_open = art_colony_open,
                               .entertainment_capacity = art_colony_capacity, .be_entertained = art_colony_be_entertained,
                               .draw = art_colony_draw};

/* ---------------------------------------------------------------- CityHall */
static void city_hall_init(Perm *p) { upgrade_first_mode(p); }
static void city_hall_destroy(Perm *p) { (void)p; upgrades.nightClubEntertainmentQualityBonus = 0; }
static void city_hall_update(Perm *p, float timeMod) {
  upgrades.nightClubEntertainmentQualityBonus = 0;
  if (p->mode == MODE_CityHall_CensusData) knowledge(p, p->nworkers * (float)ncits * timeMod * 0.0000005f);
  else upgrades.nightClubEntertainmentQualityBonus = p->nworkers * 2.0f;
}
static void city_hall_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p, (void)timeMod;
  if (stop) c->currentAction = 2;   /* (as written: a walk starts anyway) */
  int r = rnd_int(7);
  if (c->relativeY < 5) {
    if (r < 3) cit_change_floor_and_wait_random(c, 20, 30, THEN_NONE, 0);
    else if (r < 5) walk_wait(c, 3, 7, 90, 180, false);
    else walk_wait(c, 12, 15, 90, 180, false);
  } else if (r < 1) cit_change_floor_and_wait_random(c, 20, 30, THEN_NONE, 0);
  else if (r < 3) walk_wait(c, 3, 4, 90, 180, false);
  else if (r < 5) walk_wait(c, 9, 10, 90, 180, false);
  else walk_wait(c, 15, 16, 90, 180, false);
}
/* knowledge gathered; after the modes and policies (createWindowAddBottomButtons): the unhappiness warnings'
 * setting, the city's name and its button */
static void city_hall_window(Perm *p, WinOut *w) {
  total_window(p, w);
  win_later(w);
  win_setting(w, happiness.ignoreUnhappyCitizens, WA_TOGGLE, L_ignore_unhappy_citizens);
  win_space(w, 4);
  win_heading(w, L_city_name, -1, -1, false, false, NULL, NULL);
  win_para(w, FONT_15, city.cityName[0] ? city.cityName : L_unnamed_city, 300, 0);
  win_button(w, 0, L_change_city_name, WA_UI_CITY_NAME, true);
  win_space(w, 4);
}
static void city_hall_action(Perm *p, int a) {
  (void)p;
  if (a == WA_TOGGLE) happiness.ignoreUnhappyCitizens = !happiness.ignoreUnhappyCitizens;
}
const PermOps ops_CityHall = {.init = city_hall_init, .destroy = city_hall_destroy, .update = city_hall_update,
                              .work = city_hall_work, .draw = plain_draw, .window = city_hall_window, .window_action = city_hall_action};
void city_hall_post_create(Perm *p) { (void)p; }   /* (its first mode is already there) */

/* ---------------------------------------------------------------- StoneResearchCenter */
static float stone_bonus(Perm *p) {
  bool tech = has_upgrade_bit(p, UB_StoneResearchCenter_PickaxeTech);
  double k = stats.materialProduction[M_KNOWLEDGE][1];
  double d = tech ? 195 : 200, m = tech ? 0.1 : 0.075, l = tech ? 1.65 : 2;
  double bonus = k < d ? k / d * 0.15 : log(k / d) / log(l) * m + 0.15;
  if (tech) bonus += 2.5e-5 * k;
  int jobs = bld_jobs(p);
  bonus *= jobs ? (double)p->nworkers / jobs : 0;
  if (bonus < 0) bonus = 0;
  upgrades.stoneMiningSpeed = (float)(1.0 + bonus);
  ST(TotalState, p)->total = (float)bonus;   /* currentBonus */
  return (float)bonus;
}
static void stone_center_update(Perm *p, float timeMod) { (void)timeMod; stone_bonus(p); }
static void stone_center_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p, (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  int r = rnd_int(3);
  if (r == 0) cit_change_floor_and_wait(c, 50, THEN_NONE, 0);
  else if (r == 1) walk_wait(c, 3, 7, 50, 80, false);
  else walk_wait(c, 12, 16, 50, 80, false);
}
static void stone_center_window(Perm *p, WinOut *w) {
  win_line1(w, "Current stone mining boost: [0]%", (int)(stone_bonus(p) * 100));
  win_line(w, "This is based on the number of workers and on yesterday's knowledge production.");
  if (upgrades.stoneResearchCenterWithFossils == pid(p)) win_line1(w, "Fossils found: [0]", bonuses.fossilsCollected);
}
static void stone_center_destroy(Perm *p) {
  if (upgrades.stoneResearchCenterWithFossils == pid(p)) upgrades.stoneResearchCenterWithFossils = 0;
}
const PermOps ops_StoneResearchCenter = {.destroy = stone_center_destroy, .update = stone_center_update,
                                         .work = stone_center_work, .draw = plain_draw, .window = stone_center_window};
void stone_center_post_create(Perm *p) { stone_bonus(p); }

/* ---------------------------------------------------------------- ComputerResearchCenter (a Factory) */
void factory_work(Perm *p, Cit *c, float timeMod, bool stop);
static void crc_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer == LAYER_BG) factory_back_any(b, p, x, y, SPR_COMPUTERRESEARCHCENTER_FRAMES, SPR_COMPUTERRESEARCHCENTER_IDLE);
}
static void crc_window(Perm *p, WinOut *w) { win_line1(w, "[0] knowledge gathered.", (int)STC(FactoryState, p)->materialMade); }
const PermOps ops_ComputerResearchCenter = {.work = factory_work, .update = factory_update_any, .draw = crc_draw,
                                            .window = crc_window};

/* ---------------------------------------------------------------- LivingResearchCenter */
static void lrc_walk(Perm *p, Cit *c, float steps) {
  (void)p, (void)steps;
  int r = rnd_int(3);
  if (c->relativeY < 5) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  else if (r == 1) {
    if (rnd_int(2) == 0) walk_wait(c, 3, 7, 30, 60, false);
    else walk_wait(c, 12, 16, 30, 60, false);
  } else wait_ticks(c, 90, 120);
}
static void lrc_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  if (c->relativeY < 2) {
    if (rnd_int(2) == 0) walk_wait(c, 3, 6, 60, 120, true);
    else walk_wait(c, 13, 14, 60, 120, true);
  } else if (rnd_int(2) == 0) {
    walk_wait(c, 3, 15, 60, 120, true);
  }
  knowledge(p, 0.10799999999999998f * edu(c) * boost());
}
/* knowledge gathered; after the upgrades, the button to upgrade every basic house (ReplaceAllHousesWindow) */
static void lrc_window(Perm *p, WinOut *w) {
  total_window(p, w);
  win_later(w);
  win_button(w, 0, L_upgrade_basic_houses, WA_UI_REPLACE_HOUSES, true);
}
const PermOps ops_LivingResearchCenter = {.work = lrc_work, .walk_around = lrc_walk, .draw = plain_draw, .window = lrc_window};

/* ---------------------------------------------------------------- missions (HackerHQ, SecretSocietyHouse, BlossomHippieHQ) */
static const char *const hacker_missions[11] = {
    "Collect the Knowledge:\nBuild twenty Labs.", "Spread the Knowledge:\nBuild three Libraries.",
    "House the Smart:\nBuild five Tinkerers' Houses.",
    "Factory Hack:\nResearch the Firmware Modding upgrade at a Refined Metals Factory.",
    "Try our latest invention:\nBuild a Factory Pub.", "More of the basics:\nProduce at least 20 computer chips in a day.",
    "Mechanical Party:\nBuild seven Scrapyard Nightclubs.", "Let's have even more fun!\nBuild the Lab Escape Room.",
    "Watering Cans Are Overrated:\nResearch Automatic Water Management in a Tree Plantation.",
    "Change the Game:\nBuild two Holo Game Halls.", "Build our Ultimate Invention:\nBuild the Misdirector."};
static const char *const society_missions[8] = {
    "Improve Our Home:\nResearch Secretive Living in the Living Research Center.",
    "Increase Production:\nProduce 1000 Wood in one day.", "Green the City:\nBuild fifty Botanical Gardens.",
    "Display the Statues:\nBuild twelve Sculpture Gardens.", "Improve Transportation:\nBuild seven Teleporters.",
    "House the Rich:\nBuild five Villas.", "Know the Hidden:\nGain at least 1000 Knowledge in a day.",
    "Finish Our Project:\nBuild the Machine."};
static const char *const hippie_missions[8] = {
    "Flower Link:\nBuild at least two teleporters each surrounded by Botanical Gardens.",
    "Flowery Dining:\nBuild three Blossom Restaurants.", "Actual Nature:\nHave at least one protected forest.",
    "Together in Harmony:\nBuild twenty Community Houses.", "Organicize:\nBuild thirty Eco Farms.",
    "Don't Waste:\nBuild nine Farm By-Product Processors.",
    "Peaceful Society:\nResearch Societal Living in the Living Research Center.", "Make the City Sing:\nBuild the Festival HQ."};

typedef struct PACKED {
  uint8_t mission;
  uint8_t timeToCheck;         /* timeToCheckBuildings (ticks, 0 when due) */
  int8_t doingEvent;
  uint8_t eventPhase;
  uint16_t lastEventDay;
  uint16_t target;             /* SecretSocietyHouse: ownPub; BlossomHippieHQ: eventBotanicalGardens */
  uint8_t extra;               /* SecretSocietyHouse: manyTeleportAlpha x 20; hippies: canLeaveForBG */
  uint8_t flags;               /* SS_* */
} MissionState;
STATE_FITS(MissionState);
enum { SS_STATUE_GIVEN = 1, SS_SCENARIO = 2 };
static void check_time(MissionState *s, float timeMod) {
  s->timeToCheck = tick_down(s->timeToCheck, timeMod);
}
static void mission_lines(WinOut *w, const MissionState *s, int done, const char *done_text, const char *gather,
                          const char *const *texts, const Perm *p) {
  win_text(w, s->mission == done ? "Thank You" : "Current Task");
  if (s->mission >= done) win_text(w, done_text);
  else if (p->nworkers != bld_jobs(p)) win_text(w, gather);
  else win_text(w, texts[s->mission]);
}

/* HackerHQ */
int hacker_mission(const Perm *p) { return STC(MissionState, p)->mission; }
static void hacker_check(Perm *p) {
  MissionState *s = ST(MissionState, p);
  for (;;) {
    bool done = false;
    switch (s->mission) {
    case 3:
      done = unlock_state_name("RefinedMetalsFactoryHack") == UNLOCK_RESEARCHED;
      if (done) unlocks_unlock("FactoryPub");
      break;
    case 5:
      done = any_day_at_least(M_COMPUTERCHIPS, 20);
      if (done) unlocks_unlock("ScrapyardNightClub");
      break;
    case 8:
      done = unlock_state_name("AutomaticWaterManagement") == UNLOCK_RESEARCHED;
      if (done) unlocks_unlock("HoloGameHall");
      break;
    default:
      if (s->timeToCheck == 0 && s->mission != 11) {
        switch (s->mission) {
        case 0:
          done = count_class(CLS_Laboratory) + count_class(CLS_ExperimentationLab) >= 20;
          if (done) unlocks_unlock("Library");
          break;
        case 1:
          done = count_class(CLS_Library) >= 3;
          if (done) unlocks_unlock("TinkerersHome");
          break;
        case 2:
          done = count_class(CLS_TinkerersHome) >= 5;
          if (done) unlocks_unlock("RefinedMetalsFactoryHack");
          break;
        case 4: done = count_class(CLS_FactoryPub) >= 1; break;
        case 6:
          done = count_class(CLS_ScrapyardNightClub) >= 7;
          if (done) unlocks_unlock("EscapeRoom");
          break;
        case 7:
          done = count_class(CLS_EscapeRoom) >= 1;
          if (done) unlocks_unlock("AutomaticWaterManagement");
          break;
        case 9:
          done = count_class(CLS_HoloGameHall) >= 2;
          if (done) unlocks_unlock("Misdirector");
          break;
        case 10: done = count_class(CLS_Misdirector) >= 1; break;
        }
        s->timeToCheck = 30;
      }
    }
    if (!done) break;
    s->mission++;
    s->timeToCheck = 0;
  }
}
static void hacker_update(Perm *p, float timeMod) {
  hacker_check(p);
  check_time(ST(MissionState, p), timeMod);
}
static void hacker_walk_around(Perm *p, Cit *c, float steps) {
  (void)p, (void)steps;
  static const int8_t pos[4] = {3, 9, 12, 13};
  if (c->relativeY > 5) {
    if (rnd_int(4) == 1 && c->relativeX > 5) cit_change_floor(c, THEN_NONE, 0);
    else {
      int x = pos[rnd_int(4)];
      cit_move_and_wait(c, x, rnd_between(60, 120), THEN_NONE, 0, false, false);
    }
  } else if (rnd_int(4) == 1) cit_change_floor(c, THEN_NONE, 0);
  else if (rnd_int(3) == 1) walk_wait(c, 5, 7, 90, 150, false);
  else walk_wait(c, 9, 15, 60, 150, false);
}
static void hacker_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  hacker_walk_around(p, c, 150);
}
static void hacker_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  plain_draw(b, p, layer, x, y);
  /* the bolt, lit around 2:00 (not on the ground floor) */
  if (layer == LAYER_MAIN && p->row != 0) {
    int h = ((int)city.timeSinceStart / 60) % 24;
    if (h < 4) {
      float a = 4.0f - 4.0f * fabsf(1.0f - hour_now() / 2.0f);
      a = a < 0 ? 0 : a > 1 ? 1 : a;
      if (a > 0) dr_sprite(b, SPR_HACKERS_BOLT, x, y, 12, 11, 0, (uint8_t)(a * 255));
    }
  }
}
static void hacker_window(Perm *p, WinOut *w) {
  mission_lines(w, STC(MissionState, p), 11, "We'll keep on hacking!", "Gather the Hackers:\nMake sure all 6 jobs are filled.",
                hacker_missions, p);
}
const PermOps ops_HackerHQ = {.update = hacker_update, .work = hacker_work, .walk_around = hacker_walk_around,
                              .draw = hacker_draw, .window = hacker_window};

/* SecretSocietyHouse */
void key_friends_walk_around(Perm *p, Cit *c, float steps);
static void society_init(Perm *p) {
  MissionState *s = ST(MissionState, p);
  s->doingEvent = -1;
  if (story_is("cityofthekey")) s->flags |= SS_SCENARIO;
}
static void society_on_build(Perm *p) { (void)p; unlocks_unlock("SecretiveLiving"); }
static void society_check(Perm *p) {
  MissionState *s = ST(MissionState, p);
  for (;;) {
    bool done = false;
    switch (s->mission) {
    case 0: done = city_upgrade_bought(U_SecretiveLiving); break;
    case 1: done = any_day_at_least(M_WOOD, 1000); break;
    case 6:
      done = any_day_at_least(M_KNOWLEDGE, 1000);
      if (done) unlocks_unlock("TheMachine");
      break;
    }
    if (s->mission != 0 && s->mission != 1 && s->mission != 6 && s->mission < 8 && s->timeToCheck == 0) {
      switch (s->mission) {
      case 2:
        done = count_class(CLS_BotanicalGardens) >= 50;
        if (done) unlocks_unlock("StatueGarden");
        break;
      case 3: done = count_class(CLS_StatueGarden) >= 12; break;
      case 4:
        done = count_class(CLS_Teleporter) >= 7;
        if (done) unlocks_unlock("Villa");
        break;
      case 5: done = count_class(CLS_Villa) >= 5; break;
      case 7: done = count_class(CLS_TheMachine) >= 1; break;
      }
      s->timeToCheck = 30;
    }
    if (!done) break;
    s->mission++;
    s->timeToCheck = 0;
  }
}
static void society_update(Perm *p, float timeMod) {
  MissionState *s = ST(MissionState, p);
  if (s->doingEvent == 1 && s->eventPhase >= 1) {
    /* the members fade in after the mass teleport */
    if (s->extra < 20) s->extra++;
    else s->doingEvent = -1, s->eventPhase = 0;
  }
  if (!(s->flags & SS_SCENARIO)) society_check(p);
  check_time(s, timeMod);
  if (!(s->flags & SS_STATUE_GIVEN)) {
    uint16_t id = pid(p);
    for (int i = 1; i < MAX_CITS; i++)
      if ((cits[i].flags & CF_ALIVE) && cits[i].job == id && cit_age(&cits[i]) >= 500) {
        unlocks_unlock("StatueOfTheKey");
        s->flags |= SS_STATUE_GIVEN;
        break;
      }
  }
}
typedef bool (*IsGoal)(const Perm *p, const void *ctx);
typedef float (*Niceness)(const Perm *p);
uint16_t finder_quick_query(const Cit *c, IsGoal is_goal, const void *ctx, uint8_t key, float expiry_hours, Niceness niceness,
                            float max_distance);
uint16_t finder_query(uint16_t from, int from_world, float from_x, IsGoal is_goal, const void *ctx, Niceness niceness,
                      float max_distance, const Cit *citizen, float best_expected);
static bool goal_teleporter(const Perm *p, const void *ctx) { (void)ctx; return p->cls == CLS_Teleporter; }
static bool goal_unmarked_pub(const Perm *p, const void *ctx) { (void)ctx; return p->cls == CLS_Pub && !STC(PubState, p)->marked; }
static uint8_t TH_SOCIETY_FAIL;
static void th_society_fail(Cit *c, uint16_t arg) {
  (void)arg;
  Perm *h = P(c->job);
  if (h && h->cls == CLS_SecretSocietyHouse) ST(MissionState, h)->doingEvent = -1;
}
static void society_work(Perm *p, Cit *c, float timeMod, bool stop) {
  MissionState *s = ST(MissionState, p);
  uint16_t id = pid(p);
  if (s->doingEvent == 1) {
    if (s->eventPhase == 0) {
      /* all members standing at a teleporter: they all come out on the house's world */
      bool all = true;
      for (int i = 1; i < MAX_CITS && all; i++) {
        Cit *w = &cits[i];
        if (!(w->flags & CF_ALIVE) || w->job != id) continue;
        Perm *in = P(w->inPermanent);
        if (!in || in->cls != CLS_Teleporter || w->relativeX != 9 || (w->flags & (CF_HAS_PATH | CF_REQUESTING_PATH))) all = false;
      }
      if (all) {
        s->eventPhase = 1;
        s->extra = 0;
        for (int i = 1; i < MAX_CITS; i++) {
          Cit *w = &cits[i];
          if (!(w->flags & CF_ALIVE) || w->job != id) continue;
          Perm *in = P(w->inPermanent);
          if (in) {
            particle_add(SPR_TELEPORTER_RAY_KEY, 4, perm_city_x(in) + 8, perm_city_y(in) + 4);
            transport_teleport_used(in);
            leave_to_surface(w);
          }
          w->onWorld = p->world;
          set_rel(w, (float)rnd_int(worlds[p->world].w - 2), 0);
        }
      } else if (!c->inPermanent || P(c->inPermanent)->cls != CLS_Teleporter) {
        uint16_t tp = finder_quick_query(c, goal_teleporter, NULL, (uint8_t)(200 + (id & 31)), 3, NULL, -1);
        if (!tp) s->doingEvent = -1;
        else path_find(c, tp, THEN_NONE);
      } else {
        step_to_x(c, transport_teleport_x(P(c->inPermanent)), timeMod);
      }
    }
    return;
  }
  if (s->doingEvent == 2) {
    Perm *pub = P(s->target);
    if (pub && pub->cls != CLS_Pub) pub = NULL, s->target = 0;
    if (pub && c->inPermanent == s->target) {
      if (!STC(PubState, pub)->marked) pub_set_marking(pub, true);
      int h = ((int)city.timeSinceStart / 60) % 24;
      if (h >= 5 && h < 10) s->doingEvent = -1;
      else ent_be_entertained(pub, c, timeMod);
      return;
    }
    bool no_pubs = true;
    if (!pub) {
      for (int i = 1; i < MAX_PERMS; i++) {
        Perm *q = &perms[i];
        if (!(q->flags & PF_USED) || q->cls != CLS_Pub) continue;
        if (STC(PubState, q)->marked) {
          s->target = (uint16_t)i;
          pub = q;
          break;
        }
        no_pubs = false;
      }
    }
    if (!pub && !no_pubs) {
      uint16_t np = finder_quick_query(c, goal_unmarked_pub, NULL, (uint8_t)(232 + (id & 15)), 3, NULL, -1);
      if (!np) s->doingEvent = -1;
      else s->target = np, pub = P(np);
    }
    if (pub) path_find(c, s->target, TH_SOCIETY_FAIL);
    return;
  }
  if (c->inPermanent != id) {
    path_find(c, id, THEN_NONE);
    return;
  }
  if (stop) {
    c->currentAction = 2;
    return;
  }
  key_friends_walk_around(p, c, 150);
  int h = ((int)city.timeSinceStart / 60) % 24;
  if (h > 15 && day_number() > s->lastEventDay) {
    s->doingEvent = (int8_t)rnd_between(1, 3);
    if (s->doingEvent == 1 && (worlds[p->world].h == 0 || worlds[p->world].surfaceShard != p->shard)) s->doingEvent = 2;
    s->eventPhase = 0;
    s->lastEventDay = (uint16_t)day_number();
  }
}
static void society_window(Perm *p, WinOut *w) {
  const MissionState *s = STC(MissionState, p);
  if (s->flags & SS_SCENARIO) return;
  mission_lines(w, s, 8, "We will be forever grateful.", "Give us the followers we need:\nMake sure all 8 jobs are filled.",
                society_missions, p);
}
const PermOps ops_SecretSocietyHouse = {.init = society_init, .on_build = society_on_build, .update = society_update,
                                        .work = society_work, .walk_around = key_friends_walk_around, .draw = plain_draw,
                                        .window = society_window};

/* BlossomHippieHQ */
static void hippie_init(Perm *p) { ST(MissionState, p)->doingEvent = -1; }
static bool any_protected_forest(void) {
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && is_a(&perms[i], B_FOREST) && STC(LimitedState, &perms[i])->managementMode == 1) return true;
  return false;
}
static bool garden_at(const Perm *q) { return q && class_is(q->cls, CLS_BotanicalGardens); }
static void hippie_check(Perm *p) {
  MissionState *s = ST(MissionState, p);
  for (;;) {
    bool done = false;
    switch (s->mission) {
    case 0: {
      int n = 0;
      for (int i = 1; i < MAX_PERMS; i++) {
        const Perm *t = &perms[i];
        if (!(t->flags & PF_USED) || t->cls != CLS_Teleporter) continue;
        if (garden_at(P(t->left)) && garden_at(P(t->right)) && garden_at(P(t->bottom)) && garden_at(P(t->top))) n++;
      }
      if (n >= 2) {
        done = true;
        unlocks_unlock("BlossomRestaurant");
      }
      break;
    }
    case 2:
      if (any_protected_forest()) {
        done = true;
        unlocks_unlock("FarmHouse");
      }
      break;
    case 6:
      done = city_upgrade_bought(U_SuperSpaciousLiving);
      if (done) unlocks_unlock("FestivalHQ");
      break;
    }
    if (s->timeToCheck == 0 && s->mission != 0 && s->mission != 2 && s->mission != 6 && s->mission != 8) {
      switch (s->mission) {
      case 1: done = count_class(CLS_BlossomRestaurant) >= 3; break;
      case 3: done = count_class(CLS_FarmHouse) >= 20; break;
      case 4:
        done = count_class(CLS_EcoFarm) >= 30;
        if (done) unlocks_unlock("FarmByProductProcessor");
        break;
      case 5:
        done = count_class(CLS_FarmByProductProcessor) >= 9;
        if (done) unlocks_unlock("SuperSpaciousLiving");
        break;
      case 7: done = count_class(CLS_FestivalHQ) >= 1; break;
      }
      s->timeToCheck = 30;
    }
    if (!done) break;
    s->mission++;
    s->timeToCheck = 0;
  }
}
static void hippie_update(Perm *p, float timeMod) {
  MissionState *s = ST(MissionState, p);
  hippie_check(p);
  check_time(s, timeMod);
  s->extra = tick_down(s->extra, timeMod);   /* canLeaveForBG */
}
static void hippie_walk(Perm *p, Cit *c, float steps) {
  (void)p, (void)steps;
  if (c->relativeX < 7 && rnd_float() < 0.3f) cit_change_floor_and_wait_random(c, 10, 30, THEN_NONE, 0);
  else if (c->relativeY > 5 || rnd_float() > 0.5f) walk_wait(c, 3, 7, 60, 180, false);
  else walk_wait(c, 12, 16, 60, 180, false);
}
static bool goal_tree_garden(const Perm *p, const void *ctx) {
  (void)ctx;
  return garden_at(p) && (!p->bottom || !garden_at(P(p->bottom))) && garden_is_tree(p);
}
static uint8_t TH_HIPPIE_FAIL;
static void th_hippie_fail(Cit *c, uint16_t arg) {
  (void)arg;
  Perm *h = P(c->job);
  if (h && h->cls == CLS_BlossomHippieHQ) ST(MissionState, h)->doingEvent = -1;
}
static void hippie_work(Perm *p, Cit *c, float timeMod, bool stop) {
  MissionState *s = ST(MissionState, p);
  Perm *in = P(c->inPermanent);
  if (garden_at(in) && garden_is_tree(in)) {
    /* the daily climb: up the tree to the top of the stack, then down again */
    if (s->eventPhase != 0) {
      c->relativeY -= 0.4f * timeMod;
      if (c->relativeY <= 0) {
        if (garden_at(P(in->bottom))) {
          c->relativeY += 20;
          c->inPermanent = in->bottom;
          return;
        }
        c->relativeY = 0;
        s->doingEvent = -1;
      } else {
        return;
      }
    } else {
      float spd = c->pathWalkSpeed / 100.0f * timeMod;
      if (fabsf(9 - c->relativeX) >= spd) {
        c->relativeX += (9 > c->relativeX ? 1 : -1) * spd;
        return;
      }
      c->relativeX = 9;
      c->relativeY += 0.4f * timeMod;
      if (c->relativeY >= 10) {
        if (garden_at(P(in->top))) {
          c->relativeY -= 20;
          c->inPermanent = in->top;
        } else {
          s->eventPhase = 1;
        }
      }
      return;
    }
  } else if (s->doingEvent != -1) {
    c->relativeY = 0;
    if (!garden_at(P(s->target))) s->target = finder_query(pid(p), 0, 0, goal_tree_garden, NULL, NULL, -1, NULL, 1e8f);
    if (s->target) {
      if (s->extra == 0) {
        path_find(c, s->target, TH_HIPPIE_FAIL);
        s->extra = 20;
      }
      return;
    }
    s->doingEvent = -1;
  }
  if (stop) {
    c->currentAction = 2;
    return;
  }
  if (c->inPermanent != pid(p)) {
    c->relativeY = 0;
    path_find(c, pid(p), THEN_NONE);
    return;
  }
  hippie_walk(p, c, 150);
  int h = ((int)city.timeSinceStart / 60) % 24;
  if (h > 10 && day_number() > s->lastEventDay) {
    s->doingEvent = 1;
    s->eventPhase = 0;
    s->lastEventDay = (uint16_t)day_number();
  }
}
static void hippie_window(Perm *p, WinOut *w) {
  const MissionState *s = STC(MissionState, p);
  mission_lines(w, s, 8, "Peace out!", "Open the Flower Buds:\nMake sure all 6 jobs are filled.", hippie_missions, p);
  if (s->mission == 2) win_button(w, 0, "... or bribe the Hippies and skip this\ntask for 10,000 food", WA_SPECIAL, true);
}
static void hippie_action(Perm *p, int a) {
  MissionState *s = ST(MissionState, p);
  if (a == WA_SPECIAL && s->mission == 2 && city.materials.m[M_FOOD] >= 10000) {
    s->mission = 3;
    unlocks_unlock("FarmHouse");
    city.materials.m[M_FOOD] -= 10000;
  }
}
const PermOps ops_BlossomHippieHQ = {.init = hippie_init, .update = hippie_update, .work = hippie_work, .walk_around = hippie_walk,
                                     .draw = plain_draw, .window = hippie_window, .window_action = hippie_action};

/* the missions a building gives (getGlobalGoal), for the user interface's list of goals */
bool bld_global_goal(const Perm *p, const char **category, const char **text) {
  const MissionState *s = STC(MissionState, p);
  switch (p->cls) {
  case CLS_HackerHQ:
    if (s->mission == 11) return false;
    *category = "Hackers Mission";
    *text = s->mission >= 11 ? "We'll keep on hacking!" : p->nworkers != bld_jobs(p) ? "Gather the Hackers:\nMake sure all 6 jobs are filled." : hacker_missions[s->mission];
    return true;
  case CLS_SecretSocietyHouse:
    if (s->mission == 8 || (s->flags & SS_SCENARIO)) return false;
    *category = "Secret Society Mission";
    *text = p->nworkers != bld_jobs(p) ? "Give us the followers we need:\nMake sure all 8 jobs are filled." : society_missions[s->mission];
    return true;
  case CLS_BlossomHippieHQ:
    if (s->mission == 8) return false;
    *category = "Hippie Mission";
    *text = p->nworkers != bld_jobs(p) ? "Open the Flower Buds:\nMake sure all 6 jobs are filled." : hippie_missions[s->mission];
    return true;
  default: return false;
  }
}

/* ---------------------------------------------------------------- AlienDecryptor */
float decryptor_time_working(const Perm *p) { return STC(TotalState, p)->total; }
static void decryptor_update(Perm *p, float timeMod) { ST(TotalState, p)->total += timeMod * city.minutesPerTick; }
const PermOps ops_AlienDecryptor = {.update = decryptor_update, .draw = plain_draw};

/* ---------------------------------------------------------------- EscapeRoom */
#define CF_ESCAPE_PLAYER 0x8000
enum { ER_START, ER_FIRST, ER_SECOND, ER_THIRD, ER_LASERS, ER_LAST, ER_DONE };
typedef struct PACKED { uint8_t phase; float progress; } EscapeState;
static int players(const Perm *p, int *index_of, const Cit *who) {
  int n = 0;
  uint16_t id = pid(p);
  for (int i = 1; i < MAX_CITS; i++) {
    const Cit *c = &cits[i];
    if ((c->flags & CF_ALIVE) && (c->flags & CF_ESCAPE_PLAYER) && c->inPermanent == id) {
      if (c == who && index_of) *index_of = n;
      n++;
    }
  }
  return n;
}
static float escape_capacity(const Perm *p) { return p->nworkers * 120.0f; }
static bool escape_open(const Perm *p) {
  return p->nworkers == 1 && first_worker_working(p) && hour_now() < 23.99f && players(p, NULL, NULL) < 10;
}
static bool escape_open_existing(const Perm *p) { return p->nworkers == 1 && first_worker_working(p); }
static bool escape_finish(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  int n = players(p, NULL, NULL);
  if (!(n < 5 || !(c->flags & CF_ESCAPE_PLAYER) || STC(EscapeState, p)->phase == ER_DONE)) return !escape_open_existing(p);
  return true;
}
static void escape_reset(Perm *p) {
  EscapeState *s = ST(EscapeState, p);
  s->phase = ER_START;
  s->progress = 0;
}
static void escape_be_entertained(Perm *p, Cit *c, float timeMod) {
  EscapeState *s = ST(EscapeState, p);
  if (!(c->flags & CF_BUILDING_INITED)) {
    if (!(c->flags & CF_ESCAPE_PLAYER)) {
      if (players(p, NULL, NULL) > 10) {
        c->flags &= (uint16_t)~CF_ENT_INITED;   /* entertainment.stop() */
        return;
      }
      c->flags |= CF_ESCAPE_PLAYER;
    }
    c->flags |= CF_BUILDING_INITED;
  }
  if (s->phase == ER_DONE) {
    c->flags &= (uint16_t)~CF_ENT_INITED;
    return;
  }
  int idx = 0, n = players(p, &idx, c);
  if (n < 5) {
    if (c->relativeY > 5) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, 7, 16, 50, 70, false);
  } else if (s->phase == ER_START) {
    if (c->relativeY > 5) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, 7, 16, 50, 70, false);
    if (n < 8 && s->progress < 1) s->progress += 0.02f * timeMod;
    else s->phase = ER_FIRST, s->progress = 0;
  } else if (s->phase == ER_FIRST) {
    if (c->relativeY < 5 && c->relativeX > 2 && c->relativeX < 7) {
      cit_change_floor(c, THEN_NONE, 0);
    } else {
      walk_wait(c, 3, 6, 30, 60, false);
      if (rnd_float() < 0.2f) {
        s->progress += 0.2f;
        if (s->progress > 1) s->phase = ER_SECOND, s->progress = 0;
      }
    }
  } else if (s->phase == ER_SECOND) {
    if (idx % 2 == 0) {
      if (c->relativeY > 5) {
        cit_change_floor(c, THEN_NONE, 0);
      } else {
        walk_wait(c, 3, 16, 50, 120, false);
        if (rnd_float() < 0.3f) {
          s->progress += 0.34f;
          if (s->progress > 1) s->phase = ER_THIRD, s->progress = 0;
        }
      }
    } else if (c->relativeY < 5 && c->relativeX > 2 && c->relativeX < 7) {
      cit_change_floor(c, THEN_NONE, 0);
    } else {
      walk_wait(c, 3, 6, 30, 60, false);
    }
  } else if (s->phase == ER_THIRD) {
    if (c->relativeY > 5) cit_change_floor(c, THEN_NONE, 0);
    else if (c->relativeX > 4) move_x(c, rnd_between(3, 5));
  } else if (s->phase == ER_LAST) {
    if (c->relativeY < 5 && c->relativeX > 11 && c->relativeX < 16) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, 11, 16, 20, 60, false);
  }
}
static void escape_update(Perm *p, float timeMod) {
  EscapeState *s = ST(EscapeState, p);
  uint16_t id = pid(p);
  if (s->phase == ER_DONE) {
    s->progress += 0.02f * timeMod;
    if (s->progress >= 1) escape_reset(p);
  } else if (s->phase == ER_THIRD || s->phase == ER_LAST) {
    bool all = true;
    for (int i = 1; i < MAX_CITS && all; i++) {
      const Cit *c = &cits[i];
      if (!(c->flags & CF_ALIVE) || !(c->flags & CF_ESCAPE_PLAYER) || c->inPermanent != id) continue;
      if (s->phase == ER_THIRD ? !(c->relativeY < 5 && c->relativeX < 5) : !(c->relativeY > 5 && c->relativeX > 11)) all = false;
    }
    if (all) s->phase = s->phase == ER_THIRD ? ER_LASERS : ER_DONE, s->progress = 0;
  }
  if (s->phase == ER_LASERS) {
    s->progress += 0.01f * timeMod;
    if (s->progress >= 1) s->progress = 0, s->phase = ER_LAST;
  }
}
static void escape_leave(Perm *p, Cit *c, uint16_t to) {
  (void)p, (void)to;
  c->flags &= (uint16_t)~CF_ESCAPE_PLAYER;   /* onCitizenLeave: off the players' list */
}
static void escape_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop) {
    c->currentAction = 2;
    escape_reset(p);
  } else {
    step_to_x(c, 3, timeMod);
  }
}
static void escape_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  plain_draw(b, p, layer, x, y);
  /* the lasers: a red line from x 7 to 16 at y 13 */
  if (layer == LAYER_BG && STC(EscapeState, p)->phase == ER_LASERS)
    for (int z = 0; z < b->zoom; z++) gfx_band_span(b, y + 13 * b->zoom + z, x + 7 * b->zoom, x + 17 * b->zoom, rgb565(0xFF0000), 179);
}
const PermOps ops_EscapeRoom = {.update = escape_update, .work = escape_work, .is_open = escape_open,
                                .is_open_for_existing_visitors = escape_open_existing, .finish_entertainment = escape_finish,
                                .be_entertained = escape_be_entertained, .on_citizen_leave = escape_leave,
                                .entertainment_capacity = escape_capacity, .draw = escape_draw};

/* ---------------------------------------------------------------- Misdirector */
typedef struct PACKED { uint16_t timesUsed, timesUsedTo; float knowledgeGenerated; } MisdirectorState;
static void misdirector_init(Perm *p) { upgrade_first_mode(p); }
uint16_t bld_misdirector(void) {
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == CLS_Misdirector) return (uint16_t)i;
  return NONE;
}
bool bld_is_misdirector(const Perm *p) { return p && p->cls == CLS_Misdirector; }
float bld_misdirect_chance(void) {
  Perm *m = P(bld_misdirector());
  if (!m) return 0;
  switch (m->mode) {
  case MODE_Misdirector_MisdirectorNormal: return 0.025f;
  case MODE_Misdirector_MisdirectorAlways: return 1;
  case MODE_Misdirector_MisdirectorMostlyDisabled: return 0.001f;
  default: return 0;
  }
}
void bld_misdirector_reward(Perm *p) {
  if (!bld_is_misdirector(p)) return;
  float k = p->mode == MODE_Misdirector_MisdirectorAlways ? 0.05f : 0.5f;
  produce(M_KNOWLEDGE, k);
  ST(MisdirectorState, p)->knowledgeGenerated += k;
  particle_add(SPR_TELEPORTER_RAY_MISDIRECT, 4, perm_city_x(p) + 8, perm_city_y(p) + 4);
}
static void misdirector_window(Perm *p, WinOut *w) {
  win_line1(w, "Misdirected [0] times.", STC(MisdirectorState, p)->timesUsedTo);
  win_line1(w, "[0] knowledge generated.", floorf(STC(MisdirectorState, p)->knowledgeGenerated));
}
static void misdirector_on_build(Perm *p) { (void)p; operating_cost_recalculate(); }
const PermOps ops_Misdirector = {.init = misdirector_init, .on_build = misdirector_on_build, .draw = plain_draw,
                                 .window = misdirector_window};

/* ---------------------------------------------------------------- TheMachine */
static const uint8_t machine_mats[9] = {M_WOOD, M_STONE, M_FOOD, M_MACHINEPARTS, M_KNOWLEDGE, M_REFINEDMETAL, M_COMPUTERCHIPS,
                                        M_GRAPHENE, M_ROCKETFUEL};
static struct { float today[9], total[9]; int lastDay; } machine;   /* (one Machine: it is unique) */
static void set9(float *m, float a, float b, float c, float d, float e, float f, float g) {
  memset(m, 0, sizeof(float) * 9);
  m[0] = a, m[1] = b, m[2] = c, m[3] = d, m[4] = e, m[5] = f, m[6] = g;   /* Materials(wood, stone, food, mp, k, rm, cc) */
}
static int ri(int a, int b) { return rnd_between(a, b); }
static void machine_reward(Perm *p) {
  float *m = machine.today;
  switch (rnd_int(10)) {
  case 0: set9(m, ri(150, 301), 0, 0, 0, 0, 0, 0); break;
  case 1: set9(m, 0, ri(100, 501), 0, 0, 0, 0, 0); break;
  case 2: set9(m, 0, 0, ri(200, 2001), 0, 0, 0, 0); break;
  case 3: {
    float a = ri(100, 301), b = ri(50, 301), c = ri(25, 201);
    set9(m, a, b, c, 0, 0, 0, 0);
    break;
  }
  case 4: {
    float a = ri(100, 201), b = ri(150, 251);
    set9(m, a, b, 0, 0, 0, 0, 0);
    break;
  }
  case 5: set9(m, 0, 0, 0, ri(20, 31), 0, 0, 0); break;
  case 6: set9(m, 0, 0, 0, 0, 0, ri(40, 81), 0); break;
  case 7: set9(m, 0, 0, 0, 0, 0, 0, ri(5, 11)); break;
  case 8: {
    float a = ri(15, 31), b = ri(20, 51);
    set9(m, 0, 0, 0, a, 0, b, 0);
    break;
  }
  default:
    switch (rnd_int(5)) {
    case 0: {
      float a = ri(500, 1501), b = ri(1000, 2001);
      set9(m, a, 0, b, 0, 0, 0, 0);
      break;
    }
    case 1: set9(m, 0, 0, 0, 0, ri(500, 1501), 0, 0); break;
    case 2: {
      float a = ri(1000, 2001), b = ri(50, 101);
      set9(m, 0, a, 0, b, 0, 0, 0);
      break;
    }
    case 3: set9(m, 0, 0, 0, 0, 0, ri(200, 301), 0); break;
    default:
      switch (rnd_int(5)) {
      case 0: set9(m, 1000, 0, 1000, 20, 0, 40, 16); break;
      case 1: set9(m, 0, 0, 0, 0, 0, 0, 100); break;
      case 2: set9(m, 0, 0, 0, 0, 0, 1000, 0); break;
      case 3: set9(m, 0, 10000, 0, 0, 0, 0, 0); break;
      default: set9(m, 1000, 1000, 1000, 500, 0, 0, 0); break;
      }
    }
  }
  if (has_upgrade_bit(p, UB_TheMachine_TheMachineUpgrade)) {
    for (int i = 0; i < 9; i++) m[i] *= 1.5f;
    int r = rnd_int(100);
    if (r == 0) m[7] = 50;
    else if (r == 1) m[8] = 500;
    else if (r < 5) m[7] = ri(5, 16);
    else if (r < 10) m[8] = ri(100, 301);
    else if (r < 20) m[7] = ri(1, 4);
    else if (r < 30) m[8] = ri(10, 50);
  }
  /* (ConnectedMachines needs several connected cities: one here) */
  for (int i = 0; i < 9; i++) {
    m[i] = floorf(m[i] * boost() + 0.5f);
    if (m[i]) mat_add(machine_mats[i], m[i]);
    machine.total[i] += m[i];
  }
  machine.lastDay = day_number();
}
static void machine_update(Perm *p, float timeMod) {
  (void)timeMod;
  /* members of the Society and its friends live forever */
  Perm *society = first_class(CLS_SecretSocietyHouse), *friends = first_class(CLS_HouseOfTheKeyFriends);
  Perm *feather = first_class(CLS_FeatherAlliance);
  float extra = has_upgrade_bit(p, UB_TheMachine_TheMachineUpgrade) ? 20 : 0;
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE)) continue;
    if ((society && c->job == pid(society)) || (friends && c->home == pid(friends))) {
      float v = (cit_age(c) - 75 + 3 + extra) * 100;
      c->dieAgeModifier = (int16_t)(v > 32000 ? 32000 : v < -32000 ? -32000 : v);
    }
    if (feather && c->home == pid(feather) && c->dieAgeModifier < 5000) c->dieAgeModifier = 5000;
  }
  if (machine.lastDay != day_number()) machine_reward(p);
}
static void machine_on_build(Perm *p) { machine_reward(p); }
static void machine_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer != LAYER_BG) return;
  int n = frames_of(SPR_SECRETSOCIETYMACHINE_ANIM, 20), len = n * 4 * 2 + 480 + 180;
  float t = fmodf(bld_anim, (float)len);
  int f;
  if (t < 480) f = ((int)t / 4) % 2;
  else if (t - 480 - 180 > n * 4) f = n - 1 - (int)(t - n * 4 - 480 - 180) / 4;
  else if (t - 480 > n * 4) f = n - 1;
  else f = (int)(t - 480) / 4;
  f = f < 0 ? 0 : f > n - 1 ? n - 1 : f;
  dr_frame(b, SPR_SECRETSOCIETYMACHINE_ANIM, 20, f, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
static void machine_window(Perm *p, WinOut *w) {
  (void)p;
  float a[M_COUNT] = {0}, t[M_COUNT] = {0};
  for (int i = 0; i < 9; i++) a[machine_mats[i]] = machine.today[i], t[machine_mats[i]] = machine.total[i];
  win_text(w, "Today's Production:");
  win_cost(w, a);
  win_text(w, "Total Production:");
  win_cost(w, t);
  int h = ((int)city.timeSinceStart / 60) % 24;
  if (h == 23) win_line(w, "Next production in 1 hour.");
  else win_line1(w, "Next production in [0] hours.", 24 - h);
}
const PermOps ops_TheMachine = {.on_build = machine_on_build, .update = machine_update, .draw = machine_draw,
                                .window = machine_window};

/* ---------------------------------------------------------------- Supercomputer, FossilMuseum, Beacon */
static float supercomputer_per_day(void) { return 2.0f * stats.peopleWorkingAtLabs * (1 + upgrades.superComputerUpgrade); }
static void supercomputer_update(Perm *p, float timeMod) {
  knowledge(p, timeMod * (supercomputer_per_day() / (1440 / city.minutesPerTick)) * boost());
}
static void supercomputer_window(Perm *p, WinOut *w) {
  win_line1(w, "[0] knowledge gathered per day.", (int)supercomputer_per_day());
  win_line1(w, "[0] knowledge gathered so far.", (int)STC(TotalState, p)->total);
}
const PermOps ops_Supercomputer = {.update = supercomputer_update, .draw = plain_draw, .window = supercomputer_window};

static uint8_t TH_FOSSIL_MOVE;
static void th_fossil_move(Cit *c, uint16_t arg) {
  (void)arg;
  if (c->relativeY < 5) move_x_slow(c, rnd_between(12, 16));   /* (getFloat() < 1: always) */
  else move_x_slow(c, rnd_between(4, 15));
}
static bool fossil_open(const Perm *p) {
  bool any = false;
  uint16_t id = pid(p);
  for (int i = 1; i < MAX_CITS && !any; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].job == id && cits[i].currentAction == 0) any = true;
  return p->nworkers >= 1 && any && hours_in(hour_now(), 14 - 0.5f, 23);
}
static float zero_capacity(const Perm *p) { (void)p; return 0; }
static void fossil_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (rnd_int(2) == 1) cit_change_floor(c, TH_FOSSIL_MOVE, 0);
  else cit_wait_random(c, 120, 180, TH_FOSSIL_MOVE, 0);
  educate(c, 0.035f, 1.61f);
}
static void fossil_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop) {
    c->currentAction = 2;
    return;
  }
  switch (worker_index(c)) {
  case 0:
    step_to_x(c, 6, timeMod);
    knowledge(p, 0.001f * bonuses.fossilsCollected * timeMod * speed() * edu(c) * boost() * p->nworkers);
    break;
  case 1: walk_wait(c, 12, 16, 120, 180, false); break;
  case 2:
    if (c->relativeY < 5) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, 4, 16, 120, 180, false);
    break;
  }
}
const PermOps ops_FossilMuseum = {.work = fossil_work, .is_open = fossil_open, .entertainment_capacity = zero_capacity,
                                  .be_entertained = fossil_be_entertained, .draw = plain_draw, .window = total_window};

/* the Beacon declares itself entertainment but never sets isEntertainment: never visited, its capacity still counts */
static void beacon_init(Perm *p) { p->flags &= (uint8_t)~PF_IS_ENTERTAINMENT; }
static float beacon_capacity(const Perm *p) { (void)p; return 50; }
static void beacon_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (rnd_float() < 0.5f) walk_wait(c, 4, 8, 60, 90, false);
  else walk_wait(c, 11, 15, 60, 90, false);
}
static void beacon_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer != LAYER_MAIN) return;
  int h = ((int)city.timeSinceStart / 60) % 24;
  float t = hour_now(), a;
  if (h < 6 || h > 20.5f) a = 1;
  else if (h < 6.5f) a = (6.5f - t) * 2;
  else if (h > 20) a = (t - 20) * 2;
  else a = 0;
  if (a > 0) dr_sprite(b, SPR_BEACON_LIGHT, x, y, 8, 1, 0, (uint8_t)(a > 1 ? 255 : a * 255));
}
static void beacon_window(Perm *p, WinOut *w) {
  (void)p;
  win_text(w, "The Beacon has been designed together with Annabelle, who won the first building contest in the Discord!");
}
const PermOps ops_Beacon = {.init = beacon_init, .entertainment_capacity = beacon_capacity, .be_entertained = beacon_be_entertained,
                            .draw = beacon_draw, .window = beacon_window};

/* IntergalacticLibrary and FeatherAlliance: never reachable in this version (a paid building unlocks them) */
static bool galactic_open(const Perm *p) { return first_worker_working(p) && hours_in(hour_now(), 13 - 0.5f, 23); }
static void galactic_update(Perm *p, float timeMod) {
  (void)p;
  produce(M_KNOWLEDGE, timeMod * (0.05f * p->nworkers / 3.0f * ncits) / (1440 / city.minutesPerTick) * boost());
}
static void galactic_be_entertained(Perm *p, Cit *c, float timeMod) {
  fossil_be_entertained(p, c, timeMod);
  c->flags &= (uint16_t)~CF_BUILDING_INITED;
  educate(c, 0.033f, 1.81f);
}
const PermOps ops_IntergalacticLibrary = {.update = galactic_update, .is_open = galactic_open, .entertainment_capacity = zero_capacity,
                                          .be_entertained = galactic_be_entertained, .draw = plain_draw};
const PermOps ops_FeatherAlliance = {.walk_around = key_friends_walk_around, .draw = plain_draw};

/* ---------------------------------------------------------------- FestivalHQ and the music festival */
#define CF_FEST_WORKER 0x2000
#define MAX_VENUES 64
static struct {
  bool planned, started;
  double timeStart, timeEnd;
  uint16_t center;
  float coolDown;              /* festivalManager.festivalCoolDown */
  uint8_t nvenues;
  uint16_t venues[MAX_VENUES];
  uint8_t temp[MAX_VENUES];    /* temporary workers at each venue */
} fest;
static bool involved(const Cit *c) { return cit_age(c) > 18 && (!c->job || !class_is(P(c->job)->cls, CLS_School)); }
static int involved_count(void) {
  int n = 0;
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && involved(&cits[i])) n++;
  return n;
}
static bool venue_kind(const Perm *p) {
  if (!is_building(p) || !(p->flags & PF_IS_ENTERTAINMENT) || p->cls == CLS_EcoFarm) return false;
  int t = ent_type(p);
  if (t != 3 && t != 0 && t != 1) return false;
  /* couldStandHere: not an all-direction merging building on one of its own kind */
  Drawer d;
  bld_drawer(p, &d);
  return !(d.kind == DRW_ALLDIR && p->bottom && P(p->bottom)->cls == p->cls);
}
static uint16_t *relevant_out;
static int relevant_n, relevant_max;
static bool record_venue(const Perm *p, const void *ctx) {
  (void)ctx;
  if (venue_kind(p)) {
    if (relevant_n < relevant_max) relevant_out[relevant_n] = pid(p);
    relevant_n++;
  }
  return false;
}
/* MusicFestival.relevantBuildings: in the order the finder reaches them from the HQ */
static int relevant_venues(uint16_t center, uint16_t *out, int max) {
  relevant_out = out, relevant_n = 0, relevant_max = max;
  finder_query(center, 0, 0, record_venue, NULL, NULL, -1, NULL, 1e8f);
  return relevant_n;
}
static bool can_do(uint16_t center) {
  int inv = involved_count();
  return inv > 100 && relevant_venues(center, NULL, 0) > inv / 100;
}
bool festival_running(void) { return fest.planned && city.timeSinceStart >= fest.timeStart; }
static bool is_venue(uint16_t id) {
  for (int i = 0; i < fest.nvenues; i++)
    if (fest.venues[i] == id) return true;
  return false;
}
static int venue_index(uint16_t id) {
  for (int i = 0; i < fest.nvenues; i++)
    if (fest.venues[i] == id) return i;
  return -1;
}
/* doStartRepeatables: the venues (the first involved / 20, and their neighbours that were left out) */
static void festival_venues(void) {
  uint16_t all[MAX_VENUES * 2];
  int n = relevant_venues(fest.center, all, MAX_VENUES * 2);
  if (n > MAX_VENUES * 2) n = MAX_VENUES * 2;
  int want = involved_count() / 20;
  int keep = n < want ? n : want;
  if (keep > MAX_VENUES) keep = MAX_VENUES;
  fest.nvenues = (uint8_t)keep;
  memcpy(fest.venues, all, sizeof all[0] * (size_t)keep);
  bool left_out[MAX_VENUES * 2];
  for (int i = 0; i < n; i++) left_out[i] = i >= keep;
  for (int i = 0; i < keep; i++) {
    for (int dir = 0; dir < 2; dir++) {
      const Perm *b = P(fest.venues[i]);
      for (;;) {
        uint16_t nb = dir ? b->right : b->left;
        int k = -1;
        for (int j = keep; j < n; j++)
          if (left_out[j] && all[j] == nb) k = j;
        if (!nb || k < 0 || fest.nvenues >= MAX_VENUES) break;
        left_out[k] = false;
        fest.venues[fest.nvenues++] = nb;
        b = P(nb);
      }
    }
  }
  memset(fest.temp, 0, sizeof fest.temp);
}
static void festival_end(void) {
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE) || !(c->flags & CF_FEST_WORKER)) continue;
    c->flags &= (uint16_t)~CF_FEST_WORKER;
    Perm *in = P(c->inPermanent);
    if (!is_building(in) || !is_venue(c->inPermanent)) continue;
    if (park_like(in)) c->relativeY = 0;
    else {
      uint8_t ji = c->jobIndex;
      c->jobIndex = 255;
      bld_work(in, c, 0, true);
      c->jobIndex = ji;
    }
  }
  for (int i = 0; i < fest.nvenues; i++)
    if (P(fest.venues[i]) && park_like(P(fest.venues[i]))) park_festival_stop(P(fest.venues[i]));
  fest.planned = fest.started = false;
  fest.nvenues = 0;
}
void festival_update(float timeMod) {
  if (fest.planned && city.timeSinceStart >= fest.timeStart) {
    if (city.timeSinceStart > fest.timeEnd) {
      festival_end();
    } else if (!fest.started) {
      festival_venues();
      happiness_add_boost(city.timeSinceStart + 10080, 25, true, HB_MUSIC_FESTIVAL);
      fest.started = true;
    }
  }
  if (fest.coolDown > -1) fest.coolDown -= timeMod * city.minutesPerTick;
  if (festival_running()) {
    /* the temporary workers of each venue, counted again (tempWorkers, without the dead and the uninvolved) */
    memset(fest.temp, 0, sizeof fest.temp);
    for (int i = 1; i < MAX_CITS; i++) {
      Cit *c = &cits[i];
      if (!(c->flags & CF_ALIVE) || !(c->flags & CF_FEST_WORKER)) continue;
      int v = venue_index(c->inPermanent);
      if (v < 0 || !involved(c)) c->flags &= (uint16_t)~CF_FEST_WORKER;
      else if (fest.temp[v] < 255) fest.temp[v]++;
    }
  }
}
/* the entertainment's times, as citizen.c keeps them (city times in 2-hour and 4-minute units that wrap) */
static uint8_t at_units(double t, double unit) {
  double now = city.timeSinceStart / unit, d = (t - city.timeSinceStart) / unit;
  if (d > 120) d = 120;
  if (d < -120) d = -120;
  return (uint8_t)((int64_t)now + (int)floor(d + (t >= city.timeSinceStart ? 0.999 : 0)));
}
static bool past_units(uint8_t v, double unit) { return (int8_t)((uint8_t)(int64_t)(city.timeSinceStart / unit) - v) >= 0; }
/* MusicFestival.citizenFestivalUpdate */
static void festival_citizen_update(Cit *c, float timeMod) {
  if (c->currentAction == 0) bld_work(P(c->job), c, timeMod, true);   /* tryFinishWork */
  if (c->currentAction == 0) return;
  bool busy = false;
  Perm *in = P(c->inPermanent);
  int v = is_building(in) ? venue_index(c->inPermanent) : -1;
  if (v >= 0) {
    bool haver = park_like(in), work = is_a(in, B_WORK);
    bool mine = c->flags & CF_FEST_WORKER;
    int id = 0;
    if (mine) {
      for (int i = 1; i < cid(c); i++)
        if ((cits[i].flags & CF_ALIVE) && (cits[i].flags & CF_FEST_WORKER) && cits[i].inPermanent == c->inPermanent) id++;
    }
    if (!mine && ((!haver && work && fest.temp[v] < bld_jobs(in)) || (haver && fest.temp[v] < 2))) {
      mine = true;
      c->flags |= CF_FEST_WORKER;
      id = fest.temp[v]++;
    }
    if (mine) {
      busy = true;
      if (haver) {
        park_festival_work(in, c, timeMod, id);
      } else {
        uint8_t ji = c->jobIndex;
        c->jobIndex = 255;   /* (not in its workers' list) */
        bld_work(in, c, timeMod, false);
        c->jobIndex = ji;
      }
    } else {
      busy = true;
      if (!(c->flags & CF_ENT_INITED)) {
        float desirability = !c->job && !c->school ? 0.75f : 1.0f;
        c->entFinished = at_units(city.timeSinceStart + 60.0 * rnd_range(ent_min_time(in), ent_max_time(in)), 4);
        c->entRecent[ent_type(in) % 6] =
            at_units(city.timeSinceStart + 1440.0 * rnd_range(ent_min_satisfy(in), ent_max_satisfy(in)) * desirability, 120);
        c->flags |= CF_ENT_INITED;
      }
      if (past_units(c->entFinished, 4) && ent_finish(in, c, timeMod)) {
        busy = false;
        c->flags &= (uint16_t)~CF_ENT_INITED;
      } else if (haver) {
        park_festival_entertained(in, c);
      } else {
        ent_be_entertained(in, c, timeMod);
      }
    }
    c->flags |= CF_CAN_VIEW_SELF;
  }
  if (!busy && fest.nvenues) {
    uint16_t to = fest.venues[rnd_int(fest.nvenues)];
    if (P(to) && (P(to)->flags & PF_USED)) path_find(c, to, THEN_NONE);
  }
}
/* Festival.updateFestivalCitizen: one citizen's step while a festival is on (sim.c calls it instead of the usual) */
void festival_citizen(Cit *c, float timeMod) {
  if (c->flags & CF_DELAY_CAN_VIEW_SELF) c->flags &= (uint16_t)~CF_DELAY_CAN_VIEW_SELF;
  cit_update_path(c, timeMod);
  if (!(c->flags & CF_ALIVE) || (c->flags & (CF_HAS_PATH | CF_REQUESTING_PATH))) return;
  if (involved(c)) festival_citizen_update(c, timeMod);
  else cit_update_daily_life(c, timeMod);
}
static void festival_hq_update(Perm *p, float timeMod) {
  TotalState *cd = ST(TotalState, p);   /* festivalCoolDown */
  int jobs = bld_jobs(p);
  if (cd->total > -1) cd->total -= timeMod * (jobs ? (float)p->nworkers / jobs : 0) * city.minutesPerTick;
}
static void festival_hq_destroy(Perm *p) {
  (void)p;
  if (fest.planned) {
    if (fest.started) festival_end();
    fest.planned = false;
  }
}
static void festival_hq_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p, (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  if (c->relativeY > 5) {
    if (rnd_range(0, 1) < 0.5f) cit_change_floor(c, TH_CHANGE_FLOOR_WAIT_50_100, 0);
    else {
      static const int8_t up[8] = {3, 4, 5, 6, 12, 13, 14, 15};
      int x = up[rnd_int(8)];
      cit_move_and_wait(c, x, rnd_between(50, 100), THEN_NONE, 0, false, false);
    }
  } else if (rnd_range(0, 1) < 0.3f) {
    cit_change_floor(c, TH_CHANGE_FLOOR_WAIT_50_100, 0);
  } else {
    static const int8_t down[4] = {3, 8, 13, 14};
    int x = down[rnd_int(4)];
    cit_move_and_wait(c, x, rnd_between(50, 100), THEN_NONE, 0, false, false);
  }
}
static void festival_price(float m[M_COUNT]) {
  memset(m, 0, sizeof(float) * M_COUNT);
  m[M_WOOD] = (float)ncits, m[M_FOOD] = 3.0f * ncits;
}
static void basic_time(char *b, int size, float minutes) {
  if (minutes > 2880) lo1(b, size, "[0] days", floorf(minutes / 1440));
  else if (minutes > 1440) lo1(b, size, "one day", 0);
  else if (minutes > 60) lo1(b, size, "[0] hours", floorf(minutes / 60));
  else lo1(b, size, "one hour", 0);
  if (b[0] >= 'a' && b[0] <= 'z') b[0] = (char)(b[0] - 'a' + 'A');
}
/* the festival part: Organize a Festival (its Music Festival button, always pressable: it says so when it can't be
 * done), or the festival's state in Arial15 and its text */
static void festival_hq_window(Perm *p, WinOut *w) {
  float cd = STC(TotalState, p)->total;
  if (!fest.planned && cd < 0 && fest.coolDown < 0) {
    float m[M_COUNT];
    festival_price(m);
    win_heading(w, L_organize_a_festival, -1, -1, false, false, NULL, NULL);
    win_button(w, 0, L_music_festival, WA_SPECIAL, true);
    win_text(w, L_music_festival_help);
    win_cost(w, m);
    win_space(w, 3);
    return;
  }
  win_space(w, 3);
  win_para(w, FONT_15, festival_running() ? L_current_festival : fest.planned ? L_planned_festival : L_preparing, 250, 0);
  if (festival_running()) win_text(w, L_music_festival_doing);
  else if (fest.planned) win_text(w, L_music_festival_planned);
  else if (p->nworkers == 0) win_text(w, L_assign_workers_to_prepare);
  else {
    char t[64], b[200];
    basic_time(t, sizeof t, fmaxf(cd, fest.coolDown) * ((float)bld_jobs(p) / p->nworkers));
    snprintf(b, sizeof b, "%s of preparation left.%s%s", t, p->nworkers < bld_jobs(p) ? " " : "",
             p->nworkers < bld_jobs(p) ? L_assign_more_to_speed_up : "");
    win_text(w, b);
  }
}
static void festival_hq_action(Perm *p, int a) {
  if (a != WA_SPECIAL) return;
  float m[M_COUNT];
  festival_price(m);
  if (mat_can_afford(m) && !fest.planned && can_do(pid(p))) {
    mat_pay(m);
    fest.planned = true;
    fest.started = false;
    fest.center = pid(p);
    fest.timeStart = city.timeSinceStart;
    fest.timeEnd = fest.timeStart + 1440;
    TotalState *cd = ST(TotalState, p);
    cd->total += 8783.9999999999982f;
    fest.coolDown = cd->total;
  } else {
    ui_show_simple(L_cant_organize);
  }
}
const PermOps ops_FestivalHQ = {.destroy = festival_hq_destroy, .update = festival_hq_update, .work = festival_hq_work,
                                .draw = plain_draw, .window = festival_hq_window, .window_action = festival_hq_action};

/* ---------------------------------------------------------------- start, save, load */
uint8_t TH_CHANGE_FLOOR_WAIT_50_100;
static void th_change_floor_wait_50_100(Cit *c, uint16_t arg) { (void)arg; wait_ticks(c, 50, 100); }
void unique_new_city(void) {
  modern_artworks_unlocked = 4;
  memset(&machine, 0, sizeof machine);
  machine.lastDay = -1;
  memset(&fest, 0, sizeof fest);
  memset(bot_cache, 0, sizeof bot_cache);
}
void unique_save(SaveIO *io) {
  save_u8(io, modern_artworks_unlocked);
  save_bytes(io, &machine, sizeof machine);
  save_bytes(io, &fest, sizeof fest);
}
bool unique_load(SaveIO *io) {
  modern_artworks_unlocked = load_u8(io);
  load_bytes(io, &machine, sizeof machine);
  load_bytes(io, &fest, sizeof fest);
  return fest.nvenues <= MAX_VENUES;
}
void bld_unique_init(void) {
  TH_ART_MOVE = then_register(th_art_move);
  TH_SOCIETY_FAIL = then_register(th_society_fail);
  TH_HIPPIE_FAIL = then_register(th_hippie_fail);
  TH_FOSSIL_MOVE = then_register(th_fossil_move);
  TH_CHANGE_FLOOR_WAIT_50_100 = then_register(th_change_floor_wait_50_100);
}
