/* The Final Earth 2: the citizens' pathfinder (js/pathfinder.js, run in a
 * web worker in the original, and pathfinder.PathfinderManager).
 *
 * The graph's nodes are the permanents. Edges: walking through a building to
 * the one above (0), below (1), left (2) or right (3), all costing 20, left
 * and right only above the ground or across worlds; along a world's surface
 * between its ground-level permanents (5, costing the distance); between hyper
 * elevators of a column (13); between teleporters (7); and flying between
 * landing sites of an air group (11). A search is Dijkstra's from where the
 * citizen is to a goal: a permanent, any open entertainment of some types,
 * or several permanents for citizens leaving the same building.
 *
 * Requests are answered at the end of the tick that made them, a few per
 * tick, as the original's workers answer within frames. */
#include "sim.h"
#include "bld.h"
#include "scratch.h"

/* node types of the original (get_typeID) */
enum { NT_NORMAL = 0, NT_TELEPORTER = 1, NT_LANDING = 2, NT_RESOURCE = 3, NT_ELEVATOR = 4 };

/* the search's memory (scratch.h), shared with the PermanentFinder (assign.c), which never runs at the same time */
#define prio (scratch->prio)
#define prev (scratch->prev)
#define in_edge (scratch->in_edge)
#define seen (scratch->seen)
#define heap (scratch->heap)
#define heap_pos (scratch->heap_pos)
int heap_n;
static int8_t ent_bits[MAX_PERMS];        /* isEntertainmentPossibleWithType: -1 none, else 1 << type if open */
static uint8_t lowest_tunnel[MAX_COLS];   /* worldFlyingLowestTunnel */

static int node_type(const Perm *p) {
  if (!is_building(p)) return NT_RESOURCE;
  const PermClass *pc = PC(p);
  if (pc->base & B_TELEPORTER) return NT_TELEPORTER;
  if (pc->base & B_LANDINGSITE) return NT_LANDING;
  if (pc->base & B_ELEVATOR) return NT_ELEVATOR;
  return NT_NORMAL;
}

/* ---------------------------------------------------------------- the heap (a min priority queue) */
void heap_up(int i) {
  uint16_t v = heap[i];
  while (i > 1 && prio[heap[i >> 1]] > prio[v]) {
    heap[i] = heap[i >> 1];
    heap_pos[heap[i]] = (uint16_t)i;
    i >>= 1;
  }
  heap[i] = v;
  heap_pos[v] = (uint16_t)i;
}
static void heap_down(int i) {
  uint16_t v = heap[i];
  for (;;) {
    int c = i << 1;
    if (c > heap_n) break;
    if (c < heap_n && prio[heap[c + 1]] < prio[heap[c]]) c++;
    if (prio[heap[c]] >= prio[v]) break;
    heap[i] = heap[c];
    heap_pos[heap[i]] = (uint16_t)i;
    i = c;
  }
  heap[i] = v;
  heap_pos[v] = (uint16_t)i;
}
void heap_push(uint16_t id) { heap[++heap_n] = id; heap_up(heap_n); }
uint16_t heap_pop(void) {
  uint16_t top = heap[1];
  heap[1] = heap[heap_n--];
  if (heap_n) heap_down(1);
  return top;
}

uint16_t heap_index(uint16_t id) { return heap_pos[id]; }

static void relax(uint16_t from, uint16_t to, uint8_t type, float cost) {
  float np = prio[from] + cost;
  if (seen[to] == 0) {
    in_edge[to] = type, prev[to] = from, prio[to] = np;
    heap_push(to);
    seen[to] = 1;
  } else if (seen[to] == 1 && np < prio[to]) {
    in_edge[to] = type, prev[to] = from, prio[to] = np;
    heap_up(heap_pos[to]);
  }
}

/* ---------------------------------------------------------------- one search */
typedef struct {
  uint16_t cit;
  uint8_t kind;          /* RQ_* */
  uint8_t on_fail;
  uint16_t goal;         /* permanent, or entertainment type bits */
  int16_t max_distance;
} Request;
enum { RQ_PATH, RQ_COMBINED, RQ_ENTERTAINMENT };
#define MAX_REQ 128
static Request req[MAX_REQ];
static int nreq;
static int done_last_tick;

