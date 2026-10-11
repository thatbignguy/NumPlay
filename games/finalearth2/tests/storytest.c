/* Starts every scenario and free play map without drawing, runs it a while,
 * and prints its worlds, permanents, citizens and current goal. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/bld.h"
#include "../src/scratch.h"
#include "../src/story.h"

int main(int argc, char **argv) {
  static Scratch scr;
  scratch = &scr;
  int ticks = argc > 1 ? atoi(argv[1]) : 2000;
  for (int s = 0; s < nstories; s++) {
    rnd_seed(1234 + s);
    story_new_city(s, NULL);
    int cols = 0;
    for (int w = 0; w < nworlds; w++) cols += worlds[w].ncols;
    printf("%-26s worlds %2d cols %3d perms %3d citizens %3d goal %s\n", stories[s].link, nworlds, cols, perm_count(), ncits,
           story_goal() ? story_goal()->name : "-");
    for (int t = 0; t < ticks; t++) sim_tick(1.0f);
    printf("%-26s after %d ticks: day %d citizens %d food %.1f wood %.1f stone %.1f goal %s happiness %.1f\n", "", ticks,
           city_day(), ncits, city.materials.m[M_FOOD], city.materials.m[M_WOOD], city.materials.m[M_STONE],
           story_goal() ? story_goal()->name : "-", happiness.happiness);
  }
  return 0;
}
