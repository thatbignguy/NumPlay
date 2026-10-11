/* The Final Earth 2: the base classes' behaviour (Permanent, Building,
 * House, Work, School and the entertainment and medical buildings'
 * interfaces), dispatched to a class's own functions where it has them; the
 * city-wide hooks the simulation calls (city changes, jobs), and the
 * buildings' part of starting, saving and loading a city. */
#include "bldi.h"
#include "save.h"

HappinessState happiness = {.happiness = -1, .actionSpeedModifier = 1, .actionSpeedModifierWithoutPenalties = 1,
                            .lastOvertimeDay = -1, .lastRestoreTime = -1, .veryUnhappyFromDay = -1, .lastShownVeryUnhappyWarning = -1};
PoliciesVars policies = {.schoolMaxEdu = 1, .schoolClassSizeMod = 1};
UpgradesVars upgrades = {.stoneMiningSpeed = 1, .computerChipFactorySpeed = 1};
OperatingCostState operating_cost = {true};
float bld_anim;
bool bld_secret_orchid, bld_secret_dance, bld_secret_hangar;

static inline const PermOps *OPS(const Perm *p) { return PC(p)->ops; }
#define HAS(p, f) (OPS(p) && OPS(p)->f)

/* ---------------------------------------------------------------- work */
float bld_start_time(const Perm *p) { return HAS(p, start_time) ? OPS(p)->start_time(p) : PC(p)->startTime; }
float bld_end_time(const Perm *p) { return HAS(p, end_time) ? OPS(p)->end_time(p) : PC(p)->endTime; }
int bld_jobs(const Perm *p) { return HAS(p, jobs) ? OPS(p)->jobs(p) : PC(p)->jobs; }
int bld_workers(const Perm *p) { return p->nworkers; }
uint16_t bld_first_building_to_go_to(Perm *p) {
  return HAS(p, first_building_to_go_to) ? OPS(p)->first_building_to_go_to(p) : pid(p);
}
/* Work.work: stop when it's time */
void bld_work(Perm *p, Cit *c, float timeMod, bool shouldStopWorking) {
  if (HAS(p, work)) {
    OPS(p)->work(p, c, timeMod, shouldStopWorking);
    return;
  }
  if (shouldStopWorking) c->currentAction = 2;
}
/* the job given (JobAssigner, after the building counts the new worker): its place in the workers' list */
void bld_after_give_job(Perm *p, Cit *c) {
  c->jobIndex = (uint8_t)(p->nworkers - 1);
  if (HAS(p, after_give_job)) OPS(p)->after_give_job(p, c);
  if (is_a(p, B_SCHOOL)) sim_flags |= SIM_SCHOOLS_CHANGED;
}
/* c leaves job's workers: those after it move up a place (HxOverrides.remove) */
void bld_on_lose_job(Perm *job, Cit *c) {
  if (!job) return;
  uint16_t j = pid(job);
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *o = &cits[i];
    if (o != c && (o->flags & CF_ALIVE) && o->job == j && o->jobIndex > c->jobIndex) o->jobIndex--;
  }
}

/* ---------------------------------------------------------------- homes */
int bld_residents(const Perm *p) {
  int n = HAS(p, residents) ? OPS(p)->residents(p) : PC(p)->residents;
  return n ? n + upgrades_residents_bonus(p->cls) : 0;
}
/* get_attractiveness: base (a home's info.quality, which city upgrades raise) and bonus (its upgrades and
 * neighbours) */
float bld_attractiveness(const Perm *p) {
  float base;
  if (HAS(p, attractiveness)) base = OPS(p)->attractiveness(p);
  else if (is_a(p, B_HOUSE)) base = (float)PC(p)->quality + upgrades_quality_bonus(p->cls);
  else base = 50;
  return base + p->bonusAttractiveness;
}
float bld_years_to_live_longer(const Perm *p) { return HAS(p, years_to_live_longer) ? OPS(p)->years_to_live_longer(p) : 0; }
bool bld_has_private_teleporter(const Perm *p) { return HAS(p, has_private_teleporter) && OPS(p)->has_private_teleporter(p); }
/* WorkWithHome: its home is for its workers */
int bld_fixed_capacity_for_workers(const Perm *p) {
  if (HAS(p, fixed_capacity_for_workers)) return OPS(p)->fixed_capacity_for_workers(p);
  return is_a(p, B_HOUSE) && is_a(p, B_WORK) ? bld_residents(p) : 0;
}
bool bld_keeps_residents(const Perm *p) { return p->cls == CLS_HouseOfTheKeyFriends; }

