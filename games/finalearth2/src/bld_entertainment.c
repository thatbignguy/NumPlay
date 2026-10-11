/* The Final Earth 2: entertainment (buildings.md 9): bars and restaurants,
 * parks and gardens (ParkWalk), redwood trees, night clubs and their light
 * shows, the art museum, the arcade and the holo game hall. */
#include "bldi.h"

static void walk_wait(Cit *c, int xmin, int xmax, int tmin, int tmax, bool slow) {
  int x = rnd_between(xmin, xmax);
  cit_move_and_wait(c, x, rnd_between(tmin, tmax), THEN_NONE, 0, false, slow);
}
static void night_visit(Cit *c) {
  /* bars and clubs: the night's entertainment satisfied for a few days (first call per visit) */
  if (!(c->flags & CF_BUILDING_INITED)) {
    c->wantsNightEntertainmentIn = (uint8_t)rnd_between(1, 5);
    c->flags |= CF_BUILDING_INITED;
  }
}
static bool open_between(const Perm *p, float start, float close) {
  return first_worker_working(p) && hours_in(hour_now(), start, close);
}
static float workers_times(const Perm *p, float per) { return p->nworkers * per; }
static void plain_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  draw_upgrades(b, p, layer, x, y, true);
}

/* ---------------------------------------------------------------- Pub and CarrotJuiceBar */
static uint8_t TH_PUB_MOVE, TH_FACTORYPUB_MOVE, TH_TABLE, TH_LUX_TABLE, TH_ARCADE_MOVE, TH_MUSEUM_MOVE;
static void th_pub_move(Cit *c, uint16_t arg) { (void)arg; move_x(c, rnd_between(7, 16)); }
static void pub_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (c->relativeY < 5) cit_change_floor(c, TH_PUB_MOVE, 0);
  else cit_wait_random(c, 120, 180, TH_PUB_MOVE, 0);
  night_visit(c);
}
static float pub_capacity(const Perm *p) { return workers_times(p, 50); }
static bool pub_open(const Perm *p) {
  return p->nworkers == 1 && open_between(p, 20 - 0.5f, 4.5f) && !STC(PubState, p)->marked;
}
static void pub_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p;
  if (stop && hour_now() > 4.5f) c->currentAction = 2;
  else if (c->relativeY < 5) cit_change_floor(c, THEN_NONE, 0);
  else step_to_x(c, 3, timeMod);
}
void pub_set_marking(Perm *p, bool on) {
  if (p && p->cls == CLS_Pub) ST(PubState, p)->marked = on;
}
static void pub_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  plain_draw(b, p, layer, x, y);
  if (layer == LAYER_MAIN && STC(PubState, p)->marked) dr_sprite(b, SPR_PUB_MARKING_KEY, x, y, 2, 10, 0, 255);
}
const PermOps ops_Pub = {.work = pub_work, .is_open = pub_open, .entertainment_capacity = pub_capacity,
                         .be_entertained = pub_be_entertained, .draw = pub_draw};

/* (as written: with two or three workers it is never open) */
static bool juice_open(const Perm *p) { return p->nworkers == 1 && open_between(p, 20 - 0.5f, 4.5f); }
static void juice_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p;
  if (stop && hour_now() > 4.5f) {
    c->currentAction = 2;
    return;
  }
  switch (worker_index(c)) {
  case 0:
    if (c->relativeY < 5) cit_change_floor(c, THEN_NONE, 0);
    else step_to_x(c, 3, timeMod);
    break;
  case 1:
    if (c->relativeY > 5) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, 12, 16, 30, 60, false);
    break;
  case 2:
    if (c->relativeY < 5) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, 7, 16, 90, 120, false);
    break;
  }
}
const PermOps ops_CarrotJuiceBar = {.work = juice_work, .is_open = juice_open, .entertainment_capacity = pub_capacity,
                                    .be_entertained = pub_be_entertained, .draw = plain_draw};

/* ---------------------------------------------------------------- restaurants */
static bool is_restaurant(const Perm *q) { return q && class_is(q->cls, CLS_Restaurant); }
static bool restaurant_open(const Perm *p) { return p->nworkers >= 1 && open_between(p, 16 - 0.5f, 0.5f); }
static float restaurant_capacity(const Perm *p) { return (float)(int)(p->nworkers * (45 + adj_effect_lr(p, EF_RESTAURANT_CAP))); }
/* getTopTable: arg bit 0 says a restaurant is on the right */
static void th_table(Cit *c, uint16_t arg) {
  static const int8_t x[4] = {4, 7, 14, 17};
  move_x(c, x[rnd_int(3 + (arg & 1))]);
}
static void restaurant_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  if (c->flags & CF_BUILDING_INITED) {
    cit_wait_random(c, 30, 60, THEN_NONE, 0);
    return;
  }
  bool left = is_restaurant(P(p->left)), right = is_restaurant(P(p->right));
  c->wantsNightEntertainmentIn = (uint8_t)rnd_between(1, 5);
  c->flags |= CF_BUILDING_INITED;
  if (c->relativeY <= 5) {
    if (rnd_int(3) <= 1) cit_change_floor(c, TH_TABLE, right);
    else if (left) move_x(c, rnd_int(2) == 0 ? 1 : 6);
    else move_x(c, rnd_int(2) == 0 ? 3 : 6);
  } else {
    th_table(c, right);
  }
}
static void restaurant_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop && hour_now() > 0.5f) {
    c->currentAction = 2;
    return;
  }
  switch (worker_index(c)) {
  case 0:
    if (c->relativeY > 5) cit_change_floor(c, THEN_NONE, 0);
    else step_to_x(c, 12, timeMod);
    break;
  case 1:
    if (c->relativeY < 5) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, is_restaurant(P(p->left)) ? 0 : 3, is_restaurant(P(p->right)) ? 19 : 16, 60, 90, false);
    break;
  }
}
const PermOps ops_Restaurant = {.work = restaurant_work, .is_open = restaurant_open, .entertainment_capacity = restaurant_capacity,
                                .be_entertained = restaurant_be_entertained, .draw = draw_merging};

static float lux_capacity(const Perm *p) { return (float)(int)(p->nworkers * (25 + adj_effect_lr(p, EF_RESTAURANT_CAP))); }
static void th_lux_table(Cit *c, uint16_t arg) { (void)arg; move_x(c, rnd_int(2) == 0 ? 4 : 14); }
static void lux_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (c->flags & CF_BUILDING_INITED) {
    cit_wait_random(c, 30, 60, THEN_NONE, 0);
    return;
  }
  c->wantsNightEntertainmentIn = (uint8_t)rnd_between(1, 5);
  c->flags |= CF_BUILDING_INITED;
  if (c->relativeY <= 5) {
    if (rnd_int(3) == 0) move_x(c, rnd_between(4, 6));
    else cit_change_floor(c, TH_LUX_TABLE, 0);
  } else {
    th_lux_table(c, 0);
  }
}
static void lux_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p;
  if (stop && hour_now() > 0.5f) {
    c->currentAction = 2;
    return;
  }
  switch (worker_index(c)) {
  case 0:
    if (c->relativeY > 5) cit_change_floor(c, THEN_NONE, 0);
    else step_to_x(c, 12, timeMod);
    break;
  case 1:
    if (c->relativeY < 5) cit_change_floor(c, THEN_NONE, 0);
    else walk_wait(c, 3, 16, 60, 90, false);
    break;
  }
}
const PermOps ops_LuxuryRestaurant = {.work = lux_work, .is_open = restaurant_open, .entertainment_capacity = lux_capacity,
                                      .be_entertained = lux_be_entertained, .draw = plain_draw};