/* the path to id, as (type, value) pairs into out (returns the number of steps) */
static int backtrace(uint16_t id, bool from_outside, int16_t *out, int max) {
  int len = 0;
  for (uint16_t b = id; prev[b]; b = prev[b]) len++;
  int total = len + (from_outside ? 1 : 0);
  if (total > max) return -1;
  int k = total;
  uint16_t b = id;
  while (prev[b]) {
    k--;
    out[2 * k] = in_edge[b], out[2 * k + 1] = (int16_t)b;
    b = prev[b];
  }
  if (from_outside) out[0] = 5, out[1] = (int16_t)b;
  return total;
}

static int handled_air[64];
static uint16_t teleporters_found[64];
static int nteleporters;

/* Dijkstra's from a citizen's place; goal_type GOAL_PERM (goals[0]), entertainment (bits in goals[0]) or several permanents */
static void search(const Cit *c, int goal_type, const uint16_t *goals, int ngoals, float max_distance,
                   void (*found)(int goal_index, uint16_t at, bool from_outside)) {
  memset(seen, 0, sizeof seen);
  memset(handled_air, 0, sizeof handled_air);
  heap_n = 0;
  bool outside = c->inPermanent == NONE;
  uint16_t private_tp = (c->home && bld_has_private_teleporter(P(c->home))) ? c->home : NONE;
  if (!outside) {
    uint16_t s = c->inPermanent;
    prev[s] = NONE;
    in_edge[s] = 0;
    prio[s] = 0;
    heap_push(s);
    seen[s] = 1;
  } else {
    World *w = &worlds[c->onWorld];
    for (int x = 0; x < w->ncols; x++) {
      uint16_t b = perm_at(c->onWorld, x, 0);
      if (!b) continue;
      prev[b] = NONE;
      in_edge[b] = 5;
      prio[b] = fabsf(20.0f * x - (int)c->relativeX);
      heap_push(b);
      seen[b] = 1;
    }
  }
  bool handled_tp = !operating_cost.teleportersEnabled;
  int remaining = ngoals;
  while (heap_n) {
    uint16_t cur = heap_pop();
    Perm *p = &perms[cur];
    bool reached = false;
    if (goal_type == GOAL_PERM) {
      reached = cur == goals[0];
    } else if (goal_type == GOAL_ENTERTAINMENT) {
      int g = (int16_t)goals[0];
      reached = (g == -1 && ent_bits[cur] >= 0) || (g > 0 && ent_bits[cur] > 0 && (g & ent_bits[cur]) > 0);
    } else {
      for (int i = 0; i < ngoals; i++)
        if (goals[i] == cur) {
          found(i, cur, outside);
          remaining--;
        }
      if (remaining <= 0) return;
    }
    if (reached) {
      found(0, cur, outside);
      return;
    }
    if (prio[cur] > max_distance) break;
    seen[cur] = 2;
    int nt = node_type(p);
    if (nt != NT_RESOURCE) {
      /* walking through: left and right above the ground (or across worlds), up and down */
      bool lr_ok = !(p->flags & PF_NO_BUILD_ON_TOP);
      if (lr_ok && p->left && (p->row > 0 || P(p->left)->world != p->world) && !(P(p->left)->flags & PF_NO_BUILD_ON_TOP))
        relax(cur, p->left, 2, 20);
      if (lr_ok && p->right && (p->row > 0 || P(p->right)->world != p->world) && !(P(p->right)->flags & PF_NO_BUILD_ON_TOP))
        relax(cur, p->right, 3, 20);
      if (!(p->flags & PF_ROOFTOP)) {
        uint16_t up = perm_at(p->world, p->col, p->row + 1);
        if (up) relax(cur, up, 0, 20);
      }
      if (p->row > 0) {
        Perm *dn = P(perm_at(p->world, p->col, p->row - 1));
        if (dn && is_building(dn) && !(dn->flags & PF_ROOFTOP)) relax(cur, pid(dn), 1, 20);
      }
      if (nt == NT_ELEVATOR) {
        /* the next hyper elevators up and down the column, costing the buildings between */
        int h = col_stack_height(p->world, p->col);
        for (int dir = -1; dir <= 1; dir += 2) {
          int dist = 0;
          for (int y = p->row + dir; y >= 0 && y < h; y += dir) {
            Perm *q = P(perm_at(p->world, p->col, y));
            if (!q) break;
            dist++;
            if (node_type(q) == NT_ELEVATOR) {
              relax(cur, pid(q), 13, (float)dist);
              break;
            }
          }
        }
      }
    }
    if (nt == NT_LANDING) {
      int g = landing_group_of[cur] - 1;
      if (g >= 0 && g < 64 && handled_air[g] < 5) {
        float x = (float)perm_city_x(p), y = (float)worlds[p->world].y - p->row * 20.0f;
        for (int j = 1; j < MAX_PERMS; j++) {
          if (landing_group_of[j] != g + 1 || j == cur) continue;
          Perm *q = &perms[j];
          float add;
          if (q->world == p->world) {
            int maxy = 0;
            int lo = p->col < q->col ? p->col : q->col, hi = p->col > q->col ? p->col : q->col;
            for (int xx = lo; xx <= hi; xx++)
              if (lowest_tunnel[worlds[p->world].col0 + xx] > maxy) maxy = lowest_tunnel[worlds[p->world].col0 + xx];
            int a = abs((int)p->row - (int)q->row);
            int b = (q->row > maxy ? q->row : maxy) - q->row + (p->row > maxy ? p->row : maxy) - p->row;
            add = 10 + (fabsf((float)(q->col - p->col) * 20) + 20.0f * (a > b ? a : b)) / 3;
          } else {
            add = (fabsf(perm_city_x(q) - x) + fabsf(worlds[q->world].y - q->row * 20.0f - y)) / 1.5f;
          }
          relax(cur, (uint16_t)j, 11, add);
        }
        handled_air[g]++;
      }
    } else if (!handled_tp && (nt == NT_TELEPORTER || cur == private_tp)) {
      float rand_offset = rnd_range(0, 2);
      float inv = nteleporters ? 2.0f / nteleporters : 0;
      for (int t = 0; t < nteleporters; t++) {
        float cost = 10 + fmodf(rand_offset + t * inv, 2.0f);
        relax(cur, teleporters_found[t], 7, cost);
      }
      if (cur != private_tp && private_tp) relax(cur, private_tp, 7, 10);
      handled_tp = true;
    }
    if (p->row == 0 && in_edge[cur] != 5) {
      /* along the surface to the world's other ground-level permanents */
      World *w = &worlds[p->world];
      for (int x = 0; x < w->ncols; x++) {
        uint16_t b = perm_at(p->world, x, 0);
        if (b && b != cur) relax(cur, b, 5, fabsf(20.0f * (x - p->col)));
      }
    }
  }
  /* not found */
  if (goal_type == GOAL_PERM || goal_type == GOAL_ENTERTAINMENT) found(-1, NONE, outside);
  else
    for (int i = 0; i < ngoals; i++) found(-1 - i, NONE, outside);
}

