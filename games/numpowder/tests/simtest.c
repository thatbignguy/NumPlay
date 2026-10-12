/* NumPowder: the simulation checked on a computer (make test, with the sanitizers): scenes whose outcome The Powder
 * Toy is known for, the save's round trip, and long runs of a full grid of every element. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "simpriv.h"
#include "simsave.h"

int host_npart = 3625;   /* what the calculator gives on custom firmware (133652 bytes of RAM) */
static int fails, checks;
#define CHECK(cond, ...)                                     \
  do {                                                       \
    checks++;                                                \
    if (!(cond)) {                                           \
      printf("  FAIL %s:%d: ", __FILE__, __LINE__);          \
      printf(__VA_ARGS__);                                   \
      printf("\n");                                          \
      fails++;                                               \
    }                                                        \
  } while (0)

/* ---------------------------------------------------------------- helpers */
static void reset(void) {
  sim_clear();
  sim_default_settings();
  sim_settings_changed();
  rng_state[0] = 0x9E3779B97F4A7C15ull, rng_state[1] = 614;
  temp_dither = 0x2545F491;
}
static void run(int frames) {
  for (int f = 0; f < frames; f++) sim_step();
}
static int put(int x, int y, int t) { return create_part(-1, x, y, t, 0); }
static void fill(int x0, int y0, int x1, int y1, int t) {
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) put(x, y, t);
}
static void wallbox(int x0, int y0, int x1, int y1, int wall) { CreateWallBox(x0, y0, x1, y1, wall); }
static int count(int t) {
  int n = 0;
  for (int i = 0; i < parts_active; i++) n += parts[i].type == t;
  return n;
}
static int type_at(int x, int y) { return TYP(PMAPAT(x, y)); }
static int topmost(int x, int t) {   /* the highest row with t in column x, YRES if none */
  for (int y = 0; y < YRES; y++)
    if (type_at(x, y) == t) return y;
  return YRES;
}
/* the maps agree with the particles; ids, counts and positions are sane */
static bool consistent(const char *what) {
  int n = 0;
  for (int i = 0; i < parts_active; i++) {
    Particle *p = &parts[i];
    if (!p->type) continue;
    n++;
    if (p->type >= PT_COUNT) {
      printf("  %s: particle %d of type %d\n", what, i, p->type);
      return false;
    }
  }
  for (int y = 0; y < YRES; y++)
    for (int x = 0; x < XRES; x++) {
      int id = pmap_id[y][x] - 1;
      if (id >= 0) {
        Particle *p = &parts[id];
        if (!p->type || (elements[p->type].Properties & TYPE_ENERGY)) {
          printf("  %s: pmap at %d,%d holds %d (type %d)\n", what, x, y, id, p->type);
          return false;
        }
      }
      id = photons_id[y][x] - 1;
      if (id >= 0 && (!parts[id].type || !(elements[parts[id].type].Properties & TYPE_ENERGY))) {
        printf("  %s: photons at %d,%d holds %d (type %d)\n", what, x, y, id, parts[id].type);
        return false;
      }
    }
  if (n != NUM_PARTS) {
    printf("  %s: %d particles, NUM_PARTS %d\n", what, n, NUM_PARTS);
    return false;
  }
  return true;
}

/* ---------------------------------------------------------------- powders and liquids */
static void test_sand_pile(void) {
  puts("sand piles");
  reset();
  wallbox(10, 56, 89, 57, WL_WALL);
  fill(45, 10, 54, 29, PT_SAND);   /* 200 grains */
  run(400);
  CHECK(count(PT_SAND) == 200, "sand count %d", count(PT_SAND));
  /* a pile: the middle highest, falling off to the sides, no grain floating */
  int mid = topmost(50, PT_SAND), side = topmost(40, PT_SAND);
  CHECK(mid < 56 && mid < side, "pile top %d, side %d", mid, side);
  int width = 0;
  for (int x = 0; x < XRES; x++) width += type_at(x, 55) == PT_SAND;
  CHECK(width > 12 && width < 60, "pile base %d wide", width);
  bool floating = false;
  for (int y = 0; y < 55; y++)
    for (int x = 0; x < XRES; x++)
      if (type_at(x, y) == PT_SAND && !type_at(x, y + 1)) floating = true;
  CHECK(!floating, "a grain floats");
  /* the slope: TPT's sand piles at about 45 degrees (a grain slides when the two below it diagonally are free) */
  int h = 56 - mid;
  CHECK(h >= 4 && h <= width, "height %d for a base of %d", h, width);
  CHECK(consistent("sand"), "maps");
}

