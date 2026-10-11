/* The Final Earth 2: the city screen (the original's CityView keyboard
 * scrolling and zoom, Builder, gui.BuildingButtons, gui.CityGUI's city info
 * and gui.Tooltip), made for keys: a cursor on the city's cells instead of
 * the mouse, and focus moving over the build menu and the city panel. */
#include <math.h>
#include "bldi.h"
#include "game.h"
#include "save.h"
#include "uii.h"
#include "view.h"

CityUi cu;
const float simulation_speeds[5] = {1, 1.5f, 2, 0.5f, 0.75f};
static const uint16_t speed_icon[5] = {SPR_ICON_PLAY_NORMALSPEED, SPR_ONEANDHALFSPEED, SPR_ICON_SPEEDUP,
                                       SPR_ICON_HALFSPEED, SPR_ICON_SLIGHTLYSLOWERSPEED};
static int8_t fixed_world = -1, fixed_col;   /* Builder.fixBuilder (the first tutorial's woodcutting centre) */
static uint8_t build_flash;                  /* a "can't build" flash of the ghost */

void city_ui_reset(void) {
  memset(&cu, 0, sizeof cu);
  cu.category = -1;
  cu.bfocus = -1;
  building_mode = BM_NORMAL;
  fixed_world = -1;
  int s = 0;
  for (int i = 0; i < 5; i++)
    if (fabsf(simulation_speeds[i] - city.simulationSpeed) < fabsf(simulation_speeds[s] - city.simulationSpeed)) s = i;
  cu.speed = (uint8_t)s;
  /* the cursor in the middle of the screen */
  float zx = SCREEN_W / 2.0f / camera.zoom, zy = SCREEN_H / 2.0f / camera.zoom;
  cursor_to((int)(camera.x + zx), (int)(camera.y + zy));
}

/* ---------------------------------------------------------------- the camera and the cursor */
/* CityView.updateMovingView: the view's centre stays within the city's edges */
void camera_clamp_view(void) {
  float hw = SCREEN_W / 2.0f / camera.zoom, hh = SCREEN_H / 2.0f / camera.zoom;
  float cx = camera.x + hw, cy = camera.y + hh;
  if (city_edges.maxX > city_edges.minX) cx = cx < city_edges.minX ? city_edges.minX : cx > city_edges.maxX ? city_edges.maxX : cx;
  if (city_edges.maxY > city_edges.minY) cy = cy < city_edges.minY ? city_edges.minY : cy > city_edges.maxY ? city_edges.maxY : cy;
  camera.x = cx - hw, camera.y = cy - hh;
}

void camera_follow(float x, float y) {
  camera.x = x - SCREEN_W / 2.0f / camera.zoom;
  camera.y = y - SCREEN_H / 2.0f / camera.zoom;
  camera_clamp_view();
}

static int floor20(int v) { return v >= 0 ? v / 20 * 20 : -((-v + 19) / 20 * 20); }

void cursor_to(int x, int y) {
  /* not beyond the city (with a margin to build at its edges) */
  if (city_edges.maxX > city_edges.minX) {
    if (x < city_edges.minX - 60) x = city_edges.minX - 60;
    if (x > city_edges.maxX + 40) x = city_edges.maxX + 40;
    if (y < city_edges.minY - 100) y = city_edges.minY - 100;
    if (y > city_edges.maxY + 20) y = city_edges.maxY + 20;
  }
  cu.cx = (int16_t)floor20(x);
  cu.cy = (int16_t)floor20(y);
}

/* the view follows the cursor when it nears an edge */
static void camera_to_cursor(void) {
  float vw = (float)SCREEN_W / camera.zoom, vh = (float)SCREEN_H / camera.zoom;
  float mx = 40.0f / camera.zoom + 10, my = 30.0f / camera.zoom + 10;
  if (cu.cx < camera.x + mx) camera.x = cu.cx - mx;
  if (cu.cx + 20 > camera.x + vw - mx) camera.x = cu.cx + 20 - vw + mx;
  if (cu.cy < camera.y + my) camera.y = cu.cy - my;
  if (cu.cy + 20 > camera.y + vh - my) camera.y = cu.cy + 20 - vh + my;
  camera_clamp_view();
}

/* and the cursor stays on screen when the view moves on its own */
static void cursor_to_camera(void) {
  float vw = (float)SCREEN_W / camera.zoom, vh = (float)SCREEN_H / camera.zoom;
  int x = cu.cx, y = cu.cy;
  if (x < camera.x) x = (int)ceilf(camera.x / 20) * 20;
  if (x + 20 > camera.x + vw) x = (int)floorf((camera.x + vw) / 20) * 20 - 20;
  if (y < camera.y) y = (int)ceilf(camera.y / 20) * 20;
  if (y + 20 > camera.y + vh) y = (int)floorf((camera.y + vh) / 20) * 20 - 20;
  cu.cx = (int16_t)x, cu.cy = (int16_t)y;
}

/* the world, column and row of the cursor's cell (World.handleMouse); -1 when on none */
int cursor_cell(int *col, int *row) {
  for (int w = 0; w < nworlds; w++) {
    World *wd = &worlds[w];
    if (cu.cx < wd->x || cu.cx >= wd->x + wd->w || cu.cy >= wd->y) continue;
    *col = (cu.cx - wd->x) / 20;
    *row = (wd->y - (cu.cy + 10) - 1) / 20;
    return w;
  }
  return -1;
}

static uint16_t cursor_perm(void) { return permanent_at_pos(cu.cx + 10, cu.cy + 10); }

/* CityView.handleKeyboardMove: shift+arrows move the view (2 + 12 / zoom a frame) */
static void camera_keys(void) {
  float step = (2.0f + 12.0f / camera.zoom) * 2;
  bool moved = false;
  if (key_held(K_LEFT)) camera.x -= step, moved = true;
  if (key_held(K_RIGHT)) camera.x += step, moved = true;
  if (key_held(K_UP)) camera.y -= step, moved = true;
  if (key_held(K_DOWN)) camera.y += step, moved = true;
  if (moved) {
    camera_clamp_view();
    cursor_to_camera();
  }
}

static void zoom_keys(void) {
  if (key_pressed(K_PLUS) && camera.zoom == 1) {
    /* around the cursor */
    camera.x = cu.cx + 10 - SCREEN_W / 4.0f;
    camera.y = cu.cy + 10 - SCREEN_H / 4.0f;
    camera.zoom = 2;
    camera_clamp_view();
    cursor_to_camera();
  }
  if (key_pressed(K_MINUS) && camera.zoom == 2) {
    camera.x -= SCREEN_W / 4.0f, camera.y -= SCREEN_H / 4.0f;
    camera.zoom = 1;
    camera_clamp_view();
    cursor_to_camera();
  }
}

/* ---------------------------------------------------------------- the builder */
static bool decoration_available(int i) {
  const DecoText *d = &deco_text[i];
  return !(d->flags & DT_GENE_EDITING) || upgrades.hasGeneEditing;
}

/* City.createOrRemoveBuilder: a builder for a class (its look, mirroring and colour), a decoration or a world
 * resource */
void builder_start_ex(int kind, int index, int decor, bool mirror, int32_t color) {
  cu.build_kind = (int8_t)kind;
  cu.build_cls = (uint8_t)index;
  cu.build_decor = (uint8_t)decor;
  cu.build_mirror = mirror;
  cu.build_color = color;
  builder_look = (BuilderLook){decor, mirror, color};
  if (color >= 0) color_picked = color;
  if (building_mode == BM_DESTROY || building_mode == BM_DESTROY_LEAVING_HOLE) building_mode = BM_NORMAL;
  cu.zone = Z_CITY;
  /* City.postCreateBuilder: the first tutorial's builder only builds in its place */
  extern bool tut_fix_builder;
  if (tut_fix_builder && kind == BK_BUILDING) fixed_world = 0, fixed_col = 2;
}
void builder_start(int kind, int index, int decor) {
  builder_start_ex(kind, index, decor, kind == BK_BUILDING && (cls_text[index].flags & CT_MIRRORABLE) && cu.mirrored, -1);
}

void builder_stop(void) {
  if (cu.build_kind == BK_MOVE) cu.selected = 0, moving_resource = 0;
  cu.build_kind = BK_NONE;
  fixed_world = -1;
}

/* the cost now (Builder.calculateCurrentCost) */
static void build_cost(float cost[M_COUNT]) {
  memset(cost, 0, sizeof(float) * M_COUNT);
  if (cu.build_kind == BK_BUILDING) {
    Materials m;
    builder_cost(cu.build_cls, &m);
    for (int i = 0; i < M_COUNT; i++) cost[i] = (float)m.m[i];
  } else if (cu.build_kind == BK_DECORATION) {
    const DecoText *d = &deco_text[cu.build_cls];
    cost[M_FOOD] = d->food, cost[M_WOOD] = d->wood, cost[M_STONE] = d->stone;
  } else if (cu.build_kind == BK_RESOURCE) {
    cost[M_WOOD] = res_text[cu.build_cls].wood, cost[M_STONE] = res_text[cu.build_cls].stone;
  }
}

static bool can_build_on_surface(const World *w) { return !(w->flags & WF_PROTECTED_KEY) && w->h != 0 && !(w->flags & WF_UNBUILDABLE_ALIENS); }

/* World.onCannotBuild */
static void cannot_build(const World *w) {
  if (w->flags & WF_UNBUILDABLE_ALIENS) win_open_text(NULL, L_building_prevented_strange_force);
  if (w->flags & WF_PROTECTED_KEY) win_open_text(NULL, L_secret_society_forbidden);
}

/* what building at the cursor would do: BP_* for buildings; for the rest BP_OK or BP_IMPOSSIBLE */
static int build_check(int *wo, int *col, int *row) {
  int w = cursor_cell(col, row);
  *wo = w;
  if (w < 0) return BP_IMPOSSIBLE;
  if (fixed_world >= 0 && (w != fixed_world || *col != fixed_col)) return BP_IMPOSSIBLE;
  World *wd = &worlds[w];
  if (cu.build_kind == BK_BUILDING) return builder_can_build(w, *col, *row, cu.build_cls);
  /* decorations and world resources: on the ground of an empty column */
  if (*row != 0) return BP_IMPOSSIBLE;
  if (!can_build_on_surface(wd)) return BP_WORLD_NOT_ALLOWED;
  if (perm_at(w, *col, 0)) return BP_IMPOSSIBLE;
  return BP_OK;
}

/* Builder.buildingPrerequirementsValid */
static bool build_prereq(void) {
  if (cu.build_kind == BK_BUILDING) return builder_prerequisites(cu.build_cls);
  float cost[M_COUNT];
  build_cost(cost);
  return mat_can_afford(cost);
}

