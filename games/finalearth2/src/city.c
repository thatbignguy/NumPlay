/* The Final Earth 2: worlds, their columns of permanents, and how they
 * connect (World, Permanent and simulation.CityConnections). */
#include "sim.h"
#include "scratch.h"
#include "bg.h"

World worlds[MAX_WORLDS];
int nworlds;
Perm perms[MAX_PERMS];
uint8_t col_height[MAX_COLS];
uint8_t col_deco[MAX_COLS];      /* each column's ground decoration (bg.h: kind << 4 | frame, 0 none) */
BgWorld bgworlds[MAX_WORLDS];   /* the worlds' looks (bg.c) */
static int ncols_used;
City city;

/* ---------------------------------------------------------------- random */
static uint32_t rng_state = 0x2545F491u;
uint32_t rnd(void) {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return rng_state;
}
void rnd_seed(uint32_t s) { rng_state = s ? s : 1; }
float rnd_float(void) { return (float)(rnd() >> 8) * (1.0f / 16777216.0f); }
float rnd_range(float a, float b) { return a + rnd_float() * (b - a); }
int rnd_int(int n) { return n > 0 ? (int)(rnd_float() * n) : 0; }
int rnd_between(int a, int b) { return a + rnd_int(b - a); }

/* ---------------------------------------------------------------- where permanents are
 * a hash of (world, column, row) to the permanent there */
#define HASH_SIZE 1024
static uint16_t hash_tab[HASH_SIZE];
static inline uint32_t key_of(int w, int col, int row) { return (uint32_t)w << 16 | (uint32_t)col << 8 | (uint32_t)row; }
static inline uint32_t hash_of(uint32_t k) { return (k * 2654435761u) >> 22; }

static void hash_put(uint16_t id) {
  Perm *p = &perms[id];
  uint32_t h = hash_of(key_of(p->world, p->col, p->row));
  while (hash_tab[h]) h = (h + 1) & (HASH_SIZE - 1);
  hash_tab[h] = id;
}
static void hash_del(uint16_t id) {
  Perm *p = &perms[id];
  uint32_t h = hash_of(key_of(p->world, p->col, p->row));
  while (hash_tab[h] != id) {
    if (!hash_tab[h]) return;
    h = (h + 1) & (HASH_SIZE - 1);
  }
  /* close the gap (linear probing) */
  hash_tab[h] = 0;
  uint32_t j = h;
  for (;;) {
    j = (j + 1) & (HASH_SIZE - 1);
    uint16_t q = hash_tab[j];
    if (!q) break;
    uint32_t k = hash_of(key_of(perms[q].world, perms[q].col, perms[q].row));
    if ((j > h && (k <= h || k > j)) || (j < h && (k <= h && k > j))) {
      hash_tab[h] = q;
      hash_tab[j] = 0;
      h = j;
    }
  }
}
uint16_t perm_at(int w, int col, int row) {
  if (w < 0 || w >= nworlds || col < 0 || col >= worlds[w].ncols || row < 0) return NONE;
  uint32_t k = key_of(w, col, row), h = hash_of(k);
  for (uint16_t id; (id = hash_tab[h]); h = (h + 1) & (HASH_SIZE - 1)) {
    Perm *p = &perms[id];
    if (p->world == w && p->col == col && p->row == row) return id;
  }
  return NONE;
}
int perm_city_x(const Perm *p) { return worlds[p->world].x + 20 * p->col; }
int perm_city_y(const Perm *p) { return worlds[p->world].y - 20 * (p->row + 1); }
int col_stack_height(int w, int col) { return col_height[worlds[w].col0 + col]; }

