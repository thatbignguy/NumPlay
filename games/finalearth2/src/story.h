/* The Final Earth 2: scenarios and free play maps (progress.Story): their
 * data (stories_data.c, made by tools/gen_stories.py) and their running
 * (story.c). */
#ifndef FE_STORY_H
#define FE_STORY_H
#include "fe.h"

/* a building stack's entry: a class, or a hole; mirrored; a decoration's look + 1 */
#define STACK_NULL 0xFF
#define STACK_MIRROR 0x100
#define STACK_DECOR(n) (((n) + 1) << 9)

typedef struct { uint8_t col, cls; } StoryRes;
typedef struct { uint8_t col, n; const uint16_t *items; } StoryStack;
typedef struct { uint16_t amount; uint8_t ageMin, ageMax; int16_t minX, maxX; } StoryCitizens;
typedef struct { uint8_t col; uint16_t sprite; } StoryDeco;
typedef struct {
  int16_t x, y, w, h;
  uint32_t seed;
  uint8_t appearance;   /* 0 normal, 1 "key", 2 "key2" */
  uint8_t flags;        /* WF_* */
  uint16_t decoration;  /* sprite for every column, 0: grass */
  uint8_t nres; const StoryRes *res;
  uint8_t nstacks; const StoryStack *stacks;
  uint8_t ncits; const StoryCitizens *cits;
  uint8_t ndecos; const StoryDeco *decos;
} StoryWorld;

enum {
  SG_BUILDPERMANENTS, SG_GATHERMATERIALS, SG_PRODUCTIONGOAL, SG_PERFORMBUILDINGUPGRADES, SG_PERFORMCITYUPGRADE,
  SG_MINIMUMHAPPINESS, SG_MINIMUMCITIZENS, SG_HOUSINGCAPACITY, SG_WAIT, SG_INVITEPEOPLE, SG_EXPLOREALIENRUINS,
  SG_EXPLOREDWITHRESEARCHSPACECRAFT, SG_COMPLETEHACKERMISSIONS, SG_WAITFORDECRYPTION, SG_GLORYGOAL
};
typedef struct {
  uint8_t type;
  uint8_t cls;
  int8_t world, notWorld;    /* BuildPermanents: only on (or not on) that world, -1 any */
  uint16_t amount;
  float value;
  const char *name;          /* an upgrade's class */
  float mats[M_COUNT];
} SubGoal;

enum { EV_SAUCER, EV_REGULARLY };
typedef struct {
  uint8_t type;
  float time;                /* game hours after the goal starts */
  uint16_t amount;           /* a saucer's citizens, or SpawnCitizensRegularly's spawnNumber */
  uint8_t ageMin, ageMax, world;
  uint8_t groupMin, groupMax;
  float betweenMin, betweenMax;   /* hours */
  const char *afterDone;
} StoryEvent;

enum { GF_DISABLEDYING = 1, GF_DISABLEDESTROY = 2, GF_HIDDENBOOST = 4, GF_SETBUILDABLEALIENS = 8 };
typedef struct {
  const char *name;
  int8_t next;               /* the next goal's index, -1 none */
  int8_t nextStory;          /* index in stories[], -1 none */
  uint8_t nsub; const SubGoal *sub;
  uint8_t nunlocks; const char *const *unlocks;
  uint8_t nresearches; const char *const *researches;
  uint8_t nevents; const StoryEvent *events;
  uint8_t flagsSet, flagsValue;   /* GF_*: which the goal sets, and to what */
  uint16_t tutorial;
  const char *text, *quickText, *title;
} StoryGoal;

typedef struct { uint8_t cls; uint8_t min, max; bool building; } GenItem;
typedef struct {
  int16_t worldsMin, worldsMax, minX, minY, maxX, maxY, minWidth, maxWidth, minHeight, maxHeight, heightVariation;
  uint8_t nitems; const GenItem *items;
  bool spreadBuildings, spreadResources, rememberSpread, avoidOverfull;
} WorldGenerator;

typedef struct { uint16_t sprite; int16_t width, image, x, y; float velocity; } StoryBackground;

enum { ST_FREEPLAY = 1, ST_LOCK_ALL = 2, ST_INVITE = 4, ST_SPEED_UP_NIGHTS = 8, ST_NO_EXTRA_RESOURCES = 16 };
typedef struct {
  const char *link, *name, *description;
  uint8_t flags;
  float initialMaterials[M_COUNT];
  uint8_t nworlds; const StoryWorld *worlds;
  uint8_t ngoals; const StoryGoal *goals;
  uint8_t startGoal;
  uint8_t ngens; const WorldGenerator *gens;
  uint8_t nlocks; const char *const *locks;
  int16_t viewYFromBottom, viewStartX;
  uint8_t nbackgrounds; const StoryBackground *backgrounds;
} Story;

extern const Story stories[];
extern const int nstories;

/* ---------------------------------------------------------------- the running story (story.c) */
#define STORY_MAX_PLANNINGS 4
#define STORY_MAX_REGULAR 4
typedef struct { uint8_t goal, next; float timeStart; } StoryPlanning;   /* timeStart in game hours */
typedef struct { uint8_t goal, event; int left; double nextSpawn; } Regularly;   /* a SpawnCitizensRegularly running */
typedef struct {
  uint8_t index;            /* in stories[] */
  int8_t goal;              /* the current goal, -1 none */
  float goalTimer;
  bool justInvitedPeople, disableDestroy, hiddenBoost;
  uint8_t nplannings, nregular;
  StoryPlanning plannings[STORY_MAX_PLANNINGS];
  Regularly regular[STORY_MAX_REGULAR];
} StoryState;
extern StoryState story;

typedef struct { bool extraResources; int citizens; } NewCityOptions;   /* free play: citizens 0 = the map's */
void story_new_city(int index, const NewCityOptions *opt);
void story_update(float timeMod);
const StoryGoal *story_goal(void);
bool story_subgoal_complete(int i);
void invite_price(float cost[M_COUNT]);
int invite_problem(void);
bool invite_citizens(void);

/* from the buildings (bld*.c, upgrades.c) */
int upgrades_buildings_with(const char *upgrade);   /* buildings owning that upgrade */
bool upgrades_city_bought(const char *upgrade);
float bld_alien_ruins_explored(void);              /* the first ComputerAlienRuins' explored */
float bld_research_bot_time(void);                 /* the first ResearchBot's timeWorking */
float bld_hacker_mission(void);                    /* the first HackerHQ's currentMission, -1 none */
float bld_decryptor_time(void);                    /* the first AlienDecryptor's timeWorking */
void bld_customize(Perm *p, int decor, bool mirrored);
void saucer_spawn(int world, int amount, int ageMin, int ageMax, int toCol);
/* from the user interface */
void ui_show_goal_text(int goal);
void ui_show_tutorial(int tutorial);
void ui_show_message(const char *text);
bool ui_story_window_open(void);
void meta_win_scenario(int story, float time);   /* MetaGame: the best time of a won scenario */
#endif