static void test_water_level(void) {
  puts("water levels in a U-tube");
  reset();
  /* a U: two arms 8 particles wide joined at the bottom */
  wallbox(20, 56, 61, 57, WL_WALL);      /* floor */
  wallbox(20, 10, 21, 55, WL_WALL);      /* left wall */
  wallbox(60, 10, 61, 55, WL_WALL);      /* right wall */
  wallbox(30, 10, 51, 47, WL_WALL);      /* the middle, leaving a passage below */
  fill(22, 12, 29, 47, PT_WATR);         /* the left arm full */
  int n = count(PT_WATR);
  run(3000);
  CHECK(count(PT_WATR) == n, "water count %d of %d", count(PT_WATR), n);
  int left = topmost(25, PT_WATR), right = topmost(56, PT_WATR);
  CHECK(abs(left - right) <= 2, "levels left %d right %d", left, right);
  CHECK(consistent("water"), "maps");
}

/* ---------------------------------------------------------------- heat and transitions */
static void test_transitions(void) {
  puts("lava cools to stone; water boils and freezes");
  reset();
  int l = put(50, 30, PT_LAVA);
  p_set_temp(&parts[l], 900.0f);   /* below 973 K: LAVA (no ctype) -> STNE */
  int w1 = put(30, 30, PT_WATR), w2 = put(70, 30, PT_WATR);
  p_set_temp(&parts[w1], 380.0f);   /* above 373 K: steam */
  p_set_temp(&parts[w2], 268.0f);   /* below 273.15 K: ice */
  wallbox(20, 50, 80, 51, WL_WALL);
  run(60);
  CHECK(parts[l].type == PT_STNE, "lava is %d", parts[l].type);
  CHECK(parts[w1].type == PT_WTRV, "hot water is %d", parts[w1].type);
  CHECK(parts[w2].type == PT_ICEI && parts[w2].ctype == PT_WATR, "cold water is %d (%d)", parts[w2].type, parts[w2].ctype);
  /* (alone: water wanders along a floor, and would boil on the stone the lava left) */
  reset();
  int w3 = put(40, 20, PT_WATR);
  p_set_temp(&parts[w3], 360.0f);   /* stays water */
  wallbox(20, 50, 80, 51, WL_WALL);
  run(60);
  CHECK(parts[w3].type == PT_WATR, "warm water is %d", parts[w3].type);
  /* a block of lava in cold metal: the metal takes its heat, the lava turns to stone */
  reset();
  wallbox(10, 56, 89, 57, WL_WALL);
  fill(40, 40, 59, 55, PT_METL);
  fill(47, 46, 52, 49, PT_LAVA);   /* (made inside the metal: replacing) */
  for (int y = 46; y <= 49; y++)
    for (int x = 47; x <= 52; x++) {
      int i = pmap_id[y][x] - 1;
      if (i >= 0) create_part(i, x, y, PT_LAVA, 0), parts[i].ctype = PT_STNE, p_set_temp(&parts[i], 1500.0f);
    }
  int lava = count(PT_LAVA);
  run(800);
  CHECK(lava == 24 && count(PT_LAVA) == 0 && count(PT_STNE) == 24, "lava %d -> %d, stone %d", lava, count(PT_LAVA), count(PT_STNE));
  /* heat conducts along metal */
  reset();
  fill(10, 30, 89, 30, PT_METL);
  int hot = pmap_id[30][10] - 1, far = pmap_id[30][40] - 1;
  p_set_temp(&parts[hot], 2000.0f);
  float before = p_temp(&parts[far]);
  for (int f = 0; f < 600; f++) {
    p_set_temp(&parts[hot], 2000.0f);
    sim_step();
  }
  CHECK(p_temp(&parts[far]) > before + 20, "30 particles away: %.2f K from %.2f", p_temp(&parts[far]), before);
}

