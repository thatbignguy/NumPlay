/* The Final Earth 2: world resources (worldResources.*): forests and rocks
 * (LimitedWorldResource: cut or mined, forests grow back), the alien ruins
 * and their findings, the player's priorities (ResourcePriorityManager), and
 * the gather places in buildings (tree plantations, rock teleporters) seen
 * as limited resources. */
#include <stdio.h>
#include "bldi.h"
#include "lang_data.h"

uint8_t world_unavailable[MAX_WORLDS];

/* ---------------------------------------------------------------- the kinds */
typedef struct {
  uint16_t spr, grow;          /* the cut stages and the regrowing stages (0: none) */
  uint16_t initial;
  float regrow;                /* regrowSpeed per tick in day light */
  int8_t climbX, climbY;       /* treeClimbX and treeClimbY */
} ResKind;
static const ResKind kinds[] = {
    [CLS_Forest] = {SPR_FOREST, SPR_FOREST_GROW, 200, 0.003f, 7, 13},
    [CLS_PineForest] = {SPR_PINEFOREST, SPR_PINEFOREST_GROW, 200, 0.005f, 15, 3},
    [CLS_BranchyForest] = {SPR_FOREST_BRANCHES, SPR_FOREST_BRANCHES_GROW, 200, 0.005f, 16, 12},
    [CLS_RedForest] = {SPR_REDFOREST, SPR_REDFOREST_GROW, 200, 0.005f, 16, 12},
    [CLS_PurpleForest] = {SPR_PURPLEFOREST, SPR_PURPLEFOREST_GROW, 200, 0.005f, 16, 12},
    [CLS_BirchForest] = {SPR_BIRCHFOREST, SPR_PINEFOREST_GROW, 120, 0.01f, 12, 13},
    [CLS_Rock] = {SPR_ROCK, 0, 250, 0, 0, 0},
    [CLS_Boulder] = {SPR_BOULDER, 0, 200, 0, 0, 0},
};
static const ResKind *kind_of(const Perm *p) { return p->cls <= CLS_Boulder ? &kinds[p->cls] : &kinds[CLS_Forest]; }
static bool is_forest(const Perm *p) { return is_a(p, B_FOREST); }
static bool is_rock(const Perm *p) { return is_a(p, B_ROCK); }
static bool is_limited(const Perm *p) { return is_forest(p) || is_rock(p); }
static bool is_plantation(const Perm *p) { return p->cls == CLS_TreePlantation || p->cls == CLS_StoneTeleporter; }
float resource_initial(const Perm *p) { return kind_of(p)->initial; }
void resource_climb(const Perm *p, int *x, int *y) { *x = kind_of(p)->climbX, *y = kind_of(p)->climbY; }

/* ---------------------------------------------------------------- ILimitedMaterialGatherPlace */
float limited_materials_left(const Perm *p) {
  if (is_limited(p)) return STC(LimitedState, p)->materialsLeft;
  if (is_plantation(p)) return STC(PlantationState, p)->materialsLeft;
  return 0;
}
void limited_take(Perm *p, float amount) {
  if (is_limited(p)) ST(LimitedState, p)->materialsLeft -= amount;
  else if (is_plantation(p)) ST(PlantationState, p)->materialsLeft -= amount;
}
bool limited_stay_if_empty(const Perm *p) { return p->cls == CLS_StoneTeleporter; }
bool limited_do_not_gather(const Perm *p) { return is_limited(p) && STC(LimitedState, p)->managementMode == 1; }
bool limited_destroyed_on_empty(const Perm *p) {
  if (is_forest(p)) return STC(LimitedState, p)->managementMode == 2;
  return is_rock(p);
}
int limited_min_x(const Perm *p) { return p->cls == CLS_TreePlantation ? 4 : p->cls == CLS_StoneTeleporter ? 5 : 1; }
int limited_max_x(const Perm *p) { return p->cls == CLS_TreePlantation ? 14 : p->cls == CLS_StoneTeleporter ? 13 : 17; }

/* ---------------------------------------------------------------- priorities (kept in each resource's state, saved with it) */
bool resource_prioritized(const Perm *p) { return is_limited(p) && STC(LimitedState, p)->prioritized; }
bool resource_any_prioritized(int base) {
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && is_a(&perms[i], (uint32_t)base) && STC(LimitedState, &perms[i])->prioritized) return true;
  return false;
}
/* invalidatePathfindingAfterPrioritizeChange: the miners or woodcutters look again */
static void priorities_changed(const Perm *res) {
  int cls = is_rock(res) ? CLS_StoneMine : is_forest(res) ? CLS_WoodcuttingCentre : -1;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && class_is(perms[i].cls, cls)) ST(BlueCollarState, &perms[i])->nearest = NONE;
}
void resource_set_prioritized(Perm *p, bool on) {
  if (!is_limited(p)) return;
  LimitedState *s = ST(LimitedState, p);
  if (on) {
    s->prioritized = 1;
    priorities_changed(p);
  } else if (s->prioritized) {
    priorities_changed(p);
    s->prioritized = 0;
  }
}