/* FactoryPub: a refined metals factory (bld_production.c) that is also a pub */
static bool factory_pub_open(const Perm *p) { return p->nworkers >= 1 && open_between(p, 16.5f - 0.5f, 4.5f); }
static float factory_pub_capacity(const Perm *p) { return workers_times(p, 25); }
static void th_factory_pub_move(Cit *c, uint16_t arg) { (void)arg; move_x(c, rnd_between(8, 16)); }
static void factory_pub_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  if (c->relativeY < 5) {
    cit_change_floor(c, TH_FACTORYPUB_MOVE, 0);
  } else {
    int x = rnd_between(8, P(p->right) && P(p->right)->cls == CLS_FactoryPub ? 19 : 16);
    int16_t s[] = {4, (int16_t)x, 8, (int16_t)rnd_between(60, 120)};
    cit_set_path(c, s, 2);
  }
  night_visit(c);
}
void factory_work(Perm *p, Cit *c, float timeMod, bool stop);
static void factory_pub_init(Perm *p) { upgrade_first_mode(p); }
const PermOps ops_FactoryPub = {.init = factory_pub_init, .work = factory_work, .update = factory_update_any,
                                .is_open = factory_pub_open, .entertainment_capacity = factory_pub_capacity,
                                .be_entertained = factory_pub_be_entertained, .draw = factory_draw_any,
                                .window = factory_window_lines};

/* BlossomRestaurant */
static float blossom_capacity(const Perm *p) { return (float)(int)(p->nworkers * (50 + adj_effect_lr(p, EF_RESTAURANT_CAP))); }
static void blossom_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  if (c->flags & CF_BUILDING_INITED) {
    cit_wait_random(c, 30, 60, THEN_NONE, 0);
    return;
  }
  static const int8_t low[6] = {1, 2, 5, 13, 14, 17}, up[4] = {1, 4, 12, 17};
  const Perm *bo = P(p->bottom);
  if (!bo || bo->cls != CLS_BlossomRestaurant) move_x(c, low[rnd_int(6)]);
  else move_x(c, up[rnd_int(4)]);
  c->flags |= CF_BUILDING_INITED;
}
static void blossom_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p, (void)timeMod;
  if (stop && hour_now() > 0.5f) c->currentAction = 2;
  else walk_wait(c, 3, 18, 60, 120, true);
}
const PermOps ops_BlossomRestaurant = {.work = blossom_work, .is_open = restaurant_open, .entertainment_capacity = blossom_capacity,
                                       .be_entertained = blossom_be_entertained, .draw = draw_merging};

/* ---------------------------------------------------------------- ParkWalk */
bool park_open(void) {
  float h = hour_now();
  return h >= 6.0f && h < 22;
}
static bool is_park(const Perm *q) { return q && class_is(q->cls, CLS_Park); }
static bool is_garden(const Perm *q) { return q && class_is(q->cls, CLS_BotanicalGardens); }
bool park_like(const Perm *p) { return is_park(p) || is_garden(p); }
/* beEntertainedPark (as written: through a park pod, the path's last wait is cut off) */
void park_walk(const Perm *left, const Perm *right, Cit *c) {
  bool lskip = false, rskip = false;
  if (left && left->cls == CLS_ParkPod) left = P(left->left), lskip = true;
  if (right && right->cls == CLS_ParkPod) right = P(right->right), rskip = true;
  bool lg = is_garden(left), rg = is_garden(right);
  bool lwalk = lg && (!left->bottom || !is_garden(P(left->bottom)));
  bool rwalk = rg && (!right->bottom || !is_garden(P(right->bottom)));
  if (!lg && is_park(left)) lg = lwalk = true;
  if (!rg && is_park(right)) rg = rwalk = true;
  bool doing = false;
  int go = rnd_int(1 + lg + rg);
  if (lg && go == 0) {
    if (lskip) {
      if (lwalk) {
        int16_t s[] = {12, 50, 2, 0, 2, 0, 4, (int16_t)rnd_between(0, 18), 8, (int16_t)rnd_between(100, 180)};
        cit_set_path(c, s, 4);
        doing = true;
      }
    } else if (lwalk) {
      int16_t s[] = {12, 50, 2, 0, 4, (int16_t)rnd_between(0, 18), 8, (int16_t)rnd_between(100, 180)};
      cit_set_path(c, s, 4);
      doing = true;
    } else if (rnd_int(2) == 0) {
      walk_wait(c, -3, -3, 70, 90, true);
      doing = true;
    }
  } else if (rg && go == 1) {
    if (rskip) {
      if (rwalk) {
        int16_t s[] = {12, 50, 3, 0, 3, 0, 4, (int16_t)rnd_between(0, 18), 8, (int16_t)rnd_between(100, 180)};
        cit_set_path(c, s, 4);
        doing = true;
      }
    } else if (rwalk) {
      int16_t s[] = {12, 50, 3, 0, 4, (int16_t)rnd_between(0, 18), 8, (int16_t)rnd_between(100, 180)};
      cit_set_path(c, s, 4);
      doing = true;
    } else if (rnd_int(2) == 0) {
      walk_wait(c, 21, 21, 70, 90, true);
      doing = true;
    }
  }
  if (!doing) walk_wait(c, lg ? 0 : 3, rg ? 18 : 15, 100, 180, true);
}

/* ---------------------------------------------------------------- Park and its kinds (statues) */
static uint16_t park_plants(const Perm *p) {
  switch (p->cls) {
  case CLS_StatueGarden: return SPR_STATUEGARDEN_STATUES;
  case CLS_StatueOfHappiness: return SPR_STATUEOFHAPPINESS;
  case CLS_StatueOfTheKey: return SPR_STATUEOFTHEKEY;
  default: return SPR_PARK_PLANTS;
  }
}
static int park_choices(const Perm *p) {
  int n = frames_of(park_plants(p), 20);
  return p->cls == CLS_Park && !bld_secret_orchid ? n - 3 : n;   /* numberOfLockedParkTextures */
}
static void park_init(Perm *p) {
  ParkState *s = ST(ParkState, p);
  s->texture = (uint8_t)rnd_int(park_choices(p));
  if (p->cls == CLS_StatueOfHappiness || p->cls == CLS_StatueOfTheKey) s->texture = 0;
}
static float park_capacity(const Perm *p) {
  return p->cls == CLS_StatueGarden ? 40 : p->cls == CLS_StatueOfHappiness || p->cls == CLS_StatueOfTheKey ? 80 : 30;
}
static int park_type(const Perm *p) { return p->cls == CLS_Park ? 3 : 2; }
static bool park_is_open(const Perm *p) { (void)p; return park_open(); }
static void park_be_entertained(Perm *p, Cit *c, float timeMod) { (void)timeMod; park_walk(P(p->left), P(p->right), c); }
static void park_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  const ParkState *s = STC(ParkState, p);
  draw_merging(b, p, layer, x, y);
  if (layer == LAYER_BG) {
    uint16_t spr = park_plants(p);
    dr_frame(b, spr, 20, s->texture % frames_of(spr, 20), x, y, 0, 0, s->mirrored ? GFX_FLIPX : 0, 255, GFX_NO_TINT);
    if (s->podium) dr_sprite(b, SPR_FESTIVAL_PODIUM_SMALL, x, y, s->podium == 2 ? -3 : 3, 20 - spr_h(SPR_FESTIVAL_PODIUM_SMALL) - 1, 0, 255);
  }
}
static void park_window(Perm *p, WinOut *w) {
  if (p->cls == CLS_StatueGarden) {
    win_button(w, 0, "Mirror", WA_MIRROR, true);
    /* (the developer's message beside the elephant sculptures) */
    if (STC(ParkState, p)->texture == 0) {
      win_text(w, "A Message from the Developer");
      win_text(w, "Let's ensure elephants are not just pixely statues on tiny worlds in 2142. The climate crisis is the most "
                  "important threat facing wildlife - and humans.");
    }
  }
  win_button(w, 0, "Change Building Color", WA_CYCLE_0, true);
  const char *t = p->cls == CLS_Park ? "Change Plants" : p->cls == CLS_StatueGarden ? "Change Sculptures" : "Change Statue";
  win_button(w, 0, t, WA_CYCLE_1, true);
}
static void park_action(Perm *p, int a) {
  ParkState *s = ST(ParkState, p);
  if (a == WA_MIRROR) s->mirrored ^= 1;
  else if (a == WA_CYCLE_0) s->mainTexture = (uint8_t)((s->mainTexture + 1) % 3);
  else if (a == WA_CYCLE_1) s->texture = (uint8_t)((s->texture + 1) % park_choices(p));
}
const PermOps ops_Park = {.init = park_init, .is_open = park_is_open, .entertainment_capacity = park_capacity,
                          .entertainment_type = park_type, .be_entertained = park_be_entertained, .draw = park_draw,
                          .window = park_window, .window_action = park_action};

