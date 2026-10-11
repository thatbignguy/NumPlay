/* The Final Earth 2: the buildings' drawers (buildings.buildingDrawers.*):
 * a building's back on LAYER_BG and front on LAYER_MAIN, merged with its
 * neighbours' where the drawer does that, its upgrades' sprites, and colour
 * helpers for the light shows. Everything is drawn into the band with
 * gfx_band_sprite, positions scaled by the band's zoom. */
#include "bldi.h"

/* ---------------------------------------------------------------- basics */
#ifdef HOST
#include <stdio.h>
#include <stdlib.h>
int bld_bad_draws;   /* (tests: parts asked for outside their sprite) */
#endif
void dr_part(const GfxBand *b, uint16_t spr, int sx, int sy, int w, int h, int x, int y, int dx, int dy, uint8_t flags,
             uint8_t alpha, uint16_t tint) {
  if (!spr || !alpha) return;
#ifdef HOST
  if (sx < 0 || sy < 0 || w <= 0 || h <= 0 || sx + w > spr_w(spr) || sy + h > spr_h(spr)) {
    bld_bad_draws++;
    if (getenv("BLD_DEBUG")) printf("bad part: spr %d (%dx%d) %d,%d %dx%d\n", spr, spr_w(spr), spr_h(spr), sx, sy, w, h);
  }
#endif
  gfx_band_sprite(b, spr, sx, sy, w, h, x + dx * b->zoom, y + dy * b->zoom, flags, alpha, tint);
}
/* frame i of a strip of frames fw wide, 2 pixels apart (Resources.getTexturesByWidth) */
void dr_frame(const GfxBand *b, uint16_t spr, int fw, int i, int x, int y, int dx, int dy, uint8_t flags, uint8_t alpha,
              uint16_t tint) {
  dr_part(b, spr, (fw + 2) * i, 0, fw, spr_h(spr), x, y, dx, dy, flags, alpha, tint);
}
void dr_sprite(const GfxBand *b, uint16_t spr, int x, int y, int dx, int dy, uint8_t flags, uint8_t alpha) {
  dr_part(b, spr, 0, 0, spr_w(spr), spr_h(spr), x, y, dx, dy, flags, alpha, GFX_NO_TINT);
}
int frames_of(uint16_t spr, int fw) { return spr ? (spr_w(spr) + 2) / (fw + 2) : 0; }

/* thx.color: HSV and HSL to RGB (hue in degrees) */
static uint32_t rgbf(float r, float g, float b) {
  int ri = (int)(r * 255 + 0.5f), gi = (int)(g * 255 + 0.5f), bi = (int)(b * 255 + 0.5f);
  ri = ri < 0 ? 0 : ri > 255 ? 255 : ri, gi = gi < 0 ? 0 : gi > 255 ? 255 : gi, bi = bi < 0 ? 0 : bi > 255 ? 255 : bi;
  return (uint32_t)(ri << 16 | gi << 8 | bi);
}
uint32_t hsv_to_rgb(float h, float s, float v) {
  h = fmodf(h, 360);
  if (h < 0) h += 360;
  float c = v * s, hp = h / 60, x = c * (1 - fabsf(fmodf(hp, 2) - 1)), m = v - c;
  float r = 0, g = 0, b = 0;
  if (hp < 1) r = c, g = x;
  else if (hp < 2) r = x, g = c;
  else if (hp < 3) g = c, b = x;
  else if (hp < 4) g = x, b = c;
  else if (hp < 5) r = x, b = c;
  else r = c, b = x;
  return rgbf(r + m, g + m, b + m);
}
uint32_t hsl_to_rgb(float h, float s, float l) {
  float c = (1 - fabsf(2 * l - 1)) * s;
  float v = l + c / 2;
  return hsv_to_rgb(h, v > 0 ? 2 * (1 - l / v) : 0, v);
}

