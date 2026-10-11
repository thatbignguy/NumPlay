/* The Final Earth 2: running a scenario or free play map (progress.Story,
 * Story.makeWorlds, City.createBottomWorld, progress.scriptedStoryParts.
 * SpawnCitizensRegularly, progress.worldGenerators.RandomMiniWorlds and
 * inviting citizens). */
#include "bld.h"
#include "build.h"
#include "classes.h"
#include "story.h"
#include "unlocks.h"
#include "upgrades.h"
#include "bg.h"

StoryState story;
void story_set_name(const char *n);

/* ---------------------------------------------------------------- the worlds */
/* the world's build of a stack entry (World.build, then postCreate) */
static void build_on_top(int w, int col, int cls, int decor, bool mirrored) {
  int y = col_stack_height(w, col);
  if (y == 0) col_deco[worlds[w].col0 + col] = 0;
  uint16_t id = perm_new(cls, w, col, y);
  if (!id) return;
  bld_on_build(P(id));
  if (decor >= 0 || mirrored) bld_customize(P(id), decor, mirrored);
  bld_post_create(P(id));
}

static void make_world(const StoryWorld *sw) {
  int w = world_add(sw->x, sw->y, sw->w, sw->h, sw->seed, sw->appearance);
  if (w < 0) return;
  worlds[w].flags |= sw->flags;
  if (sw->decoration) {
    int kind = 0;
    for (int k = 1; k < DECO_KINDS; k++)
      if (bg_deco_sprite(k) == sw->decoration) kind = k;
    for (int x = 0; x < worlds[w].ncols; x++) bg_set_decoration(col_deco + worlds[w].col0, x, kind, -1, false);
  }
  for (int i = 0; i < sw->nres; i++)
    if (sw->res[i].col < worlds[w].ncols) {
      col_deco[worlds[w].col0 + sw->res[i].col] = 0;
      perm_new(sw->res[i].cls, w, sw->res[i].col, 0);
    }
  for (int i = 0; i < sw->nstacks; i++) {
    const StoryStack *st = &sw->stacks[i];
    for (int k = 0; k < st->n; k++) {
      uint16_t it = st->items[k];
      if ((it & 0xFF) == STACK_NULL) {
        col_height[worlds[w].col0 + st->col]++;   /* a hole */
        continue;
      }
      build_on_top(w, st->col, it & 0xFF, (it >> 9) ? (int)(it >> 9) - 1 : -1, (it & STACK_MIRROR) != 0);
    }
  }
  for (int i = 0; i < sw->ndecos; i++) {
    int kind = 0;
    for (int k = 1; k < DECO_KINDS; k++)
      if (bg_deco_sprite(k) == sw->decos[i].sprite) kind = k;
    bg_set_decoration(col_deco + worlds[w].col0, sw->decos[i].col, kind, -1, perm_at(w, sw->decos[i].col, 0) != NONE);
  }
  for (int i = 0; i < sw->ncits; i++) {
    const StoryCitizens *sc = &sw->cits[i];
    for (int k = 0; k < sc->amount; k++) {
      float age = sc->ageMin == sc->ageMax ? sc->ageMin : rnd_range(sc->ageMin, sc->ageMax);
      int x = sc->minX < 0 ? -1 : sc->minX == sc->maxX ? sc->minX : rnd_between(sc->minX, sc->maxX);
      cit_create(w, age, NONE, x);
    }
  }
}

/* progress.worldGenerators.RandomMiniWorlds; the calculator holds MAX_WORLDS worlds and MAX_COLS
 * columns, so a map asking for more (Random Mini Worlds: 400) gets as many as fit, and its
 * permanents in proportion */
