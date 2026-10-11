/* The Final Earth 2: transport (buildings.md 10): teleporters (and private
 * ones in homes), hyper elevators, landing pads with their flying saucers and
 * the flying pathfinder (pathfinder.FlyingPathfinder), saucers bringing new
 * citizens (simulation.SpawnFlyingSaucer), and the teleport particles. */
#include "bldi.h"
#include "save.h"
#include "scratch.h"
#include "view.h"

/* ---------------------------------------------------------------- particles (graphics.Particles) */
#define MAX_PARTICLES 8
static struct { int16_t x, y; uint16_t spr; uint8_t fw, frames; float frame; } particles[MAX_PARTICLES];
void particle_add(uint16_t spr, int fw, int x, int y) {
  for (int i = 0; i < MAX_PARTICLES; i++)
    if (!particles[i].spr) {
      particles[i].x = (int16_t)x, particles[i].y = (int16_t)y;
      particles[i].spr = spr;
      particles[i].fw = (uint8_t)fw;
      particles[i].frames = (uint8_t)frames_of(spr, fw);
      particles[i].frame = 0;
      return;
    }
}
static void particles_update(float timeMod) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].spr) continue;
    particles[i].frame += 0.5f * timeMod;
    if (particles[i].frame >= particles[i].frames) particles[i].spr = 0;
  }
}

/* ---------------------------------------------------------------- teleporters */
static void teleport_particle(const Perm *p) {
  int x = perm_city_x(p), y = perm_city_y(p);
  switch (p->cls) {
  case CLS_Teleporter: particle_add(SPR_TELEPORTER_RAY, 4, x + 8, y + 4); break;
  case CLS_Misdirector: particle_add(SPR_TELEPORTER_RAY_MISDIRECT, 4, x + 8, y + 4); break;
  case CLS_Villa: particle_add(SPR_SMALLTELEPORTER_RAY, 3, x + 3, y + 12); break;
  case CLS_Mansion: particle_add(SPR_SMALLTELEPORTER_RAY, 3, x + 14, y + 12); break;
  case CLS_FlowerPenthouse: {
    bool m = STC(PrivateTeleporterState, p)->mirrored;
    particle_add(m ? SPR_SMALLTELEPORTER_RAY_PENTHOUSE_MIRROR : SPR_SMALLTELEPORTER_RAY_PENTHOUSE, 3, x + (m ? 3 : 14), y + 12);
    break;
  }
  default: break;
  }
}
/* teleportX: where a citizen stands to teleport */
float transport_teleport_x(const Perm *p) {
  switch (p->cls) {
  case CLS_Villa: return 3;
  case CLS_Mansion: return 14;
  case CLS_FlowerPenthouse: return STC(PrivateTeleporterState, p)->mirrored ? 19 - 15 : 15;
  default: return 9;
  }
}
static uint16_t *times_used(Perm *p, bool to) {
  /* both counters start their state (2-byte aligned: Perm is, and s[] is at an even offset) */
  _Static_assert(offsetof(TeleporterState, timesUsedTo) == 2 && offsetof(PrivateTeleporterState, timesUsedTo) == 2 &&
                 offsetof(Perm, s) % 2 == 0, "counters at the start of the state");
  if (p->cls == CLS_Teleporter || p->cls == CLS_Misdirector || p->cls == CLS_Villa || p->cls == CLS_Mansion ||
      p->cls == CLS_FlowerPenthouse)
    return (uint16_t *)(void *)&p->s[to ? 2 : 0];
  return NULL;
}
void transport_teleport_used(Perm *p) {
  teleport_particle(p);
  uint16_t *n = times_used(p, false);
  if (n && *n < 65535) (*n)++;
}
void transport_teleport_arrived(Perm *p) {
  uint16_t *n = times_used(p, true);
  if (n && *n < 65535) (*n)++;
}
static void teleporter_window(Perm *p, WinOut *w) {
  const TeleporterState *s = STC(TeleporterState, p);
  win_line1(w, "Teleported from [0] times.", s->timesUsed);
  win_line1(w, "Teleported to [0] times.", s->timesUsedTo);
  teleporter_upkeep_lines(w);
}
/* createUpkeepInfo: the teleporters' cost a day, and the button to switch them all on or off */
void teleporter_upkeep_lines(WinOut *w) {
  float k = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    const Perm *p = &perms[i];
    if ((p->flags & PF_USED) && is_building(p) && PC(p)->teleporterOperatingCost > 0)
      k += PC(p)->teleporterOperatingCost * (bld_misdirector() ? 0.5f : 1) * (1 - upgrades.rocketTeleporterReward);
  }
  char b[96];
  lo1(b, sizeof b, "Teleporters operating cost:\n[0] knowledge/day", floorf(k + 0.5f));
  win_line(w, b);
  const char *t = operating_cost.teleportersEnabled ? "Disable All Teleporters"
                  : operating_cost_user_enabled()    ? "Keep Teleporters Disabled"
                                                     : "Enable All Teleporters";
  win_button(w, 0, t, WA_SPECIAL_2, true);
}
void teleporter_upkeep_action(int a) {
  if (a != WA_SPECIAL_2) return;
  if (!(!operating_cost.teleportersEnabled && operating_cost_user_enabled()))
    operating_cost_set_teleporters(!operating_cost.teleportersEnabled, true);
  else if (operating_cost_user_enabled()) operating_cost_set_teleporters(false, true);
  /* (else the original shows "cant_enable_teleporters") */
}
static void teleporter_action(Perm *p, int a) { (void)p; teleporter_upkeep_action(a); }
static void teleporter_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
}
const PermOps ops_Teleporter = {.draw = teleporter_draw, .window = teleporter_window, .window_action = teleporter_action};