/* ---------------------------------------------------------------- worlds */
int world_add(int x, int y, int w, int h, uint32_t seed, uint8_t appearance) {
  int ncols = (w + 19) / 20;
  if (nworlds >= MAX_WORLDS || ncols_used + ncols > MAX_COLS) return -1;
  World *wd = &worlds[nworlds];
  memset(wd, 0, sizeof *wd);
  wd->x = (int16_t)x, wd->y = (int16_t)y, wd->w = (int16_t)w, wd->h = (int16_t)h;
  wd->seed = seed;
  wd->appearance = appearance;
  wd->col0 = (uint16_t)ncols_used;
  wd->ncols = (uint16_t)ncols;
  int maxid = -1;
  for (int i = 0; i < nworlds; i++)
    if (worlds[i].id > maxid) maxid = worlds[i].id;
  wd->id = (uint8_t)(maxid + 1);
  memset(col_height + ncols_used, 0, (size_t)ncols);
  memset(col_deco + ncols_used, 0, (size_t)ncols);
  BgWorld *bw = &bgworlds[nworlds];
  memset(bw, 0, sizeof *bw);
  bw->x = (int16_t)x, bw->y = (int16_t)y, bw->w = (int16_t)w, bw->h = (int16_t)h;
  bw->seed = (int32_t)seed;
  bw->look = appearance;
  if (h > 0) bg_world_decorate(bw, col_deco + ncols_used);
  ncols_used += ncols;
  return nworlds++;
}

/* ---------------------------------------------------------------- permanents */
static int nperms;
int perm_count(void) { return nperms; }

/* a new permanent of class cls at (w, col, row): row is the stack's height (on top) or a hole in it */
uint16_t perm_new(int cls, int w, int col, int row) {
  uint16_t id = 0;
  for (int i = 1; i < MAX_PERMS; i++)
    if (!(perms[i].flags & PF_USED)) { id = (uint16_t)i; break; }
  if (!id) return NONE;
  Perm *p = &perms[id];
  memset(p, 0, sizeof *p);
  p->cls = (uint16_t)cls;
  p->world = (uint8_t)w, p->col = (uint8_t)col, p->row = (uint8_t)row;
  const PermClass *pc = &pcls[cls];
  p->flags = PF_USED;
  if (pc->kind == KIND_BUILDING) {
    p->flags |= PF_BUILDING;
    if (pc->base & B_ROOFTOP) p->flags |= PF_ROOFTOP | PF_NO_BUILD_ON_TOP;
    if (pc->base & B_NO_BUILD_ON_TOP) p->flags |= PF_NO_BUILD_ON_TOP;
    if (pc->base & B_ENTERTAINMENT) p->flags |= PF_IS_ENTERTAINMENT;
    if (pc->base & B_MEDICAL) p->flags |= PF_IS_MEDICAL;
    sim_flags |= SIM_JOBS_CHANGED | SIM_HOUSES_CHANGED | SIM_SCHOOLS_CHANGED;
  }
  uint8_t *h = &col_height[worlds[w].col0 + col];
  if (row >= *h) *h = (uint8_t)(row + 1);
  hash_put(id);
  nperms++;
  if (pc->ops && pc->ops->init) pc->ops->init(p);
  return id;
}

/* Permanent.destroy: the permanent goes; those above it come down a row
 * (unless it is replaced, or a world resource under a stack); citizens in it
 * move to a neighbour or out onto the world */