/* ---------------------------------------------------------------- forests and rocks */
static void limited_init(Perm *p) {
  LimitedState *s = ST(LimitedState, p);
  s->materialsLeft = kind_of(p)->initial;
  s->regrowProgress = 0;
  s->managementMode = 0;
  s->prioritized = 0;
}
static void limited_destroy(Perm *p) { resource_set_prioritized(p, false); }
/* LimitedWorldResource.update: grows back once empty (rocks never: their speed is 0) */
static void limited_update(Perm *p, float timeMod) {
  LimitedState *s = ST(LimitedState, p);
  if (s->materialsLeft > 0) return;
  float speed = kind_of(p)->regrow;
  if (day_light()) s->regrowProgress += speed * timeMod;
  else if (dim_light()) s->regrowProgress += speed * timeMod * 0.5f;
  if (s->regrowProgress >= 100) {
    s->materialsLeft = kind_of(p)->initial;
    s->regrowProgress = 0;
  }
}
static void limited_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  if (layer != LAYER_MAIN) return;
  const ResKind *k = kind_of(p);
  const LimitedState *s = STC(LimitedState, p);
  int n = frames_of(k->spr, 20), ng = k->grow ? frames_of(k->grow, 20) : 0;
  int f = limited_frame(s->materialsLeft, k->initial, s->regrowProgress, n, ng);
  if (s->materialsLeft <= 0.0000001f && ng) dr_frame(b, k->grow, 20, f < ng ? f : ng - 1, x, y, 0, 0, 0, 255, GFX_NO_TINT);
  else dr_frame(b, k->spr, 20, f < n ? f : n - 1, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
/* LimitedWorldResource.addWindowInfoLines: "[0]% grown." or "[0]/[1] [2] left." */
static void left_line(const Perm *p, WinOut *w) {
  const LimitedState *s = STC(LimitedState, p);
  char t[64], n[16];
  if (s->materialsLeft <= 0) {
    win_text(w, lo1(t, sizeof t, L_pct_grown, (int)s->regrowProgress));
    return;
  }
  /* (get_resourceName: "wood" or "stone", as written) */
  lo2(t, sizeof t, L_pct_left, (int)s->materialsLeft, kind_of(p)->initial);
  char *q = strstr(t, "[2]");
  if (q) {
    snprintf(n, sizeof n, "%s", q + 3);
    snprintf(q, sizeof t - (size_t)(q - t), "%s%s", is_forest(p) ? "wood" : "stone", n);
  }
  win_text(w, t);
}

/* the management window: for this one or for all of its kind (gui_UpgradeWindowParts.hasMultiUpgradeModeOn) */
static bool manage_all;
static uint16_t uproot_confirm;   /* the forest whose Uproot button asks "Really Uproot?" */
static void set_mode(Perm *p, int mode) {
  if (manage_all) {
    bool forest = is_forest(p);
    for (int i = 1; i < MAX_PERMS; i++) {
      Perm *q = &perms[i];
      if ((q->flags & PF_USED) && (forest ? is_forest(q) : is_rock(q))) ST(LimitedState, q)->managementMode = (uint8_t)mode;
    }
  } else {
    ST(LimitedState, p)->managementMode = (uint8_t)mode;
  }
}
static int count_kind(bool forest) {
  int n = 0;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && (forest ? is_forest(&perms[i]) : is_rock(&perms[i]))) n++;
  return n;
}
/* the header with its One and All buttons (addOneAndMaxButtons, no cost), and the question */
static void manage_header(WinOut *w, bool forest, char *all_tip, int size) {
  snprintf(all_tip, (size_t)size, "%s (%d).", forest ? L_forest_management_all : L_set_management_all_rocks, count_kind(forest));
  win_heading(w, forest ? L_forest_management : L_rock_mining, WA_SPECIAL, WA_SPECIAL_2, manage_all, false,
              forest ? L_forest_management_only_this : L_set_management_rock, all_tip);
  char t[80];
  const char *q = forest ? (manage_all ? L_forest_management_question_1 : L_forest_management_question_2)
                         : (manage_all ? L_rock_management_question_1 : L_rock_management_question_2);
  snprintf(t, sizeof t, "%s ", q);
  win_text(w, t);
}
/* createActivatableButton: the mode chosen is the active box */
static void mode_option(WinOut *w, const LimitedState *s, int mode, const char *name, const char *desc_all,
                        const char *desc_one) {
  win_button(w, 0, name, WA_MODE_0 + mode, s->managementMode != mode);
  win_text(w, manage_all ? desc_all : desc_one);
}
static const char texture_credit[] = "Texture by DT from the Discord, who won the first art contest!";
static void forest_window(Perm *p, WinOut *w) {
  const LimitedState *s = STC(LimitedState, p);
  if (p->cls == CLS_BranchyForest) {
    win_text(w, texture_credit);
    win_space(w, 4);
  }
  left_line(p, w);
  char all_tip[80];
  manage_header(w, true, all_tip, sizeof all_tip);
  mode_option(w, s, 0, L_forest_regrow, L_forest_regrow_description_1, L_forest_regrow_description_2);
  mode_option(w, s, 1, L_forest_protect, L_forest_protect_description_1, L_forest_protect_description_2);
  mode_option(w, s, 2, L_forest_uproot, L_forest_uproot_description_1, L_forest_uproot_description_2);
  win_space(w, 4);
  /* the bottom row: Uproot (then "Really Uproot?") once cut down, else Prioritize */
  if (s->materialsLeft <= 0) {
    bool confirm = uproot_confirm == pid(p);
    win_bottom_button(w, confirm ? L_really_uproot : L_forest_instant_uproot, WA_PRIORITIZE, NULL,
                      confirm ? NULL : L_forest_instant_uproot_description);
  } else if (s->prioritized) {
    win_bottom_button(w, L_prioritized, WA_PRIORITIZE, L_prioritized, L_prioritize_description_1);
  } else {
    win_bottom_button(w, L_prioritize, WA_PRIORITIZE, L_prioritize, L_prioritize_description_2);
  }
}
static void rock_window(Perm *p, WinOut *w) {
  const LimitedState *s = STC(LimitedState, p);
  if (p->cls == CLS_Boulder) {
    win_text(w, texture_credit);
    win_space(w, 4);
  }
  left_line(p, w);
  char all_tip[80];
  manage_header(w, false, all_tip, sizeof all_tip);
  mode_option(w, s, 0, L_rock_management_mine, L_rock_management_mine_description_1, L_rock_management_mine_description_2);
  mode_option(w, s, 1, L_forest_protect, L_rock_management_protect_description_1, L_rock_management_protect_description_2);
  win_space(w, 4);
  if (s->prioritized) win_bottom_button(w, L_prioritized, WA_PRIORITIZE, L_prioritized, L_rock_prioritized_description);
  else win_bottom_button(w, L_prioritize, WA_PRIORITIZE, L_prioritize, L_rock_prioritized_description_2);
}
static void limited_action(Perm *p, int a) {
  LimitedState *s = ST(LimitedState, p);
  if (a == WA_SPECIAL) manage_all = false;
  else if (a == WA_SPECIAL_2) manage_all = true;
  else if (a >= WA_MODE_0 && a <= WA_MODE_0 + (is_forest(p) ? 2 : 1)) set_mode(p, a - WA_MODE_0);
  else if (a == WA_PRIORITIZE) {
    if (is_forest(p) && s->materialsLeft <= 0) {
      /* Uproot, then "Really Uproot?" */
      if (uproot_confirm == pid(p)) {
        uproot_confirm = 0;
        perm_destroy(pid(p), false);
      } else {
        uproot_confirm = pid(p);
      }
      return;
    }
    resource_set_prioritized(p, !s->prioritized);
  }
  if (a != WA_PRIORITIZE) uproot_confirm = 0;
}
#define LIMITED_OPS(win) \
  {.init = limited_init, .destroy = limited_destroy, .update = limited_update, .draw = limited_draw, .window = win, \
   .window_action = limited_action}