/* ---------------------------------------------------------------- HyperElevator */
bool transport_elevator_open_door(Perm *p) {
  if (p->cls != CLS_HyperElevator) return true;
  ElevatorState *s = ST(ElevatorState, p);
  s->doorOpenFor = 45;
  return s->door >= (uint8_t)((frames_of(SPR_HYPERELEVATOR_DOOR, 10) - 1.01f) * 32 + 0.999f);
}
void transport_elevator_used(Perm *p) {
  if (p->cls == CLS_HyperElevator && ST(ElevatorState, p)->timesUsed < 65535) ST(ElevatorState, p)->timesUsed++;
}
void transport_elevator_arrived(Perm *p) {
  if (p->cls == CLS_HyperElevator && ST(ElevatorState, p)->timesUsedTo < 65535) ST(ElevatorState, p)->timesUsedTo++;
}
static void elevator_update(Perm *p, float timeMod) {
  ElevatorState *s = ST(ElevatorState, p);
  int max = (frames_of(SPR_HYPERELEVATOR_DOOR, 10) - 1) * 32;
  int step = round_random(0.5f * timeMod * 32);
  if (s->doorOpenFor > 0) {
    s->doorOpenFor = tick_down(s->doorOpenFor, timeMod);
    s->door = (uint8_t)(s->door + step > max ? max : s->door + step);
  } else {
    s->door = (uint8_t)(s->door > step ? s->door - step : 0);
  }
}
static void elevator_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  /* the door: on the front on the ground floor, else behind */
  if (layer == (p->row == 0 ? LAYER_MAIN : LAYER_BG))
    dr_frame(b, SPR_HYPERELEVATOR_DOOR, 10, STC(ElevatorState, p)->door / 32, x, y, 5, 10, 0, 255, GFX_NO_TINT);
}
static void elevator_window(Perm *p, WinOut *w) {
  win_line1(w, "Elevator entered here [0] times.", STC(ElevatorState, p)->timesUsed);
  win_line1(w, "Elevator left here [0] times.", STC(ElevatorState, p)->timesUsedTo);
}
const PermOps ops_HyperElevator = {.update = elevator_update, .draw = elevator_draw, .window = elevator_window};

/* ---------------------------------------------------------------- the flying pathfinder
 * The air is cut into 20-pixel columns over the whole city. In each, the
 * worlds' stacks and islands are obstructions; the free spans between them
 * hold waypoints (the heights of landing pads, of the next column's
 * obstructions' edges, and those carried over from the column before),
 * linked up and down within a span and sideways at equal heights. A route is
 * the shortest (Manhattan) way along them. The graph is made when a route is
 * asked for, for the columns between the two pads (a few more on each side),
 * in the search memory (scratch.h). */
#define FLY_MAX_OB 32
#define FLY_MAX_WP 64
#define FLY_MARGIN 6
static int grid_istart, grid_len;
static uint8_t sorted_worlds[MAX_WORLDS];
static int nsorted;