/* ParkFestivalEntertainment: festival staff on a podium, visitors in front of it */
static uint8_t *podium_of(Perm *p) { return is_park(p) ? &ST(ParkState, p)->podium : &ST(GardenState, p)->podium; }
void park_festival_work(Perm *p, Cit *c, float timeMod, int id) {
  bool show = true;
  int px = 3;
  const Perm *l = P(p->left);
  int main_texture = is_park(p) ? STC(ParkState, p)->mainTexture : 0;
  if (l) {
    if (show && is_park(l)) show = false;
    if (show && is_garden(l)) {
      show = false;
      if (l->bottom && is_garden(P(l->bottom))) {
        if (main_texture == 0) px = -3;
        show = true;
      }
    }
  }
  uint8_t *podium = podium_of(p);
  if (show) {
    *podium = px == 3 ? 1 : 2;
    int my = id == 0 ? px + 3 : px;
    step_to_x(c, (float)my, timeMod);
    if (c->relativeX >= my && c->relativeX <= my + 1) set_rel(c, (float)my, 4);
  } else {
    *podium = 0;
    walk_wait(c, 0, 17, 60, 90, false);
  }
}
void park_festival_entertained(Perm *p, Cit *c) {
  uint8_t podium = *podium_of(p);
  if (!podium) {
    park_walk(P(p->left), P(p->right), c);
    return;
  }
  int xmin = (podium == 2 ? -3 : 3) + spr_w(SPR_FESTIVAL_PODIUM_SMALL);
  const Perm *r = P(p->right);
  walk_wait(c, xmin, r && park_like(r) ? 19 : 16, 100, 180, true);
}
void park_festival_stop(Perm *p) { *podium_of(p) = 0; }

/* ---------------------------------------------------------------- BotanicalGardens and the dome */
static const uint8_t garden_sets[6] = {2, 3, 2, 4, 2, 2};
static int garden_group(int texture) {
  int g = -1, i = 0;
  while (i <= texture && g < 5) i += garden_sets[++g];
  return g;
}
static int garden_first(int group) {
  int t = 0;
  for (int i = 0; i < group; i++) t += garden_sets[i];
  return t;
}
static void garden_set_group(Perm *p, int group) {
  GardenState *s = ST(GardenState, p);
  s->texture = (uint8_t)(garden_first(group) + rnd_int(garden_sets[group]));
}
bool garden_is_tree(const Perm *p) { return STC(GardenState, p)->texture >= 7 && STC(GardenState, p)->texture <= 10; }
static void garden_on_build(Perm *p) { ST(GardenState, p)->justBuilt = 1; }
/* postCreate: a new garden takes the plants' group of the garden below (or above), else any plants */
void garden_post_create(Perm *p) {
  GardenState *s = ST(GardenState, p);
  if (!s->justBuilt) return;
  const Perm *from = NULL;
  if (is_garden(P(p->bottom))) from = P(p->bottom);
  if (is_garden(P(p->top))) from = P(p->top);
  if (from) garden_set_group(p, garden_group(STC(GardenState, from)->texture));
  else s->texture = (uint8_t)rnd_int(15);
  s->justBuilt = 0;
}
/* positionSpritesMerging: an upper garden follows the plants' group of its stack's bottom garden */
static void garden_city_change(Perm *p) {
  const Perm *leader = p;
  while (leader->bottom && is_garden(P(leader->bottom))) leader = P(leader->bottom);
  if (leader == p) return;
  int g = garden_group(STC(GardenState, leader)->texture);
  if (g != garden_group(STC(GardenState, p)->texture)) garden_set_group(p, g);
}
static float garden_capacity(const Perm *p) { (void)p; return 40; }
static bool garden_open(const Perm *p) { return (!p->bottom || !is_garden(P(p->bottom))) && park_open(); }
static void garden_be_entertained(Perm *p, Cit *c, float timeMod) { (void)timeMod; park_walk(P(p->left), P(p->right), c); }
static void garden_window(Perm *p, WinOut *w) {
  if (p->cls == CLS_BotanicalGardensDome) win_button(w, 0, "Change Dome Variant", WA_CYCLE_3, true);
  win_button(w, 0, "Change Building Color", WA_CYCLE_0, true);
  win_button(w, 0, "Change Plants", WA_CYCLE_1, true);
  win_button(w, 0, "Change Plants Variant", WA_CYCLE_2, true);
}
/* setGroupOfSecondaryTexture: the whole connected stack of gardens gets the next group */
static void garden_action(Perm *p, int a) {
  GardenState *s = ST(GardenState, p);
  if (a == WA_CYCLE_0) {
    s->mainTexture = (uint8_t)((s->mainTexture + 1) % 3);
  } else if (a == WA_CYCLE_3) {
    s->domeVersion ^= 1;
  } else if (a == WA_CYCLE_1) {
    int g = (garden_group(s->texture) + 1) % 6;
    Drawer d, od;
    bld_drawer(p, &d);
    Perm *b = p;
    int cls = p->cls == CLS_BotanicalGardensDome ? CLS_BotanicalGardens : p->cls;
    for (;;) {
      Perm *below = P(b->bottom);
      if (!below || !class_is(below->cls, cls)) break;
      bld_drawer(below, &od);
      if (od.name != d.name && od.group != d.group) break;
      b = below;
    }
    for (;;) {
      garden_set_group(b, g);
      Perm *above = P(b->top);
      if (!above || !class_is(above->cls, cls)) break;
      bld_drawer(above, &od);
      if (od.name != d.name && od.group != d.group) break;
      b = above;
    }
  } else if (a == WA_CYCLE_2) {
    int g = garden_group(s->texture), first = garden_first(g);
    s->texture = (uint8_t)(first + (s->texture - first + 1) % garden_sets[g]);
  }
}
static void dome_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer == LAYER_BG) {
    Drawer d;
    bld_drawer(p, &d);
    const Perm *bo = P(p->bottom);
    bool hb = false;
    if (is_building(bo)) {
      Drawer od;
      bld_drawer(bo, &od);
      hb = (od.kind == DRW_ALLDIR || od.kind == DRW_ROOFDOWN) && (od.name == d.name || od.group == d.group);
    }
    draw_garden_plants(b, p, x, y, SPR_DOME_BOTANICALGARDEN_PLANTS, STC(GardenState, p)->texture, false, hb);
  }
}
const PermOps ops_BotanicalGardens = {.on_build = garden_on_build, .on_city_change = garden_city_change,
                                      .is_open = garden_open, .entertainment_capacity = garden_capacity,
                                      .be_entertained = garden_be_entertained, .draw = draw_merging, .window = garden_window,
                                      .window_action = garden_action};
const PermOps ops_BotanicalGardensDome = {.on_build = garden_on_build, .on_city_change = garden_city_change,
                                          .is_open = garden_open, .entertainment_capacity = garden_capacity,
                                          .be_entertained = garden_be_entertained, .draw = dome_draw,
                                          .window = garden_window, .window_action = garden_action};

