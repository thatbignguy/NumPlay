/* The Final Earth 2: production buildings (buildings.md 7): the woodcutters'
 * and stone miners' centres (BlueCollarWork), farms, the exploration centre,
 * the factories (Factory, MaterialConvertingFactory), labs, tree
 * plantations, rock teleporters, eco farms (FarmGrowArea) and the farm
 * by-product processor. */
#include "bldi.h"

static inline float speed(void) { return happiness.actionSpeedModifier; }
static inline float edu(const Cit *c) { return cit_education_speed_modifier(c); }
static inline int arg_perm(uint16_t arg) { return arg & 0x3FF; }

/* ---------------------------------------------------------------- BlueCollarWork */
enum { GATHER_WOOD, GATHER_STONE };
static uint8_t TH_DROP_TOOLS, TH_TAKE_TOOLS, TH_CUT_WOOD, TH_MINE_STONE;

static bool gather_option(const Perm *p, int kind) {
  if (!p || !(p->flags & PF_USED)) return false;
  if (kind == GATHER_WOOD) return is_a(p, B_FOREST) || p->cls == CLS_TreePlantation;
  return is_a(p, B_ROCK) || p->cls == CLS_StoneTeleporter;
}
typedef struct { int kind; bool prioritized; } GatherGoal;
static bool gather_goal(const Perm *p, const void *ctx) {
  const GatherGoal *g = ctx;
  if (!gather_option(p, g->kind) || limited_materials_left(p) <= 0 || limited_do_not_gather(p)) return false;
  return !g->prioritized || resource_prioritized(p);
}

static void th_drop_tools(Cit *c, uint16_t arg) { (void)arg; c->flags &= (uint16_t)~CF_HAS_WORK_TOOLS; }
static void th_take_tools(Cit *c, uint16_t arg) { (void)arg; c->flags |= CF_HAS_WORK_TOOLS; }

/* after a gather action: the place may be empty now */
static void gathered(Cit *c, Perm *res) {
  Perm *centre = P(c->job);
  BlueCollarState *bs = centre && (centre->cls == CLS_WoodcuttingCentre || centre->cls == CLS_StoneMine) ? ST(BlueCollarState, centre) : NULL;
  if (limited_materials_left(res) <= 0 && !limited_stay_if_empty(res)) {
    if (bs) bs->nearest = NONE;
    if (limited_destroyed_on_empty(res)) perm_destroy(pid(res), false);
  }
}
/* WoodcuttingCentre's getResource */
static void th_cut_wood(Cit *c, uint16_t arg) {
  Perm *res = P(arg_perm(arg));
  if (!gather_option(res, GATHER_WOOD)) return;
  if (limited_do_not_gather(res)) {
    Perm *centre = P(c->job);
    if (centre && centre->cls == CLS_WoodcuttingCentre) ST(BlueCollarState, centre)->nearest = NONE;
    return;
  }
  float w = 0.2f * (story.hiddenBoost && city.materials.m[M_WOOD] <= 6 ? 1.75f : 1) * early_fix() * boost();
  produce(M_WOOD, w);
  limited_take(res, w);
  gathered(c, res);
}
/* StoneMine's getResource */
static float stone_mined_per_action_boost(const Perm *mine) {
  float b = 1;
  if (mine && mine->cls == CLS_StoneMine) {
    if (has_upgrade_bit(mine, UB_StoneMine_BetterPickaxes)) b += 0.2f;
    if (has_upgrade_bit(mine, UB_StoneMine_AIMining)) b += 0.3f;
  }
  return b;
}
static void th_mine_stone(Cit *c, uint16_t arg) {
  Perm *res = P(arg_perm(arg));
  if (!gather_option(res, GATHER_STONE)) return;
  if (limited_do_not_gather(res)) {
    Perm *centre = P(c->job);
    if (centre && centre->cls == CLS_StoneMine) ST(BlueCollarState, centre)->nearest = NONE;
    return;
  }
  float s = 0.2f * stone_mined_per_action_boost(P(c->job));
  s = s * (story.hiddenBoost && city.materials.m[M_STONE] <= 4 ? 1.6f : 1) * upgrades.stoneMiningSpeed * early_fix() * boost();
  produce(M_STONE, s);
  limited_take(res, s);
  gathered(c, res);
}

bool finder_can_query(void);
typedef bool (*IsGoal)(const Perm *p, const void *ctx);
typedef float (*Niceness)(const Perm *p);
uint16_t finder_query(uint16_t from, int from_world, float from_x, IsGoal is_goal, const void *ctx, Niceness niceness,
                      float max_distance, const Cit *citizen, float best_expected);

/* doBlueCollarJob */
static void blue_collar_job(Perm *p, Cit *c, float timeMod, bool stop, int kind, int tmin, int tmax) {
  BlueCollarState *s = ST(BlueCollarState, p);
  bool tools = c->flags & CF_HAS_WORK_TOOLS;
  if (stop) {
    if (!tools) c->currentAction = 2;
    else if (c->inPermanent == pid(p)) cit_move_and_wait(c, 4, 20, TH_DROP_TOOLS, 0, false, false);
    else path_find(c, pid(p), THEN_NONE);
    return;
  }
  if (!tools) {
    if (c->inPermanent == pid(p)) cit_move_and_wait(c, 4, 20, TH_TAKE_TOOLS, 0, false, false);
    else path_find_combined(c, pid(p), THEN_NONE);
    return;
  }
  Perm *in = P(c->inPermanent);
  if (gather_option(in, kind) && (limited_materials_left(in) > 0 || limited_stay_if_empty(in)) && !limited_do_not_gather(in)) {
    if (limited_materials_left(in) > 0) {
      int x = rnd_between(limited_min_x(in), limited_max_x(in));
      cit_move_and_wait(c, x, rnd_between(tmin, tmax), kind == GATHER_WOOD ? TH_CUT_WOOD : TH_MINE_STONE, pid(in), true, false);
    }
    return;
  }
  bool perform = true;
  Perm *near = P(s->nearest);
  if (near && !gather_option(near, kind)) near = NULL, s->nearest = NONE;   /* (gone, and its place taken) */
  if (!near || (limited_materials_left(near) <= 0 && !limited_stay_if_empty(near)) || limited_do_not_gather(near)) {
    s->nearest = NONE;
    bool can = finder_can_query();
    if (!can) perform = false;
    uint8_t bit = kind == GATHER_WOOD ? WU_FOREST : WU_ROCK;
    if (can && !s->searched && !(world_unavailable[p->world] & bit)) {
      GatherGoal g = {kind, true};
      if (resource_any_prioritized(kind == GATHER_WOOD ? B_FOREST : B_ROCK))
        s->nearest = finder_query(pid(p), 0, 0, gather_goal, &g, NULL, -1, NULL, 1e8f);
      if (!s->nearest) {
        g.prioritized = false;
        s->nearest = finder_query(pid(p), 0, 0, gather_goal, &g, NULL, -1, NULL, 1e8f);
      }
      if (!s->nearest) world_unavailable[p->world] |= bit;
      s->searched = 1;
    }
  }
  if (perform) {
    if (s->nearest) path_find_combined(c, s->nearest, THEN_NONE);
    else cit_wander(c, timeMod);
  }
}
static void blue_collar_update(Perm *p, float timeMod) { (void)timeMod; ST(BlueCollarState, p)->searched = 0; }