/* OK while building (Builder.tryBuildingAt and its decoration and world resource cases) */
static void build_here(void) {
  int w, col, row;
  int bp = build_check(&w, &col, &row);
  if (bp == BP_WORLD_NOT_ALLOWED) {
    cannot_build(&worlds[w]);
    return;
  }
  if (bp != BP_OK || !build_prereq()) {
    build_flash = 12;
    return;
  }
  float cost[M_COUNT];
  build_cost(cost);
  if (cu.build_kind == BK_MOVE) {
    /* createWorldResourceBuilderMove: the ruins go there, the builder ends */
    if (bld_move_resource(moving_resource, w, col)) builder_stop();
  } else if (cu.build_kind == BK_BUILDING) {
    uint16_t id = builder_build(w, col, row, cu.build_cls);
    if (!id) return;
    const ClsText *ct = &cls_text[cu.build_cls];
    if (ct->flags & CT_MULTI_DECOR) bld_customize(P(id), cu.build_decor, cu.build_mirror);
    if ((ct->flags & CT_ANYCOLOR) && cu.build_color >= 0) bld_customize_color(P(id), cu.build_color);
    fixed_world = -1;
  } else if (cu.build_kind == BK_DECORATION) {
    const DecoText *d = &deco_text[cu.build_cls];
    uint8_t *decos = col_deco + worlds[w].col0;
    if (d->flags & DT_REMOVE) {
      decos[col] = 0;
    } else {
      int kind = 0;
      for (int k = 1; k < DECO_KINDS; k++)
        if (bg_deco_sprite(k) == d->spr) kind = k;
      if (kind && decos[col] >> 4 == kind) bg_set_decoration(decos, col, kind, ((decos[col] & 15) + 1) % bg_deco_frames(kind), false);
      else bg_set_decoration(decos, col, kind, -1, false);
    }
    mat_pay(cost);
  } else if (cu.build_kind == BK_RESOURCE) {
    const ResText *r = &res_text[cu.build_cls];
    col_deco[worlds[w].col0 + col] = 0;
    uint16_t id = perm_new(r->cls, w, col, 0);
    if (!id) return;
    resource_planted(P(id), r->resources < 0 ? 0 : r->resources, r->grow < 0 ? 0 : r->grow);
    mat_pay(cost);
    city_update_connections();
    sim_update_pathfinder(false, id);
  }
  view_edges(&city_edges);
}

/* Builder.tryBuildingMulti: in the drag modes, while OK is held, each cell the cursor passes on its way from
 * (x0, y0) is built on, where it can be and the city can pay (cells that can't are passed over silently) */
static void build_line(int x0, int y0, int x1, int y1) {
  int sx = cu.cx, sy = cu.cy;
  int dx = (x1 - x0) / 20, dy = (y1 - y0) / 20, n = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
  for (int i = 1; i <= n; i++) {
    cu.cx = (int16_t)(x0 + 20 * (int)lroundf((float)dx * i / n));
    cu.cy = (int16_t)(y0 + 20 * (int)lroundf((float)dy * i / n));
    int w, col, row;
    if (build_check(&w, &col, &row) == BP_OK && build_prereq()) build_here();
    if (cu.build_kind != BK_BUILDING) break;
  }
  cu.cx = (int16_t)sx, cu.cy = (int16_t)sy;
}

/* the ghost: the building's first 20x20 (Builder's sprite), see-through, with the insert/replace mark */
static void draw_builder(void) {
  int z = camera.zoom;
  int x = bg_screen_x(&camera, (float)cu.cx), y = bg_screen_y(&camera, (float)cu.cy);
  int w, col, row;
  int bp = build_check(&w, &col, &row);
  bool prereq = build_prereq();
  uint16_t spr = 0;
  int sx = 0;
  uint8_t flags = 0;
  if (cu.build_kind == BK_BUILDING) {
    const ClsText *ct = &cls_text[cu.build_cls];
    spr = ct->on_build ? ct->on_build : pcls[cu.build_cls].sprite;
    if (ct->flags & CT_MULTI_SPEC) spr = cu.build_decor ? SPR_TREEPLANTATION : SPR_INDOORFARM;
    else sx = cu.build_decor * 22;
    if (cu.build_mirror) flags = GFX_FLIPX;
  } else if (cu.build_kind == BK_DECORATION) {
    spr = deco_text[cu.build_cls].spr;
  } else if (cu.build_kind == BK_RESOURCE) {
    spr = pcls[res_text[cu.build_cls].cls].sprite;
  } else if (cu.build_kind == BK_MOVE && P(moving_resource)) {
    /* the ruins as they look */
    static const uint16_t ruins[3] = {SPR_ALIENRUINS, SPR_ALIENRUINS_2, SPR_ALIENRUINS_3};
    const Perm *p = P(moving_resource);
    spr = is_a(p, B_RUINS) ? ruins[STC(RuinsState, p)->sprite % 3] : pcls[p->cls].sprite;
  }
  /* (a builder with a colour shows it, as decorationAppearanceColor tints the sprite) */
  uint16_t own = cu.build_kind == BK_BUILDING && cu.build_color >= 0 ? rgb565_near((uint32_t)cu.build_color) : GFX_NO_TINT;
  uint16_t tint = own;
  uint8_t alpha = 51;   /* 0.2 */
  bool mark = false, replace = false;
  if (!prereq || build_flash) {
    tint = rgb565(0xFF8080), alpha = 153;   /* 0.6 */
  } else if (bp == BP_IN_WORLD) {
    tint = rgb565(0xFF8080);
  } else if (bp == BP_OK) {
    alpha = key_held(K_OK) ? 204 : 128;   /* 0.5 (0.8 while pressed) */
    if (cu.build_kind == BK_BUILDING && w >= 0 && row < col_stack_height(w, col) && perm_at(w, col, row)) {
      mark = true;
      replace = building_mode_can_replace();
      if (!replace && own == GFX_NO_TINT) tint = rgb565(0xC0C0C0);
    }
  }
  if (spr) gfx_sprite_ex(spr, sx, 0, 20, 20, x, y, flags, alpha, tint);
  if (mark) gfx_sprite(replace ? SPR_REPLACEBUILDINGHERE : SPR_INSERTBUILDINGHERE, x, y, 0, 255);
  (void)z;
}

/* the mobile version's buildable places (Builder: spr_buildhereoutline, yellow where it would replace) */
static void draw_buildable(void) {
  if (cu.build_kind == BK_NONE || !build_prereq()) return;
  int z = camera.zoom;
  float x0 = camera.x - 20, x1 = camera.x + (float)SCREEN_W / z;
  int drawn = 0;
  for (int w = 0; w < nworlds && drawn < 64; w++) {
    World *wd = &worlds[w];
    if (wd->x + wd->w < x0 || wd->x > x1) continue;
    if (fixed_world >= 0 && w != fixed_world) continue;
    for (int c = 0; c < wd->ncols && drawn < 64; c++) {
      int cx = wd->x + 20 * c;
      if (cx < x0 || cx > x1) continue;
      if (fixed_world >= 0 && c != fixed_col) continue;
      int h = col_stack_height(w, c), top = h + 1;
      if (c > 0 && col_stack_height(w, c - 1) > top) top = col_stack_height(w, c - 1);
      if (c + 1 < wd->ncols && col_stack_height(w, c + 1) > top) top = col_stack_height(w, c + 1);
      if (cu.build_kind != BK_BUILDING) top = 1;
      for (int r = 0; r < top && drawn < 64; r++) {
        int py = wd->y - 20 * (r + 1);
        int sy = bg_screen_y(&camera, (float)py);
        if (sy < -20 * z || sy > SCREEN_H) continue;
        bool ok;
        if (cu.build_kind == BK_BUILDING) ok = builder_can_build(w, c, r, cu.build_cls) == BP_OK;
        else ok = r == 0 && can_build_on_surface(wd) && !perm_at(w, c, 0);
        if (!ok) continue;
        bool rep = building_mode_can_replace() && r < h && perm_at(w, c, r);
        gfx_sprite_ex(SPR_BUILDHEREOUTLINE, 0, 0, 20, 20, bg_screen_x(&camera, (float)cx), sy, 0, 255,
                      rep ? rgb565(0xFFFF00) : GFX_NO_TINT);
        drawn++;
      }
    }
  }
  if (fixed_world >= 0) {
    World *wd = &worlds[fixed_world];
    gfx_sprite(SPR_FORCEDBUILDINGLOCATION, bg_screen_x(&camera, (float)(wd->x + 20 * fixed_col - 1)),
               bg_screen_y(&camera, (float)(wd->y - 20 - 31)), 0, 255);
  }
}

/* ---------------------------------------------------------------- the city's layer */
/* setHoverHightlight's pulse: 0.5 + (sin(guiTimer / 10) + 1) / 4 */
static uint8_t pulse(float lo, float range) {
  return (uint8_t)(255 * (lo + (sinf(cu.gui_timer / 10) + 1) / 2 * range));
}

/* the cursor: the original's hover outline on the cell, with a dark edge around it so that it shows on a light sky
 * as well as on a dark one (the original's mouse pointer shows where it is; a keypad has none) */
static void cursor_box(void) {
  int x = bg_screen_x(&camera, (float)(cu.cx - 1)), y = bg_screen_y(&camera, (float)(cu.cy - 1)), s = 22 * camera.zoom;
  uint16_t dark = rgb565(0x202020);
  gfx_rect(x - 1, y - 1, s + 2, 1, dark, 150), gfx_rect(x - 1, y + s, s + 2, 1, dark, 150);
  gfx_rect(x - 1, y, 1, s, dark, 150), gfx_rect(x + s, y, 1, s, dark, 150);
  gfx_sprite(SPR_WHITEOUTLINE, x, y, 0, pulse(0.55f, 0.45f));
}

static void outline_perm(uint16_t id, uint16_t spr, uint16_t tint, uint8_t alpha, int frame) {
  Perm *p = P(id);
  if (!p) return;
  int x = bg_screen_x(&camera, (float)(perm_city_x(p) - 1)), y = bg_screen_y(&camera, (float)(perm_city_y(p) - 1));
  if (frame >= 0) gfx_sprite_ex(spr, frame * 24, 0, 22, 22, x, y, 0, alpha, tint);
  else gfx_sprite_ex(spr, 0, 0, 22, 22, x, y, 0, alpha, tint);
}

