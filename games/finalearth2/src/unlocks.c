/* The Final Earth 2: what can be built and bought (progress.Unlocks).
 *
 * Every building, world resource, building upgrade, city upgrade and policy
 * is Locked, Unlocked (shown, may need a hint's condition, or research),
 * Createable (its condition met) or Researched (its knowledge price paid, or
 * none). Building more of some buildings, and reaching some numbers, unlocks
 * others. */
#include "bld.h"
#include "build.h"
#include "unlocks.h"
#include "upgrades.h"

Unlocks unlocks;

/* the unified number of a class (its pcls index) or an upgrade (after the classes) */
static int uid_of_name(const char *name) {
  int c = pcls_find(name);
  if (c >= 0) return c;
  int u = upgrade_find(name);
  return u >= 0 ? npcls + u : -1;
}
static uint8_t info_flags(int uid) { return uid < npcls ? pcls[uid].unlockFlags : upgrade_info[uid - npcls].unlockFlags; }
static float info_knowledge(int uid) { return uid < npcls ? pcls[uid].cost[M_KNOWLEDGE] : upgrade_info[uid - npcls].cost[M_KNOWLEDGE]; }
static bool is_building_uid(int uid) { return uid < npcls && pcls[uid].kind == KIND_BUILDING; }

void unlocks_init(void) {
  memset(&unlocks, 0, sizeof unlocks);
  unlocks.unlockedManagementOptions = unlocks.unlockedBuildingModeButton = unlocks.unlockedDecorationTab = true;
  for (int i = 0; i < npcls; i++) {
    bool def = pcls[i].unlockFlags & UF_DEFAULT;
    if (pcls[i].kind == KIND_RESOURCE) unlocks.state[i] = def ? UNLOCK_UNLOCKED : UNLOCK_LOCKED;
    else unlocks.state[i] = def && pcls[i].cost[M_KNOWLEDGE] == 0 ? UNLOCK_RESEARCHED : def ? UNLOCK_UNLOCKED : UNLOCK_LOCKED;
  }
  for (int i = 0; i < nupgrades; i++) {
    const UpgradeInfo *u = &upgrade_info[i];
    bool def = u->unlockFlags & UF_DEFAULT;
    if (u->kind == UK_BUILDING_UPGRADE || u->kind == UK_BUILDING_MODE)
      unlocks.state[npcls + i] = def && u->cost[M_KNOWLEDGE] == 0 ? UNLOCK_RESEARCHED : def ? UNLOCK_UNLOCKED : UNLOCK_LOCKED;
    else
      unlocks.state[npcls + i] = def ? UNLOCK_UNLOCKED : UNLOCK_LOCKED;
  }
}

int unlock_state_uid(int uid) { return uid >= 0 ? unlocks.state[uid] : UNLOCK_LOCKED; }
int unlock_state(int cls) { return unlocks.state[cls]; }
int unlock_state_name(const char *name) { return unlock_state_uid(uid_of_name(name)); }

static void notify(int uid) {
  unlocks.notify[uid >> 3] |= (uint8_t)(1 << (uid & 7));
  if (uid < npcls) unlocks.notifyCategory |= (uint16_t)(1 << pcls[uid].category);
}

void unlocks_lock_uid(int uid) {
  if (uid < 0) return;
  unlocks.state[uid] = UNLOCK_LOCKED;
  unlocks.explicitlyLocked[uid >> 3] |= (uint8_t)(1 << (uid & 7));
}
/* unlock: true if it was locked */
bool unlocks_unlock_uid(int uid) {
  if (uid < 0 || unlocks.state[uid] != UNLOCK_LOCKED) return false;
  uint8_t f = info_flags(uid);
  bool researched = info_knowledge(uid) == 0 && (!is_building_uid(uid) || !(f & UF_HINT));
  unlocks.state[uid] = researched ? UNLOCK_RESEARCHED : UNLOCK_UNLOCKED;
  if (!(f & UF_HINT)) notify(uid);
  unlocks_check_build_related();
  return true;
}
void unlocks_remove_uid(int uid) {
  if (uid >= 0) unlocks.state[uid] = UNLOCK_LOCKED;
}
void unlocks_fully_unlock_uid(int uid) {
  if (uid >= 0 && unlocks.state[uid] == UNLOCK_UNLOCKED) {
    unlocks.state[uid] = UNLOCK_CREATEABLE;
    notify(uid);
  }
}
void unlocks_research_uid(int uid) {
  if (uid >= 0) unlocks.state[uid] = UNLOCK_RESEARCHED;
}
void unlocks_research(int cls) { unlocks_research_uid(cls); }
bool unlocks_unlock(const char *name) { return unlocks_unlock_uid(uid_of_name(name)); }
void unlocks_fully_unlock(const char *name) { unlocks_fully_unlock_uid(uid_of_name(name)); }
void unlocks_lock(const char *name) { unlocks_lock_uid(uid_of_name(name)); }

