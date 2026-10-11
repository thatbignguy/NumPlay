/* The Final Earth 2: what buildings do for citizens, and the city-wide
 * systems they touch (happiness, policies, research, operating costs). */
#ifndef FE_BLD_H
#define FE_BLD_H
#include <math.h>
#include <stdlib.h>
#include "sim.h"

/* the citizens' sprites (spr_children and spr_humans, 51 each, then Dracula and Waldo) and names */
#define NCITIZEN_SPRITES 53
#define NCITIZEN_NAMES 8913
#define NAME_WALDO 3730
#define NAME_DRACULA 8913

/* ---------------------------------------------------------------- city-wide state */
typedef struct {
  float happiness;              /* -1 until first computed */
  float actualHappiness;
  float actionSpeedModifier, actionSpeedModifierWithoutPenalties;
  float homeHappiness, schoolHappiness, purposeHappiness, entertainmentHappiness;
  float medicalHappiness, specializedMedicalHappiness;
  float perEntertainmentType[5];
  float healthCapacityByType[10];
  int numberOfGroupsWithCitizens;
  float foodShortageUnhappiness, foodRationingUnhappiness;
  float overtimeUnhappinessActual, overtimeUnhappinessShown, lastOvertime, lastOvertimeDay, lastRestoreTime;
  float enthusiasmHappiness, happinessEnthusiasmLevel;
  float fullHappinessTime, fullHappinessTimeout;
  int veryUnhappyFromDay, lastShownVeryUnhappyWarning;
  bool ignoreUnhappyCitizens;
  bool gloryOfTheKey;
} HappinessState;
extern HappinessState happiness;
/* a HappinessBoost's text: HB_* (the user interface says it in the original's words) */
enum { HB_MUSIC_FESTIVAL, HB_CITIZEN_GRATEFULNESS, HB_HAUNTED_LABS };
void happiness_add_boost(double until, float boost, bool over_max, uint8_t text);
int happiness_boosts(void);                       /* the boosts running */
float happiness_boost(int i, uint8_t *text);       /* boost i: its amount and its text (HB_*) */
/* simulation.GloryOfTheKey (cityofthekey): its happiness and the three parts it is the mean of */
typedef struct {
  float gloryOfTheKeyHappiness, highEndHomeHappiness, expensiveRestaurantHappiness, sculpturesHappiness;
} GloryOfTheKey;
float glory_of_the_key_happiness(void);
const GloryOfTheKey *glory_of_the_key(void);

#define STATS_DAYS 7
typedef struct {
  int people, jobs, houseCapacity, peopleWithHome, peopleWithAJob, children, laborForce, peopleWorkingAtLabs;
  double materialProduction[M_COUNT][STATS_DAYS], materialUsed[M_COUNT][STATS_DAYS];   /* today first */
  float lastMorningCommute;
} Stats;
extern Stats stats;
void stats_add_production(int m, double amount);
void stats_add_used(int m, double amount);
/* the city's materials, recording the stats (Materials setters) */
void mat_add(int m, double amount);       /* produced */
bool mat_take(int m, double amount);      /* used, if there is enough */
bool mat_can_afford(const float cost[M_COUNT]);
void mat_pay(const float cost[M_COUNT]); /* (no stats, like building costs) */

typedef struct {
  bool foodRationing;
  float currentFoodSaveByHerbsPct, herbsCap, foodShortage, totalConsumedFoodPerDay;
} EatingState;
extern EatingState eating;

typedef struct {
  int softPopLimit, mode, targetPopAmount;
  float updateStatsIn;
} BabyMakerState;
extern BabyMakerState babies;
int spawners_pending(void);
void unlocks_check_stat_related(void);
void ui_pause_for_window(void);
void operating_cost_recalculate(void);
void operating_cost_set_teleporters(bool on, bool by_user);