static void woodcutting_work(Perm *p, Cit *c, float timeMod, bool stop) { blue_collar_job(p, c, timeMod, stop, GATHER_WOOD, 60, 90); }
static void stone_mine_work(Perm *p, Cit *c, float timeMod, bool stop) { blue_collar_job(p, c, timeMod, stop, GATHER_STONE, 90, 120); }

static void draw_plain(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  draw_upgrades(b, p, layer, x, y, true);
}

const PermOps ops_WoodcuttingCentre = {.work = woodcutting_work, .update = blue_collar_update, .draw = draw_plain};
const PermOps ops_StoneMine = {.work = stone_mine_work, .update = blue_collar_update, .draw = draw_plain};

/* ---------------------------------------------------------------- IndoorFarm */
static uint8_t TH_FARM_WORK;
static float indoor_farm_harvest_size(const Perm *p) {
  return 25 + bonuses.extraFoodFromFarms + adj_effect(p, EF_CROPS);
}
static void farm_grow(Perm *p, float amount) {
  IndoorFarmState *s = ST(IndoorFarmState, p);
  s->value += amount;
  if (s->value >= 100) {
    s->stage = 1;
    s->value = indoor_farm_harvest_size(p);
  }
}
static void indoor_farm_init(Perm *p) {
  IndoorFarmState *s = ST(IndoorFarmState, p);
  s->stage = 0;
  s->value = 20;
}
static void th_farm_work(Cit *c, uint16_t arg) {
  (void)c;
  Perm *p = P(arg_perm(arg));
  if (!p || p->cls != CLS_IndoorFarm) return;
  IndoorFarmState *s = ST(IndoorFarmState, p);
  switch (s->stage) {
  case 0: farm_grow(p, 1.35f * bonuses.indoorFarmSpeed); break;
  case 1: {
    float h = 0.25f * boost();
    s->value -= h;
    produce(M_FOOD, h);
    if (s->value <= 0) s->stage = 2, s->value = 0;
    break;
  }
  case 2:
    s->value += 5;
    if (s->value >= 100) s->stage = 0, s->value = 0;
    break;
  }
}
static void indoor_farm_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  IndoorFarmState *s = ST(IndoorFarmState, p);
  float np = happiness.actionSpeedModifierWithoutPenalties;
  int tmin = (int)((s->stage == 1 ? 15 : 30) / np), tmax = (int)((s->stage == 1 ? 30 : 60) / np);
  int x = rnd_between(3, 15);
  cit_move_and_wait(c, x, rnd_between(tmin, tmax), TH_FARM_WORK, pid(p), false, false);
}
static void indoor_farm_update(Perm *p, float timeMod) {
  IndoorFarmState *s = ST(IndoorFarmState, p);
  if (s->stage != 0) return;
  if (day_light()) farm_grow(p, timeMod * 0.03f * bonuses.indoorFarmSpeed);
  else if (dim_light()) farm_grow(p, timeMod * 0.015f * bonuses.indoorFarmSpeed);
}
static void indoor_farm_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer == LAYER_BG) {
    const IndoorFarmState *s = STC(IndoorFarmState, p);
    int n = frames_of(SPR_INDOORFARM_CROPS, 20);
    int f = s->stage == 0 ? (int)floorf(s->value / 100 * (n - 2)) : s->stage == 1 ? n - 2 : n - 1;
    if (f > n - 1) f = n - 1;
    if (f < 0) f = 0;
    dr_frame(b, SPR_INDOORFARM_CROPS, 20, f, x, y, 0, 0, 0, 255, GFX_NO_TINT);
  }
}
static void indoor_farm_window(Perm *p, WinOut *w) {
  const IndoorFarmState *s = STC(IndoorFarmState, p);
  if (s->stage == 0) win_line1(w, "Crops [0]% grown.", floorf(s->value));
  else if (s->stage == 1) win_line1(w, "[0] food left to harvest.", floorf(s->value));
  else win_line1(w, "Old crops [0]% cleaned.", floorf(s->value));
}
const PermOps ops_IndoorFarm = {.init = indoor_farm_init, .work = indoor_farm_work, .update = indoor_farm_update,
                                .draw = indoor_farm_draw, .window = indoor_farm_window};

/* ---------------------------------------------------------------- ExplorationCentre */
static uint8_t TH_EXPLORE;
static const int8_t standing_places[7] = {2, 3, 8, 9, 10, 15, 16};
static bool ruins_goal(const Perm *p, const void *ctx) { (void)ctx; return is_a(p, B_RUINS) && ruins_explored(p) < 100; }