void city_draw_overlay(void) {
  gfx_zoom(camera.zoom);
  /* FindBuildingsOfType: spr_hoverhint frames and white outlines on the buildings of the class */
  if (cu.highlight_cls) {
    int c = cu.highlight_cls - 1;
    int z = camera.zoom, n = 0;
    float x0 = camera.x - 22, x1 = camera.x + (float)SCREEN_W / z, y0 = camera.y - 22, y1 = camera.y + (float)SCREEN_H / z;
    for (int i = 1; i < MAX_PERMS && n < 40; i++) {
      Perm *p = &perms[i];
      if (!(p->flags & PF_USED) || p->cls != c) continue;
      float px = (float)perm_city_x(p), py = (float)perm_city_y(p);
      if (px < x0 || px > x1 || py < y0 || py > y1) continue;
      int frame = 1;
      UpgradeOption opts[1];
      (void)opts;
      outline_perm((uint16_t)i, SPR_HOVERHINT, GFX_NO_TINT, pulse(0.5f, 0.5f), frame);
      outline_perm((uint16_t)i, SPR_WHITEOUTLINE, GFX_NO_TINT, 255, -1);
      n++;
    }
  }
  /* FollowingCitizen (and in its change window, the citizen's job, home and school) */
  bool changing = nwins && wins[nwins - 1].kind == WK_CITIZEN && (wins[nwins - 1].flags & WF2_CONFIRM);
  if (cu.following && (cits[cu.following].flags & CF_ALIVE)) {
    Cit *c = &cits[cu.following];
    uint16_t green = rgb565(0x66C600);
    if (!changing) {
    } else if (c->job && c->job == c->home) {
      outline_perm(c->job, SPR_HOVERHINTS_CITIZEN, GFX_NO_TINT, pulse(0.5f, 0.5f), 3);
      outline_perm(c->job, SPR_WHITEOUTLINE, green, 255, -1);
    } else {
      if (c->job) outline_perm(c->job, SPR_HOVERHINTS_CITIZEN, GFX_NO_TINT, pulse(0.5f, 0.5f), 0), outline_perm(c->job, SPR_WHITEOUTLINE, green, 255, -1);
      if (c->home) outline_perm(c->home, SPR_HOVERHINTS_CITIZEN, GFX_NO_TINT, pulse(0.5f, 0.5f), 1), outline_perm(c->home, SPR_WHITEOUTLINE, green, 255, -1);
    }
    if (c->school && changing) outline_perm(c->school, SPR_HOVERHINTS_CITIZEN, GFX_NO_TINT, pulse(0.5f, 0.5f), 2), outline_perm(c->school, SPR_WHITEOUTLINE, green, 255, -1);
    /* the selected citizen (spr_selectedhuman) */
    float x, y;
    cit_city_position(c, &x, &y);
    gfx_sprite(SPR_SELECTEDHUMAN, bg_screen_x(&camera, x - 1), bg_screen_y(&camera, y - 6), 0, 255);
  }
  /* the building whose window is open (spr_selectedbuilding) */
  if (cu.selected && (perms[cu.selected].flags & PF_USED)) outline_perm(cu.selected, SPR_SELECTEDBUILDING, GFX_NO_TINT, 255, -1);
  /* the story's goal: buildings that can take the upgrade it asks for (BuildingButtons.updateGoalAttentions) */
  extern void goal_upgrade_highlights(void);
  goal_upgrade_highlights();
  if (changing) cursor_box();
  if (cu.zone == Z_CITY && !nwins && !ui.title) {
    if (cu.build_kind != BK_NONE) {
      draw_buildable();
      draw_builder();
      /* the cursor stays in sight over the ghost, also where nothing can be built (the ghost is faint there) */
      cursor_box();
    } else {
      uint16_t id = cursor_perm();
      if ((building_mode == BM_DESTROY || building_mode == BM_DESTROY_LEAVING_HOLE) && is_building(P(id))) {
        /* Building.onHover in destroy mode: spr_destroying */
        Perm *p = P(id);
        gfx_sprite(SPR_DESTROYING, bg_screen_x(&camera, (float)perm_city_x(p)), bg_screen_y(&camera, (float)perm_city_y(p)), 0,
                   key_held(K_OK) ? 255 : 128);
      }
      cursor_box();
    }
  }
  gfx_zoom(1);
}

/* ---------------------------------------------------------------- the tooltip (gui.Tooltip) */
void ui_tooltip(const Tip *t, int ax, int ay, bool continuous) {
  if (!t || ((!t->header || !*t->header) && (!t->text || !*t->text))) return;
  int max_w = continuous ? SCREEN_W : SCREEN_W / 2;
  if (!continuous) {
    int m = ax - 10 > SCREEN_W - ax - 10 ? ax - 10 : SCREEN_W - ax - 10;
    if (m > 240) m = 240;
    if (m > max_w) max_w = m;
  }
  int extra_w = t->nextra ? ui_icon_list(0, INT16_MIN, t->extra, t->nextra) : 0;
  int width = continuous ? max_w : 0, y = 0, header_w = 0, hw = 0, tw = 0, th = 0, hh = 0;
  int hmax = max_w - extra_w - 10, tmax = max_w - 10;
  int cap = continuous ? 1000 : 350;
  hmax = hmax < 100 ? 100 : hmax > cap ? cap : hmax;
  tmax = tmax < 100 ? 100 : tmax > cap ? cap : tmax;
  int header_y = 0, text_y = 0, extra2_y = 0, cost_y = 0, cost2_y = 0;
  if (t->header && *t->header) {
    y--;
    hh = ui_para(FONT_16, 0, INT16_MIN, hmax, t->header, C_BLACK, true, &hw);
    header_w = hw + 6;
    if (header_w > width) width = header_w;
    header_y = y;
    y += hh + 2;
  }
  if (t->text && *t->text) {
    y++;
    th = ui_para(FONT_STANDARD, 0, INT16_MIN, tmax, t->text, C_BLACK, true, &tw);
    text_y = y;
    if (tw + 6 > width) width = tw + 6;
    y += th;
  }
  if (t->nextra2) {
    extra2_y = y + 1;
    int w2 = ui_icon_list(0, INT16_MIN, t->extra2, t->nextra2) + 6;
    if (w2 > width) width = w2;
    y += 12;
  }
  bool any_cost = false;
  if (t->cost)
    for (int m = 0; m < M_COUNT; m++) any_cost |= t->cost[m] > 0;
  char after[24] = " ";
  str_cat(after, sizeof after, L_to_research);
  if (any_cost) {
    y += 2;
    cost_y = y;
    int cw = ui_cost(0, INT16_MIN, t->cost, "", after, true);
    if (cw > width) width = cw;
    y += 11;
  }
  bool any_cost2 = false;
  int c2x = 3;
  if (t->cost2)
    for (int m = 0; m < M_COUNT; m++) any_cost2 |= t->cost2[m] > 0;
  if (any_cost2) {
    cost2_y = y;
    if (t->cost2_text && *t->cost2_text) c2x += ui_text_w(FONT_STANDARD, t->cost2_text) + 3;
    y += 2;
    int cw = ui_cost(0, INT16_MIN, t->cost2, "", "", false) + c2x;
    if (cw > width) width = cw;
    y += 11;
  }
  if (t->nextra && header_w + extra_w > width) width = header_w + extra_w;
  y += 2;
  int height = y;
  /* where: at the top-left (continuous), else above the button (or under it if there's no room) */
  int px, py;
  if (continuous) {
    px = 0, py = 0;
  } else {
    px = ax, py = ay > height ? ay - height : ay + 21;
    if (px + width > SCREEN_W) {
      px -= width;
      if (px < 0) px = 0;
    }
  }
  ui_noclip();
  ui_box(px, py, width, height, BX_TOOLTIP);
  /* (copied: tooltips are often made in a buffer that is gone by the time the frame is drawn) */
  if (hh) ui_para(FONT_16, px + 2, py + header_y, hmax, t->header, C_BLACK, false, NULL);
  if (th) ui_para(FONT_STANDARD, px + 3, py + text_y, tmax, t->text, C_BLACK, false, NULL);
  if (t->nextra2) ui_icon_list(px + 2, py + extra2_y, t->extra2, t->nextra2);
  if (any_cost) ui_cost(px + 3, py + cost_y, t->cost, "", after, true);
  if (any_cost2) {
    if (t->cost2_text) ui_text(FONT_STANDARD, px + 3, py + cost2_y, t->cost2_text, C_BLACK);
    ui_cost(px + c2x, py + cost2_y + 2, t->cost2, "", "", false);
  }
  if (t->nextra) ui_icon_list(px + width - 1 - extra_w, py + header_y + 3, t->extra, t->nextra);
}

/* ---------------------------------------------------------------- the build menu (gui.BuildingButtons) */
enum { BB_CATEGORY, BB_MODE, BB_MODES_MORE, BB_MANAGEMENT, BB_BUILDING, BB_DECORATION, BB_RESOURCE, BB_SUBCATEGORY,
       BB_SUB_BACK, BB_SUB_MIRROR, BB_MODE_PICK, BB_INVITE, BB_FOLLOW, BB_UPGRADES, BB_FIND,
       BB_ANYCOLOR_LAST, BB_ANYCOLOR_PICK };   /* an any-colour class's last colour, and its colour picker */
typedef struct {
  int16_t x, y;
  uint8_t w, kind, arg, arg2;   /* arg: category, class, decoration, resource, mode...; arg2: a decoration's look */
} BButton;
#define BB_MAX 64
typedef struct {
  BButton b[BB_MAX];
  int n, nrow;                 /* the first nrow are the bottom row (categories, modes, management) */
  int at_width;                /* the shown category's button's x */
  int8_t sub;                  /* the shown subcategory + 1 */
} BMenu;
static int8_t shown_sub;      /* the decoration subcategory shown, 0 none */
static int8_t last_category = -1;   /* the category the menu showed last */
static uint8_t menu_opened;         /* times the menu was opened (its keys are told the first times) */
static struct { int8_t kind; uint8_t arg, arg2; } last_pick = {-1, 0, 0};   /* the button picked last */
bool follow_mode;             /* viewActions.showFollow: OK picks the citizen nearest the cursor */

static bool cat_any_unlocked(int c) {
  for (int i = 0; i < nbuild_order; i++) {
    int k = build_order[i];
    if (pcls[k].category == c && !cls_text[k].sub && unlock_state(k) != UNLOCK_LOCKED) return true;
  }
  if (c == 6 && unlocks.unlockedDecorationTab) return true;   /* decorations and world resources */
  return false;
}

static void bb_add(BMenu *m, int x, int y, int w, int kind, int arg, int arg2) {
  if (m->n >= BB_MAX) return;
  m->b[m->n++] = (BButton){(int16_t)x, (int16_t)y, (uint8_t)w, (uint8_t)kind, (uint8_t)arg, (uint8_t)arg2};
}

/* the left edge of the city panel for a row of the screen (rows of buttons stop before it) */
static int panel_left_at(int y0, int y1);

/* a row of buttons to the right of x0 at y, going on in rows above it when it doesn't fit */
static void bb_row(BMenu *m, const BButton *list, int n, int x0, int y) {
  int k = 0;
  while (k < n && y >= 0) {
    int limit = panel_left_at(y, y + 22), x = x0;
    if (limit - x0 < 22) limit = x0 + 22;
    for (; k < n && x + 22 <= limit; k++, x += 22) {
      BButton b = list[k];
      b.x = (int16_t)x, b.y = (int16_t)y;
      if (m->n < BB_MAX) m->b[m->n++] = b;
    }
    y -= 22;
  }
}

/* The build menu, as UI.md has it: the categories' buttons (gui.BuildingButtons) in a column at the
 * bottom left, then the building mode and the management buttons; the shown category's buildings in a row
 * to the right of its button (more rows above it if they don't fit before the city panel). */