typedef struct {
  float workTimeChange;        /* policies.vars.workTimeChange (MandatoryOvertime) */
  float schoolMaxEdu, schoolClassSizeMod;   /* SmallerClasses (1 and 1 until bld_new_city) */
} PoliciesVars;
extern PoliciesVars policies;

typedef struct {
  float extendAgeBy;           /* upgrades.vars.extendAgeBy */
  float rocketTeleporterReward;
  /* cityUpgrades.UpgradeVars, set by bld_new_city and the city upgrades (upgrades.c) */
  float stoneMiningSpeed, computerChipFactorySpeed, recyclingAmount, stoneTeleporterChanceForRefinedMetals;
  float extraEcoFarmFoodToPlot1, extraEcoFarmFoodToPlot2, superComputerUpgrade, fasterRocketFuelFactory;
  float nightClubEntertainmentQualityBonus;
  uint16_t stoneResearchCenterWithFossils;   /* the building, 0 none */
  bool hasBirthControl, starDatabaseIsMoreEffective, stoneTeleporterHasBigStones, advancedBuildingAllowed;
  bool hasGalacticLibrary, hasConnectedMachines, hasGeneEditing;
} UpgradesVars;
extern UpgradesVars upgrades;

typedef struct {
  bool teleportersEnabled;
} OperatingCostState;
extern OperatingCostState operating_cost;

void stats_on_commute(float minutes);
void baby_on_citizen_death(void);
bool story_is(const char *name);

/* ---------------------------------------------------------------- what a building is and does */
float bld_start_time(const Perm *p);
float bld_end_time(const Perm *p);
float bld_school_start(const Perm *p);
float bld_school_end(const Perm *p);
int bld_jobs(const Perm *p);
int bld_residents(const Perm *p);
int bld_workers(const Perm *p);
int bld_student_capacity(const Perm *p);
int bld_fixed_capacity_for_workers(const Perm *p);
float bld_attractiveness(const Perm *p);            /* get_attractiveness (base + bonus) */
float bld_years_to_live_longer(const Perm *p);
bool bld_has_private_teleporter(const Perm *p);
bool bld_keeps_residents(const Perm *p);           /* HouseOfTheKeyFriends */
uint16_t bld_first_building_to_go_to(Perm *p);
float bld_door_x(const Perm *p);                   /* doorX (mirrored buildings move it) */
void bld_work(Perm *p, Cit *c, float timeMod, bool shouldStopWorking);
void bld_walk_around(Perm *p, Cit *c, float stepsInBuilding);
void bld_be_at_school(Perm *p, Cit *c, float timeMod);
void bld_after_give_job(Perm *p, Cit *c);
/* transport */
float bld_teleport_x(const Perm *p);
void bld_teleport_used(Perm *p);
void bld_teleport_arrived(Perm *p);
bool bld_elevator_open_door(Perm *p);
void bld_elevator_used(Perm *p);
void bld_elevator_arrived(Perm *p);
float bld_flying_distance(const Perm *a, const Perm *b);   /* estimatedFlyingDistanceTo */
bool flying_board(Cit *c, Perm *in, Perm *dest);   /* a landing site's step: false if the path was dropped */
uint16_t bld_misdirector(void);
bool bld_is_misdirector(const Perm *p);
float bld_misdirect_chance(void);
void bld_misdirector_reward(Perm *p);

bool bld_is_lab(const Perm *p);                    /* Laboratory, ExperimentationLab, GrapheneLab (stats) */
float bld_herb_capacity(const Perm *p);
float bld_entertainment_capacity(const Perm *p);   /* base + bonus (any permanent) */
int bld_secondary_entertainment_types(const Perm *p);
float bld_entertainment_quality(const Perm *p);
float bld_medical_quality(const Perm *p);
float bld_medical_capacity(const Perm *p);

