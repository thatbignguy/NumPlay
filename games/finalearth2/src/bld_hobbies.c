/* The Final Earth 2: citizens' hobbies (simulation.CitizenSpecialAction and
 * citizenSpecialActions.*): a few citizens climb trees and ruins at night,
 * walk to the world's edge, watch the stars, protest or dance on a ball.
 * Each hobby is given to one citizen at a time (CitySimulation's
 * possibleCitizenHobbies), so its state is kept by hobby, not by citizen. */
#include "bldi.h"
#include "save.h"
#include "view.h"

enum {
  H_NONE, H_CLIMB_TREE, H_MOVE_TO_EDGE, H_WATCH_STARS, H_PROTEST, H_PROTEST2, H_PROTEST3, H_PROTEST4, H_DANCE_BALL,
  H_CLIMB_RUINS, H_COUNT
};
static const uint8_t hobby_min[H_COUNT] = {0, 1, 2, 5, 6, 12, 18, 24, 35, 80};
/* the pool, in its order (the pick is random among those allowed, in this order) */
static struct { uint8_t kind, min; } pool[H_COUNT - 1];
static int npool;

enum { HS_PROP = 1, HS_FLIP = 2, HS_HAPPY = 4, HS_LEFT = 8, HS_CANCEL = 16 };
static struct {
  uint8_t flags;
  int16_t x;                   /* the prop's place on its world (telescope) */
  float timer;                 /* goingIntoThisDirection */
} hob[H_COUNT];

static int hour(void) { return ((int)city.timeSinceStart / 60) % 24; }
static int minute(void) { return (int)city.timeSinceStart % 60; }
/* the citizen's x on its world (get_worldX) */
static float world_x(const Cit *c) {
  const Perm *in = P(c->inPermanent);
  return in ? in->col * 20 + c->relativeX : c->relativeX;
}

static void pool_init(void) {
  npool = 0;
  for (int k = 1; k < H_COUNT; k++) pool[npool].kind = (uint8_t)k, pool[npool].min = hobby_min[k], npool++;
}
static void pool_remove(int kind) {
  for (int i = 0; i < npool; i++)
    if (pool[i].kind == kind) {
      memmove(&pool[i], &pool[i + 1], sizeof pool[0] * (size_t)(npool - i - 1));
      npool--;
      return;
    }
}
/* setHobby: a citizen with a hobby does not go to night clubs */
static void set_hobby(Cit *c, int kind) {
  c->hobby = (uint8_t)kind;
  c->flags &= (uint16_t)~CF_LIKES_NIGHTCLUBS;
  memset(&hob[kind], 0, sizeof hob[kind]);
}
void hobby_give(Cit *c) {
  int allowed = 0;
  for (int i = 0; i < npool; i++)
    if (ncits >= pool[i].min) allowed++;
  if (!allowed || (story_is("cityofthekey") && ncits <= 100)) return;
  int pick = rnd_int(allowed);
  for (int i = 0; i < npool; i++)
    if (ncits >= pool[i].min && pick-- == 0) {
      int kind = pool[i].kind;
      set_hobby(c, kind);
      pool_remove(kind);
      return;
    }
}
/* CitySimulation.createCitizen */
uint16_t cit_create(int world, float age, uint16_t start_in, int start_x) {
  if (start_x < 0) start_x = start_in ? rnd_int(20) : rnd_int(worlds[world].w - 2);
  uint16_t id = cit_new(world, (float)start_x, age, start_in);
  if (id) hobby_give(&cits[id]);
  return id;
}

/* the steps of the original's setPath arrays */
static void path2(Cit *c, int a, int b) {
  int16_t s[2] = {(int16_t)a, (int16_t)b};
  cit_set_path(c, s, 1);
}
static void path4(Cit *c, int a, int b, int d, int e) {
  int16_t s[4] = {(int16_t)a, (int16_t)b, (int16_t)d, (int16_t)e};
  cit_set_path(c, s, 2);
}
static void path_here(Cit *c, int a, int b) {
  path2(c, a, b);
  c->pathOnlyRelatedTo = c->inPermanent;
}

bool hobby_is_active(const Cit *c) {
  int h = hour(), k = c->hobby;
  if (k <= H_NONE || k >= H_COUNT) return false;
  bool prop = hob[k].flags & HS_PROP;
  switch (k) {
  case H_CLIMB_TREE: return h <= 4 && c->currentAction != 3;
  case H_MOVE_TO_EDGE: {
    float t = (float)fmod(city.timeSinceStart / 60, 24);
    return (t >= 23 || t < 5) && c->currentAction != 3;
  }
  case H_WATCH_STARS: return (h <= 4 && c->currentAction != 3) || prop;
  case H_PROTEST: case H_PROTEST2: case H_PROTEST3: case H_PROTEST4: {
    float hp = happiness.happiness;
    bool happy = k == H_PROTEST && hp >= 99.99f;
    bool sad = hp <= 10.01f;
    if (k != H_PROTEST && sad) {
      int days = k == H_PROTEST2 ? 2 : k == H_PROTEST3 ? 4 : 7;
      sad = happiness.veryUnhappyFromDay < day_number() - days;
    }
    return (h <= 4 && c->currentAction != 3 && (happy || sad)) || prop;
  }
  case H_DANCE_BALL: return (h >= 1 && h <= 5 && c->currentAction != 3) || prop;
  case H_CLIMB_RUINS: return h >= 1 && h <= 4 && !(hob[k].flags & HS_CANCEL) && c->currentAction != 3;
  }
  return false;
}