static void th_explore(Cit *c, uint16_t arg) {
  Perm *r = P(arg_perm(arg));
  if (!r || !is_a(r, B_RUINS)) return;
  Perm *centre = P(c->job);
  ExplorationState *es = centre && centre->cls == CLS_ExplorationCentre ? ST(ExplorationState, centre) : NULL;
  if (ruins_explored(r) < 100) {
    ruins_explore(r, 0.25f);
    produce(M_KNOWLEDGE, 0.25f * early_fix() * boost());
    if (ruins_explored(r) >= 100 && c->inPermanent) {
      leave_to_surface(c);
      if (es) es->nearestRuins = NONE;
    }
  } else if (c->inPermanent) {
    leave_to_surface(c);
    if (es) es->nearestRuins = NONE;
  }
}
static void exploration_work(Perm *p, Cit *c, float timeMod, bool stop) {
  ExplorationState *s = ST(ExplorationState, p);
  if (stop) {
    c->currentAction = 2;
    c->flags &= (uint16_t)~CF_HAS_WORK_TOOLS;
    return;
  }
  if (!(c->flags & CF_HAS_WORK_TOOLS)) {
    if (c->inPermanent == pid(p)) cit_move_and_wait(c, 4, 30, TH_TAKE_TOOLS, 0, true, false);
    else path_find(c, pid(p), THEN_NONE);
    return;
  }
  Perm *in = P(c->inPermanent);
  if (in && is_a(in, B_RUINS)) {
    int x = standing_places[rnd_int(7)];
    cit_move_and_wait(c, x, rnd_between(90, 180), TH_EXPLORE, pid(in), true, false);
    return;
  }
  if (!s->nearestRuins && !s->searched && !(world_unavailable[p->world] & WU_RUINS)) {
    if (!finder_can_query()) return;
    s->nearestRuins = finder_query(pid(p), 0, 0, ruins_goal, NULL, NULL, -1, NULL, 1e8f);
    s->searched = 1;
    if (!s->nearestRuins) world_unavailable[p->world] |= WU_RUINS;
  }
  /* exploreOrInvestigate */
  if (s->nearestRuins) {
    path_find(c, s->nearestRuins, THEN_NONE);
    s->currentAction = 1;
    return;
  }
  s->currentAction = 2;
  if (c->inPermanent != pid(p)) {
    path_find(c, pid(p), THEN_NONE);
    return;
  }
  int idx = worker_index(c);
  if ((idx == 1 || idx == 2) && cit_is_at_ground_level(c)) {
    cit_change_floor(c, THEN_NONE, 0);
    return;
  }
  int xmin = idx == 2 ? 12 : 3, xmax = idx == 2 ? 16 : 7;
  if (c->relativeX >= xmin && c->relativeX < xmax) produce(M_KNOWLEDGE, 0.001f * timeMod * speed() * edu(c) * boost());
  else move_x(c, rnd_between(xmin, xmax));
}
static void exploration_update(Perm *p, float timeMod) { (void)timeMod; ST(ExplorationState, p)->searched = 0; }
static void exploration_window(Perm *p, WinOut *w) {
  static const char *const t[3] = {"Current task: looking for ruins.", "Current task: exploring ruins.",
                                   "Current task: investigating findings."};
  win_line(w, t[STC(ExplorationState, p)->currentAction % 3]);
}
const PermOps ops_ExplorationCentre = {.work = exploration_work, .update = exploration_update, .draw = draw_plain,
                                       .window = exploration_window};

/* ---------------------------------------------------------------- Factory and MaterialConvertingFactory */
enum { FX_SHOW_ACTIVE = 1 };
void factory_work(Perm *p, Cit *c, float timeMod, bool stop);
static void work_animation(Perm *p, Cit *c, float timeMod) {
  int idx = worker_index(c);
  if (c->relativeY == 10) return;
  switch (p->cls) {
  case CLS_MachinePartsFactory:
  case CLS_FarmByProductProcessor: set_rel(c, idx == 0 ? 12 : 15, 10); break;
  case CLS_RefinedMetalFactory:
    if (idx == 0) set_rel(c, 3, 10);
    else step_to_x(c, 3, timeMod), c->relativeY = 0;
    break;
  case CLS_ComputerChipFactory:
    if (idx == 1) set_rel(c, 3, 10);
    else if (idx == 0) step_to_x(c, 3, timeMod), c->relativeY = 0;
    else step_to_x(c, 13, timeMod), c->relativeY = 0;
    break;
  case CLS_ExperimentationLab:
    if (idx == 0) set_rel(c, 4, 10);
    else set_rel(c, 3, 0);
    break;
  case CLS_FactoryPub:
    if (idx == 0) set_rel(c, 8, 10);
    else set_rel(c, 3, 1);
    break;
  case CLS_ComputerResearchCenter: {
    static const int8_t pos[5][2] = {{13, 10}, {6, 0}, {6, 10}, {15, 0}, {8, 0}};
    if (idx < 5) set_rel(c, pos[idx][0], pos[idx][1]);
    break;
  }
  default: break;
  }
}
void factory_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop) {
    c->currentAction = 2;
    return;
  }
  work_animation(p, c, timeMod);
  FactoryState *s = ST(FactoryState, p);
  s->activeWorkers++;
  uint32_t e = s->totalEdu + (uint32_t)(edu(c) * 4096 + 0.5f);
  s->totalEdu = (uint16_t)(e > 65535 ? 65535 : e);
}
static inline float total_edu(const FactoryState *s) { return s->totalEdu / 4096.0f; }