/* ---------------------------------------------------------------- electronics */
static void test_sparks(void) {
  puts("sparks through metal, PSCN and NSCN, INST");
  reset();
  fill(20, 30, 79, 30, PT_METL);
  create_part(-2, 20, 30, PT_SPRK, 0);   /* the brush sparks a conductor */
  CHECK(parts[pmap_id[30][20] - 1].type == PT_SPRK, "first spark");
  int reached = -1;
  for (int f = 1; f <= 200 && reached < 0; f++) {
    sim_step();
    if (type_at(79, 30) == PT_SPRK) reached = f;
  }
  /* SPRK reaches 2 particles a frame along a line (its range), so 60 particles in about 30 frames */
  CHECK(reached > 0 && reached <= 60, "the spark reached the end at frame %d", reached);
  run(10);
  /* PSCN -> NSCN conducts, NSCN -> PSCN doesn't */
  reset();
  /* (a spark reaches 2 particles away: the chains are longer than that) */
  put(30, 30, PT_PSCN), fill(31, 30, 34, 30, PT_NSCN), put(35, 30, PT_METL);
  put(60, 30, PT_NSCN), fill(61, 30, 64, 30, PT_PSCN), put(65, 30, PT_METL);
  create_part(-2, 30, 30, PT_SPRK, 0);
  create_part(-2, 60, 30, PT_SPRK, 0);
  bool metalA = false, metalB = false;
  for (int f = 0; f < 12; f++) {
    sim_step();
    metalA |= type_at(35, 30) == PT_SPRK;
    metalB |= type_at(65, 30) == PT_SPRK || type_at(61, 30) == PT_SPRK;
  }
  CHECK(metalA, "PSCN -> NSCN -> METL");
  CHECK(!metalB, "NSCN -> PSCN conducted");
  /* PSCN turns SWCH on, NSCN off */
  reset();
  put(30, 30, PT_PSCN), put(31, 30, PT_SWCH);
  create_part(-2, 30, 30, PT_SPRK, 0);
  run(3);
  CHECK(parts[pmap_id[30][31] - 1].life == 10, "SWCH life %d", parts[pmap_id[30][31] - 1].life);
  /* INST conducts at once: through a T junction, not across a one-particle crossing */
  reset();
  fill(10, 30, 89, 30, PT_INST);
  fill(50, 31, 50, 50, PT_INST);   /* a T below */
  fill(70, 10, 70, 29, PT_INST), fill(70, 31, 70, 50, PT_INST);   /* a crossing */
  create_part(-2, 10, 30, PT_SPRK, 0);   /* the brush floods it */
  CHECK(count(PT_SPRK) == 80 + 20, "INST sparked at once: %d of 100", count(PT_SPRK));
  run(30);
  CHECK(count(PT_INST) == 140, "back to INST: %d", count(PT_INST));
  /* a battery lights a line of metal through to NSCN... and a spark-powered wall */
  reset();
  put(30, 30, PT_BTRY), fill(31, 30, 40, 30, PT_METL);
  run(20);
  CHECK(count(PT_SPRK) > 0, "BTRY sparks");
  CHECK(consistent("sparks"), "maps");
}

/* ---------------------------------------------------------------- chemistry, fire, explosives */
static void test_acid_fire_c4(void) {
  puts("acid dissolves; fire burns wood out; C-4 explodes with pressure");
  reset();
  wallbox(10, 56, 89, 57, WL_WALL);
  fill(30, 46, 69, 55, PT_DUST);
  int dust = count(PT_DUST);
  fill(40, 30, 59, 40, PT_ACID);
  run(400);
  CHECK(count(PT_DUST) < dust / 2, "dust left %d of %d", count(PT_DUST), dust);
  reset();
  fill(30, 30, 69, 40, PT_WOOD);
  int wood = count(PT_WOOD);
  fill(29, 30, 29, 40, PT_FIRE);
  bool spread = false;
  for (int f = 0; f < 4000; f++) {
    sim_step();
    if (count(PT_WOOD) < wood * 3 / 4) spread = true;
  }
  CHECK(spread, "fire didn't spread: wood %d of %d", count(PT_WOOD), wood);
  CHECK(count(PT_FIRE) == 0, "fire still burning: %d", count(PT_FIRE));
  reset();
  wallbox(10, 56, 89, 57, WL_WALL);
  fill(40, 40, 59, 55, PT_PLEX);
  run(20);
  CHECK(count(PT_PLEX) == 320, "C-4 went off on its own");
  for (int f = 0; f < 5; f++) pv[44 / ACELL][50 / ACELL] = 10.0f, sim_step();   /* pressure (as the AIR tool) */
  float pmax = 0;   /* (the blast's pressure, the most it reaches) */
  for (int f = 0; f < 60; f++) {
    sim_step();
    for (int y = 0; y < YACELLS; y++)
      for (int x = 0; x < XACELLS; x++) pmax = fmaxf(pmax, fabsf(pv[y][x]));
  }
  CHECK(count(PT_PLEX) == 0, "C-4 left %d", count(PT_PLEX));
  CHECK(pmax > 5.0f, "the blast's pressure %.2f", pmax);
  CHECK(consistent("explosives"), "maps");
}

