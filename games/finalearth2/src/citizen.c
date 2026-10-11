/* The Final Earth 2: citizens (Citizen and simulation.CitizenEntertainment).
 *
 * A citizen is on a world's surface (inPermanent 0, relativeX from the
 * world's left) or in a permanent (relativeX from its left, relativeY up from
 * its floor). What they do comes from their path: a list of steps, each a type
 * and a value. Steps from the pathfinder walk between permanents, steps from
 * scripts move, wait or change floor in place:
 *   0 up, 1 down, 2 left, 3 right (to the neighbouring building)
 *   4 move to x, 5 to a permanent's door from outside (or out of one)
 *   7 teleport to, 8 wait, 9 change floor to y, 10 nothing, 11 fly to (landing sites)
 *   12 set the walking speed (percent), 13 hyper elevator to, 15/16 bridge left/right */
#include "sim.h"
#include "bld.h"
#include "scratch.h"

Cit cits[MAX_CITS];
int ncits;
ThenFn then_fns[THEN_COUNT_];
static uint8_t nthen = 1;
uint8_t then_register(void (*fn)(Cit *c, uint16_t arg)) {
  if (nthen >= THEN_COUNT_) return THEN_NONE;
  then_fns[nthen] = fn;
  return nthen++;
}

static inline uint16_t now_ticks(void) { return (uint16_t)(int64_t)(city.timeSinceStart * 2.0); }
static inline uint16_t now_minutes(void) { return (uint16_t)(int64_t)city.timeSinceStart; }
/* the entertainment times: 2-hour and 4-minute units that wrap (kept near now by cit_infrequent_update) */
static inline uint8_t now_2h(void) { return (uint8_t)(int64_t)(city.timeSinceStart / 120.0); }
static inline uint8_t now_4m(void) { return (uint8_t)(int64_t)(city.timeSinceStart / 4.0); }
static inline uint8_t at_2h(double t) {
  double d = (t - city.timeSinceStart) / 120.0;
  if (d > 120) d = 120;
  if (d < -120) d = -120;
  return (uint8_t)(now_2h() + (int)floor(d + (t >= city.timeSinceStart ? 0.999 : 0)));
}
static inline uint8_t at_4m(double t) {
  double d = (t - city.timeSinceStart) / 4.0;
  if (d > 120) d = 120;
  if (d < -120) d = -120;
  return (uint8_t)(now_4m() + (int)floor(d + (t >= city.timeSinceStart ? 0.999 : 0)));
}
static inline bool past_2h(uint8_t v) { return (int8_t)(now_2h() - v) >= 0; }
static inline bool past_4m(uint8_t v) { return (int8_t)(now_4m() - v) >= 0; }
/* a past time far back moves up to 64 units back, so it never wraps around to look ahead */
static inline void keep_near(uint8_t *v, uint8_t now) { if ((int8_t)(now - *v) > 64) *v = (uint8_t)(now - 64); }
static inline void die_add(Cit *c, float years) {
  float v = c->dieAgeModifier + years * 100;
  c->dieAgeModifier = (int16_t)(v > 32000 ? 32000 : v < -32000 ? -32000 : v);
}
static inline bool in_hours(float t, float start, float end) {
  return start < end ? t >= start && t < end : t >= start || t < end;
}

/* ---------------------------------------------------------------- the path pool
 * every citizen's steps, packed in one array: a step is its type (5 bits)
 * and its value + 256 (11 bits) */
#define PATH_MEM 3072
static uint16_t path_mem[PATH_MEM];
static int path_top = 1;   /* 0 is never used, so pathOff 0 means none */

static inline uint16_t step_pack(int type, int value) {
  int v = value + 256;
  if (v < 0) v = 0;
  if (v > 2047) v = 2047;
  return (uint16_t)(type << 11 | v);
}
static inline int step_type(uint16_t s) { return s >> 11; }
static inline int step_value(uint16_t s) { return (int)(s & 2047) - 256; }

static int cmp_off(const void *a, const void *b);
static void path_compact(void) {
  /* move every live path down, in order of where it is */
  uint16_t *order = scratch->u.compact.order;
  int n = 0;
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].pathOff) order[n++] = (uint16_t)i;
  /* insertion sort by offset (paths are mostly in order already) */
  for (int i = 1; i < n; i++) {
    uint16_t k = order[i];
    int j = i - 1;
    while (j >= 0 && cits[order[j]].pathOff > cits[k].pathOff) order[j + 1] = order[j], j--;
    order[j + 1] = k;
  }
  int top = 1;
  for (int i = 0; i < n; i++) {
    Cit *c = &cits[order[i]];
    if (c->pathOff != top) memmove(path_mem + top, path_mem + c->pathOff, sizeof path_mem[0] * c->pathLen);
    c->pathOff = (uint16_t)top;
    top += c->pathLen;
  }
  path_top = top;
  (void)cmp_off;
}
static int cmp_off(const void *a, const void *b) { (void)a; (void)b; return 0; }

/* room for n steps (0 if there is none even after compacting) */
static uint16_t path_room(int n) {
  if (path_top + n > PATH_MEM) path_compact();
  if (path_top + n > PATH_MEM) return 0;
  uint16_t off = (uint16_t)path_top;
  path_top += n;
  return off;
}
uint16_t *cit_steps(const Cit *c) { return path_mem + c->pathOff; }

static inline bool has_path(const Cit *c) { return (c->flags & CF_HAS_PATH) != 0; }
static inline int path_type_at(const Cit *c, int pos) { return step_type(path_mem[c->pathOff + pos]); }
static inline int path_value_at(const Cit *c, int pos) { return step_value(path_mem[c->pathOff + pos]); }

