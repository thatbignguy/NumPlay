/* The Final Earth 2: decoration (buildings.md 12): painted blocks,
 * decorative windows and the rooftop decorations, their looks, colours and
 * mirroring; and the story's customised buildings (bld_customize). */
#include "bldi.h"
#include "lang_data.h"

static uint16_t decor_sprite(const Perm *p) {
  switch (p->cls) {
  case CLS_ColoredBlock: return SPR_COLOREDBLOCK;
  case CLS_RooftopDecoration: return SPR_SLANTEDROOF;
  case CLS_RooftopDecoration2: return SPR_SLANTEDROOFTOP;
  case CLS_RooftopDecoration3: return SPR_DOMEROOF;
  default: return SPR_ROOFBOTTOM;
  }
}
/* getSubImageNumber: (width + 2) / 22 */
static int sub_images(const Perm *p) { return (spr_w(decor_sprite(p)) + 2) / 22; }

static void decor_init(Perm *p) { memset(p->s, 0, sizeof(DecorState)); }
static uint16_t decor_tint(const DecorState *s) {
  return s->bits & DS_TINTED ? tint565((uint32_t)s->rgb[0] << 16 | (uint32_t)s->rgb[1] << 8 | s->rgb[2]) : GFX_NO_TINT;
}
static void decor_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  const DecorState *s = STC(DecorState, p);
  draw_decoration(b, p, layer, x, y, decor_sprite(p), s->look, s->bits & DS_FLIPPED, decor_tint(s));
}
/* mirror(): the field and the drawer turn, and the door moves */
static void decor_mirror(Perm *p) {
  DecorState *s = ST(DecorState, p);
  s->bits ^= DS_MIRRORED | DS_FLIPPED;
  s->bits = (uint8_t)((s->bits & ~DS_DOOR14) | DS_DOOR_SET | (s->bits & DS_MIRRORED ? DS_DOOR14 : 0));
}
float decoration_door_x(const Perm *p) {
  const DecorState *s = STC(DecorState, p);
  return s->bits & DS_DOOR_SET ? (s->bits & DS_DOOR14 ? 14 : 4) : PC(p)->doorX;
}
/* postCreate: a mirrored one is drawn mirrored; mirror() turns the field back (it is set again after) and so
 * puts the door at 4 (as written). Done as its outcome, so that doing it twice changes nothing. */
void decoration_post_create(Perm *p) {
  if (p->cls != CLS_RooftopDecoration && p->cls != CLS_RooftopDecoration2 && p->cls != CLS_RooftopDecoration3 &&
      p->cls != CLS_RooftopDecoration4)
    return;
  DecorState *s = ST(DecorState, p);
  if (s->bits & DS_MIRRORED) s->bits = (uint8_t)((s->bits & ~DS_DOOR14) | DS_FLIPPED | DS_DOOR_SET);
}
static void decor_window(Perm *p, WinOut *w) {
  const DecorState *s = STC(DecorState, p);
  if (!(s->bits & DS_TINTED)) {
    win_button(w, 0, L_change_building_color, WA_CYCLE_0, true);
  } else {
    /* the user interface's colour picker sets it with bld_customize_color */
    win_button(w, 0, L_change_building_color, WA_UI_COLOR_PICKER, true);
    win_space(w, 4);
    win_button(w, 0, L_duplicate_building, WA_UI_DUPLICATE, true);
  }
  if (p->cls == CLS_RooftopDecoration) win_button(w, 0, L_mirror, WA_MIRROR, true);   /* (only the slanted roof) */
}
static void decor_action(Perm *p, int a) {
  DecorState *s = ST(DecorState, p);
  if (a == WA_CYCLE_0) {
    /* CycleValueButton over getSubImageNumber() - 1 looks (the last one is for any colour) */
    int n = sub_images(p) - 1;
    s->look = (uint8_t)(n > 0 ? (s->look + 1) % n : 0);
  } else if (a == WA_MIRROR) {
    decor_mirror(p);
  }
}
#define DECOR_OPS {.init = decor_init, .draw = decor_draw, .window = decor_window, .window_action = decor_action}
const PermOps ops_ColoredBlock = DECOR_OPS;
const PermOps ops_RooftopDecoration = DECOR_OPS;
const PermOps ops_RooftopDecoration2 = DECOR_OPS;
const PermOps ops_RooftopDecoration3 = DECOR_OPS;
const PermOps ops_RooftopDecoration4 = DECOR_OPS;