/* a converting factory's numbers: from, to, materialsMadePerStepPerWorker, efficiency, bonus speed, enabled */
typedef struct { int from, to; float rate, efficiency, bonus; bool enabled, show_active_ok; } Converter;
static bool exp_lab_at(const Perm *q) { return q && q->cls == CLS_ExperimentationLab; }
static void converter(const Perm *p, Converter *k) {
  const FactoryState *s = STC(FactoryState, p);
  float avg_edu = s->activeWorkers == 0 ? 1 : total_edu(s) / s->activeWorkers;
  k->enabled = true;
  k->show_active_ok = true;
  k->bonus = 1;
  switch (p->cls) {
  case CLS_MachinePartsFactory: {
    bool turbo = p->mode == MODE_MachinePartsFactory_MachinePartsFactoryTurbo;
    k->from = M_STONE, k->to = M_MACHINEPARTS;
    k->rate = 0.0015f + (turbo ? 0.0015f : 0);
    k->efficiency = 3 - (has_upgrade_bit(p, UB_MachinePartsFactory_MachinePartsFactoryEfficiency) ? 1 : 0) + (turbo ? 2 : 0);
    k->bonus = bonuses.machinePartsFactorySpeed;
    k->enabled = p->mode != MODE_MachinePartsFactory_StoneUsingFactoryDisabled;
    break;
  }
  case CLS_RefinedMetalFactory:
  case CLS_FactoryPub: {
    /* RefinedMetalFactory.onCityChange: the norms, then each experimentation lab above or below */
    bool hack = p->mode == MODE_RefinedMetalFactory_RefinedMetalsFactoryHack;
    bool eff = has_upgrade_bit(p, UB_RefinedMetalFactory_RefinedMetalFactoryEfficiency);
    k->from = M_STONE, k->to = M_REFINEDMETAL;
    k->efficiency = 5.0f + (hack ? 1 : 0) - (eff ? 1 : 0);
    k->rate = 0.001f + (hack ? 0.0005f : 0) + (eff ? 0.00015f : 0);
    if (exp_lab_at(P(p->bottom))) k->efficiency -= 0.5f, k->rate += 0.0001f;
    if (exp_lab_at(P(p->top))) k->efficiency -= 0.5f, k->rate += 0.0001f;
    k->enabled = p->mode != MODE_RefinedMetalFactory_StoneUsingFactoryDisabled;
    break;
  }
  case CLS_ComputerChipFactory: {
    bool die = has_upgrade_bit(p, UB_ComputerChipFactory_DieShrink);
    k->from = M_REFINEDMETAL, k->to = M_COMPUTERCHIPS;
    k->rate = 0.0001f + (die ? 0.00005f : 0);
    k->efficiency = 7 - (die ? 2 : 0);
    k->bonus = avg_edu * upgrades.computerChipFactorySpeed;
    k->enabled = p->mode != MODE_ComputerChipFactory_RefinedStoneUsingFactoryDisabled;
    break;
  }
  case CLS_ExperimentationLab:
    k->from = M_MACHINEPARTS, k->to = M_KNOWLEDGE;
    k->rate = 0.0023f;
    k->efficiency = 0.2f - (has_upgrade_bit(p, UB_ExperimentationLab_UltimateScrewdriver) ? 0.058f : 0);
    k->bonus = avg_edu;
    k->enabled = p->mode != MODE_ExperimentationLab_LabDisabled;
    k->show_active_ok = false;
    break;
  default:
    k->from = k->to = -1;
    k->rate = k->efficiency = 0;
  }
}
/* MaterialConvertingFactory.possiblyBeActive */
static bool convert(Perm *p, float timeMod, const Converter *k) {
  FactoryState *s = ST(FactoryState, p);
  double *from = &city.materials.m[k->from];
  if (*from < 1) return false;
  float made = timeMod * k->rate * s->activeWorkers * speed() * k->bonus * boost();
  mat_add(k->to, made);
  s->materialMade += made;
  float used = made * k->efficiency;
  *from -= used;
  stats_add_used(k->from, used);
  s->totalMaterialUsed += used;
  return true;
}
/* Factory.update (MaterialConvertingFactory's possiblyBeActive, or the class's own) */
static void factory_update(Perm *p, float timeMod) {
  FactoryState *s = ST(FactoryState, p);
  bool active = false, show_ok = false;
  if (p->cls == CLS_FarmByProductProcessor) {
    ByProductState *bs = ST(ByProductState, p);
    if (bs->activeWorkers > 0) {
      /* cachedAdjacencyExpiration: the farms are counted again every 72 calls */
      int exp = bs->extra >> 1;
      if (exp > 0) exp--;
      float n = bs->cacheValue;
      if (exp == 0) {
        n = by_product_farms(p, true);
        bs->cacheValue = n;
        exp = 72;
      }
      bs->extra = (uint8_t)((bs->extra & 1) | exp << 1);
      float wood = 0.0014f * (bs->totalEdu / 4096.0f) * speed() * boost() * timeMod * n;
      produce(M_WOOD, wood);
      bs->woodMade += wood;
      active = wood > 0;
    }
    show_ok = true;
  } else if (p->cls == CLS_ComputerResearchCenter) {
    if (s->activeWorkers > 0) {
      float k = 0.0014f * timeMod * speed() * total_edu(s) * boost();
      produce(M_KNOWLEDGE, k);
      s->materialMade += k;   /* (its window's knowledge gathered) */
      active = true;
    }
    show_ok = true;
  } else {
    Converter k;
    converter(p, &k);
    if (s->activeWorkers > 0 && k.enabled) active = convert(p, timeMod, &k);
    show_ok = k.show_active_ok && city.materials.m[k.from] >= 3;
  }
  if (active && show_ok) s->extra |= FX_SHOW_ACTIVE;
  else s->extra &= (uint8_t)~FX_SHOW_ACTIVE;
  s->activeWorkers = 0;
  s->totalEdu = 0;
}
/* the back animation: the factory's active or idle frames, 4 ticks each (Factory.timePerFrame) */
static void factory_back(const GfxBand *b, const Perm *p, int x, int y, uint16_t active, uint16_t idle) {
  bool on = STC(FactoryState, p)->extra & FX_SHOW_ACTIVE;
  uint16_t spr = on ? active : idle;
  int n = frames_of(spr, 20);
  if (n <= 0) return;
  int f = (int)((bld_anim + (hash32(pid(p)) & 255)) / 4) % n;
  dr_frame(b, spr, 20, f, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
int factory_frame(const Perm *p, int nframes) {   /* (MechanicalHouse shows its factory's phase) */
  return nframes > 0 ? (int)((bld_anim + (hash32(pid(p)) & 255)) / 4) % nframes : 0;
}
static void factory_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  if (layer == LAYER_BG) {
    switch (p->cls) {
    case CLS_MachinePartsFactory: factory_back(b, p, x, y, SPR_MACHINEPARTSFACTORY_FRAMES, SPR_MACHINEPARTSFACTORY_IDLE); break;
    case CLS_RefinedMetalFactory: factory_back(b, p, x, y, SPR_REFINEDMETALFACTORY_FRAMES, SPR_REFINEDMETALFACTORY_IDLE); break;
    case CLS_ComputerChipFactory: factory_back(b, p, x, y, SPR_COMPUTERCHIPFACTORY_FRAMES, SPR_COMPUTERCHIPFACTORY_IDLE); break;
    case CLS_ExperimentationLab: factory_back(b, p, x, y, SPR_EXPERIMENTATIONLAB_IDLE, SPR_EXPERIMENTATIONLAB_IDLE); break;
    case CLS_FarmByProductProcessor:
      factory_back(b, p, x, y, SPR_FARMBYPRODUCTPROCESSOR_FRAMES, SPR_FARMBYPRODUCTPROCESSOR_IDLE);
      break;
    default: break;
    }
  }
  if (layer == LAYER_ABOVE && p->cls == CLS_ExperimentationLab) {
    /* pipes to refined metal factories above and below (not factory pubs) */
    const Perm *bo = P(p->bottom), *t = P(p->top);
    if (bo && bo->cls == CLS_RefinedMetalFactory) dr_sprite(b, SPR_EXPERIMENTATIONLAB_PIPE_BOTTOM, x, y, 7, 17, 0, 255);
    if (t && t->cls == CLS_RefinedMetalFactory) dr_sprite(b, SPR_EXPERIMENTATIONLAB_PIPE_TOP, x, y, 10, -11, 0, 255);
  }
  draw_upgrades(b, p, layer, x, y, true);
}
static void converter_window(Perm *p, WinOut *w) {
  const FactoryState *s = STC(FactoryState, p);
  char b[160];
  switch (p->cls) {
  case CLS_MachinePartsFactory:
    lo2(b, sizeof b, "[0] stone used to make [1] machine parts.", (int)s->totalMaterialUsed, (int)s->materialMade);
    win_line(w, b);
    break;
  case CLS_RefinedMetalFactory:
  case CLS_FactoryPub:
    win_line2(w, "[0] stone used to make [1] units of refined metals.", (int)s->totalMaterialUsed, (int)s->materialMade);
    break;
  case CLS_ComputerChipFactory:
    win_line2(w, "[0] units of refined metals used to make [1] computer chips.", (int)s->totalMaterialUsed, (int)s->materialMade);
    break;
  case CLS_ExperimentationLab:
    win_line2(w, "[0] machine parts used to gain [1] knowledge.", (int)s->totalMaterialUsed, (int)s->materialMade);
    break;
  default: break;
  }
}
void factory_window_lines(Perm *p, WinOut *w) { converter_window(p, w); }

static void mpf_on_build(Perm *p) { (void)p; unlocks_unlock("MechanicalHouse"); }
static void rmf_on_build(Perm *p) {
  (void)p;
  unlocks_unlock("HyperElevator");
  unlocks_unlock("ComputerResearchCenter");
  unlocks_fully_unlock("MedicalClinic");
  unlocks_fully_unlock("ModernArtMuseum");
  unlocks_unlock("SpaciousHouse");
  unlocks_unlock("StoneResearchCenter");
}
static void ccf_on_build(Perm *p) {
  (void)p;
  unlocks_unlock("AIMining");
  unlocks_unlock("HeatedBed");
  unlocks_unlock("Supercomputer");
  unlocks_unlock("LivingResearchCenter");
  unlocks_unlock("Teleporter");
  unlocks_unlock("CuttingEdgeHome");
  unlocks_fully_unlock("Arcade");
  unlocks_unlock("ChipBinning");
  unlocks_unlock("PickaxeTech");
  unlocks_unlock("FossilScanner");
}
static void factory_init(Perm *p) { upgrade_first_mode(p); }
const PermOps ops_MachinePartsFactory = {.init = factory_init, .on_build = mpf_on_build, .work = factory_work,
                                         .update = factory_update, .draw = factory_draw, .window = converter_window};
const PermOps ops_RefinedMetalFactory = {.init = factory_init, .on_build = rmf_on_build, .work = factory_work,
                                         .update = factory_update, .draw = factory_draw, .window = converter_window};
const PermOps ops_ComputerChipFactory = {.init = factory_init, .on_build = ccf_on_build, .work = factory_work,
                                         .update = factory_update, .draw = factory_draw, .window = converter_window};
const PermOps ops_ExperimentationLab = {.init = factory_init, .work = factory_work, .update = factory_update,
                                        .draw = factory_draw, .window = converter_window};
/* (ComputerResearchCenter and FactoryPub use these from their files) */
void factory_update_any(Perm *p, float timeMod) { factory_update(p, timeMod); }
void factory_draw_any(const GfxBand *b, const Perm *p, int layer, int x, int y) { factory_draw(b, p, layer, x, y); }
void factory_back_any(const GfxBand *b, const Perm *p, int x, int y, uint16_t active, uint16_t idle) {
  factory_back(b, p, x, y, active, idle);
}

/* ---------------------------------------------------------------- FarmByProductProcessor */
static bool is_bpp(const Perm *q) { return q && q->cls == CLS_FarmByProductProcessor; }
static float fill_ratio(const Perm *q, bool true_value) {
  if (bld_effect(q, EF_FARM) <= 0.5f) return 0;
  if (!true_value) return 1;
  if (is_a(q, B_WORK)) {
    int j = bld_jobs(q);
    return j ? (float)q->nworkers / j : 0;
  }
  int r = bld_residents(q);
  return r ? (float)q->nresidents / r : 0;
}
/* adjacentFarmNumber: each farm counted by one processor only */
float by_product_farms(const Perm *p, bool true_value) {
  float total = 0;
  const Perm *l = P(p->left), *r = P(p->right), *t = P(p->top), *bo = P(p->bottom);
  if (l) total += fill_ratio(l, true_value);
  if (r && !is_bpp(P(r->right))) total += fill_ratio(r, true_value);
  if (t && !is_bpp(P(t->left)) && !is_bpp(P(t->right))) total += fill_ratio(t, true_value);
  if (bo && !is_bpp(P(bo->left)) && !is_bpp(P(bo->right)) && !is_bpp(P(bo->bottom))) total += fill_ratio(bo, true_value);
  return total;
}
static void by_product_window(Perm *p, WinOut *w) {
  int n = (int)by_product_farms(p, false);
  if (n == 1) win_line(w, "Processing by-products for one farm.");
  else win_line1(w, "Processing by-products for [0] farms.", n);
  win_line1(w, "[0] wood produced.", (int)ST(ByProductState, p)->woodMade);
}
const PermOps ops_FarmByProductProcessor = {.work = factory_work, .update = factory_update, .draw = factory_draw,
                                            .window = by_product_window};

/* ---------------------------------------------------------------- Laboratory */
enum { LAB_WORKED = 1, LAB_LIGHTS = 2 };
static void lab_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop) {
    c->currentAction = 2;
    return;
  }
  LabState *s = ST(LabState, p);
  s->anyoneHasWorkedHere |= LAB_WORKED;
  if (cit_is_at_ground_level(c)) {
    set_rel(c, worker_index(c) == 0 ? 5 : 13, 9);
  } else {
    float k = 0.00125f * timeMod * speed() * bonuses.labSpeed * edu(c) * boost();
    produce(M_KNOWLEDGE, k);
    s->totalKnowledgeGenerated += k;
  }
}
static void lab_update(Perm *p, float timeMod) {
  (void)timeMod;
  LabState *s = ST(LabState, p);
  s->anyoneHasWorkedHere = (s->anyoneHasWorkedHere & LAB_WORKED) ? LAB_LIGHTS : 0;
}
static void lab_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  if (layer == LAYER_BG) {
    if (STC(LabState, p)->anyoneHasWorkedHere & LAB_LIGHTS) dr_frame(b, SPR_LABORATORY, 20, 2, x, y, 0, 0, 0, 255, GFX_NO_TINT);
    else dr_sprite(b, SPR_LABORATORY_LIGHTSOFF, x, y, 0, 0, 0, 255);
  }
  draw_upgrades(b, p, layer, x, y, true);
}
static void lab_window(Perm *p, WinOut *w) {
  win_line1(w, "[0] knowledge gathered.", (int)STC(LabState, p)->totalKnowledgeGenerated);
  /* gui.GlobalUpgradeWindow.addSmartUpgradeForLabs: the user interface shows it with upgrade_smart_labs_* */
}
const PermOps ops_Laboratory = {.work = lab_work, .update = lab_update, .draw = lab_draw, .window = lab_window};