/* ---------------------------------------------------------------- basics */
float cit_age(const Cit *c) { return (float)((city.timeSinceStart - c->bornOn) / 1440.0); }
float cit_steps_in_building(const Cit *c) { return (float)(uint16_t)(now_ticks() - c->inBuildingSince); }
float cit_work_time_preference(const Cit *c) {
  float p = c->workTimePref / 127.0f;
  return c->job ? p * PC(P(c->job))->workTimePreferenceMod : p;
}
float cit_education_speed_modifier(const Cit *c) { return 1 + c->educationLevel / 2; }

bool cit_should_work_now(const Cit *c) {
  if (!c->job) return false;
  Perm *j = P(c->job);
  float t = city_hour();
  float start = bld_start_time(j) + cit_work_time_preference(c);
  float end = fmodf(bld_end_time(j) + cit_work_time_preference(c) + policies.workTimeChange, 24);
  return in_hours(t, start, end);
}
bool cit_should_school_now(const Cit *c) {
  if (!c->school) return false;
  Perm *s = P(c->school);
  return in_hours(city_hour(), bld_school_start(s), bld_school_end(s));
}

/* the citizen's position in the city (getCityPosition) */
void cit_city_position(const Cit *c, float *x, float *y) {
  if (c->inPermanent) {
    Perm *p = P(c->inPermanent);
    *x = (float)perm_city_x(p) + c->relativeX;
    *y = (float)perm_city_y(p) + 20 - c->relativeY - (is_building(p) ? 1 : 0);
  } else {
    World *w = &worlds[c->onWorld];
    *x = w->x + c->relativeX;
    *y = w->y - c->relativeY;
  }
  if (has_path(c)) {
    if (c->currentPathAction == 0) *y -= cit_vpp(c);
    else if (c->currentPathAction == 1) *y += cit_vpp(c);
  }
}

/* ---------------------------------------------------------------- entering and leaving */
static void on_leave(Cit *c, uint16_t to) {
  Perm *p = P(c->inPermanent);
  if (p && PC(p)->ops && PC(p)->ops->on_citizen_leave) PC(p)->ops->on_citizen_leave(p, c, to);
}
void cit_go_to_permanent(Cit *c, uint16_t perm) {
  if (c->inPermanent) on_leave(c, perm);
  c->inBuildingSince = now_ticks();
  c->inPermanent = perm;
  c->flags &= (uint16_t)~CF_BUILDING_INITED;
}
/* out of the permanent, onto its world's surface at its door */
void cit_instant_leave_permanent(Cit *c) {
  Perm *p = P(c->inPermanent);
  if (is_building(p)) c->relativeX = (float)(p->col * 20 + bld_door_x(p));
  else c->relativeX += p->col * 20;
  on_leave(c, NONE);
  c->inBuildingSince = now_ticks();
  c->inPermanent = NONE;
  c->relativeY = 0;
}
bool cit_move_towards_x(Cit *c, float x, float timeMod) {
  float spd = c->pathWalkSpeed / 100.0f * timeMod;
  if (fabsf(x - c->relativeX) < spd) {
    c->relativeX = x;
    return true;
  }
  c->relativeX += (x > c->relativeX ? 1 : -1) * spd;
  return false;
}
/* leavePermanent: walk to the door, then out (true once out) */
bool cit_leave_permanent(Cit *c, float timeMod) {
  Perm *p = P(c->inPermanent);
  if (is_building(p)) {
    if (!cit_move_towards_x(c, bld_door_x(p), timeMod)) return false;
  }
  cit_instant_leave_permanent(c);
  return true;
}

/* ---------------------------------------------------------------- paths */
void cit_reset_path(Cit *c, bool reset_can_view_self) {
  c->flags &= (uint16_t)~CF_HAS_PATH;
  c->pathOff = 0;
  c->pathLen = c->pathPos = 0;
  c->currentPathAction = -2;
  if (reset_can_view_self) {
    if (!(c->flags & CF_CAN_VIEW_SELF)) c->flags |= CF_DELAY_CAN_VIEW_SELF;
    c->flags |= CF_CAN_VIEW_SELF;
  }
  cit_set_vpp(c, 0);
  c->then = THEN_NONE;
  c->requestingPathGoal = NONE;
  c->pathOnlyRelatedTo = NONE;
  c->pathWalkSpeed = 100;
  c->flags |= CF_PATH_RECONSIDER;
  cit_set_timer(c, 0);
}

/* setPath with packed steps; failed (a path of -1) when n < 0 */
static void set_path_packed(Cit *c, const uint16_t *steps, int n, bool from_pathfinder) {
  if (n < 0) {
    c->flags &= (uint16_t)~CF_REQUESTING_PATH;
    if (c->requestingPathGoal && c->requestingPathGoal == c->home) cit_evict_from_home(c);
    else if (c->requestingPathGoal && c->requestingPathGoal == c->job) cit_lose_job(c, false);
    else if (c->requestingPathGoal && c->requestingPathGoal == c->school) cit_leave_school(c);
    uint8_t fail = c->onFail;
    c->onFail = THEN_NONE;
    if (fail && then_fns[fail]) then_fns[fail](c, c->thenArg);
    cit_reset_path(c, false);
    return;
  }
  c->pathDestination = c->requestingPathGoal;
  cit_reset_path(c, false);
  if (n) {
    uint16_t off = path_room(n);
    if (!off) {   /* no room: as if the path is still coming (asked again later) */
      if (from_pathfinder) c->pathDestination = NONE;
      c->flags &= (uint16_t)~CF_REQUESTING_PATH;
      return;
    }
    memcpy(path_mem + off, steps, sizeof *steps * n);
    c->pathOff = off;
    c->pathLen = (uint8_t)n;
    c->pathPos = 0;
    c->currentPathAction = -1;
    c->flags |= CF_HAS_PATH;
    if (c->inPermanent && (c->relativeX < 0 || c->relativeX > 18)) {
      int t = step_type(steps[0]);
      if (t != 15 && t != 16) c->relativeX = c->relativeX < 0 ? 0 : 18;
    }
  }
  c->flags &= (uint16_t)~CF_REQUESTING_PATH;
  c->pathStartTime = now_minutes();
}

