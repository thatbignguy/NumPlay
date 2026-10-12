/* NumPowder: the interface's modes and keys (the original's GameView and GameController on a keypad): the cursor
 * and the brush, drawing (DrawPoints), lines, boxes and fills (shift-drag, ctrl-drag, flood fill), sampling,
 * pausing, the stickmen, the welcome card, and the menus and windows (ui_menu.c, ui_opts.c, ui_save.c). */
#include "ui.h"
#include <string.h>

Ui ui;
/* the welcome card's clock: when it began to fade (by 60 a second, as the original's introText) */
static uint32_t intro_t0;

void ui_message(const char *text) {
  ui.tip_text = text;
  ui.tip = 120;   /* (the original's infoTipPresence) */
}

void ui_init(void) {
  memset(&ui, 0, sizeof ui);
  ui.cx = XRES / 2, ui.cy = YRES / 2;
  ui.tool[0] = PT_DUST, ui.tool[1] = PT_NONE;   /* (the original's: DUST and NONE) */
  ui.section = SC_POWDERS;
  ui.held = -1;
  ui.intro = 2048;
  brush.rx = brush.ry = 2, brush.shape = BRUSH_CIRCLE;
  /* things stop at the screen's edges (the original's default lets them fall out of it: a bottomless pit on so small
   * a screen); a configuration or an autosave keeps its own */
  settings.edge_mode = EDGE_SOLID;
  sim_settings_changed();
  saves_restore();
  config_load();
  /* the last session's simulation, from the autosave */
  if (slot_used(0, NULL)) load_slot(0);
  ui.tip = 0;
}

bool ui_home(void) {
  static uint32_t failed_at;
  bool ok = save_slot(0) != SAVE_NOROOM;
  if (!ok && (!failed_at || plat_millis() - failed_at > 4000)) {
    /* (the save failed: Home again within a few seconds quits all the same) */
    failed_at = plat_millis();
    ui_message("Not saved: Home again quits anyway");
    return false;
  }
  return true;
}

/* ---------------------------------------------------------------- the cursor */
/* the arrows: a step when pressed, then held a step every frame (shift: 4) */
static void cursor_keys(void) {
  int step = key_held(K_SHIFT) ? 4 : 1;
  static const uint32_t keys[4] = {K_LEFT, K_RIGHT, K_UP, K_DOWN};
  static const int dx[4] = {-1, 1, 0, 0}, dy[4] = {0, 0, -1, 1};
  for (int k = 0; k < 4; k++) {
    int bit = __builtin_ctz(keys[k]);
    int held = input.held_ticks[bit];
    if (key_pressed(keys[k]) || (held > 6)) ui.cx += dx[k] * step, ui.cy += dy[k] * step;
  }
  ui.cx = ui.cx < 0 ? 0 : ui.cx >= XRES ? XRES - 1 : ui.cx;
  ui.cy = ui.cy < 0 ? 0 : ui.cy >= YRES ? YRES - 1 : ui.cy;
}

/* Brush::AdjustSize(delta, logarithmic) */
static void brush_size(int d) {
  int r = brush.rx;
  r += d * (r / 5 > 1 ? r / 5 : 1);
  r = r < 0 ? 0 : r > 40 ? 40 : r;
  brush.rx = brush.ry = r;
}

static bool is_plop(int tool) {
  int t = TYP(tool);
  return tool_is_element(tool) && (t == PT_STKM || t == PT_STKM2 || t == PT_FIGH);
}

/* a tool pressed (GameView's mouse down) */
static void tool_press(int side) {
  int tool = ui.tool[side];
  if (tool == TOOL_SAMPLE) {
    sample_at(ui.cx, ui.cy);
    return;
  }
  if (is_plop(tool)) {
    /* PlopTool: one, where clicked; then the arrows move it */
    sim_tool_points(tool, ui.cx, ui.cy, ui.cx, ui.cy, false);
    if (TYP(tool) != PT_FIGH && (elementCount[PT_STKM] || elementCount[PT_STKM2] || TYP(tool) == PT_STKM || TYP(tool) == PT_STKM2)) {
      ui.mode = MODE_STKM;
      ui_message("Arrows: the stickman. Back: the brush");
    }
    return;
  }
  ui.held = (int8_t)side;
  ui.lx = ui.cx, ui.ly = ui.cy;
  tool_strength = 1.0f;
  sim_tool_points(tool, ui.cx, ui.cy, ui.cx, ui.cy, false);
}

