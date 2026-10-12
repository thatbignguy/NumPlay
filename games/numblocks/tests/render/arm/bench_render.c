/* Bare-metal Cortex-M7 benchmark of the renderer, run by bench_render.py in Unicorn.
 * It follows the game's pattern: a world is settled around a spot, then a set of
 * fixed cameras is rendered (eye-level sweeps, high above looking down, horizon
 * from high up). phase is bumped after each frame so the runner can count
 * instructions per frame; sums[] holds a checksum of every pushed strip, so two
 * builds can be checked for pixel-identical output.
 *
 * Links every game source except main.c / plat_*.c; the platform is stubbed below. */
#include <stdint.h>
#include <string.h>
#include "../../../src/nb.h"
#include "../../../src/gen.h"

#ifndef SEED
#define SEED 12345
#endif
#ifndef NCAM
#define NCAM 18
#endif
#ifndef FANCY
#define FANCY 0
#endif
#ifndef RAIN
#define RAIN 0
#endif
#ifndef NMOBS
#define NMOBS 0
#endif
#ifndef NTICKS
#define NTICKS 0
#endif

volatile uint32_t phase;
volatile uint32_t sums[64];
volatile uint32_t npixels[64];

uint32_t ticks_run;
uint32_t game_time;
uint32_t perf_frames;

/* ---------------------------------------------------------------- platform stubs */
uint32_t plat_keys(void) { return 0; }
uint64_t plat_scan(void) { return 0; }
uint32_t plat_millis(void) { return 0; }
void plat_push(int x, int y, int w, int h, const uint16_t *px) {
  (void)x;
  (void)y;
  if (phase >= 64) return;
  uint32_t s = sums[phase];
  uint32_t n = npixels[phase];
  for (int i = 0; i < w * h; i++) s = s * 31 + px[i];
  sums[phase] = s;
  npixels[phase] = n + (uint32_t)(w * h);
}
void plat_pull(int x, int y, int w, int h, uint16_t *px) {
  (void)x;
  (void)y;
  memset(px, 0, (size_t)w * h * 2);
}
bool plat_save(const char *name, const void *data, uint32_t len) {
  (void)name;
  (void)data;
  (void)len;
  return true;
}
const uint8_t *plat_load(const char *name, uint32_t *len) {
  (void)name;
  *len = 0;
  return NULL;
}
void plat_remove_prefix(const char *prefix) { (void)prefix; }
uint32_t plat_storage_free(void) { return 1u << 30; }
bool plat_record(int i, char *name, int max) {
  (void)i;
  (void)name;
  (void)max;
  return false;
}
uint8_t *plat_save_open(const char *name, uint32_t len) {
  (void)name;
  (void)len;
  return NULL;
}
void plat_save_close(const char *name) { (void)name; }
bool plat_in_launcher(void) { return false; }
void plat_begin(void) {}
int plat_end(void) { return 0; }

/* ---------------------------------------------------------------- the benchmark */
static int fno;
static void frame(float x, float y, float z, float yaw, float pitch) {
  Camera c = {x, y, z, yaw, pitch};
  fno++;
  phase = (uint32_t)fno;
  render_frame(&c, game_time);
}

/* the generator's scratch: it lives for the whole of a call that makes the world, so the
 * gen_top calls here (outside any world_follow) each get their own */
static int top_at(int x, int z) {
  uint8_t gs[GEN_SCRATCH];
  gen_scratch(gs);
  int t = gen_top(x, z);
  gen_scratch(NULL);
  return t;
}