/* a path from the pathfinder: (type, value) pairs, or n < 0 if none was found */
void cit_set_found_path(Cit *c, const int16_t *pairs, int n) {
  uint16_t buf[128];
  if (n > 128) n = -1;   /* longer than a citizen can hold: as no path */
  for (int i = 0; i < n; i++) buf[i] = step_pack(pairs[2 * i], pairs[2 * i + 1]);
  set_path_packed(c, buf, n, true);
}

/* a scripted path (type, value pairs) */
void cit_set_path(Cit *c, const int16_t *pairs, int n) {
  uint16_t buf[CIT_SCRIPTED_MAX];
  if (n > CIT_SCRIPTED_MAX) n = CIT_SCRIPTED_MAX;
  for (int i = 0; i < n; i++) buf[i] = step_pack(pairs[2 * i], pairs[2 * i + 1]);
  set_path_packed(c, buf, n, false);
}
void cit_set_path_then(Cit *c, const int16_t *pairs, int n, uint8_t then, uint16_t arg) {
  cit_set_path(c, pairs, n);
  c->then = then;
  c->thenArg = arg;
}
static void scripted(Cit *c, const int16_t *pairs, int n, uint8_t then, uint16_t arg) {
  cit_set_path(c, pairs, n);
  c->then = then;
  c->thenArg = arg;
  c->pathOnlyRelatedTo = c->inPermanent;
}

void cit_wait(Cit *c, int time, uint8_t then, uint16_t arg) {
  int16_t s[] = {8, (int16_t)time};
  scripted(c, s, 1, then, arg);
}
void cit_wait_random(Cit *c, int tmin, int tmax, uint8_t then, uint16_t arg) {
  int16_t s[] = {8, (int16_t)rnd_between(tmin, tmax)};
  scripted(c, s, 1, then, arg);
}
void cit_move(Cit *c, int x, uint8_t then, uint16_t arg, bool slow) {
  if (slow) {
    int16_t s[] = {12, 50, 4, (int16_t)x};
    scripted(c, s, 2, then, arg);
  } else {
    int16_t s[] = {4, (int16_t)x};
    scripted(c, s, 1, then, arg);
  }
}
void cit_move_random(Cit *c, int xmin, int xmax, uint8_t then, uint16_t arg, bool slow) {
  cit_move(c, rnd_between(xmin, xmax), then, arg, slow);
}
void cit_move_and_wait(Cit *c, int x, int time, uint8_t then, uint16_t arg, bool modify_with_happiness, bool slow) {
  if (modify_with_happiness) time = (int)(time / happiness.actionSpeedModifier);
  if (slow) {
    int16_t s[] = {12, 50, 4, (int16_t)x, 8, (int16_t)time};
    scripted(c, s, 3, then, arg);
  } else {
    int16_t s[] = {4, (int16_t)x, 8, (int16_t)time};
    scripted(c, s, 2, then, arg);
  }
}
void cit_move_and_wait_random(Cit *c, int xmin, int xmax, int tmin, int tmax, uint8_t then, uint16_t arg,
                              bool modify_with_happiness, bool slow) {
  int x = rnd_between(xmin, xmax);
  cit_move_and_wait(c, x, rnd_between(tmin, tmax), then, arg, modify_with_happiness, slow);
}
void cit_wait_and_move_and_wait_random(Cit *c, int t1min, int t1max, int xmin, int xmax, int tmin, int tmax,
                                       uint8_t then, uint16_t arg, bool modify_with_happiness, bool slow) {
  int t1 = rnd_between(t1min, t1max), t2 = rnd_between(tmin, tmax), x = rnd_between(xmin, xmax);
  if (modify_with_happiness) {
    t1 = (int)(t1 / happiness.actionSpeedModifier);
    t2 = (int)(t2 / happiness.actionSpeedModifier);
  }
  if (slow) {
    int16_t s[] = {8, (int16_t)t1, 12, 50, 4, (int16_t)x, 8, (int16_t)t2};
    scripted(c, s, 4, then, arg);
  } else {
    int16_t s[] = {8, (int16_t)t1, 4, (int16_t)x, 8, (int16_t)t2};
    scripted(c, s, 3, then, arg);
  }
}
void cit_move_towards_random_in_world_coords(Cit *c, int xmin, int xmax, uint8_t then, uint16_t arg) {
  int to = rnd_between(xmin, xmax);
  Perm *p = P(c->inPermanent);
  if (p) {
    float diff = to - c->relativeX - perm_city_x(p);
    if (diff < -c->relativeX) {
      if (p->left) {
        int16_t s[] = {2, 0, 4, 18};
        cit_set_path(c, s, 2);
      } else {
        int16_t s[] = {4, 0};
        scripted(c, s, 1, THEN_NONE, 0);
      }
    } else if (diff >= 20 - c->relativeX) {
      if (p->right) {
        int16_t s[] = {3, 0, 4, 0};
        cit_set_path(c, s, 2);
      } else {
        int16_t s[] = {4, 18};
        scripted(c, s, 1, THEN_NONE, 0);
      }
    } else {
      int16_t s[] = {4, (int16_t)floorf(diff + c->relativeX)};
      scripted(c, s, 1, THEN_NONE, 0);
    }
    c->then = then;
    c->thenArg = arg;
  } else {
    World *w = &worlds[c->onWorld];
    int16_t s[] = {4, (int16_t)rnd_between(xmin - w->x, xmax - w->x)};
    scripted(c, s, 1, then, arg);
  }
}
bool cit_is_at_ground_level(const Cit *c) { return c->relativeY < 5; }
void cit_change_floor(Cit *c, uint8_t then, uint16_t arg) {
  int16_t s[] = {9, (int16_t)(cit_is_at_ground_level(c) ? 10 : 0)};
  scripted(c, s, 1, then, arg);
}
void cit_change_floor_and_wait(Cit *c, int wait, uint8_t then, uint16_t arg) {
  int16_t s[] = {9, (int16_t)(c->relativeY == 0 ? 10 : 0), 8, (int16_t)wait};
  scripted(c, s, 2, then, arg);
}
void cit_change_floor_and_wait_random(Cit *c, int tmin, int tmax, uint8_t then, uint16_t arg) {
  cit_change_floor_and_wait(c, rnd_between(tmin, tmax), then, arg);
}
void cit_change_floor_and_move_random(Cit *c, int xmin, int xmax, uint8_t then, uint16_t arg) {
  int16_t s[] = {9, (int16_t)(cit_is_at_ground_level(c) ? 10 : 0), 4, (int16_t)rnd_between(xmin, xmax)};
  scripted(c, s, 2, then, arg);
}