/* ---------------------------------------------------------------- TreePlantation */
static uint8_t TH_PLANTATION;
static float extra_tree_growth(const Perm *p) { return 1 + adj_effect(p, EF_TREE_GROWTH); }
float plantation_regrow_speed(const Perm *p) {
  float boost_ = 1 + (has_upgrade_bit(p, UB_TreePlantation_AutomaticWaterManagement) ? 2 : 0);
  return 0.008f * extra_tree_growth(p) * boost_;
}
static void plantation_update(Perm *p, float timeMod) {
  PlantationState *s = ST(PlantationState, p);
  if (s->materialsLeft > 0) return;
  if (day_light()) s->regrowProgress += plantation_regrow_speed(p) * timeMod;
  else if (dim_light()) s->regrowProgress += plantation_regrow_speed(p) * timeMod * 0.5f;
  if (s->regrowProgress >= 100) s->materialsLeft = 100, s->regrowProgress = 0;
}
static void th_plantation(Cit *c, uint16_t arg) {
  (void)c;
  Perm *p = P(arg_perm(arg));
  if (!p || p->cls != CLS_TreePlantation) return;
  PlantationState *s = ST(PlantationState, p);
  if (arg & 0x8000) {
    float w = boost() * 0.2f;
    s->materialsLeft -= w;
    produce(M_WOOD, w);
  } else {
    s->regrowProgress += 0.5f * extra_tree_growth(p);
  }
}
static void plantation_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  bool any = ST(PlantationState, p)->materialsLeft > 0;
  int x = rnd_between(4, 14);
  int t = rnd_between(any ? 60 : 120, any ? 90 : 180);
  cit_move_and_wait(c, x, t, TH_PLANTATION, (uint16_t)(pid(p) | (any ? 0x8000 : 0)), true, false);
}
/* LimitedWorldResource.updateTextureOfLimitedResourceSprite's frame */
int limited_frame(float left, float initial, float regrow, int nframes, int nregrow) {
  if (left <= 0.0000001f) return nregrow > 0 ? (int)(regrow / 100 * nregrow) : nframes - 1;
  int i = (int)floorf((initial - left) / initial * nframes);
  if (initial != left && i == 0 && nframes > 1) i = 1;
  return i;
}
static void plantation_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  if (layer == LAYER_BG) {
    const PlantationState *s = STC(PlantationState, p);
    int n = frames_of(SPR_TREEPLANTATION_TREES, 20), ng = frames_of(SPR_TREEPLANTATION_TREES_GROW, 20);
    int f = limited_frame(s->materialsLeft, 100, s->regrowProgress, n, ng);
    if (s->materialsLeft <= 0.0000001f) dr_frame(b, SPR_TREEPLANTATION_TREES_GROW, 20, f < ng ? f : ng - 1, x, y, 0, 0, 0, 255, GFX_NO_TINT);
    else dr_frame(b, SPR_TREEPLANTATION_TREES, 20, f < n ? f : n - 1, x, y, 0, 0, 0, 255, GFX_NO_TINT);
  }
  draw_upgrades(b, p, layer, x, y, true);
}
static void plantation_window(Perm *p, WinOut *w) {
  const PlantationState *s = STC(PlantationState, p);
  if (s->materialsLeft <= 0) win_line1(w, "Trees [0]% grown.", (int)s->regrowProgress);
  else win_line2(w, "[0]/[1] wood left.", (int)s->materialsLeft, 100);
}
const PermOps ops_TreePlantation = {.work = plantation_work, .update = plantation_update, .draw = plantation_draw,
                                    .window = plantation_window};