static bool destroying_for_replacement;
static void perm_destroy_ex(uint16_t id, bool for_replacement, bool keep_hole) {
  Perm *p = &perms[id];
  const PermClass *pc = PC(p);
  int w = p->world, col = p->col, row = p->row;
  uint8_t *h = &col_height[worlds[w].col0 + col];
  bool disable_move_down = for_replacement || keep_hole || (pc->kind == KIND_RESOURCE && *h > 1);
  destroying_for_replacement = for_replacement;
  p->flags |= PF_DESTROYED;
  /* the class's own leaving (homes evict, work places fire) happens first, as Building.destroy calls up */
  if (pc->ops && pc->ops->destroy) pc->ops->destroy(p);
  cits_leave_destroyed(id);
  if (pc->base & B_WORK) cits_lose_job_at(id);
  if (pc->base & B_HOUSE) cits_evict_from(id);
  if (pc->base & B_SCHOOL) cits_leave_school(id);
  hash_del(id);
  p->flags = 0;
  nperms--;
  if (!disable_move_down) {
    /* everything above comes down until a hole (or a rooftop building, under which a hole is kept) */
    for (int y = row + 1; y < *h; y++) {
      uint16_t q = perm_at(w, col, y);
      if (!q) break;
      hash_del(q);
      perms[q].row--;
      hash_put(q);
      if ((perms[q].flags & PF_ROOFTOP) && y + 1 < *h) break;
    }
    /* drop holes left at the top */
    while (*h > 0 && !perm_at(w, col, *h - 1)) (*h)--;
  } else if (row == *h - 1) {
    while (*h > 0 && !perm_at(w, col, *h - 1)) (*h)--;
  }
  if (!for_replacement) {
    city_update_connections();
    sim_update_pathfinder(true, id);
  }
  destroying_for_replacement = false;
}
void perm_destroy(uint16_t id, bool for_replacement) { perm_destroy_ex(id, for_replacement, false); }
void perm_destroy_keep_hole(uint16_t id, bool keep_hole) { perm_destroy_ex(id, false, keep_hole); }

/* a saved permanent back in its place (save.c) */
bool perm_restore(uint16_t id, const Perm *p) {
  if (perms[id].flags & PF_USED) return false;
  if (p->world >= nworlds || p->col >= worlds[p->world].ncols) return false;
  perms[id] = *p;
  perms[id].left = perms[id].right = perms[id].top = perms[id].bottom = NONE;
  hash_put(id);
  nperms++;
  return true;
}

/* a permanent to another row of its column (a building inserted below it) */
void perm_move(uint16_t id, int row) {
  hash_del(id);
  perms[id].row = (uint8_t)row;
  hash_put(id);
  uint8_t *h = &col_height[worlds[perms[id].world].col0 + perms[id].col];
  if (row >= *h) *h = (uint8_t)(row + 1);
}

/* ---------------------------------------------------------------- connections
 * simulation.CityConnections.updateCityConnections: the buildings' neighbours,
 * which worlds citizens can reach from each other, and the groups (shards) of
 * permanents they can walk between */
uint8_t landing_group_of[MAX_PERMS];      /* landingSiteGroup + 1 */
int nlanding_groups;

static void set_neighbours(void) {
  for (int i = 1; i < MAX_PERMS; i++)
    if (perms[i].flags & PF_BUILDING) perms[i].left = perms[i].right = perms[i].top = perms[i].bottom = NONE;
  for (int w = 0; w < nworlds; w++) {
    World *wd = &worlds[w];
    for (int x = 0; x + 1 < wd->ncols; x++) {
      int n = col_height[wd->col0 + x], m = col_height[wd->col0 + x + 1];
      for (int y = 0; y < n && y < m; y++) {
        Perm *a = P(perm_at(w, x, y)), *b = P(perm_at(w, x + 1, y));
        if (is_building(a) && is_building(b) && !(a->flags & PF_NO_BUILD_ON_TOP) && !(b->flags & PF_NO_BUILD_ON_TOP))
          a->right = pid(b), b->left = pid(a);
      }
    }
    for (int x = 0; x < wd->ncols; x++)
      for (int y = 0; y + 1 < col_height[wd->col0 + x]; y++) {
        Perm *a = P(perm_at(w, x, y)), *b = P(perm_at(w, x, y + 1));
        if (is_building(a) && is_building(b) && !(a->flags & PF_ROOFTOP)) a->top = pid(b), b->bottom = pid(a);
      }
  }
}

int direct_world_connections;
static uint8_t reach[MAX_WORLDS];   /* each world's reachable-worlds set, as a set number (worlds sharing it reach each other) */

