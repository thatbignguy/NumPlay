/* The Final Earth 2: what the buildings' files (bld*.c, upgrades.c) share:
 * each class's own state in Perm.s, small helpers the original's classes
 * have in common, and the parts of each file the others call. */
#ifndef FE_BLDI_H
#define FE_BLDI_H
#include "bld.h"
#include "build.h"
#include "classes.h"
#include "data.h"
#include "story.h"
#include "unlocks.h"
#include "upgrades.h"
#include "win.h"
#include <stddef.h>

/* a class's state, laid over Perm.s (12 bytes, 2-aligned: packed) */
#define PACKED __attribute__((packed, may_alias))
#define ST(T, p) ((T *)(void *)(p)->s)
#define STC(T, p) ((const T *)(const void *)(p)->s)
#define STATE_FITS(T) _Static_assert(sizeof(T) <= sizeof(((Perm *)0)->s), #T " does not fit in Perm.s")

/* ---------------------------------------------------------------- small helpers */
static inline float hour_now(void) { return city_hour(); }
/* the original's time-of-day ranges, wrapping past midnight */
static inline bool hours_in(float t, float start, float end) {
  return start < end ? t >= start && t < end : t >= start || t < end;
}
static inline bool day_light(void) { return hours_in(hour_now(), 7, 20); }
static inline bool dim_light(void) { return hours_in(hour_now(), 6, 21); }
/* Config.earlyGameFix1: small cities produce faster */
static inline float early_fix(void) { return ncits < 30 ? 1.3f - 0.3f * (ncits / 60.0f) : 1; }
/* boostManager.currentGlobalBoostAmount: the ad boost, never on the calculator */
static inline float boost(void) { return 1; }
static inline int day_number(void) { return 1 + (int)((int64_t)city.timeSinceStart / 1440); }
static inline bool is_class(const Perm *p, int cls) { return p && (p->flags & PF_USED) && class_is(p->cls, cls); }
static inline bool has_upgrade_bit(const Perm *p, int bit) { return p->upgrades >> bit & 1; }

/* the Building's adjacent effects (getEffectsOfAdjecentBuildings) */
enum { EF_FARM, EF_ALIEN_HOUSE, EF_COMMUNITY, EF_CROPS, EF_TREE_GROWTH, EF_RESTAURANT_CAP };
float bld_effect(const Perm *p, int effect);      /* what p gives its neighbours */
float adj_effect(const Perm *p, int effect);      /* left, right, top and bottom */
float adj_effect_lr(const Perm *p, int effect);

/* the citizen's place in its job's list (workers.indexOf) */
static inline int worker_index(const Cit *c) { return c->jobIndex; }
/* students.indexOf: by citizen number (the order the assigner gives places in) */
int student_index(const Perm *school, const Cit *c);

/* moving a citizen straight toward x this tick (the original's inline steps in work()) */
void step_to_x(Cit *c, float x, float timeMod);
void set_rel(Cit *c, float x, float y);   /* setRelativePos */
void wait_ticks(Cit *c, int tmin, int tmax);   /* setPath([8, getInt(tmin, tmax)]) in its building */
void move_x(Cit *c, int x);                    /* setPath([4, x]) in its building */
void move_x_slow(Cit *c, int x);               /* [12, 50, 4, x] */
bool educate(Cit *c, float amount, float cap);   /* once per visit: true if it did */
void leave_to_surface(Cit *c);                 /* out of its permanent onto the world, at the door */

/* counters kept in whole ticks, stepped by timeMod (often 2/3): rounded at random, so that on average they go at
 * the right pace */
static inline int round_random(float x) { return x <= 0 ? 0 : (int)(x + rnd_float()); }
static inline uint8_t tick_down(uint8_t left, float by) {
  float t = left - by;
  return t <= 0 ? 0 : (uint8_t)(t + rnd_float());
}
static inline uint8_t tick_up(uint8_t n, float by) {
  float t = n + by + rnd_float();
  return t >= 255 ? 255 : (uint8_t)t;
}

/* a material produced (stats too) */
static inline void produce(int m, double amount) { mat_add(m, amount); }

/* the closures of the original: functions run at the end of a citizen's scripted path (sim.h) */
extern uint8_t TH_CHANGE_FLOOR, TH_CHANGE_FLOOR_WAIT_30_60, TH_CHANGE_FLOOR_WAIT_50_100;
/* each file registers its own at start */
void bld_houses_init(void);
void bld_production_init(void);
void bld_services_init(void);
void bld_entertainment_init(void);
void bld_transport_init(void);
void bld_unique_init(void);
void bld_world_init(void);
void bld_hobbies_init(void);