void cit_wander(Cit *c, float timeMod) {
  if (!c->inPermanent) {
    int rx = (int)c->relativeX;
    int lo = rx - 100 > 0 ? rx - 100 : 0;
    int hi = worlds[c->onWorld].w - 2 < rx + 100 ? worlds[c->onWorld].w - 2 : rx + 100;
    cit_move_and_wait(c, rnd_between(lo, hi), rnd_between(30, 60), THEN_NONE, 0, false, false);
  } else {
    cit_go_down_towards_world_surface(c, timeMod);
  }
}

static uint8_t impossible_surface_world[MAX_CITS];   /* impossibleToReachWorldSurfaceForWorld + 1 */
void cit_go_down_towards_world_surface(Cit *c, float timeMod) {
  Perm *p = P(c->inPermanent);
  if (p->row == 0 && worlds[p->world].h > 0) {
    cit_leave_permanent(c, timeMod);
    impossible_surface_world[cid(c)] = 0;
    return;
  }
  if (impossible_surface_world[cid(c)] == c->onWorld + 1) return;
  /* (a path holds CIT_SCRIPTED_MAX steps: from higher up, the citizen goes that far down, then on from there) */
  int16_t s[2 * CIT_SCRIPTED_MAX];
  int n = 0;
  Perm *pm = p;
  while (pm->row != 0 && n < CIT_SCRIPTED_MAX - 1) {
    if (!pm->bottom) {
      if (pm->left) s[2 * n] = 2, s[2 * n + 1] = 0, n++, pm = P(pm->left);
      else if (pm->right) s[2 * n] = 3, s[2 * n + 1] = 0, n++, pm = P(pm->right);
    }
    if (pm->bottom) {
      s[2 * n] = 1, s[2 * n + 1] = 0, n++;
      pm = P(pm->bottom);
    } else {
      n = 0;
      impossible_surface_world[cid(c)] = (uint8_t)(c->onWorld + 1);
      break;
    }
  }
  if (n > 0) cit_set_path(c, s, n);
}

/* ---------------------------------------------------------------- following the path */
static void path_end(Cit *c) {
  uint8_t then = c->then;
  uint16_t arg = c->thenArg;
  cit_reset_path(c, true);
  if (then && then_fns[then]) then_fns[then](c, arg);
  if (c->inPermanent && c->inPermanent == c->job && c->currentAction != 0 && cit_should_work_now(c))
    stats_on_commute((float)(uint16_t)(now_minutes() - c->pathStartTime));
}

