/* The Final Earth 2 for the NumWorks calculator: the city and its simulation.
 *
 * Inspired by The Final Earth 2 by Florian van Strien, not affiliated with
 * him. Made by Mason Chen as part of NumPlay.
 *
 * The simulation follows the original's code (its class and field names are
 * kept, so each part can be checked against it): floating worlds with columns
 * of 20x20 permanents (buildings, forests, rocks...) stacked on them, and
 * citizens who walk the worlds' surfaces and through the buildings along paths
 * the pathfinder gives them. The original runs at 60 frames a second with
 * timeMod 1; here a tick is a 30th of a second, timeMod 2. */
#ifndef FE_H
#define FE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "plat.h"

#define FPS 30
#define TIMEMOD 2.0f

/* ---------------------------------------------------------------- sizes */
#define MAX_WORLDS 64
#define MAX_COLS 512        /* columns of all the worlds together (a column is 20 pixels) */
#define MAX_SHARDS 256
#define MAX_PERMS 700       /* permanents (ids 1 to MAX_PERMS - 1, 0 is none) */
#define MAX_CITS 800        /* citizens (ids 1 to MAX_CITS - 1) */
#define NONE 0

/* ---------------------------------------------------------------- materials */
enum { M_FOOD, M_WOOD, M_STONE, M_MACHINEPARTS, M_REFINEDMETAL, M_COMPUTERCHIPS, M_CACAO, M_CHOCOLATE,
       M_GRAPHENE, M_ROCKETFUEL, M_KNOWLEDGE, M_COUNT };
/* stocks are doubles: production comes in tiny steps every tick, which a float of 100000 would lose */
typedef struct { double m[M_COUNT]; } Materials;

/* ---------------------------------------------------------------- worlds */
typedef struct {
  int16_t x, y, w, h;      /* rect in city pixels; h 0: an invisible world (bridges, platforms) */
  uint32_t seed;
  uint8_t appearance;      /* APPEAR_* */
  uint8_t id;              /* the original's world id (saves, stories) */
  uint16_t col0;           /* its first column in the column tables */
  uint16_t ncols;          /* w / 20 */
  uint8_t flags;           /* WF_* */
  uint8_t group;           /* worldGroup: worlds citizens can reach from each other */
  uint16_t surfaceShard;   /* surfaceShardId */
} World;
enum { WF_UNBUILDABLE_ALIENS = 1, WF_PROTECTED_KEY = 2, WF_HAS_TELEPORTER_ON_GROUP = 4 };

/* ---------------------------------------------------------------- permanents
 * A permanent is a building or a world resource (forest, rock, ruins), one
 * 20x20 slot of a world's column. Its class (pcls[]) gives its data and what it
 * does. */
typedef struct {
  uint8_t cls;             /* index in pcls[] */
  uint8_t world, col, row; /* worldPosition: column and height in the stack (0 on the ground) */
  uint8_t flags;           /* PF_* */
  uint8_t upgrades;        /* bits: the class's possible upgrades bought */
  uint8_t mode;            /* building mode */
  int8_t bonusAttractiveness;
  uint16_t left, right, top, bottom;   /* neighbouring buildings (CityConnections) */
  uint8_t nworkers, nresidents, nstudents, nvisitors;   /* citizens who work, live, study or are here for fun */
  int16_t bonusEntertainmentCapacity;
  uint16_t shard;          /* group of permanents citizens can walk between (CityConnections) */
  uint8_t s[12];           /* the class's own state (see its bld file) */
} Perm;
enum {
  PF_BUILDING = 1, PF_ROOFTOP = 2, PF_NO_BUILD_ON_TOP = 4, PF_DESTROYED = 8, PF_IS_ENTERTAINMENT = 16,
  PF_IS_MEDICAL = 32
};

extern World worlds[MAX_WORLDS];
extern int nworlds;
extern Perm perms[MAX_PERMS];
extern uint8_t col_height[MAX_COLS];   /* entries in each column's stack, holes included */

static inline Perm *P(uint16_t id) { return id ? &perms[id] : NULL; }
static inline uint16_t pid(const Perm *p) { return p ? (uint16_t)(p - perms) : 0; }
static inline bool is_building(const Perm *p) { return p && (p->flags & PF_BUILDING); }
uint16_t perm_at(int world, int col, int row);    /* 0 if none (or a hole) */
int perm_city_x(const Perm *p);                   /* position.x */
int perm_city_y(const Perm *p);                   /* position.y: its top */