/* ---------------------------------------------------------------- light and walls */
static void test_light_walls(void) {
  puts("photons pass through glass; walls block");
  reset();
  fill(50, 20, 53, 40, PT_GLAS);
  for (int y = 25; y <= 35; y++) {
    int i = create_part(-3, 20, y, PT_PHOT, 0);
    p_set_vx(&parts[i], 3), p_set_vy(&parts[i], 0);
  }
  run(18);
  int beyond = 0;
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type == PT_PHOT && p_x(&parts[i]) > 54) beyond++;
  CHECK(beyond >= 8, "%d photons went through the glass", beyond);
  /* a photon into metal reflects */
  reset();
  fill(50, 20, 52, 40, PT_METL);
  int ph = create_part(-3, 30, 30, PT_PHOT, 0);
  p_set_vx(&parts[ph], 3), p_set_vy(&parts[ph], 0);
  run(20);
  CHECK(parts[ph].type == PT_PHOT && p_vx(&parts[ph]) < 0, "the photon came back: type %d vx %.2f", parts[ph].type, p_vx(&parts[ph]));
  /* walls: sand stops on a wall, water passes a liquid wall, sand doesn't */
  reset();
  wallbox(10, 40, 49, 41, WL_WALL);
  wallbox(50, 40, 89, 41, WL_ALLOWLIQUID);
  wallbox(10, 56, 89, 57, WL_WALL), wallbox(10, 42, 11, 55, WL_WALL), wallbox(88, 42, 89, 55, WL_WALL);
  fill(20, 20, 29, 25, PT_SAND), fill(60, 20, 69, 25, PT_SAND), fill(75, 20, 84, 25, PT_WATR);
  run(300);
  int below = 0, sandBelow = 0;
  for (int y = 42; y < YRES; y++)
    for (int x = 0; x < XRES; x++) below += type_at(x, y) == PT_WATR, sandBelow += type_at(x, y) == PT_SAND;
  CHECK(below >= 50, "water through the liquid wall: %d of 60", below);
  CHECK(sandBelow == 0, "sand through a wall: %d", sandBelow);
  /* an absorb wall eats what falls in */
  reset();
  wallbox(10, 50, 89, 51, WL_DESTROYALL);
  fill(40, 20, 49, 29, PT_SAND);
  run(200);
  CHECK(count(PT_SAND) == 0, "the absorb wall left %d", count(PT_SAND));
  /* walls block air: pressure stays inside a box */
  reset();
  wallbox(36, 20, 63, 21, WL_WALL), wallbox(36, 42, 63, 43, WL_WALL);
  wallbox(36, 20, 37, 43, WL_WALL), wallbox(62, 20, 63, 43, WL_WALL);
  for (int k = 0; k < 4; k++) pv[32 / ACELL][48 / ACELL + k - 1] = 50;
  run(100);
  CHECK(pv[32 / ACELL][48 / ACELL] > 3 && fabsf(pv[10 / ACELL][20 / ACELL]) < 0.5f, "inside %.1f, outside %.1f",
        pv[32 / ACELL][48 / ACELL], pv[10 / ACELL][20 / ACELL]);
  CHECK(consistent("walls"), "maps");
}

/* ---------------------------------------------------------------- Game of Life */
static bool life_at(int x, int y) { return type_at(x, y) == PT_LIFE; }
static void test_life(void) {
  puts("Game of Life: blinker, glider");
  reset();
  put(49, 30, PT_LIFE), put(50, 30, PT_LIFE), put(51, 30, PT_LIFE);   /* a blinker (GOL, v 0) */
  run(1);
  CHECK(life_at(50, 29) && life_at(50, 30) && life_at(50, 31) && !life_at(49, 30) && !life_at(51, 30), "blinker vertical");
  run(1);
  CHECK(life_at(49, 30) && life_at(50, 30) && life_at(51, 30) && !life_at(50, 29) && !life_at(50, 31), "blinker horizontal");
  CHECK(count(PT_LIFE) == 3, "blinker cells %d", count(PT_LIFE));
  reset();
  /* a glider going down and right:  .#. / ..# / ### */
  put(31, 20, PT_LIFE), put(32, 21, PT_LIFE), put(30, 22, PT_LIFE), put(31, 22, PT_LIFE), put(32, 22, PT_LIFE);
  run(8);
  CHECK(count(PT_LIFE) == 5, "glider cells %d", count(PT_LIFE));
  CHECK(life_at(33, 22) && life_at(34, 23) && life_at(32, 24) && life_at(33, 24) && life_at(34, 24), "glider moved 2,2");
  /* another rule: HLIF (B36/S23) by its number */
  reset();
  create_part(-1, 50, 30, PT_LIFE, 1);
  CHECK(parts[pmap_id[30][50] - 1].ctype == 1, "HLIF ctype");
  char rule[24];
  SerialiseGOLRule(builtinGol[1].ruleset, rule);
  CHECK(!strcmp(rule, "B36/S23") && ParseGOLString("B36/S23") == builtinGol[1].ruleset, "rule %s", rule);
  CHECK(ParseGOLString("B37/S3458/4") == builtinGol[19].ruleset, "LOTE");
}