const PermOps ops_Forest = LIMITED_OPS(forest_window);
const PermOps ops_PineForest = LIMITED_OPS(forest_window);
const PermOps ops_BranchyForest = LIMITED_OPS(forest_window);
const PermOps ops_RedForest = LIMITED_OPS(forest_window);
const PermOps ops_PurpleForest = LIMITED_OPS(forest_window);
const PermOps ops_BirchForest = LIMITED_OPS(forest_window);
const PermOps ops_Rock = LIMITED_OPS(rock_window);
const PermOps ops_Boulder = LIMITED_OPS(rock_window);

/* a forest or rock planted from the build menu (Builder): its stock and growth as buildableWorldResourcesInfo says */
void resource_planted(Perm *p, float materials_left, float regrow) {
  if (!p || !is_limited(p)) return;
  ST(LimitedState, p)->materialsLeft = materials_left;
  ST(LimitedState, p)->regrowProgress = regrow;
}

/* World.update: what a world knows is not there is forgotten each new day (and on every city change) */
static int unavailable_day;
void worlds_update(float timeMod) {
  (void)timeMod;
  int d = day_number();
  if (d != unavailable_day) {
    memset(world_unavailable, 0, sizeof world_unavailable);
    unavailable_day = d;
  }
}

/* ---------------------------------------------------------------- AlienRuins */
typedef struct { const char *name, *text; uint8_t m1, m2; uint8_t a1, a2; } Finding;
/* level by level: options a to d (AlienRuins) and e (ComputerAlienRuins) */
static const Finding findings[4][5] = {
    {{L_alien_ruins_find_1a, L_alien_ruins_description_1a, M_WOOD, 0, 20, 0},
     {L_alien_ruins_find_1b, L_alien_ruins_description_1b, M_STONE, 0, 25, 0},
     {L_alien_ruins_find_1c, L_alien_ruins_description_1c, M_STONE, 0, 22, 0},
     {L_alien_ruins_find_1d, L_alien_ruins_description_1d, M_FOOD, M_MACHINEPARTS, 12, 5},
     {L_alien_ruins_find_1e, L_alien_ruins_description_1e, M_STONE, 0, 35, 0}},
    {{L_alien_ruins_find_2a, L_alien_ruins_description_2a, M_WOOD, 0, 40, 0},
     {L_alien_ruins_find_2b, L_alien_ruins_description_2b, M_STONE, 0, 35, 0},
     {L_alien_ruins_find_2c, L_alien_ruins_description_2c, M_MACHINEPARTS, 0, 20, 0},
     {L_alien_ruins_find_2d, L_alien_ruins_description_2d, M_FOOD, 0, 42, 0},
     {L_alien_ruins_find_2e, L_alien_ruins_description_2e, M_FOOD, 0, 55, 0}},
    {{L_alien_ruins_find_3a, L_alien_ruins_description_3a, M_WOOD, M_MACHINEPARTS, 10, 5},
     {L_alien_ruins_find_3b, L_alien_ruins_description_3b, M_WOOD, 0, 17, 0},
     {L_alien_ruins_find_3c, L_alien_ruins_description_3c, M_STONE, 0, 15, 0},
     {L_alien_ruins_find_3d, L_alien_ruins_description_3d, M_FOOD, 0, 22, 0},
     {L_alien_ruins_find_3e, L_alien_ruins_description_3e, M_MACHINEPARTS, 0, 7, 0}},
    {{L_alien_ruins_find_4a, L_alien_ruins_description_4a, 0, 0, 0, 0},
     {L_alien_ruins_find_4b, L_alien_ruins_description_4b, 0, 0, 0, 0},
     {L_alien_ruins_find_4c, L_alien_ruins_description_4c, 0, 0, 0, 0},
     {L_alien_ruins_find_4d, L_alien_ruins_description_4d, 0, 0, 0, 0},
     {L_alien_ruins_find_4e, L_alien_ruins_description_4e, 0, 0, 0, 0}}};