/* the nearest forest left untouched, or ruins, on the citizen's world (the signed distance, as written) */
float resource_initial(const Perm *p);
void resource_climb(const Perm *p, int *x, int *y);
void ruins_climb(const Perm *p, int *x, int *y);
static uint16_t nearest_on_world(const Cit *c, bool ruins) {
  uint16_t best = NONE;
  float bestd = 100000;
  int w = c->onWorld;
  for (int col = 0; col < worlds[w].ncols; col++) {
    Perm *p = P(perm_at(w, col, 0));
    if (!p) continue;
    if (ruins ? !is_a(p, B_RUINS) : !is_a(p, B_FOREST) || STC(LimitedState, p)->materialsLeft != resource_initial(p)) continue;
    float d = col * 20 - world_x(c);
    if (d < bestd) best = pid(p), bestd = d;
  }
  return best;
}
static void climb(Cit *c, bool ruins) {
  int k = c->hobby;
  if (ruins && (hob[k].flags & HS_CANCEL)) return;
  Perm *in = P(c->inPermanent);
  if (in && (ruins ? is_a(in, B_RUINS) : is_a(in, B_FOREST))) {
    /* (for the ruins the hour test is 4.5: never true, so the climber stays up) */
    if (!ruins && hour() == 4 && minute() > 45) {
      if (c->relativeY != 0) path2(c, 9, 0);
    } else if (c->relativeY == 0) {
      int x, y;
      if (ruins) ruins_climb(in, &x, &y);
      else resource_climb(in, &x, &y);
      path4(c, 4, x, 9, y);
    }
    return;
  }
  uint16_t to = nearest_on_world(c, ruins);
  if (to) path_find(c, to, THEN_NONE);
  else if (ruins) hob[k].flags |= HS_CANCEL;
}
/* a prop put away: the citizen waits a little (setPath([8, 5])) */
static void prop_off(Cit *c) {
  hob[c->hobby].flags &= (uint8_t)~HS_PROP;
  path_here(c, 8, 5);
}
static void walk_with_prop(Cit *c, float timeMod, float speed, bool turns) {
  int k = c->hobby;
  float maxx = worlds[c->onWorld].w - 2;
  int dir = hob[k].flags & HS_LEFT ? -1 : 1;
  c->relativeX += timeMod * speed * dir;
  if ((dir == 1 && c->relativeX >= maxx) || (dir == -1 && c->relativeX <= 0)) {
    hob[k].flags ^= HS_LEFT;
  } else if (turns) {
    if (hob[k].timer > 150 && rnd_int(90) == 1) hob[k].flags ^= HS_LEFT, hob[k].timer = 0;
    else hob[k].timer += timeMod;
  }
}
/* findNearestBestBuildingStack: the nearest column with nothing (1000), else with a forest (999) */
static int best_column(const Cit *c) {
  int w = c->onWorld, best = -1, bestv = -1;
  float bestd = 1e9f;
  for (int col = 0; col < worlds[w].ncols; col++) {
    Perm *p = P(perm_at(w, col, 0));
    int v = !p ? 1000 : is_a(p, B_FOREST) ? 999 : 0;
    float d = fabsf(col * 20 + 9 - c->relativeX);
    if (v > bestv || (v == bestv && d < bestd)) best = col, bestv = v, bestd = d;
  }
  return best < 0 ? 0 : best;
}
static uint8_t TH_TELESCOPE;
static void th_telescope(Cit *c, uint16_t arg) {
  int k = c->hobby;
  if (k != H_WATCH_STARS) return;
  int pos = arg;
  hob[k].flags |= HS_PROP;
  if (rnd_int(2) == 0) hob[k].x = (int16_t)(pos - 4), hob[k].flags &= (uint8_t)~HS_FLIP;
  else hob[k].x = (int16_t)(pos + 2 + 4), hob[k].flags |= HS_FLIP;
}