static void grid_setup(void) {
  int minx = 0, maxx = 20;
  if (nworlds > 0) {
    minx = 1 << 30, maxx = -(1 << 30);
    for (int w = 0; w < nworlds; w++) {
      if (worlds[w].x < minx) minx = worlds[w].x;
      if (worlds[w].x + worlds[w].w > maxx) maxx = worlds[w].x + worlds[w].w;
    }
    minx -= 40, maxx += 40;
  }
  grid_istart = minx / 20;
  grid_len = maxx / 20 - grid_istart + 1;
  /* the worlds from the top down (a stable sort) */
  nsorted = 0;
  for (int w = 0; w < nworlds; w++) {
    int k = nsorted++;
    while (k > 0 && worlds[sorted_worlds[k - 1]].y > worlds[w].y) sorted_worlds[k] = sorted_worlds[k - 1], k--;
    sorted_worlds[k] = (uint8_t)w;
  }
}
static bool is_pad(const Perm *p) { return p && (p->flags & PF_USED) && p->cls == CLS_LandingSite; }
static int pad_column(const Perm *p) { return perm_city_x(p) / 20 - grid_istart; }
static bool pad_in_column_at(int i, int y) {
  for (int k = 1; k < MAX_PERMS; k++)
    if (is_pad(&perms[k]) && pad_column(&perms[k]) == i && perm_city_y(&perms[k]) == y) return true;
  return false;
}
/* a column's obstructions: pairs of (top, bottom), from the top down */
static int column_obstructions(int i, int16_t *ob) {
  int n = 0, x = (i + grid_istart) * 20;
  for (int k = 0; k < nsorted; k++) {
    int w = sorted_worlds[k];
    const World *wd = &worlds[w];
    if (x < wd->x || x >= wd->x + wd->w) continue;
    int idx = (x - wd->x) / 20;
    int maxy = -1;
    if (wd->h > 0)
      for (int j = 0; j * 20 < wd->h; j++)
        if (world_mask_at(w, idx, j)) maxy = j;
    int len = col_stack_height(w, idx);
    int start = wd->y - len * 20;
    bool hole = false;
    const Perm *top = len ? P(perm_at(w, idx, len - 1)) : NULL;
    bool top_pad = is_pad(top);
    for (int pm = len - 1; pm >= 0; pm--) {
      const Perm *q = P(perm_at(w, idx, pm));
      if (!q) {
        if (!hole) {
          if (pm != len - 1) {
            int end = wd->y - (pm + 1) * 20;
            if (top_pad) start += 20;
            if (n == 0 || start > ob[n - 1]) {
              if (n + 2 <= FLY_MAX_OB) ob[n++] = (int16_t)start, ob[n++] = (int16_t)end;
            } else {
              ob[n - 1] = (int16_t)end;
            }
          }
          hole = true;
        }
      } else if (hole) {
        start = wd->y - (pm + 1) * 20;
        hole = false;
        top_pad = is_pad(q);
      }
    }
    if (hole) start = wd->y;
    if (maxy >= 0) {
      int end = (maxy + 1) * 20 + wd->y;
      if (pad_in_column_at(i, start)) start += 20;
      if (n == 0 || start > ob[n - 1]) {
        if (n + 2 <= FLY_MAX_OB) ob[n++] = (int16_t)start, ob[n++] = (int16_t)end;
      } else {
        ob[n - 1] = (int16_t)end;
      }
    }
  }
  return n;
}
static void sort_unique(int16_t *v, int *n) {
  for (int a = 1; a < *n; a++) {
    int16_t k = v[a];
    int b = a - 1;
    while (b >= 0 && v[b] > k) v[b + 1] = v[b], b--;
    v[b + 1] = k;
  }
  int m = 0;
  for (int a = 0; a < *n; a++)
    if (m == 0 || v[m - 1] != v[a]) v[m++] = v[a];
  *n = m;
}
/* addNextWaypointToAvoidBlockers */
static void avoid_blockers(int wp, const int16_t *next, int nn, int yfrom, int yto, int16_t *out, int *no) {
  for (int b = 0; b < nn; b += 2) {
    if (wp >= next[b] && wp < next[b + 1]) {
      if (*no < FLY_MAX_WP) {
        if (next[b] > yfrom) out[(*no)++] = (int16_t)(next[b] - 20);
        else if (next[b + 1] < yto) out[(*no)++] = next[b + 1];
      }
      return;
    }
    if (next[b] > wp) return;
  }
}
#define fly_y (scratch->u.fly.y)
#define fly_col (scratch->u.fly.col)
#define fly_seg (scratch->u.fly.seg)
#define fly_start (scratch->u.fly.colstart)
static int fly_lo, fly_n, fly_ncols;
/* the waypoints of columns lo..hi (grid indexes); false if they do not fit */
static bool build_waypoints(int lo, int hi) {
  int16_t cur[FLY_MAX_OB], next[FLY_MAX_OB];   /* (on the stack: only while a route is looked for) */
  int16_t prev_wp[FLY_MAX_WP], wp[FLY_MAX_WP];
  if (hi - lo + 1 > 255) return false;
  int nprev = 0, ncur, nnext;
  fly_lo = lo;
  fly_n = 0;
  fly_ncols = hi - lo + 1;
  ncur = column_obstructions(1, cur);
  bool ok = true;
  for (int i = 1; i < grid_len - 1 && i <= hi; i++) {
    nnext = column_obstructions(i + 1, next);
    int nwp = 0, segs = ncur / 2;
    int prevpos = 0, k = 0;
    bool store = i >= lo;
    if (store) fly_start[i - lo] = (uint16_t)fly_n;
    for (int j = 0; j <= segs; j++) {
      int16_t these[FLY_MAX_WP];
      int nt = 0;
      int yfrom = -1000000, yto = 1000000;
      if (segs != 0) {
        if (j == 0) yto = cur[0];
        else if (j == segs) yfrom = cur[segs * 2 - 1];
        else yfrom = cur[j * 2 - 1], yto = cur[j * 2];
      }
      while (k < nnext) {
        if (next[k] > yfrom && next[k + 1] < yto) {
          if (nt + 2 <= FLY_MAX_WP) these[nt++] = (int16_t)(next[k] - 20), these[nt++] = next[k + 1];
        } else if (next[k] > yfrom) {
          break;
        }
        k += 2;
      }
      while (prevpos < nprev) {
        int v = prev_wp[prevpos];
        if (v >= yfrom && v < yto) {
          if (nt < FLY_MAX_WP) these[nt++] = (int16_t)v;
          avoid_blockers(v, next, nnext, yfrom, yto, these, &nt);
        } else if (v >= yfrom) {
          break;
        }
        prevpos++;
      }
      for (int q = 1; q < MAX_PERMS; q++) {
        const Perm *pad = &perms[q];
        if (!is_pad(pad) || pad_column(pad) != i) continue;
        int py = perm_city_y(pad);
        if (py >= yfrom && py <= yto - 20) {
          if (nt < FLY_MAX_WP) these[nt++] = (int16_t)py;
          avoid_blockers(py, next, nnext, yfrom, yto, these, &nt);
        }
      }
      sort_unique(these, &nt);
      for (int t = 0; t < nt; t++) {
        if (nwp < FLY_MAX_WP) wp[nwp++] = these[t];
        if (store) {
          if (fly_n >= MAX_PERMS) {
            ok = false;
            continue;
          }
          fly_y[fly_n] = these[t];
          fly_col[fly_n] = (uint8_t)(i - lo);
          fly_seg[fly_n] = (uint8_t)j;
          fly_n++;
        }
      }
    }
    memcpy(prev_wp, wp, sizeof wp[0] * (size_t)nwp);
    nprev = nwp;
    memcpy(cur, next, sizeof cur[0] * (size_t)nnext);
    ncur = nnext;
  }
  fly_start[hi - lo + 1] = (uint16_t)fly_n;
  return ok;
}
static int node_at(int col, int y) {
  if (col < 0 || col >= fly_ncols) return -1;
  for (int k = fly_start[col]; k < fly_start[col + 1]; k++)
    if (fly_y[k] == y) return k;
  return -1;
}
#define fprio (scratch->prio)
#define fprev (scratch->prev)
#define fseen (scratch->seen)
static void fly_relax(int from, int to) {
  int fx = (fly_col[from] + fly_lo + grid_istart) * 20, tx = (fly_col[to] + fly_lo + grid_istart) * 20;
  float np = fprio[from] + (float)(abs(fx - tx) + abs(fly_y[from] - fly_y[to]));
  if (fseen[to] == 0) {
    fseen[to] = 1;
    fprev[to] = (uint16_t)from;
    fprio[to] = np;
    heap_push((uint16_t)to);
  } else if (fseen[to] == 1 && np < fprio[to]) {
    fprev[to] = (uint16_t)from;
    fprio[to] = np;
    heap_up(heap_index((uint16_t)to));
  }
}
/* findRoute: the waypoints from b back to a (out[0] is b), as turning points; -1 if none, 0 if too big to find */
static int find_route(const Perm *a, const Perm *b, int16_t out[][2], int max) {
  grid_setup();
  int ca = pad_column(a), cb = pad_column(b);
  int lo = (ca < cb ? ca : cb) - FLY_MARGIN, hi = (ca > cb ? ca : cb) + FLY_MARGIN;
  if (lo < 1) lo = 1;
  if (hi > grid_len - 2) hi = grid_len - 2;
  if (ca < 1 || cb < 1 || ca > grid_len - 2 || cb > grid_len - 2) return -1;
  if (!build_waypoints(lo, hi)) return 0;
  int s = node_at(ca - lo, perm_city_y(a)), g = node_at(cb - lo, perm_city_y(b));
  if (s < 0 || g < 0) return -1;
  memset(fseen, 0, (size_t)fly_n);
  heap_n = 0;
  fprio[s] = 0;
  fseen[s] = 1;
  heap_push((uint16_t)s);
  bool found = false;
  while (heap_n) {
    int v = heap_pop();
    fseen[v] = 2;
    if (v == g) {
      found = true;
      break;
    }
    if (v > 0 && fly_col[v - 1] == fly_col[v] && fly_seg[v - 1] == fly_seg[v]) fly_relax(v, v - 1);
    if (v + 1 < fly_n && fly_col[v + 1] == fly_col[v] && fly_seg[v + 1] == fly_seg[v]) fly_relax(v, v + 1);
    int l = node_at(fly_col[v] - 1, fly_y[v]), r = node_at(fly_col[v] + 1, fly_y[v]);
    if (l >= 0 && fly_col[v] > 0) fly_relax(v, l);
    if (r >= 0 && fly_col[v] + 1 <= hi - lo) fly_relax(v, r);
  }
  if (!found) return -1;
  /* the route from the goal back to the start, without the points in straight runs */
  int n = 0, v = g;
  int16_t px = 0, py = 0;
  bool first = true;
  for (;;) {
    int16_t x = (int16_t)((fly_col[v] + lo + grid_istart) * 20), y = fly_y[v];
    if (!first && n >= 2 && ((out[n - 1][0] == x && out[n - 2][0] == x) || (out[n - 1][1] == y && out[n - 2][1] == y))) {
      out[n - 1][0] = x, out[n - 1][1] = y;
    } else if (n < max) {
      out[n][0] = x, out[n][1] = y;
      n++;
    } else {
      out[n - 1][0] = x, out[n - 1][1] = y;   /* (too many turns: the last is cut straight to the start) */
    }
    first = false;
    px = x, py = y;
    if (v == s) break;
    v = fprev[v];
  }
  (void)px, (void)py;
  return n;
}
/* a pad in its column's open sky (no obstruction above it) */
static bool pad_in_sky(const Perm *p) {
  int16_t ob[FLY_MAX_OB];
  int n = column_obstructions(pad_column(p), ob);
  return n == 0 || perm_city_y(p) <= ob[0] - 20;
}
static bool pads_connected(const Perm *a, const Perm *b) {
  grid_setup();
  if (pad_in_sky(a) && pad_in_sky(b)) return true;
  int16_t route[2][2];
  int n = find_route(a, b, route, 2);
  return n != -1;
}
/* findAnyOtherLandingSite (for CityConnections' landing groups): a pad already in a group it can fly to */
uint16_t flying_find_any_other_landing_site(uint16_t ls) {
  const Perm *a = P(ls);
  if (!is_pad(a)) return NONE;
  grid_setup();
  int ca = pad_column(a);
  if (ca < 1 || ca > grid_len - 2) return NONE;
  for (int i = 1; i < MAX_PERMS; i++)
    if (i != ls && is_pad(&perms[i]) && landing_group_of[i] && pads_connected(a, &perms[i])) return (uint16_t)i;
  return NONE;
}
int flying_columns(void) {
  grid_setup();
  return grid_len > 2 ? grid_len - 2 : 0;
}
int flying_highest_y(int index, int *x) {
  int16_t ob[FLY_MAX_OB];
  int i = index + 1;
  *x = (i + grid_istart) * 20;
  int n = column_obstructions(i, ob);
  return n == 0 ? -1000000 : ob[0];
}

