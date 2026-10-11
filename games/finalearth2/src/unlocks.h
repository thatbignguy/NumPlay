/* The Final Earth 2: unlocks (unlocks.c). A thing is a class (its pcls
 * index) or an upgrade (npcls + its upgrade_info index). */
#ifndef FE_UNLOCKS_H
#define FE_UNLOCKS_H
#include "fe.h"

enum { UNLOCK_CREATEABLE = 3 };   /* after UNLOCK_LOCKED, UNLOCK_UNLOCKED, UNLOCK_RESEARCHED (build.h) */
typedef struct {
  uint8_t state[256];
  uint8_t explicitlyLocked[32], notify[32];   /* bits */
  uint16_t notifyCategory;                    /* bits of categories with something new */
  uint16_t materials;                         /* bits of materials unlocked (unlockMaterial) */
  float checkIn;
  uint8_t checking;
  bool unlockedManagementOptions, unlockedBuildingModeButton, unlockedDecorationTab;
} Unlocks;
extern Unlocks unlocks;

typedef struct {
  int fossilsCollected;
  bool waldoFound;
  /* simulation.BonusResults (bld_new_city sets them) */
  float extraFoodFromFarms, machinePartsFactorySpeed, labSpeed, indoorFarmSpeed, ecoFarmSpeed;
} Bonuses;
extern Bonuses bonuses;

void unlocks_init(void);
int unlock_state_uid(int uid);
int unlock_state_name(const char *name);
bool unlocks_unlock(const char *name);
void unlocks_fully_unlock(const char *name);
void unlocks_lock(const char *name);
bool unlocks_unlock_uid(int uid);
void unlocks_fully_unlock_uid(int uid);
void unlocks_lock_uid(int uid);
void unlocks_remove_uid(int uid);
void unlocks_research_uid(int uid);
bool unlocks_should_notify(int uid);
void unlocks_stop_notify(int uid);
void unlocks_stop_all_notify(void);
void unlocks_check_build_related_soon(void);
void unlocks_update(float timeMod);
bool unlocks_material_shown(int m);
void unlocks_material(int m);
#endif
