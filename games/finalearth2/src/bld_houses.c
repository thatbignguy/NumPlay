/* The Final Earth 2: houses (buildings.md 6): what residents do at home
 * (walkAround), the homes that change with their neighbours (modern rustic
 * homes, mechanical houses, mansions), community houses' production, private
 * teleporters, and the looks the player can change. */
#include "bldi.h"
#include "lang_data.h"

int rnd_getint(int a, int b) { return a + (int)floorf(rnd_float() * (float)(b - a)); }

/* the house's resident waits (setPath([8, getInt(90, 120)]) in its building) */
static void idle(Cit *c) { wait_ticks(c, 90, 120); }
static void walk_wait(Cit *c, int xmin, int xmax, int tmin, int tmax) {
  int x = rnd_between(xmin, xmax);
  cit_move_and_wait(c, x, rnd_between(tmin, tmax), THEN_NONE, 0, false, false);
}

/* ---------------------------------------------------------------- the frames of upgrades' sprites */
static uint8_t *look_of(Perm *p, int upgrade) {
  switch (upgrade) {
  case U_Flowers: return p->cls == CLS_NormalHouse ? &ST(NormalHouseState, p)->flowersLook : NULL;
  case U_LivingComputer: return p->cls == CLS_AlienHouse ? &ST(AlienHouseState, p)->look : NULL;
  case U_ModernHomePlant: return p->cls == CLS_CuttingEdgeHome ? &ST(CuttingEdgeState, p)->plantLook : NULL;
  case U_BetterTools: return p->cls == CLS_TinkerersHome ? &ST(TinkerersState, p)->toolsLook : NULL;
  default: return NULL;
  }
}
int upgrade_look(const Perm *p, int upgrade) {
  const uint8_t *l = look_of((Perm *)p, upgrade);
  return l ? *l : 0;
}
void upgrade_set_look(Perm *p, int upgrade, int look) {
  uint8_t *l = look_of(p, upgrade);
  if (l) *l = (uint8_t)look;
}

/* ---------------------------------------------------------------- NormalHouse */
static void normal_house_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  /* the coloured door, on the ground floor */
  if (layer == LAYER_MAIN && p->row == 0)
    dr_frame(b, SPR_COLOREDDOORS, 20, STC(NormalHouseState, p)->doorMainTexture % 6, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
/* createWindowAddBottomButtons: the door's colour (a ground floor's), after the upgrades */
static void normal_house_window(Perm *p, WinOut *w) {
  win_later(w);
  if (p->row == 0) win_button(w, 0, L_change_door_color, WA_CYCLE_0, true);
}
static void normal_house_action(Perm *p, int a) {
  if (a == WA_CYCLE_0 && p->row == 0) {
    NormalHouseState *s = ST(NormalHouseState, p);
    s->doorMainTexture = (uint8_t)((s->doorMainTexture + 1) % 6);
  }
}
const PermOps ops_NormalHouse = {.draw = normal_house_draw, .window = normal_house_window, .window_action = normal_house_action};

/* ---------------------------------------------------------------- simple homes */
static void house_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  draw_upgrades(b, p, layer, x, y, true);
}
const PermOps ops_AlienHouse = {.draw = house_draw};
const PermOps ops_ComfortableHouse = {.draw = house_draw};

/* ModernRusticHome: +15 with three neighbours of community feeling */
static void rustic_city_change(Perm *p) {
  RusticState *s = ST(RusticState, p);
  int now = adj_effect(p, EF_COMMUNITY) >= 3 ? 15 : 0;
  p->bonusAttractiveness = (int8_t)(p->bonusAttractiveness + now - s->adjBonus);
  if (now != s->adjBonus) sim_flags |= SIM_HOUSES_CHANGED;
  s->adjBonus = (uint8_t)now;
}
const PermOps ops_ModernRusticHome = {.on_city_change = rustic_city_change, .draw = house_draw};

