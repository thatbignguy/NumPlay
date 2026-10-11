/* The Final Earth 2: building and destroying (Builder.canBuildBuilding,
 * tryBuildingAt, World.build, City.onBuildBuilding, Building.tryDestroy).
 *
 * A building goes on top of a column's stack, into a hole in it, between two
 * buildings above a gap (a "bridge" position), inserted into a stack (those
 * above move up to the next hole), or in place of another (replace mode). */
#include "bld.h"
#include "build.h"

BuildingMode building_mode = BM_NORMAL;
float recycling_amount;   /* upgrades.vars.recyclingAmount */
BuilderLook builder_look = {0, false, -1};

static bool is_city_top(const Perm *p) { return is_building(p) && (PC(p)->base & B_CITY_TOP); }
static bool no_insert(const Perm *p) { return is_building(p) && (PC(p)->base & B_NO_INSERT); }

/* World.canBuildOnSurface, canBuildOnNotSurface */
static bool can_build_on_surface(const World *w) { return !(w->flags & WF_PROTECTED_KEY) && w->h != 0 && !(w->flags & WF_UNBUILDABLE_ALIENS); }
static bool can_build_on_not_surface(const World *w, bool is_machine) { return !(w->flags & WF_PROTECTED_KEY) || is_machine; }

/* the permanent whose 20x20 cell holds the city point */
uint16_t permanent_at_pos(int x, int y) {
  for (int w = 0; w < nworlds; w++) {
    World *wd = &worlds[w];
    if (x < wd->x || x >= wd->x + wd->w) continue;
    int col = (x - wd->x) / 20;
    if (y >= wd->y) continue;
    int row = (wd->y - 1 - y) / 20;
    uint16_t p = perm_at(w, col, row);
    if (p) return p;
  }
  return NONE;
}

static bool another_world_body_at(int w, int x, int y) {
  for (int o = 0; o < nworlds; o++) {
    World *ow = &worlds[o];
    if (o == w || ow->h == 0) continue;
    if (x + 20 <= ow->x || x >= ow->x + ow->w || y + 20 <= ow->y || y >= ow->y + ow->h) continue;
    if (world_mask_at(o, (x - ow->x) / 20, (y - ow->y) / 20)) return true;
  }
  return false;
}

/* Builder.canRemoveFromLRPerspective (without the paid version's advanced building) */
static bool can_remove_lr(const Perm *b) {
  const Perm *l = P(b->left), *r = P(b->right);
  if (!l || l->row == 0 || l->bottom) return !r || r->row == 0 || r->bottom;
  return false;
}
/* Builder.canRemoveLeavingHoleFromStructuralPerspective */
static bool can_remove_leaving_hole(const Perm *b) {
  const Perm *l = P(b->left), *r = P(b->right), *t = P(b->top);
  if ((!l || l->row == 0 || l->bottom) && (!r || r->row == 0 || r->bottom) && ((!l && !r && !t) || (l && r) || !t)) {
    if (t) return t->left && t->right;
    return true;
  }
  return false;
}