/* ---------------------------------------------------------------- drawing (bld_draw.c) */
/* the drawers (buildings.buildingDrawers.*): what a building shows and merges with */
enum { DRW_NORMAL, DRW_AUTOMERGE, DRW_ALLDIR, DRW_DIFFTYPE, DRW_ROOFTOP_AM, DRW_ROOFDOWN, DRW_DECORATION, DRW_NONE };
typedef struct {
  uint8_t kind;           /* DRW_* */
  uint16_t name, group;   /* currentTextureName and currentTextureGroupName (SPR_*) */
} Drawer;
void bld_drawer(const Perm *p, Drawer *d);
/* zoom-scaled helpers: a sprite's part, a frame of a strip of 20-wide frames 2 apart, a whole sprite */
void dr_part(const GfxBand *b, uint16_t spr, int sx, int sy, int w, int h, int x, int y, int dx, int dy, uint8_t flags,
             uint8_t alpha, uint16_t tint);
void dr_frame(const GfxBand *b, uint16_t spr, int fw, int i, int x, int y, int dx, int dy, uint8_t flags, uint8_t alpha,
              uint16_t tint);
void dr_sprite(const GfxBand *b, uint16_t spr, int x, int y, int dx, int dy, uint8_t flags, uint8_t alpha);
int frames_of(uint16_t spr, int fw);          /* getTexturesByWidth(name, fw).length */
/* the drawers' layers: background on LAYER_BG, front on LAYER_MAIN */
void draw_normal(const GfxBand *b, const Perm *p, int layer, int x, int y, uint16_t spr, bool mirrored);
void draw_custom(const GfxBand *b, const Perm *p, int layer, int x, int y, uint16_t spr, bool mirrored,
                 uint16_t front_spr, int front_frame, uint16_t back_spr, int back_frame);
void draw_merging(const GfxBand *b, const Perm *p, int layer, int x, int y);   /* any merging drawer */
void draw_decoration(const GfxBand *b, const Perm *p, int layer, int x, int y, uint16_t spr, int sub, bool mirrored,
                     uint16_t tint);
void draw_upgrades(const GfxBand *b, const Perm *p, int layer, int x, int y, bool no_cache);
/* the AllDir drawer's secondary background (botanical gardens' plants): which row */
void draw_garden_plants(const GfxBand *b, const Perm *p, int x, int y, uint16_t spr, int texture, bool top, bool bottom);
uint32_t hsv_to_rgb(float h, float s, float v);
uint32_t hsl_to_rgb(float h, float s, float l);
static inline uint16_t tint565(uint32_t rgb) { return rgb565(rgb); }
/* a time clock for animations: game ticks (timeMod units) since start, and a hash for per-building variety */
extern float bld_anim;
static inline uint32_t hash32(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
  return x;
}

/* ---------------------------------------------------------------- windows (bld_window.c) */
/* lang_en.csv's "[0]" and "[1]" replaced by numbers (Localize.lo) */
const char *lo1(char *buf, int size, const char *text, double v0);
const char *lo2(char *buf, int size, const char *text, double v0, double v1);
void win_line(WinOut *w, const char *text);
void win_line1(WinOut *w, const char *text, double v0);
void win_line2(WinOut *w, const char *text, double v0, double v1);
/* the window's buttons call window_action with these (WA_UI_*: the user interface opens a window of its own) */
enum {
  WA_CYCLE_0 = 1, WA_CYCLE_1, WA_CYCLE_2, WA_CYCLE_3,   /* CycleValueButton: the next value */
  WA_MIRROR = 8, WA_TOGGLE, WA_BUILD, WA_BUILD_2, WA_SPECIAL, WA_SPECIAL_2, WA_PRIORITIZE, WA_MODE_0 = 16
};
/* (WA_UI_* are in bld.h: the user interface answers them) */
extern bool bld_secret_orchid, bld_secret_dance, bld_secret_hangar;