static void merge_reach(int a, int b) {
  uint8_t ra = reach[a], rb = reach[b];
  if (ra == rb) return;
  for (int i = 0; i < nworlds; i++)
    if (reach[i] == rb) reach[i] = ra;
}

/* worlds next to each other, aligned on the 20-pixel grid: the buildings of
 * one's edge column and the other's neighbouring column join at equal heights */
static void direct_connections(int w, int edge_col, int dir) {
  World *wd = &worlds[w];
  int edge_x = wd->x + 20 * edge_col;
  for (int o = 0; o < nworlds; o++) {
    World *ow = &worlds[o];
    if (!(ow->y > wd->y || (ow->y == wd->y && ow->x > wd->x))) continue;
    if (((ow->x % 20) + 20) % 20 != ((wd->x % 20) + 20) % 20 || ((ow->y % 20) + 20) % 20 != ((wd->y % 20) + 20) % 20) continue;
    int ox = (edge_x - ow->x) / 20 + dir;
    if (edge_x - ow->x < 0 && (edge_x - ow->x) % 20) ox = -((ow->x - edge_x) / 20) + dir;   /* truncation toward zero, as | 0 */
    if (ox < 0 || ox >= ow->ncols) continue;
    int between = (ow->y - wd->y) / 20;
    int oh = col_height[ow->col0 + ox], th = col_height[wd->col0 + edge_col];
    if (oh <= between) continue;
    bool any = false;
    int n = oh - between < th ? oh - between : th;
    for (int i = 0; i < n; i++) {
      Perm *a = P(perm_at(w, edge_col, i)), *b = P(perm_at(o, ox, i + between));
      if (is_building(a) && is_building(b) && !(a->flags & PF_NO_BUILD_ON_TOP) && !(b->flags & PF_NO_BUILD_ON_TOP)) {
        if (dir == 1) a->right = pid(b), b->left = pid(a);
        else a->left = pid(b), b->right = pid(a);
        any = true;
        direct_world_connections += 2;
      }
    }
    if (any) merge_reach(o, w);
  }
}

static void flood_fill_shards(void) {
  uint16_t *stack = scratch->u.flood.stack;
  uint8_t *seen = scratch->u.flood.seen;
  static uint8_t seen_world[MAX_WORLDS];
  memset(seen, 0, MAX_PERMS);
  memset(seen_world, 0, sizeof seen_world);
  for (int w = 0; w < nworlds; w++) worlds[w].surfaceShard = 0xFFFF;
  bool handled_tp = false;
  uint8_t *handled_ls = scratch->u.flood.handled_ls;
  memset(handled_ls, 0, MAX_PERMS);
  uint16_t shard = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    if (!(perms[i].flags & PF_USED) || seen[i]) continue;
    int sp = 0;
    stack[sp++] = (uint16_t)i;
    seen[i] = 1;
    while (sp) {
      uint16_t id = stack[--sp];
      Perm *pm = &perms[id];
      pm->shard = shard;
#define PUSH(q) do { uint16_t q_ = (q); if (q_ && !seen[q_]) { seen[q_] = 1; stack[sp++] = q_; } } while (0)
      if (pm->flags & PF_BUILDING) {
        PUSH(pm->left);
        PUSH(pm->top);
        PUSH(pm->bottom);
        PUSH(pm->right);
        if (!handled_tp && is_a(pm, B_TELEPORTER)) {
          for (int j = 1; j < MAX_PERMS; j++)
            if ((perms[j].flags & PF_USED) && is_a(&perms[j], B_TELEPORTER)) PUSH((uint16_t)j);
          handled_tp = true;
        }
        if (is_a(pm, B_LANDINGSITE) && landing_group_of[id]) {
          int g = landing_group_of[id] - 1;
          if (!handled_ls[g]) {
            for (int j = 1; j < MAX_PERMS; j++)
              if ((perms[j].flags & PF_USED) && landing_group_of[j] == g + 1) PUSH((uint16_t)j);
            handled_ls[g] = 1;
          }
        }
      }
      if (pm->row == 0 && !seen_world[pm->world]) {
        seen_world[pm->world] = 1;
        worlds[pm->world].surfaceShard = shard;
        World *wd = &worlds[pm->world];
        for (int x = 0; x < wd->ncols; x++) PUSH(perm_at(pm->world, x, 0));
      }
#undef PUSH
    }
    shard++;
  }
  for (int w = 0; w < nworlds; w++)
    if (worlds[w].surfaceShard == 0xFFFF) worlds[w].surfaceShard = shard++;
}