/* ---------------------------------------------------------------- what the audit found (P1.2, P1.3, P1.4, P1.7, P2.2) */
static void test_audit(void) {
  puts("velocities decay, LIGH's and PHOT's numbers, HEAT on PUMP, whole degrees");
  /* a slow particle's velocity times Loss decays to 0 (rounded to the nearest it stayed at 10/256 for ever) */
  reset();
  settings.gravity_mode = GRAV_OFF, settings.air_mode = AIR_OFF;
  sim_settings_changed();
  int s = put(20, 30, PT_SAND);
  parts[s].vx = 64;   /* 0.25 a frame */
  run(300);
  float x1 = p_x(&parts[s]);
  run(100);
  CHECK(parts[s].vx == 0 && parts[s].vy == 0 && p_x(&parts[s]) == x1 && x1 > 23 && x1 < 29,
        "sand slid to %.2f, then %.2f, at %.4f", x1, p_x(&parts[s]), p_vx(&parts[s]));

  /* LIGH's powderful is an int (LIGH.cpp): its heat into the air is a whole number of kelvins */
  reset();
  settings.ambient_heat = true;
  sim_settings_changed();
  int l = put(50, 30, PT_LIGH);
  parts[l].life = 0, parts[l].tmp2 = 0;
  p_set_temp(&parts[l], 3010.0f);
  hv[30 / ACELL][50 / ACELL] = 300.0f;
  int powderful = (int)(p_temp(&parts[l]) * 0.65f);
  elements[PT_LIGH].Update(l, 50, 30, 8, 8);
  float dh = hv[30 / ACELL][50 / ACELL] - 300.0f;
  CHECK(powderful % 50 && dh == (float)(powderful / 50), "air heated by %.3f (powderful %d)", dh, powderful);

  /* PHOT turning ISOZ into PHOT: the new one's angle is any whole degree (an int angle was a whole radian) */
  int made = 0, off = 0;
  for (unsigned seed = 1; seed < 6000 && made < 12; seed++) {
    reset();
    int ph = create_part(-3, 50, 30, PT_PHOT, 0), iz = put(51, 30, PT_ISOZ);
    rng_seed(seed);
    elements[PT_PHOT].Update(ph, 50, 30, 8, 8);
    if (parts[iz].type != PT_PHOT || (!parts[iz].vx && !parts[iz].vy)) continue;
    made++;
    float a = atan2f(p_vy(&parts[iz]), p_vx(&parts[iz]));
    if (a < 0) a += 2 * NP_PI;
    off += fabsf(a - roundf(a)) > 0.05f;
  }
  CHECK(made >= 8 && off > 0, "%d photons made, %d not at a whole radian", made, off);

  /* HEAT on PUMP: 0.1 K a time, kept on average (dithered) */
  reset();
  int pu = put(50, 30, PT_PUMP);
  p_set_temp(&parts[pu], 300.0f);
  Brush b = brush;
  brush.rx = brush.ry = 0;
  for (int k = 0; k < 100; k++) sim_tool_points(TOOL_SIMTOOLS + TOOL_HEAT, 50, 30, 50, 30, false);
  brush = b;
  CHECK(fabsf(p_temp(&parts[pu]) - 310.0f) < 2.0f, "PUMP heated to %.2f K", p_temp(&parts[pu]));

  /* whole degrees Celsius are kept exactly, and the ends */
  Particle q;
  p_set_temp(&q, 295.15f);
  float t22 = p_temp(&q);
  p_set_temp(&q, elements[PT_WATR].DefaultProperties.temp);
  float tw = p_temp(&q);
  p_set_temp(&q, MAX_TEMP);
  float tmax = p_temp(&q);
  p_set_temp(&q, MIN_TEMP);
  float tmin = p_temp(&q);
  CHECK(t22 == 295.15f && tw == 293.15f && tmax == MAX_TEMP && tmin == MIN_TEMP, "22 C: %.4f, water %.4f, %.2f, %.2f",
        t22, tw, tmax, tmin);
}