/* House.walkAround */
void bld_walk_around(Perm *p, Cit *c, float steps) {
  if (HAS(p, walk_around)) {
    OPS(p)->walk_around(p, c, steps);
    return;
  }
  int r = rnd_int(4);
  if (r == 0 && steps > 120) {
    cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0);
  } else if (r == 1) {
    int x = rnd_between(3, 7);
    cit_move_and_wait(c, x, rnd_between(30, 60), THEN_NONE, 0, false, false);
  } else if (r == 2) {
    int x = rnd_between(12, 16);
    cit_move_and_wait(c, x, rnd_between(30, 60), THEN_NONE, 0, false, false);
  } else {
    wait_ticks(c, 90, 120);
  }
}

/* ---------------------------------------------------------------- schools */
float bld_school_start(const Perm *p) { (void)p; return 7; }
float bld_school_end(const Perm *p) { (void)p; return 16; }
int bld_student_capacity(const Perm *p) { return HAS(p, student_capacity) ? OPS(p)->student_capacity(p) : 0; }
void bld_be_at_school(Perm *p, Cit *c, float timeMod) {
  if (HAS(p, be_at_school)) OPS(p)->be_at_school(p, c, timeMod);
}
int student_index(const Perm *school, const Cit *c) {
  int n = 0;
  uint16_t s = pid(school);
  for (int i = 1; i < cid(c); i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].school == s) n++;
  return n;
}

/* ---------------------------------------------------------------- entertainment */
bool ent_is_open(const Perm *p) { return HAS(p, is_open) ? OPS(p)->is_open(p) : true; }
bool ent_is_open_for_existing_visitors(const Perm *p) {
  return HAS(p, is_open_for_existing_visitors) ? OPS(p)->is_open_for_existing_visitors(p) : ent_is_open(p);
}
int ent_type(const Perm *p) { return HAS(p, entertainment_type) ? OPS(p)->entertainment_type(p) : PC(p)->entType; }
float ent_min_time(const Perm *p) { return PC(p)->entMinTime; }
float ent_max_time(const Perm *p) { return PC(p)->entMaxTime; }
float ent_min_satisfy(const Perm *p) { return PC(p)->entMinSatisfy; }
float ent_max_satisfy(const Perm *p) { return PC(p)->entMaxSatisfy; }
bool ent_finish(Perm *p, Cit *c, float timeMod) {
  return HAS(p, finish_entertainment) ? OPS(p)->finish_entertainment(p, c, timeMod) : true;
}
void ent_be_entertained(Perm *p, Cit *c, float timeMod) {
  if (HAS(p, be_entertained)) OPS(p)->be_entertained(p, c, timeMod);
}

bool bld_is_lab(const Perm *p) { return p->cls == CLS_Laboratory || p->cls == CLS_ExperimentationLab; }
float bld_herb_capacity(const Perm *p) { return HAS(p, herb_capacity) ? OPS(p)->herb_capacity(p) : 0; }
float bld_entertainment_capacity(const Perm *p) {
  return (HAS(p, entertainment_capacity) ? OPS(p)->entertainment_capacity(p) : 0) + p->bonusEntertainmentCapacity;
}
int bld_secondary_entertainment_types(const Perm *p) { return HAS(p, secondary_entertainment_types) ? OPS(p)->secondary_entertainment_types(p) : 0; }
float bld_entertainment_quality(const Perm *p) { return HAS(p, entertainment_quality) ? OPS(p)->entertainment_quality(p) : PC(p)->entQuality; }
float bld_medical_quality(const Perm *p) { return HAS(p, medical_quality) ? OPS(p)->medical_quality(p) : 0; }
float bld_medical_capacity(const Perm *p) { return HAS(p, medical_capacity) ? OPS(p)->medical_capacity(p) : 0; }