/* ---------------------------------------------------------------- what each building's drawer shows */
static const uint16_t main_textures[3] = {SPR_BOTANICALGARDENS, SPR_INDOORPARK, SPR_INDOORPARK_ALT1};
static const uint16_t dome_textures[2][3] = {
    {SPR_DOME_BOTANICALGARDEN, SPR_DOME_BOTANICALGARDEN_ALT1, SPR_DOME_BOTANICALGARDEN_ALT2},
    {SPR_DOME_BOTANICALGARDEN_B, SPR_DOME_BOTANICALGARDEN_ALT1B, SPR_DOME_BOTANICALGARDEN_ALT2B}};

void bld_drawer(const Perm *p, Drawer *d) {
  const PermClass *pc = PC(p);
  d->kind = DRW_NORMAL;
  d->name = d->group = pc->sprite;
  if (pc->kind != KIND_BUILDING) {
    d->kind = DRW_NONE;
    return;
  }
  switch (p->cls) {
  case CLS_Park: case CLS_StatueGarden: case CLS_StatueOfHappiness: case CLS_StatueOfTheKey:
    d->kind = DRW_AUTOMERGE;
    d->name = d->group = main_textures[STC(ParkState, p)->mainTexture % 3];
    break;
  case CLS_ParkPod:
    d->kind = DRW_AUTOMERGE;
    d->name = d->group = main_textures[STC(ParkPodState, p)->mainTexture % 3];
    break;
  case CLS_Library: case CLS_Restaurant: case CLS_NightClub: case CLS_ScrapyardNightClub: case CLS_FactoryPub:
  case CLS_HoloGameHall: case CLS_TreeHuggerBase:
    d->kind = DRW_AUTOMERGE;
    break;
  case CLS_BotanicalGardens:
    d->kind = DRW_ALLDIR;
    d->name = d->group = main_textures[STC(GardenState, p)->mainTexture % 3];
    break;
  case CLS_BotanicalGardensDome: {
    const GardenState *g = STC(GardenState, p);
    d->kind = DRW_ROOFDOWN;
    d->name = dome_textures[g->domeVersion & 1][g->mainTexture % 3];
    d->group = main_textures[g->mainTexture % 3];
    break;
  }
  case CLS_BlossomRestaurant: case CLS_BlossomHut:
    d->kind = DRW_ALLDIR;
    break;
  case CLS_HerbGarden:
    d->kind = DRW_DIFFTYPE;
    if (p->mode == MODE_HerbGarden_MedicalHerbs) d->name = SPR_HERBGARDEN_MEDIC, d->group = SPR_BOTANICALGARDENS;
    else d->name = SPR_HERBGARDEN, d->group = SPR_RESTAURANT;
    break;
  case CLS_RooftopPark:
    d->kind = DRW_ROOFTOP_AM;
    break;
  case CLS_ColoredBlock: case CLS_RooftopDecoration: case CLS_RooftopDecoration2: case CLS_RooftopDecoration3:
  case CLS_RooftopDecoration4: case CLS_RedwoodTree:
    d->kind = DRW_DECORATION;
    break;
  case CLS_MechanicalHouse: d->name = d->group = mechanical_house_texture(p); break;
  case CLS_Mansion: d->name = d->group = mansion_texture(p); break;
  case CLS_GlassPiece: d->name = d->group = STC(DecorState, p)->look ? SPR_TREEPLANTATION : SPR_INDOORFARM; break;
  default: break;
  }
}