/* ---------------------------------------------------------------- flying saucers (simulation.FlyingSaucer) */
#define MAX_SAUCERS 12
#define SAUCER_POINTS 8
typedef struct {
  uint16_t start, dest;          /* landing sites (start 0: a free slot) */
  float x, y;
  uint8_t leavingIn;             /* ticks x 8 */
  uint8_t pathPos, npts, type;
  int16_t pts[SAUCER_POINTS][2]; /* the route: pts[0] is the destination */
} Saucer;
static Saucer saucers[MAX_SAUCERS];
#define PASSENGER 0x8000         /* a passenger's thenArg: PASSENGER | its saucer */

static const uint16_t saucer_sprites[6] = {SPR_FLYINGSAUCER, SPR_FLYINGSAUCER_ALT, SPR_FLOWERCRAFT, SPR_FLYINGSAUCER_CUBE,
                                           SPR_FLYINGSAUCER_DEEPBLUE, SPR_FLYINGSAUCER_SHADE};
static void release(Cit *c) {
  c->flags &= (uint16_t)~CF_CONTROLLED;
  c->thenArg = 0;
}
/* cancel: the passengers stay where they were, without a path */
static void saucer_cancel(int k) {
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if ((c->flags & CF_ALIVE) && (c->flags & CF_CONTROLLED) && c->thenArg == (PASSENGER | k)) {
      release(c);
      cit_reset_path(c, true);
    }
  }
  saucers[k].start = 0;
  saucers[k].dest = 0;
}
void saucer_on_landing_destroyed(uint16_t ls) {
  for (int k = 0; k < MAX_SAUCERS; k++)
    if (saucers[k].start && saucers[k].dest == ls) saucer_cancel(k);
}
static float pad_distance(const Perm *a, const Perm *b) { return bld_flying_distance(a, b); }
static void pad_remove_saucer(Perm *ls) {
  LandingState *s = ST(LandingState, ls);
  if (s->hasSaucer) {
    s->hasSaucer = 0;
    s->newStage = 16 * 8;
    s->newWait = 30;
  }
}
static void pad_add_saucer(Perm *ls) {
  LandingState *s = ST(LandingState, ls);
  if (!s->hasSaucer) s->hasSaucer = 1, s->newStage = 0, s->newWait = 0;
}
static void add_passenger(int k, Cit *c) {
  Perm *start = P(saucers[k].start);
  if (is_pad(start) && ST(LandingState, start)->timesUsed < 65535) ST(LandingState, start)->timesUsed++;
  c->flags |= CF_CONTROLLED;
  c->flags &= (uint16_t)~CF_CAN_VIEW_SELF;
  c->thenArg = (uint16_t)(PASSENGER | k);
}
/* a landing site's path step (Citizen.updatePath, step 11): board a saucer (false: the path was dropped) */
bool flying_board(Cit *c, Perm *in, Perm *dest) {
  if (!is_pad(in) || !is_pad(dest)) {
    cit_reset_path(c, true);
    return false;
  }
  LandingState *s = ST(LandingState, in);
  int landed = s->landed - 1;
  if (landed >= 0 && saucers[landed].start == pid(in) && saucers[landed].leavingIn > 0 &&
      (saucers[landed].dest == pid(dest) || pad_distance(P(saucers[landed].dest), dest) < pad_distance(in, dest))) {
    Saucer *sc = &saucers[landed];
    add_passenger(landed, c);
    if (sc->dest != pid(dest) && pad_distance(in, dest) < pad_distance(in, P(sc->dest))) {
      int16_t pts[SAUCER_POINTS][2];
      int n = find_route(in, dest, pts, SAUCER_POINTS);
      if (n > 0) {
        memcpy(sc->pts, pts, sizeof pts);
        sc->npts = (uint8_t)n;
        sc->pathPos = (uint8_t)(n - 1);
        sc->dest = pid(dest);
      }
    }
    return true;
  }
  if (s->hasSaucer) {
    int k = -1;
    for (int i = 0; i < MAX_SAUCERS && k < 0; i++)
      if (!saucers[i].start) k = i;
    if (k < 0) {   /* (every saucer is in the air: wait for one) */
      c->relativeY = -1;
      return true;
    }
    int16_t pts[SAUCER_POINTS][2];
    int n = find_route(in, dest, pts, SAUCER_POINTS);
    if (n == 0) {   /* too far to work out in memory: straight up, across and down */
      int ax = perm_city_x(in) / 20 * 20, bx = perm_city_x(dest) / 20 * 20;
      int top = perm_city_y(in) < perm_city_y(dest) ? perm_city_y(in) : perm_city_y(dest);
      grid_setup();
      int lo = pad_column(in) < pad_column(dest) ? pad_column(in) : pad_column(dest);
      int hi = pad_column(in) > pad_column(dest) ? pad_column(in) : pad_column(dest);
      for (int i = lo; i <= hi; i++) {
        int16_t ob[FLY_MAX_OB];
        int no = column_obstructions(i, ob);
        if (no && ob[0] - 20 < top) top = ob[0] - 20;
      }
      pts[0][0] = (int16_t)bx, pts[0][1] = (int16_t)perm_city_y(dest);
      pts[1][0] = (int16_t)bx, pts[1][1] = (int16_t)top;
      pts[2][0] = (int16_t)ax, pts[2][1] = (int16_t)top;
      pts[3][0] = (int16_t)ax, pts[3][1] = (int16_t)perm_city_y(in);
      n = 4;
    }
    if (n < 0) {
      cit_reset_path(c, true);
      return false;
    }
    Saucer *sc = &saucers[k];
    memset(sc, 0, sizeof *sc);
    sc->start = pid(in), sc->dest = pid(dest);
    memcpy(sc->pts, pts, sizeof pts);
    sc->npts = (uint8_t)n;
    sc->pathPos = (uint8_t)(n - 1);
    sc->x = sc->pts[n - 1][0], sc->y = sc->pts[n - 1][1];
    sc->leavingIn = 12 * 8;
    sc->type = s->saucerType;
    s->landed = (uint8_t)(k + 1);
    add_passenger(k, c);
    pad_remove_saucer(in);
    c->relativeX = (float)rnd_between(10 - 3, 10 + 4);
    return true;
  }
  c->relativeY = -1;
  return true;
}
static void saucer_arrive(int k) {
  Saucer *sc = &saucers[k];
  Perm *dest = P(sc->dest);
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE) || !(c->flags & CF_CONTROLLED) || c->thenArg != (PASSENGER | k)) continue;
    release(c);
    if (!is_pad(dest)) continue;
    cit_go_to_permanent(c, sc->dest);
    c->onWorld = dest->world;
    LandingState *ds = ST(LandingState, dest);
    if (c->pathActionPerm != sc->dest) {
      if (ds->timesUsedStopOver < 65535) ds->timesUsedStopOver++;
    } else if (ds->timesUsedTo < 65535) {
      ds->timesUsedTo++;
    }
  }
  if (is_pad(dest)) pad_add_saucer(dest);
  sc->start = 0;
}
static void saucers_update(float timeMod) {
  for (int k = 0; k < MAX_SAUCERS; k++) {
    Saucer *sc = &saucers[k];
    if (!sc->start) continue;
    if (sc->leavingIn > 0) {
      int d = round_random(timeMod * 8);
      sc->leavingIn = (uint8_t)(sc->leavingIn > d ? sc->leavingIn - d : 0);
      if (sc->leavingIn == 0 && is_pad(P(sc->start)) && ST(LandingState, P(sc->start))->landed == k + 1)
        ST(LandingState, P(sc->start))->landed = 0;
      continue;
    }
    float left = timeMod * 3;
    int guard = 0;
    while (left > 0 && guard++ < 64) {
      float gx = sc->pts[sc->pathPos][0], gy = sc->pts[sc->pathPos][1];
      if (fabsf(sc->x - gx) > 0.01f) {
        float d = gx - sc->x;
        sc->x += (d > 0 ? 1 : -1) * fminf(left, fabsf(d));
        left -= fabsf(d);
      } else if (fabsf(sc->y - gy) > 0.01f) {
        float d = gy - sc->y;
        sc->y += (d > 0 ? 1 : -1) * fminf(left, fabsf(d));
        left -= fabsf(d);
      } else {
        sc->x = gx, sc->y = gy;
        if (sc->pathPos == 0) {
          saucer_arrive(k);
          break;
        }
        sc->pathPos--;
      }
    }
  }
}