static bool rect_hits(int x, int y, int w, int h) {
  for (int i = 0; i < nworlds; i++) {
    World *o = &worlds[i];
    if (x < o->x + o->w && x + w > o->x && y < o->y + o->h && y + h > o->y) return true;
  }
  return false;
}
static int empty_cols(int w) {
  int n = 0;
  for (int x = 0; x < worlds[w].ncols; x++)
    if (col_stack_height(w, x) == 0) n++;
  return n;
}
static void generate(const WorldGenerator *g) {
  int first = nworlds;
  int want = rnd_between(g->worldsMin, g->worldsMax);
  int room = MAX_WORLDS - 2 - nworlds;   /* (the bottom world comes after) */
  float scale = 1;
  if (want > room) {
    scale = (float)room / want;
    want = room;
  }
  for (int i = 0; i < want; i++) {
    for (int tries = 0; tries < 10000; tries++) {
      int x = rnd_between(g->minX / 20, g->maxX / 20 + 1) * 20;
      int y = rnd_between(g->minY / 20, g->maxY / 20 + 1) * 20;
      int w = rnd_between(g->minWidth / 20, g->maxWidth / 20 + 1) * 20;
      float hf = ((float)(w - g->minWidth) / (g->maxWidth - g->minWidth) * (g->maxHeight - g->minHeight) + g->minHeight +
                  rnd_int(g->heightVariation)) / 20.0f;
      int h = (int)roundf(hf);
      h = (h < 1 ? 1 : h > 100 ? 100 : h) * 20;
      if (rect_hits(x - 20, y - 40, w + 41, h + 61)) continue;
      world_add(x, y, w, h, (uint32_t)rnd_int(1000000), 0);
      break;
    }
  }
  int last = nworlds;
  /* the permanents: buildings first, then world resources */
  uint8_t list[512];
  int n = 0;
  for (int b = 1; b >= 0; b--)
    for (int i = 0; i < g->nitems; i++) {
      if (g->items[i].building != (b == 1)) continue;
      int k = (int)(rnd_between(g->items[i].min, g->items[i].max) * scale + 0.5f);
      for (int j = 0; j < k && n < 512; j++) list[n++] = g->items[i].cls;
    }
  uint8_t build_on[MAX_WORLDS];
  int nb = 0;
  for (int w = first; w < last; w++) build_on[nb++] = (uint8_t)w;
  bool first_resource = true;
  for (int i = 0; i < n; i++) {
    bool building = pcls[list[i]].kind == KIND_BUILDING;
    if (!building && first_resource) {
      first_resource = false;
      if (g->rememberSpread) {
        nb = 0;
        for (int w = first; w < last; w++) build_on[nb++] = (uint8_t)w;
      }
    }
    bool spread = building ? g->spreadBuildings : g->spreadResources;
    for (int attempt = 0; attempt < 2; attempt++) {
      /* candidates: every empty column of the worlds allowed */
      int count = 0;
      for (int w = first; w < last; w++) {
        bool ok = !spread;
        for (int k = 0; k < nb && !ok; k++) ok = build_on[k] == w;
        if (!ok) continue;
        if (!building && g->avoidOverfull && (worlds[w].ncols <= 1 || empty_cols(w) <= 1)) continue;
        count += empty_cols(w);
      }
      if (count == 0) {
        nb = 0;
        for (int w = first; w < last; w++) build_on[nb++] = (uint8_t)w;
        continue;
      }
      int pick = rnd_int(count);
      for (int w = first; w < last && pick >= 0; w++) {
        bool ok = !spread;
        for (int k = 0; k < nb && !ok; k++) ok = build_on[k] == w;
        if (!ok) continue;
        if (!building && g->avoidOverfull && (worlds[w].ncols <= 1 || empty_cols(w) <= 1)) continue;
        for (int x = 0; x < worlds[w].ncols; x++) {
          if (col_stack_height(w, x) != 0) continue;
          if (pick-- == 0) {
            for (int k = 0; k < nb; k++)
              if (build_on[k] == w) { build_on[k] = build_on[--nb]; break; }
            if (building) build_on_top(w, x, list[i], -1, false);
            else {
              col_deco[worlds[w].col0 + x] = 0;
              perm_new(list[i], w, x, 0);
            }
            break;
          }
        }
      }
      break;
    }
  }
}

/* City.createBottomWorld: an invisible world under the whole city */
static void create_bottom_world(void) {
  int minx = 1 << 30, maxx = -(1 << 30), maxy = -(1 << 30);
  for (int i = 0; i < nworlds; i++) {
    World *w = &worlds[i];
    if (w->x < minx) minx = w->x;
    if (w->x + w->w > maxx) maxx = w->x + w->w;
    if (w->y + w->h > maxy) maxy = w->y + w->h;
  }
  minx = (minx - 20) / 20 * 20;
  world_add(minx, ((maxy + 40) + 19) / 20 * 20, maxx - minx + 20, 0, 0, 0);
}