void cit_update_path(Cit *c, float timeMod) {
  if (c->currentPathAction < -1) return;
  if (c->currentPathAction == -1) {
    while (path_type_at(c, c->pathPos) == 12) {
      c->pathWalkSpeed = (uint8_t)path_value_at(c, c->pathPos);
      c->pathPos++;
    }
    int a = path_type_at(c, c->pathPos);
    c->currentPathAction = (int8_t)a;
    Perm *in = P(c->inPermanent);
    if (a == 0 || a == 1 || a == 9) c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
    else if (!c->pathOnlyRelatedTo && a != 4 && a != 8) {
      if (!is_building(in) || !(PC(in)->base & B_HIDE_WALKERS)) c->flags |= CF_CAN_VIEW_SELF;
      else c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
    }
    if (a <= 1 && c->relativeY > 0) {
      cit_set_vpp(c, c->relativeY * (a - 0.5f) * -2);
      c->relativeY = 0;
    } else if (a != 4 && a != 8 && a != 9) {
      c->relativeY = 0;
    }
    cit_set_timer(c, 0);
    if (a == 5 || a == 7 || a == 11 || (a >= 13 && a <= 16)) c->pathActionPerm = (uint16_t)path_value_at(c, c->pathPos);
    else if (a == 4 || a == 9 || a == 8) cit_set_timer(c, (float)path_value_at(c, c->pathPos));
  }
  bool reached = false;
  Perm *in = P(c->inPermanent);
  float spd = c->pathWalkSpeed / 100.0f * timeMod;
  switch (c->currentPathAction) {
  case 0:
  case 1:
    cit_set_vpp(c, cit_vpp(c) + (timeMod));
    if (cit_vpp(c) >= 20) {
      cit_go_to_permanent(c, c->currentPathAction == 0 ? in->top : in->bottom);
      cit_set_vpp(c, cit_vpp(c) - (20));
      reached = true;
    }
    break;
  case 2:
    c->relativeX -= spd;
    if (c->relativeX < -2) {
      cit_go_to_permanent(c, in->left);
      c->onWorld = P(c->inPermanent)->world;
      c->relativeX += 20;
      reached = true;
    }
    break;
  case 3:
    c->relativeX += spd;
    if (c->relativeX > 20) {
      cit_go_to_permanent(c, in->right);
      c->onWorld = P(c->inPermanent)->world;
      c->relativeX -= 20;
      reached = true;
    }
    break;
  case 4:
    reached = cit_move_towards_x(c, cit_timer(c), timeMod);
    break;
  case 5: {
    if (in) {
      if (is_building(in)) {
        if (cit_move_towards_x(c, bld_door_x(in), timeMod)) cit_instant_leave_permanent(c);
      } else {
        cit_instant_leave_permanent(c);
      }
      /* (out of it; the next tick walks to the goal) */
    } else {
      Perm *to = P(c->pathActionPerm);
      if (is_building(to)) {
        reached = cit_move_towards_x(c, (float)(to->col * 20 + bld_door_x(to)), timeMod);
        if (reached) {
          cit_go_to_permanent(c, c->pathActionPerm);
          c->relativeX = bld_door_x(to);
        }
      } else {
        float px = (float)(to->col * 20);
        float num = px + 10.0f - c->relativeX;
        c->relativeX += (num > 0 ? 1 : num < 0 ? -1 : 0) * (timeMod * c->pathWalkSpeed / 100.0f);
        if (c->relativeX > px && c->relativeX < px + 20 - 2) {
          cit_go_to_permanent(c, c->pathActionPerm);
          c->relativeX -= to->col * 20;
          reached = true;
        }
      }
    }
    break;
  }
  case 7:
    if (cit_timer(c) > 0) {
      cit_set_timer(c, cit_timer(c) + (0.1f * timeMod));
      if (cit_timer(c) >= 1) {
        if (bld_is_misdirector(in)) {
          cit_reset_path(c, true);
          cit_wait_and_move_and_wait_random(c, 30, 45, 3, 16, 120, 180, THEN_NONE, 0, false, true);
          c->flags &= (uint16_t)~CF_PATH_RECONSIDER;
          bld_misdirector_reward(in);
          return;
        }
        reached = true;
      }
    } else if (!operating_cost.teleportersEnabled) {
      cit_reset_path(c, true);
      return;
    } else {
      float tx = bld_teleport_x(in);
      if (cit_move_towards_x(c, tx, timeMod)) {
        bld_teleport_used(in);
        uint16_t to = c->pathActionPerm;
        uint16_t mis = bld_misdirector();
        if (mis && bld_misdirect_chance() > 0 && rnd_float() < bld_misdirect_chance()) to = c->pathActionPerm = mis;
        cit_go_to_permanent(c, to);
        bld_teleport_arrived(P(to));
        c->onWorld = P(to)->world;
        c->relativeX = bld_teleport_x(P(to));
        cit_set_timer(c, 0.1f * timeMod);
      }
    }
    break;
  case 8:
    cit_set_timer(c, cit_timer(c) - (timeMod));
    reached = cit_timer(c) <= 0.5f * (timeMod - 0.6666667f);
    break;
  case 9:
    if (fabsf(cit_timer(c) - c->relativeY) < timeMod) {
      c->relativeY = cit_timer(c);
      reached = true;
    } else {
      c->relativeY += (cit_timer(c) > c->relativeY ? 1 : -1) * timeMod;
    }
    break;
  case 10:
    reached = true;
    break;
  case 11:
    if (c->inPermanent == c->pathActionPerm) reached = true;
    else if (!flying_board(c, in, P(c->pathActionPerm))) return;   /* (no route: the path is dropped) */
    break;
  case 13: {
    Perm *to = P(c->pathActionPerm);
    float diff = 20.0f * ((int)to->row - (int)in->row);
    if (in == to) {
      if (cit_timer(c) > 3) {
        if (c->pathPos + 1 < c->pathLen) {
          int nt = path_type_at(c, c->pathPos + 1);
          if (nt == 0 || nt == 1) {
            if (cit_move_towards_x(c, c->relativeX > 10 ? 18 : 0, timeMod)) reached = true;
          } else {
            reached = true;
          }
        } else {
          reached = true;
        }
      } else {
        cit_set_timer(c, cit_timer(c) + (timeMod));
      }
    } else if (cit_timer(c) > 0) {
      if (fabsf(cit_vpp(c)) < fabsf(diff)) {
        cit_set_vpp(c, cit_vpp(c) + (timeMod * 20 * (diff > 0 ? 1 : diff < 0 ? -1 : 0)));
        c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
      }
      if (fabsf(cit_vpp(c)) >= fabsf(diff) && bld_elevator_open_door(to)) {
        cit_go_to_permanent(c, c->pathActionPerm);
        bld_elevator_arrived(to);
        cit_set_vpp(c, 0);
        c->flags |= CF_CAN_VIEW_SELF;
        c->relativeX += rnd_between(-2, 3);
      }
    } else {
      c->flags |= CF_CAN_VIEW_SELF;
      if (cit_move_towards_x(c, 9, timeMod) && bld_elevator_open_door(in)) {
        bld_elevator_used(in);
        cit_set_timer(c, 1);
        c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
      }
    }
    break;
  }
  default:   /* 12 and 14 do nothing; 15 and 16 (bridges) are only in the paid version */
    break;
  }
  if (!reached) return;
  if (c->pathPos + 1 >= c->pathLen) {
    path_end(c);
    return;
  }
  c->pathPos++;
  c->currentPathAction = -1;
  int t = path_type_at(c, c->pathPos);
  if (t == 13 || t == 14)
    while (c->pathPos + 1 < c->pathLen && path_type_at(c, c->pathPos + 1) == t) c->pathPos++;
  if (path_type_at(c, c->pathPos) == 10) {
    if (c->pathPos + 1 >= c->pathLen) path_end(c);
    else {
      c->flags |= CF_CAN_VIEW_SELF;
      c->pathPos++;
    }
  }
}