/* ---------------------------------------------------------------- shared parts of the families */
/* Factory (bld_production.c) */
void factory_work(Perm *p, Cit *c, float timeMod, bool stop);
void factory_window_lines(Perm *p, WinOut *w);
/* Park walking (ParkWalk, bld_entertainment.c) */
void park_walk(const Perm *left, const Perm *right, Cit *c);
bool park_open(void);
/* houses' private teleporters, teleporters and elevators (bld_transport.c) */
float transport_teleport_x(const Perm *p);
void transport_teleport_used(Perm *p);
void transport_teleport_arrived(Perm *p);
bool transport_elevator_open_door(Perm *p);
void transport_elevator_used(Perm *p);
void transport_elevator_arrived(Perm *p);
void teleporter_upkeep_lines(WinOut *w);
void saucers_new_city(void);
void saucer_on_landing_destroyed(uint16_t ls);
void flying_draw_free(const GfxBand *b, int layer);
void research_bot_draw_free(const GfxBand *b, int layer);
/* world resources (bld_world.c) */
bool resource_prioritized(const Perm *p);
bool resource_any_prioritized(int base);       /* B_FOREST or B_ROCK */
void resource_set_prioritized(Perm *p, bool on);
float limited_materials_left(const Perm *p);    /* forests, rocks, plantations and rock teleporters */
void limited_take(Perm *p, float amount);
bool limited_stay_if_empty(const Perm *p);
bool limited_do_not_gather(const Perm *p);
bool limited_destroyed_on_empty(const Perm *p);
int limited_min_x(const Perm *p), limited_max_x(const Perm *p);
float ruins_explored(const Perm *p);
void ruins_explore(Perm *p, float amount);      /* explored += amount, and any bonuses */
void ruins_new_city(void);
void ruins_after_load(void);
extern uint8_t world_unavailable[MAX_WORLDS];   /* knownResourceInavailability: bits WU_* */
enum { WU_FOREST = 1, WU_ROCK = 2, WU_RUINS = 4 };
/* hobbies (bld_hobbies.c) */
void hobbies_new_city(void);
void hobbies_after_load(void);
void hobby_give(Cit *c);
void hobby_draw_free(const GfxBand *b, int layer);
/* unique buildings (bld_unique.c) */
void unique_new_city(void);
void festival_update(float timeMod);
bool festival_running(void);

/* ---------------------------------------------------------------- building a building in place (World.build + City.onBuildBuilding) */
uint16_t bld_build_at(int w, int col, int row, int cls, bool replace);

/* ---------------------------------------------------------------- each class's state
 * (fixed point where a float would not fit; floats are 4 bytes, packed) */

/* world resources: forests, rocks (LimitedWorldResource) */
typedef struct PACKED {
  float materialsLeft, regrowProgress;
  uint8_t managementMode;      /* Forest: 0 CutDownAndRegrow, 1 Protect, 2 CutDownAndUproot; Rock: 0 Mine, 1 Protect */
  uint8_t prioritized;
} LimitedState;
STATE_FITS(LimitedState);
/* AlienRuins */
typedef struct PACKED {
  uint16_t explored4;          /* explored x 4 (it grows by 0.25) */
  uint8_t bonusesAwarded;
  uint8_t sprite;              /* 0 spr_alienruins, 1 _2, 2 _3 */
  uint8_t found[4];            /* each level's find: 0 none, 1 to 5 = a to e */
} RuinsState;
STATE_FITS(RuinsState);