/* ---------------------------------------------------------------- goals */
static int count_class(int cls, int world, int not_world) {
  int n = 0;
  for (int i = 1; i < MAX_PERMS; i++) {
    Perm *p = &perms[i];
    if (!(p->flags & PF_USED)) continue;
    if (!class_is(p->cls, cls)) continue;
    if (world >= 0 && p->world != world) continue;
    if (not_world >= 0 && p->world == not_world) continue;
    n++;
  }
  return n;
}

static bool subgoal_complete(const SubGoal *s) {
  switch (s->type) {
  case SG_BUILDPERMANENTS: return count_class(s->cls, s->world, s->notWorld) >= s->amount;
  case SG_GATHERMATERIALS:
    for (int m = 0; m < M_COUNT; m++)
      if (s->mats[m] > 0 && city.materials.m[m] < s->mats[m]) return false;
    return true;
  case SG_PRODUCTIONGOAL:
    for (int m = 0; m < M_COUNT; m++) {
      if (s->mats[m] <= 0) continue;
      bool ok = false;
      for (int d = 0; d < STATS_DAYS; d++)
        if (stats.materialProduction[m][d] >= s->mats[m]) ok = true;
      if (!ok) return false;
    }
    return true;
  case SG_PERFORMBUILDINGUPGRADES: return upgrades_buildings_with(s->name) >= s->amount;
  case SG_PERFORMCITYUPGRADE: return upgrades_city_bought(s->name);
  case SG_MINIMUMHAPPINESS: return happiness.happiness >= s->value;
  case SG_MINIMUMCITIZENS: return ncits >= s->amount;
  case SG_HOUSINGCAPACITY: return stats.houseCapacity >= s->amount;
  case SG_WAIT: return story.goalTimer > s->value;
  case SG_INVITEPEOPLE: return story.justInvitedPeople;
  case SG_EXPLOREALIENRUINS: return bld_alien_ruins_explored() >= 99.99f;
  case SG_EXPLOREDWITHRESEARCHSPACECRAFT: return bld_research_bot_time() >= 540;
  case SG_COMPLETEHACKERMISSIONS: return bld_hacker_mission() >= s->amount;
  case SG_WAITFORDECRYPTION: return bld_decryptor_time() >= 1440;
  case SG_GLORYGOAL: return glory_of_the_key_happiness() >= 99.99f;
  }
  return false;
}
bool story_subgoal_complete(int i) {
  const StoryGoal *g = story_goal();
  return g && i < g->nsub && subgoal_complete(&g->sub[i]);
}

const StoryGoal *story_goal(void) {
  return story.goal >= 0 ? &stories[story.index].goals[story.goal] : NULL;
}

static void handle_flags(const StoryGoal *g) {
  if (g->flagsSet & GF_DISABLEDYING) city.disableDying = (g->flagsValue & GF_DISABLEDYING) != 0;
  if (g->flagsSet & GF_DISABLEDESTROY) story.disableDestroy = (g->flagsValue & GF_DISABLEDESTROY) != 0;
  if (g->flagsSet & GF_HIDDENBOOST) story.hiddenBoost = (g->flagsValue & GF_HIDDENBOOST) != 0;
  if ((g->flagsSet & GF_SETBUILDABLEALIENS) && (g->flagsValue & GF_SETBUILDABLEALIENS))
    for (int w = 0; w < nworlds; w++) worlds[w].flags &= (uint8_t)~WF_UNBUILDABLE_ALIENS;
}

