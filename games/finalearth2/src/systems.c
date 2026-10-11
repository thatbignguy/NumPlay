/* The Final Earth 2: city-wide systems (simulation.Stats, Eating,
 * BabyMaker, Happiness, HappinessBoost, OperatingCost, BoostManager). */
#include "bld.h"
#include "save.h"
#include "scratch.h"
#include "classes.h"
#include "story.h"
#include "unlocks.h"

Bonuses bonuses;
static void glory_update(void);

/* ---------------------------------------------------------------- Stats */
Stats stats;
static float stats_update_in = -1;

void stats_add_production(int m, double amount) { stats.materialProduction[m][0] += amount; }
void stats_add_used(int m, double amount) { stats.materialUsed[m][0] += amount; }
void mat_add(int m, double amount) {
  city.materials.m[m] += amount;
  stats_add_production(m, amount);
}
bool mat_take(int m, double amount) {
  if (city.materials.m[m] < amount) return false;
  city.materials.m[m] -= amount;
  stats_add_used(m, amount);
  return true;
}
bool mat_can_afford(const float cost[M_COUNT]) {
  for (int i = 0; i < M_COUNT; i++)
    if (city.materials.m[i] < cost[i]) return false;
  return true;
}
void mat_pay(const float cost[M_COUNT]) {
  for (int i = 0; i < M_COUNT; i++) city.materials.m[i] -= cost[i];
}
void stats_on_commute(float minutes) { stats.lastMorningCommute = minutes; }

void stats_update(float timeMod) {
  if (stats_update_in <= 0) {
    stats.laborForce = stats.peopleWithAJob = stats.peopleWithHome = stats.children = 0;
    stats.houseCapacity = stats.peopleWorkingAtLabs = stats.jobs = 0;
    stats.people = ncits;
    eating.herbsCap = 0;
    for (int i = 1; i < MAX_CITS; i++) {
      Cit *c = &cits[i];
      if (!(c->flags & CF_ALIVE)) continue;
      if (cit_age(c) >= 16) {
        stats.laborForce++;
        if (c->job) {
          stats.peopleWithAJob++;
          if (bld_is_lab(P(c->job))) stats.peopleWorkingAtLabs++;
        }
      } else {
        stats.children++;
      }
      if (c->home) stats.peopleWithHome++;
    }
    for (int i = 1; i < MAX_PERMS; i++) {
      Perm *p = &perms[i];
      if (!(p->flags & PF_USED)) continue;
      if (is_a(p, B_HOUSE)) stats.houseCapacity += bld_residents(p);
      if (is_a(p, B_WORK)) stats.jobs += bld_jobs(p);
      eating.herbsCap += bld_herb_capacity(p);
    }
    stats_update_in += 20;
    unlocks_check_stat_related();
  } else {
    stats_update_in -= timeMod > 1 ? timeMod : 1;
  }
}

void stats_midnight_update(void) {
  for (int m = 0; m < M_COUNT; m++) {
    for (int i = STATS_DAYS - 1; i >= 1; i--) {
      stats.materialProduction[m][i] = stats.materialProduction[m][i - 1];
      stats.materialUsed[m][i] = stats.materialUsed[m][i - 1];
    }
    stats.materialProduction[m][0] = stats.materialUsed[m][0] = 0;
  }
}

/* ---------------------------------------------------------------- Eating */
EatingState eating = {.totalConsumedFoodPerDay = -1};

