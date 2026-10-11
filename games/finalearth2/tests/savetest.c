/* Saves cities and loads them back: the same city, which then runs on. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../src/bld.h"
#include "../src/save.h"
#include "../src/scratch.h"
#include "../src/story.h"

void host_save_dir(const char *d);

static unsigned long city_sum(void) {
  unsigned long h = (unsigned long)ncits * 31 + (unsigned long)perm_count() * 7 + (unsigned long)city.timeSinceStart;
  for (int i = 1; i < MAX_CITS; i++)
    if (cits[i].flags & CF_ALIVE) h = h * 131 + cits[i].home * 3 + cits[i].job * 5 + cits[i].inPermanent;
  return h;
}

int main(int argc, char **argv) {
  static Scratch scr;
  scratch = &scr;
  const char *dir = argc > 1 ? argv[1] : "build/savetest_out";
  mkdir(dir, 0755);
  host_save_dir(dir);
  int bad = 0;
  for (int s = 0; s < nstories; s++) {
    rnd_seed(99 + s);
    story_new_city(s, NULL);
    for (int t = 0; t < 2000; t++) sim_tick(1.0f);
    unsigned long before = city_sum();
    double food = city.materials.m[M_FOOD];
    if (!save_game()) { printf("%s: save failed\n", stories[s].link); bad++; continue; }
    char p[512];
    snprintf(p, sizeof p, "%s/finalearth2.sav", dir);
    FILE *f = fopen(p, "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);
    story_new_city((s + 1) % nstories, NULL);   /* something else in between */
    /* every other city: the save gone, as after installing the app again; it comes back from the script's copy */
    bool restored = s % 2 == 1;
    if (restored) {
      remove(p);
      save_restore();
    }
    if (!load_game()) { printf("%s: load failed\n", stories[s].link); bad++; continue; }
    unsigned long after = city_sum();
    printf("%-26s save %5ld bytes, citizens %3d permanents %3d: %s%s\n", stories[s].link, size, ncits, perm_count(),
           before == after && food == city.materials.m[M_FOOD] ? "same city" : "DIFFERENT",
           restored ? " (from the copy)" : "");
    if (before != after) bad++;
    for (int t = 0; t < 1000; t++) sim_tick(1.0f);
  }
  printf("%s\n", bad ? "FAIL" : "all good");
  return bad != 0;
}