static void start_goal(int gi) {
  const Story *st = &stories[story.index];
  const StoryGoal *g = &st->goals[gi];
  story.goal = (int8_t)gi;
  story.goalTimer = 0;
  handle_flags(g);
  if (!strcmp(g->name, "Win")) meta_win_scenario(story.index, (float)city.timeSinceStart);
  if (g->text || g->title) ui_show_goal_text(gi);
  for (int i = 0; i < g->nunlocks; i++) {
    const char *u = g->unlocks[i];
    if (!strcmp(u, "decorations")) unlocks.unlockedDecorationTab = true;
    else if (!strcmp(u, "buildingModes")) unlocks.unlockedBuildingModeButton = true;
    else if (!strcmp(u, "managementOptions")) unlocks.unlockedManagementOptions = true;
    else unlocks_unlock(u);
  }
  for (int i = 0; i < g->nresearches; i++) {
    int c = pcls_find(g->researches[i]);
    if (c >= 0) unlocks_research_uid(c);
  }
  if (g->nevents && story.nplannings < STORY_MAX_PLANNINGS) {
    StoryPlanning *pl = &story.plannings[story.nplannings++];
    pl->goal = (uint8_t)gi;
    pl->next = 0;
    pl->timeStart = (float)(city.timeSinceStart / 60);
  }
  if (g->tutorial) ui_show_tutorial(g->tutorial);
}

/* SpawnCitizensRegularly */
static void regularly_start(int goal, int event) {
  if (story.nregular >= STORY_MAX_REGULAR) return;
  Regularly *r = &story.regular[story.nregular++];
  const StoryEvent *e = &stories[story.index].goals[goal].events[event];
  r->goal = (uint8_t)goal;
  r->event = (uint8_t)event;
  r->left = e->amount;
  r->nextSpawn = 0;
}
static bool regularly_update(Regularly *r) {
  const StoryEvent *e = &stories[story.index].goals[r->goal].events[r->event];
  if (r->left > 0 && city.timeSinceStart >= r->nextSpawn) {
    int n = rnd_between(e->groupMin, e->groupMax + 1);
    if (n > r->left) n = r->left;
    r->left -= n;
    saucer_spawn(e->world, n, e->ageMin, e->ageMax, -1);
    float m = 1;
    if (stats.houseCapacity > stats.peopleWithHome + e->groupMin && stats.jobs > stats.laborForce + e->groupMin && r->left > 5)
      m = city.materials.m[M_FOOD] > 10 ? 0.6f : 0.7f;
    r->nextSpawn = city.timeSinceStart + 60.0 * m * rnd_range(e->betweenMin, e->betweenMax);
  }
  if (r->left <= 0 && spawners_pending() == 0) {
    if (e->afterDone) ui_show_message(e->afterDone);
    return false;
  }
  return true;
}

void story_update(float timeMod) {
  const Story *st = &stories[story.index];
  if (story.goal >= 0 && !ui_story_window_open()) {
    const StoryGoal *g = &st->goals[story.goal];
    bool done = true;
    for (int i = 0; i < g->nsub; i++)
      if (!subgoal_complete(&g->sub[i])) done = false;
    if (done) {
      if (g->next >= 0) start_goal(g->next);
      else story.goal = -1;
    }
  }
  /* plannings: each event fires its time (game hours) after its goal started */
  for (int i = story.nplannings - 1; i >= 0; i--) {
    StoryPlanning *pl = &story.plannings[i];
    const StoryGoal *g = &st->goals[pl->goal];
    if (pl->next < g->nevents) {
      const StoryEvent *e = &g->events[pl->next];
      if (pl->timeStart + e->time < city.timeSinceStart / 60) {
        if (e->type == EV_SAUCER) saucer_spawn(e->world, e->amount, e->ageMin, e->ageMax, -1);
        else regularly_start(pl->goal, pl->next);
        pl->next++;
      }
    }
    if (pl->next >= g->nevents) story.plannings[i] = story.plannings[--story.nplannings];
  }
  for (int i = story.nregular - 1; i >= 0; i--)
    if (!regularly_update(&story.regular[i])) story.regular[i] = story.regular[--story.nregular];
  story.goalTimer += timeMod;
}