static void bmenu_layout(BMenu *m) {
  m->n = 0;
  int y = SCREEN_H - 22;
  int cat_y = -1, mode_y = -1, mgmt_y = -1;
  m->at_width = -1;
  for (int c = 0; c < NCATEGORIES; c++) {
    if (!cat_any_unlocked(c)) continue;
    if (c == cu.category) cat_y = y;
    bb_add(m, 0, y, 22, BB_CATEGORY, c, 0);
    y -= 22;
  }
  if (unlocks.unlockedBuildingModeButton) {
    mode_y = y;
    bb_add(m, 0, y, 22, BB_MODE, 0, 0);
    y -= 22;
  }
  if (unlocks.unlockedManagementOptions) {
    mgmt_y = y;
    bb_add(m, 0, y, 22, BB_MANAGEMENT, 0, 0);
  }
  m->nrow = m->n;
  m->at_width = cat_y;
  /* the building modes, right of the mode button */
  if (cu.modes_open && mode_y >= 0) {
    BButton list[BM_COUNT];
    for (int i = 0; i < BM_COUNT; i++) list[i] = (BButton){0, 0, 22, BB_MODE_PICK, (uint8_t)i, 0};
    bb_row(m, list, BM_COUNT, 22, mode_y);
  }
  /* the management options, right of the management button */
  if (cu.mgmt_open && mgmt_y >= 0) {
    BButton list[4];
    int k = 0;
    if (stories[story.index].flags & ST_INVITE) list[k++] = (BButton){0, 0, 22, BB_INVITE, 0, 0};
    list[k++] = (BButton){0, 0, 22, BB_FOLLOW, 0, 0};
    list[k++] = (BButton){0, 0, 22, BB_UPGRADES, 0, 0};
    list[k++] = (BButton){0, 0, 22, BB_FIND, 0, 0};
    bb_row(m, list, k, 22, mgmt_y);
  }
  if (cu.category < 0 || cat_y < 0) return;
  /* the buttons of the category, in the original's order */
  BButton list[BB_MAX];
  int n = 0;
#define ADD(kind, a, a2) do { if (n < BB_MAX) list[n++] = (BButton){0, 0, 22, (kind), (uint8_t)(a), (uint8_t)(a2)}; } while (0)
  int c = cu.category;
  if (shown_sub) {
    ADD(BB_SUB_BACK, 0, 0);
    if (shown_sub - 1 == 1) ADD(BB_SUB_MIRROR, 0, 0);   /* slanted roofs */
  }
  for (int i = 0; i < nbuild_order; i++) {
    int k = build_order[i];
    if (unlock_state(k) == UNLOCK_LOCKED) continue;
    if (shown_sub ? cls_text[k].sub != shown_sub : (pcls[k].category != c || cls_text[k].sub)) continue;
    const ClsText *ct = &cls_text[k];
    bool needs_unlock = ct->hint && unlock_state(k) == UNLOCK_UNLOCKED;
    if ((ct->flags & CT_MULTI_DECOR) && !needs_unlock) {
      int looks = (ct->flags & CT_MULTI_SPEC) ? 2 : (spr_w(pcls[k].sprite) + 2) / 22;
      if (ct->flags & CT_ANYCOLOR) {
        /* the last look in the colour picked when the menu opened, the looks, then the last look: the picker */
        if (color_picked_on_menu_open >= 0) ADD(BB_ANYCOLOR_LAST, k, looks - 1);
        for (int l = 0; l < looks - 1; l++) ADD(BB_BUILDING, k, l);
        ADD(BB_ANYCOLOR_PICK, k, looks - 1);
      } else {
        for (int l = 0; l < looks; l++) ADD(BB_BUILDING, k, l);
      }
    } else {
      ADD(BB_BUILDING, k, 0);
    }
  }
  if (!shown_sub && c == 6 && unlocks.unlockedDecorationTab) {
    for (int i = 0; i < ndeco_text; i++)
      if (decoration_available(i)) ADD(BB_DECORATION, i, 0);
    for (int i = 0; i < nres_text; i++)
      if (unlock_state(res_text[i].cls) != UNLOCK_LOCKED) ADD(BB_RESOURCE, i, 0);
    for (int s = 0; s < NSUBCATEGORIES; s++) ADD(BB_SUBCATEGORY, s, 0);
  }
#undef ADD
  bb_row(m, list, n, 22, cat_y);
}

static const uint16_t mode_icon[] = {
  [BM_NORMAL] = SPR_ICON_BUILDINGMODE_ONTOP, [BM_INSERT] = SPR_ICON_BUILDINGMODE_INSERT,
  [BM_REPLACE] = SPR_ICON_BUILDINGMODE_REPLACE, [BM_DRAG] = SPR_ICON_BUILDINGMODE_DRAG,
  [BM_DRAG_REPLACE] = SPR_ICON_BUILDINGMODE_DRAGREPLACE, [BM_DESTROY] = SPR_ICON_BUILDINGMODE_DESTROY,
  [BM_DESTROY_LEAVING_HOLE] = SPR_ICON_BUILDINGMODE_DESTROYLEAVINGHOLE};

static bool bb_active(const BButton *b) {
  switch (b->kind) {
  case BB_CATEGORY: return cu.category == b->arg;
  case BB_MODE: return cu.modes_open;
  case BB_MANAGEMENT: return cu.mgmt_open;
  case BB_MODE_PICK: return building_mode == b->arg;
  case BB_BUILDING:
  case BB_ANYCOLOR_PICK:
    /* (the picker's button is never on once there is a last-colour button) */
    if (b->kind == BB_ANYCOLOR_PICK && color_picked_on_menu_open >= 0) return false;
    return cu.build_kind == BK_BUILDING && cu.build_cls == b->arg && cu.build_decor == b->arg2 && cu.build_color < 0;
  case BB_ANYCOLOR_LAST:
    return cu.build_kind == BK_BUILDING && cu.build_cls == b->arg && cu.build_decor == b->arg2 &&
           cu.build_color == color_picked_on_menu_open;
  case BB_DECORATION: return cu.build_kind == BK_DECORATION && cu.build_cls == b->arg;
  case BB_RESOURCE: return cu.build_kind == BK_RESOURCE && cu.build_cls == b->arg;
  case BB_INVITE: return win_is_open(WK_INVITE);
  case BB_FOLLOW: return follow_mode;
  case BB_UPGRADES: return win_is_open(WK_UPGRADES);
  case BB_FIND: return win_is_open(WK_FIND);
  }
  return false;
}

/* ---------------------------------------------------------------- the story's highlights (updateGoalAttentions) */
#define ATTENTION_TINT 0x6620   /* HSV (89, 1, 0.77) */
/* the classes and upgrades the current goal still asks for */
static struct { float stamp; int8_t n; int16_t cls[4], ups[4]; } wants;   /* worked out once a frame */
static int goal_wants(int *cls, int *ups, int max) {
  if (wants.stamp != cu.gui_timer) {
    wants.stamp = cu.gui_timer;
    wants.n = 0;
    const StoryGoal *g = story_goal();
    for (int i = 0; g && !ui.title && i < g->nsub && wants.n < 4; i++) {
      if (story_subgoal_complete(i)) continue;
      const SubGoal *sg = &g->sub[i];
      if (sg->type == SG_BUILDPERMANENTS) {
        wants.cls[wants.n] = sg->cls, wants.ups[wants.n] = -1, wants.n++;
      } else if (sg->type == SG_PERFORMBUILDINGUPGRADES || sg->type == SG_PERFORMCITYUPGRADE) {
        wants.cls[wants.n] = -1, wants.ups[wants.n] = (int16_t)upgrade_find(sg->name), wants.n++;
      }
    }
  }
  int n = wants.n < max ? wants.n : max;
  for (int i = 0; i < n; i++) cls[i] = wants.cls[i], ups[i] = wants.ups[i];
  return n;
}

static bool cls_needs_attention(int k) {
  int cls[4], ups[4], n = goal_wants(cls, ups, 4);
  for (int i = 0; i < n; i++) {
    if (cls[i] != k) continue;
    float cost[M_COUNT];
    for (int m = 0; m < M_COUNT; m++) cost[m] = pcls[k].cost[m];
    return mat_can_afford(cost);
  }
  return false;
}

static bool cat_needs_attention(int c) {
  int cls[4], ups[4], n = goal_wants(cls, ups, 4);
  for (int i = 0; i < n; i++)
    if (cls[i] >= 0 && pcls[cls[i]].category == c && cls_needs_attention(cls[i])) return true;
  return false;
}

/* buildings that can take the upgrade the goal asks for: a pulsing green outline (unless their window shows it) */
void goal_upgrade_highlights(void) {
  int cls[4], ups[4], n = goal_wants(cls, ups, 4);
  for (int i = 0; i < n; i++) {
    int u = ups[i];
    if (u < 0) continue;
    if (!mat_can_afford(upgrade_info[u].cost)) continue;
    int drawn = 0;
    for (int id = 1; id < MAX_PERMS && drawn < 24; id++) {
      Perm *p = &perms[id];
      if (!(p->flags & PF_USED) || !is_building(p)) continue;
      UpgradeOption opts[16];
      int k = upgrade_options(p, opts, 16), j = 0;
      for (; j < k; j++)
        if (opts[j].upgrade == u && !opts[j].owned) break;
      if (j == k) continue;
      if (nwins && wins[nwins - 1].kind == WK_BUILDING && wins[nwins - 1].arg == id) continue;
      outline_perm((uint16_t)id, SPR_WHITEOUTLINE, ATTENTION_TINT, pulse(0, 1), -1);
      drawn++;
    }
  }
}