/* ---------------------------------------------------------------- RooftopPark */
static void rooftop_neighbours(const Perm *p, const Perm **l, const Perm **r) {
  const Perm *bo = P(p->bottom);
  if (!bo) {
    *l = P(permanent_at_pos(perm_city_x(p) - 20, perm_city_y(p)));
    *r = P(permanent_at_pos(perm_city_x(p) + 20, perm_city_y(p)));
    if (!is_building(*l)) *l = NULL;
    if (!is_building(*r)) *r = NULL;
  } else {
    *l = P(bo->left) ? P(P(bo->left)->top) : NULL;
    *r = P(bo->right) ? P(P(bo->right)->top) : NULL;
  }
}
static void rooftop_park_init(Perm *p) { ST(RooftopParkState, p)->texture = (uint8_t)rnd_int(frames_of(SPR_ROOFTOPPARK_PLANTS, 20)); }
static float rooftop_capacity(const Perm *p) { (void)p; return 35; }
/* beEntertainedRooftopPark */
static void rooftop_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  const Perm *l, *r;
  rooftop_neighbours(p, &l, &r);
  bool lg = l && l->cls == CLS_RooftopPark, rg = r && r->cls == CLS_RooftopPark;
  if (lg && c->relativeX < 1) {
    cit_go_to_permanent(c, pid(l));
    c->relativeX += 20;
    walk_wait(c, 3, 15, 100, 180, true);
    return;
  }
  if (rg && c->relativeX > 19) {
    cit_go_to_permanent(c, pid(r));
    c->relativeX -= 20;
    walk_wait(c, 3, 15, 100, 180, true);
    return;
  }
  int go = rnd_int(1 + lg + rg);
  if (lg && go == 0) move_x_slow(c, 0);
  else if (rg && go == 1) move_x_slow(c, 20);
  else walk_wait(c, 0, 19, 100, 180, true);
}
static void rooftop_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer == LAYER_BG) dr_frame(b, SPR_ROOFTOPPARK_PLANTS, 20, STC(RooftopParkState, p)->texture, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
static void rooftop_window(Perm *p, WinOut *w) { (void)p; win_button(w, 0, "Change Plants", WA_CYCLE_0, true); }
static void rooftop_action(Perm *p, int a) {
  if (a == WA_CYCLE_0) {
    RooftopParkState *s = ST(RooftopParkState, p);
    s->texture = (uint8_t)((s->texture + 1) % frames_of(SPR_ROOFTOPPARK_PLANTS, 20));
  }
}
const PermOps ops_RooftopPark = {.init = rooftop_park_init, .is_open = park_is_open, .entertainment_capacity = rooftop_capacity,
                                 .be_entertained = rooftop_be_entertained, .draw = rooftop_draw, .window = rooftop_window,
                                 .window_action = rooftop_action};

/* ---------------------------------------------------------------- RedwoodTree and RedwoodTreeHouse */
static bool is_redwood(const Perm *q) { return q && class_is(q->cls, CLS_RedwoodTree); }
static Perm *redwood_primary(Perm *p) {
  while (p->top && is_redwood(P(p->top))) p = P(p->top);
  return p;
}
static void set_primary(Perm *p, bool on) {
  ST(RedwoodState, p)->isPrimary = on;
  if (on) p->flags |= PF_ROOFTOP;
  else p->flags &= (uint8_t)~PF_ROOFTOP;
}
static void redwood_init(Perm *p) {
  RedwoodState *s = ST(RedwoodState, p);
  s->lastExpanded = (float)city.timeSinceStart;
  s->subImage = (uint8_t)rnd_between(4, 7);
  set_primary(p, true);
}
void redwood_post_create(Perm *p) {
  /* (postCreate: rooftop only at the top of its trunk; the tree house keeps what its maker set) */
  if (p->cls == CLS_RedwoodTree) {
    bool top = redwood_primary(p) == p;
    if (top) p->flags |= PF_ROOFTOP;
    else p->flags &= (uint8_t)~PF_ROOFTOP;
  }
}
static void redwood_destroy(Perm *p) {
  /* the primary's status goes to the segment below */
  if (redwood_primary(p) == p && is_redwood(P(p->bottom))) set_primary(P(p->bottom), true);
}
static float redwood_capacity(const Perm *p) { (void)p; return 30; }
static void redwood_be_entertained(Perm *p, Cit *c, float timeMod) { (void)p, (void)c, (void)timeMod; }
/* updateSprite: the sub-image from the trunk's shape */
static int redwood_sub_image(const Perm *p) {
  const RedwoodState *s = STC(RedwoodState, p);
  const Perm *t = P(p->top), *b = P(p->bottom);
  if (!t && !b) return s->hasGrass ? 9 : 0;
  if (t && !t->top && !b) return s->hasGrass ? 10 : 1;
  if (!t && b && !b->bottom) return 2;
  if (!b) return s->hasGrass ? 11 : 3;
  if (!t) return 8;
  if (!t->top) return 7;
  return s->subImage;
}
static void redwood_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  if (p->cls == CLS_RedwoodTreeHouse) draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  else draw_decoration(b, p, layer, x, y, SPR_REDWOOD, redwood_sub_image(p), false, GFX_NO_TINT);
}
static bool redwood_can_build_house(const Perm *p) { return p->top && P(p->top)->top && p->bottom; }
static bool redwood_can_build_above(Perm *p) {
  Perm *pr = redwood_primary(p);
  return !perm_at(pr->world, pr->col, pr->row + 1) && pr->row + 1 < 255;
}
static void house_price(float m[M_COUNT]) {
  memset(m, 0, sizeof(float) * M_COUNT);
  m[M_WOOD] = 50, m[M_MACHINEPARTS] = 20, m[M_REFINEDMETAL] = 10;
  if (unlock_state(CLS_RedwoodTreeHouse) != UNLOCK_RESEARCHED) m[M_KNOWLEDGE] = 2500;
}
static void redwood_window(Perm *p, WinOut *w) {
  RedwoodState *s = ST(RedwoodState, p);
  bool house = p->cls == CLS_RedwoodTreeHouse;
  if (!house) {
    if (p->top && p->bottom && P(p->top)->top) win_button(w, 0, "Change Variant", WA_CYCLE_0, true);
    if (!p->bottom) win_button(w, 0, s->hasGrass ? "Remove Grass" : "Add Grass", WA_TOGGLE, true);
  }
  Perm *pr = redwood_primary(p);
  if (city.timeSinceStart > STC(RedwoodState, pr)->lastExpanded + 1440) {
    static const float grow[M_COUNT] = {[M_WOOD] = 100, [M_FOOD] = 50};
    win_button(w, 0, "Make Redwood Grow", WA_BUILD, mat_can_afford(grow));
    win_text(w, "Make this redwood grow taller. You can do this once every 24 hours.");
    win_cost(w, grow);
  } else {
    int hrs = (int)ceil(24 - (city.timeSinceStart - STC(RedwoodState, pr)->lastExpanded) / 60);
    if (hrs <= 1) win_line(w, "You can make this redwood grow in one hour.");
    else win_line1(w, "You can make this redwood grow in [0] hours.", hrs);
  }
  if (!house && redwood_can_build_house(p)) {
    float m[M_COUNT];
    house_price(m);
    win_button(w, 0, "Build Tree House", WA_BUILD_2, mat_can_afford(m));
    win_text(w, "Build a tree house here. It can house two people.");
    win_cost(w, m);
  } else if (house) {
    win_button(w, 0, "Remove Tree House", WA_BUILD_2, true);
    win_text(w, "");   /* (createActivatableButton without a description) */
  }
  win_text(w, "Texture and concept by Luk XD 99");
}
/* the segment replaced by a tree house (or back): destroyForReplacement and World.build, not primary */
static uint16_t redwood_replace(Perm *p, int cls) {
  int w = p->world, col = p->col, row = p->row;
  perm_destroy(pid(p), true);
  uint16_t id = perm_new(cls, w, col, row);
  if (!id) return NONE;
  bld_on_build(P(id));
  set_primary(P(id), false);
  city_update_connections();
  sim_update_pathfinder(true, id);
  unlocks_research(cls);
  bld_post_create(P(id));
  unlocks_check_build_related();
  return id;
}
static void redwood_action(Perm *p, int a) {
  RedwoodState *s = ST(RedwoodState, p);
  if (a == WA_CYCLE_0) {
    s->subImage = (uint8_t)(4 + (s->subImage - 4 + 1) % 3);
  } else if (a == WA_TOGGLE) {
    s->hasGrass ^= 1;
  } else if (a == WA_BUILD) {
    static const float grow[M_COUNT] = {[M_WOOD] = 100, [M_FOOD] = 50};
    Perm *pr = redwood_primary(p);
    if (city.timeSinceStart <= STC(RedwoodState, pr)->lastExpanded + 1440) return;
    if (!mat_can_afford(grow) || !redwood_can_build_above(p)) return;
    mat_pay(grow);
    set_primary(pr, false);
    int w = pr->world, col = pr->col, row = pr->row + 1;
    uint16_t id = perm_new(CLS_RedwoodTree, w, col, row);
    if (!id) return;
    bld_on_build(P(id));
    set_primary(P(id), true);
    city_update_connections();
    sim_update_pathfinder(false, id);
    unlocks_research(CLS_RedwoodTree);
    bld_post_create(P(id));
    unlocks_check_build_related();
  } else if (a == WA_BUILD_2) {
    float m[M_COUNT];
    house_price(m);
    if (p->cls == CLS_RedwoodTreeHouse) {
      for (int i = 0; i < M_COUNT; i++) city.materials.m[i] += m[i] * upgrades.recyclingAmount;
      redwood_replace(p, CLS_RedwoodTree);
    } else if (mat_can_afford(m) && redwood_can_build_house(p)) {
      mat_pay(m);
      redwood_replace(p, CLS_RedwoodTreeHouse);
    }
  }
}
/* tryDestroy: not where a tree house would be left in a wrong place */
void redwood_destroy_refusal(const Perm *p, const char **why) {
  const Perm *t = P(p->top), *b = P(p->bottom);
  bool house_t = t && t->cls == CLS_RedwoodTreeHouse, house_b = b && b->cls == CLS_RedwoodTreeHouse;
  const Perm *bb = b ? P(b->bottom) : NULL;
  if ((!t && b && bb && bb->cls == CLS_RedwoodTreeHouse) || (t && !t->top && house_b) || (!b && house_t))
    *why = "You cannot destroy this redwood tree, as that would result a tree house to be placed in an incorrect location.";
}
static bool never_open_any(const Perm *p) { (void)p; return false; }   /* get_isOpen: false */
static void redwood_house_walk(Perm *p, Cit *c, float steps) {
  (void)p, (void)steps;
  c->relativeY = 5;
  int x = rnd_between(2, 17);
  cit_move_and_wait(c, x, rnd_between(60, 90), THEN_NONE, 0, false, false);
}
const PermOps ops_RedwoodTree = {.init = redwood_init, .destroy = redwood_destroy, .entertainment_capacity = redwood_capacity,
                                 .is_open = never_open_any, .be_entertained = redwood_be_entertained, .draw = redwood_draw,
                                 .window = redwood_window, .window_action = redwood_action};