/* ---------------------------------------------------------------- adjacent effects */
float bld_effect(const Perm *p, int effect) {
  if (!is_building(p)) return 0;
  switch (effect) {
  case EF_FARM:
    if (p->cls == CLS_IndoorFarm || p->cls == CLS_EcoFarm || p->cls == CLS_ExperimentalFarm) return 1;
    if (p->cls == CLS_FarmHouse) return farmhouse_farm_effects(p);
    return 0;
  case EF_ALIEN_HOUSE: return p->cls == CLS_AlienHouse ? 1 : 0;
  case EF_COMMUNITY:
    return p->cls == CLS_ModernRusticHome || p->cls == CLS_ParkPod || class_is(p->cls, CLS_Park) ||
                   class_is(p->cls, CLS_BotanicalGardens) || p->cls == CLS_RooftopPark
               ? 1
               : 0;
  case EF_CROPS: return p->cls == CLS_Laboratory && has_upgrade_bit(p, UB_Laboratory_FarmingResearch) ? 7.5f : 0;
  case EF_TREE_GROWTH: return p->cls == CLS_Laboratory && has_upgrade_bit(p, UB_Laboratory_TreePlantationResearch) ? 0.2f : 0;
  case EF_RESTAURANT_CAP: return p->cls == CLS_HerbGarden && p->mode == MODE_HerbGarden_CulinaryHerbs ? 10 : 0;
  }
  return 0;
}
float adj_effect_lr(const Perm *p, int effect) { return bld_effect(P(p->left), effect) + bld_effect(P(p->right), effect); }
float adj_effect(const Perm *p, int effect) {
  return bld_effect(P(p->left), effect) + bld_effect(P(p->right), effect) + bld_effect(P(p->top), effect) +
         bld_effect(P(p->bottom), effect);
}

/* ---------------------------------------------------------------- citizens' small moves */
/* the original's steps inline in work(): toward x at the walking speed, no path */
void step_to_x(Cit *c, float x, float timeMod) {
  float spd = c->pathWalkSpeed / 100.0f * timeMod;
  if (fabsf(x - c->relativeX) < spd) c->relativeX = x;
  else c->relativeX += (x > c->relativeX ? 1 : -1) * spd;
}
void set_rel(Cit *c, float x, float y) { c->relativeX = x, c->relativeY = y; }
void wait_ticks(Cit *c, int tmin, int tmax) { cit_wait(c, rnd_between(tmin, tmax), THEN_NONE, 0); }
void move_x(Cit *c, int x) { cit_move(c, x, THEN_NONE, 0, false); }
void move_x_slow(Cit *c, int x) { cit_move(c, x, THEN_NONE, 0, true); }
/* educationLevel = max(min(edu + amount, cap), edu), once per visit */
bool educate(Cit *c, float amount, float cap) {
  if (c->flags & CF_BUILDING_INITED) return false;
  float e = c->educationLevel + amount;
  if (e > cap) e = cap;
  if (e > c->educationLevel) c->educationLevel = e;
  c->flags |= CF_BUILDING_INITED;
  return true;
}
/* out of its permanent, at the door (a building) or where it stands (a world resource) */
void leave_to_surface(Cit *c) { cit_instant_leave_permanent(c); }

/* ---------------------------------------------------------------- transport (bld_transport.c) */
float bld_teleport_x(const Perm *p) { return transport_teleport_x(p); }
void bld_teleport_used(Perm *p) { transport_teleport_used(p); }
void bld_teleport_arrived(Perm *p) { transport_teleport_arrived(p); }
bool bld_elevator_open_door(Perm *p) { return transport_elevator_open_door(p); }
void bld_elevator_used(Perm *p) { transport_elevator_used(p); }
void bld_elevator_arrived(Perm *p) { transport_elevator_arrived(p); }
float bld_flying_distance(const Perm *a, const Perm *b) {
  return (fabsf((float)(perm_city_x(a) - perm_city_x(b))) + fabsf((float)(perm_city_y(a) - perm_city_y(b)))) / 3;
}

/* ---------------------------------------------------------------- the city changes (CitySimulation.updatePathfinder) */
static void invalidate_pathfinding_info(Perm *p) {
  if (p->cls == CLS_WoodcuttingCentre || p->cls == CLS_StoneMine) ST(BlueCollarState, p)->nearest = NONE;
  else if (p->cls == CLS_ExplorationCentre) ST(ExplorationState, p)->nearestRuins = NONE;
}
void finder_request_limiting(bool strict_first);
void bld_city_changed(bool invalidate) {
  if (invalidate) {
    for (int i = 1; i < MAX_PERMS; i++)
      if (perms[i].flags & PF_USED) invalidate_pathfinding_info(&perms[i]);
    finder_request_limiting(true);
  }
  memset(world_unavailable, 0, sizeof world_unavailable);   /* World.onCityChange */
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *p = &perms[i];
    if ((p->flags & PF_USED) && HAS(p, on_city_change)) OPS(p)->on_city_change(p);
  }
  operating_cost_recalculate();
}