/* ---------------------------------------------------------------- drawing the build menu */
static void draw_bbutton(const BButton *b, bool focused) {
  bool active = bb_active(b), down = focused && key_held(K_OK);
  int frame = down || active ? 2 : focused ? 1 : 0;
  int x = b->x, y = b->y;
  bool attention = false;
  if (b->kind == BB_CATEGORY || b->kind == BB_MODE || b->kind == BB_MANAGEMENT) {
    ui_frame(SPR_TRANSPARENTBUTTON, 22, frame, x, y, 0, 255);
  } else if (b->kind == BB_MODES_MORE) {
    ui_frame(SPR_MOREBUILDINGMODEBUTTONS, 12, frame, x, y, 0, 255);
  } else {
    ui_frame(SPR_BUILDINGBUTTON, 22, frame, x, y, 0, 255);
  }
  switch (b->kind) {
  case BB_CATEGORY:
    ui_spr(cat_text[b->arg].icon, x + 1, y + 1);
    attention = cat_needs_attention(b->arg);
    if ((unlocks.notifyCategory >> b->arg & 1) && cu.category != b->arg) ui_spr(SPR_BUTTONNOTIFICATION, x, y);
    break;
  case BB_MODE: ui_spr(mode_icon[building_mode], x + 1, y + 1); break;
  case BB_MODES_MORE: ui_spr(SPR_MOREBUILDINGMODES_ARROWUP, x + 1, y + 1); break;
  case BB_MANAGEMENT: ui_spr(SPR_ICON_MANAGEMENT, x + 1, y + 1); break;
  case BB_MODE_PICK: ui_spr(mode_icon[b->arg], x + 1, y + 1); break;
  case BB_INVITE: ui_spr(SPR_ICON_INVITEPEOPLE, x + 1, y + 1); break;
  case BB_FOLLOW: ui_spr(SPR_ICON_FOLLOWCITIZEN, x + 1, y + 1); break;
  case BB_UPGRADES: ui_spr(SPR_ICON_UPGRADE, x + 1, y + 1); break;
  case BB_FIND: ui_spr(SPR_ICON_FINDBUILDING, x + 1, y + 1); break;
  case BB_SUB_BACK: ui_spr(SPR_ARROW_BACK, x + 1, y + 1); break;
  case BB_SUB_MIRROR: ui_spr(SPR_MIRROR, x + 1, y + 1); break;
  case BB_SUBCATEGORY: ui_spr_ex(sub_text[b->arg].icon, 0, 0, 20, 20, x + 1, y + 1, 0, 255, GFX_NO_TINT); break;
  case BB_DECORATION: ui_spr_ex(deco_text[b->arg].spr, 0, 0, 20, 20, x + 1, y + 1, 0, 255, GFX_NO_TINT); break;
  case BB_RESOURCE: ui_spr_ex(pcls[res_text[b->arg].cls].sprite, 0, 0, 20, 20, x + 1, y + 1, 0, 255, GFX_NO_TINT); break;
  case BB_ANYCOLOR_LAST:
  case BB_ANYCOLOR_PICK:
  case BB_BUILDING: {
    int k = b->arg;
    const ClsText *ct = &cls_text[k];
    bool locked = ct->hint && unlock_state(k) == UNLOCK_UNLOCKED;
    uint8_t flags = (ct->flags & CT_MIRRORABLE) && cu.mirrored ? GFX_FLIPX : 0;
    uint16_t spr = pcls[k].sprite;
    if (locked) {
      ui_spr_ex(SPR_LOCKEDBUILDING, 22, 0, 20, 20, x + 1, y + 1, 0, 255, GFX_NO_TINT);
      ui_spr_ex(SPR_LOCKEDBUILDING, 0, 0, 20, 20, x + 1, y + 1, 0, 255, GFX_NO_TINT);
    } else if (ct->flags & CT_MULTI_SPEC) {
      uint16_t s2 = b->arg2 ? SPR_TREEPLANTATION : SPR_INDOORFARM;
      ui_spr_ex(s2, 44, 0, 20, 20, x + 1, y + 1, 0, 255, GFX_NO_TINT);
      ui_spr_ex(s2, 0, 0, 20, 20, x + 1, y + 1, 0, 255, GFX_NO_TINT);
    } else if (ct->flags & CT_MULTI_DECOR) {
      uint16_t tint = b->kind == BB_ANYCOLOR_LAST ? rgb565_near((uint32_t)color_picked_on_menu_open) : GFX_NO_TINT;
      ui_spr_ex(spr, 22 * b->arg2, 0, 20, 20, x + 1, y + 1, flags, 255, tint);
      if (b->kind == BB_ANYCOLOR_PICK) ui_spr(SPR_RECOLORABLE, x, y);
    } else {
      if (ct->back == BACK_OWN && spr_w(spr) >= 64) ui_spr_ex(spr, 44, 0, 20, 20, x + 1, y + 1, flags, 255, GFX_NO_TINT);
      else if (ct->back && ct->back != BACK_OWN) ui_spr_ex(ct->back, 0, 0, 20, 20, x + 1, y + 1, flags, 255, GFX_NO_TINT);
      ui_spr_ex(spr, 0, 0, 20, 20, x + 1, y + 1, flags, 255, GFX_NO_TINT);
    }
    if (unlocks_should_notify(k)) ui_spr(SPR_BUTTONNOTIFICATION, x, y);
    attention = cls_needs_attention(k);
    break;
  }
  }
  if (attention && frame == 0) ui_spr_ex(SPR_WHITEOUTLINE, 0, 0, 22, 22, x, y, 0, pulse(0, 1), ATTENTION_TINT);
}

/* the focused build menu button's tooltip */
static void bbutton_tooltip(const BButton *b) {
  Tip t = {0};
  char text[400], hb[64];
  float cost[M_COUNT], cost2[M_COUNT];
  IconText extra[4];
  char e0[16], e1[16], e2[16], e3[40];
  IconText extra2[1];
  switch (b->kind) {
  case BB_CATEGORY: t.header = cat_text[b->arg].name, t.text = cat_text[b->arg].desc; break;
  case BB_MODE:
  case BB_MODE_PICK: {
    int mode = b->kind == BB_MODE ? (int)building_mode : b->arg;
    static const char *const names[BM_COUNT] = {L_build_mode_normal, L_build_mode_insert, L_build_mode_replace,
                                                L_build_mode_multi, L_build_mode_multi_replace, L_build_mode_destroy,
                                                L_build_mode_destroyleavinghole};
    static const char *const texts[BM_COUNT] = {L_build_mode_normal_explanation, L_build_anywhere,
                                                L_build_mode_replace_explanation, L_build_mode_multi_explanation,
                                                L_build_mode_multi_replace_explanation, L_build_mode_destroy_explanation,
                                                L_build_mode_destroyleavinghole_explanation};
    t.header = names[mode], t.text = texts[mode];
    break;
  }
  case BB_MODES_MORE: t.text = L_all_building_modes; break;
  case BB_MANAGEMENT: t.text = L_management; break;
  case BB_INVITE: t.text = L_invite_people; break;
  case BB_FOLLOW: t.text = L_follow_citizen; break;
  case BB_UPGRADES: t.text = L_upgrades_and_policies; break;
  case BB_FIND: t.text = L_highlight_buildings; break;
  case BB_SUB_BACK: t.text = L_back; break;
  case BB_SUB_MIRROR: t.text = L_mirror; break;
  case BB_SUBCATEGORY: t.text = sub_text[b->arg].name; break;
  case BB_DECORATION: {
    const DecoText *d = &deco_text[b->arg];
    memset(cost, 0, sizeof cost);
    cost[M_FOOD] = d->food, cost[M_WOOD] = d->wood, cost[M_STONE] = d->stone;
    t.header = d->name, t.text = d->desc, t.cost = cost;
    break;
  }
  case BB_RESOURCE: {
    const ResText *r = &res_text[b->arg];
    memset(cost, 0, sizeof cost);
    cost[M_WOOD] = r->wood, cost[M_STONE] = r->stone;
    t.header = r->name, t.text = r->desc, t.cost = cost;
    break;
  }
  case BB_ANYCOLOR_LAST:
  case BB_ANYCOLOR_PICK:
  case BB_BUILDING: {
    /* BuildingButtons.showBuildingTooltip */
    int k = b->arg;
    const ClsText *ct = &cls_text[k];
    if (ct->hint && unlock_state(k) == UNLOCK_UNLOCKED) {
      t.text = ct->hint;
      break;
    }
    Materials m;
    builder_cost(k, &m);
    for (int i = 0; i < M_COUNT; i++) cost[i] = (float)m.m[i];
    t.cost = cost;
    int n = 0, count = 0;
    for (int i = 1; i < MAX_PERMS; i++)
      if ((perms[i].flags & PF_USED) && perms[i].cls == k) count++;
    if (pcls[k].jobs) fmt_int(e0, sizeof e0, pcls[k].jobs), extra[n++] = (IconText){SPR_WORK, e0};
    if (pcls[k].residents) fmt_int(e1, sizeof e1, pcls[k].residents), extra[n++] = (IconText){SPR_RESIDENTS, e1};
    if (pcls[k].quality) fmt_int(e2, sizeof e2, pcls[k].quality), extra[n++] = (IconText){SPR_HAPPINESS, e2};
    if (upgrades.hasGalacticLibrary && (ct->flags & CT_NOT_UNIQUE_GL)) {
      e3[0] = 0;
      if (count >= 2) str_cat(e3, sizeof e3, "[red]");
      char lim[32];
      str_cat(e3, sizeof e3, lo_1(lim, sizeof lim, L_limitedBuilding, LS("2")));
      extra[n++] = (IconText){SPR_UNIQUEBUILDING, e3};
    } else if (ct->flags & CT_UNIQUE) {
      extra[n++] = (IconText){SPR_UNIQUEBUILDING, count ? "[red]Already Built" : L_unique};
    }
    if (ct->flags & CT_LIMITED) {
      char lim[40];
      lo_1(lim, sizeof lim, L_limited_per_citizens, LN(ct->limited));
      e3[0] = 0;
      if (count >= (ncits + ct->limited - 1) / ct->limited) str_cat(e3, sizeof e3, "[red]");
      str_cat(e3, sizeof e3, lim);
      extra[n++] = (IconText){SPR_UNIQUEBUILDING, e3};
    }
    t.extra = extra, t.nextra = n;
    if (ct->tip_icon) extra2[0] = (IconText){ct->tip_icon, ""}, t.extra2 = extra2, t.nextra2 = 1;
    /* "!unlocks", and the first house's lower price */
    text[0] = 0;
    const char *d = ct->desc, *u = strstr(d, "!unlocks");
    if (u) {
      int len = (int)(u - d);
      if (len > (int)sizeof text - 1) len = (int)sizeof text - 1;
      memcpy(text, d, (size_t)len);
      text[len] = 0;
      if (!count) str_cat(str_cat(text, sizeof text, " "), sizeof text, L_also_unlocks_new);
      str_cat(text, sizeof text, u + 8);
    } else {
      str_cat(text, sizeof text, d);
    }
    if (k == CLS_NormalHouse && story_is("theLostShip") && story_goal() && !strcmp(story_goal()->name, "BuildIndoorFarmAndHouse") && !count)
      str_cat(str_cat(text, sizeof text, " "), sizeof text, L_first_house_cheaper);
    t.header = ct->name, t.text = text;
    if (pcls[k].teleporterOperatingCost > 0) {
      memset(cost2, 0, sizeof cost2);
      cost2[M_KNOWLEDGE] = pcls[k].teleporterOperatingCost * (bld_misdirector() ? 0.5f : 1) * (1 - upgrades.rocketTeleporterReward);
      t.cost2 = cost2;
      hb[0] = 0;
      t.cost2_text = str_cat(str_cat(hb, sizeof hb, L_operating_cost_pd), sizeof hb, " ");
    }
    break;
  }
  }
  /* the menu's keys, the first times it opens */
  char with_hint[480];
  if (menu_opened <= 2) {
    with_hint[0] = 0;
    if (t.text && *t.text) str_cat(str_cat(with_hint, sizeof with_hint, t.text), sizeof with_hint, "\n");
    t.text = str_cat(with_hint, sizeof with_hint, L_keypad_menu_hint);
  }
  ui_tooltip(&t, b->x + 11, b->y, false);
}