/* ---------------------------------------------------------------- MechanicalHouse */
enum { MH_NONE, MH_LEFTRIGHT, MH_LEFT, MH_TOP, MH_RIGHT, MH_BOTTOM };
static bool is_mpf(const Perm *q) { return q && class_is(q->cls, CLS_MachinePartsFactory); }
uint16_t mechanical_house_texture(const Perm *p) {
  static const uint16_t t[6] = {SPR_MECHANICALHOUSE, SPR_MECHANICALHOUSE_LEFTRIGHT, SPR_MECHANICALHOUSE_LEFT,
                                SPR_MECHANICALHOUSE_TOP, SPR_MECHANICALHOUSE_RIGHT, SPR_MECHANICALHOUSE_BOTTOM};
  return t[STC(MechanicalState, p)->variant % 6];
}
static void mechanical_city_change(Perm *p) {
  MechanicalState *s = ST(MechanicalState, p);
  int old_bonus = s->variant != MH_NONE ? 50 : 0;
  const Perm *l = P(p->left), *r = P(p->right), *t = P(p->top), *bo = P(p->bottom);
  s->movementType = 0;
  s->pipeAlt = 0;
  if (is_mpf(r)) {
    if (is_mpf(l)) s->variant = MH_LEFTRIGHT, s->pipeAlt = l->bottom == NONE;
    else s->variant = MH_LEFT;
    s->movementType = 1;
  } else if (is_mpf(bo)) {
    s->variant = MH_TOP;
    s->pipeAlt = is_mpf(t);
    s->movementType = 2;
  } else if (is_mpf(l)) {
    s->variant = MH_RIGHT;
    s->pipeAlt = l->bottom == NONE;
  } else if (is_mpf(t)) {
    s->variant = MH_BOTTOM;
    s->movementType = 3;
  } else {
    s->variant = MH_NONE;
  }
  int now = s->variant != MH_NONE ? 50 : 0;
  p->bonusAttractiveness = (int8_t)(p->bonusAttractiveness + now - old_bonus);
  if (now != old_bonus) sim_flags |= SIM_HOUSES_CHANGED;
}
static void mechanical_walk(Perm *p, Cit *c, float steps) {
  int mt = STC(MechanicalState, p)->movementType;
  int r = rnd_int(4);
  if (mt == 0) {
    if (r == 0 && steps > 120) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
    else if (r == 1) walk_wait(c, 3, 7, 30, 60);
    else if (r == 2) walk_wait(c, 12, 16, 30, 60);
    else idle(c);
  } else if (mt == 1) {
    if (r == 0 && steps > 120 && c->relativeX < 11) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
    else if (r == 1) walk_wait(c, 3, 7, 30, 60);
    else if (r == 2 && c->relativeY < 5) walk_wait(c, 12, 16, 30, 60);
    else idle(c);
  } else if (mt == 2) {
    if (r == 0 && steps > 120 && c->relativeX > 11) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
    else if (r <= 2 && c->relativeY < 5) walk_wait(c, 7, 16, 30, 60);
    else if (r <= 2) walk_wait(c, 12, 16, 30, 60);
    else idle(c);
  } else {
    if (r == 0 && steps > 120 && c->relativeX > 11) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
    else if ((r == 1 || r == 2) && c->relativeY < 5) walk_wait(c, 7, 16, 30, 60);
    else if (r == 2) walk_wait(c, 12, 16, 30, 60);
    else idle(c);
  }
}
static void mechanical_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  const MechanicalState *s = STC(MechanicalState, p);
  draw_normal(b, p, layer, x, y, mechanical_house_texture(p), false);
  if (layer == LAYER_BG) {
    static const uint16_t anim[6] = {0, SPR_MECHANICALHOUSE_LEFTRIGHT_BGANIM, SPR_MECHANICALHOUSE_LEFT_BGANIM,
                                     SPR_MECHANICALHOUSE_TOP_BGANIM, SPR_MECHANICALHOUSE_RIGHT_BGANIM,
                                     SPR_MECHANICALHOUSE_BOTTOM_BGANIM};
    if (s->variant != MH_NONE) {
      /* its factory's back animation's phase */
      const Perm *f = s->variant <= MH_LEFT ? P(p->right) : s->variant == MH_TOP ? P(p->bottom) : s->variant == MH_RIGHT ? P(p->left) : P(p->top);
      uint16_t a = anim[s->variant % 6];
      int n = frames_of(a, 20);
      dr_frame(b, a, 20, f ? factory_frame(f, n) : 0, x, y, 0, 0, 0, 255, GFX_NO_TINT);
    }
    if (has_upgrade_bit(p, UB_MechanicalHouse_HeatedBed)) dr_sprite(b, SPR_MECHANICALHOUSE_HEATEDBED, x, y, 0, 0, 0, 255);
  } else if (layer == LAYER_ABOVE) {
    switch (s->variant) {
    case MH_LEFTRIGHT: dr_sprite(b, s->pipeAlt ? SPR_MECHANICALHOUSE_PIPE_H3_BF : SPR_MECHANICALHOUSE_PIPE_H3, x, y, -3, 0, 0, 255); break;
    case MH_LEFT: dr_sprite(b, SPR_MECHANICALHOUSE_PIPE_H, x, y, 20, 4, 0, 255); break;
    case MH_TOP:
      if (s->pipeAlt) dr_sprite(b, SPR_MECHANICALHOUSE_PIPE_V3, x, y, 1, -3, 0, 255);
      else dr_sprite(b, SPR_MECHANICALHOUSE_PIPE_V1, x, y, 1, 20, 0, 255);
      break;
    case MH_RIGHT:
      if (s->pipeAlt) dr_sprite(b, SPR_MECHANICALHOUSE_PIPE_H2_BF, x, y, -3, 5, 0, 255);
      else dr_sprite(b, SPR_MECHANICALHOUSE_PIPE_H2, x, y, -3, 11, 0, 255);
      break;
    case MH_BOTTOM: dr_sprite(b, SPR_MECHANICALHOUSE_PIPE_V2, x, y, 3, -3, 0, 255); break;
    default: break;
    }
  }
}
const PermOps ops_MechanicalHouse = {.on_city_change = mechanical_city_change, .walk_around = mechanical_walk,
                                     .draw = mechanical_draw};