/* ---------------------------------------------------------------- answering */
static int16_t pairs[256];
static const Cit *cur_cit;
static uint16_t multi_cits[32];

static void give_found(int goal_index, uint16_t at, bool from_outside) {
  Cit *c = (Cit *)cur_cit;
  if (goal_index < 0) {
    cit_set_found_path(c, NULL, -1);
    return;
  }
  int n = backtrace(at, from_outside, pairs, 128);
  cit_set_found_path(c, pairs, n);
}
static void give_found_multi(int goal_index, uint16_t at, bool from_outside) {
  if (goal_index < 0) {
    Cit *c = &cits[multi_cits[-1 - goal_index]];
    if ((c->flags & CF_ALIVE) && (c->flags & CF_REQUESTING_PATH)) cit_set_found_path(c, NULL, -1);
    return;
  }
  Cit *c = &cits[multi_cits[goal_index]];
  multi_cits[goal_index] = 0;   /* answered */
  if (!(c->flags & CF_ALIVE) || !(c->flags & CF_REQUESTING_PATH)) return;
  int n = backtrace(at, from_outside, pairs, 128);
  cit_set_found_path(c, pairs, n);
}

static void refresh_graph_info(void) {
  /* entertainment that is open, teleporters, and the lowest gap of each column (for flying) */
  nteleporters = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *p = &perms[i];
    ent_bits[i] = -1;
    if (!(p->flags & PF_USED)) continue;
    if ((p->flags & PF_IS_ENTERTAINMENT) && ent_is_open(p)) ent_bits[i] = (int8_t)(1 << ent_type(p));
    if (is_a(p, B_TELEPORTER) && nteleporters < 64) teleporters_found[nteleporters++] = (uint16_t)i;
  }
  for (int w = 0; w < nworlds; w++)
    for (int x = 0; x < worlds[w].ncols; x++) {
      int h = col_stack_height(w, x), lowest = h;
      for (int y = 0; y < h; y++) {
        Perm *q = P(perm_at(w, x, y));
        if ((!q || node_type(q) == NT_LANDING) && y < lowest) lowest = y;
      }
      lowest_tunnel[worlds[w].col0 + x] = (uint8_t)(lowest > 255 ? 255 : lowest);
    }
}