/* ---------------------------------------------------------------- StoneTeleporter */
static uint8_t TH_TELEPORTER_MINE;
float stone_teleporter_initial(const Perm *p) { return STC(PlantationState, p)->flags & TP_INITIAL_BIG ? 150 : 50; }
static void th_teleporter_mine(Cit *c, uint16_t arg) {
  (void)c;
  Perm *p = P(arg_perm(arg));
  if (!p || p->cls != CLS_StoneTeleporter) return;
  PlantationState *s = ST(PlantationState, p);
  if (s->materialsLeft > 0) {
    float st = boost() * 0.2f * upgrades.stoneMiningSpeed;
    s->materialsLeft -= st;
    produce(M_STONE, st);
  }
}
static bool has_remote(const Perm *p) { return has_upgrade_bit(p, UB_StoneTeleporter_StoneTeleporterRemote); }
static void stone_teleporter_work(Perm *p, Cit *c, float timeMod, bool stop) {
  if (stop) {
    c->currentAction = 2;
    return;
  }
  PlantationState *s = ST(PlantationState, p);
  if (!has_remote(p)) step_to_x(c, 3, timeMod);
  if (s->materialsLeft <= 0) {
    s->flags |= TP_PRESSED;
    s->notPressedTime = 0;
  } else if (has_remote(p)) {
    int x = rnd_between(5, 13);
    cit_move_and_wait(c, x, rnd_between(90, 120), TH_TELEPORTER_MINE, pid(p), true, false);
  }
}
static void stone_teleporter_update(Perm *p, float timeMod) {
  PlantationState *s = ST(PlantationState, p);
  if (s->materialsLeft > 0) return;
  if (s->flags & TP_PRESSED) {
    if (s->regrowProgress == 0) {
      if (upgrades.stoneTeleporterHasBigStones) s->flags |= TP_BIG;
      else s->flags &= (uint8_t)~TP_BIG;
    }
    s->regrowProgress += 2 * timeMod;
    if (s->regrowProgress >= 100) {
      s->regrowProgress = 0;
      if (s->flags & TP_BIG) s->flags |= TP_INITIAL_BIG;
      float initial = stone_teleporter_initial(p);
      s->materialsLeft = initial;
      s->flags &= (uint8_t)~TP_PRESSED;
      s->notPressedTime = 0;
      if (upgrades.stoneResearchCenterWithFossils && rnd_float() < 0.00001f * initial) {
        produce(M_KNOWLEDGE, 10000);
        bonuses.fossilsCollected++;
      }
    }
  } else if (has_remote(p) && p->nworkers > 0) {
    s->flags |= TP_PRESSED;
    s->notPressedTime = 0;
  } else {
    s->notPressedTime = tick_up(s->notPressedTime, timeMod);
  }
}
static void stone_teleporter_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  if (layer == LAYER_BG) {
    const PlantationState *s = STC(PlantationState, p);
    uint16_t spr = (s->flags & TP_BIG) ? SPR_STONETELEPORTER_STONE_BIG : SPR_STONETELEPORTER_STONE;
    int n = frames_of(spr, 20);
    if (s->materialsLeft <= 0) {
      float a = s->regrowProgress / 100;
      dr_frame(b, spr, 20, 0, x, y, 0, 0, 0, (uint8_t)(a < 0 ? 0 : a > 1 ? 255 : a * 255), GFX_NO_TINT);
    } else {
      int f = limited_frame(s->materialsLeft, stone_teleporter_initial(p), 0, n, 0);
      dr_frame(b, spr, 20, f < n ? f : n - 1, x, y, 0, 0, 0, 255, GFX_NO_TINT);
    }
  }
  draw_upgrades(b, p, layer, x, y, true);
}
static void stone_teleporter_window(Perm *p, WinOut *w) {
  const PlantationState *s = STC(PlantationState, p);
  bool pressed = s->flags & TP_PRESSED;
  if (s->materialsLeft > 0 || (!pressed && s->notPressedTime < 3)) win_line1(w, "[0] stone left in this meteoroid.", (int)s->materialsLeft);
  else if (!pressed) win_line(w, "Waiting for a worker to press the teleport button...");
  else win_line(w, "Teleporting a meteoroid...");
}
const PermOps ops_StoneTeleporter = {.work = stone_teleporter_work, .update = stone_teleporter_update,
                                     .draw = stone_teleporter_draw, .window = stone_teleporter_window};