/* Work buildings that gather from world resources (BlueCollarWork) */
typedef struct PACKED {
  uint16_t nearest;            /* nearestResourceGatherPlace */
  uint8_t searched;            /* !hasNotSearchedForResourceGatherPlaceThisStep */
} BlueCollarState;
STATE_FITS(BlueCollarState);
/* Factory and MaterialConvertingFactory */
typedef struct PACKED {
  uint8_t activeWorkers;
  uint16_t totalEdu;           /* activeWorkersTotalEducation x 4096 */
  uint8_t extra;               /* FX_SHOW_ACTIVE; FarmByProductProcessor: cachedAdjacencyExpiration << 1 */
  float materialMade, totalMaterialUsed;   /* (MaterialConvertingFactory) */
} FactoryState;
STATE_FITS(FactoryState);
/* FarmByProductProcessor (a Factory) */
typedef struct PACKED {
  uint8_t activeWorkers;
  uint16_t totalEdu;
  uint8_t extra;
  float cacheValue, woodMade;  /* cachedAdjacencyValue */
} ByProductState;
STATE_FITS(ByProductState);
/* IndoorFarm: the stage's value (percentGrown, foodLeft or percentCleaned; only one is in use at a time) */
typedef struct PACKED {
  uint8_t stage;
  float value;
} IndoorFarmState;
STATE_FITS(IndoorFarmState);
/* EcoFarm and ExperimentalFarm: three FarmGrowAreas, each value in 24-bit fixed point (2^-17) */
typedef struct PACKED {
  uint8_t value[3][3];         /* percentGrown, foodLeft or percentCleaned, by stage */
  uint8_t stages;              /* 2 bits each */
  uint8_t eco;                 /* EcoFarm: openDay (3 bits), crop2Sprite (2), crop3Sprite (1), plot 2 bonus (1) */
  uint8_t pad;
} GrowFarmState;
STATE_FITS(GrowFarmState);
/* Laboratory */
typedef struct PACKED {
  float totalKnowledgeGenerated;
  uint8_t anyoneHasWorkedHere;
} LabState;
STATE_FITS(LabState);
/* TreePlantation and StoneTeleporter (gather places in buildings) */
typedef struct PACKED {
  float materialsLeft, regrowProgress;
  uint8_t flags;               /* StoneTeleporter: TP_* */
  uint8_t notPressedTime;      /* teleportButtonNotPressedTime (capped) */
} PlantationState;
STATE_FITS(PlantationState);
enum { TP_PRESSED = 1, TP_BIG = 2, TP_INITIAL_BIG = 4 };
/* ExplorationCentre */
typedef struct PACKED {
  uint16_t nearestRuins;
  uint8_t searched;            /* hasSearchedForRuinsThisStep */
  uint8_t currentAction;       /* 0 Unknown (looking), 1 Exploring, 2 Investigating */
} ExplorationState;
STATE_FITS(ExplorationState);

/* ---------------------------------------------------------------- houses (bld_houses.c) */
typedef struct PACKED { uint8_t doorMainTexture, flowersLook; } NormalHouseState;
typedef struct PACKED { uint8_t look; } AlienHouseState;                 /* LivingComputer's frame */
typedef struct PACKED { uint8_t adjBonus; } RusticState;                 /* currentAdjBonus */
typedef struct PACKED { uint8_t variant, movementType, pipeAlt; } MechanicalState;
typedef struct PACKED { uint8_t plantLook; } CuttingEdgeState;           /* ModernHomePlant's frame */
typedef struct PACKED { uint8_t mainTexture, podTexture; } ParkPodState;
typedef struct PACKED { uint8_t interior, mirrored; } LoftState;
typedef struct PACKED { uint8_t front, back, toolsLook; } TinkerersState;
typedef struct PACKED {
  uint8_t bits;                /* materialPhase (2 bits), farmEffects << 6 */
  float stageProgress, totalMaterialsProduced;
} FarmHouseState;
typedef struct PACKED {
  uint16_t timesUsed, timesUsedTo;
  uint8_t mirrored;            /* FlowerPenthouse, LandedExplorationShip */
  uint8_t variant;             /* Mansion: 0 alone, 1 left of a pair, 2 right */
} PrivateTeleporterState;
STATE_FITS(FarmHouseState);
int farmhouse_farm_effects(const Perm *p);
int upgrade_look(const Perm *p, int upgrade);        /* the frame an upgrade's sprite shows */
void upgrade_set_look(Perm *p, int upgrade, int look);
uint16_t mechanical_house_texture(const Perm *p);
uint16_t mansion_texture(const Perm *p);
int rnd_getint(int a, int b);                        /* random_Random.getInt(a, b), also when b < a */