/* while building, the mobile version's continuous tooltip at the top: what is being built, its cost */
static void builder_tooltip(void) {
  if (cu.build_kind == BK_NONE || nwins) return;
  Tip t = {0};
  char head[96];
  float cost[M_COUNT];
  build_cost(cost);
  t.cost = cost;
  if (cu.build_kind == BK_BUILDING) {
    t.header = lo_1(head, sizeof head, L_building_a_building, LS(cls_text[cu.build_cls].name));
    t.text = L_keypad_builder_hint;   /* (the keypad's keys, where the original needs none) */
  } else if (cu.build_kind == BK_DECORATION) {
    const DecoText *d = &deco_text[cu.build_cls];
    t.header = d->flags & DT_REMOVE ? d->name : lo_1(head, sizeof head, L_placing_a_decoration, LS(d->name));
    t.text = d->desc;
  } else if (cu.build_kind == BK_RESOURCE) {
    const ResText *r = &res_text[cu.build_cls];
    t.header = lo_1(head, sizeof head, pcls[r->cls].base & B_FOREST ? L_planting_a_forest : L_placing_a_decoration, LS(r->name));
    t.text = r->desc;
  } else {
    /* moving alien ruins (the original's simple window at the top) */
    t.cost = NULL;
    t.text = L_move_ruins_howto;
  }
  ui_tooltip(&t, 0, 0, true);
}

static void bmenu_draw(bool tip) {
  BMenu m;
  bmenu_layout(&m);
  int focus = cu.zone == Z_BUILD && !nwins ? cu.bfocus : -2;
  if (!tip) {
    for (int i = 0; i < m.n; i++) draw_bbutton(&m.b[i], i == focus);
    /* the keypad's focus: the window buttons' green frame, over the button's edge */
    if (focus >= 0 && focus < m.n) ui_ring(m.b[focus].x + 2, m.b[focus].y + 2, m.b[focus].w - 4, 18, 2, FOCUS_RING);
  } else if (focus >= 0 && focus < m.n) {
    bbutton_tooltip(&m.b[focus]);
  }
}

/* a build menu button for the tutorial arrows */
bool city_build_rect(int which, int16_t r[4]) {
  BMenu m;
  bmenu_layout(&m);
  int pick = -1;
  if (which == BR_FIRST_CATEGORY) {
    for (int i = 0; i < m.nrow && pick < 0; i++)
      if (m.b[i].kind == BB_CATEGORY) pick = i;
  } else if (which == BR_LAST_BUILDING || which == BR_SECOND_LAST_BUILDING) {
    /* the last button of the category's first row (the latest added) */
    int best = -1;
    for (int i = m.nrow; i < m.n; i++)
      if (m.b[i].y == m.at_width && (best < 0 || m.b[i].x > m.b[best].x)) best = i;
    pick = best;
  } else if (which == BR_INVITE || which == BR_MANAGEMENT) {
    for (int i = 0; i < m.n && pick < 0; i++)
      if (m.b[i].kind == (which == BR_INVITE ? BB_INVITE : BB_MANAGEMENT)) pick = i;
  }
  if (pick < 0) return false;
  r[0] = m.b[pick].x, r[1] = m.b[pick].y, r[2] = m.b[pick].w, r[3] = 22;
  return true;
}

/* ---------------------------------------------------------------- the build menu's keys */

/* ---------------------------------------------------------------- the build menu on a keypad
 * The column at the left works as tabs: the focus on one shows its row (a category's buildings, the building
 * modes, the management options). toolbox opens the menu on a category's buildings, left and right go along the
 * row, up and down to the next tab (its row shown at once), OK picks. */

/* the column's button of the row shown (its tab), -1 none */
static int bmenu_tab(const BMenu *m) {
  for (int i = 0; i < m->nrow; i++) {
    const BButton *b = &m->b[i];
    if ((b->kind == BB_CATEGORY && b->arg == cu.category && !cu.modes_open && !cu.mgmt_open) ||
        (b->kind == BB_MODE && cu.modes_open) || (b->kind == BB_MANAGEMENT && cu.mgmt_open))
      return i;
  }
  return -1;
}

/* a tab's row shown (a builder stays) */
static void bmenu_show_tab(int kind, int arg) {
  cu.modes_open = cu.mgmt_open = false;
  shown_sub = 0;
  cu.category = -1;
  if (kind == BB_CATEGORY) {
    cu.category = (int8_t)arg, last_category = (int8_t)arg;
    unlocks.notifyCategory &= (uint16_t)~(1 << arg);
  } else if (kind == BB_MODE) {
    cu.modes_open = true;
  } else if (kind == BB_MANAGEMENT) {
    cu.mgmt_open = true;
  }
}

/* the focus into the row shown: the button of what is being built (or was picked last), else the one nearest x on
 * the row's bottom line (x < 0: its first); a row without buttons: its tab */
static void bmenu_focus_row(int x) {
  BMenu m;
  bmenu_layout(&m);
  for (int i = m.nrow; i < m.n; i++)
    if (bb_active(&m.b[i]) && m.b[i].kind != BB_UPGRADES && m.b[i].kind != BB_FIND && m.b[i].kind != BB_INVITE) {
      cu.bfocus = (int8_t)i;
      return;
    }
  if (x < 0)
    for (int i = m.nrow; i < m.n; i++)
      if (m.b[i].kind == last_pick.kind && m.b[i].arg == last_pick.arg && m.b[i].arg2 == last_pick.arg2) {
        cu.bfocus = (int8_t)i;
        return;
      }
  int base = -1, best = -1, bd = 1 << 30;
  for (int i = m.nrow; i < m.n; i++)
    if (m.b[i].y > base) base = m.b[i].y;
  for (int i = m.nrow; i < m.n; i++) {
    if (m.b[i].y != base) continue;
    int d = x < 0 ? m.b[i].x : abs(m.b[i].x + 11 - x);
    if (d < bd) bd = d, best = i;
  }
  cu.bfocus = (int8_t)(best >= 0 ? best : bmenu_tab(&m));
}

/* toolbox: the menu, on the category it showed last (else the first), the focus on its buildings */
static void bmenu_open(void) {
  cu.zone = Z_BUILD;
  if (menu_opened < 255) menu_opened++;
  if (cu.category < 0 && !cu.modes_open && !cu.mgmt_open) {
    BMenu m;
    bmenu_layout(&m);
    int pick = -1;
    for (int i = 0; i < m.nrow; i++)
      if (m.b[i].kind == BB_CATEGORY && (pick < 0 || m.b[i].arg == last_category)) pick = i;
    if (pick >= 0) bmenu_show_tab(BB_CATEGORY, m.b[pick].arg);
  }
  bmenu_focus_row(-1);
}

/* out of the menu: its rows close (a builder keeps its category shown) */
static void bmenu_close(void) {
  if (cu.build_kind == BK_NONE) cu.category = -1;
  cu.modes_open = cu.mgmt_open = false;
  if (cu.build_kind == BK_NONE) shown_sub = 0;
  cu.zone = Z_CITY;
}

/* a button of the row pressed (the tabs are bmenu_input's) */
static void bmenu_press(BMenu *m, int i) {
  BButton *b = &m->b[i];
  switch (b->kind) {
  case BB_MODE_PICK:
    building_mode = (BuildingMode)b->arg;
    if (b->arg == BM_DESTROY || b->arg == BM_DESTROY_LEAVING_HOLE) {
      builder_stop();
      cu.zone = Z_CITY;   /* to pick what to destroy */
    }
    cu.modes_open = false;
    cu.bfocus = -1;
    break;
  case BB_INVITE: win_open_only(WK_INVITE, 0, 0); break;
  case BB_FOLLOW:
    follow_mode = !follow_mode;
    if (follow_mode) {
      builder_stop();
      cu.zone = Z_CITY;
      notify_top(L_tap_near_citizen_to_follow, NULL, 240);
    }
    break;
  case BB_UPGRADES: win_open_only(WK_UPGRADES, 0, 0); break;
  case BB_FIND: win_open_only(WK_FIND, 0, 0); break;
  case BB_SUB_BACK: shown_sub = 0; break;
  case BB_SUB_MIRROR:
    /* the builder stays, mirrored or not */
    cu.mirrored = !cu.mirrored;
    if (cu.build_kind == BK_BUILDING)
      builder_start_ex(BK_BUILDING, cu.build_cls, cu.build_decor, (cls_text[cu.build_cls].flags & CT_MIRRORABLE) && cu.mirrored,
                       cu.build_color);
    break;
  case BB_SUBCATEGORY: {
    color_picked_on_menu_open = color_picked;
    shown_sub = (int8_t)(b->arg + 1);
    /* the subcategory's first button is picked (addBuildingButton): an any-colour class's last colour */
    for (int k = 0; k < nbuild_order; k++) {
      int c = build_order[k];
      if (cls_text[c].sub != shown_sub || unlock_state(c) == UNLOCK_LOCKED) continue;
      const ClsText *ct = &cls_text[c];
      if ((ct->flags & CT_ANYCOLOR) && !(ct->hint && unlock_state(c) == UNLOCK_UNLOCKED))
        builder_start_ex(BK_BUILDING, c, (spr_w(pcls[c].sprite) + 2) / 22 - 1, (ct->flags & CT_MIRRORABLE) && cu.mirrored,
                         color_picked_on_menu_open);
      else
        builder_start(BK_BUILDING, c, 0);
      cu.zone = Z_BUILD;
      break;
    }
    cu.bfocus = (int8_t)i;
    break;
  }
  case BB_ANYCOLOR_LAST:
  case BB_ANYCOLOR_PICK:
  case BB_BUILDING: {
    int k = b->arg;
    const ClsText *ct = &cls_text[k];
    unlocks_stop_notify(k);
    if (ct->hint && unlock_state(k) == UNLOCK_UNLOCKED) {
      /* showSimpleWindow(showUnlockHint) */
      builder_stop();
      win_open_text(NULL, ct->hint);
      break;
    }
    if (b->kind == BB_ANYCOLOR_PICK) {
      /* the colour picker, then a builder of this look in its colour */
      builder_stop();
      color_picker_open(COLOR_FOR_BUILDER, (uint16_t)k);
      break;
    }
    int32_t col = b->kind == BB_ANYCOLOR_LAST ? color_picked_on_menu_open : -1;
    if (cu.build_kind == BK_BUILDING && cu.build_cls == k && cu.build_decor == b->arg2 && cu.build_color == col) {
      builder_stop();   /* createOrRemoveBuilder: the same again removes it */
      break;
    }
    builder_start_ex(BK_BUILDING, k, b->arg2, (ct->flags & CT_MIRRORABLE) && cu.mirrored, col);
    break;
  }
  case BB_DECORATION:
    if (cu.build_kind == BK_DECORATION && cu.build_cls == b->arg) builder_stop();
    else builder_start(BK_DECORATION, b->arg, 0);
    break;
  case BB_RESOURCE:
    if (cu.build_kind == BK_RESOURCE && cu.build_cls == b->arg) builder_stop();
    else builder_start(BK_RESOURCE, b->arg, 0);
    break;
  }
}