/* ---------------------------------------------------------------- SpaciousHouse, CuttingEdgeHome */
static void spacious_walk(Perm *p, Cit *c, float steps) {
  (void)p;
  int r = rnd_int(3);
  if (r == 0 && steps > 120) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  else if (r == 1) {
    if (c->relativeY < 5) walk_wait(c, 3, 15, 30, 60);
    else if (rnd_int(2) == 1) walk_wait(c, 3, 7, 30, 60);
    else walk_wait(c, 12, 16, 30, 60);
  } else idle(c);
}
const PermOps ops_SpaciousHouse = {.walk_around = spacious_walk, .draw = house_draw};

static void cutting_edge_walk(Perm *p, Cit *c, float steps) {
  (void)p;
  int r = rnd_int(3);
  if (r == 0 && steps > 120) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  else if (r == 1) {
    if (c->relativeY < 5) walk_wait(c, 3, 15, 30, 60);
    else walk_wait(c, 5, 15, 30, 60);
  } else idle(c);
}
static int cutting_edge_residents(const Perm *p) {
  return PC(p)->residents + (has_upgrade_bit(p, UB_CuttingEdgeHome_BiggerTV) ? 1 : 0);   /* extraCapacity */
}
static float cutting_edge_years(const Perm *p) { return has_upgrade_bit(p, UB_CuttingEdgeHome_ModernHomePlant) ? 0.05f : 0; }
const PermOps ops_CuttingEdgeHome = {.walk_around = cutting_edge_walk, .residents = cutting_edge_residents,
                                     .years_to_live_longer = cutting_edge_years, .draw = house_draw};

