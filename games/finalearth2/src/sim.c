/* The Final Earth 2: the simulation's step (CitySimulation, CityTime and
 * the parts of City.update that run it).
 *
 * The original updates once a frame at 60 frames a second, its timeMod (1)
 * times 2/3 of the game speed, in as many steps as the speed needs. The
 * calculator draws 30 frames a second, so each tick does two frames' worth of
 * steps: the same steps, with the same timeMods, as the original. */
#include "bld.h"
#include "scratch.h"

Scratch *scratch;   /* set by main() to memory on its stack */

uint8_t sim_flags = SIM_JOBS_CHANGED | SIM_HOUSES_CHANGED | SIM_SCHOOLS_CHANGED;
static float infrequent_start;

void sim_update_pathfinder(bool invalidate, uint16_t related_to) {
  bld_city_changed(invalidate);
  finder_invalidate();
  if (!invalidate) return;
  path_invalidate_all();
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE) || !(c->flags & CF_HAS_PATH)) continue;
    if (!c->pathOnlyRelatedTo || !related_to || related_to == c->pathOnlyRelatedTo) cit_reset_path(c, false);
  }
}

void cits_reset(void);
void path_reset(void);
void systems_reset(void);
/* a fresh game: every part's state as at the start */
void game_reset(void) {
  city_reset();
  cits_reset();
  path_reset();
  systems_reset();
  finder_invalidate();
  sim_flags = SIM_JOBS_CHANGED | SIM_HOUSES_CHANGED | SIM_SCHOOLS_CHANGED;
  infrequent_start = 0;
  bld_new_city();
}

/* CityTime.update: the clock (nights at the story's start go faster) */
static void time_update(float timeMod) {
  city.timeSinceStart += timeMod * city.minutesPerTick;
  float h = city_hour();
  if ((h > 23 || h < 4) && city.speedUpStartNights) {
    int day = city_day();
    if (day == 1 || day == 2 || (day == 3 && h < 4)) city.timeSinceStart += timeMod * city.minutesPerTick * 2.75f;
    else if (day == 3) city.timeSinceStart += timeMod * city.minutesPerTick;
    else if (day == 4) city.timeSinceStart += timeMod * city.minutesPerTick * 0.5f;
  }
}

/* CitySimulation.update */
static void sim_update(float timeMod) {
  int prev_hour = ((int)city.timeSinceStart / 60) % 24;
  time_update(timeMod);
  int hour = ((int)city.timeSinceStart / 60) % 24;
  if (prev_hour >= 23 && hour < 1) stats_midnight_update();
  stats_update(timeMod);
  finder_pre_update();
  path_distribute();
  flying_update(timeMod);
  spawners_update(timeMod);
  if (ncits > 0) {
    /* CitizenUpdater */
    for (int i = 1; i < MAX_CITS; i++) {
      Cit *c = &cits[i];
      if (!(c->flags & CF_ALIVE) || (c->flags & CF_CONTROLLED)) continue;
      if (festival_running()) {
        festival_citizen(c, timeMod);
        continue;
      }
      if (c->flags & CF_DELAY_CAN_VIEW_SELF) c->flags &= (uint16_t)~CF_DELAY_CAN_VIEW_SELF;
      cit_update_path(c, timeMod);
      if ((c->flags & CF_ALIVE) && !(c->flags & CF_HAS_PATH) && !(c->flags & CF_REQUESTING_PATH)) cit_update_daily_life(c, timeMod);
    }
    if (prev_hour < 12 && hour >= 12)
      for (int i = 1; i < MAX_CITS; i++)
        if (cits[i].flags & CF_ALIVE) cit_mid_day_update(&cits[i]);
    /* infrequent updates: a 60th of the citizens each step (in the order of the original's list: from the top) */
    float per = ncits / 60.0f;
    int lo = (int)ceilf(infrequent_start), hi = (int)ceilf(infrequent_start + per);
    if (hi > ncits) hi = ncits;
    int k = 0;
    for (int i = 1; i < MAX_CITS && k < hi; i++) {
      if (!(cits[i].flags & CF_ALIVE)) continue;
      if (k >= lo) cit_infrequent_update(&cits[i]);
      k++;
    }
    infrequent_start += per;
    if (infrequent_start >= ncits) infrequent_start = 0;
  }
  animals_update(timeMod);
  baby_update(timeMod);
  path_update(timeMod);
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *p = &perms[i];
    if ((p->flags & PF_USED) && PC(p)->ops && PC(p)->ops->update) PC(p)->ops->update(p, timeMod);
  }
  building_upgrades_update(timeMod);
  boosts_update(timeMod);
  operating_cost_update(timeMod);
}

/* CitySimulation.lateUpdate */
static void sim_late_update(float timeMod) {
  assign_jobs();
  assign_schools();
  assign_houses();
  happiness_update(timeMod);
  eating_update(timeMod);
}

/* a 30th of a second: two of the original's frames */
void sim_tick(float frameTimeMod) {
  float used = city.paused ? 0 : city.simulationSpeed * 0.6666667f;
  for (int f = 0; f < 2; f++) {
    worlds_update(frameTimeMod * used);
    int steps = (int)ceilf(used / 1.67f);
    for (int s = 0; s < steps; s++) sim_update(frameTimeMod * (used / steps));
    if (steps == 0) stats_update(0);
    progress_update(frameTimeMod);
    sim_late_update(frameTimeMod * used);
  }
}
