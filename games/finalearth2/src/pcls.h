/* The Final Earth 2: classes of permanents (buildings and world resources).
 *
 * Each class has its data (buildinginfo.json) and what it does: the
 * original's subclasses of Building become a PermClass with function
 * pointers, NULL where the class does what its base does. */
#ifndef FE_PCLS_H
#define FE_PCLS_H
#include "fe.h"
#include "gfx.h"

enum { KIND_RESOURCE, KIND_BUILDING };
/* unlocks: unlocked at the start, has a hint while only unlocked (showUnlockHint), not unlocked by "unlock all" */
enum { UF_DEFAULT = 1, UF_HINT = 2, UF_NOT_WITH_ALL = 4 };
/* the original's base classes, as bits (a class can be several: WorkWithHome is a House and a Work) */
enum {
  B_HOUSE = 1, B_WORK = 2, B_ENTERTAINMENT = 4, B_SCHOOL = 8, B_MEDICAL = 16, B_TELEPORTER = 32,
  B_LANDINGSITE = 64, B_ELEVATOR = 128, B_UNIQUE = 256, B_ROOFTOP = 512, B_NO_BUILD_ON_TOP = 1024,
  B_BUILD_ON_GROUND = 2048, B_FOREST = 4096, B_ROCK = 8192, B_RUINS = 16384,
  B_HIDE_WALKERS = 32768,  /* get_walkThroughCanViewSelfInThisBuilding false */
  B_CITY_TOP = 1 << 16,    /* nothing may be above it */
  B_NO_INSERT = 1 << 17    /* disableInsertReplaceOrBuildOnTop */
};

/* drawing layers, back to front (the original's stages, buildings.md 2.6): the citizens inside
 * buildings are drawn between LAYER_BG and LAYER_MID, those outside after LAYER_MAIN */
enum { LAYER_BG, LAYER_MID, LAYER_MAIN, LAYER_ABOVE, LAYER_FRONT, LAYER_COUNT };

/* a building's window: what it shows (win.h) */
typedef struct WinOut WinOut;

struct PermClass;
typedef struct {
  void (*init)(Perm *p);                                    /* the constructor's defaults */
  void (*on_build)(Perm *p);
  void (*destroy)(Perm *p);
  void (*update)(Perm *p, float timeMod);
  void (*work)(Perm *p, Cit *c, float timeMod, bool shouldStopWorking);
  void (*walk_around)(Perm *p, Cit *c, float stepsInBuilding);   /* homes */
  void (*on_citizen_leave)(Perm *p, Cit *c, uint16_t to);
  void (*after_give_job)(Perm *p, Cit *c);
  float (*attractiveness)(const Perm *p);                    /* get_baseAttractiveness, NULL: the class's quality */
  int (*jobs)(const Perm *p);                               /* NULL: the class's jobs */
  int (*residents)(const Perm *p);                          /* NULL: the class's residents */
  uint16_t (*first_building_to_go_to)(Perm *p);             /* NULL: itself */
  /* schools */
  void (*be_at_school)(Perm *p, Cit *c, float timeMod);
  int (*student_capacity)(const Perm *p);
  /* entertainment (IEntertainmentBuilding): NULL uses the class's numbers */
  bool (*is_open)(const Perm *p);
  bool (*is_open_for_existing_visitors)(const Perm *p);    /* NULL: is_open */
  bool (*finish_entertainment)(Perm *p, Cit *c, float timeMod);   /* NULL: true */
  void (*be_entertained)(Perm *p, Cit *c, float timeMod);
  float (*entertainment_capacity)(const Perm *p);         /* get_baseEntertainmentCapacity (any permanent can give some) */
  int (*entertainment_type)(const Perm *p);
  int (*secondary_entertainment_types)(const Perm *p);    /* bits */
  float (*entertainment_quality)(const Perm *p);          /* NULL: the class's */
  /* medical buildings */
  float (*medical_quality)(const Perm *p);
  float (*medical_capacity)(const Perm *p);
  float (*herb_capacity)(const Perm *p);                  /* HerbGarden.getHerbCapacity */
  /* homes */
  float (*years_to_live_longer)(const Perm *p);
  bool (*has_private_teleporter)(const Perm *p);
  int (*fixed_capacity_for_workers)(const Perm *p);
  /* work hours that change (policies, upgrades, modes) */
  float (*start_time)(const Perm *p);
  float (*end_time)(const Perm *p);
  /* drawing: one layer of it, its top-left at (x, y) on screen (scaled by the band's zoom) */
  void (*draw)(const GfxBand *b, const Perm *p, int layer, int x, int y);
  /* its window's own lines and buttons (addWindowInfoLines and friends), and a button's action */
  void (*window)(Perm *p, WinOut *w);
  void (*window_action)(Perm *p, int action);
  /* after any build or destroy (onCityChange) */
  void (*on_city_change)(Perm *p);
} PermOps;

typedef struct PermClass {
  const char *name;          /* the original's class name */
  uint8_t kind;
  uint32_t base;             /* B_* */
  uint8_t category;
  float cost[M_COUNT];       /* to build (knowledge is the research price: see unlocks) */
  uint8_t residents, jobs;
  uint8_t quality;           /* homes' quality; the attractiveness of others is 50 unless ops say */
  float startTime, endTime;  /* work hours (Work: 6 to 19) */
  uint8_t doorX;             /* 13 */
  float workTimePreferenceMod;   /* Work: 1 */
  /* entertainment: type (0 night clubs, 1 pubs and restaurants, 2 art, 3 nature, 4 gaming, 5 libraries),
   * hours spent (min, max), days it satisfies (min, max), quality */
  int8_t entType;
  float entMinTime, entMaxTime, entMinSatisfy, entMaxSatisfy;
  uint8_t entQuality;
  int8_t medicalTypeID;       /* get_medicalTypeID */
  float medicalTypeLimit;     /* get_medicalTypeLimit */
  float teleporterOperatingCost;   /* knowledge a day */
  uint16_t limitedTo, limitedPerCitizens;   /* specialInfo limited: at most n, or one per n citizens */
  uint8_t unlockFlags;       /* UF_* */
  int8_t parent;             /* the parent class's index (is() checks), -1 none in the table */
  uint16_t sprite;           /* SPR_* */
  const PermOps *ops;
} PermClass;

extern const PermClass pcls[];
extern const int npcls;
int pcls_find(const char *name);   /* -1 if unknown */
static inline const PermClass *PC(const Perm *p) { return &pcls[p->cls]; }
static inline bool is_a(const Perm *p, uint32_t base) { return p && (pcls[p->cls].base & base); }
/* the original's is(): this class or one of its ancestors */
static inline bool class_is(int cls, int ancestor) {
  for (int c = cls; c >= 0; c = pcls[c].parent)
    if (c == ancestor) return true;
  return false;
}
#endif