/* ---------------------------------------------------------------- ParkPod, BlossomHut, TreeHuggerBase: residents disappear inside */
static void vanish_at(Cit *c, int x) {
  if (c->relativeX == x) c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
  else move_x(c, x);
}
static void park_pod_init(Perm *p) {
  ParkPodState *s = ST(ParkPodState, p);
  s->podTexture = (uint8_t)rnd_int(frames_of(SPR_PARKPOD, 20));
}
static void park_pod_walk(Perm *p, Cit *c, float steps) { (void)p, (void)steps; vanish_at(c, 9); }
static void park_pod_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer == LAYER_BG) dr_frame(b, SPR_PARKPOD, 20, STC(ParkPodState, p)->podTexture, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
static void park_pod_window(Perm *p, WinOut *w) {
  (void)p;
  win_button(w, 0, "Change Building Color", WA_CYCLE_0, true);
  win_button(w, 0, "Change Variant", WA_CYCLE_1, true);
}
static void park_pod_action(Perm *p, int a) {
  ParkPodState *s = ST(ParkPodState, p);
  if (a == WA_CYCLE_0) s->mainTexture = (uint8_t)((s->mainTexture + 1) % 3);
  else if (a == WA_CYCLE_1) s->podTexture = (uint8_t)((s->podTexture + 1) % frames_of(SPR_PARKPOD, 20));
}
const PermOps ops_ParkPod = {.init = park_pod_init, .walk_around = park_pod_walk, .draw = park_pod_draw,
                             .window = park_pod_window, .window_action = park_pod_action};

static void blossom_hut_walk(Perm *p, Cit *c, float steps) { (void)p, (void)steps; vanish_at(c, 9); }
const PermOps ops_BlossomHut = {.walk_around = blossom_hut_walk, .draw = draw_merging};

static void tree_hugger_walk(Perm *p, Cit *c, float steps) { (void)p, (void)steps; vanish_at(c, 10); }
static void tree_hugger_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer == LAYER_BG) dr_sprite(b, SPR_TREEHUGGERBASE, x, y, 0, 0, 0, 255);
}
const PermOps ops_TreeHuggerBase = {.walk_around = tree_hugger_walk, .draw = tree_hugger_draw};

/* ---------------------------------------------------------------- Loft */
static void loft_init(Perm *p) {
  LoftState *s = ST(LoftState, p);
  s->interior = (uint8_t)rnd_int(frames_of(SPR_LOFT_FURNISHINGS, 20));
  p->flags |= PF_IS_ENTERTAINMENT;
}
static float loft_attractiveness(const Perm *p) { (void)p; return 88; }
static float loft_capacity(const Perm *p) { return p->nresidents * 5; }
static bool never_open(const Perm *p) { (void)p; return false; }
static void loft_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  int x = rnd_between(3, 16);
  cit_move_and_wait(c, x, rnd_between(200, 240), THEN_NONE, 0, false, true);
}
static void loft_walk(Perm *p, Cit *c, float steps) {
  (void)steps;
  if (c->relativeY >= 5) return;
  bool mirrored = STC(LoftState, p)->mirrored;
  if (mirrored ? c->relativeX >= 5 || rnd_float() < 0.5f : c->relativeX < 14 || rnd_float() < 0.5f)
    walk_wait(c, 3, 16, 60, 90);
}
static void loft_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  const LoftState *s = STC(LoftState, p);
  draw_custom(b, p, layer, x, y, PC(p)->sprite, s->mirrored, 0, 0, SPR_LOFT_FURNISHINGS, s->interior);
}
static void loft_window(Perm *p, WinOut *w) {
  (void)p;
  win_button(w, 0, "Mirror", WA_MIRROR, true);
  win_button(w, 0, "Change Interior", WA_CYCLE_0, true);
}
static void loft_action(Perm *p, int a) {
  LoftState *s = ST(LoftState, p);
  if (a == WA_MIRROR) s->mirrored ^= 1;
  else if (a == WA_CYCLE_0) s->interior = (uint8_t)((s->interior + 1) % frames_of(SPR_LOFT_FURNISHINGS, 20));
}
const PermOps ops_Loft = {.init = loft_init, .attractiveness = loft_attractiveness, .entertainment_capacity = loft_capacity,
                          .is_open = never_open, .is_open_for_existing_visitors = never_open,
                          .be_entertained = loft_be_entertained, .walk_around = loft_walk, .draw = loft_draw,
                          .window = loft_window, .window_action = loft_action};
float loft_door_x(const Perm *p) { return STC(LoftState, p)->mirrored ? 4 : 14; }