/* ---------------------------------------------------------------- FarmGrowArea (EcoFarm, ExperimentalFarm) */
static uint8_t TH_PLOT_WORK;
typedef struct { int8_t x, y; uint8_t width; float food, mult, clean; uint8_t harvestPart; } PlotDef;
static const uint16_t eco_crop2[3] = {SPR_ECOFARM_CROP2, SPR_ECOFARM_CROP2_ALT, SPR_ECOFARM_CROP2_ALT2};
static const uint16_t eco_crop3[2] = {SPR_ECOFARM_CROP3, SPR_ECOFARM_CROP3_ALT};
static bool eco(const Perm *p) { return p->cls == CLS_EcoFarm; }
static void plot_def(const Perm *p, int i, PlotDef *d, uint16_t *spr) {
  const GrowFarmState *s = STC(GrowFarmState, p);
  if (eco(p)) {
    static const PlotDef e[3] = {{3, 3, 6, 20, 0.7f, 0, 6}, {9, 3, 8, 10, 1.4f, 10, 1}, {9, 12, 8, 12, 0.6f, 7, 1}};
    *d = e[i];
    /* EcoFarmUpgrade: plot 1 gets 4 more (new farms too), plot 2 three more only on farms that existed then */
    if (i == 0 && city_upgrade_bought(U_EcoFarmUpgrade)) d->food += 4;
    if (i == 1) d->food += upgrades.extraEcoFarmFoodToPlot2 + ((s->eco >> 6 & 1) ? 3 : 0);
    *spr = i == 0 ? SPR_ECOFARM_CROP1 : i == 1 ? eco_crop2[(s->eco >> 3 & 3) % 3] : eco_crop3[s->eco >> 5 & 1];
  } else {
    static const PlotDef x[3] = {{3, 3, 5, 12, 1, 5, 1}, {12, 3, 5, 10, 1.2f, 10, 1}, {12, 12, 5, 6, 1.5f, 10, 1}};
    static const uint16_t sp[3] = {SPR_EXPERIMENTALFARM_CROP1, SPR_EXPERIMENTALFARM_CROP2, SPR_EXPERIMENTALFARM_CROP3};
    *d = x[i];
    *spr = sp[i];
  }
}
/* 24-bit fixed point values (2^-17) */
static float plot_value(const Perm *p, int i) {
  const uint8_t *v = STC(GrowFarmState, p)->value[i];
  return (float)((uint32_t)v[0] | (uint32_t)v[1] << 8 | (uint32_t)v[2] << 16) / 131072.0f;
}
static void plot_set_value(Perm *p, int i, float f) {
  if (f < 0) f = 0;
  if (f > 127.99f) f = 127.99f;
  uint32_t u = (uint32_t)(f * 131072.0f + 0.5f);
  uint8_t *v = ST(GrowFarmState, p)->value[i];
  v[0] = (uint8_t)u, v[1] = (uint8_t)(u >> 8), v[2] = (uint8_t)(u >> 16);
}
int plot_stage(const Perm *p, int i) { return STC(GrowFarmState, p)->stages >> (2 * i) & 3; }
static void plot_set_stage(Perm *p, int i, int st) {
  GrowFarmState *s = ST(GrowFarmState, p);
  s->stages = (uint8_t)((s->stages & ~(3 << (2 * i))) | st << (2 * i));
}
static float plot_harvest_size(const Perm *p, int i) {
  PlotDef d;
  uint16_t spr;
  plot_def(p, i, &d, &spr);
  return d.food + bonuses.extraFoodFromFarms + adj_effect(p, EF_CROPS);
}
static void plot_grow(Perm *p, int i, float amount) {
  float v = plot_value(p, i) + amount;
  if (v >= 100) {
    plot_set_stage(p, i, 1);
    v = plot_harvest_size(p, i);
  }
  plot_set_value(p, i, v);
}
static void plots_update(Perm *p, float timeMod) {
  for (int i = 0; i < 3; i++) {
    if (plot_stage(p, i) != 0) continue;
    PlotDef d;
    uint16_t spr;
    plot_def(p, i, &d, &spr);
    if (day_light()) plot_grow(p, i, timeMod * (0.012f * d.mult));
    else if (dim_light()) plot_grow(p, i, timeMod * (0.006f * d.mult));
  }
}
/* onCitizenWork */
static void th_plot_work(Cit *c, uint16_t arg) {
  (void)c;
  Perm *p = P(arg_perm(arg));
  int i = arg >> 12 & 3;
  if (!p || (p->cls != CLS_EcoFarm && p->cls != CLS_ExperimentalFarm) || i > 2) return;
  PlotDef d;
  uint16_t spr;
  plot_def(p, i, &d, &spr);
  switch (plot_stage(p, i)) {
  case 0: plot_grow(p, i, 0.85f * d.mult); break;
  case 1: {
    float h = 0.25f * boost();
    float left = plot_value(p, i) - h;
    produce(M_FOOD, h);
    if (left <= 0) {
      left = 0;
      plot_set_stage(p, i, d.clean <= 0 ? 0 : 2);
    }
    plot_set_value(p, i, left);
    break;
  }
  case 2: {
    float v = plot_value(p, i) + d.clean;
    if (v >= 100) plot_set_stage(p, i, 0), v = 0;
    plot_set_value(p, i, v);
    break;
  }
  }
}
static void plot_work(Perm *p, Cit *c, int i) {
  PlotDef d;
  uint16_t spr;
  plot_def(p, i, &d, &spr);
  float np = happiness.actionSpeedModifierWithoutPenalties;
  int st = plot_stage(p, i);
  int tmin = (int)((st == 1 ? 15 : 30) / np), tmax = (int)((st == 1 ? 30 : 60) / np);
  int x = rnd_between(d.x, d.x + d.width - 2 + 1);
  cit_move_and_wait(c, x, rnd_between(tmin, tmax), TH_PLOT_WORK, (uint16_t)(pid(p) | i << 12), false, false);
}
static void plot_work_no_move(Perm *p, Cit *c, int i) {
  int st = plot_stage(p, i);
  cit_wait(c, rnd_between(st == 1 ? 15 : 30, st == 1 ? 30 : 60), TH_PLOT_WORK, (uint16_t)(pid(p) | i << 12));
}
static int plot_frame(const Perm *p, int i, int n) {
  PlotDef d;
  uint16_t spr;
  plot_def(p, i, &d, &spr);
  float v = plot_value(p, i);
  switch (plot_stage(p, i)) {
  case 0: return (int)floorf(v / 100 * (n - 1 - d.harvestPart));
  case 1: {
    float orig = plot_harvest_size(p, i);   /* originalFoodLeft (as it is now) */
    int val = n - 1 - (int)ceilf(v / orig * 0.9999f * d.harvestPart);
    return val < 1 ? 1 : val > n - 2 ? n - 2 : val;
  }
  default: return n - 1;
  }
}
static void plots_draw(const GfxBand *b, const Perm *p, int x, int y) {
  for (int i = 0; i < 3; i++) {
    PlotDef d;
    uint16_t spr;
    plot_def(p, i, &d, &spr);
    int n = frames_of(spr, d.width);
    int f = plot_frame(p, i, n);
    if (f < 0) f = 0;
    if (f >= n) f = n - 1;
    dr_frame(b, spr, d.width, f, x, y, d.x, d.y, 0, 255, GFX_NO_TINT);
  }
}
static void plots_window(Perm *p, WinOut *w) {
  static const char *const name[3] = {"Left plot: ", "Top right plot: ", "Bottom right plot: "};
  for (int i = 0; i < 3; i++) {
    char t[96], b[160];
    float v = plot_value(p, i);
    int st = plot_stage(p, i);
    lo1(t, sizeof t, st == 0 ? "Crops [0]% grown." : st == 1 ? "[0] food left to harvest." : "Old crops [0]% cleaned.", floorf(v));
    int k = 0;
    for (const char *s = name[i]; *s && k < 100; s++) b[k++] = *s;
    for (const char *s = t; *s && k < 158; s++) b[k++] = *s;
    b[k] = 0;
    win_line(w, b);
  }
}