void eating_update(float timeMod) {
  float time = city_hour();
  eating.currentFoodSaveByHerbsPct = eating.totalConsumedFoodPerDay != 0 ? fminf(1, eating.herbsCap / eating.totalConsumedFoodPerDay) : 0;
  float saved = 0.2f * fminf(eating.totalConsumedFoodPerDay, eating.herbsCap);
  eating.totalConsumedFoodPerDay = ncits - stats.children * 0.5f;
  eating.totalConsumedFoodPerDay -= saved;
  if (eating.foodRationing) eating.totalConsumedFoodPerDay *= 0.5f;
  double *food = &city.materials.m[M_FOOD];
  if (time >= 7 && time < 20) {
    float part = city.minutesPerTick * timeMod / 60 / (20 - 7);
    double wants = fmax(0, eating.totalConsumedFoodPerDay * part);
    double now = fmin(*food, wants);
    *food -= now;
    stats_add_used(M_FOOD, now);
    eating.foodShortage += (float)(wants - now);
    eating.foodShortage = fminf(eating.foodShortage, ceilf(eating.totalConsumedFoodPerDay / 2));
  }
  if (eating.foodShortage > 0 && *food > 0.1) {
    double r = fmin(*food, eating.foodShortage);
    eating.foodShortage -= (float)r;
    *food -= r;
    stats_add_used(M_FOOD, r);
    if (*food > 0) eating.foodShortage = 0;
  }
}

/* ---------------------------------------------------------------- BabyMaker */
BabyMakerState babies = {.softPopLimit = MAX_CITS - 1, .targetPopAmount = -1, .updateStatsIn = -2};
static float baby_capacity[MAX_WORLDS], babies_per_year[MAX_WORLDS], people_per_world[MAX_WORLDS];
static float total_baby_capacity;

static float baby_production(void) {
  if (happiness.happiness < 10 && stats.peopleWithHome < stats.people / 2 && city.materials.m[M_FOOD] < 1) return 0;
  if (happiness.happiness < 10) return 0.8f;
  float scale = powf((powf((float)ncits, 1.2f) + 500) / 1400, 0.4f);
  switch (babies.mode) {
  case 0: return 1 + happiness.happiness / 140 / scale;
  case 1: return babies.targetPopAmount == -1 ? 1 : ncits >= babies.targetPopAmount ? 0.1f : 3.0f;
  case 2: return 1 + 0.4f * (happiness.happiness / 140) / scale;
  default: return 1 + (happiness.happiness / 140 + fmaxf(0, (happiness.happiness - 60) / 120)) / scale;
  }
}
static void baby_update_stats(void) {
  memset(baby_capacity, 0, sizeof baby_capacity);
  memset(people_per_world, 0, sizeof people_per_world);
  float total = 0;
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE)) continue;
    float age = cit_age(c);
    float add = age >= 20 && age < 45 ? 1 : age >= 45 && age < 55 ? 1 - (age - 45) / 10 : 0;
    baby_capacity[c->onWorld] += add;
    if (age >= 20) people_per_world[c->onWorld] += 1;
    total += add;
  }
  float per = baby_production();
  for (int w = 0; w < nworlds; w++) {
    if (ncits > 300 && total < 0.1f * ncits && baby_capacity[w] < 0.1f * people_per_world[w]) baby_capacity[w] = 0.1f * people_per_world[w];
    babies_per_year[w] = baby_capacity[w] * per / 30.0f;
    total_baby_capacity += baby_capacity[w];
  }
}
static void create_baby(int w) {
  if (ncits == 0) return;
  /* a random citizen of that world aged 20 to 55 (else the first) */
  int n = 0;
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if ((c->flags & CF_ALIVE) && c->onWorld == w && cit_age(c) >= 20 && cit_age(c) < 55) n++;
  }
  Cit *parent = NULL;
  if (n) {
    int k = rnd_int(n);
    for (int i = 1; i < MAX_CITS && !parent; i++) {
      Cit *c = &cits[i];
      if ((c->flags & CF_ALIVE) && c->onWorld == w && cit_age(c) >= 20 && cit_age(c) < 55 && k-- == 0) parent = c;
    }
  } else {
    for (int i = 1; i < MAX_CITS && !parent; i++)
      if (cits[i].flags & CF_ALIVE) parent = &cits[i];
  }
  cit_create(parent->onWorld, 0, parent->inPermanent, (int)parent->relativeX);   /* (createCitizen: a hobby maybe) */
}
void baby_update(float timeMod) {
  if (babies.updateStatsIn <= 0) {
    babies.updateStatsIn += 60;
    baby_update_stats();
  }
  babies.updateStatsIn -= timeMod;
  float years = city.minutesPerTick * timeMod / 60 / 24;
  for (int w = 0; w < nworlds; w++)
    if (ncits + spawners_pending() < babies.softPopLimit && ncits >= 2 && baby_capacity[w] >= 2 && rnd_float() < babies_per_year[w] * years)
      create_baby(w);
}
void baby_on_citizen_death(void) {
  if (ncits != babies.softPopLimit) return;
  if (total_baby_capacity == 0) baby_update_stats();
  if (total_baby_capacity == 0) return;
  float r = rnd_float();
  int w = 0;
  for (int i = 0; i < nworlds; i++) {
    r -= baby_capacity[i] / total_baby_capacity;
    if (r <= 0) w = i;
  }
  create_baby(w);
}