/* ---------------------------------------------------------------- TinkerersHome */
static void tinkerers_init(Perm *p) {
  TinkerersState *s = ST(TinkerersState, p);
  s->front = (uint8_t)rnd_int(frames_of(SPR_TINKERERSHOUSE_FRONT_ALT, 20));
  s->back = (uint8_t)rnd_int(frames_of(SPR_TINKERERSHOUSE_BACK_ALT, 20));
}
static void tinkerers_on_build(Perm *p) { (void)p; unlocks_unlock("MechanicalLiving"); }
static void tinkerers_walk(Perm *p, Cit *c, float steps) {
  (void)p;
  educate(c, 0.01f, 1.25f);
  int r = rnd_int(6);
  if (r < 2 && steps > 120) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  else if (r == 2 || r == 3 || steps < 10) {
    if (c->relativeY < 5) {
      if (r == 2) walk_wait(c, 3, 6, 30, 60);
      else walk_wait(c, 15, 16, 30, 60);
    } else if (r == 2) walk_wait(c, 3, 6, 30, 60);
    else walk_wait(c, 12, 16, 30, 60);
  } else idle(c);
}
static void tinkerers_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  const TinkerersState *s = STC(TinkerersState, p);
  draw_custom(b, p, layer, x, y, PC(p)->sprite, false, SPR_TINKERERSHOUSE_FRONT_ALT, s->front, SPR_TINKERERSHOUSE_BACK_ALT, s->back);
  draw_upgrades(b, p, layer, x, y, false);
}
const PermOps ops_TinkerersHome = {.init = tinkerers_init, .on_build = tinkerers_on_build, .walk_around = tinkerers_walk,
                                   .draw = tinkerers_draw};

/* ---------------------------------------------------------------- FarmHouse (Community House) */
static uint8_t TH_COMMUNITY;
/* the material its mode makes: 0 stone, 1 wood, 2 food */
static int material_type(const Perm *p) {
  return p->mode == MODE_FarmHouse_CHProduceStone ? 0 : p->mode == MODE_FarmHouse_CHProduceWood ? 1 : 2;
}
static int phase(const Perm *p) { return STC(FarmHouseState, p)->bits & 3; }
static void set_phase(Perm *p, int ph) {
  FarmHouseState *s = ST(FarmHouseState, p);
  s->bits = (uint8_t)((s->bits & ~3) | ph);
}
int farmhouse_farm_effects(const Perm *p) { return p->cls == CLS_FarmHouse ? STC(FarmHouseState, p)->bits >> 6 : 0; }
static float original_materials(int type) { return type == 0 ? 20 : type == 1 ? 25 : 10; }
/* a CHProduce* mode's constructor: progress reset if the type changes, then setMaterialType (food mode adds a farm
 * effect each time; other modes remove one: the original's list can hold two) */
