/* The Final Earth 2: buying building upgrades and modes, research (city
 * upgrades) and policies (BuildingUpgrade.js, gui.CreateBuildingUpgrades,
 * gui.CreateCityUpgrades, gui.CreatePolicies, cityUpgrades.*, policies.*).
 * A building keeps its upgrades as bits and its mode as a number (Perm.upgrades
 * and Perm.mode, in its class's lists); the city keeps a bit per city upgrade
 * bought and per policy on. What they change is read where it is used. */
#include "bldi.h"

CityUpgradesState city_upgrades;

static inline int uid(int u) { return npcls + u; }
static int ustate(int u) { return unlock_state_uid(uid(u)); }
static bool bit(int u) { return city_upgrades.bought[u >> 5] >> (u & 31) & 1; }
static void set_bit(int u, bool on) {
  if (on) city_upgrades.bought[u >> 5] |= 1u << (u & 31);
  else city_upgrades.bought[u >> 5] &= ~(1u << (u & 31));
}
bool city_upgrade_bought(int u) { return u >= 0 && u < U_COUNT && upgrade_info[u].kind == UK_CITY_UPGRADE && bit(u); }
bool policy_on(int u) { return u >= 0 && u < U_COUNT && upgrade_info[u].kind == UK_POLICY && bit(u); }

/* the place of upgrade u in p's class lists, -1 if not there */
static int up_index(int cls, int u) {
  for (int k = 0; k < 3; k++)
    if (class_upgrades[cls].up[k] == u + 1) return k;
  return -1;
}
static int mode_index(int cls, int u) {
  for (int k = 0; k < 4; k++)
    if (class_upgrades[cls].mode[k] == u + 1) return k;
  return -1;
}
static bool has_modes(int cls) { return class_upgrades[cls].mode[0] != 0; }
static int mode_upgrade(const Perm *p) {
  return has_modes(p->cls) && p->mode < 4 && class_upgrades[p->cls].mode[p->mode] ? class_upgrades[p->cls].mode[p->mode] - 1 : -1;
}
bool upgrade_has(const Perm *p, int u) {
  int k = up_index(p->cls, u);
  if (k >= 0) return p->upgrades >> k & 1;
  return mode_upgrade(p) == u;
}
static bool in_upgrades(const Perm *p, int u) {
  int k = up_index(p->cls, u);
  return k >= 0 && (p->upgrades >> k & 1);
}

/* Materials.fromBuildingUpgradesInfo, without knowledge once researched */
static void price_of(int u, float out[M_COUNT]) {
  memcpy(out, upgrade_info[u].cost, sizeof(float) * M_COUNT);
  if (upgrade_info[u].kind <= UK_BUILDING_MODE && ustate(u) == UNLOCK_RESEARCHED) out[M_KNOWLEDGE] = 0;
}
static bool can_pay(const float m[M_COUNT]) { return mat_can_afford(m); }
static void pay(const float m[M_COUNT]) { mat_pay(m); }

/* availableTextures: the frames the player may see (the last ones only with the secret code "orchid") */
static int available_looks(int u) {
  const UpgradeInfo *ui = &upgrade_info[u];
  return ui->frames - (bld_secret_orchid ? 0 : ui->lockedFrames);
}

/* ---------------------------------------------------------------- a building's upgrades and modes */
/* the BuildingUpgrade constructor (and the upgrade's own) */
void upgrade_add(Perm *p, int u) {
  int k = up_index(p->cls, u);
  if (k < 0 || (p->upgrades >> k & 1)) return;
  p->upgrades |= (uint8_t)(1 << k);
  p->bonusAttractiveness = (int8_t)(p->bonusAttractiveness + upgrade_info[u].bonusAttractiveness);
  if (upgrade_info[u].sprite) {
    int n = available_looks(u);
    upgrade_set_look(p, u, n > 0 ? rnd_int(n) : 0);
  }
  if (u == U_FossilScanner) upgrades.stoneResearchCenterWithFossils = pid(p);
  if (u == U_HeatedBed && PC(p)->ops && PC(p)->ops->on_city_change) PC(p)->ops->on_city_change(p);
}
void herb_garden_mode(Perm *p);
void farmhouse_mode_set(Perm *p, int old_type, int type);
static int farm_type(int mode) {
  return mode == MODE_FarmHouse_CHProduceStone ? 0 : mode == MODE_FarmHouse_CHProduceWood ? 1 : 2;
}
/* the old mode's destroy, then the new one's constructor */
void upgrade_set_mode(Perm *p, int u) {
  int k = mode_index(p->cls, u);
  if (k < 0) return;
  int old = p->mode;
  p->mode = (uint8_t)k;
  if (p->cls == CLS_FarmHouse) farmhouse_mode_set(p, farm_type(old), farm_type(k));
  else if (p->cls == CLS_HerbGarden) herb_garden_mode(p);
  else if (u == U_RefinedMetalsFactoryHack && PC(p)->ops && PC(p)->ops->on_city_change) PC(p)->ops->on_city_change(p);
}
void upgrade_first_mode(Perm *p) { p->mode = 0; }