/* entertainment buildings */
bool ent_is_open(const Perm *p);
bool ent_is_open_for_existing_visitors(const Perm *p);
int ent_type(const Perm *p);
float ent_min_time(const Perm *p), ent_max_time(const Perm *p);       /* hours */
float ent_min_satisfy(const Perm *p), ent_max_satisfy(const Perm *p); /* days */
bool ent_finish(Perm *p, Cit *c, float timeMod);
void ent_be_entertained(Perm *p, Cit *c, float timeMod);

/* hobbies (simulation.citizenSpecialActions) */
bool hobby_is_active(const Cit *c);
void hobby_update(Cit *c, float timeMod);
void hobby_on_die(Cit *c);

/* the buildings' part of the city (bld*.c): bld_init once at start (it registers the closures);
 * bld_new_city when a city is made (before its worlds); bld_after_load after a save is read */
void bld_init(void);
void bld_new_city(void);
void bld_after_load(void);
void bld_city_changed(bool invalidate);           /* updatePathfinder: onCityChange and friends (sim.c) */
void bld_on_lose_job(Perm *job, Cit *c);           /* before c leaves job's workers (citizen.c) */
void bld_customize(Perm *p, int decor, bool mirrored);   /* a story stack's look: decoration (-1 none), mirrored */
void saucer_spawn(int world, int amount, int ageMin, int ageMax, int toCol);   /* SpawnFlyingSaucer */
/* window buttons the user interface answers itself (a class's window passes them to win_button; pressing one
 * calls window_action with it, which ignores it): */
enum {
  WA_UI_BIRTH_CONTROL = 100,   /* MedicalClinic: "Manage Birth Control" window */
  WA_UI_CITY_NAME,             /* CityHall: "Change City Name" */
  WA_UI_REPLACE_HOUSES,        /* LivingResearchCenter: "Upgrade Basic Houses" (the house upgrade window) */
  WA_UI_MOVE_RUINS,            /* explored alien ruins: pick an empty column, then bld_move_resource */
  WA_UI_COLOR_PICKER,          /* a tinted decoration: pick a colour, then bld_customize_color */
  WA_UI_DUPLICATE              /* a tinted decoration: build another like it (bld_customize, bld_customize_color) */
};
/* a class's own tryDestroy: DR_OK, or DR_PROTECTED or DR_REFUSED (build.h) with the window's words in why */
int bld_destroy_check(const Perm *p, const char **why);
void bld_customize_color(Perm *p, int32_t rgb);   /* customizeColor: 0xRRGGBB, -1 none */
int32_t bld_decoration_color(const Perm *p);       /* currentRecolor, -1 none */
int bld_decoration_look(const Perm *p, bool *mirrored);   /* currentTexture and mirrored (Duplicate) */
uint16_t bld_move_resource(uint16_t id, int world, int col);   /* a world resource to an empty column (its new id) */
void resource_planted(Perm *p, float materials_left, float regrow);   /* a forest or rock from the build menu */
/* GUI.showSimpleWindow(text, null, true): the open windows give way to a text (the user interface's) */
void ui_show_simple(const char *text);
/* a building's mission as a goal (getGlobalGoal: hackers, Secret Society, hippies): false if none */
bool bld_global_goal(const Perm *p, const char **category, const char **text);
/* the music festival (simulation.festival): while it runs, sim.c updates citizens with festival_citizen */
bool festival_running(void);
void festival_citizen(Cit *c, float timeMod);
/* drawing what belongs to no permanent on screen (flying saucers and craft, hobbies' props): at the end of
 * each layer's band function, after the permanents (view.c) */
void bld_draw_free(const GfxBand *b, int layer);
/* how far a permanent may draw outside its 20x20 cell (pipes, flowers on roofs), in city pixels */
#define BLD_DRAW_ABOVE 12
#define BLD_DRAW_BELOW 12

/* assign.c */
void assign_jobs(void);
void assign_houses(void);
void assign_schools(void);
void finder_pre_update(void);
void finder_invalidate(void);
#endif