const PermOps ops_RedwoodTreeHouse = {.init = redwood_init, .destroy = redwood_destroy, .entertainment_capacity = redwood_capacity,
                                      .is_open = never_open_any, .be_entertained = redwood_be_entertained,
                                      .walk_around = redwood_house_walk, .draw = redwood_draw, .window = redwood_window,
                                      .window_action = redwood_action};

/* ---------------------------------------------------------------- NightClub */
static bool is_club(const Perm *q) { return q && q->cls == CLS_NightClub; }
static float club_capacity(const Perm *p) {
  float q = has_upgrade_bit(p, UB_NightClub_BetterLightShow) ? 40 : 0;   /* nightClubEntertainmentQuantity */
  return p->nworkers * (40 + q) + upgrades.nightClubEntertainmentQualityBonus;
}
static bool club_open(const Perm *p) { return p->nworkers == 1 && open_between(p, 20 - 0.5f, 6); }
static void club_work(Perm *p, Cit *c, float timeMod, bool stop) {
  NightClubState *s = ST(NightClubState, p);
  if (stop && hour_now() > 6) {
    c->currentAction = 2;
  } else if (is_club(P(p->left))) {
    walk_wait(c, 0, is_club(P(p->right)) ? 19 : 17, 20, 50, false);
    s->lightsAreOn = 60 * 4;
  } else {
    step_to_x(c, 1, timeMod);
    s->lightsAreOn = 3 * 4;
  }
}
static void club_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  NightClubState *s = ST(NightClubState, p);
  if (!s->knownCitizen) s->knownCitizen = cid(c), s->knownSet = 60;
  else if (s->knownCitizen == cid(c)) s->knownSet = 60;
  int wait_mod = s->color == 1 ? -3 : s->color == 2 || s->color == 4 ? 3 : 0;
  const Perm *l = P(p->left), *r = P(p->right);
  bool hl = is_club(l), hr = is_club(r);
  int go = rnd_int(2 + hl + hr);
  bool lr = s->color != 4 || s->knownCitizen != cid(c);
  if (hl && lr && go == 0 && club_open(l)) {
    int16_t st[] = {2, 0, 4, (int16_t)rnd_between(5, 19), 8, (int16_t)rnd_between(5, 6)};
    cit_set_path(c, st, 3);
  } else if (hr && lr && go == 1 && club_open(r)) {
    int16_t st[] = {3, 0, 4, (int16_t)rnd_between(0, 17), 8, (int16_t)rnd_between(5, 6)};
    cit_set_path(c, st, 3);
  } else {
    walk_wait(c, hl ? 0 : 5, hr ? 19 : 17, 5 + wait_mod, 6 + wait_mod, s->color == 2 || s->color == 4);
  }
  night_visit(c);
}
static bool dj_working_here(const Perm *p) {
  Cit *dj = worker_at(p, 0);
  return dj && dj->inPermanent == pid(p) && dj->currentAction == 0;
}
static void club_update(Perm *p, float timeMod) {
  NightClubState *s = ST(NightClubState, p);
  int dec = round_random(fminf(1, timeMod) * 4);
  s->lightsAreOn = (uint8_t)(s->lightsAreOn > dec ? s->lightsAreOn - dec : 0);
  if (s->knownCitizen) {
    if (s->knownSet < timeMod) s->knownCitizen = 0, s->knownSet = 0;
    else s->knownSet = tick_down(s->knownSet, timeMod);
  }
}
/* the light show, from the clock: lasers sweeping between their ends, lights fading in and out */
static float triangle(float t, float lo, float hi) {
  float span = hi - lo;
  if (span <= 0) return lo;
  float m = fmodf(t, 2 * span);
  if (m < 0) m += 2 * span;
  return m < span ? lo + m : hi - (m - span);
}
static uint16_t club_laser_tint(const Perm *p, int k, float t) {
  int color = STC(NightClubState, p)->color;
  static const float start_hue[4] = {180, 0, 90, 270};
  float hue;
  switch (color) {
  case 1: hue = 0; break;
  case 2: hue = 170 + 75 * (0.5f + 0.5f * sinf(t * 0.004f + k * 1.7f)); break;
  case 3: hue = fmodf(t * 0.6f + k * 97, 360); break;
  case 4: hue = 270 + 60 * (0.5f + 0.5f * sinf(t * 0.004f + k * 1.3f)); break;
  default: hue = fmodf(start_hue[k] + t, 360); break;
  }
  return tint565(hsv_to_rgb(hue, 0.7f, 1));
}
static void club_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer != LAYER_BG) return;
  const NightClubState *s = STC(NightClubState, p);
  if (s->lightsAreOn == 0 && !dj_working_here(p)) return;
  float t = bld_anim + (hash32(pid(p)) & 1023);
  /* the two lights at (5, 4) and (10, 4) */
  for (int k = 0; k < 2; k++) {
    float period = s->color == 1 || s->color == 3 ? 35 : s->color == 4 ? 110 : 50;
    float ph = t + k * 17;
    int cycle = (int)(ph / period);
    float in = fmodf(ph, period);
    float rise = s->color == 4 ? 40 : 10;
    float a = in < rise ? in / rise : in < 2 * rise ? 2 - in / rise : 0;
    if (a <= 0) continue;
    uint32_t h = hash32((uint32_t)pid(p) * 7919u + (uint32_t)k * 104729u + (uint32_t)cycle);
    uint32_t rgb;
    switch (s->color) {
    case 1: rgb = hsl_to_rgb((float)(h % 60), 0.8f, 0.6f); break;
    case 2: rgb = hsl_to_rgb((float)(170 + h % 75), 0.8f, 0.7f); break;
    case 3: rgb = 0xFFFFFF; break;
    case 4: rgb = hsl_to_rgb((float)(270 + h % 60), 0.8f, 0.7f); break;
    default: rgb = hsl_to_rgb((float)(h % 360), 0.8f, 0.5f); break;
    }
    dr_part(b, s->color == 3 ? SPR_NIGHTCLUB_LIGHT_RAINBOW : SPR_NIGHTCLUB_LIGHT, 0, 0, 5, 15, x, y, 5 + 5 * k, 4, 0,
            (uint8_t)(a * 255), tint565(rgb));
  }
  /* lasers: from (2, 3) and (17, 3), and (7, 4) and (12, 4) with the better light show */
  static const int8_t sx[4] = {2, 17, 7, 12}, sy[4] = {3, 3, 4, 4};
  int n = has_upgrade_bit(p, UB_NightClub_BetterLightShow) ? 4 : 2;
  float lo = is_club(P(p->left)) ? 1 : 6;
  for (int k = 0; k < n; k++) {
    float speed = s->color == 2 || s->color == 4 ? 0.65f : s->color == 1 ? 1.5f : 1.0f;
    float ex = triangle(t * speed + k * 23, lo, 18);
    int f = (int)floorf(fabsf(ex - sx[k]));
    f = f < 0 ? 0 : f > 17 ? 17 : f;
    bool flip = ex < sx[k];
    dr_part(b, SPR_NIGHTCLUB_LASER, f * 20, 0, 18, spr_h(SPR_NIGHTCLUB_LASER), x, y, flip ? sx[k] + 1 - 18 : sx[k], sy[k],
            flip ? GFX_FLIPX : 0, 255, club_laser_tint(p, k, t));
  }
}
static void club_window(Perm *p, WinOut *w) {
  if (!bld_secret_dance) return;
  static const char *const names[5] = {"Colorful", "Fast & Red", "Slow & Blue", "Rainbow Light", "Romantic & Pink"};
  char b[96];
  int k = 0;
  for (const char *s = "Change Light Show ("; *s; s++) b[k++] = *s;
  for (const char *s = names[STC(NightClubState, p)->color % 5]; *s; s++) b[k++] = *s;
  b[k++] = ')';
  b[k] = 0;
  win_button(w, 0, b, WA_CYCLE_0, true);
}
static void club_action(Perm *p, int a) {
  if (a == WA_CYCLE_0 && bld_secret_dance) {
    NightClubState *s = ST(NightClubState, p);
    s->color = (uint8_t)((s->color + 1) % 5);
  }
}
const PermOps ops_NightClub = {.work = club_work, .update = club_update, .is_open = club_open,
                               .entertainment_capacity = club_capacity, .be_entertained = club_be_entertained,
                               .draw = club_draw, .window = club_window, .window_action = club_action};