static void sim_keys(void) {
  bool shift = key_held(K_SHIFT);
  if (key_pressed(K_TOOLBOX)) {
    ui.held = -1;
    menu_open();
    return;
  }
  if (key_pressed(K_VAR)) {
    ui.held = -1;
    options_open();
    return;
  }
  cursor_keys();
  /* drawing: OK the left tool, Back the right one; with shift, a line from here */
  bool pressed = false;
  for (int side = 0; side < 2; side++) {
    uint32_t k = side ? K_BACK : K_OK;
    if (key_pressed(k) && ui.held < 0) {
      if (shift && !is_plop(ui.tool[side]) && ui.tool[side] != TOOL_SAMPLE) {
        ui.mode = MODE_SHAPE;
        ui.shape = SHAPE_LINE;
        ui.shape_side = (uint8_t)side;
        ui.sx = ui.cx, ui.sy = ui.cy;
        return;
      }
      tool_press(side);
      pressed = true;
    }
  }
  if (ui.held >= 0 && !pressed) {
    uint32_t k = ui.held ? K_BACK : K_OK;
    if (!key_held(k)) {
      ui.held = -1;
    } else {
      /* held: every frame, from where it drew last (DrawPoints held) */
      sim_tool_points(ui.tool[(int)ui.held], ui.lx, ui.ly, ui.cx, ui.cy, true);
      ui.lx = ui.cx, ui.ly = ui.cy;
    }
  }
  if (key_pressed(K_ALPHA)) {
    if (shift) {
      sys_pause = true;
      ui.step = true;
    } else {
      sys_pause = !sys_pause;
      ui_message(sys_pause ? "Paused" : "Resumed");
    }
  }
  if (key_repeat(K_PLUS)) {
    if (shift) brush.shape = (uint8_t)((brush.shape + 1) % NUM_BRUSHES);
    else brush_size(1);
  }
  if (key_repeat(K_MINUS)) {
    if (shift) brush.shape = (uint8_t)((brush.shape + NUM_BRUSHES - 1) % NUM_BRUSHES);
    else brush_size(-1);
  }
  if (key_pressed(K_BACKSPACE)) sample_at(ui.cx, ui.cy);
}

static void shape_keys(void) {
  cursor_keys();
  /* (backspace cycles the shapes: shift is the cursor's speed, as everywhere) */
  if (key_pressed(K_BACKSPACE)) ui.shape = (uint8_t)((ui.shape + 1) % NUM_SHAPES);
  if (key_pressed(K_BACK) || key_pressed(K_TOOLBOX) || key_pressed(K_VAR)) {
    /* (the shape cancelled; toolbox and var still open their menus) */
    ui.mode = MODE_SIM;
    if (key_pressed(K_TOOLBOX)) menu_open();
    else if (key_pressed(K_VAR)) options_open();
    return;
  }
  if (key_pressed(K_OK)) {
    int tool = ui.tool[ui.shape_side];
    tool_strength = 1.0f;
    if (ui.shape == SHAPE_LINE) sim_tool_line(tool, ui.sx, ui.sy, ui.cx, ui.cy);
    else if (ui.shape == SHAPE_BOX) sim_tool_box(tool, ui.sx, ui.sy, ui.cx, ui.cy);
    else sim_tool_fill(tool, ui.cx, ui.cy);
    ui.mode = MODE_SIM;
  }
}

/* the stickmen (GameController::KeyPress/KeyRelease's arrows): shift held for the second one */
static uint8_t stkm_down[2];
static void stkm_keys(void) {
  static const uint32_t keys[4] = {K_LEFT, K_RIGHT, K_UP, K_DOWN};
  static const int cmds[4] = {STKM_LEFT, STKM_RIGHT, STKM_UP, STKM_DOWN};
  /* (kept while a stickman or a spawn point exists: dying, it comes back at its SPWN the next frame) */
  bool leave = key_pressed(K_BACK) || key_pressed(K_TOOLBOX) || key_pressed(K_VAR) ||
               !(elementCount[PT_STKM] || elementCount[PT_STKM2] || elementCount[PT_SPAWN] || elementCount[PT_SPAWN2]);
  int who = key_held(K_SHIFT) ? 1 : 0;
  for (int k = 0; k < 4; k++) {
    for (int w = 0; w < 2; w++) {
      bool want = !leave && w == who && key_held(keys[k]);
      bool was = stkm_down[w] & (1 << k);
      if (want != was) {
        stkm_key(w, cmds[k], want);
        stkm_down[w] = (uint8_t)(want ? stkm_down[w] | (1 << k) : stkm_down[w] & ~(1 << k));
      }
    }
  }
  if (key_pressed(K_ALPHA)) sys_pause = !sys_pause;
  if (leave) {
    ui.mode = MODE_SIM;
    if (key_pressed(K_TOOLBOX)) menu_open();
    if (key_pressed(K_VAR)) options_open();
  }
}