/* ---------------------------------------------------------------- LandingSite */
static void landing_init(Perm *p) { ST(LandingState, p)->hasSaucer = 1; }
static void landing_destroy(Perm *p) { saucer_on_landing_destroyed(pid(p)); }
static void landing_update(Perm *p, float timeMod) {
  LandingState *s = ST(LandingState, p);
  if (s->newWait > 0) {
    s->newWait = tick_down(s->newWait, timeMod);
  } else if (s->newStage > 0) {
    int d = round_random(timeMod * 8);
    s->newStage = (uint8_t)(s->newStage > d ? s->newStage - d : 0);
    if (s->newStage == 0) s->hasSaucer = 1;
  }
}
static uint16_t pad_texture(const Perm *p) {
  int t = STC(LandingState, p)->saucerType;
  return t == 1 ? SPR_LANDINGPLACE_ALT : t == 4 ? SPR_LANDINGPLACE_DEEPBLUE : SPR_LANDINGPLACE;
}
static void landing_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, pad_texture(p), false);
  if (layer != LAYER_MID) return;
  /* the saucer on the pad (aboveCitizensInBuildingStage), coming down when a new one arrives */
  const LandingState *s = STC(LandingState, p);
  uint16_t spr = saucer_sprites[s->saucerType % 6];
  if (s->landed && saucers[s->landed - 1].start == pid(p)) {
    dr_sprite(b, saucer_sprites[saucers[s->landed - 1].type % 6], x, y, 0, 0, 0, 255);
  } else if (s->hasSaucer) {
    dr_sprite(b, spr, x, y, 0, 0, 0, 255);
  } else if (s->newWait == 0 && s->newStage > 0) {
    float stage = s->newStage / 8.0f;
    float a = 1 - stage / 16;
    dr_sprite(b, spr, x, y, 0, -(int)(stage * 0.25f), 0, (uint8_t)(a < 0 ? 0 : a * 255));
  }
}
static void landing_window(Perm *p, WinOut *w) {
  const LandingState *s = STC(LandingState, p);
  int from = s->timesUsed - s->timesUsedStopOver;
  win_line1(w, "Transported from [0] times.", from > 0 ? from : 0);
  win_line1(w, "Transported to [0] times.", s->timesUsedTo);
  win_line1(w, "Used as stopover [0] times.", s->timesUsedStopOver);
  if (bld_secret_hangar) win_button(w, 0, "Change Space Ship", WA_CYCLE_0, true);
}
static void landing_action(Perm *p, int a) {
  if (a == WA_CYCLE_0 && bld_secret_hangar) {
    LandingState *s = ST(LandingState, p);
    s->saucerType = (uint8_t)((s->saucerType + 1) % 6);
  }
}
const PermOps ops_LandingSite = {.init = landing_init, .destroy = landing_destroy, .update = landing_update,
                                 .draw = landing_draw, .window = landing_window, .window_action = landing_action};