/* ---------------------------------------------------------------- entertainment */

static void set_age_related_entertainment(Cit *c) {
  if (cit_age(c) < 16 && rnd_int(1000) < 998)
    c->entRecent[0] = at_2h(city.timeSinceStart + 1440.0 * (cit_age(c) - rnd_range(14, 16)));
}
static float normally_at_home_from(const Cit *c) {
  float base = c->job ? fmodf(bld_end_time(P(c->job)) + 4, 24) : 23;
  return cit_work_time_preference(c) + base;
}
static float normally_at_home_to(const Cit *c) {
  return cit_work_time_preference(c) + (c->job ? bld_start_time(P(c->job)) : 8);
}
static bool search_night_entertainment(Cit *c) {
  if (!past_4m(c->entTry)) return false;
  if (!path_may_request_now(NONE)) return true;
  path_find_entertainment(c, rnd_float() < 0.02f ? 480 : 320, -1);
  c->onFail = THEN_NONE;
  c->flags &= (uint16_t)~CF_ENT_INITED;
  c->entTry = at_4m(city.timeSinceStart + 120 * city.minutesPerTick);
  return true;
}
static bool search_day_entertainment(Cit *c) {
  if (!past_4m(c->entTryDay)) return false;
  if (!path_may_request_now(NONE)) return true;
  int bits = 0;
  for (int i = 0; i < 6; i++)
    if (past_2h(c->entRecent[i])) bits |= 1 << i;
  path_find_entertainment(c, rnd_float() < 0.02f ? 480 : 320, bits);
  c->onFail = THEN_NONE;
  c->flags &= (uint16_t)~CF_ENT_INITED;
  c->entTryDay = at_4m(city.timeSinceStart + 120 * city.minutesPerTick);
  return true;
}
/* updateEntertainment: true while busy with it */
static bool update_entertainment(Cit *c, float timeMod) {
  Perm *in = P(c->inPermanent);
  if (is_building(in) && (in->flags & PF_IS_ENTERTAINMENT)) {
    bool busy = true;
    if (!(c->flags & CF_ENT_INITED)) {
      if (ent_is_open(in)) {
        float desirability = (!c->job && !c->school) ? 0.75f : 1.0f;
        c->entFinished = at_4m(city.timeSinceStart + 60.0 * rnd_range(ent_min_time(in), ent_max_time(in)));
        c->entRecent[ent_type(in)] =
            at_2h(city.timeSinceStart + 1440.0 * rnd_range(ent_min_satisfy(in), ent_max_satisfy(in)) * desirability);
        c->flags |= CF_ENT_INITED;
      } else {
        c->entFinished = at_4m(city.timeSinceStart - 1000);
      }
    }
    if (!ent_is_open_for_existing_visitors(in) || past_4m(c->entFinished)) {
      if (ent_finish(in, c, timeMod)) {
        busy = false;
        c->flags &= (uint16_t)~CF_ENT_INITED;
      } else {
        ent_be_entertained(in, c, timeMod);
      }
    } else {
      ent_be_entertained(in, c, timeMod);
    }
    if (busy) return true;
  }
  float t = city_hour();
  if (cit_age(c) > 16 && (c->flags & CF_LIKES_NIGHTCLUBS) &&
      in_hours(t, 23 + cit_work_time_preference(c), 6 + cit_work_time_preference(c)) && c->wantsNightEntertainmentIn == 0)
    return search_night_entertainment(c);
  if (!in_hours(t, normally_at_home_from(c), normally_at_home_to(c))) return search_day_entertainment(c);
  return false;
}
/* the entertainment building lets the citizen go (true) or keeps them a little longer */
static bool finish_entertainment_if_in(Cit *c, float timeMod) {
  Perm *in = P(c->inPermanent);
  return (is_building(in) && (in->flags & PF_IS_ENTERTAINMENT)) ? ent_finish(in, c, timeMod) : true;
}