static int ent_refresh_in;
void path_update(float timeMod) {
  ent_refresh_in -= (int)timeMod;
  if (ent_refresh_in <= 0) {
    refresh_graph_info();
    ent_refresh_in = 30;
  }
  /* answer requests, a few searches a tick (more on the faster N0120) */
  int budget = plat_slow() ? 6 : 14;
  int i = 0, done = 0;
  while (i < nreq && budget > 0) {
    Request r = req[i];
    Cit *c = &cits[r.cit];
    if (!(c->flags & CF_ALIVE) || !(c->flags & CF_REQUESTING_PATH)) { i++; continue; }
    if (r.kind == RQ_COMBINED && c->inPermanent) {
      /* everyone leaving the same building, in one search */
      uint16_t goals[32];
      int n = 0;
      for (int j = i; j < nreq && n < 32; j++) {
        Cit *d = &cits[req[j].cit];
        if (req[j].kind == RQ_COMBINED && (d->flags & CF_ALIVE) && (d->flags & CF_REQUESTING_PATH) &&
            d->inPermanent == c->inPermanent) {
          goals[n] = req[j].goal;
          multi_cits[n] = req[j].cit;
          req[j].kind = 0xFF;   /* taken */
          n++;
        }
      }
      cur_cit = c;
      search(c, 2, goals, n, 1000000.0f, give_found_multi);
      done += n;
    } else if (r.kind != 0xFF) {
      cur_cit = c;
      uint16_t g = r.goal;
      if (r.kind == RQ_ENTERTAINMENT) search(c, GOAL_ENTERTAINMENT, &g, 1, r.max_distance, give_found);
      else search(c, GOAL_PERM, &g, 1, 10000000.0f, give_found);
      done++;
    }
    budget--;
    i++;
  }
  /* drop the answered requests */
  int k = 0;
  for (int j = i; j < nreq; j++)
    if (req[j].kind != 0xFF) req[k++] = req[j];
  nreq = k;
  done_last_tick = done;
}

void path_distribute(void) {}

static void add_request(Cit *c, uint8_t kind, uint16_t goal, int max_distance, uint8_t on_fail) {
  c->flags |= CF_REQUESTING_PATH;
  c->onFail = on_fail;
  if (nreq >= MAX_REQ) {   /* full: the citizen asks again next tick */
    c->flags &= (uint16_t)~CF_REQUESTING_PATH;
    return;
  }
  req[nreq++] = (Request){cid(c), kind, on_fail, goal, (int16_t)(max_distance > 32767 ? 32767 : max_distance)};
}

void path_find(Cit *c, uint16_t goal, uint8_t on_fail) {
  c->requestingPathGoal = goal;
  add_request(c, RQ_PATH, goal, 10000000, on_fail);
}
void path_find_combined(Cit *c, uint16_t goal, uint8_t on_fail) {
  c->requestingPathGoal = goal;
  add_request(c, c->inPermanent ? RQ_COMBINED : RQ_PATH, goal, 10000000, on_fail);
}
void path_find_entertainment(Cit *c, int max_distance, int type_bit) {
  c->requestingPathGoal = NONE;
  add_request(c, RQ_ENTERTAINMENT, (uint16_t)(int16_t)type_bit, max_distance, THEN_NONE);
}
/* mayRequestPathNow: not too many requests waiting (or one joining a building's shared search) */
bool path_may_request_now(uint16_t in_perm) {
  if (in_perm)
    for (int i = 0; i < nreq; i++)
      if (req[i].kind == RQ_COMBINED && cits[req[i].cit].inPermanent == in_perm) return true;
  return nreq < done_last_tick + 100 && nreq < MAX_REQ - 8;
}
void path_reset(void) {
  nreq = 0;
  done_last_tick = 0;
  ent_refresh_in = 0;
}
void path_invalidate_all(void) {
  for (int i = 0; i < nreq; i++) cits[req[i].cit].flags &= (uint16_t)~CF_REQUESTING_PATH;
  nreq = 0;
  ent_refresh_in = 0;
}
