/* The Final Earth 2: building upgrades and modes, city upgrades (research)
 * and policies: their table (upgrades_data.c, made by tools/gen_upgrades.py
 * from buildingUpgradesInfo.json, cityUpgradesInfo.json and policiesInfo.json)
 * and buying them (upgrades.c). What they do is in the buildings' own code
 * (bld_*.c), which reads the bits and modes kept in each building. */
#ifndef FE_UPGRADES_H
#define FE_UPGRADES_H
#include "fe.h"
#include "upgrades_data.h"

enum { UK_BUILDING_UPGRADE, UK_BUILDING_MODE, UK_CITY_UPGRADE, UK_POLICY };
typedef struct {
  const char *name;          /* the original's class name */
  const char *title, *text;  /* its name and description (lang_en.csv) */
  const char *titleDone, *textDone;   /* city upgrades: shown instead once bought (nameIfNotDone...) */
  uint8_t kind;              /* UK_* */
  uint8_t unlockFlags;       /* UF_* (pcls.h) */
  int8_t layer;              /* the layer its sprite is drawn on (LAYER_*), -1 none */
  uint8_t frames;            /* frames of its sprite (the player may pick one), 0 none */
  uint8_t lockedFrames;      /* the last frames, shown only after the secret code "orchid" */
  int8_t bonusAttractiveness;
  uint8_t noCache;           /* drawn after the building's other sprites of its layer */
  uint8_t next;              /* city upgrades: the next step of its chain, 0xFF none */
  uint16_t sprite;           /* SPR_* */
  float cost[M_COUNT];       /* knowledge only until researched */
} UpgradeInfo;

extern const UpgradeInfo upgrade_info[];
extern const int nupgrades;
int upgrade_find(const char *name);   /* -1 if unknown */

/* what a class offers in its window: upgrade_info indexes + 1, 0 ends the list */
typedef struct { uint8_t up[3], mode[4], city[6], pol[3]; } ClassUpgrades;
extern const ClassUpgrades class_upgrades[];

/* ---------------------------------------------------------------- the user interface's view (upgrades.c)
 * A building's window lists, in this order, its upgrades, its modes, the
 * city upgrades it offers, then the policies it offers. Each entry: */
enum { UO_UPGRADE, UO_MODE, UO_CITY, UO_POLICY };
typedef struct {
  uint8_t kind;              /* UO_* */
  uint8_t upgrade;           /* index in upgrade_info */
  bool owned;                /* bought (an upgrade, a city upgrade), the current mode, or the policy is on */
  bool affordable;           /* the city can pay price now */
  bool canChangeLook;        /* owned and with several sprites: "change_variant" (upgrade_change_look) */
  const char *title, *text;  /* in the original's words (a bought chain step shows its titleDone) */
  float price[M_COUNT];      /* for this building (knowledge only while not researched) */
} UpgradeOption;
/* the options of p's window (at most max); returns how many */
int upgrade_options(const Perm *p, UpgradeOption *out, int max);
/* the "all" button: how many buildings of p's class the option would be bought for, and its total price
 * (gui.CreateBuildingUpgrades.getUpgradeAllNumberAndCanAfford) */
int upgrade_all_count(const Perm *p, int upgrade, float total[M_COUNT], bool *can_afford_all);
/* presses an option's button: buys it (or switches the mode, buys the research, toggles the policy);
 * all: the window's "all" button is on, so every building of its class gets it too. False if it could not
 * be paid */
bool upgrade_press(Perm *p, int upgrade, bool all);
/* the "all" button pressed while the modes' "all" is selected: p's mode for every building of its class */
void upgrade_mode_to_all(Perm *p);
void upgrade_change_look(Perm *p, int upgrade);     /* the next frame of an owned upgrade's sprite */
/* the labs' smart upgrade (gui.GlobalUpgradeWindow.addSmartUpgradeForLabs): what it would cost, whether there
 * is anything to buy, and buying it */
bool upgrade_smart_labs_price(float price[M_COUNT]);
bool upgrade_smart_labs_buy(void);

/* ---------------------------------------------------------------- state */
bool upgrade_has(const Perm *p, int upgrade);       /* a building's upgrade or current mode */
bool city_upgrade_bought(int upgrade);
bool policy_on(int upgrade);
/* buys a building upgrade for p without paying (story actions, the load quirk) */
void upgrade_add(Perm *p, int upgrade);
/* p's mode becomes upgrade (its old one goes first) */
void upgrade_set_mode(Perm *p, int upgrade);
void upgrade_first_mode(Perm *p);                   /* the Building constructor's first mode */
/* each class's per-city changes (city upgrades on buildingInfo): quality and residents */
int upgrades_quality_bonus(int cls);
int upgrades_residents_bonus(int cls);
/* city upgrades bought and policies on, as bits of upgrade_info indexes (saved) */
#define UPG_WORDS ((U_COUNT + 31) / 32)
typedef struct {
  uint32_t bought[UPG_WORDS];
} CityUpgradesState;
extern CityUpgradesState city_upgrades;
void upgrades_new_city(void);
void upgrades_after_load(void);   /* CityUpgrades.load: their effects again (and its quirk) */
#endif