static bool merges_h(int kind) {
  return kind == DRW_AUTOMERGE || kind == DRW_ALLDIR || kind == DRW_DIFFTYPE || kind == DRW_ROOFTOP_AM || kind == DRW_ROOFDOWN;
}
static bool merges_v(int kind) { return kind == DRW_ALLDIR || kind == DRW_ROOFDOWN; }
/* isConnectedBuilding: the other merges (both ways if both is set) and shows the same texture or group */
static bool connected(const Drawer *me, const Perm *o, bool both) {
  if (!is_building(o)) return false;
  Drawer od;
  bld_drawer(o, &od);
  if (!merges_h(od.kind) || (both && !merges_v(od.kind))) return false;
  return od.name == me->name || od.group == me->group;
}
static bool same_texture(const Drawer *me, const Perm *o) {
  if (!is_building(o)) return false;
  Drawer od;
  bld_drawer(o, &od);
  return merges_h(od.kind) && od.name == me->name;
}

/* NormalBuildingDrawer: the 64x20 sheet's front (0), front with door (22) and back (44) */
void draw_normal(const GfxBand *b, const Perm *p, int layer, int x, int y, uint16_t spr, bool mirrored) {
  uint8_t f = mirrored ? GFX_FLIPX : 0;
  if (layer == LAYER_BG) dr_part(b, spr, 44, 0, 20, 20, x, y, 0, 0, f, 255, GFX_NO_TINT);
  else if (layer == LAYER_MAIN) dr_part(b, spr, p->row == 0 ? 22 : 0, 0, 20, 20, x, y, 0, 0, f, 255, GFX_NO_TINT);
}
/* CustomizableBuildingDrawer: a front without a door and a back of their own (0: the standard frames) */
void draw_custom(const GfxBand *b, const Perm *p, int layer, int x, int y, uint16_t spr, bool mirrored,
                 uint16_t front_spr, int front_frame, uint16_t back_spr, int back_frame) {
  uint8_t f = mirrored ? GFX_FLIPX : 0;
  if (layer == LAYER_BG) {
    if (back_spr) dr_frame(b, back_spr, 20, back_frame, x, y, 0, 0, f, 255, GFX_NO_TINT);
    else dr_part(b, spr, 44, 0, 20, 20, x, y, 0, 0, f, 255, GFX_NO_TINT);
  } else if (layer == LAYER_MAIN) {
    if (p->row != 0 && front_spr) dr_frame(b, front_spr, 20, front_frame, x, y, 0, 0, f, 255, GFX_NO_TINT);
    else dr_part(b, spr, p->row == 0 ? 22 : 0, 0, 20, 20, x, y, 0, 0, f, 255, GFX_NO_TINT);
  }
}
/* DecorationBuildingDrawer: sub-image n at x 22n, on the main stage */
void draw_decoration(const GfxBand *b, const Perm *p, int layer, int x, int y, uint16_t spr, int sub, bool mirrored,
                     uint16_t tint) {
  (void)p;
  if (layer == LAYER_MAIN) dr_part(b, spr, sub * 22, 0, 20, 20, x, y, 0, 0, mirrored ? GFX_FLIPX : 0, 255, tint);
}

/* the half frames of a striped sheet (getTexturesByWidth(name, 10, true)) */
static inline int half_x(int i) { return 11 * i - (i & 1); }
static void half(const GfxBand *b, uint16_t spr, int i, int row, int h, int x, int y, int dx, int dy) {
  dr_part(b, spr, half_x(i), row * h, 10, h, x, y, dx, dy, 0, 255, GFX_NO_TINT);
}

/* the secondary background's row: alone, a garden below only, both, a garden above only */
void draw_garden_plants(const GfxBand *b, const Perm *p, int x, int y, uint16_t spr, int texture, bool top, bool bottom) {
  (void)p;
  int row = (bottom ? 1 : 0) + (top ? 3 : 0) - (top && bottom ? 2 : 0);
  dr_part(b, spr, 20 * texture, 20 * row, 20, 20, x, y, 0, 0, 0, 255, GFX_NO_TINT);
}