/* ---------------------------------------------------------------- Happiness */
#define MAX_BOOSTS 8
static struct { double until; float boost; bool canGoOverMax; uint8_t text; } boosts[MAX_BOOSTS];
static int nboosts;
static float update_happiness_in;

void happiness_add_boost(double until, float boost, bool over_max, uint8_t text) {
  if (nboosts < MAX_BOOSTS) boosts[nboosts++] = (typeof(boosts[0])){until, boost, over_max, text};
}
/* happinessBoosts, for the happiness window and the panel's tooltip */
int happiness_boosts(void) { return nboosts; }
float happiness_boost(int i, uint8_t *text) {
  *text = boosts[i].text;
  return boosts[i].boost;
}
/* saved with the city (HappinessBoost.save: until, boost, its text, canGoOverMax) */
void boosts_save(SaveIO *io) {
  save_u8(io, (uint8_t)nboosts);
  for (int i = 0; i < nboosts; i++) {
    save_f64(io, boosts[i].until);
    save_f32(io, boosts[i].boost);
    save_u8(io, boosts[i].text);
    save_u8(io, boosts[i].canGoOverMax);
  }
}
bool boosts_load(SaveIO *io) {
  int n = load_u8(io);
  if (n > MAX_BOOSTS) return false;
  nboosts = 0;
  for (int i = 0; i < n; i++) {
    double until = load_f64(io);
    float boost = load_f32(io);
    uint8_t text = load_u8(io);
    happiness_add_boost(until, boost, load_u8(io) != 0, text);
  }
  return true;
}