/* ---------------------------------------------------------------- daily life */
void cit_update_daily_life(Cit *c, float timeMod) {
  float time = city_hour();
  if (c->currentAction == 0) {
    Perm *j = P(c->job);
    float start = bld_start_time(j) + cit_work_time_preference(c);
    float end = bld_end_time(j) + cit_work_time_preference(c) + policies.workTimeChange;
    bld_work(j, c, timeMod, !in_hours(time, start, end));
  } else if (c->currentAction == 1) {
    Perm *s = P(c->school);
    if (in_hours(time, bld_school_start(s), bld_school_end(s))) bld_be_at_school(s, c, timeMod);
    else c->currentAction = 2;
  } else if (c->hobby && hobby_is_active(c)) {
    hobby_update(c, timeMod);
  } else if (c->currentAction != 3) {
    if (cit_should_work_now(c)) {
      if (c->inPermanent == c->job) c->currentAction = 0;
      else if (path_may_request_now(c->inPermanent) && finish_entertainment_if_in(c, timeMod))
        path_find_combined(c, bld_first_building_to_go_to(P(c->job)), THEN_NONE);
    } else if (cit_should_school_now(c)) {
      if (c->inPermanent == c->school) c->currentAction = 1;
      else if (path_may_request_now(c->inPermanent) && finish_entertainment_if_in(c, timeMod)) {
        if (bld_workers(P(c->school)) == 0) cit_leave_school(c);
        else path_find_combined(c, c->school, THEN_NONE);
      }
    } else if (!update_entertainment(c, timeMod)) {
      bool go_home;
      if (c->home) {
        if (!c->job) {
          float s = 8 + cit_work_time_preference(c), e = 21 + cit_work_time_preference(c);
          go_home = !in_hours(time, s, e);
        } else {
          go_home = true;
        }
      } else {
        go_home = false;
      }
      if (go_home) {
        if (c->inPermanent != c->home) {
          if (path_may_request_now(c->inPermanent)) path_find_combined(c, c->home, THEN_NONE);
        } else {
          float st, en;
          if (!c->job) st = 8, en = 21;
          else st = bld_start_time(P(c->job)), en = bld_end_time(P(c->job));
          float s = fmodf(st + cit_work_time_preference(c) - 1, 24);
          float e = fmodf(en + cit_work_time_preference(c) + 1, 24);
          if (in_hours(time, s, e) || cit_steps_in_building(c) < 240) {
            c->flags |= CF_CAN_VIEW_SELF;
            bld_walk_around(P(c->home), c, cit_steps_in_building(c));
          } else {
            c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
          }
        }
      } else {
        cit_wander(c, timeMod);
      }
    }
  }
}

/* ---------------------------------------------------------------- jobs, homes, schools */
void cit_evict_from_home(Cit *c) {
  if (!c->home) return;
  P(c->home)->nresidents--;
  c->home = NONE;
  sim_flags |= SIM_HOUSES_CHANGED;
}
void cit_leave_school(Cit *c) {
  if (!c->school) return;
  P(c->school)->nstudents--;
  c->school = NONE;
  sim_flags |= SIM_SCHOOLS_CHANGED;
  if (c->currentAction == 1) c->currentAction = 2;
}
void cit_lose_job(Cit *c, bool instant_replace) {
  if (!c->job) return;
  bld_on_lose_job(P(c->job), c);
  P(c->job)->nworkers--;
  c->job = NONE;
  if (!instant_replace) {
    sim_flags |= SIM_JOBS_CHANGED;
    c->flags |= CF_WITHOUT_JOB;
  }
  c->flags &= (uint16_t)~CF_HAS_WORK_TOOLS;
  if (c->currentAction == 0) c->currentAction = 2;
}
void cit_add_to_workers(Cit *c) {
  c->flags |= CF_WITHOUT_JOB;
  sim_flags |= SIM_JOBS_CHANGED;
  if (c->school) cit_leave_school(c);
}

void cit_infrequent_update(Cit *c) {
  float age = cit_age(c);
  /* lastInfrequentUpdateAge < 16: a child that just turned 16 */
  if (age >= 16 && !(c->flags & CF_WITHOUT_JOB) && !c->job && age - 16 < 1) cit_add_to_workers(c);
  else if (age >= 16 && c->job && c->school) cit_leave_school(c);
  for (int i = 0; i < 6; i++) keep_near(&c->entRecent[i], now_2h());
  keep_near(&c->entTry, now_4m());
  keep_near(&c->entTryDay, now_4m());
  keep_near(&c->entFinished, now_4m());
  if (age >= 75 + c->dieAgeModifier / 100.0f && !city.disableDying) {
    cit_try_remove(c, false);
    return;
  }
  if (has_path(c) && (c->flags & CF_PATH_RECONSIDER)) {
    bool work = c->currentAction != 0 && cit_should_work_now(c);
    bool school = c->currentAction != 1 && cit_should_school_now(c);
    if ((work && c->pathDestination != c->job) || (school && c->pathDestination != c->school)) cit_reset_path(c, false);
  }
}

void cit_mid_day_update(Cit *c) {
  if (c->wantsNightEntertainmentIn > 0) c->wantsNightEntertainmentIn--;
  die_add(c, 0.2f * (happiness.medicalHappiness / 100.0f) + 0.05f * (happiness.specializedMedicalHappiness / 100.0f));
  if (happiness.happiness < 10 && !c->home && city.materials.m[M_FOOD] < 1) {
    if (!c->job) die_add(c, -10);
    die_add(c, -1);
  }
  if (c->home) die_add(c, bld_years_to_live_longer(P(c->home)));
  die_add(c, upgrades.extendAgeBy);
  impossible_surface_world[cid(c)] = 0;
}