/* ---------------------------------------------------------------- saves */
static uint8_t save_a[1 << 16], save_b[1 << 16];
static void mix(uint32_t *h, const void *p, size_t n) {
  for (size_t k = 0; k < n; k++) *h = (*h ^ ((const uint8_t *)p)[k]) * 16777619u;
}
static uint32_t state_hash(void) {
  uint32_t h = 2166136261u;
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type) mix(&h, &i, sizeof i), mix(&h, &parts[i], sizeof(Particle));
  mix(&h, pv, sizeof pv), mix(&h, vx, sizeof vx), mix(&h, vy, sizeof vy), mix(&h, hv, sizeof hv);
  mix(&h, bmap, sizeof bmap), mix(&h, emap, sizeof emap), mix(&h, rng_state, sizeof rng_state);
  return h;
}
static uint32_t full_hash(void) {
  uint32_t h = state_hash();
  mix(&h, gravx, sizeof gravx), mix(&h, gravy, sizeof gravy), mix(&h, &temp_dither, sizeof temp_dither);
  mix(&h, &NUM_PARTS, sizeof NUM_PARTS), mix(&h, &parts_active, sizeof parts_active);
  return h;
}
static uint32_t every_hash[2];   /* test_every_element's, after its long run and after the brim */
static void busy_scene(void) {
  reset();
  wallbox(4, 58, 95, 59, WL_WALL);
  fill(10, 40, 30, 57, PT_WATR), fill(35, 30, 45, 40, PT_SAND), fill(50, 50, 60, 57, PT_WOOD);
  fill(49, 45, 61, 45, PT_FIRE), fill(70, 20, 80, 30, PT_GAS), fill(65, 50, 90, 52, PT_METL);
  put(65, 49, PT_BTRY), fill(84, 40, 88, 44, PT_PLNT), fill(20, 10, 22, 12, PT_LIFE);
  int ph = create_part(-3, 15, 15, PT_PHOT, 0);
  p_set_vx(&parts[ph], 2), p_set_vy(&parts[ph], 1);
  put(15, 30, PT_STKM);   /* (over the water: away from the fire and the singularity, it lives) */
  fill(86, 10, 90, 14, PT_VIRS), fill(5, 20, 8, 22, PT_SEED);
  settings.ambient_heat = true;
  settings.newtonian_gravity = true;
  sim_settings_changed();
  put(40, 15, PT_SING), put(60, 15, PT_GRVT);
  run(120);
}
/* a save, written to buf; its size */
static uint32_t save_to(uint8_t *buf, uint32_t room) {
  uint32_t n = simsave_size();
  if (n > room) return 0;
  return simsave_write(buf, n) == n ? n : 0;
}
static int soap_links(void) {
  int n = 0;
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type == PT_SOAP && (parts[i].ctype & 2)) {
      int j = parts[i].tmp;
      n += j >= 0 && j < parts_active && parts[j].type == PT_SOAP && (parts[j].ctype & 4) && parts[j].tmp2 == i;
    }
  return n;
}
static void test_saves(void) {
  puts("saves round-trip");
  busy_scene();
  uint32_t h0 = state_hash();
  uint32_t n = simsave_size();
  CHECK(n > 0 && n < sizeof save_a, "save of %u bytes", n);
  CHECK(simsave_write(save_a, n) == n, "written");
  CHECK(simsave_write(save_b, n - 1) == 0, "too small a room");
  CHECK(state_hash() == h0, "saving changes nothing");
  CHECK(simsave_count(save_a, n) == NUM_PARTS, "count %d of %d", simsave_count(save_a, n), NUM_PARTS);
  int stkm = count(PT_STKM), parts0 = NUM_PARTS;
  CHECK(stkm == 1, "the stickman is there to save (%d)", stkm);
  player.rocketBoots = true;
  n = save_to(save_a, sizeof save_a);
  uint32_t busy = n;
  run(50);
  CHECK(simsave_read(save_a, n), "read");
  CHECK(consistent("loaded"), "maps after loading");
  CHECK(NUM_PARTS == parts0 && count(PT_STKM) == stkm && player.rocketBoots, "%d particles of %d, stickman %d, boots %d", NUM_PARTS, parts0, count(PT_STKM), player.rocketBoots);
  /* saved again, the same (but for the stickman's SPAWN: loading a stickman makes one where it is, as the
   * original's Load does through create_part, and the one saved is then left out; from then on the same) */
  uint32_t n2 = save_to(save_b, sizeof save_b);
  CHECK(simsave_read(save_b, n2), "read the second save");
  static uint8_t save_c[1 << 16];
  uint32_t n3 = save_to(save_c, sizeof save_c);
  CHECK(n3 == n2 && !memcmp(save_b, save_c, n2), "saved again: %u bytes of %u, the same %d", n3, n2, n3 == n2 && !memcmp(save_b, save_c, n2));
  CHECK(n2 + 8 > n && n2 < n + 8, "the first save again: %u bytes of %u", n2, n);
  /* a save loaded twice goes on the same way */
  CHECK(simsave_read(save_a, n), "read the first save");
  run(200);
  uint32_t a = state_hash();
  CHECK(simsave_read(save_a, n), "read again");
  run(200);
  CHECK(state_hash() == a, "200 frames on from the same save are the same");
  /* SOAP keeps its links (bubbles are made by pressure and last a few frames), PIPE what it carries */
  reset();
  wallbox(4, 58, 95, 59, WL_WALL);
  fill(30, 30, 45, 40, PT_SOAP);
  for (int y = 30 / ACELL; y <= 40 / ACELL; y++)
    for (int x = 30 / ACELL; x <= 45 / ACELL; x++) pv[y][x] = 2;
  run(8);
  int links = soap_links();
  n = save_to(save_a, sizeof save_a);
  CHECK(simsave_read(save_a, n), "SOAP read");
  CHECK(links > 20 && soap_links() == links, "SOAP links %d of %d", soap_links(), links);
  n2 = save_to(save_b, sizeof save_b);
  CHECK(n2 == n && !memcmp(save_a, save_b, n), "SOAP saved again the same");
  /* pipes with water and photons in them (all 30 bits of wavelengths: the top byte of tmp4) */
  reset();
  fill(20, 50, 70, 50, PT_PIPE);
  run(3);
  for (int x = 20; x <= 70; x += 2) {
    int pp = PMAPAT(x, 50), q = create_part(-3, x, 45, x % 4 ? PT_WATR : PT_PHOT, -1);
    if (TYP(pp) != PT_PIPE || q < 0) continue;
    parts[q].ctype = x % 4 ? 0 : 0x3FFFFFFF;
    Element_PIPE_transfer_part_to_pipe(&parts[q], &parts[ID(pp)]);
    kill_part(q);
  }
  run(3);
  int pipe = 0, photons = 0;
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type == PT_PIPE && TYP(parts[i].ctype)) pipe++, photons += TYP(parts[i].ctype) == PT_PHOT && tmp4_get(i) == 0x3FFFFFFF;
  n = save_to(save_a, sizeof save_a);
  CHECK(simsave_read(save_a, n), "PIPE read");
  int pipe2 = 0, photons2 = 0;
  for (int i = 0; i < parts_active; i++)
    if (parts[i].type == PT_PIPE && TYP(parts[i].ctype)) pipe2++, photons2 += TYP(parts[i].ctype) == PT_PHOT && tmp4_get(i) == 0x3FFFFFFF;
  CHECK(pipe > 20 && pipe2 == pipe && photons > 5 && photons2 == photons, "pipes carrying %d of %d, photons %d of %d", pipe2, pipe, photons2, photons);
  n2 = save_to(save_b, sizeof save_b);
  CHECK(n2 == n && !memcmp(save_a, save_b, n), "PIPE saved again the same");
  /* a damaged save is refused and changes nothing; a garbled one doesn't break anything */
  uint32_t h = state_hash();
  CHECK(!simsave_read(save_a, n / 2), "a cut save refused");
  CHECK(state_hash() == h, "unchanged by a bad save");
  memcpy(save_b, save_a, n);
  save_b[0] = 'X';
  CHECK(!simsave_read(save_b, n), "a bad magic refused");
  int garbled = 0;
  for (int k = 0; k < 300; k++) {
    memcpy(save_b, save_a, n);
    for (int j = 0; j < 1 + k % 4; j++) save_b[5 + rng_between(0, (int)n - 6)] ^= (uint8_t)rng_between(1, 255);
    if (simsave_read(save_b, n)) {
      garbled++;
      run(3);
      if (!consistent("garbled")) break;
    }
  }
  printf("  (%d of 300 garbled saves loaded)\n", garbled);
  /* sand drawn is about a byte a grain, settled too */
  reset();
  fill(10, 20, 89, 49, PT_SAND);
  wallbox(4, 50, 95, 51, WL_WALL);
  uint32_t drawn = simsave_size();
  CHECK(drawn < (uint32_t)NUM_PARTS + 600, "%d grains drawn in %u bytes", NUM_PARTS, drawn);
  run(150);
  uint32_t s = simsave_size();
  int grains = NUM_PARTS;
  CHECK(s < (uint32_t)NUM_PARTS * 2 + 600, "%d grains settled in %u bytes", NUM_PARTS, s);
  reset();
  wallbox(4, 58, 95, 59, WL_WALL), wallbox(4, 30, 5, 57, WL_WALL), wallbox(94, 30, 95, 57, WL_WALL);
  fill(6, 40, 93, 57, PT_WATR);
  run(150);
  uint32_t water = simsave_size();
  printf("  (busy scene: %u bytes, %d particles; sand: %u bytes drawn, %u settled, %d grains; water: %u bytes, %d)\n", busy, parts0, drawn, s,
         grains, water, NUM_PARTS);
}