/* ---------------------------------------------------------------- a new city */
void story_new_city(int index, const NewCityOptions *opt) {
  game_reset();
  memset(&story, 0, sizeof story);
  story.index = (uint8_t)index;
  story.goal = -1;
  const Story *st = &stories[index];
  story_set_name(st->link);
  unlocks_init();
  city.speedUpStartNights = (st->flags & ST_SPEED_UP_NIGHTS) != 0;
  for (int i = 0; i < st->nworlds; i++) make_world(&st->worlds[i]);
  for (int i = 0; i < st->ngens; i++) generate(&st->gens[i]);
  create_bottom_world();
  /* Story.start */
  for (int m = 0; m < M_COUNT; m++) city.materials.m[m] += st->initialMaterials[m];
  for (int i = 0; i < st->nlocks; i++) unlocks_lock(st->locks[i]);
  if (st->flags & ST_LOCK_ALL) {
    unlocks.unlockedDecorationTab = unlocks.unlockedBuildingModeButton = unlocks.unlockedManagementOptions = false;
    for (int c = 0; c < npcls; c++)
      if (pcls[c].kind == KIND_BUILDING) unlocks_lock_uid(c);
  }
  /* free play options: extra resources, and more or fewer citizens to start with */
  if (opt && (st->flags & ST_FREEPLAY)) {
    int have = ncits;
    if (opt->extraResources) {
      static const float extra[M_COUNT] = {[M_WOOD] = 1000, [M_STONE] = 1000, [M_FOOD] = 1000, [M_MACHINEPARTS] = 100, [M_KNOWLEDGE] = 500};
      memset(&city.materials, 0, sizeof city.materials);
      for (int m = 0; m < M_COUNT; m++) city.materials.m[m] = extra[m];
      city.materials.m[M_FOOD] += 3.0 * (have > 3 ? have - 3 : 0);
    }
    if (opt->citizens > 0 && opt->citizens != have) {
      int w = 0;
      for (int i = 1; i < MAX_CITS; i++)
        if (cits[i].flags & CF_ALIVE) { w = cits[i].onWorld; break; }
      while (ncits < opt->citizens) {
        cit_create(w, rnd_range(18, 40), NONE, -1);
        city.materials.m[M_FOOD] += 3;
      }
      for (int i = MAX_CITS - 1; i >= 1 && ncits > opt->citizens; i--)
        if (cits[i].flags & CF_ALIVE) {
          cit_try_remove(&cits[i], true);
          city.materials.m[M_FOOD] -= 3;
        }
    }
  }
  start_goal(st->startGoal);
  if (story_is("cityofthekey")) happiness.gloryOfTheKey = true;
  city_update_connections();
  sim_update_pathfinder(true, NONE);
  unlocks_stop_all_notify();
}

/* ---------------------------------------------------------------- inviting citizens (InvitePeopleWindow) */
static int invite_population(void) { return ncits + spawners_pending(); }
void invite_price(float cost[M_COUNT]) {
  memset(cost, 0, sizeof(float) * M_COUNT);
  int n = invite_population();
  if (n <= 500) {
    int k = n / 10 - 5;
    cost[M_FOOD] = 10.0f * (k > 1 ? k : 1);
  } else {
    cost[M_FOOD] = 10.0f * (n / 10 + 1);
    cost[M_REFINEDMETAL] = 10.0f * (n / 50 - 8);
    if (n > 5000) cost[M_COMPUTERCHIPS] = 50;
  }
}
/* why one can't invite now (0 if one can): 1 the limit, 2 not enough homes, 3 not enough materials */
int invite_problem(void) {
  int n = invite_population();
  int limit = (stories[story.index].flags & ST_FREEPLAY) ? babies.softPopLimit : 1900;
  if (n + 10 >= limit) return 1;
  if (stats.houseCapacity < n + 10) return 2;
  float cost[M_COUNT];
  invite_price(cost);
  return mat_can_afford(cost) ? 0 : 3;
}
bool invite_citizens(void) {
  if (invite_problem()) return false;
  float cost[M_COUNT];
  invite_price(cost);
  mat_pay(cost);
  story.justInvitedPeople = true;
  /* to the top-most world with citizens (in cityofthekey the top-most of all) */
  int best = -1;
  bool key = story_is("cityofthekey");
  for (int w = 0; w < nworlds; w++) {
    if (worlds[w].h == 0) continue;
    bool has = key;
    for (int i = 1; i < MAX_CITS && !has; i++)
      if ((cits[i].flags & CF_ALIVE) && cits[i].onWorld == w) has = true;
    if (!has) continue;
    if (best < 0 || worlds[w].y < worlds[best].y || (worlds[w].y == worlds[best].y && worlds[w].x < worlds[best].x)) best = w;
  }
  saucer_spawn(best < 0 ? 0 : best, 10, 18, 22, -1);
  return true;
}