/* GlassPiece: an empty indoor farm (0) or tree plantation (1) */
static void glass_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, STC(DecorState, p)->look ? SPR_TREEPLANTATION : SPR_INDOORFARM, false);
}
static void glass_window(Perm *p, WinOut *w) { (void)p, win_button(w, 0, "Change Building Color", WA_CYCLE_0, true); }
static void glass_action(Perm *p, int a) {
  if (a == WA_CYCLE_0) ST(DecorState, p)->look = (uint8_t)((ST(DecorState, p)->look + 1) % 2);
}
const PermOps ops_GlassPiece = {.init = decor_init, .draw = glass_draw, .window = glass_window, .window_action = glass_action};

static bool is_decoration(const Perm *p) {
  return p->cls == CLS_ColoredBlock || p->cls == CLS_GlassPiece || p->cls == CLS_RooftopDecoration ||
         p->cls == CLS_RooftopDecoration2 || p->cls == CLS_RooftopDecoration3 || p->cls == CLS_RooftopDecoration4;
}
void house_mirror(Perm *p);   /* (bld_houses.c: Loft, LandedExplorationShip) */
/* the story's "Class*mirrored.decor" and "Class_mirrored" buildings (Story: customize, or mirror) */
void bld_customize(Perm *p, int decor, bool mirrored) {
  if (!p) return;
  if (is_decoration(p)) {
    DecorState *s = ST(DecorState, p);
    s->look = (uint8_t)(decor < 0 ? 0 : decor);
    /* (ColoredBlock and GlassPiece take no mirroring) */
    if (mirrored && p->cls != CLS_ColoredBlock && p->cls != CLS_GlassPiece) s->bits |= DS_MIRRORED;
    else s->bits &= (uint8_t)~DS_MIRRORED;
    decoration_post_create(p);
    return;
  }
  if (mirrored) house_mirror(p);
}
/* customizeColor: the "any colour" look, tinted (rgb < 0: none) */
void bld_customize_color(Perm *p, int32_t rgb) {
  if (!p || !is_decoration(p) || p->cls == CLS_GlassPiece) return;
  DecorState *s = ST(DecorState, p);
  if (rgb < 0) {
    s->bits &= (uint8_t)~DS_TINTED;
    return;
  }
  s->bits |= DS_TINTED;
  s->rgb[0] = (uint8_t)(rgb >> 16), s->rgb[1] = (uint8_t)(rgb >> 8), s->rgb[2] = (uint8_t)rgb;
}
/* ICustomizableOnBuild.areSameCustomizations: the look (and colour; a slanted roof's mirroring too) the builder
 * would give; true for classes not customised on building */
bool bld_same_customizations(const Perm *p, int decor, bool mirrored, int32_t color) {
  if (!is_decoration(p)) return true;
  const DecorState *s = STC(DecorState, p);
  if (s->look != decor) return false;
  if (p->cls == CLS_GlassPiece) return true;
  if (bld_decoration_color(p) != color) return false;
  return p->cls != CLS_RooftopDecoration || ((s->bits & DS_MIRRORED) != 0) == mirrored;
}
/* the look the Duplicate button builds again (createOrRemoveBuilderForThis): look, mirroring (the roof pieces) */
int bld_decoration_look(const Perm *p, bool *mirrored) {
  *mirrored = false;
  if (!p || !is_decoration(p)) return 0;
  const DecorState *s = STC(DecorState, p);
  *mirrored = p->cls != CLS_ColoredBlock && p->cls != CLS_GlassPiece && (s->bits & DS_MIRRORED);
  return s->look;
}
/* currentRecolor, for the colour picker (-1 none) */
int32_t bld_decoration_color(const Perm *p) {
  if (!p || !is_decoration(p)) return -1;
  const DecorState *s = STC(DecorState, p);
  return s->bits & DS_TINTED ? (int32_t)((uint32_t)s->rgb[0] << 16 | (uint32_t)s->rgb[1] << 8 | s->rgb[2]) : -1;
}