/* ---------------------------------------------------------------- every element, a full grid, a long run */
static void test_every_element(void) {
  puts("every element on a full grid, a long run");
  reset();
  int t = 1;
  for (int y = BORDER; y < YRES - BORDER; y += 4)
    for (int x = BORDER; x < XRES - BORDER; x += 4) {
      while (t < PT_COUNT && !elements[t].Enabled) t++;
      if (t >= PT_COUNT) t = 1;
      for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++) put(x + i, y + j, t);
      t++;
    }
  /* walls of every kind along the bottom */
  for (int w = 1; w < UI_WALLCOUNT; w++)
    if (w != WL_ERASEALL) CreateWalls(BORDER + w * 4, YRES - BORDER - 2, 1, 1, w, false);
  CHECK(consistent("placed"), "maps");
  int frames = 3000;
  for (int f = 0; f < frames; f++) {
    if (f % 500 == 0) {
      settings.ambient_heat = f % 1000 == 0;
      settings.newtonian_gravity = f % 1500 == 0;
      settings.gravity_mode = (uint8_t)(f / 500 % 4);
      settings.edge_mode = (uint8_t)(f / 500 % 3);
      settings.water_equalization = f % 1000 != 0;
      sim_settings_changed();
    }
    sim_step();
    if (f % 250 == 0 && !consistent("running")) {
      CHECK(false, "maps at frame %d", f);
      break;
    }
  }
  CHECK(consistent("after"), "maps");
  every_hash[0] = full_hash();
  printf("  (%d particles after %d frames)\n", NUM_PARTS, frames);
  /* filled to the brim: all particles, every cell */
  reset();
  for (int y = BORDER; y < YRES - BORDER; y++)
    for (int x = BORDER; x < XRES - BORDER; x++) put(x, y, 1 + (x * 7 + y * 13) % (PT_COUNT - 1));
  run(1000);
  CHECK(consistent("brim"), "maps");
  every_hash[1] = full_hash();
  printf("  (brim: %d particles, %d the most)\n", NUM_PARTS, npart_max);
}