/* the entertainment, medical care and citizens of each group of worlds */
static float actual_happiness(void) {
  static float ent_per_group[MAX_WORLDS], cits_per_group[MAX_WORLDS];
  int ngroups = 0;
  for (int w = 0; w < nworlds; w++)
    if (worlds[w].group + 1 > ngroups) ngroups = worlds[w].group + 1;
  memset(ent_per_group, 0, sizeof ent_per_group);
  memset(cits_per_group, 0, sizeof cits_per_group);
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *p = &perms[i];
    if (!(p->flags & PF_USED)) continue;
    ent_per_group[worlds[p->world].group] += bld_entertainment_capacity(p);
  }
  happiness.homeHappiness = happiness.schoolHappiness = happiness.purposeHappiness = 0;
  int kids = 0;
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE)) continue;
    if (c->home) happiness.homeHappiness += bld_attractiveness(P(c->home));
    if (c->job) happiness.purposeHappiness += 100;
    else if (cit_age(c) < 16) {
      kids++;
      if (c->school) happiness.schoolHappiness += 100, happiness.purposeHappiness += 100;
    }
    cits_per_group[worlds[c->onWorld].group] += 1;
  }
  if (ncits > 0) happiness.homeHappiness /= ncits, happiness.purposeHappiness /= ncits;
  if (kids > 0) happiness.schoolHappiness /= kids;
  else {
    happiness.schoolHappiness = 0;
    for (int i = 1; i < MAX_PERMS; i++)
      if ((perms[i].flags & PF_USED) && is_a(&perms[i], B_SCHOOL) && perms[i].nworkers >= 1) { happiness.schoolHappiness = 100; break; }
  }
  memset(happiness.perEntertainmentType, 0, sizeof happiness.perEntertainmentType);
  happiness.entertainmentHappiness = happiness.medicalHappiness = 0;
  memset(happiness.healthCapacityByType, 0, sizeof happiness.healthCapacityByType);
  happiness.numberOfGroupsWithCitizens = 0;
  uint16_t *list = scratch->u.happy.list;
  for (int g = 0; g < ngroups; g++) {
    if (cits_per_group[g] == 0) continue;
    happiness.numberOfGroupsWithCitizens++;
    float this_ent = 32.999999999999993f * fminf(ent_per_group[g], cits_per_group[g]);
    /* each of the five entertainment types: the best buildings first, as many citizens as they hold */
    for (int t = 0; t < 5; t++) {
      int n = 0;
      for (int i = 1; i < MAX_PERMS; i++) {
        Perm *p = &perms[i];
        if (!(p->flags & PF_USED) || !(p->flags & PF_IS_ENTERTAINMENT) || worlds[p->world].group != g) continue;
        if (ent_type(p) == t || (bld_secondary_entertainment_types(p) >> t & 1)) list[n++] = (uint16_t)i;
      }
      for (int a = 1; a < n; a++) {   /* by quality, best first (stable) */
        uint16_t k = list[a];
        int b = a - 1;
        while (b >= 0 && bld_entertainment_quality(P(list[b])) < bld_entertainment_quality(P(k))) list[b + 1] = list[b], b--;
        list[b + 1] = k;
      }
      float left = cits_per_group[g], part = 0;
      for (int a = 0; a < n; a++) {
        Perm *p = P(list[a]);
        float cap = bld_entertainment_capacity(p);
        float take = left > cap ? cap : left;
        part += take * bld_entertainment_quality(p);
        left -= take;
        if (left <= 0) break;
      }
      this_ent += part * (0.67f / 5);
      happiness.perEntertainmentType[t] += part / ncits;
    }
    happiness.entertainmentHappiness += this_ent / ncits;
    /* medical care */
    int n = 0, types = 0;
    for (int i = 1; i < MAX_PERMS; i++) {
      Perm *p = &perms[i];
      if ((p->flags & PF_USED) && (p->flags & PF_IS_MEDICAL) && !(p->flags & PF_IS_ENTERTAINMENT) && worlds[p->world].group == g) {
        list[n++] = (uint16_t)i;
        if (PC(p)->medicalTypeID + 1 > types) types = PC(p)->medicalTypeID + 1;
      }
    }
    for (int a = 1; a < n; a++) {
      uint16_t k = list[a];
      int b = a - 1;
      while (b >= 0) {
        Perm *x = P(list[b]), *y = P(k);
        float d = bld_medical_quality(y) - bld_medical_quality(x) + 0.01f * (PC(y)->medicalTypeID - PC(x)->medicalTypeID);
        if (d <= 0) break;
        list[b + 1] = list[b], b--;
      }
      list[b + 1] = k;
    }
    float used[10] = {0}, left = cits_per_group[g], part = 0;
    for (int a = 0; a < n; a++) {
      Perm *p = P(list[a]);
      int id = PC(p)->medicalTypeID;
      float take = left > bld_medical_capacity(p) ? bld_medical_capacity(p) : left;
      float val = PC(p)->medicalTypeLimit * cits_per_group[g] - used[id];
      if (val < 0) take = 0;
      else if (!(val > take)) take = val;
      used[id] += take;
      happiness.healthCapacityByType[id] += take;
      part += take * bld_medical_quality(p);
      left -= take;
      if (left <= 0) break;
    }
    happiness.medicalHappiness += part / ncits;
  }
  happiness.entertainmentHappiness = fminf(100, happiness.entertainmentHappiness + 0.01f);
  happiness.medicalHappiness = fminf(100, happiness.medicalHappiness + 0.01f);
  int special = 0;
  int sm = pcls_find("SpecializedMedicalClinic");
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == sm) special++;
  happiness.specializedMedicalHappiness = special == 0 ? 0 : fminf(100, fmaxf(0, (float)special / ncits * 1000 * 100));
  float pp = 0.1f, hp = 0.4f, mp = 0.1f, sp = 0.1f, ep = 0.3f;
  if (happiness.gloryOfTheKey) pp -= 0.02f, hp -= 0.02f, mp -= 0.02f, sp -= 0.02f, ep -= 0.02f;
  float h = happiness.homeHappiness * hp + happiness.entertainmentHappiness * ep + happiness.schoolHappiness * sp +
            happiness.medicalHappiness * mp + happiness.purposeHappiness * pp;
  if (happiness.gloryOfTheKey) {
    glory_update();
    h += 0.1f * glory_of_the_key_happiness();
  }
  h = fminf(100, h + 0.01f);
  happiness.actionSpeedModifierWithoutPenalties = h <= 50 ? 0.25f + h / 50 * 0.75f : 1 + (h - 50) / 50;
  if (eating.foodShortage > 0) {
    happiness.foodRationingUnhappiness = 0;
    if (happiness.actionSpeedModifierWithoutPenalties < 1) happiness.actionSpeedModifierWithoutPenalties = 1;
    if (eating.foodShortage >= 10) happiness.foodShortageUnhappiness = h, h = 0;
    else if (h > 10) happiness.foodShortageUnhappiness = h - 10, h = 10;
    else happiness.foodShortageUnhappiness = 0;
  } else {
    happiness.foodShortageUnhappiness = happiness.foodRationingUnhappiness = 0;
    if (h > 10 && eating.foodRationing) {
      float after = fmaxf(10, h * 0.67f);
      happiness.foodRationingUnhappiness = h - after;
      h = after;
    }
  }
  float day = (float)(city.timeSinceStart / 1440);
  if (policies.workTimeChange > 0) {
    happiness.lastOvertime = day;
    happiness.overtimeUnhappinessActual = fmaxf(happiness.overtimeUnhappinessActual, 5);
    if (happiness.lastOvertimeDay == -1) happiness.lastOvertimeDay = day;
    else if (day > happiness.lastOvertimeDay + 1) {
      happiness.lastOvertimeDay = day;
      happiness.overtimeUnhappinessActual = fminf(happiness.overtimeUnhappinessActual + 5, 50);
    }
    happiness.lastRestoreTime = -1;
  } else if (day > happiness.lastOvertimeDay + 1) {
    if (happiness.lastRestoreTime == -1) happiness.lastRestoreTime = day;
    else {
      float reduce = (day - happiness.lastRestoreTime) * 10;
      happiness.lastRestoreTime = day;
      happiness.overtimeUnhappinessActual = fmaxf(0, happiness.overtimeUnhappinessActual - reduce);
    }
    happiness.lastOvertimeDay = -1;
  }
  happiness.overtimeUnhappinessShown = 0;
  if (happiness.overtimeUnhappinessActual > 0) {
    happiness.overtimeUnhappinessShown = fminf(happiness.overtimeUnhappinessActual, h);
    h -= happiness.overtimeUnhappinessShown;
  }
  if (h < happiness.happinessEnthusiasmLevel) {
    happiness.enthusiasmHappiness = happiness.happinessEnthusiasmLevel - h;
    h = happiness.happinessEnthusiasmLevel;
  } else {
    happiness.enthusiasmHappiness = 0;
  }
  for (int i = 0; i < nboosts; i++)
    if (!boosts[i].canGoOverMax) h = fminf(100, h + boosts[i].boost);
  for (int i = 0; i < nboosts; i++)
    if (boosts[i].canGoOverMax) h += boosts[i].boost;
  return fmaxf(h, 0);
}