/* Building.onBuild (after World.build put it in its stack) */
void bld_on_build(Perm *p) {
  if (HAS(p, on_build)) OPS(p)->on_build(p);
}
/* postCreate (after City.onBuildBuilding linked it): the classes that have one */
void garden_post_create(Perm *p);
void redwood_post_create(Perm *p);
void hall_post_create(Perm *p);
void stone_center_post_create(Perm *p);
void bld_post_create(Perm *p) {
  if (!p) return;
  if (class_is(p->cls, CLS_BotanicalGardens)) garden_post_create(p);
  else if (class_is(p->cls, CLS_RedwoodTree)) redwood_post_create(p);
  else if (p->cls == CLS_HoloGameHall) hall_post_create(p);
  else if (p->cls == CLS_StoneResearchCenter) stone_center_post_create(p);
  else if (p->cls == CLS_CityHall) {
    if (p->mode >= 4) upgrade_first_mode(p);   /* (its first mode, if missing) */
  } else {
    decoration_post_create(p);   /* (the rooftop decorations only) */
  }
}

/* tryDestroy of the classes that refuse: the cityofthekey's first landing sites, redwoods holding a tree house up */
int bld_destroy_check(const Perm *p, const char **why) {
  *why = NULL;
  if (p->cls == CLS_LandingSite && story_is("cityofthekey") && (perm_city_x(p) == 100 || perm_city_x(p) == -100)) {
    *why = "The Key wouldn't want that.";
    return DR_PROTECTED;
  }
  if (class_is(p->cls, CLS_RedwoodTree)) {
    redwood_destroy_refusal(p, why);
    if (*why) return DR_REFUSED;
  }
  return DR_OK;
}

/* doorX: mirrored buildings move it */
float house_door_x(const Perm *p);
float bld_door_x(const Perm *p) {
  switch (p->cls) {
  case CLS_RooftopDecoration: case CLS_RooftopDecoration2: case CLS_RooftopDecoration3: case CLS_RooftopDecoration4:
    return decoration_door_x(p);
  default: return house_door_x(p);
  }
}

/* workers[index]: the worker at that place of the list (jobIndex), NULL if none */
Cit *worker_at(const Perm *p, int index) {
  uint16_t id = pid(p);
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].job == id && cits[i].jobIndex == index) return &cits[i];
  return NULL;
}
/* workers[0].currentAction == 0 (is_open tests): the buildings whose first worker works, found once a step */
static uint8_t first_working[(MAX_PERMS + 7) / 8];
static uint32_t first_working_step = 0xFFFFFFFFu, bld_step;
bool first_worker_working(const Perm *p) {
  if (first_working_step != bld_step) {
    memset(first_working, 0, sizeof first_working);
    for (int i = 1; i < MAX_CITS; i++) {
      const Cit *c = &cits[i];
      if ((c->flags & CF_ALIVE) && c->job && c->jobIndex == 0 && c->currentAction == 0)
        first_working[c->job >> 3] |= (uint8_t)(1 << (c->job & 7));
    }
    first_working_step = bld_step;
  }
  uint16_t id = pid(p);
  return p->nworkers > 0 && (first_working[id >> 3] >> (id & 7) & 1);
}

/* World.build and City.onBuildBuilding for a building the code makes itself (a redwood growing, a tree house):
 * no cost; replace: the permanent at (col, row) goes first */
uint16_t bld_build_at(int w, int col, int row, int cls, bool replace) {
  if (replace) {
    uint16_t old = perm_at(w, col, row);
    if (old) perm_destroy(old, true);
  }
  uint16_t id = perm_new(cls, w, col, row);
  if (!id) return NONE;
  bld_on_build(P(id));
  city_update_connections();
  sim_update_pathfinder(replace, id);
  unlocks_research(cls);
  bld_post_create(P(id));
  unlocks_check_build_related();
  return id;
}

/* ---------------------------------------------------------------- drawing */
void bld_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  if (HAS(p, draw)) {
    OPS(p)->draw(b, p, layer, x, y);
    return;
  }
  if (PC(p)->kind == KIND_BUILDING) {
    draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
    draw_upgrades(b, p, layer, x, y, false);
    draw_upgrades(b, p, layer, x, y, true);
  }
}
void bld_draw_free(const GfxBand *b, int layer) {
  flying_draw_free(b, layer);
  research_bot_draw_free(b, layer);
  hobby_draw_free(b, layer);
}