/* Builder.canBuildBuilding */
BuildPossibility builder_can_build(int w, int x, int y, int cls) {
  if (w < 0 || w >= nworlds || x < 0 || x >= worlds[w].ncols || y < 0) return BP_IMPOSSIBLE;
  World *wd = &worlds[w];
  const PermClass *pc = &pcls[cls];
  bool rooftop = pc->base & B_ROOFTOP;
  if (y == 0 && !can_build_on_surface(wd)) return BP_WORLD_NOT_ALLOWED;
  if (!can_build_on_not_surface(wd, !strcmp(pc->name, "TheMachine") && !building_mode_can_replace())) return BP_WORLD_NOT_ALLOWED;
  int h = col_stack_height(w, x);
  if (pc->base & B_BUILD_ON_GROUND) {
    if (y != 0) return BP_IMPOSSIBLE;
    if (h > 0 && perm_at(w, x, 0)) return BP_IMPOSSIBLE;
  }
  int y_top = wd->y - 20 * (h + 1);
  int bx = wd->x + 20 * x, by = wd->y - 20 * (y + 1);
  bool none_yet = h == 0;
  Perm *top = h ? P(perm_at(w, x, h - 1)) : NULL;
  Perm *bottom = h ? P(perm_at(w, x, 0)) : NULL;
  bool top_is_building = !none_yet && is_building(top);
  bool bottom_not_resource = none_yet || !bottom || is_building(bottom);
  bool top_rooftop = top_is_building && (top->flags & PF_NO_BUILD_ON_TOP);
  int y_top_ex = top_rooftop ? y_top + 20 : y_top;
  bool allow_bridge = false;
  bool can_insert = building_mode == BM_INSERT;
  bool can_replace = building_mode_can_replace();
  Perm *here = y < h ? P(perm_at(w, x, y)) : NULL;
  Perm *below = y > 0 && y - 1 < h ? P(perm_at(w, x, y - 1)) : NULL;
  if (!can_replace && is_building(here) && (here->flags & PF_NO_BUILD_ON_TOP)) can_insert = true;
  bool building_here = is_building(here);
  bool rooftop_here = false, has_lr = false, same = false;
  if (building_here) {
    /* DragReplace: not over one of its own class that already looks the same */
    same = building_mode == BM_DRAG_REPLACE && here->cls == cls &&
           bld_same_customizations(here, builder_look.decor, builder_look.mirrored, builder_look.color);
    rooftop_here = here->flags & PF_NO_BUILD_ON_TOP;
    if (no_insert(here)) return BP_IMPOSSIBLE;
  }
  if (story_is("cityofthekey") && building_here && can_replace && is_a(here, B_LANDINGSITE) &&
      (perm_city_x(here) == 100 || perm_city_x(here) == -100))
    return BP_IMPOSSIBLE;
  if (!here && y > 0 && !rooftop) {
    Perm *l = P(permanent_at_pos(bx - 20, by)), *r = P(permanent_at_pos(bx + 20, by));
    if (is_building(l) && is_building(r)) {
      has_lr = true;
      if (!(l->flags & PF_NO_BUILD_ON_TOP) && !(r->flags & PF_NO_BUILD_ON_TOP)) allow_bridge = true;
    }
    if (allow_bridge) {
      /* not over another world that is nearer below */
      for (int o = 0; o < nworlds; o++) {
        World *ow = &worlds[o];
        if (o != w && bx >= ow->x && bx < ow->x + ow->w && ow->y - by > 0 && ow->y - by < wd->y - by) { allow_bridge = false; break; }
      }
    }
  }
  bool null_between = false;
  if (can_insert) {
    for (int i = y; i < h - 1; i++) {
      Perm *q = P(perm_at(w, x, i));
      if (!q) { null_between = true; break; }
      if (is_building(q) && (q->flags & PF_NO_BUILD_ON_TOP) && perm_at(w, x, i + 1)) return BP_IMPOSSIBLE;
    }
  }
  if (pc->base & B_CITY_TOP) {
    if (y != h) return BP_IMPOSSIBLE;
    for (int o = 0; o < nworlds; o++) {
      World *ow = &worlds[o];
      if (ow->y < by && ow->x <= bx && ow->x + ow->w > bx) return BP_IMPOSSIBLE;
    }
  } else if (y != 0 && y >= h && h && is_city_top(top)) {
    return BP_IMPOSSIBLE;
  }
  bool ok =
      (can_replace && building_here && (!rooftop || rooftop_here) && !same) ||
      ((none_yet || top_is_building || allow_bridge) && (y != 0 || bottom_not_resource) &&
       (((can_insert || (y < h && !here)) && (by >= y_top_ex && by < wd->y) &&
         (y == 0 || has_lr || (y > 0 && building_here) || (y > 0 && is_building(below) && !(below->flags & PF_NO_BUILD_ON_TOP)))) ||
        (!can_insert && (by >= y_top_ex && by < y_top_ex + 20)) || allow_bridge) &&
       (y >= h || (!here && allow_bridge) || y == 0 || below || can_insert) &&
       (!rooftop || y == h || !here ||
        (y == h - 1 && can_replace && building_here && !same &&
         (y == 0 || (here->bottom && !(P(here->bottom)->flags & PF_NO_BUILD_ON_TOP))) &&
         can_remove_lr(here))));
  if (!ok) return BP_IMPOSSIBLE;
  int check_y = can_insert ? (by < y_top ? by : y_top) : by;
  if (null_between) return BP_OK;
  if (can_insert && y < h && !here) return BP_OK;
  if (y < h && !here) return BP_OK;
  return another_world_body_at(w, bx, check_y) ? BP_IN_WORLD : BP_OK;
}