static void set_happiness(float h) {
  happiness.actionSpeedModifier = h <= 50 ? 0.25f + h / 50 * 0.75f : 1 + (h - 50) / 50;
  happiness.happiness = h;
}

void happiness_update(float timeMod) {
  for (int b = nboosts - 1; b >= 0; b--)
    if (city.timeSinceStart > boosts[b].until) {
      for (int k = b; k + 1 < nboosts; k++) boosts[k] = boosts[k + 1];
      nboosts--;
    }
  if (update_happiness_in <= 0) {
    happiness.actualHappiness = actual_happiness();
    update_happiness_in = 60;
  } else {
    update_happiness_in -= fmaxf(1, timeMod);
  }
  if (happiness.happiness == -1) set_happiness(happiness.actualHappiness);
  float change = 0.05f * timeMod;
  if (fabsf(happiness.actualHappiness - happiness.happiness) < change) set_happiness(happiness.actualHappiness);
  else set_happiness(happiness.happiness + (happiness.actualHappiness > happiness.happiness ? 1 : -1) * change);
  if (happiness.happiness >= 99.99f) {
    happiness.fullHappinessTime += timeMod * city.minutesPerTick;
    happiness.fullHappinessTimeout = 300;
  } else if (happiness.fullHappinessTimeout < 0) {
    happiness.fullHappinessTime = 0;
  } else {
    happiness.fullHappinessTimeout -= timeMod;
  }
  if (happiness.happiness <= 10.01f) {
    if (city_day() != happiness.lastShownVeryUnhappyWarning && happiness.veryUnhappyFromDay == -1) {
      happiness.veryUnhappyFromDay = city_day();
      happiness.lastShownVeryUnhappyWarning = city_day();
      if (!happiness.ignoreUnhappyCitizens) ui_pause_for_window();
    }
  } else {
    happiness.veryUnhappyFromDay = -1;
  }
}