/* ---------------------------------------------------------------- ScrapyardNightClub */
static bool is_scrapyard(const Perm *q) { return q && q->cls == CLS_ScrapyardNightClub; }
static void scrapyard_init(Perm *p) {
  ScrapyardState *s = ST(ScrapyardState, p);
  s->texLeft = (uint8_t)rnd_int(frames_of(SPR_SYNIGHTCLUB_LEFTTEXTURES, 20));
  s->texOther = (uint8_t)rnd_int(frames_of(SPR_SYNIGHTCLUB_MAINTEXTURES, 20));
  s->shownLeft = 0;   /* (as written: the other machines show until the left neighbour changes) */
}
static float scrapyard_capacity(const Perm *p) { return p->nworkers * 120 + upgrades.nightClubEntertainmentQualityBonus; }
static bool scrapyard_open(const Perm *p) { return p->nworkers == 1 && open_between(p, 20 - 0.5f, 6); }
static void scrapyard_city_change(Perm *p) {
  ScrapyardState *s = ST(ScrapyardState, p);
  bool hl = is_scrapyard(P(p->left));
  if (s->hadLeft != hl) {
    s->shownLeft = !hl;
    s->hadLeft = hl;
  }
}
static void scrapyard_work(Perm *p, Cit *c, float timeMod, bool stop) {
  ScrapyardState *s = ST(ScrapyardState, p);
  if (stop && hour_now() > 6) {
    c->currentAction = 2;
  } else if (is_scrapyard(P(p->left))) {
    walk_wait(c, 0, is_scrapyard(P(p->right)) ? 19 : 17, 20, 50, false);
    s->lightsAreOn = 60 * 4;
  } else {
    step_to_x(c, 1, timeMod);
    s->lightsAreOn = 3 * 4;
  }
}
static void scrapyard_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  const Perm *l = P(p->left), *r = P(p->right);
  bool hl = is_scrapyard(l), hr = is_scrapyard(r);
  int go = rnd_int(2 + hl + hr);
  if (hl && go == 0 && scrapyard_open(l)) {
    int16_t st[] = {2, 0, 4, (int16_t)rnd_between(5, 19), 8, (int16_t)rnd_between(5, 6)};
    cit_set_path(c, st, 3);
  } else if (hr && go == 1 && scrapyard_open(r)) {
    int16_t st[] = {3, 0, 4, (int16_t)rnd_between(0, 17), 8, (int16_t)rnd_between(5, 6)};
    cit_set_path(c, st, 3);
  } else {
    walk_wait(c, hl ? 0 : 5, hr ? 19 : 17, 8, 9, false);
  }
  night_visit(c);
}
static void scrapyard_update(Perm *p, float timeMod) {
  ScrapyardState *s = ST(ScrapyardState, p);
  int dec = round_random(fminf(1, timeMod) * 4);
  s->lightsAreOn = (uint8_t)(s->lightsAreOn > dec ? s->lightsAreOn - dec : 0);
  s->active = s->lightsAreOn > 0 || dj_working_here(p);
}
/* the machines' lights, each pair of textures its own (secondaryBackgroundSprites), from the clock */
static void sy_light(const GfxBand *b, uint16_t spr, int x, int y, int dx, int dy, float sxs, float sys, float hue, float sat,
                     float alpha) {
  int w = spr_w(spr), h = spr_h(spr);
  int dw = (int)(w * sxs + 0.5f), dh = (int)(h * sys + 0.5f);
  if (dw < 1) dw = 1;
  if (dh < 1) dh = 1;
  uint16_t t = tint565(hsv_to_rgb(hue, sat, 1));
  uint8_t a = (uint8_t)(alpha < 0 ? 0 : alpha > 1 ? 255 : alpha * 255);
  /* (scaled lights are drawn at their scaled size, a pixel column or row of the sprite repeated) */
  for (int i = 0; i < dh; i += h)
    for (int j = 0; j < dw; j += w) dr_part(b, spr, 0, 0, dw - j < w ? dw - j : w, dh - i < h ? dh - i : h, x, y, dx + j, dy + i, 0, a, t);
}
static void scrapyard_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer != LAYER_BG) return;
  const ScrapyardState *s = STC(ScrapyardState, p);
  bool left = s->shownLeft;
  int tex = left ? s->texLeft : s->texOther;
  dr_frame(b, left ? SPR_SYNIGHTCLUB_LEFTTEXTURES : SPR_SYNIGHTCLUB_MAINTEXTURES, 20, tex, x, y, 0, 0, 0, 255, GFX_NO_TINT);
  if (!s->active) return;
  float t = bld_anim;
  if (left) {
    if (tex == 0) {
      static const int8_t pos[4][2] = {{3, 2}, {7, 4}, {12, 3}, {16, 4}};
      static const float ys[4] = {0.8f, 0.4f, 0.8f, 0.5f};
      for (int k = 0; k < 4; k++) sy_light(b, SPR_SYNIGHTCLUB_LIGHTS, x, y, pos[k][0], pos[k][1], 1, ys[k], fmodf(72 * k + t, 360), 0.65f, 1);
    } else if (tex == 1) {
      float base = fmodf((float)(p->col * 60 + p->row * 75) + 0.5f * t, 360);
      static const int8_t pos[3][2] = {{2, 1}, {6, 5}, {10, 9}};
      for (int k = 0; k < 3; k++) sy_light(b, SPR_SYNIGHTCLUB_WATERFALLLIGHT, x, y, pos[k][0], pos[k][1], 1, 1, base + 25 * k, 0.7f, 1);
      sy_light(b, SPR_SYNIGHTCLUB_WATERFALLLIGHT_LAST, x, y, 15, 13, 1, 1, base + 75, 0.7f, 1);
    } else {
      static const int8_t pos[3][2] = {{4, 3}, {3, 7}, {6, 11}};
      static const float xs[3] = {1.4f, 1.5f, 1.2f};
      for (int k = 0; k < 3; k++) {
        float hue = 120 * k + 180 * (0.5f + 0.5f * sinf(t * 0.01f + k * 2.1f + p->col));
        sy_light(b, SPR_SYNIGHTCLUB_LIGHTS_H, x, y, pos[k][0], pos[k][1], xs[k], 1, hue, 0.65f, 1);
      }
    }
  } else if (tex == 0) {
    static const int8_t pos[5][2] = {{2, 3}, {6, 2}, {10, 3}, {14, 4}, {17, 2}};
    static const float ys[5] = {1.1f, 1, 1.1f, 1.3f, 1};
    for (int k = 0; k < 5; k++) sy_light(b, SPR_SYNIGHTCLUB_LIGHTS, x, y, pos[k][0], pos[k][1], 1, ys[k], fmodf(60 * k + t, 360), 0.65f, 1);
  } else if (tex == 1) {
    static const int8_t pos[9][2] = {{2, 2}, {1, 7}, {2, 13}, {8, 3}, {6, 8}, {8, 13}, {13, 3}, {13, 8}, {12, 13}};
    for (int k = 0; k < 9; k++) {
      /* each mini light: a wait, a colour, fade in, hold, fade out */
      float ph = t + k * 37 + (hash32(pid(p) * 31u + (uint32_t)k) & 127);
      int cycle = (int)(ph / 100);
      float in = fmodf(ph, 100);
      uint32_t h = hash32((uint32_t)pid(p) * 977u + (uint32_t)k * 131u + (uint32_t)cycle);
      float a = in < 10 ? in / 10 : in < 65 ? 1 : in < 75 ? (75 - in) / 10 : 0;
      if (a > 0) sy_light(b, SPR_SYNIGHTCLUB_MINILIGHT, x, y, pos[k][0], pos[k][1], 1, 1, (float)(h % 360), 0.6f + (h >> 9) % 30 / 100.0f, a);
    }
  } else {
    static const int8_t pos[6][2] = {{3, 3}, {4, 3}, {9, 8}, {10, 8}, {15, 5}, {16, 5}};
    static const float ys[6] = {1.2f, 1.2f, 0.7f, 0.7f, 1, 1};
    for (int k = 0; k < 6; k++) {
      float hue = fmodf(120 * k + (k & 1 ? -1.5f : 1.5f) * t, 360);
      if (hue < 0) hue += 360;
      sy_light(b, SPR_SYNIGHTCLUB_LIGHTS, x, y, pos[k][0], pos[k][1], 1, ys[k], hue, 0.75f - 0.01f * k, 1);
    }
  }
}
static void scrapyard_window(Perm *p, WinOut *w) { (void)p; win_button(w, 0, "Change Machines", WA_CYCLE_0, true); }
static void scrapyard_action(Perm *p, int a) {
  if (a != WA_CYCLE_0) return;
  ScrapyardState *s = ST(ScrapyardState, p);
  if (is_scrapyard(P(p->left))) {
    s->texOther = (uint8_t)((s->texOther + 1) % frames_of(SPR_SYNIGHTCLUB_MAINTEXTURES, 20));
    s->shownLeft = 0;
  } else {
    s->texLeft = (uint8_t)((s->texLeft + 1) % frames_of(SPR_SYNIGHTCLUB_LEFTTEXTURES, 20));
    s->shownLeft = 1;
  }
}
const PermOps ops_ScrapyardNightClub = {.init = scrapyard_init, .work = scrapyard_work, .update = scrapyard_update,
                                        .on_city_change = scrapyard_city_change, .is_open = scrapyard_open,
                                        .entertainment_capacity = scrapyard_capacity,
                                        .be_entertained = scrapyard_be_entertained, .draw = scrapyard_draw,
                                        .window = scrapyard_window, .window_action = scrapyard_action};

