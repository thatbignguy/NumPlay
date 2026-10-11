/* Runs the simulation without drawing: a world, some buildings and
 * citizens, for many ticks, and prints what the citizens are doing. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/bld.h"
#include "../src/scratch.h"

void story_set_name(const char *n);

static const char *action_name(const Cit *c) {
  static char b[64];
  snprintf(b, sizeof b, "act %d path %d step %d", c->currentAction, c->currentPathAction, c->pathPos);
  return b;
}

int main(int argc, char **argv) {
  int ticks = argc > 1 ? atoi(argv[1]) : 3000;
  static Scratch scr;
  scratch = &scr;
  game_reset();
  story_set_name("theLostShip");
  int w = world_add(0, 160, 440, 100, 1000, 0);
  perm_new(pcls_find("Forest"), w, 0, 0);
  perm_new(pcls_find("Rock"), w, 6, 0);
  perm_new(pcls_find("Forest"), w, 7, 0);
  uint16_t ship = perm_new(pcls_find("LandedExplorationShip"), w, 10, 0);
  uint16_t wc = perm_new(pcls_find("WoodcuttingCentre"), w, 3, 0);
  uint16_t h1 = perm_new(pcls_find("NormalHouse"), w, 4, 0);
  uint16_t h2 = perm_new(pcls_find("NormalHouse"), w, 4, 1);
  uint16_t sm = perm_new(pcls_find("StoneMine"), w, 5, 0);
  uint16_t farm = perm_new(pcls_find("IndoorFarm"), w, 5, 1);
  city_update_connections();
  for (int i = 0; i < 8; i++) cit_new(w, (float)(220 + i * 3), 20 + i, ship);
  cit_new(w, 100, 5, NONE);
  printf("perms %d citizens %d (ship %u wc %u houses %u %u mine %u farm %u)\n", perm_count(), ncits, ship, wc, h1, h2, sm, farm);
  for (int t = 0; t < ticks; t++) {
    sim_tick(1.0f);
    if (t % 600 == 0 || t == ticks - 1) {
      printf("--- tick %d day %d %.1fh\n", t, city_day(), city_hour());
      for (int i = 1; i < MAX_CITS; i++) {
        Cit *c = &cits[i];
        if (!(c->flags & CF_ALIVE)) continue;
        float x, y;
        cit_city_position(c, &x, &y);
        printf("  cit %d age %.1f home %u job %u school %u in %u at %.0f,%.0f %s\n", i, cit_age(c), c->home, c->job, c->school,
               c->inPermanent, x, y, action_name(c));
      }
    }
  }
  return 0;
}