static void bmenu_input(void) {
  BMenu m;
  bmenu_layout(&m);
  if (!m.n) {
    cu.zone = Z_CITY;
    return;
  }
  if (cu.bfocus < 0 || cu.bfocus >= m.n) {
    bmenu_focus_row(-1);
    if (cu.bfocus < 0 || cu.bfocus >= m.n) cu.bfocus = 0;
  }
  const BButton f = m.b[cu.bfocus];
  bool in_row = cu.bfocus >= m.nrow;
  int dx = key_repeat(K_RIGHT) ? 1 : key_repeat(K_LEFT) ? -1 : 0;
  int dy = key_repeat(K_DOWN) ? 1 : key_repeat(K_UP) ? -1 : 0;
  if (dx && in_row) {
    /* along the row (its lines read as one), and from its first button to its tab */
    int to = cu.bfocus + dx;
    if (to >= m.nrow && to < m.n) cu.bfocus = (int8_t)to;
    else if (dx < 0 && bmenu_tab(&m) >= 0) cu.bfocus = (int8_t)bmenu_tab(&m);
  } else if (dx > 0) {
    /* from a tab into its row */
    if (bmenu_tab(&m) != cu.bfocus) bmenu_show_tab(f.kind, f.arg);
    bmenu_focus_row(-1);
  } else if (dy) {
    /* another line of the row, else the next tab that way (its row shown at once) */
    int to = -1, best = 1 << 30;
    for (int i = m.nrow; in_row && i < m.n; i++) {
      int ddy = m.b[i].y - f.y, ddx = m.b[i].x - f.x;
      if (ddy * dy <= 0) continue;
      int score = (ddy < 0 ? -ddy : ddy) * 16 + (ddx < 0 ? -ddx : ddx);
      if (score < best) best = score, to = i;
    }
    if (to >= 0) {
      cu.bfocus = (int8_t)to;
    } else {
      int tab = in_row ? bmenu_tab(&m) : cu.bfocus, next = -1;
      for (int i = 0; tab >= 0 && i < m.nrow; i++)
        if (m.b[i].y == m.b[tab].y + 22 * dy) next = i;
      if (next >= 0) {
        bmenu_show_tab(m.b[next].kind, m.b[next].arg);
        if (in_row) bmenu_focus_row(f.x + 11);
        else cu.bfocus = (int8_t)next;   /* (the column doesn't move when the rows change) */
      }
    }
  }
  if (cu.bfocus >= m.nrow) {
    BMenu m2;
    bmenu_layout(&m2);
    if (cu.bfocus < m2.n && (m2.b[cu.bfocus].kind == BB_BUILDING || m2.b[cu.bfocus].kind >= BB_ANYCOLOR_LAST))
      unlocks_stop_notify(m2.b[cu.bfocus].arg);
  }
  if (input.released & K_OK) {
    bmenu_layout(&m);
    if (cu.bfocus < m.nrow) {
      /* a tab: into its row */
      if (bmenu_tab(&m) != cu.bfocus) bmenu_show_tab(m.b[cu.bfocus].kind, m.b[cu.bfocus].arg);
      bmenu_focus_row(-1);
    } else {
      BButton b = m.b[cu.bfocus];
      if (b.kind != BB_MODE_PICK) last_pick.kind = (int8_t)b.kind, last_pick.arg = b.arg, last_pick.arg2 = b.arg2;
      bmenu_press(&m, cu.bfocus);
      /* a building mode picked to build with: back to the buildings */
      if (b.kind == BB_MODE_PICK && cu.zone == Z_BUILD) {
        cu.category = -1;
        bmenu_open();
      }
    }
  }
  if (key_pressed(K_BACK)) bmenu_close();
}

/* ---------------------------------------------------------------- the city panel (CityGUI.createCityInfo) */
typedef struct { int16_t x, y, w, h; } Rect;
typedef struct {
  Rect r[PR_MAT0 + M_COUNT];
  bool shown[PR_MAT0 + M_COUNT];
  uint8_t mats[M_COUNT];
  int nmats;
} Panel;
static const uint8_t mat_order[] = {M_FOOD, M_WOOD, M_STONE, M_KNOWLEDGE, M_MACHINEPARTS, M_REFINEDMETAL, M_COMPUTERCHIPS,
                                    M_GRAPHENE, M_ROCKETFUEL};

static const char *panel_text(int which, char *buf, int size) {
  buf[0] = 0;
  switch (which) {
  case PR_POP: fmt_int(buf, size, ncits); break;
  case PR_HAPPY:
    if (happiness.happiness >= 124.99f) str_cat(buf, size, "xD");
    else if (happiness.happiness >= 99.99f) str_cat(buf, size, ":D");
    else fmt_int(buf, size, (long)happiness.happiness);
    break;
  case PR_HOUSES:
    if (stats.peopleWithHome >= stats.people) fmt_int(buf, size, stats.houseCapacity - stats.peopleWithHome);
    else {
      str_cat(buf, size, "[red]-");
      fmt_int(buf + 6, size - 6, stats.people - stats.peopleWithHome);
    }
    break;
  case PR_WORK:
    fmt_int(buf, size, stats.peopleWithAJob);
    str_cat(buf, size, "/");
    fmt_int(buf + strlen(buf), size - (int)strlen(buf), stats.laborForce);
    break;
  default:
    if (which >= PR_MAT0) {
      int m = which - PR_MAT0;
      if (m == M_FOOD && eating.foodShortage > 0) str_cat(buf, size, "[red]");
      fmt_int(buf + strlen(buf), size - (int)strlen(buf), (long)floor(city.materials.m[m]));
    }
  }
  return buf;
}

static int info_w(int which, int min_w) {
  char t[24];
  uint16_t col;
  const char *s = ui_color_prefix(panel_text(which, t, sizeof t), &col);
  int w = 10 + 1 + ui_text_w(FONT_STANDARD, s);
  return (w > min_w ? w : min_w) + 5;
}

static void panel_layout(Panel *pl) {
  memset(pl, 0, sizeof *pl);
  int right = SCREEN_W, bottom = SCREEN_H;
  if (!cu.panel_hidden) {
    /* the materials' column */
    int mw = 0;
    pl->nmats = 0;
    for (unsigned i = 0; i < sizeof mat_order; i++)
      if (unlocks_material_shown(mat_order[i])) pl->mats[pl->nmats++] = mat_order[i];
    for (int i = 0; i < pl->nmats; i++) {
      char t[24];
      uint16_t col;
      const char *s = ui_color_prefix(panel_text(PR_MAT0 + pl->mats[i], t, sizeof t), &col);
      int w = 10 + 1 + ui_text_w(FONT_STANDARD, s) + 1;
      w = (w > 42 ? w : 42) + 4;
      if (w > mw) mw = w;
    }
    int my = bottom - 14 * pl->nmats;
    for (int i = 0; i < pl->nmats; i++) {
      int k = PR_MAT0 + pl->mats[i];
      pl->r[k] = (Rect){(int16_t)(right - mw), (int16_t)(my + 14 * i), (int16_t)mw, 14};
      pl->shown[k] = true;
    }
    right -= mw;
    /* population and happiness, homes, work */
    int pw = info_w(PR_POP, 23), hw = info_w(PR_HAPPY, 26), ow = info_w(PR_HOUSES, 20), ww = info_w(PR_WORK, 20);
    int sw = pw + hw;
    if (ow > sw) sw = ow;
    if (ww > sw) sw = ww;
    int sy = bottom - 42, sx = right - sw;
    pl->r[PR_POP] = (Rect){(int16_t)sx, (int16_t)sy, (int16_t)pw, 14};
    pl->r[PR_HAPPY] = (Rect){(int16_t)(sx + pw), (int16_t)sy, (int16_t)(sw - pw), 14};
    pl->r[PR_HOUSES] = (Rect){(int16_t)sx, (int16_t)(sy + 14), (int16_t)sw, 14};
    pl->r[PR_WORK] = (Rect){(int16_t)sx, (int16_t)(sy + 28), (int16_t)sw, 14};
    pl->shown[PR_POP] = pl->shown[PR_HAPPY] = pl->shown[PR_HOUSES] = pl->shown[PR_WORK] = true;
    right = sx;
  }
  /* the quick buttons: help, pause, speed, menu and the hide-UI toggle (14 x 14) */
  int k = 0;
  static const uint8_t quick[] = {PR_HELP, PR_PAUSE, PR_SPEED, PR_MENU, PR_HIDE};
  int qx = right - 14 * 5;
  for (k = 0; k < 5; k++) {
    pl->r[quick[k]] = (Rect){(int16_t)(qx + 14 * k), (int16_t)(bottom - 14), 14, 14};
    pl->shown[quick[k]] = quick[k] != PR_HELP || story_has_quick_text();
  }
}

static int panel_left_at(int y0, int y1) {
  Panel pl;
  panel_layout(&pl);
  int left = SCREEN_W;
  for (int k = 0; k < PR_MAT0 + M_COUNT; k++) {
    if (!pl.shown[k] && k != PR_HELP) continue;
    Rect *r = &pl.r[k];
    if (r->y < y1 && r->y + r->h > y0 && r->x < left) left = r->x;
  }
  return left;
}

bool city_panel_rect(int which, int16_t r[4]) {
  Panel pl;
  panel_layout(&pl);
  if (which < 0 || which >= PR_MAT0 + M_COUNT || !pl.shown[which]) return false;
  r[0] = pl.r[which].x, r[1] = pl.r[which].y, r[2] = pl.r[which].w, r[3] = pl.r[which].h;
  return true;
}

/* whether a panel button's window is the one open (its active look) */
static bool panel_active(int which) {
  Win *wn = win_top();
  switch (which) {
  case PR_HELP: return win_is_open(WK_MISSIONS);
  case PR_PAUSE: return city.paused;
  case PR_MENU: return win_is_open(WK_GAME_MENU);
  case PR_POP: return win_is_open(WK_POPULATION);
  case PR_HAPPY: return win_is_open(WK_HAPPINESS);
  case PR_HOUSES: return win_is_open(WK_HOUSES);
  case PR_WORK: return win_is_open(WK_WORKERS);
  default:
    if (which >= PR_MAT0) return wn && wn->kind == WK_MATERIAL && wn->arg == which - PR_MAT0;
  }
  return false;
}

/* InfoBox's downward trend arrow: more used than made lately */
static bool mat_downtrend(int m) {
  double used1 = stats.materialUsed[m][1], prod1 = stats.materialProduction[m][1];
  bool no_arrow = used1 + 0.2 < prod1 || (used1 < 0.2 && m != M_FOOD) || city_day() <= 5;
  if (!no_arrow) {
    double use3 = 0, prod3 = 0;
    for (int d = 1; d <= 3; d++) use3 += stats.materialUsed[m][d], prod3 += stats.materialProduction[m][d];
    no_arrow = use3 + 0.2 <= prod3;
  }
  if (!no_arrow) {
    double prod0 = stats.materialProduction[m][0], cons0 = stats.materialUsed[m][0];
    float hour = city_hour();
    if (hour > 10 && prod0 > cons0 && prod0 * (1.1 + 0.1 * fmax(0, 12.0 - hour)) > cons0) no_arrow = true;
  }
  return !no_arrow;
}