/* alreadyFoundBonuses: the findings any ruins made (bit level * 5 + option), made again from the ruins on load */
static uint32_t ruins_found;
static const uint16_t ruins_sprites[3] = {SPR_ALIENRUINS, SPR_ALIENRUINS_2, SPR_ALIENRUINS_3};

static void ruins_init(Perm *p) {
  RuinsState *s = ST(RuinsState, p);
  memset(s, 0, sizeof *s);
  s->sprite = p->cls == CLS_ComputerAlienRuins ? 2 : (uint8_t)rnd_int(2);
}
float ruins_explored(const Perm *p) { return STC(RuinsState, p)->explored4 / 4.0f; }
void ruins_climb(const Perm *p, int *x, int *y) {
  int sp = STC(RuinsState, p)->sprite;
  if (p->cls == CLS_ComputerAlienRuins) *x = 9, *y = 15;
  else if (sp == 0) *x = 9, *y = 17;
  else *x = 15, *y = 16;
}
static int count_ruins(void) {
  int n = 0;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && is_a(&perms[i], B_RUINS)) n++;
  return n;
}
static void finding_get(int level, int opt) {
  const Finding *f = &findings[level][opt];
  if (f->a1) produce(f->m1, f->a1);
  if (f->a2) produce(f->m2, f->a2);
  if (level == 3) switch (opt) {
    case 0: bonuses.extraFoodFromFarms += 5; break;
    case 1:
      unlocks_unlock("AlienHouse");
      unlocks_unlock("SlimyLiving");
      break;
    case 2: bonuses.machinePartsFactorySpeed *= 1.1f; break;
    case 3: bonuses.labSpeed *= 1.05f; break;
    }
}
/* awardAnyBonuses: one level's finding once a quarter more is explored */
static void ruins_award(Perm *p) {
  RuinsState *s = ST(RuinsState, p);
  float explored = ruins_explored(p);
  if (!(s->bonusesAwarded < (int)explored / 25 && explored >= 25 * (s->bonusesAwarded + 1)) || s->bonusesAwarded >= 4) return;
  int level = s->bonusesAwarded;
  int first = p->cls == CLS_ComputerAlienRuins ? 4 : 0, last = p->cls == CLS_ComputerAlienRuins ? 4 : 3;
  uint8_t options[5];
  int n = 0;
  for (int o = first; o <= last; o++)
    if (!(ruins_found >> (level * 5 + o) & 1)) options[n++] = (uint8_t)o;
  if (n > 0) {
    int pick = options[rnd_int(n)];
    bool housing = false;
    for (int i = 0; i < n; i++)
      if (level == 3 && options[i] == 1) housing = true;
    if (housing && count_ruins() < 4) pick = 1;
    finding_get(level, pick);
    s->found[level] = (uint8_t)(pick + 1);
    ruins_found |= 1u << (level * 5 + pick);
  }
  s->bonusesAwarded++;
}
void ruins_explore(Perm *p, float amount) {
  RuinsState *s = ST(RuinsState, p);
  float e = ruins_explored(p) + amount;
  s->explored4 = (uint16_t)(e * 4 + 0.5f);
  ruins_award(p);
}
static void ruins_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  if (layer == LAYER_MAIN) dr_sprite(b, ruins_sprites[STC(RuinsState, p)->sprite % 3], x, y, 0, 0, 0, 255);
}
/* addWindowInfoLines: explored, the "Findings" heading and each find; explored ruins can move (createMoveWindow:
 * the user interface picks an empty column and calls bld_move_resource) */