bool unlocks_should_notify(int uid) { return uid >= 0 && (unlocks.notify[uid >> 3] >> (uid & 7) & 1); }
void unlocks_stop_notify(int uid) {
  if (uid >= 0) unlocks.notify[uid >> 3] &= (uint8_t)~(1 << (uid & 7));
}
void unlocks_stop_all_notify(void) {
  memset(unlocks.notify, 0, sizeof unlocks.notify);
  unlocks.notifyCategory = 0;
}

void unlocks_check_build_related_soon(void) {
  if (unlocks.checkIn <= 0) unlocks.checkIn = 60;
}
void unlocks_update(float timeMod) {
  if (unlocks.checkIn > 0) {
    unlocks.checkIn -= timeMod;
    if (unlocks.checkIn <= 0) unlocks_check_build_related();
  }
}

/* city.getAmountOfPermanentsPerType */
static int count(const char *name) {
  int c = pcls_find(name), n = 0;
  if (c < 0) return 0;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == c) n++;
  return n;
}
static bool researched_or_createable(const char *name) {
  int s = unlock_state_name(name);
  return s == UNLOCK_RESEARCHED || s == UNLOCK_CREATEABLE;
}

/* checkBuildRelatedUnlocks (the paid versions' and holidays' rules are left out) */
void unlocks_check_build_related(void) {
  if (unlocks.checking >= 4) return;   /* an unlock in here checks again, as in the original (a few levels are plenty) */
  unlocks.checking++;
  int parks = count("Park"), labs = count("Laboratory"), gardens = count("BotanicalGardens");
  if (parks >= 5) {
    unlocks_unlock("BotanicalGardens");
    if (parks >= 10 && labs >= 5) unlocks_fully_unlock("BotanicalGardens");
  }
  if (count("ModernArtMuseum") >= 3) unlocks_fully_unlock("ArtColony");
  if (researched_or_createable("BotanicalGardens")) {
    unlocks_unlock("EcoFarm");
    if (gardens >= 10 && count("ExperimentalFarm") >= 1) unlocks_fully_unlock("EcoFarm");
    if (gardens >= 50 && count("RefinedMetalFactory") >= 1) unlocks_unlock("BotanicalGardensDome");
  }
  int rmf = count("RefinedMetalFactory"), ccf = count("ComputerChipFactory");
  if (labs >= 5 && rmf >= 1) unlocks_fully_unlock("ComputerResearchCenter");
  if (count("ComputerResearchCenter") >= 1) unlocks_unlock("ComputerChipFactory");
  if (count("MachinePartsFactory") >= 3) unlocks_fully_unlock("MechanicalHouse");
  if (count("LivingResearchCenter") >= 1) {
    unlocks_fully_unlock("CuttingEdgeHome");
    unlocks_unlock("ParkPod");
    unlocks_unlock("ComfortableCouch");
    unlocks_unlock("SmartSpeaker");
  }
  if (ccf >= 1) {
    unlocks_unlock("HackerHQ");
    if (count("Arcade") >= 3) unlocks_fully_unlock("HackerHQ");
  }
  int pubs = count("Pub");
  if (pubs >= 2) {
    unlocks_unlock("Restaurant");
    if (pubs >= 5) {
      unlocks_fully_unlock("Restaurant");
      unlocks_unlock("HerbGarden");
    }
  }
  if (count("Restaurant") >= 2) unlocks_fully_unlock("HerbGarden");
  int blossom = count("BlossomRestaurant");
  if (blossom >= 2) {
    unlocks_unlock("BlossomHut");
    if (blossom >= 10) unlocks_fully_unlock("BlossomHut");
  }
  if (count("FarmByProductProcessor") >= 10 && gardens >= 200 && count("BlossomHut") >= 50) unlocks_unlock("TreeHuggerBase");
  if (rmf >= 1 && count("StoneMine") >= 15 && count("StoneTeleporter") >= 3) unlocks_fully_unlock("StoneResearchCenter");
  int mh = count("MechanicalHouse");
  if ((mh >= 4 && rmf >= 3 && ccf >= 1) || count("ComputerResearchCenter") >= 1) {
    unlocks_unlock("ExperimentationLab");
    if ((mh >= 12 && rmf >= 10 && ccf >= 3) || count("ComputerResearchCenter") >= 1) unlocks_fully_unlock("ExperimentationLab");
  }
  if (count("TheMachine") >= 1 && count("FestivalHQ") >= 1) unlocks_unlock("FlowerPenthouse");
  int fh = count("FarmHouse");
  if (fh >= 20) {
    unlocks_unlock("RedwoodTree");
    if (fh >= 40) unlocks_fully_unlock("RedwoodTree");
  }
  if (count("School") >= 5) unlocks_unlock("Library");
  if (count("StatueOfHappiness") >= 1) unlocks_unlock("Beacon");
  if (unlock_state_name("BlossomHippieHQ") == UNLOCK_UNLOCKED && gardens >= 20) {
    /* a stack of 20 botanical gardens */
    int g = pcls_find("BotanicalGardens");
    for (int i = 1; i < MAX_PERMS; i++) {
      Perm *p = &perms[i];
      if (!(p->flags & PF_USED) || p->cls != g || (p->bottom && P(p->bottom)->cls == g)) continue;
      int n = 1;
      for (Perm *t = P(p->top); t && t->cls == g; t = P(t->top)) n++;
      if (n >= 20) {
        unlocks_fully_unlock("BlossomHippieHQ");
        break;
      }
    }
  }
  if (count("LivingResearchCenter") >= 1 && count("ArtColony") >= 1 && count("ModernArtMuseum") >= 10) unlocks_unlock("Loft");
  unlocks.checking--;
}