void upgrade_change_look(Perm *p, int u) {
  if (!upgrade_has(p, u) || upgrade_info[u].frames <= 1) return;
  int n = available_looks(u), l = upgrade_look(p, u) + 1;
  upgrade_set_look(p, u, l >= n ? 0 : l);
}

/* what a press does to one building (onUpgrade) */
static void apply(Perm *p, int u) {
  if (upgrade_info[u].kind == UK_BUILDING_MODE) upgrade_set_mode(p, u);
  else upgrade_add(p, u);
}
/* CreateBuildingUpgrades.upgradeAll: every building of the exact class without it, while the city can pay */
static void upgrade_all(const Perm *p, int u, const float price[M_COUNT]) {
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *q = &perms[i];
    if (!(q->flags & PF_USED) || !is_building(q) || q->cls != p->cls) continue;
    if (!in_upgrades(q, u) && mode_upgrade(q) != u && can_pay(price)) {
      pay(price);
      apply(q, u);
    }
  }
}
int upgrade_all_count(const Perm *p, int u, float total[M_COUNT], bool *can_afford_all) {
  float price[M_COUNT], each[M_COUNT];
  price_of(u, price);
  memcpy(each, price, sizeof each);
  each[M_KNOWLEDGE] = 0;
  double have[M_COUNT];
  for (int m = 0; m < M_COUNT; m++) have[m] = city.materials.m[m];
  int num = 0;
  bool all = true;
  for (int i = 1; i < MAX_PERMS; i++) {
    const Perm *q = &perms[i];
    if (!(q->flags & PF_USED) || !is_building(q) || q->cls != p->cls || in_upgrades(q, u)) continue;
    bool ok = true;
    for (int m = 0; m < M_COUNT; m++)
      if (have[m] < each[m]) ok = false;
    if (ok) {
      for (int m = 0; m < M_COUNT; m++) have[m] -= each[m];
      num++;
    } else {
      all = false;
    }
  }
  if (!all && num == 0) num = 1;
  if (can_afford_all) *can_afford_all = all;
  if (total) {
    for (int m = 0; m < M_COUNT; m++) total[m] = num <= 1 ? price[m] : each[m] * num;
    if (num > 1) total[M_KNOWLEDGE] = price[M_KNOWLEDGE];
  }
  return num;
}