/* ---------------------------------------------------------------- GloryOfTheKey (cityofthekey)
 * expensive homes, upscale restaurants and sculptures for the number of citizens */
static GloryOfTheKey glory;
float glory_of_the_key_happiness(void) { return glory.gloryOfTheKeyHappiness; }
const GloryOfTheKey *glory_of_the_key(void) { return &glory; }
static void glory_update(void) {
  int vil = pcls_find("Villa"), pent = pcls_find("FlowerPenthouse"), rest = pcls_find("LuxuryRestaurant");
  int s1 = pcls_find("StatueGarden"), s2 = pcls_find("StatueOfHappiness"), s3 = pcls_find("StatueOfTheKey");
  int houses = 0, restaurants = 0, sculptures = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    const Perm *p = &perms[i];
    if (!(p->flags & PF_USED)) continue;
    if (class_is(p->cls, vil) || class_is(p->cls, pent)) houses++;
    else if (class_is(p->cls, rest)) restaurants++;
    else if (class_is(p->cls, s1) || class_is(p->cls, s2) || class_is(p->cls, s3)) sculptures++;
  }
  float n = (float)ncits + 1;
  glory.highEndHomeHappiness = fminf(100, fmaxf(0, 16000.0f * houses / n));
  glory.expensiveRestaurantHappiness = fminf(100, fmaxf(0, 11000.0f * restaurants / n));
  glory.sculpturesHappiness = fminf(100, fmaxf(0, 5000.0f * sculptures / n));
  float sum = glory.highEndHomeHappiness + glory.expensiveRestaurantHappiness + glory.sculpturesHappiness;
  glory.gloryOfTheKeyHappiness = fminf(100, fmaxf(0, sum / 3 + 0.01f));
}

/* ---------------------------------------------------------------- OperatingCost (teleporters cost knowledge) */
static Materials op_cost;
static bool tp_user_enabled = true;
bool operating_cost_user_enabled(void) { return tp_user_enabled; }   /* (bld_transport.c's windows) */