/* ---------------------------------------------------------------- ModernArtMuseum */
extern uint8_t modern_artworks_unlocked;   /* unlocks.numberOfModernArtMuseumArtworksUnlocked (bld_unique.c) */
static uint8_t TH_TOUR_1, TH_TOUR_2, TH_TOUR_3, TH_TOUR_4;
static void museum_init(Perm *p) { ST(ArtMuseumState, p)->texture = (uint8_t)rnd_int(modern_artworks_unlocked); }
static void museum_on_build(Perm *p) { (void)p; unlocks_unlock("ArtColony"); }
static float museum_capacity(const Perm *p) { return workers_times(p, 100); }
static bool museum_open(const Perm *p) { return p->nworkers == 1 && first_worker_working(p) && hour_now() < 21.5f; }
static void th_museum_move(Cit *c, uint16_t arg) { (void)arg; walk_wait(c, 3, 16, 100, 120, true); }
static void museum_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (c->relativeY < 5) cit_change_floor(c, TH_MUSEUM_MOVE, 0);
  else th_museum_move(c, 0);
}
/* the curator's tour: upstairs, to the far end and back, downstairs */
static void th_tour_1(Cit *c, uint16_t arg) { (void)arg; cit_move(c, rnd_between(14, 15), TH_TOUR_2, 0, true); }
static void th_tour_2(Cit *c, uint16_t arg) { (void)arg; cit_wait_random(c, 30, 60, TH_TOUR_3, 0); }
static void th_tour_3(Cit *c, uint16_t arg) { (void)arg; cit_move(c, rnd_between(4, 6), TH_TOUR_4, 0, true); }
static void th_tour_4(Cit *c, uint16_t arg) { (void)arg; cit_wait_random(c, 30, 60, TH_CHANGE_FLOOR, 0); }
static void museum_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p, (void)timeMod;
  if (stop) c->currentAction = 2;
  else if (c->relativeX >= 4 && c->relativeX < 6) {
    if (rnd_int(10) == 0) cit_change_floor_and_wait_random(c, 30, 60, TH_TOUR_1, 0);
    else wait_ticks(c, 30, 60);
  } else {
    move_x(c, rnd_between(4, 6));
  }
}
static void museum_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer == LAYER_BG) dr_frame(b, SPR_MODERNART, 20, STC(ArtMuseumState, p)->texture, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
static void museum_window(Perm *p, WinOut *w) { (void)p; win_button(w, 0, "Change Exhibition", WA_CYCLE_0, true); }
static void museum_action(Perm *p, int a) {
  if (a == WA_CYCLE_0 && modern_artworks_unlocked) {
    ArtMuseumState *s = ST(ArtMuseumState, p);
    s->texture = (uint8_t)((s->texture + 1) % modern_artworks_unlocked);
  }
}
const PermOps ops_ModernArtMuseum = {.init = museum_init, .on_build = museum_on_build, .work = museum_work,
                                     .is_open = museum_open, .entertainment_capacity = museum_capacity,
                                     .be_entertained = museum_be_entertained, .draw = museum_draw, .window = museum_window,
                                     .window_action = museum_action};

/* ---------------------------------------------------------------- Arcade */
static float arcade_capacity(const Perm *p) { return workers_times(p, 120); }
static bool arcade_open(const Perm *p) { return p->nworkers == 1 && first_worker_working(p) && hour_now() < 23; }
static void th_arcade_move(Cit *c, uint16_t arg) {
  (void)arg;
  int xx = c->relativeY < 5 ? 5 + 4 * rnd_int(3) : 5 + 8 * rnd_int(2);
  walk_wait(c, xx, xx + 1, 100, 120, false);
}
static void arcade_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (rnd_int(3) == 1) cit_change_floor(c, TH_ARCADE_MOVE, 0);
  else th_arcade_move(c, 0);
}
static void arcade_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p;
  if (stop) c->currentAction = 2;
  else step_to_x(c, 6, timeMod);
}
const PermOps ops_Arcade = {.work = arcade_work, .is_open = arcade_open, .entertainment_capacity = arcade_capacity,
                            .be_entertained = arcade_be_entertained, .draw = plain_draw};