int main(void) {
  static uint8_t sumc_buf[GEN_CACHE_BYTES];   /* the summary cache: on main's stack in the game */
  gen_set_cache(sumc_buf);
  memset(&opt, 0, sizeof opt);
  opt.fancy = FANCY;
  opt.clouds = 1;
  opt.difficulty = 2;
  world_type = WT_DEFAULT;
  world_new(SEED, "nb9");  /* settle the cache around a spot on the ground */
  float px = 8.5f, pz = 8.5f;
  int gt = top_at(8, 8);
  float py = (float)gt + 2;
  for (int k = 0; k < 60 && (k < 3 || world_pending()); k++) world_follow(px, py, pz);
  while (world_pending()) world_follow(px, py, pz);
  int top = top_at(8, 8);
  float eye = (float)top + 1.62f, high = (float)top + 30;

  memset(&pl, 0, sizeof pl);
  pl.x = px;
  pl.y = py;
  pl.z = pz;
  pl.health = 20;
  pl.hit_face = -1;
  memset(ents, 0, sizeof ents);
#if NMOBS
  /* a zombie close ahead (south), a cow mid-range, a pig far, two dropped items */
  {
    Entity *z = ent_new(E_ZOMBIE, px, (float)top_at(8, 11) + 1, pz + 2.5f);
    Entity *c = ent_new(E_COW, px - 3.0f, (float)top_at(5, 14) + 1, pz + 6.0f);
    Entity *p = ent_new(E_PIG, px + 4.0f, (float)top_at(12, 20) + 1, pz + 12.0f);
    if (z) {
      z->health = 20;
      z->yaw = 180;
    }
    if (c) {
      c->health = 10;
      c->yaw = 90;
    }
    if (p) {
      p->health = 10;
      p->yaw = 270;
    }
    Entity *i1 = ent_drop(B_DIRT, 3, 0, px + 0.5f, (float)top_at(9, 10) + 1, pz + 1.5f, false);
    Entity *i2 = ent_drop(I_STICK, 2, 0, px - 1.0f, (float)top_at(7, 12) + 1, pz + 4.0f, false);
    if (i1) i1->age = 20;
    if (i2) i2->age = 20;
  }
#endif
  memset(parts, 0, sizeof parts);
  memset(&weather, 0, sizeof weather);
  rain_str = thunder_str = 0;
#if RAIN
  weather.raining = 1;
  weather.rain_time = 12000;
  rain_str = 1;
#endif
  bolt.on = 0;
  last_bolt = 0;
  game_time = 6000;
  ticks_run = 100;
  tick_frac = 0.5f;
  gui = GUI_NONE;
  phase = 1000;   /* settle is over: mark it so frame 1 starts clean */
  fno = 0;
  phase = 0;
#if NTICKS
  /* 60 world ticks with mobs about (no rendering): the tick cost between frames */
  for (int t = 0; t < 60; t++) {
    player_tick(0, 0);
    ents_tick();
    world_tick();
    gui_tick();
    hand_tick();
    ticks_run++;
    if (rule(GR_DAYLIGHT_CYCLE)) game_time++;
    fno++;
    phase = (uint32_t)(100 + fno);
  }
  fno = 0;
  phase = 0;
#endif

  /* eye level, looking around */
  for (int k = 0; k < 8; k++) frame(px, eye, pz, (float)(k * 45), 0);
  /* high above, looking straight down */
  frame(px, high, pz, 0, -90);
  frame(px, high, pz, 90, -90);
  /* high above, looking down at an angle */
  for (int k = 0; k < 4; k++) frame(px, high, pz, (float)(k * 90), -45);
  /* high above, at the horizon: the longest rays */
  frame(px, high, pz, 0, 0);
  frame(px, high, pz, 180, 0);
  /* on the ground, down at the terrain disc */
  frame(px, eye, pz, 0, -45);
  frame(px, eye, pz, 180, -60);
  return 0;
}

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
void Reset_Handler(void) {
  uint32_t *s = &_sidata, *d = &_sdata;
  while (d < &_edata) *d++ = *s++;
  for (d = &_sbss; d < &_ebss;) *d++ = 0;
  main();
  for (;;) __asm volatile("bkpt #0");
}

__attribute__((section(".isr_vector"), used)) const void *const vectors[2] = {&_estack, (void *)Reset_Handler};