void city_update_connections(void) {
  set_neighbours();
  /* worlds with a teleporter reach each other */
  bool tp_world[MAX_WORLDS] = {0};
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && is_a(&perms[i], B_TELEPORTER)) tp_world[perms[i].world] = true;
  for (int w = 0; w < nworlds; w++) reach[w] = (uint8_t)w;
  int first_tp = -1;
  for (int w = 0; w < nworlds; w++)
    if (tp_world[w]) {
      if (first_tp < 0) first_tp = w;
      reach[w] = (uint8_t)first_tp;
    }
  /* landing sites that can fly to each other form groups, and join their worlds */
  memset(landing_group_of, 0, sizeof landing_group_of);
  nlanding_groups = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    if (!(perms[i].flags & PF_USED) || !is_a(&perms[i], B_LANDINGSITE)) continue;
    uint16_t other = flying_find_any_other_landing_site((uint16_t)i);
    if (other && landing_group_of[other]) landing_group_of[i] = landing_group_of[other];
    else landing_group_of[i] = (uint8_t)++nlanding_groups;
  }
  for (int g = 1; g <= nlanding_groups; g++) {
    int first = -1;
    for (int i = 1; i < MAX_PERMS; i++)
      if ((perms[i].flags & PF_USED) && landing_group_of[i] == g) {
        if (first < 0) first = perms[i].world;
        else merge_reach(first, perms[i].world);
      }
  }
  direct_world_connections = 0;
  for (int w = 0; w < nworlds; w++) {
    direct_connections(w, 0, -1);
    direct_connections(w, worlds[w].ncols - 1, 1);
  }
  /* world groups, in the order of the worlds */
  uint8_t group_of_set[MAX_WORLDS];
  memset(group_of_set, 0xFF, sizeof group_of_set);
  uint8_t groups = 0;
  for (int w = 0; w < nworlds; w++) {
    if (group_of_set[reach[w]] == 0xFF) group_of_set[reach[w]] = groups++;
    worlds[w].group = group_of_set[reach[w]];
  }
  for (int w = 0; w < nworlds; w++) {
    bool tp = false;
    for (int v = 0; v < nworlds; v++)
      if (worlds[v].group == worlds[w].group && tp_world[v]) tp = true;
    if (tp) worlds[w].flags |= WF_HAS_TELEPORTER_ON_GROUP;
    else worlds[w].flags &= (uint8_t)~WF_HAS_TELEPORTER_ON_GROUP;
  }
  flood_fill_shards();
}

void city_reset(void) {
  memset(worlds, 0, sizeof worlds);
  memset(perms, 0, sizeof perms);
  memset(hash_tab, 0, sizeof hash_tab);
  memset(col_height, 0, sizeof col_height);
  memset(col_deco, 0, sizeof col_deco);
  memset(bgworlds, 0, sizeof bgworlds);
  bg_forget_shapes();
  nworlds = ncols_used = nperms = 0;
  memset(&city, 0, sizeof city);
  city.minutesPerTick = 0.5f;
  city.timeSinceStart = 120;
  city.simulationSpeed = 1;
}

/* the island's collision mask (Builder: no building in another world's body) */
bool world_mask_at(int w, int cx, int cy) { return bg_world_mask(&bgworlds[w], cx, cy); }