/* ---------------------------------------------------------------- HoloGameHall */
static uint8_t TH_HGH_BLOCK;
static bool is_hall(const Perm *q) { return q && q->cls == CLS_HoloGameHall; }
static Perm *hall_coordinator(Perm *p) {
  while (is_hall(P(p->left))) p = P(p->left);
  return p;
}
static int hall_total_width(Perm *coord) {
  int w = 18;
  for (Perm *q = coord; is_hall(P(q->right)); q = P(q->right)) w += 20;
  return w;
}
static void hall_set_game_type(Perm *p) {
  HoloState *s = ST(HoloState, p);
  s->gameType = (is_hall(P(p->left)) || is_hall(P(p->right))) && rnd_int(2) == 0 ? 0 : 1;
}
static void hall_clear(Perm *p) {
  HoloState *s = ST(HoloState, p);
  memset(s->blocks, 0, sizeof s->blocks);
  s->progress1 = s->progress2 = 0;
}
void hall_post_create(Perm *p) {
  /* calculateCoordinatorBuilding, then setGameType */
  hall_set_game_type(p);
  Perm *coord = hall_coordinator(p);
  ST(HoloState, p)->gameType = STC(HoloState, coord)->gameType;
}
static void hall_city_change(Perm *p) {
  HoloState *s = ST(HoloState, p);
  Perm *coord = hall_coordinator(p);
  if (!is_hall(P(p->left)) && !is_hall(P(p->right)) && s->gameType == 0) {
    s->gameType = 1;
    hall_clear(p);
  }
  if (s->gameType != STC(HoloState, coord)->gameType) {
    hall_clear(p);
    s->gameType = STC(HoloState, coord)->gameType;
  }
  if (!is_hall(P(p->right))) s->blocks[4] = 0;
}
static float hall_capacity(const Perm *p) { (void)p; return 75; }
static bool hall_open(const Perm *p) {
  (void)p;
  float h = hour_now();
  return h >= 12 || h < 2;
}
/* the block game: a visitor adds a block in a column, or brightens the newest, or tints one */
static void th_hgh_block(Cit *c, uint16_t arg) {
  (void)c;
  Perm *p = P(arg & 0x3FF);
  int col = arg >> 12 & 7;
  if (!is_hall(p) || STC(HoloState, hall_coordinator(p))->gameType != 1 || (col == 4 && !is_hall(P(p->right)))) return;
  HoloState *s = ST(HoloState, p);
  int n = s->blocks[col] & 7, alpha = s->blocks[col] >> 3;
  if (n == 0 || alpha >= 10) {
    s->gameInited = 1;
    if (n < 4) s->blocks[col] = (uint8_t)((n + 1) | 1 << 3);
    else s->hueShift++;
  } else {
    s->blocks[col] = (uint8_t)(n | (alpha + 1) << 3);
  }
}
static void hall_be_entertained(Perm *p, Cit *c, float timeMod) {
  Perm *coord = hall_coordinator(p);
  HoloState *s = ST(HoloState, p);
  bool hl = is_hall(P(p->left)), hr = is_hall(P(p->right));
  if (STC(HoloState, coord)->gameType == 1) {
    int k = rnd_int(4 + hr), r = rnd_int(6);
    if (hl && r == 0) {
      int16_t st[] = {2, 0, 4, 19};
      cit_set_path(c, st, 2);
    } else if (hr && r == 1) {
      int16_t st[] = {3, 0, 4, 0};
      cit_set_path(c, st, 2);
    } else {
      cit_move_and_wait(c, 3 + k * 4, rnd_between(45, 90), TH_HGH_BLOCK, (uint16_t)(pid(p) | k << 12), false, false);
    }
  } else {
    /* the spaceship battle: two teams push their monsters toward each other (team by citizen) */
    HoloState *cs = ST(HoloState, coord);
    int team = (cid(c) + day_number()) & 1;
    int total = hall_total_width(coord) - 28;
    float pr1 = cs->progress1 / 65535.0f, pr2 = cs->progress2 / 65535.0f;
    float x = (float)(perm_city_x(p) - perm_city_x(coord)) + c->relativeX;
    float mpos = team == 0 ? 1 + floorf(pr1 * total) : hall_total_width(coord) - 14 + 1 - floorf(pr2 * total);
    if (x > mpos && x < mpos + 13 && rnd_range(0, 30) > timeMod && total > 0) {
      float *mine = team == 0 ? &pr1 : &pr2, *other = team == 0 ? &pr2 : &pr1;
      *mine = fminf(*mine + timeMod / (total * (10 + 20 * *mine)), 1);
      if (*mine + *other > 1) *other = 1 - *mine;
      cs->progress1 = (uint16_t)(pr1 * 65535), cs->progress2 = (uint16_t)(pr2 * 65535);
    } else {
      int wx = perm_city_x(coord) + (int)mpos;
      cit_move_towards_random_in_world_coords(c, wx, wx + 13, THEN_NONE, 0);
    }
  }
  s->gameType = STC(HoloState, coord)->gameType;
}
static void hall_update(Perm *p, float timeMod) {
  (void)timeMod;
  HoloState *s = ST(HoloState, p);
  if (!hall_open(p) && hour_now() > 4) {
    /* after closing: the game is cleared and a new kind of game chosen */
    if (s->gameInited || s->progress1 || s->progress2) {
      hall_clear(p);
      s->gameInited = 0;
      if (hall_coordinator(p) == p) hall_set_game_type(p);
    }
  }
}
static void hall_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer != LAYER_BG) return;
  const HoloState *s = STC(HoloState, p);
  if (s->gameType == 1) {
    for (int col = 0; col < 5; col++) {
      int n = s->blocks[col] & 7, alpha = s->blocks[col] >> 3;
      for (int i = 0; i < n; i++) {
        uint32_t h = hash32((uint32_t)pid(p) * 131u + (uint32_t)col * 17u + (uint32_t)i);
        float hue = (float)(h % 360) + 5.0f * ((s->hueShift * (h >> 12 & 3)) & 63);
        float a = i == n - 1 ? alpha / 10.0f : 1;
        dr_part(b, SPR_HGH_BLOCK, 0, 0, 4, 4, x, y, 2 + col * 4, 20 - 1 - 4 * (i + 1), 0, (uint8_t)(a * 255),
                tint565(hsv_to_rgb(hue, 1, 1)));
      }
    }
  } else if (hall_coordinator((Perm *)p) == p) {
    /* the monsters of the run (drawn by its leftmost hall) */
    int total = hall_total_width((Perm *)p) - 28;
    if (total <= 0) return;
    int x1 = 1 + (int)floorf(s->progress1 / 65535.0f * total);
    int x2 = hall_total_width((Perm *)p) - 14 + 1 - (int)floorf(s->progress2 / 65535.0f * total);
    dr_part(b, SPR_HGH_MONSTER_1, 0, 0, 14, 9, x, y, x1, 10, 0, 255, rgb565(0xFF0000));
    dr_part(b, SPR_HGH_MONSTER_1, 0, 0, 14, 9, x, y, x2, 10, GFX_FLIPX, 255, rgb565(0x0000FF));
  }
}
const PermOps ops_HoloGameHall = {.on_city_change = hall_city_change, .update = hall_update, .is_open = hall_open,
                                  .entertainment_capacity = hall_capacity, .be_entertained = hall_be_entertained,
                                  .draw = hall_draw};

void bld_entertainment_init(void) {
  TH_PUB_MOVE = then_register(th_pub_move);
  TH_FACTORYPUB_MOVE = then_register(th_factory_pub_move);
  TH_TABLE = then_register(th_table);
  TH_LUX_TABLE = then_register(th_lux_table);
  TH_ARCADE_MOVE = then_register(th_arcade_move);
  TH_MUSEUM_MOVE = then_register(th_museum_move);
  TH_TOUR_1 = then_register(th_tour_1);
  TH_TOUR_2 = then_register(th_tour_2);
  TH_TOUR_3 = then_register(th_tour_3);
  TH_TOUR_4 = then_register(th_tour_4);
  TH_HGH_BLOCK = then_register(th_hgh_block);
}