void farmhouse_mode_set(Perm *p, int old_type, int type) {
  FarmHouseState *s = ST(FarmHouseState, p);
  if (old_type != type) {
    set_phase(p, 0);
    s->stageProgress = 0;
  }
  int fx = s->bits >> 6;
  if (type == 2) fx = fx < 3 ? fx + 1 : 3;
  else if (fx > 0) fx--;
  s->bits = (uint8_t)((s->bits & 0x3F) | fx << 6);
}
static void farmhouse_init(Perm *p) {
  /* Building's first mode (food), then the FarmHouse's random one */
  p->mode = MODE_FarmHouse_CHProduceFood;
  farmhouse_mode_set(p, -1, 2);
  int m = rnd_int(3);
  int old = material_type(p);
  p->mode = (uint8_t)m;
  farmhouse_mode_set(p, old, material_type(p));
}
static void farmhouse_update(Perm *p, float timeMod) {
  if (phase(p) != 0) return;
  FarmHouseState *s = ST(FarmHouseState, p);
  int type = material_type(p);
  if (type == 0) {
    s->stageProgress += timeMod * 0.05f;
    if (s->stageProgress >= 1) set_phase(p, 1), s->stageProgress = original_materials(0);
  } else if (day_light()) {
    s->stageProgress += (type == 1 ? 0.00018f : 0.00008f) * timeMod;
    if (s->stageProgress >= 1) set_phase(p, 1), s->stageProgress = original_materials(type);
  }
}
static void harvest(Perm *p, int m, float amount, int phase_after) {
  FarmHouseState *s = ST(FarmHouseState, p);
  float h = amount * boost();
  s->stageProgress -= h;
  produce(m, h);
  s->totalMaterialsProduced += h;
  if (s->stageProgress <= 0) s->stageProgress = 0, set_phase(p, phase_after);
}
static void th_community(Cit *c, uint16_t arg) {
  (void)c;
  Perm *p = P(arg & 0x3FF);
  if (!p || p->cls != CLS_FarmHouse) return;
  FarmHouseState *s = ST(FarmHouseState, p);
  switch (material_type(p)) {
  case 0:
    if (phase(p) == 1) harvest(p, M_STONE, 0.1f, 0);
    break;
  case 1:
    if (phase(p) == 0) {
      s->stageProgress += 0.005f;
      if (s->stageProgress >= 1) s->stageProgress = original_materials(1), set_phase(p, 1);
    } else {
      harvest(p, M_WOOD, 0.2f, 0);
    }
    break;
  case 2:
    if (phase(p) == 1) {
      harvest(p, M_FOOD, 0.25f, 2);
    } else if (phase(p) == 0) {
      s->stageProgress += 0.01f;
      if (s->stageProgress >= 1) s->stageProgress = original_materials(2), set_phase(p, 1);
    } else if (phase(p) == 2) {
      s->stageProgress += 0.1f;
      if (s->stageProgress >= 1) s->stageProgress = 0, set_phase(p, 0);
    }
    break;
  }
}
static void farmhouse_walk(Perm *p, Cit *c, float steps) {
  if (steps < 180) {
    if (c->relativeY > 5) {
      cit_change_floor(c, THEN_NONE, 0);
    } else {
      int tmin = phase(p) == 2 ? 20 : 50, tmax = phase(p) == 2 ? 30 : 80;
      int x = rnd_between(3, 7);
      cit_move_and_wait(c, x, rnd_between(tmin, tmax), TH_COMMUNITY, pid(p), true, false);
    }
  } else if (c->relativeX > 11 && rnd_int(3) == 1) {
    cit_change_floor_and_wait_random(c, 60, 90, THEN_NONE, 0);
  } else {
    walk_wait(c, 12, 16, 60, 90);
  }
}
static void farmhouse_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  if (layer != LAYER_BG) return;
  /* setResourceSpriteTexture: 5-pixel frames at (3, 3) */
  const FarmHouseState *s = STC(FarmHouseState, p);
  int type = material_type(p), ph = phase(p);
  static const uint16_t tex[3] = {SPR_COMMUNITYHOUSE_ROCK, SPR_COMMUNITYHOUSE_TREE, SPR_COMMUNITYHOUSE_CROP};
  uint16_t spr = tex[type];
  int n = frames_of(spr, 5), f = 0;
  uint8_t alpha = 255;
  if ((type == 0 || type == 1) && ph == 1) {
    f = (int)ceilf((1 - s->stageProgress / original_materials(type)) * (n - 1));
  } else if (type == 0) {
    float a = s->stageProgress;
    alpha = (uint8_t)(a <= 0 ? 0 : a >= 1 ? 255 : a * 255);
  } else if (type == 2 && ph == 0) {
    f = (int)(s->stageProgress * (n - 2));
  } else if (type == 1 && ph == 0) {
    spr = SPR_COMMUNITYHOUSE_TREE_GROW;
    n = frames_of(spr, 5);
    f = (int)(s->stageProgress * n);
  } else if (type == 2) {
    f = ph == 1 ? n - 2 : n - 1;
  }
  if (f < 0) f = 0;
  if (f > n - 1) f = n - 1;
  dr_frame(b, spr, 5, f, x, y, 3, 3, 0, alpha, GFX_NO_TINT);
}
static void farmhouse_window(Perm *p, WinOut *w) {
  win_line1(w, "[0] resources produced.", (int)STC(FarmHouseState, p)->totalMaterialsProduced);
}
const PermOps ops_FarmHouse = {.init = farmhouse_init, .update = farmhouse_update, .walk_around = farmhouse_walk,
                               .draw = farmhouse_draw, .window = farmhouse_window};

/* ---------------------------------------------------------------- homes with private teleporters */
static bool yes(const Perm *p) { (void)p; return true; }
static void teleport_lines(Perm *p, WinOut *w) {
  const PrivateTeleporterState *s = STC(PrivateTeleporterState, p);
  win_line1(w, "Teleported from [0] times.", s->timesUsed);
  win_line1(w, "Teleported to [0] times.", s->timesUsedTo);
}
static void villa_walk(Perm *p, Cit *c, float steps, int gxmin, int gxmax, bool villa) {
  float tx = transport_teleport_x(p);
  int r = rnd_int(3);
  if (r == 0 && steps > 120) {
    if (c->relativeX < 5) cit_move(c, 5, TH_CHANGE_FLOOR_WAIT_30_60, 0, false);
    else cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  } else if (r == 1 || (steps <= 10 && c->relativeX < tx + 0.5f)) {
    if (c->relativeY < 5) walk_wait(c, gxmin, gxmax, 30, 60);
    else if (!villa) walk_wait(c, 3, 16, 30, 60);
    else if (rnd_int(2) == 1) walk_wait(c, 5, 8, 30, 60);
    else walk_wait(c, 13, 16, 30, 60);
  } else {
    idle(c);
  }
}
static void villa_walk_around(Perm *p, Cit *c, float steps) { villa_walk(p, c, steps, 10, 16, true); }
static void villa_window(Perm *p, WinOut *w) {
  teleport_lines(p, w);
  teleporter_upkeep_lines(w);
}
void teleporter_upkeep_action(int a);
static void villa_action(Perm *p, int a) { (void)p; teleporter_upkeep_action(a); }
const PermOps ops_Villa = {.has_private_teleporter = yes, .walk_around = villa_walk_around, .draw = house_draw,
                           .window = villa_window, .window_action = villa_action};