void hobby_update(Cit *c, float timeMod) {
  int k = c->hobby, h = hour();
  switch (k) {
  case H_CLIMB_TREE: climb(c, false); break;
  case H_CLIMB_RUINS: climb(c, true); break;
  case H_MOVE_TO_EDGE: {
    float maxx = worlds[c->onWorld].w - 2;
    if (c->inPermanent || (c->relativeX > 0 && c->relativeX < maxx)) {
      if (!c->inPermanent) path_here(c, 4, c->relativeX < maxx / 2 ? 0 : (int)maxx);
      else cit_go_down_towards_world_surface(c, timeMod);
    }
    break;
  }
  case H_WATCH_STARS:
    if (h <= 4) {
      if (hob[k].flags & HS_PROP) break;
      if (c->inPermanent) {
        cit_go_down_towards_world_surface(c, timeMod);
      } else {
        int pos = best_column(c) * 20 + rnd_between(4, 18 - 4);
        int16_t s[2] = {4, (int16_t)pos};
        cit_set_path_then(c, s, 1, TH_TELESCOPE, (uint16_t)pos);
        c->pathOnlyRelatedTo = c->inPermanent;
      }
    } else if (hob[k].flags & HS_PROP) {
      prop_off(c);
    }
    break;
  case H_PROTEST: case H_PROTEST2: case H_PROTEST3: case H_PROTEST4: {
    float hp = happiness.happiness;
    bool happy = k == H_PROTEST && hp >= 99.99f, sad = hp <= 10.01f;
    if (k != H_PROTEST && sad) sad = happiness.veryUnhappyFromDay < day_number() - (k == H_PROTEST2 ? 2 : k == H_PROTEST3 ? 4 : 7);
    if (h <= 4 && (happy || sad)) {
      if (c->inPermanent) {
        cit_go_down_towards_world_surface(c, timeMod);
        break;
      }
      if (!(hob[k].flags & HS_PROP)) {
        hob[k].flags = (uint8_t)((hob[k].flags & HS_CANCEL) | HS_PROP | (rnd_int(2) ? 0 : HS_LEFT) | (happy ? HS_HAPPY : 0));
      }
      walk_with_prop(c, timeMod, 0.6666667f, true);
    } else if (hob[k].flags & HS_PROP) {
      prop_off(c);
    }
    break;
  }
  case H_DANCE_BALL:
    if (h >= 1 && h <= 5) {
      if (c->inPermanent) {
        cit_go_down_towards_world_surface(c, timeMod);
        break;
      }
      c->relativeY = 4;
      if (!(hob[k].flags & HS_PROP)) hob[k].flags = (uint8_t)(HS_PROP | (rnd_int(2) ? 0 : HS_LEFT));
      walk_with_prop(c, timeMod, 0.5f, false);
    } else if (hob[k].flags & HS_PROP) {
      prop_off(c);
      c->relativeY = 0;
    }
    break;
  }
}

/* onDie: the hobby goes back to the pool, for anyone */
void hobby_on_die(Cit *c) {
  int k = c->hobby;
  if (k <= H_NONE || k >= H_COUNT) return;
  hob[k].flags = 0;
  if (npool < H_COUNT - 1) pool[npool].kind = (uint8_t)k, pool[npool].min = 0, npool++;
  c->hobby = H_NONE;
}

/* the props (on the far foreground): telescope, protest signs, balls */
void hobby_draw_free(const GfxBand *b, int layer) {
  if (layer != LAYER_FRONT) return;
  for (int i = 1; i < MAX_CITS; i++) {
    const Cit *c = &cits[i];
    int k = c->hobby;
    if (!(c->flags & CF_ALIVE) || k <= H_NONE || k >= H_COUNT || !(hob[k].flags & HS_PROP)) continue;
    const World *w = &worlds[c->onWorld];
    uint16_t spr;
    float x, y;
    uint8_t flip = 0;
    if (k == H_WATCH_STARS) {
      spr = SPR_TELESCOPE, x = w->x + hob[k].x, y = w->y - 5;
      if (hob[k].flags & HS_FLIP) flip = GFX_FLIPX, x -= spr_w(spr);   /* (scale -1 draws to the left of x) */
    } else if (k == H_DANCE_BALL) {
      spr = SPR_CITIZEN_BALL, x = w->x + c->relativeX - 1, y = w->y - 4;
    } else {
      spr = hob[k].flags & HS_HAPPY ? SPR_PROTESTSIGN_HAPPY : SPR_PROTESTSIGN_SAD;
      x = w->x + c->relativeX - (hob[k].flags & HS_LEFT ? 5 : 2), y = w->y - spr_h(spr);
    }
    int sx = bg_screen_x(&camera, x), sy = bg_screen_y(&camera, y);
    gfx_band_sprite(b, spr, 0, 0, spr_w(spr), spr_h(spr), sx, sy, flip, 255, GFX_NO_TINT);
  }
}

/* a new city: the whole pool; after a load: the pool without the living citizens' hobbies (the props are gone) */
void hobbies_new_city(void) {
  pool_init();
  memset(hob, 0, sizeof hob);
}
void hobbies_after_load(void) {
  pool_init();
  memset(hob, 0, sizeof hob);
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].hobby) {
      if (cits[i].hobby >= H_COUNT) cits[i].hobby = H_NONE;
      else pool_remove(cits[i].hobby);
    }
}
void hobbies_save(SaveIO *io) { (void)io; }   /* (the citizens keep their hobbies) */
bool hobbies_load(SaveIO *io) { (void)io; return true; }

void bld_hobbies_init(void) { TH_TELESCOPE = then_register(th_telescope); }