static void ruins_window(Perm *p, WinOut *w) {
  const RuinsState *s = STC(RuinsState, p);
  char t[256];
  win_text(w, lo1(t, sizeof t, L_n_pct_explored, (int)ruins_explored(p)));
  win_heading(w, L_findings, -1, -1, false, false, NULL, NULL);
  bool any = false;
  for (int l = 0; l < 4; l++) {
    if (!s->found[l]) continue;
    const Finding *f = &findings[l][(s->found[l] - 1) % 5];
    snprintf(t, sizeof t, "%s: %s", f->name, f->text);
    win_text(w, t);
    any = true;
  }
  if (!any) win_text(w, L_exploration_nothing_found);
  if (ruins_explored(p) >= 100) win_bottom_button(w, L_move, WA_UI_MOVE_RUINS, NULL, NULL);
}
const PermOps ops_AlienRuins = {.init = ruins_init, .draw = ruins_draw, .window = ruins_window};
const PermOps ops_ComputerAlienRuins = {.init = ruins_init, .draw = ruins_draw, .window = ruins_window};

void ruins_new_city(void) { ruins_found = 0; }
void ruins_after_load(void) {
  ruins_found = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    const Perm *p = &perms[i];
    if (!(p->flags & PF_USED) || !is_a(p, B_RUINS)) continue;
    for (int l = 0; l < 4; l++)
      if (STC(RuinsState, p)->found[l]) ruins_found |= 1u << (l * 5 + (STC(RuinsState, p)->found[l] - 1) % 5);
  }
}

/* a world resource moved to an empty ground column (createWorldResourceBuilderMove): its state goes with it */
uint16_t bld_move_resource(uint16_t id, int w, int col) {
  Perm *p = P(id);
  if (!p || PC(p)->kind != KIND_RESOURCE || w < 0 || w >= nworlds || col < 0 || col >= worlds[w].ncols) return NONE;
  if (col_stack_height(w, col) != 0) return NONE;
  uint8_t cls = p->cls, s[sizeof p->s];
  memcpy(s, p->s, sizeof s);
  perm_destroy(id, false);
  uint16_t n = perm_new(cls, w, col, 0);
  if (!n) return NONE;
  memcpy(perms[n].s, s, sizeof s);
  city_update_connections();
  sim_update_pathfinder(true, n);
  return n;
}

void bld_world_init(void) {}