/* ---------------------------------------------------------------- citizens */
typedef struct {
  float relativeX, relativeY;
  float vpp;                /* verticalPathProgress (cit_vpp) */
  float timer;              /* currentPathActionTimerXY (cit_timer) */
  float bornOn;             /* city time (minutes) of birth */
  float educationLevel;
  int16_t dieAgeModifier;   /* years, x 100 */
  uint16_t inBuildingSince; /* city time in half minutes (ticks), wrapping */
  uint16_t pathStartTime;   /* city time in minutes, wrapping */
  int8_t workTimePref;      /* actualWorkTimePreference, -1 to 1, x 127 */
  uint8_t onWorld;
  uint16_t inPermanent, home, job, school;
  uint16_t pathDestination, requestingPathGoal, pathOnlyRelatedTo, pathActionPerm;
  uint16_t nameIndex;
  uint16_t pathOff;         /* its path's steps in the path pool */
  uint8_t pathLen, pathPos; /* steps in it, the next one */
  uint8_t spriteIndex;
  int8_t currentAction;     /* 0 work, 1 school, 2 free, 3 controlled */
  int8_t currentPathAction; /* -2 none, -1 about to start the next step, else the step's type */
  uint8_t pathWalkSpeed;    /* percent */
  uint8_t then, onFail;     /* what to do at the path's end, or if no path is found (THEN_*) */
  uint16_t thenArg;
  uint8_t wantsNightEntertainmentIn;
  uint8_t hobby;
  uint16_t flags;           /* CF_* */
  /* CitizenEntertainment, as city times that wrap: recentEntertainmentTypes in 2-hour units,
   * tryForDayEntertainment, tryForEntertainment and entertainmentFinishedTime in 4-minute units */
  uint8_t entRecent[6];
  uint8_t entTryDay, entTry, entFinished;
  uint8_t jobIndex;         /* its place in its job's list of workers (workers.indexOf), kept by bld.c */
} Cit;
enum {
  CF_ALIVE = 1, CF_CAN_VIEW_SELF = 2, CF_DELAY_CAN_VIEW_SELF = 4, CF_REQUESTING_PATH = 8,
  CF_PATH_RECONSIDER = 16, CF_HAS_WORK_TOOLS = 32, CF_LIKES_NIGHTCLUBS = 64, CF_FORCED_HOME = 128,
  CF_CONTROLLED = 256, CF_WITHOUT_JOB = 512, CF_BUILDING_INITED = 1024, CF_ENT_INITED = 2048,
  CF_HAS_PATH = 4096
};

extern Cit cits[MAX_CITS];
/* (kept as accessors: they may be packed later) */
static inline float cit_vpp(const Cit *c) { return c->vpp; }
static inline void cit_set_vpp(Cit *c, float v) { c->vpp = v; }
static inline float cit_timer(const Cit *c) { return c->timer; }
static inline void cit_set_timer(Cit *c, float v) { c->timer = v; }
extern int ncits;          /* living citizens */
static inline Cit *C(uint16_t id) { return id ? &cits[id] : NULL; }
static inline uint16_t cid(const Cit *c) { return c ? (uint16_t)(c - cits) : 0; }

/* ---------------------------------------------------------------- the city */
#define CITY_NAME_MAX 24    /* bytes of a city's name */
typedef struct {
  double timeSinceStart;    /* minutes since the city began (CityTime) */
  float minutesPerTick;     /* 0.5 */
  Materials materials;
  float happiness;
  float simulationSpeed;    /* 1, 1.5, 2, 0.5 or 0.75 (gui.CityGUI.simulationSpeeds) */
  bool paused;
  uint8_t story;            /* index of the story or free play map */
  bool speedUpStartNights;
  bool disableDying;
  char cityName[CITY_NAME_MAX + 1];   /* the player's name for the city (CityHall), "" none (UTF-8) */
} City;
extern City city;

static inline float city_hour(void) { return (float)(city.timeSinceStart / 60.0 - 24.0 * (int)(city.timeSinceStart / 1440.0)); }
static inline int city_day(void) { return 1 + (int)city.timeSinceStart / 1440; }

/* random numbers (the original's random.Random: getInt(a, b) is a to b - 1) */
uint32_t rnd(void);
float rnd_float(void);                 /* [0, 1) */
float rnd_range(float a, float b);     /* [a, b) */
int rnd_int(int n);                    /* [0, n) */
int rnd_between(int a, int b);         /* [a, b) */
#endif