/* AutoMerging (and its rooftop and different-type kinds), AllDir and RooftopDown drawers */
void draw_merging(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  if (layer != LAYER_BG && layer != LAYER_MAIN) return;
  Drawer d;
  bld_drawer(p, &d);
  bool door = p->row == 0;
  bool bg = layer == LAYER_BG;
  if (d.kind == DRW_AUTOMERGE || d.kind == DRW_ROOFTOP_AM || d.kind == DRW_DIFFTYPE) {
    const Perm *l = P(p->left), *r = P(p->right);
    if (d.kind == DRW_ROOFTOP_AM) {
      /* AutoMergingBuildingDrawerRooftop: on the ground, the buildings 20 pixels aside; else beside the one below */
      const Perm *bot = P(p->bottom);
      if (!bot) {
        l = P(permanent_at_pos(perm_city_x(p) - 20, perm_city_y(p)));
        r = P(permanent_at_pos(perm_city_x(p) + 20, perm_city_y(p)));
        if (!is_building(l)) l = NULL;
        if (!is_building(r)) r = NULL;
      } else {
        l = P(bot->left) ? P(P(bot->left)->top) : NULL;
        r = P(bot->right) ? P(P(bot->right)->top) : NULL;
      }
    }
    bool hl = connected(&d, l, false), hr = connected(&d, r, false);
    int el = 0, er = 0;
    if (d.kind == DRW_DIFFTYPE) {
      el = hl && !same_texture(&d, l) ? 6 : 0;
      er = hr && !same_texture(&d, r) ? 6 : 0;
    }
    if (bg) {
      half(b, d.name, 4 + (hl ? 6 : 0) + el, 0, 20, x, y, 0, 0);
      half(b, d.name, 5 + (hr ? 6 : 0) + er, 0, 20, x, y, 10, 0);
    } else {
      half(b, d.name, (door ? 2 : 0) + (hl ? 6 : 0) + el, 0, 20, x, y, 0, 0);
      half(b, d.name, 1 + (door ? 2 : 0) + (hr ? 6 : 0) + er, 0, 20, x, y, 10, 0);
    }
    return;
  }
  if (d.kind == DRW_ROOFDOWN) {
    /* RooftopDownMergingDrawer: one 20x20 frame of six */
    bool hb = connected(&d, P(p->bottom), true);
    if (bg) dr_frame(b, d.name, 20, hb ? 5 : 2, x, y, 0, 0, 0, 255, GFX_NO_TINT);
    else dr_frame(b, d.name, 20, (hb ? 3 : 0) + (door ? 1 : 0), x, y, 0, 0, 0, 255, GFX_NO_TINT);
    return;
  }
  if (d.kind != DRW_ALLDIR) return;
  const Perm *l = P(p->left), *r = P(p->right), *t = P(p->top), *bo = P(p->bottom);
  bool hl = connected(&d, l, true), hr = connected(&d, r, true), ht = connected(&d, t, true), hb = connected(&d, bo, true);
  bool htl = hl && ht && connected(&d, P(t->left), true);
  bool htr = hr && ht && connected(&d, P(t->right), true);
  bool hbl = hl && hb && connected(&d, P(l->bottom), true);
  bool hbr = hr && hb && connected(&d, P(r->bottom), true);
  hl = hl || connected(&d, l, false);
  hr = hr || connected(&d, r, false);
  int ktl = (hl ? 1 : 0) + (ht ? 2 : 0) + (htl ? 1 : 0), ktr = (hr ? 1 : 0) + (ht ? 2 : 0) + (htr ? 1 : 0);
  int kbl = (hl ? 1 : 0) + (hb ? 2 : 0) + (hbl ? 1 : 0), kbr = (hr ? 1 : 0) + (hb ? 2 : 0) + (hbr ? 1 : 0);
  uint16_t set = 0;   /* setForegroundTextures / setBackgroundTextures (grids of 10 half columns) */
  if (bg) {
    if (p->cls == CLS_BlossomHut) set = SPR_BLOOMHOUSE;
    if (set) {
      half(b, set, 2 * ktl, 0, 10, x, y, 0, 0);
      half(b, set, 1 + 2 * ktr, 0, 10, x, y, 10, 0);
      half(b, set, 2 * kbl, 1, 10, x, y, 0, 10);
      half(b, set, 1 + 2 * kbr, 1, 10, x, y, 10, 10);
    } else {
      half(b, d.name, 6 * ktl + 4, 0, 10, x, y, 0, 0);
      half(b, d.name, 1 + 6 * ktr + 4, 0, 10, x, y, 10, 0);
      half(b, d.name, 6 * kbl + 4, 1, 10, x, y, 0, 10);
      half(b, d.name, 1 + 6 * kbr + 4, 1, 10, x, y, 10, 10);
    }
    if (p->cls == CLS_BotanicalGardens)
      draw_garden_plants(b, p, x, y, SPR_BOTANICALGARDENS_PLANTS, STC(GardenState, p)->texture, ht, hb);
  } else {
    if (p->cls == CLS_BlossomRestaurant && !door) set = SPR_BLOOMRESTAURANT_FOREGROUND;
    if (set) {
      half(b, set, 2 * ktl, 0, 10, x, y, 0, 0);
      half(b, set, 1 + 2 * ktr, 0, 10, x, y, 10, 0);
      half(b, set, 2 * kbl, 1, 10, x, y, 0, 10);
      half(b, set, 1 + 2 * kbr, 1, 10, x, y, 10, 10);
    } else {
      int dd = door ? 2 : 0;
      half(b, d.name, 6 * ktl + dd, 0, 10, x, y, 0, 0);
      half(b, d.name, 1 + 6 * ktr + dd, 0, 10, x, y, 10, 0);
      half(b, d.name, 6 * kbl + dd, 1, 10, x, y, 0, 10);
      half(b, d.name, 1 + 6 * kbr + dd, 1, 10, x, y, 10, 10);
    }
  }
}