static void panel_draw(void) {
  Panel pl;
  panel_layout(&pl);
  int focus = cu.zone == Z_PANEL && !nwins ? cu.pfocus : -1;
  for (int k = 0; k < PR_MAT0 + M_COUNT; k++) {
    if (!pl.shown[k]) continue;
    Rect *r = &pl.r[k];
    bool f = k == focus, down = (f && key_held(K_OK)) || panel_active(k);
    if (k <= PR_HIDE) {
      ui_frame(SPR_TRANSPARENTBUTTON_INFO_MINI, 14, down ? 2 : f ? 1 : 0, r->x, r->y, 0, 255);
      uint16_t icon = k == PR_HELP ? SPR_ICON_HELP : k == PR_PAUSE ? SPR_ICON_PAUSE : k == PR_SPEED ? speed_icon[cu.speed]
                    : k == PR_MENU ? SPR_ICON_MENU : 0;
      if (icon) ui_spr(icon, r->x + 1, r->y + 1);
      else ui_frame(SPR_HIDEUI, 12, cu.panel_hidden ? 0 : 1, r->x + 1, r->y + 1, 0, 255);
      continue;
    }
    ui_box(r->x, r->y, r->w, r->h, down ? BX_TRANS_DOWN : f ? BX_TRANS_HOVER : BX_TRANS);
    uint16_t icon = k == PR_POP ? SPR_POPULATION : k == PR_HAPPY ? SPR_HAPPINESS : k == PR_HOUSES ? SPR_HOUSING
                  : k == PR_WORK ? SPR_WORK : mat_icon[k - PR_MAT0];
    ui_spr(icon, r->x + 2, r->y + 2);
    if (k >= PR_MAT0 && mat_downtrend(k - PR_MAT0)) ui_spr(SPR_RESOURCE_DOWNTREND, r->x + 2, r->y + 2);
    char t[24];
    uint16_t col = C_BLACK;
    const char *s = ui_color_prefix(panel_text(k, t, sizeof t), &col);
    ui_text(FONT_STANDARD, r->x + 2 + 10 + 1, r->y + 1, s, col);
  }
  if (focus >= 0 && pl.shown[focus]) ui_ring(pl.r[focus].x + 2, pl.r[focus].y + 2, pl.r[focus].w - 4, pl.r[focus].h - 4, 2, FOCUS_RING);
}

/* the focused panel button's tooltip */
static void panel_draw_tip(void) {
  Panel pl;
  panel_layout(&pl);
  int focus = cu.zone == Z_PANEL && !nwins ? cu.pfocus : -1;
  if (focus >= 0 && focus < PR_MAT0 + M_COUNT && pl.shown[focus]) {
    extern void panel_tooltip(int which, int x, int y);
    panel_tooltip(focus, pl.r[focus].x + pl.r[focus].w / 2, pl.r[focus].y);
  }
}

/* the panel's buttons, pressed */
static void panel_press(int k) {
  switch (k) {
  case PR_HELP:
    if (win_is_open(WK_MISSIONS)) win_close();
    else if (story_has_quick_text()) {
      win_open_only(WK_MISSIONS, 0, 0);
      tut_flags |= TF_HELP_USED;
    }
    break;
  case PR_PAUSE:
    city.paused = !city.paused;
    if (!city.paused) ui.paused_for_window = false;
    break;
  case PR_SPEED:
    /* a click toggles 1x and 2x (from the slow speeds, 1x); Ctrl+Click (here shift and OK) the next of the five.
     * The game goes on */
    if (key_held(K_SHIFT)) cu.speed = (uint8_t)((cu.speed + 1) % 5);
    else cu.speed = cu.speed > 1 ? 0 : 2;
    city.simulationSpeed = simulation_speeds[cu.speed];
    city.paused = false;
    tut_flags |= TF_SPEED_USED;
    break;
  case PR_MENU:
    if (win_is_open(WK_GAME_MENU)) win_close();
    else win_open_only(WK_GAME_MENU, 0, 0);
    break;
  case PR_HIDE: cu.panel_hidden = !cu.panel_hidden; break;
  case PR_POP: win_open_only(WK_POPULATION, 0, 0); break;
  case PR_HAPPY: win_open_only(WK_HAPPINESS, 0, 0); break;
  case PR_HOUSES: win_open_only(WK_HOUSES, 0, 0); break;
  case PR_WORK: win_open_only(WK_WORKERS, 0, 0); break;
  default:
    if (k >= PR_MAT0) win_open_only(WK_MATERIAL, (uint16_t)(k - PR_MAT0), 0);
  }
}

static void panel_input(void) {
  Panel pl;
  panel_layout(&pl);
  if (cu.pfocus < 0 || cu.pfocus >= PR_MAT0 + M_COUNT || !pl.shown[cu.pfocus]) cu.pfocus = PR_PAUSE;
  int dx = key_repeat(K_RIGHT) ? 1 : key_repeat(K_LEFT) ? -1 : 0;
  int dy = key_repeat(K_DOWN) ? 1 : key_repeat(K_UP) ? -1 : 0;
  if (dx || dy) {
    Rect *c = &pl.r[cu.pfocus];
    int cx = c->x + c->w / 2, cy = c->y + c->h / 2, best = -1, best_score = 1 << 30;
    for (int k = 0; k < PR_MAT0 + M_COUNT; k++) {
      if (!pl.shown[k] || k == cu.pfocus) continue;
      Rect *r = &pl.r[k];
      int ddx = r->x + r->w / 2 - cx, ddy = r->y + r->h / 2 - cy, score;
      if (dx) {
        if (ddx * dx <= 0) continue;
        score = ddx * dx + (ddy < 0 ? -ddy : ddy) * 3;
      } else {
        if (ddy * dy <= 0) continue;
        score = ddy * dy + (ddx < 0 ? -ddx : ddx) * 3;
      }
      if (score < best_score) best_score = score, best = k;
    }
    if (best >= 0) cu.pfocus = (int8_t)best;
  }
  if (input.released & K_OK) panel_press(cu.pfocus);
  if (key_pressed(K_BACK)) cu.zone = Z_CITY;
}

/* ---------------------------------------------------------------- the city's keys */
/* the arrows move the cursor a cell (the view follows), shift and the arrows the view */
bool city_cursor_keys(void) {
  if (key_held(K_SHIFT)) {
    camera_keys();
    return true;
  }
  int dx = 0, dy = 0;
  if (key_repeat(K_LEFT)) dx -= 20;
  if (key_repeat(K_RIGHT)) dx += 20;
  if (key_repeat(K_UP)) dy -= 20;
  if (key_repeat(K_DOWN)) dy += 20;
  if (dx || dy) {
    cursor_to(cu.cx + dx, cu.cy + dy);
    camera_to_cursor();
  }
  return dx || dy;
}

/* the citizen nearest the cursor (viewActions.showFollow) */
static uint16_t citizen_near_cursor(void) {
  uint16_t best = 0;
  float bd = 30 * 30;
  for (int i = 1; i < MAX_CITS; i++) {
    Cit *c = &cits[i];
    if (!(c->flags & CF_ALIVE)) continue;
    float x, y;
    cit_city_position(c, &x, &y);
    float dx = x - (cu.cx + 10), dy = y - (cu.cy + 10), d = dx * dx + dy * dy;
    if (d < bd) bd = d, best = (uint16_t)i;
  }
  return best;
}

/* Building.onClick (and the destroy modes), Permanent.onClick */
static void city_press(void) {
  if (cu.build_kind != BK_NONE) {
    build_here();
    return;
  }
  if (follow_mode) {
    uint16_t c = citizen_near_cursor();
    if (c) {
      follow_mode = false;
      cu.following = c;
      win_open_only(WK_CITIZEN, c, 0);
    }
    return;
  }
  uint16_t id = cursor_perm();
  if (!id) return;
  Perm *p = P(id);
  if ((building_mode == BM_DESTROY || building_mode == BM_DESTROY_LEAVING_HOLE) && is_building(p)) {
    if (story.disableDestroy) {
      win_open_text(NULL, L_no_destroy_allowed);
      return;
    }
    extern void try_destroy(uint16_t id);
    try_destroy(id);
    return;
  }
  cu.selected = id;
  win_open_only(WK_BUILDING, id, 0);
}

void city_input(void) {
  cu.gui_timer += TIMEMOD;
  if (build_flash) build_flash--;
  if (key_pressed(K_ALPHA)) {
    /* the original's space bar */
    city.paused = !city.paused;
    if (!city.paused) ui.paused_for_window = false;
  }
  zoom_keys();
  if (key_pressed(K_TOOLBOX)) {
    if (cu.zone == Z_BUILD) bmenu_close();
    else bmenu_open();
    return;
  }
  if (key_pressed(K_VAR)) {
    cu.zone = cu.zone == Z_PANEL ? Z_CITY : Z_PANEL;
    return;
  }
  if (cu.zone == Z_BUILD) {
    bmenu_input();
    return;
  }
  if (cu.zone == Z_PANEL) {
    panel_input();
    return;
  }
  int16_t from_x = cu.cx, from_y = cu.cy;
  city_cursor_keys();
  if (cu.build_kind == BK_BUILDING && building_mode_can_drag()) {
    /* the drag modes: OK builds at once, and while held, on every cell the cursor moves over */
    static bool dragging;
    if (key_pressed(K_OK)) dragging = true, build_here();
    else if (dragging && key_held(K_OK) && (cu.cx != from_x || cu.cy != from_y)) build_line(from_x, from_y, cu.cx, cu.cy);
    if (!key_held(K_OK)) dragging = false;
  } else if (input.released & K_OK) {
    city_press();
  }
  if (key_pressed(K_BACKSPACE)) {
    /* the destroy button of the building under the cursor, already asking "Really Destroy?" */
    uint16_t id = cursor_perm();
    if (is_building(P(id))) {
      if (story.disableDestroy) {
        win_open_text(NULL, L_no_destroy_allowed);
      } else {
        cu.selected = id;
        win_open_only(WK_BUILDING, id, WF2_CONFIRM);
      }
    }
  }
  if (key_pressed(K_BACK)) {
    /* stop building, leave a destroy mode or follow mode, else the game menu (onEscapePressWithoutWindow) */
    if (cu.build_kind != BK_NONE) {
      /* the builder goes, and the menu comes back on its button, to pick another */
      bool menu = cu.build_kind != BK_MOVE;
      builder_stop();
      if (menu) bmenu_open();
    } else if (follow_mode) follow_mode = false;
    else if (building_mode == BM_DESTROY || building_mode == BM_DESTROY_LEAVING_HOLE) building_mode = BM_NORMAL;
    else win_open_only(WK_GAME_MENU, 0, 0);
  }
}

void city_draw_ui(void) {
  ui_noclip();
  bmenu_draw(false);
  panel_draw();
}

/* the tooltips (the original's tooltip stage: over the windows and the help arrows) */
void city_draw_tips(void) {
  ui_noclip();
  panel_draw_tip();
  bmenu_draw(true);
  if (cu.zone == Z_CITY) builder_tooltip();
}