/* ---------------------------------------------------------------- production helpers (bld_production.c) */
float by_product_farms(const Perm *p, bool true_value);   /* adjacentFarmNumber */
int factory_frame(const Perm *p, int nframes);
int limited_frame(float left, float initial, float regrow, int nframes, int nregrow);
float plantation_regrow_speed(const Perm *p);
float stone_teleporter_initial(const Perm *p);
int plot_stage(const Perm *p, int i);
void eco_farms_upgrade(void);
void factory_update_any(Perm *p, float timeMod);
void factory_draw_any(const GfxBand *b, const Perm *p, int layer, int x, int y);
void factory_back_any(const GfxBand *b, const Perm *p, int x, int y, uint16_t active, uint16_t idle);
bool first_worker_working(const Perm *p);            /* workers[0].currentAction == 0 */
Cit *worker_at(const Perm *p, int index);           /* workers[index], NULL if none */
bool operating_cost_user_enabled(void);              /* teleportersAreUserEnabled (systems.c) */
/* ---------------------------------------------------------------- entertainment (bld_entertainment.c) */
typedef struct PACKED { uint8_t marked; } PubState;                  /* the Secret Society's marking */
typedef struct PACKED {
  uint8_t mainTexture, texture;      /* currentMainTexture, currentTexture (plants, sculptures, statue) */
  uint8_t mirrored;                  /* StatueGarden */
  uint8_t podium;                    /* festival: 0 none, 1 at x 3, 2 at x -3 */
} ParkState;
typedef struct PACKED {
  uint8_t mainTexture, texture;      /* the colour, the plants (0 to 14) */
  uint8_t domeVersion, justBuilt;
  uint8_t podium;
} GardenState;
typedef struct PACKED { uint8_t texture; } RooftopParkState;
typedef struct PACKED {
  float lastExpanded;                /* (the primary's counts) */
  uint8_t subImage, hasGrass, isPrimary;
} RedwoodState;
STATE_FITS(RedwoodState);
typedef struct PACKED {
  uint8_t lightsAreOn;               /* ticks x 4 */
  uint8_t color;                     /* nightClubColor */
  uint16_t knownCitizen;             /* knownEntertainedCitizen */
  uint8_t knownSet;                  /* knownEntertainedCitizenSet (ticks) */
} NightClubState;
typedef struct PACKED {
  uint8_t lightsAreOn;
  uint8_t texLeft, texOther, hadLeft, shownLeft, active;
} ScrapyardState;
typedef struct PACKED { uint8_t texture; } ArtMuseumState;
typedef struct PACKED {
  uint8_t gameType;                  /* 0 spaceship battle, 1 block game (the coordinator's counts) */
  uint8_t gameInited;
  uint8_t blocks[5];                 /* each column's blocks (low 3 bits) and its top block's alpha in tenths (<< 3) */
  uint8_t hueShift;
  uint16_t progress1, progress2;     /* spaceship battle: progressTeam1 and 2 x 65535 */
} HoloState;
STATE_FITS(HoloState);
void pub_set_marking(Perm *p, bool on);
bool park_like(const Perm *p);                       /* Park or BotanicalGardens (and their kinds) */
void park_festival_work(Perm *p, Cit *c, float timeMod, int id);
void park_festival_entertained(Perm *p, Cit *c);
void park_festival_stop(Perm *p);
bool garden_is_tree(const Perm *p);
void redwood_destroy_refusal(const Perm *p, const char **why);
/* ---------------------------------------------------------------- decoration (bld_decoration.c) */
typedef struct PACKED {
  uint8_t look;                      /* currentTexture: the sub-image (GlassPiece: 0 farm, 1 plantation) */
  uint8_t bits;                      /* DS_* */
  uint8_t rgb[3];                    /* currentRecolor, when DS_TINTED */
} DecorState;
STATE_FITS(DecorState);
enum { DS_MIRRORED = 1, DS_FLIPPED = 2, DS_TINTED = 4, DS_DOOR_SET = 8, DS_DOOR14 = 16 };
float decoration_door_x(const Perm *p);
void decoration_post_create(Perm *p);
/* ---------------------------------------------------------------- transport (bld_transport.c) */
typedef struct PACKED { uint16_t timesUsed, timesUsedTo; } TeleporterState;   /* Teleporter, Misdirector */
typedef struct PACKED {
  uint16_t timesUsed, timesUsedTo;
  uint8_t doorOpenFor;               /* doorFullyOpenFor (ticks) */
  uint8_t door;                      /* currentDoorTexture x 32 */
} ElevatorState;
typedef struct PACKED {
  uint16_t timesUsed, timesUsedTo, timesUsedStopOver;
  uint8_t hasSaucer, saucerType;
  uint8_t newStage;                  /* gettingNewFlyingSaucerStage x 8 */
  uint8_t newWait;                   /* timeUntilCanGetNewFlyingSaucer (ticks) */
  uint8_t landed;                    /* currentlyLandedSaucer + 1 */
} LandingState;
STATE_FITS(LandingState);
void particle_add(uint16_t spr, int fw, int x, int y);   /* city.particles.addParticle at a city position */
int flying_columns(void);                            /* FlyingPathfinder.highestYPositions: how many */
int flying_highest_y(int index, int *x);             /* and one of them: the column's x, its first obstruction */
#endif