/* ---------------------------------------------------------------- city upgrades (addToCity) and policies */
static void city_upgrade_effect(int u, bool loading) {
  switch (u) {
  case U_BirthControl: upgrades.hasBirthControl = true; break;
  case U_BuildingRecycling:
    recycling_amount = upgrades.recyclingAmount = fmaxf(upgrades.recyclingAmount, 0.5f);
    unlocks_unlock("BuildingRecycling2");
    break;
  case U_BuildingRecycling2:
    recycling_amount = upgrades.recyclingAmount = fmaxf(upgrades.recyclingAmount, 0.75f);
    unlocks_unlock("BuildingRecycling3");
    break;
  case U_BuildingRecycling3: recycling_amount = upgrades.recyclingAmount = fmaxf(upgrades.recyclingAmount, 0.9f); break;
  case U_ChipBinning: upgrades.computerChipFactorySpeed *= 1.2f; break;
  case U_OptimizedChipLayouts: upgrades.computerChipFactorySpeed *= 1.33f; break;
  case U_EcoFarmUpgrade:
    /* every eco farm standing gets the second plot's bonus (on load too: all of them then) */
    upgrades.extraEcoFarmFoodToPlot1 = 4;
    eco_farms_upgrade();
    break;
  case U_SlimyLiving: unlocks_unlock("LivingComputer"); break;
  case U_UniverseMapping:
    upgrades.stoneTeleporterChanceForRefinedMetals = 0.04f;
    upgrades.stoneTeleporterHasBigStones = true;
    upgrades.starDatabaseIsMoreEffective = true;
    break;
  case U_MedicalMachineLearning: upgrades.extendAgeBy += 0.05f; break;
  case U_GeneEditing:
    upgrades.hasGeneEditing = true;
    unlocks_unlock("RedForest");
    unlocks_unlock("PurpleForest");
    break;
  case U_LibraryAllowDuplicates: upgrades.hasGalacticLibrary = true; break;
  case U_ConnectedMachines: upgrades.hasConnectedMachines = true; break;
  default: break;   /* the Living ones change their classes' quality and residents: upgrades_*_bonus */
  }
  (void)loading;
}
/* buildingInfo's quality and residents, as the city upgrades bought change them */
int upgrades_quality_bonus(int cls) {
  switch (cls) {
  case CLS_ComfortableHouse: return bit(U_WoodenLiving) ? 10 : 0;
  case CLS_SpaciousHouse: return bit(U_SpaciousLiving) ? 15 : 0;
  case CLS_AlienHouse: return bit(U_SlimyLiving) ? 25 : 0;
  case CLS_SecretSocietyHouse: return bit(U_SecretiveLiving) ? 25 : 0;
  default: return 0;
  }
}
int upgrades_residents_bonus(int cls) {
  switch (cls) {
  case CLS_MechanicalHouse: case CLS_TinkerersHome: return bit(U_MechanicalLiving) ? 1 : 0;
  case CLS_SpaciousHouse: return bit(U_SuperSpaciousLiving) ? 1 : 0;
  default: return 0;
  }
}
/* SmallerClasses: the students beyond a school's new capacity leave, the last ones first */
static void schools_trim(void) {
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *s = &perms[i];
    if (!(s->flags & PF_USED) || !is_a(s, B_SCHOOL)) continue;
    for (int c = MAX_CITS - 1; c >= 1 && s->nstudents > bld_student_capacity(s); c--)
      if ((cits[c].flags & CF_ALIVE) && cits[c].school == i) cit_leave_school(&cits[c]);
  }
}
static void policy_effect(int u, bool on) {
  switch (u) {
  case U_FoodRationing: eating.foodRationing = on; break;
  case U_SmallerClasses:
    policies.schoolClassSizeMod = on ? 0.6666667f : 1;
    policies.schoolMaxEdu = on ? 1.15f : 1;
    sim_flags |= SIM_SCHOOLS_CHANGED;
    if (on) schools_trim();
    break;
  case U_MandatoryOvertime: policies.workTimeChange = on ? 2 : 0; break;
  }
}

/* ---------------------------------------------------------------- pressing a window's button */
bool upgrade_press(Perm *p, int u, bool all) {
  if (u < 0 || u >= U_COUNT) return false;
  const UpgradeInfo *ui = &upgrade_info[u];
  float price[M_COUNT];
  if (ui->kind == UK_CITY_UPGRADE) {
    if (bit(u) || ustate(u) != UNLOCK_UNLOCKED || !can_pay(ui->cost)) return false;
    pay(ui->cost);
    set_bit(u, true);
    city_upgrade_effect(u, false);
    sim_flags |= SIM_HOUSES_CHANGED;
    return true;
  }
  if (ui->kind == UK_POLICY) {
    if (bit(u)) {
      set_bit(u, false);
      policy_effect(u, false);
      sim_flags |= SIM_HOUSES_CHANGED;
      return true;
    }
    if (!can_pay(ui->cost)) return false;
    pay(ui->cost);
    set_bit(u, true);
    policy_effect(u, true);
    return true;
  }
  /* a building upgrade or mode (addUpgradeButton) */
  if (!p || (up_index(p->cls, u) < 0 && mode_index(p->cls, u) < 0)) return false;
  price_of(u, price);
  if (!can_pay(price)) return false;
  if (!upgrade_has(p, u)) {
    pay(price);
    apply(p, u);
  }
  sim_flags |= SIM_HOUSES_CHANGED;
  unlocks_research_uid(uid(u));
  price[M_KNOWLEDGE] = 0;
  if (all) upgrade_all(p, u, price);
  return true;
}
/* the modes' "all" chosen: this building's mode for all of its class (onAllButtonSelect) */
void upgrade_mode_to_all(Perm *p) {
  int u = mode_upgrade(p);
  if (u < 0) return;
  float price[M_COUNT];
  memcpy(price, upgrade_info[u].cost, sizeof price);
  price[M_KNOWLEDGE] = 0;
  upgrade_all(p, u, price);
}