void cit_try_remove(Cit *c, bool for_send_away) {
  if (c->flags & CF_CONTROLLED) return;
  if (c->currentAction == 3) return;
  if (!for_send_away) baby_on_citizen_death();
  c->flags &= (uint16_t)~CF_WITHOUT_JOB;
  if (c->job) bld_on_lose_job(P(c->job), c), P(c->job)->nworkers--, sim_flags |= SIM_JOBS_CHANGED;
  if (c->home) P(c->home)->nresidents--, sim_flags |= SIM_HOUSES_CHANGED;
  if (c->school) P(c->school)->nstudents--, sim_flags |= SIM_SCHOOLS_CHANGED;
  if (c->hobby) hobby_on_die(c);
  if (c->inPermanent) on_leave(c, NONE);
  c->job = c->home = c->school = NONE;
  c->flags = 0;
  c->pathOff = 0;
  ncits--;
}

/* ---------------------------------------------------------------- a permanent's citizens */
void cits_lose_job_at(uint16_t perm) {
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].job == perm) cit_lose_job(&cits[i], false);
}
void cits_evict_from(uint16_t perm) {
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].home == perm) cit_evict_from_home(&cits[i]);
}
void cits_leave_school(uint16_t perm) {
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].school == perm) cit_leave_school(&cits[i]);
}
/* the citizens in a permanent that goes: to a neighbouring building, or out onto a world */
void cits_leave_destroyed(uint16_t perm) {
  Perm *p = P(perm);
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE) || c->inPermanent != perm) continue;
    if (p->row == 0) {
      cit_instant_leave_permanent(c);
    } else {
      uint16_t to = NONE;
      if (is_building(p)) to = p->bottom ? p->bottom : p->left ? p->left : p->right ? p->right : p->top;
      if (to) {
        cit_go_to_permanent(c, to);
        c->onWorld = P(to)->world;
      } else if (worlds[c->onWorld].h > 0) {
        cit_instant_leave_permanent(c);
      } else {
        /* a building below at its x, or the nearest world */
        float cx, cy;
        cit_city_position(c, &cx, &cy);
        uint16_t below = NONE;
        for (int j = 1; j < MAX_PERMS && !below; j++) {
          Perm *q = &perms[j];
          if (!(q->flags & PF_USED) || j == perm) continue;
          if (cx >= perm_city_x(q) && cx < perm_city_x(q) + 20 && perm_city_y(q) > cy) below = (uint16_t)j;
        }
        if (below) {
          cit_go_to_permanent(c, below);
          c->onWorld = P(below)->world;
        } else {
          int best = 0;
          float bd = 1e30f;
          for (int w = 0; w < nworlds; w++) {
            if (worlds[w].h <= 0) continue;
            float d = fabsf(worlds[w].x + worlds[w].w / 2.0f - cx) + fabsf(worlds[w].y + worlds[w].h / 2.0f - cy);
            if (d < bd) bd = d, best = w;
          }
          cit_instant_leave_permanent(c);
          c->onWorld = (uint8_t)best;
          c->relativeX = 0;
        }
      }
      c->relativeY = 0;
    }
    cit_reset_path(c, true);
  }
}

/* after loading: the count of the living */
void cits_recount(void) {
  ncits = 0;
  for (int i = 1; i < MAX_CITS; i++)
    if (cits[i].flags & CF_ALIVE) ncits++;
}

void cits_reset(void) {
  memset(cits, 0, sizeof cits);
  ncits = 0;
  path_top = 1;
  memset(impossible_surface_world, 0, sizeof impossible_surface_world);
}

/* a new citizen (the Citizen constructor) */
uint16_t cit_new(int world, float x, float age, uint16_t start_in) {
  int id = 0;
  for (int i = 1; i < MAX_CITS; i++)
    if (!(cits[i].flags & CF_ALIVE)) { id = i; break; }
  if (!id) return NONE;
  Cit *c = &cits[id];
  memset(c, 0, sizeof *c);
  c->flags = CF_ALIVE | CF_CAN_VIEW_SELF | CF_PATH_RECONSIDER;
  c->currentAction = 2;
  c->currentPathAction = -2;
  c->pathWalkSpeed = 100;
  c->spriteIndex = (uint8_t)rnd_int(NCITIZEN_SPRITES - 2);
  c->nameIndex = (uint16_t)rnd_between(1, NCITIZEN_NAMES);
  if (c->nameIndex == NAME_WALDO) c->spriteIndex = NCITIZEN_SPRITES - 1;
  if (c->nameIndex == NAME_DRACULA) c->spriteIndex = NCITIZEN_SPRITES - 2;
  c->onWorld = (uint8_t)world;
  c->relativeX = x;
  sim_flags |= SIM_HOUSES_CHANGED | SIM_SCHOOLS_CHANGED;
  c->bornOn = (float)(city.timeSinceStart - age * 1440.0);
  c->dieAgeModifier = (int16_t)(rnd_between(-5, 5) * 100);
  c->workTimePref = (int8_t)(rnd_range(-1, 1) * 127);
  /* likesNightclubs: the original's spriteIndex mixes the name in (nameIndex << 14) */
  if (((uint32_t)c->spriteIndex + ((uint32_t)c->nameIndex << 14)) % 5 != 0) c->flags |= CF_LIKES_NIGHTCLUBS;
  ncits++;
  if (age >= 16) cit_add_to_workers(c);
  if (start_in) {
    c->inBuildingSince = now_ticks();
    c->inPermanent = start_in;
    c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
  }
  set_age_related_entertainment(c);
  c->wantsNightEntertainmentIn = (uint8_t)rnd_int(5);
  c->inBuildingSince = now_ticks();
  return (uint16_t)id;
}