bool ui_tick(void) {
  if (key_pressed(K_HOME)) return !ui_home();
  if (ui.intro > 0) {
    /* the welcome card stays until a key (but Home), which does nothing else; then it fades out in under a second */
    uint32_t now = plat_millis();
    if (ui.intro > 50) {
      if (!input.pressed) return true;
      intro_t0 = now;
      ui.intro = 50;
      return true;
    }
    int32_t left = 50 - (int32_t)((now - intro_t0) * 60 / 1000);
    ui.intro = (int16_t)(left > 0 ? left : 0);
  }
  switch (ui.mode) {
    case MODE_SIM: sim_keys(); break;
    case MODE_SHAPE: shape_keys(); break;
    case MODE_STKM: stkm_keys(); break;
    case MODE_MENU: menu_keys(); break;
    case MODE_OPTIONS: options_keys(); break;
    case MODE_SETTINGS: settings_keys(); break;
    case MODE_CONFIRM: confirm_keys(); break;
    case MODE_SAVES: saves_keys(); break;
    case MODE_TEXT: text_keys(); break;
  }
  if (ui.mode != MODE_SIM) ui.held = -1;
  if (ui.tip > 0) ui.tip--;
  return true;
}

/* ---------------------------------------------------------------- drawing */
/* what the panel below the sim shows: drawn again only when it changes */
static uint32_t panel_sig(void) {
  uint32_t h = 2166136261u;
#define MIX(v) (h = (h ^ (uint32_t)(v)) * 16777619u)
  MIX(ui.mode), MIX(ui.section), MIX(ui.sel), MIX(ui.scroll), MIX(ui.text), MIX(ui.shape);
  MIX(ui.tool[0]), MIX(ui.tool[1]), MIX(sys_pause), MIX(brush.rx), MIX(brush.ry), MIX(brush.shape);
  MIX(ui.nfav), MIX(ui.intro > 0 ? ui.intro : 0);
  for (int k = 0; k < ui.nfav; k++) MIX(ui.fav[k]);
#undef MIX
  return h;
}

int ui_draw(Overlay *o, bool *panel) {
  static uint32_t last_sig;
  uint32_t sig = panel_sig();
  if (sig != last_sig || ui.intro > 0) *panel = true;
  last_sig = sig;
  bool sim_view = (ui.mode == MODE_SIM || ui.mode == MODE_SHAPE || ui.mode == MODE_MENU) && ui.intro <= 0;
  if (sim_view) brush_draw(o);
  if ((sim_view || ui.mode == MODE_STKM) && ui.intro <= 0) stickman_hp(o);
  if (ui.intro < 51) hud_draw(o);
  switch (ui.mode) {
    case MODE_MENU: menu_draw(o, *panel); break;
    case MODE_OPTIONS: options_draw(o); break;
    case MODE_SETTINGS: settings_draw(o); break;
    case MODE_CONFIRM: confirm_draw(o); break;
    case MODE_SAVES: saves_draw(o); break;
    case MODE_TEXT: text_draw(o); break;
  }
  /* the info tip (GameView's infoTip): in the middle, fading; not over a window's text (the settings show it on
   * their details' line, the others just below the sim) */
  if (ui.tip > 0 && ui.tip_text && ui.mode != MODE_SETTINGS) {
    int a = (ui.tip > 50 ? 50 : ui.tip) * 5;
    bool window = ui.mode == MODE_SAVES || ui.mode == MODE_CONFIRM || ui.mode == MODE_TEXT || ui.mode == MODE_OPTIONS;
    int tw = font_text_width(ui.tip_text) - 1;
    if (window) ov_text_outline(o, SIM_X + (SIM_W - tw) / 2, SIM_H - 14, ui.tip_text, 0xFFFFFF, a);
    else ov_text_outline(o, SIM_X + (SIM_W - tw) / 2, SIM_H / 2 - 2, ui.tip_text, 0xFFFFFF, a);
  }
  if (*panel) panel_draw(o);
  if (ui.intro > 0) intro_draw(o);
  /* (the material menu's list of sections stays on the screen as it is until the panel changes) */
  return ui.mode == MODE_MENU ? MENU_LIST_X : SCREEN_W;
}