/* ---------------------------------------------------------------- what the window lists */
static void option(UpgradeOption *o, int kind, int u, bool owned) {
  const UpgradeInfo *ui = &upgrade_info[u];
  o->kind = (uint8_t)kind;
  o->upgrade = (uint8_t)u;
  o->owned = owned;
  o->title = ui->title;
  o->text = ui->text;
  if (kind == UO_CITY || kind == UO_POLICY) memcpy(o->price, ui->cost, sizeof o->price);
  else price_of(u, o->price);
  o->affordable = can_pay(o->price);
  o->canChangeLook = false;
}
int upgrade_options(const Perm *p, UpgradeOption *out, int max) {
  if (!p || p->cls >= CLS_COUNT) return 0;
  const ClassUpgrades *cu = &class_upgrades[p->cls];
  int n = 0;
  for (int k = 0; k < 3 && cu->up[k] && n < max; k++) {
    int u = cu->up[k] - 1;
    if (ustate(u) == UNLOCK_LOCKED) continue;
    bool owned = p->upgrades >> k & 1;
    option(&out[n], UO_UPGRADE, u, owned);
    out[n].canChangeLook = owned && upgrade_info[u].frames > 1;
    n++;
  }
  for (int k = 0; k < 4 && cu->mode[k] && n < max; k++) {
    int u = cu->mode[k] - 1;
    if (ustate(u) == UNLOCK_LOCKED) continue;
    option(&out[n++], UO_MODE, u, p->mode == k);
  }
  /* city upgrades: those exactly Unlocked; a chain shows only its newest step */
  for (int k = 0; k < 6 && cu->city[k] && n < max; k++) {
    int u = cu->city[k] - 1;
    const UpgradeInfo *ui = &upgrade_info[u];
    if (ustate(u) != UNLOCK_UNLOCKED) continue;
    if (ui->next != 0xFF && ustate(ui->next) != UNLOCK_LOCKED) continue;
    option(&out[n], UO_CITY, u, bit(u));
    if (bit(u) && ui->next == 0xFF) {
      if (ui->titleDone) out[n].title = ui->titleDone;
      if (ui->textDone) out[n].text = ui->textDone;
    }
    n++;
  }
  for (int k = 0; k < 3 && cu->pol[k] && n < max; k++) {
    int u = cu->pol[k] - 1;
    if (ustate(u) != UNLOCK_UNLOCKED) continue;
    option(&out[n++], UO_POLICY, u, bit(u));
  }
  return n;
}