/* the cost now: the class's, without its research price once researched */
void builder_cost(int cls, Materials *out) {
  for (int i = 0; i < M_COUNT; i++) out->m[i] = pcls[cls].cost[i];   /* (floats into doubles) */
  building_cost_scale(cls, out);
  if (unlock_state(cls) == UNLOCK_RESEARCHED) out->m[M_KNOWLEDGE] = 0;
}
bool can_afford(const Materials *m) {
  for (int i = 0; i < M_COUNT; i++)
    if (city.materials.m[i] < m->m[i]) return false;
  return true;
}
/* Builder.buildingPrerequirementsValid: affordable, and not a second unique (or more than its limit) */
bool builder_prerequisites(int cls) {
  const PermClass *pc = &pcls[cls];
  int count = 0;
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == cls) count++;
  bool ok = !(pc->base & B_UNIQUE) || count == 0 || unique_not_unique(cls);
  if (ok && pc->limitedTo > 0) ok = count < pc->limitedTo;
  if (ok && pc->limitedPerCitizens > 0) ok = count < (ncits + pc->limitedPerCitizens - 1) / pc->limitedPerCitizens;
  Materials cost;
  builder_cost(cls, &cost);
  return ok && can_afford(&cost);
}

/* Building.giveRecycleReward */
void give_recycle_reward(const Perm *p, bool full) {
  Materials m = {{0}};
  if (full)
    for (int i = 0; i < M_COUNT; i++) m.m[i] = PC(p)->cost[i];   /* (floats into doubles) */
  building_upgrades_cost_sum(p, &m);
  m.m[M_KNOWLEDGE] = 0;
  for (int i = 0; i < M_COUNT; i++) city.materials.m[i] += m.m[i] * recycling_amount;
}

/* tryBuildingAt and World.build: returns the new building */
uint16_t builder_build(int w, int x, int y, int cls) {
  Materials cost;
  builder_cost(cls, &cost);
  int h = col_stack_height(w, x);
  Perm *here = y < h ? P(perm_at(w, x, y)) : NULL;
  bool replace = building_mode_can_replace() && here;
  bool insert = !building_mode_can_replace() && here;
  bool pay = true;
  if (replace) {
    pay = here->cls != cls;
    if (pay && is_building(here)) give_recycle_reward(here, pay);
    perm_destroy(pid(here), true);
  } else if (insert) {
    /* the run above moves up a row, into the first hole */
    int top = y;
    while (top < h && perm_at(w, x, top)) top++;
    for (int i = top - 1; i >= y; i--) perm_move(perm_at(w, x, i), i + 1);
  }
  if (y == 0) col_deco[worlds[w].col0 + x] = 0;   /* removeDecoration */
  uint16_t id = perm_new(cls, w, x, y);
  if (!id) return NONE;
  bld_on_build(P(id));
  if (pay)
    for (int i = 0; i < M_COUNT; i++) city.materials.m[i] -= cost.m[i];
  /* City.onBuildBuilding */
  city_update_connections();
  sim_update_pathfinder(insert || replace, id);
  unlocks_research(cls);
  bld_post_create(P(id));
  unlocks_check_build_related();
  return id;
}

/* Building.tryDestroy: false (and why) when the structure would not hold */
DestroyResult building_try_destroy(uint16_t id) {
  Perm *p = P(id);
  if (worlds[p->world].flags & WF_PROTECTED_KEY) return DR_PROTECTED;
  const char *why;
  int refused = bld_destroy_check(p, &why);   /* (the class's own tryDestroy) */
  if (refused != DR_OK) return (DestroyResult)refused;
  bool failed = false;
  if (!(p->flags & PF_ROOFTOP)) {
    Perm *top = p;
    while (top->top && !(P(top->top)->flags & PF_ROOFTOP)) top = P(top->top);
    failed = building_mode == BM_DESTROY_LEAVING_HOLE ? !can_remove_leaving_hole(p) : !can_remove_lr(top);
    if (p->top && (P(p->top)->flags & PF_ROOFTOP) && !p->bottom && p->row != 0) failed = true;
  }
  if (failed) return building_mode == BM_DESTROY_LEAVING_HOLE ? DR_INVALID_STRUCTURE_HOLE : DR_INVALID_STRUCTURE;
  give_recycle_reward(p, true);
  bool keep_hole = ((p->flags & PF_ROOFTOP) || building_mode == BM_DESTROY_LEAVING_HOLE) && p->row != col_stack_height(p->world, p->col) - 1;
  perm_destroy_keep_hole(id, keep_hole);
  return DR_OK;
}