/* ---------------------------------------------------------------- saucers bringing citizens (SpawnFlyingSaucer) */
#define MAX_SPAWNERS 6
typedef struct { uint8_t world, col, ageMin, ageMax; uint16_t amount; float above; } Spawner;
static Spawner spawners[MAX_SPAWNERS];
static int nspawners;
int spawners_pending(void) {
  int n = 0;
  for (int i = 0; i < nspawners; i++) n += spawners[i].amount;
  return n;
}
/* simulation.SpawnFlyingSaucer: toCol < 0 picks an empty column (or one with a building) at random */
void saucer_spawn(int world, int amount, int ageMin, int ageMax, int toCol) {
  if (world < 0 || world >= nworlds || amount <= 0) return;
  const World *wd = &worlds[world];
  if (toCol < 0) {
    int best = -5, n = 0;
    uint8_t options[255];
    for (int x = 0; x < wd->ncols && x < 255; x++) {
      int q = -1, h = col_stack_height(world, x);
      const Perm *bottom = h ? P(perm_at(world, x, 0)) : NULL;
      if (h == 0) q = 1;
      else if (is_building(bottom) && !(bottom->flags & PF_ROOFTOP)) q = 0;
      if (q > best) best = q, n = 0;
      if (q >= best) options[n++] = (uint8_t)x;
    }
    if (!n) return;
    toCol = options[rnd_int(n)];
  }
  if (nspawners >= MAX_SPAWNERS) {   /* (no room for another in the air: they come at once) */
    nspawners--;
  }
  Spawner *s = &spawners[nspawners++];
  s->world = (uint8_t)world, s->col = (uint8_t)toCol;
  s->ageMin = (uint8_t)ageMin, s->ageMax = (uint8_t)ageMax;
  s->amount = (uint16_t)amount;
  s->above = 1000.0f + col_stack_height(world, toCol) * 20;
}
uint16_t cit_create(int world, float age, uint16_t start_in, int start_x);
static void spawn_citizens(const Spawner *s) {
  int h = col_stack_height(s->world, s->col);
  uint16_t start = NONE;
  int x = -1;
  if (h == 0) x = s->col * 20;
  else start = perm_at(s->world, s->col, h - 1);
  for (int i = 0; i < s->amount; i++) {
    float age = s->ageMin;
    if (s->ageMin != s->ageMax) age = rnd_range(s->ageMin, s->ageMax);
    if (ncits + spawners_pending() < babies.softPopLimit) cit_create(s->world, age, start, x);
  }
}
void spawners_update(float timeMod) {
  for (int i = nspawners - 1; i >= 0; i--) {
    Spawner *s = &spawners[i];
    s->above -= timeMod * 3;
    if (s->above <= col_stack_height(s->world, s->col) * 20) {
      Spawner done = *s;
      spawn_citizens(&done);
      for (int k = i; k + 1 < nspawners; k++) spawners[k] = spawners[k + 1];
      nspawners--;
    }
  }
  festival_update(timeMod);   /* (CitySimulation: festivalManager.update comes after the spawners) */
}