/* ---------------------------------------------------------------- the labs' smart upgrade */
static bool next_to_plantation(const Perm *p) {
  const uint16_t nb[4] = {p->left, p->right, p->top, p->bottom};
  for (int i = 0; i < 4; i++)
    if (P(nb[i]) && P(nb[i])->cls == CLS_TreePlantation) return true;
  return false;
}
static bool lab_wants(const Perm *p, int which) {
  if (!(p->flags & PF_USED) || p->cls != CLS_Laboratory) return false;
  if (which == 0) return adj_effect(p, EF_FARM) > 0 && !(p->upgrades >> UB_Laboratory_FarmingResearch & 1);
  return next_to_plantation(p) && !(p->upgrades >> UB_Laboratory_TreePlantationResearch & 1);
}
bool upgrade_smart_labs_price(float price[M_COUNT]) {
  int n[2] = {0, 0};
  for (int i = 1; i < MAX_PERMS; i++)
    for (int w = 0; w < 2; w++)
      if (lab_wants(&perms[i], w)) n[w]++;
  memset(price, 0, sizeof(float) * M_COUNT);
  const int us[2] = {U_FarmingResearch, U_TreePlantationResearch};
  for (int w = 0; w < 2; w++) {
    const float *c = upgrade_info[us[w]].cost;
    for (int m = 0; m < M_COUNT; m++) {
      float v = c[m] * n[w];
      if (m == M_KNOWLEDGE) v = ustate(us[w]) == UNLOCK_RESEARCHED ? 0 : fminf(v, c[m]);
      price[m] += v;
    }
  }
  return n[0] + n[1] > 0;
}
bool upgrade_smart_labs_buy(void) {
  float price[M_COUNT];
  if (!upgrade_smart_labs_price(price) || !can_pay(price)) return false;
  pay(price);
  /* (the lists are made before any lab changes) */
  uint8_t want[(MAX_PERMS + 7) / 8 * 2];
  memset(want, 0, sizeof want);
  for (int i = 1; i < MAX_PERMS; i++)
    for (int w = 0; w < 2; w++)
      if (lab_wants(&perms[i], w)) want[w * ((MAX_PERMS + 7) / 8) + (i >> 3)] |= (uint8_t)(1 << (i & 7));
  for (int w = 0; w < 2; w++)
    for (int i = 1; i < MAX_PERMS; i++)
      if (want[w * ((MAX_PERMS + 7) / 8) + (i >> 3)] >> (i & 7) & 1) upgrade_add(&perms[i], w ? U_TreePlantationResearch : U_FarmingResearch);
  unlocks_research_uid(uid(U_FarmingResearch));
  unlocks_research_uid(uid(U_TreePlantationResearch));
  return true;
}

/* ---------------------------------------------------------------- the building's recycle reward and the story */
/* each owned upgrade's info cost (not modes); give_recycle_reward drops knowledge and scales it */
void building_upgrades_cost_sum(const Perm *p, Materials *m) {
  if (!p || p->cls >= CLS_COUNT) return;
  const ClassUpgrades *cu = &class_upgrades[p->cls];
  for (int k = 0; k < 3 && cu->up[k]; k++)
    if (p->upgrades >> k & 1)
      for (int i = 0; i < M_COUNT; i++) m->m[i] += upgrade_info[cu->up[k] - 1].cost[i];
}
/* the story's questions: buildings that have a building upgrade, and whether a city upgrade was bought */
int upgrades_buildings_with(const char *name) {
  int u = upgrade_find(name), n = 0;
  if (u < 0) return 0;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && is_building(&perms[i]) && in_upgrades(&perms[i], u)) n++;
  return n;
}
bool upgrades_city_bought(const char *name) {
  int u = upgrade_find(name);
  return u >= 0 && bit(u);
}

/* ---------------------------------------------------------------- start and load */
void upgrades_new_city(void) { memset(&city_upgrades, 0, sizeof city_upgrades); }
/* CityUpgrades.load and Policies.load add each one again on fresh variables: the city upgrades' variables go
 * back to their defaults and each effect is made again (the old saves' SlimyLiving fix is for version 11
 * saves, never ours) */
void upgrades_after_load(void) {
  upgrades.computerChipFactorySpeed = 1;
  upgrades.recyclingAmount = 0;
  upgrades.hasBirthControl = upgrades.starDatabaseIsMoreEffective = upgrades.stoneTeleporterHasBigStones = false;
  upgrades.hasGalacticLibrary = upgrades.hasConnectedMachines = upgrades.hasGeneEditing = false;
  upgrades.stoneTeleporterChanceForRefinedMetals = upgrades.extraEcoFarmFoodToPlot1 = upgrades.extraEcoFarmFoodToPlot2 = 0;
  upgrades.extendAgeBy = 0;
  recycling_amount = 0;
  for (int u = 0; u < U_COUNT; u++) {
    if (!bit(u)) continue;
    if (upgrade_info[u].kind == UK_CITY_UPGRADE) city_upgrade_effect(u, true);
    else if (upgrade_info[u].kind == UK_POLICY) policy_effect(u, true);
  }
}