/* Mansion: pairs of mansions side by side are worth 25 more each (set from the leftmost of a run) */
uint16_t mansion_texture(const Perm *p) {
  int v = STC(PrivateTeleporterState, p)->variant;
  return v == 1 ? SPR_MANSION_LEFT : v == 2 ? SPR_MANSION_RIGHT : SPR_MANSION;
}
static void mansion_city_change(Perm *p) {
  if (p->left && P(p->left)->cls == CLS_Mansion) return;
  bool has_left_pair = false;
  for (Perm *b = p; b && b->cls == CLS_Mansion; b = P(b->right)) {
    PrivateTeleporterState *s = ST(PrivateTeleporterState, b);
    if (has_left_pair) {
      s->variant = 2;
      has_left_pair = false;
      b->bonusAttractiveness = 25;
    } else {
      bool connected = b->right && P(b->right)->cls == CLS_Mansion;
      s->variant = connected ? 1 : 0;
      has_left_pair = true;
      b->bonusAttractiveness = connected ? 25 : 0;
    }
  }
}
static void mansion_walk(Perm *p, Cit *c, float steps) {
  educate(c, 0.01f, 1.25f);
  villa_walk(p, c, steps, 3, 9, false);
}
static float mansion_years(const Perm *p) { (void)p; return 0.025f; }
static void mansion_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, mansion_texture(p), false);
}
const PermOps ops_Mansion = {.has_private_teleporter = yes, .walk_around = mansion_walk, .years_to_live_longer = mansion_years,
                             .on_city_change = mansion_city_change, .draw = mansion_draw, .window = villa_window,
                             .window_action = villa_action};

/* FlowerPenthouse */
static float penthouse_attractiveness(const Perm *p) { (void)p; return 100; }
static float penthouse_years(const Perm *p) { (void)p; return 0.05f; }
static void penthouse_walk(Perm *p, Cit *c, float steps) {
  (void)steps;
  bool m = STC(PrivateTeleporterState, p)->mirrored;
  if (c->relativeY < 5) {
    if (m ? c->relativeX >= 5 || rnd_float() < 0.5f : c->relativeX < 14 || rnd_float() < 0.5f) walk_wait(c, 3, 16, 60, 90);
    else cit_change_floor_and_wait(c, 60, THEN_NONE, 0);
  } else if (m ? c->relativeX >= 5 || rnd_float() < 0.8f : c->relativeX < 14 || rnd_float() < 0.8f) {
    float r = rnd_float();
    if (r < 0.5f) walk_wait(c, m ? 15 : 1, m ? 18 : 4, 40, 60);
    else if (r < 0.8f) walk_wait(c, m ? 10 : 7, m ? 12 : 9, 60, 90);
    else walk_wait(c, m ? 3 : 14, m ? 5 : 16, 60, 90);
  } else {
    cit_change_floor_and_wait(c, 60, THEN_NONE, 0);
  }
}
static void penthouse_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  bool m = STC(PrivateTeleporterState, p)->mirrored;
  draw_normal(b, p, layer, x, y, PC(p)->sprite, m);
  /* the flower on top, when nothing is above */
  if (layer == LAYER_MAIN && !perm_at(p->world, p->col, p->row + 1)) {
    int h = spr_h(SPR_FLOWER_UP), w = spr_w(SPR_FLOWER_UP);
    dr_sprite(b, SPR_FLOWER_UP, x, y, m ? 20 - w : 0, -h, m ? GFX_FLIPX : 0, 255);
  }
}
static void penthouse_window(Perm *p, WinOut *w) {
  teleport_lines(p, w);
  win_button(w, 0, "Mirror", WA_MIRROR, true);
  teleporter_upkeep_lines(w);
}
static void penthouse_action(Perm *p, int a) {
  if (a == WA_MIRROR) ST(PrivateTeleporterState, p)->mirrored ^= 1;
  else teleporter_upkeep_action(a);
}
const PermOps ops_FlowerPenthouse = {.attractiveness = penthouse_attractiveness, .has_private_teleporter = yes,
                                     .years_to_live_longer = penthouse_years, .walk_around = penthouse_walk,
                                     .draw = penthouse_draw, .window = penthouse_window, .window_action = penthouse_action};