/* checkStatRelatedUnlocks (each time the stats are counted) */
void unlocks_check_stat_related(void) {
  if (stats.peopleWithHome >= 200) unlocks_fully_unlock("LivingResearchCenter");
  if (ncits >= 200) unlocks_fully_unlock("HyperElevator");
  if (happiness.happiness >= 99.99f) unlocks_unlock("StatueOfHappiness");
  int tps = 0, works = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    if (!(perms[i].flags & PF_USED)) continue;
    if (is_a(&perms[i], B_TELEPORTER)) tps++;
    if (is_a(&perms[i], B_WORK)) works++;
  }
  if (ncits >= 500 && tps > 1) unlocks_unlock("SecretSocietyHouse");
  if (bonuses.fossilsCollected >= 5) unlocks_unlock("FossilMuseum");
  if (ncits >= 750) unlocks_unlock("BlossomHippieHQ");
  if (unlock_state_name("Beacon") == UNLOCK_UNLOCKED && happiness.fullHappinessTime >= 20160) unlocks_fully_unlock("Beacon");
  if (ncits >= 100 && works > 1) unlocks_unlock("CityHall");
}

/* the materials shown once they matter (cacao, chocolate, graphene and rocket fuel are paid-version materials) */
bool unlocks_material_shown(int m) {
  return m == M_FOOD || m == M_WOOD || m == M_STONE || m == M_KNOWLEDGE || m == M_MACHINEPARTS || m == M_REFINEDMETAL ||
         m == M_COMPUTERCHIPS || (unlocks.materials >> m & 1);
}
void unlocks_material(int m) { unlocks.materials |= (uint16_t)(1 << m); }