/* ---------------------------------------------------------------- each step, before the citizens */
void flying_update(float timeMod) {
  for (int k = 0; k < MAX_SAUCERS; k++) (void)k;
  saucers_update(timeMod);
  particles_update(timeMod);
}

/* the saucers in the air, the spawners' saucers and the particles */
void flying_draw_free(const GfxBand *b, int layer) {
  if (layer == LAYER_MID) {
    for (int k = 0; k < MAX_SAUCERS; k++) {
      const Saucer *sc = &saucers[k];
      if (!sc->start || sc->leavingIn > 0) continue;
      gfx_band_sprite(b, saucer_sprites[sc->type % 6], 0, 0, 20, 20, bg_screen_x(&camera, sc->x), bg_screen_y(&camera, sc->y), 0,
                      255, GFX_NO_TINT);
    }
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (!particles[i].spr) continue;
      int f = (int)particles[i].frame;
      gfx_band_sprite(b, particles[i].spr, (particles[i].fw + 2) * f, 0, particles[i].fw, spr_h(particles[i].spr),
                      bg_screen_x(&camera, particles[i].x), bg_screen_y(&camera, particles[i].y), 0, 255, GFX_NO_TINT);
    }
  } else if (layer == LAYER_FRONT) {
    for (int i = 0; i < nspawners; i++) {
      const Spawner *s = &spawners[i];
      const World *wd = &worlds[s->world];
      float x = (float)(wd->x + s->col * 20), y = wd->y - s->above - 20 + 1;
      gfx_band_sprite(b, SPR_FLYINGSAUCER, 0, 0, 20, 20, bg_screen_x(&camera, x), bg_screen_y(&camera, y), 0, 255, GFX_NO_TINT);
    }
  }
}

/* ---------------------------------------------------------------- start, save, load */
void saucers_new_city(void) {
  memset(saucers, 0, sizeof saucers);
  memset(spawners, 0, sizeof spawners);
  memset(particles, 0, sizeof particles);
  nspawners = 0;
}
void saucers_save(SaveIO *io) {
  save_bytes(io, saucers, sizeof saucers);
  save_u8(io, (uint8_t)nspawners);
  save_bytes(io, spawners, sizeof spawners);
}
bool saucers_load(SaveIO *io) {
  load_bytes(io, saucers, sizeof saucers);
  nspawners = load_u8(io);
  load_bytes(io, spawners, sizeof spawners);
  if (nspawners > MAX_SPAWNERS) return false;
  /* (a passenger was let go where they were when the city was saved: the flights land empty) */
  return true;
}

void bld_transport_init(void) {}