/* ---------------------------------------------------------------- LandedExplorationShip */
static float ship_attractiveness(const Perm *p) { (void)p; return 50; }
static void ship_walk(Perm *p, Cit *c, float steps) {
  (void)steps;
  bool m = STC(PrivateTeleporterState, p)->mirrored;
  int r = rnd_int(3);
  int mm = m ? -1 : 1, ma = m ? 20 : 0;
  /* (as written: the mirrored tests compare the other way round) */
  if (c->relativeX * mm > (ma + 11 * mm) * mm) c->relativeX = 11;
  if (c->relativeX * mm <= (ma + mm) * mm) c->relativeX = 2;
  if (r == 1) {
    int x = rnd_getint(ma + mm * 4, ma + mm * 5);
    cit_move_and_wait(c, x, rnd_between(30, 60), THEN_NONE, 0, false, false);
  } else if (r == 2) {
    int x = rnd_getint(ma + mm * 9, ma + mm * 12);
    cit_move_and_wait(c, x, rnd_between(30, 60), THEN_NONE, 0, false, false);
  } else {
    idle(c);
  }
}
static void ship_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, STC(PrivateTeleporterState, p)->mirrored);
}
static void ship_window(Perm *p, WinOut *w) {
  (void)p;
  win_line(w, "Used as a house.");
  win_button(w, 0, "Mirror", WA_MIRROR, true);
}
static void ship_action(Perm *p, int a) {
  if (a == WA_MIRROR) ST(PrivateTeleporterState, p)->mirrored ^= 1;
}
const PermOps ops_LandedExplorationShip = {.attractiveness = ship_attractiveness, .walk_around = ship_walk, .draw = ship_draw,
                                           .window = ship_window, .window_action = ship_action};

/* ---------------------------------------------------------------- HouseOfTheKeyFriends */
static void key_friends_walk(Perm *p, Cit *c, float steps) {
  (void)p;
  int r = rnd_int(5);
  if (r == 0 && steps > 120) cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  else if (r == 1 || (r == 2 && c->relativeY < 5)) walk_wait(c, 3, 7, 30, 60);
  else if (r == 2) walk_wait(c, 12, 16, 30, 60);
  else idle(c);
}
const PermOps ops_HouseOfTheKeyFriends = {.walk_around = key_friends_walk, .draw = house_draw};
/* (SecretSocietyHouse walks around the same way) */
void key_friends_walk_around(Perm *p, Cit *c, float steps) { key_friends_walk(p, c, steps); }

/* a house's door x (doorX: mirrored homes move it) */
float house_door_x(const Perm *p) {
  switch (p->cls) {
  case CLS_Loft: return loft_door_x(p);
  case CLS_FlowerPenthouse: return STC(PrivateTeleporterState, p)->mirrored ? 4 : 14;
  case CLS_LandedExplorationShip: return STC(PrivateTeleporterState, p)->mirrored ? 14 : 4;
  default: return PC(p)->doorX;
  }
}

/* mirror() of the mirrorable homes (the story's "Loft_mirrored", "LandedExplorationShip_mirrored") */
void house_mirror(Perm *p) {
  if (p->cls == CLS_Loft) ST(LoftState, p)->mirrored ^= 1;
  else if (p->cls == CLS_FlowerPenthouse || p->cls == CLS_LandedExplorationShip) ST(PrivateTeleporterState, p)->mirrored ^= 1;
}

void bld_houses_init(void) { TH_COMMUNITY = then_register(th_community); }