/* ---------------------------------------------------------------- the story's questions */
static Perm *first_of(int cls) {
  /* world by world, column by column (the original's permanents order for scenario worlds) */
  for (int w = 0; w < nworlds; w++)
    for (int x = 0; x < worlds[w].ncols; x++)
      for (int y = 0; y < col_stack_height(w, x); y++) {
        Perm *p = P(perm_at(w, x, y));
        if (p && p->cls == cls) return p;
      }
  return NULL;
}
float bld_alien_ruins_explored(void) {
  Perm *p = first_of(CLS_ComputerAlienRuins);
  return p ? ruins_explored(p) : 0;
}
float research_bot_time_working(const Perm *p);
float bld_research_bot_time(void) {
  Perm *p = first_of(CLS_ResearchBot);
  return p ? research_bot_time_working(p) : 0;
}
int hacker_mission(const Perm *p);
float bld_hacker_mission(void) {
  Perm *p = first_of(CLS_HackerHQ);
  return p ? (float)hacker_mission(p) : -1;
}
float decryptor_time_working(const Perm *p);
float bld_decryptor_time(void) {
  Perm *p = first_of(CLS_AlienDecryptor);
  return p ? decryptor_time_working(p) : 0;
}

/* ---------------------------------------------------------------- start, save and load */
uint8_t TH_CHANGE_FLOOR, TH_CHANGE_FLOOR_WAIT_30_60;
static void th_change_floor(Cit *c, uint16_t arg) { (void)arg; cit_change_floor(c, THEN_NONE, 0); }
static void th_change_floor_wait(Cit *c, uint16_t arg) { (void)arg; cit_change_floor_and_wait_random(c, 30, 60, THEN_NONE, 0); }

void bld_init(void) {
  static bool done;
  if (done) return;
  done = true;
  TH_CHANGE_FLOOR = then_register(th_change_floor);
  TH_CHANGE_FLOOR_WAIT_30_60 = then_register(th_change_floor_wait);
  bld_houses_init();
  bld_production_init();
  bld_services_init();
  bld_entertainment_init();
  bld_transport_init();
  bld_unique_init();
  bld_world_init();
  bld_hobbies_init();
}

/* a new city: the city-wide state of the buildings as the original's constructors make it */
void bld_new_city(void) {
  bld_init();
  memset(&upgrades, 0, sizeof upgrades);
  upgrades.stoneMiningSpeed = upgrades.computerChipFactorySpeed = 1;
  memset(&policies, 0, sizeof policies);
  policies.schoolMaxEdu = policies.schoolClassSizeMod = 1;
  bonuses.extraFoodFromFarms = 0;
  bonuses.machinePartsFactorySpeed = bonuses.labSpeed = bonuses.indoorFarmSpeed = bonuses.ecoFarmSpeed = 1;
  recycling_amount = 0;
  bld_anim = 0;
  eating.foodRationing = false;
  upgrades_new_city();
  saucers_new_city();
  ruins_new_city();
  hobbies_new_city();
  unique_new_city();
}

void bld_after_load(void) {
  ruins_after_load();
  hobbies_after_load();
  upgrades_after_load();
}

void saucers_save(SaveIO *io);
bool saucers_load(SaveIO *io);
void hobbies_save(SaveIO *io);
bool hobbies_load(SaveIO *io);
void unique_save(SaveIO *io);
bool unique_load(SaveIO *io);
/* what is not in the permanents (save.c calls these after the citizens) */
void bld_save(SaveIO *io) {
  save_u32(io, 0x424C4431u);   /* "BLD1" */
  save_bytes(io, &city_upgrades, sizeof city_upgrades);
  save_f32(io, recycling_amount);
  save_u8(io, (uint8_t)(bld_secret_orchid | bld_secret_dance << 1 | bld_secret_hangar << 2));
  /* the workers' places in their jobs' lists, citizen by citizen */
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].job) save_u8(io, cits[i].jobIndex);
  saucers_save(io);
  hobbies_save(io);
  unique_save(io);
}
bool bld_load(SaveIO *io) {
  if (load_u32(io) != 0x424C4431u) return false;
  load_bytes(io, &city_upgrades, sizeof city_upgrades);
  recycling_amount = load_f32(io);
  uint8_t secrets = load_u8(io);
  bld_secret_orchid = secrets & 1, bld_secret_dance = secrets >> 1 & 1, bld_secret_hangar = secrets >> 2 & 1;
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].job) cits[i].jobIndex = load_u8(io);
  if (!saucers_load(io) || !hobbies_load(io) || !unique_load(io)) return false;
  bld_after_load();
  return true;
}

/* once a simulation step, after the permanents (CitySimulation: buildingUpgradesToUpdate) */
void building_upgrades_update(float timeMod) {
  bld_anim += timeMod;
  bld_step++;
}
