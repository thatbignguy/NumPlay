/* The Final Earth 2: drawing the city (the original's City stages, back to
 * front): the sky, the buildings' backs, the citizens inside them, upgrade
 * sprites, the buildings' fronts and the worlds' islands, what is above them,
 * the citizens outside, and the foreground. Each layer is drawn band by band
 * (gfx_call), over the permanents on screen. */
#include "bld.h"
#include "bg.h"
#include "data.h"
#include "view.h"

BgCamera camera = {0, 0, 1};
extern BgWorld bgworlds[MAX_WORLDS];

#define MAX_VISIBLE 384
static uint16_t visible[MAX_VISIBLE];
static int16_t vis_x[MAX_VISIBLE], vis_y[MAX_VISIBLE];   /* screen positions of their top-left */
static int nvisible;

/* the permanents on screen this frame */
static void find_visible(void) {
  nvisible = 0;
  int z = camera.zoom;
  float x0 = camera.x - 20, y0 = camera.y - 20;
  float x1 = camera.x + (float)SCREEN_W / z, y1 = camera.y + (float)SCREEN_H / z;
  for (int w = 0; w < nworlds; w++) {
    World *wd = &worlds[w];
    if (wd->x + wd->w < x0 || wd->x > x1 || wd->y < y0) continue;
    int c0 = (int)((x0 - wd->x) / 20), c1 = (int)((x1 - wd->x) / 20) + 1;
    if (c0 < 0) c0 = 0;
    if (c1 > wd->ncols) c1 = wd->ncols;
    for (int c = c0; c < c1; c++) {
      int h = col_stack_height(w, c);
      for (int r = 0; r < h; r++) {
        int py = wd->y - 20 * (r + 1);
        if (py > y1) continue;
        if (py + 20 < y0) break;
        uint16_t id = perm_at(w, c, r);
        if (!id || nvisible >= MAX_VISIBLE) continue;
        visible[nvisible] = id;
        vis_x[nvisible] = (int16_t)bg_screen_x(&camera, (float)(wd->x + 20 * c));
        vis_y[nvisible] = (int16_t)bg_screen_y(&camera, (float)py);
        nvisible++;
      }
    }
  }
}

static void band_layer(const GfxBand *b, void *ctx) {
  int layer = (int)(intptr_t)ctx;
  int size = 20 * camera.zoom;
  for (int i = 0; i < nvisible; i++) {
    if (vis_y[i] >= b->y1 || vis_y[i] + size + 20 * camera.zoom <= b->y0) continue;   /* (some draw a little above themselves) */
    bld_draw(b, P(visible[i]), layer, vis_x[i], vis_y[i]);
  }
  bld_draw_free(b, layer);
}

/* a citizen's sprite: children and adults have 51 looks each, then Dracula and Waldo */
static void draw_citizen(const GfxBand *b, const Cit *c, float cx, float cy, uint8_t alpha) {
  bool child = cit_age(c) < 16;
  uint16_t spr;
  int frame = 0;
  if (c->spriteIndex == NCITIZEN_SPRITES - 1) spr = child ? SPR_WALDO_CHILD : SPR_WALDO_ADULT;
  else if (c->spriteIndex == NCITIZEN_SPRITES - 2) spr = child ? SPR_DRACULA_CHILD : SPR_DRACULA_ADULT;
  else spr = child ? SPR_CHILDREN : SPR_HUMANS, frame = c->spriteIndex;
  int h = spr_h(spr);
  int x = bg_screen_x(&camera, cx), y = bg_screen_y(&camera, cy - h);   /* the sprite's bottom at cy */
  gfx_band_sprite(b, spr, frame * 4, 0, 2, h, x, y, 0, alpha, GFX_NO_TINT);
}

/* citizens inside buildings (inside != 0) or outside them */
static void band_citizens(const GfxBand *b, void *ctx) {
  bool inside = ctx != NULL;
  int z = camera.zoom;
  float x0 = camera.x - 4, x1 = camera.x + (float)SCREEN_W / z + 4;
  float y0 = camera.y + (float)b->y0 / z - 1, y1 = camera.y + (float)b->y1 / z + 6;
  for (int i = 1; i < MAX_CITS; i++) {
    const Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE) || (c->flags & CF_CONTROLLED)) continue;
    const Perm *in = P(c->inPermanent);
    bool in_building = is_building(in);
    if (in_building != inside) continue;
    float x, y;
    uint8_t alpha = 255;
    if (in) {
      x = (float)perm_city_x(in) + c->relativeX;
      y = (float)perm_city_y(in) + 20 - c->relativeY - (in_building ? 1 : 0);
      if (in_building) {
        if (!(c->flags & CF_CAN_VIEW_SELF) || (c->flags & CF_DELAY_CAN_VIEW_SELF)) continue;
        if (c->currentPathAction == 7 && cit_timer(c) > 0 && cit_timer(c) < 1)
          alpha = (uint8_t)(cit_timer(c) * 255);
      }
    } else {
      x = worlds[c->onWorld].x + c->relativeX;
      y = worlds[c->onWorld].y - c->relativeY;
    }
    if (x < x0 || x > x1 || y < y0 || y > y1) continue;
    draw_citizen(b, c, x, y, alpha);
  }
}

void view_draw(void) {
  find_visible();
  bg_draw_sky(&camera, city.timeSinceStart);
  gfx_zoom(camera.zoom);
  gfx_call(band_layer, (void *)(intptr_t)LAYER_BG, 0, SCREEN_H);
  gfx_call(band_citizens, (void *)1, 0, SCREEN_H);
  gfx_call(band_layer, (void *)(intptr_t)LAYER_MID, 0, SCREEN_H);
  gfx_call(band_layer, (void *)(intptr_t)LAYER_MAIN, 0, SCREEN_H);
  for (int w = 0; w < nworlds; w++)
    if (worlds[w].h > 0) bg_draw_world(&bgworlds[w], col_deco + worlds[w].col0, &camera);
  gfx_call(band_layer, (void *)(intptr_t)LAYER_ABOVE, 0, SCREEN_H);
  gfx_call(band_citizens, NULL, 0, SCREEN_H);
  gfx_call(band_layer, (void *)(intptr_t)LAYER_FRONT, 0, SCREEN_H);
  gfx_zoom(1);
}

/* the city's edges (City.getCityEdges), for the clouds and the camera's limits */
void view_edges(BgEdges *e) {
  e->minX = 1 << 30, e->maxX = -(1 << 30), e->minY = 1 << 30, e->maxY = -(1 << 30);
  for (int w = 0; w < nworlds; w++) {
    World *wd = &worlds[w];
    if (wd->h == 0) continue;
    int top = wd->y;
    for (int c = 0; c < wd->ncols; c++) {
      int t = wd->y - 20 * col_stack_height(w, c);
      if (t < top) top = t;
    }
    if (wd->x < e->minX) e->minX = wd->x;
    if (wd->x + wd->w > e->maxX) e->maxX = wd->x + wd->w;
    if (top < e->minY) e->minY = top;
    if (wd->y + wd->h > e->maxY) e->maxY = wd->y + wd->h;
  }
}