void operating_cost_recalculate(void) {
  memset(&op_cost, 0, sizeof op_cost);
  if (!operating_cost.teleportersEnabled) return;
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *p = &perms[i];
    if ((p->flags & PF_USED) && is_building(p) && PC(p)->teleporterOperatingCost > 0)
      op_cost.m[M_KNOWLEDGE] += PC(p)->teleporterOperatingCost * (bld_misdirector() ? 0.5f : 1) * (1 - upgrades.rocketTeleporterReward);
  }
}
void operating_cost_set_teleporters(bool on, bool by_user) {
  if (on == operating_cost.teleportersEnabled) {
    if (by_user) tp_user_enabled = on;
    return;
  }
  if (!by_user && !tp_user_enabled && !operating_cost.teleportersEnabled) return;
  operating_cost.teleportersEnabled = on;
  if (by_user) tp_user_enabled = on;
  if (on) sim_flags |= SIM_HOUSES_CHANGED;
  operating_cost_recalculate();
}
void operating_cost_update(float timeMod) {
  if (city.materials.m[M_KNOWLEDGE] > 10 * timeMod && tp_user_enabled && !operating_cost.teleportersEnabled)
    operating_cost_set_teleporters(true, false);
  float f = timeMod / 1440 * city.minutesPerTick;
  bool afford = true;
  for (int m = 0; m < M_COUNT; m++)
    if (city.materials.m[m] < op_cost.m[m] * f) afford = false;
  if (afford) {
    for (int m = 0; m < M_COUNT; m++)
      if (op_cost.m[m] > 0) {
        city.materials.m[m] -= op_cost.m[m] * f;
        stats_add_used(m, op_cost.m[m] * f);
      }
  } else {
    operating_cost_set_teleporters(false, false);
  }
}

/* ---------------------------------------------------------------- the rest (filled in as they are ported) */
void boosts_update(float timeMod) { (void)timeMod; }   /* the boost needs an ad: never on the calculator */
void animals_update(float timeMod) { (void)timeMod; }
void story_update(float timeMod);
void unlocks_update(float timeMod);
/* GameProgress.update: the story, then the delayed unlock check */
void progress_update(float timeMod) {
  story_update(timeMod);
  unlocks_update(timeMod);
}
static const char *story_name = "";
void story_set_name(const char *n) { story_name = n; }
bool story_is(const char *name) { return !strcmp(story_name, name); }
/* (until progress.Unlocks and BuildingCost are ported) */
#include "build.h"
bool unique_not_unique(int cls) { (void)cls; return false; }
/* progress.BuildingCost.getBuildingCost: the Lost Ship's first house is cheaper (scrap from the ship) */
void building_cost_scale(int cls, Materials *cost) {
  if (cls != CLS_NormalHouse || !story_is("theLostShip") || !story_goal() || strcmp(story_goal()->name, "BuildIndoorFarmAndHouse"))
    return;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == CLS_NormalHouse) return;
  memset(cost, 0, sizeof *cost);
  cost->m[M_WOOD] = 2, cost->m[M_STONE] = 2;
}

void systems_reset(void) {
  memset(&stats, 0, sizeof stats);
  stats_update_in = -1;
  memset(&eating, 0, sizeof eating);
  eating.totalConsumedFoodPerDay = -1;
  memset(&babies, 0, sizeof babies);
  babies.softPopLimit = MAX_CITS - 1;
  babies.targetPopAmount = -1;
  babies.updateStatsIn = -2;
  total_baby_capacity = 0;
  memset(&happiness, 0, sizeof happiness);
  happiness.happiness = -1;
  happiness.actionSpeedModifier = happiness.actionSpeedModifierWithoutPenalties = 1;
  happiness.lastOvertimeDay = happiness.lastRestoreTime = -1;
  happiness.veryUnhappyFromDay = happiness.lastShownVeryUnhappyWarning = -1;
  nboosts = 0;
  memset(&glory, 0, sizeof glory);
  update_happiness_in = 0;
  memset(&op_cost, 0, sizeof op_cost);
  tp_user_enabled = true;
  operating_cost.teleportersEnabled = true;
  memset(&bonuses, 0, sizeof bonuses);
}