/* ---------------------------------------------------------------- the same simulation after the speed work
 * The speed tests' scenes (sim.c's sim_scene 1 to 6, main.c's TEST_SCENE 1 and 2) and the full grid of every element
 * (test_every_element), run for a long time: the state's hash must be the reference's (expect[]). The speed work kept
 * the hashes the code had before it; the audit's fixes (dithered velocities, the temperatures' encoding, LIGH, PHOT,
 * WARP, VIRS, MORT...) changed the simulation on purpose and made these. A change that means to change the
 * simulation updates them (make test prints the new ones). */
static void test_scenes_unchanged(void) {
  puts("the speed tests' scenes, a long run: the reference simulation");
  static const uint32_t expect[11] = {0, 0x9b1d32c0, 0xf1995947, 0xa45ef928, 0x96ba526e, 0xae6a35fc, 0x892c76fd, 0x7e5748cd, 0x528dc52b, 0x47e98ea7, 0x11c96084};
  uint32_t got[11] = {0};
  for (int n = 1; n <= 8; n++) {
    reset();
    currentTick = 0, CGOL = 0;
    if (n <= 6) {
      sim_scene(n);
    } else {
      /* main.c's TEST_SCENE: 7 still elements, 8 with fire, plasma and lava */
      static const uint8_t still[] = {PT_STNE, PT_GLAS, PT_METL, PT_WOOD, PT_BRCK, PT_DMND, PT_IRON, PT_CNCT};
      static const uint8_t hot[] = {PT_SAND, PT_WATR, PT_STNE, PT_OIL, PT_GLAS, PT_LAVA, PT_FIRE, PT_PLSM, PT_METL, PT_SALT};
      for (int y = 4; y < YRES - 4; y++)
        for (int x = 2; x < XRES - 2; x++) {
          int k = x / 10 + y / 16 * 3;
          create_part(-1, x, y, n == 7 ? still[k % sizeof still] : hot[k % sizeof hot], -1);
        }
    }
    run(400);
    got[n] = full_hash();
  }
  got[9] = every_hash[0], got[10] = every_hash[1];
  for (int n = 1; n <= 10; n++) CHECK(got[n] == expect[n], "scene %d: hash %08x, %08x before", n, got[n], expect[n]);
  printf("  (hashes: 0");
  for (int n = 1; n <= 10; n++) printf(", 0x%08x", got[n]);
  printf(")\n");
}

/* ---------------------------------------------------------------- speed on the computer (a rough guide) */
static void test_speed(void) {
  reset();
  wallbox(4, 58, 95, 59, WL_WALL);
  fill(5, 20, 94, 57, PT_WATR);
  fill(20, 5, 80, 19, PT_SAND);
  clock_t c = clock();
  run(300);
  double ms = (double)(clock() - c) * 1000.0 / CLOCKS_PER_SEC / 300;
  printf("speed: %d particles, %.2f ms a frame on this computer (with the sanitizers)\n", NUM_PARTS, ms);
}

int main(int argc, char **argv) {
  sim_init();
  printf("NumPowder simtest: %d particles of room\n", npart_max);
  test_sand_pile();
  test_water_level();
  test_transitions();
  test_sparks();
  test_acid_fire_c4();
  test_light_walls();
  test_life();
  test_audit();
  test_saves();
  test_every_element();
  test_scenes_unchanged();
  if (argc > 1 && !strcmp(argv[1], "speed")) test_speed();
  printf("%d checks, %d failed\n", checks, fails);
  return fails != 0;
}