/* EcoFarm */
static int eco_open_day(const Perm *p) { return STC(GrowFarmState, p)->eco & 7; }
static void eco_init(Perm *p) {
  GrowFarmState *s = ST(GrowFarmState, p);
  int c2 = rnd_int(3), c3 = rnd_int(2);
  s->eco = (uint8_t)(c2 << 3 | c3 << 5);
  s->eco |= (uint8_t)rnd_int(7);
}
static void eco_on_build(Perm *p) { (void)p; unlocks_unlock("EcoFarmUpgrade"); }
static float eco_end_time(const Perm *p) { return day_number() % 7 == eco_open_day(p) ? 21 : 19; }
bool first_worker_working(const Perm *p);
static bool eco_is_open(const Perm *p) {
  float h = hour_now();
  return p->nworkers > 0 && first_worker_working(p) && day_number() % 7 == eco_open_day(p) && h < 20.5f && h > 12;
}
static float eco_capacity(const Perm *p) { return p->nworkers * 5; }
static void eco_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  int x = rnd_between(3, 16);
  cit_move_and_wait(c, x, rnd_between(50, 100), THEN_NONE, 0, false, true);
}
static void grow_farm_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  int idx = worker_index(c);
  if (idx > 2) idx = 2;
  if (idx == 1) {
    if (c->relativeY < 5) cit_move(c, 18, TH_CHANGE_FLOOR, 0, false);
    else plot_work(p, c, 1);
  } else if (eco(p) && idx == 0 && plot_stage(p, 0) == 1) {
    if (c->relativeY <= 1 || c->relativeX < 4 || c->relativeX > 6) {
      static const int16_t s[] = {4, 5, 9, 8};
      cit_set_path(c, s, 2);
    } else {
      plot_work_no_move(p, c, 0);
    }
  } else if (c->relativeY > 5) {
    cit_change_floor(c, THEN_NONE, 0);
  } else {
    plot_work(p, c, idx);
  }
}
static void grow_farm_update(Perm *p, float timeMod) { plots_update(p, timeMod); }
static void grow_farm_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  if (layer == LAYER_BG) plots_draw(b, p, x, y);
}
const PermOps ops_EcoFarm = {.init = eco_init, .on_build = eco_on_build, .work = grow_farm_work, .update = grow_farm_update,
                             .end_time = eco_end_time, .is_open = eco_is_open, .entertainment_capacity = eco_capacity,
                             .be_entertained = eco_be_entertained, .draw = grow_farm_draw, .window = plots_window};
/* ExperimentalFarm (a unique building, but its plots are these) */
static void exp_farm_on_build(Perm *p) { (void)p; bonuses.indoorFarmSpeed += 0.1f; }
static void exp_farm_destroy(Perm *p) { (void)p; bonuses.indoorFarmSpeed -= 0.1f; }
const PermOps ops_ExperimentalFarm = {.on_build = exp_farm_on_build, .destroy = exp_farm_destroy, .work = grow_farm_work,
                                      .update = grow_farm_update, .draw = grow_farm_draw, .window = plots_window};
/* EcoFarmUpgrade's addToCity: every eco farm built so far gets 3 more food on its second plot */
void eco_farms_upgrade(void) {
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == CLS_EcoFarm) ST(GrowFarmState, &perms[i])->eco |= 1 << 6;
}

void bld_production_init(void) {
  TH_DROP_TOOLS = then_register(th_drop_tools);
  TH_TAKE_TOOLS = then_register(th_take_tools);
  TH_CUT_WOOD = then_register(th_cut_wood);
  TH_MINE_STONE = then_register(th_mine_stone);
  TH_FARM_WORK = then_register(th_farm_work);
  TH_EXPLORE = then_register(th_explore);
  TH_PLANTATION = then_register(th_plantation);
  TH_TELEPORTER_MINE = then_register(th_teleporter_mine);
  TH_PLOT_WORK = then_register(th_plot_work);
}