/* SmartLights: CityTime.getCurrentColor over its keys (linear in RGB) */
static uint16_t smart_light_tint(void) {
  static const float t[7] = {0, 4, 6, 19, 21, 23, 24};
  static const uint32_t c[7] = {0xE00000, 0xE00000, 0xFFFFFF, 0xFFFFFF, 0xFFD547, 0xFFD547, 0xE00000};
  float h = hour_now();
  int i = 0;
  while (i < 5 && h >= t[i + 1]) i++;
  float f = (h - t[i]) / (t[i + 1] - t[i]);
  uint32_t a = c[i], bb = c[i + 1], out = 0;
  for (int s = 16; s >= 0; s -= 8) {
    float v = (float)(a >> s & 255) + ((float)(bb >> s & 255) - (float)(a >> s & 255)) * f;
    out |= (uint32_t)(int)(v + 0.5f) << s;
  }
  return rgb565(out);
}

/* the building's upgrades' sprites (and its mode's) on this layer: cached ones, or the others after them */
void draw_upgrades(const GfxBand *b, const Perm *p, int layer, int x, int y, bool no_cache) {
  if (p->cls >= CLS_COUNT) return;
  const ClassUpgrades *cu = &class_upgrades[p->cls];
  for (int k = 0; k < 4; k++) {
    int u;
    if (k < 3) {
      if (!cu->up[k] || !(p->upgrades >> k & 1)) continue;
      u = cu->up[k] - 1;
    } else {
      if (!cu->mode[0] || p->mode >= 4 || !cu->mode[p->mode]) continue;
      u = cu->mode[p->mode] - 1;
    }
    const UpgradeInfo *ui = &upgrade_info[u];
    if (!ui->sprite || ui->layer != layer || (ui->noCache != 0) != no_cache) continue;
    uint16_t tint = u == U_SmartLights ? smart_light_tint() : GFX_NO_TINT;
    dr_frame(b, ui->sprite, 20, upgrade_look(p, u), x, y, 0, 0, 0, 255, tint);
  }
}
