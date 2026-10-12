/* NumPowder: the menu (var) and its windows, in the original's window look: the menu itself (resume, how to play,
 * keys, save, load, clear, the display preset, the settings, the credits), the settings (the original's Simulation
 * options and quick options), the confirm prompt (ConfirmPrompt) and the text pages; and the line of keys each mode
 * shows below the sim. */
#include "ui.h"
#include <string.h>

/* ui::Window: black, a grey border, its title in the original's InformationTitle colour over a separator */
void window_draw(Overlay *o, int x, int y, int w, int h, const char *title) {
  ov_fill(o, x - 1, y - 1, w + 2, h + 2, 0x000000, 255);
  ov_rect(o, x, y, w, h, 0xC8C8C8, 255);
  if (title) {
    ov_text(o, x + 4, y + 5, title, 0x8C8CFF, 255);
    ov_fill(o, x + 1, y + 18, w - 2, 1, 0xC8C8C8, 255);
  }
}

/* the keys line below the sim, for each mode: the keys that do something now */
const char *mode_keys(void) {
  switch (ui.mode) {
    case MODE_MENU:
      return "\bo" FONT_LEFT " " FONT_RIGHT "\bg: material " FONT_DOT " \bo" FONT_UP " " FONT_DOWN "\bg: category " FONT_DOT
             " \boOK\bg: choose " FONT_DOT " \boBack\bg: close";
    case MODE_SHAPE:
      return "move the other end " FONT_DOT " \boOK\bg: draw " FONT_DOT " \bo" FONT_DEL "\bg: box/fill " FONT_DOT
             " \boBack\bg: cancel";
    case MODE_STKM: return "\boarrows\bg: move the stickman " FONT_DOT " \boBack\bg: brush";
    case MODE_OPTIONS:
    case MODE_SETTINGS:
      return "\bo" FONT_UP " " FONT_DOWN "\bg: choose " FONT_DOT " \bo" FONT_LEFT " " FONT_RIGHT "\bg: change " FONT_DOT
             " \boOK " FONT_DOT " Back\bg: close";
    case MODE_SAVES: return "\bo" FONT_UP " " FONT_DOWN "\bg: choose a slot " FONT_DOT " \boOK " FONT_DOT " Back\bg: close";
    case MODE_CONFIRM: return "\boOK\bg: yes " FONT_DOT " \boBack\bg: no";
    case MODE_TEXT:
      if (ui.text == TEXT_HELP) return "\bo" FONT_LEFT " " FONT_RIGHT "\bg: page " FONT_DOT " \boBack\bg: close";
      if (ui.text == TEXT_KEYS) return "\bo" FONT_UP " " FONT_DOWN "\bg: scroll " FONT_DOT " \boBack\bg: close";
      return "\boBack\bg: close";
  }
  return "\botoolbox\bg: materials " FONT_DOT " \bovar\bg: menu " FONT_DOT " \bo+/-\bg: brush size " FONT_DOT
         " \boalpha\bg: pause";
}

/* ---------------------------------------------------------------- the menu (var) */
enum { MENU_RESUME, MENU_HELP, MENU_KEYS, MENU_SAVE, MENU_LOAD, MENU_CLEAR, MENU_DISPLAY, MENU_SETTINGS, MENU_CREDITS,
       MENU_COUNT };
static const char *const menu_names[MENU_COUNT] = {"Resume", "How to play", "Keys", "Save", "Load", "Clear",
                                                   "Display", "Settings...", "Credits"};
/* the display presets, in the order of the original's render options (its 0-9 keys: Velocity is 1, Alternative
 * Velocity 0, Life 10), then the ones only its keys give */
static const uint8_t preset_order[PRESET_COUNT] = {
  PRESET_VELOCITY, PRESET_PRESSURE, PRESET_PERSISTENT, PRESET_FIRE, PRESET_BLOB, PRESET_HEAT, PRESET_FANCY,
  PRESET_NOTHING, PRESET_HEATGRAD, PRESET_ALTAIR, PRESET_LIFEGRAD, PRESET_DYNHEAT, PRESET_VORTICITY, PRESET_DEST};

void options_open(void) {
  ui.mode = MODE_OPTIONS;
}

static void preset_step(int d) {
  int now = render_preset_now(), at = 3;   /* (fire, the original's default) */
  for (int i = 0; i < PRESET_COUNT; i++)
    if (preset_order[i] == now) at = i;
  at = (at + d + PRESET_COUNT) % PRESET_COUNT;
  render_apply_preset(preset_order[at]);
}

void options_keys(void) {
  if (key_pressed(K_BACK) || key_pressed(K_VAR)) {
    ui.mode = MODE_SIM;
    return;
  }
  if (key_pressed(K_TOOLBOX)) {
    menu_open();
    return;
  }
  if (key_repeat(K_UP)) ui.opt = (uint8_t)((ui.opt + MENU_COUNT - 1) % MENU_COUNT);
  if (key_repeat(K_DOWN)) ui.opt = (uint8_t)((ui.opt + 1) % MENU_COUNT);
  if (ui.opt == MENU_DISPLAY) {
    if (key_repeat(K_LEFT)) preset_step(-1);
    if (key_repeat(K_RIGHT) || key_pressed(K_OK)) preset_step(1);
    return;
  }
  if (!key_pressed(K_OK)) return;
  switch (ui.opt) {
    case MENU_RESUME: ui.mode = MODE_SIM; break;
    case MENU_HELP: text_open(TEXT_HELP); break;
    case MENU_KEYS: text_open(TEXT_KEYS); break;
    case MENU_SAVE: saves_open(true); break;
    case MENU_LOAD: saves_open(false); break;
    case MENU_CLEAR: confirm_open(CONFIRM_CLEAR); break;
    case MENU_SETTINGS: ui.mode = MODE_SETTINGS; break;
    case MENU_CREDITS: text_open(TEXT_CREDITS); break;
  }
}

/* a row's selection: a thick yellow frame */
static void row_frame(Overlay *o, int x, int y, int w, int h) {
  ov_frame(o, x, y, w, h, 2, 0xFFFF00);
}
/* a choice: "< value >", the arrows grey */
static void choice(Overlay *o, int right, int y, const char *v, bool sel) {
  char t[48] = FONT_LEFT " ";
  strcat(t, v);
  strcat(t, " " FONT_RIGHT);
  ov_text(o, right - font_text_width(t), y, t, 0xFFFFFF, sel ? 255 : 200);
}

#define MENU_W 220
#define MENU_ROW 15
#define MENU_H (24 + MENU_COUNT * MENU_ROW + 4)
void options_draw(Overlay *o) {
  int x = SIM_X + (SIM_W - MENU_W) / 2, y = (SIM_H - MENU_H) / 2;
  window_draw(o, x, y, MENU_W, MENU_H, "Menu");
  for (int k = 0; k < MENU_COUNT; k++) {
    int ry = y + 24 + k * MENU_ROW;
    bool sel = k == ui.opt;
    ov_text(o, x + 10, ry + 3, menu_names[k], k == MENU_CLEAR ? 0xFFD820 : 0xFFFFFF, sel ? 255 : 200);
    if (k == MENU_DISPLAY) {
      /* the preset's name, without " Display" */
      int p = render_preset_now();
      char name[40] = "custom";
      if (p >= 0) {
        strcpy(name, render_presets[p].name);
        char *d = strstr(name, " Display");
        if (d) *d = 0;
      }
      choice(o, x + MENU_W - 10, ry + 3, name, sel);
    }
    if (sel) row_frame(o, x + 4, ry, MENU_W - 8, MENU_ROW - 1);
  }
}

/* ---------------------------------------------------------------- the settings */
enum { SET_HEAT, SET_AHEAT, SET_NGRAV, SET_WATER, SET_AIR, SET_GRAVITY, SET_EDGE, SET_GRAVFIELD, SET_STKM, SET_DEBUG,
       SET_COUNT };
static const char *const set_names[SET_COUNT] = {
  "Heat simulation", "Ambient heat", "Newtonian gravity", "Water equalisation", "Air", "Gravity", "Edges",
  "Gravity field lines", "Control the stickman", "Details in the HUD"};
static const char *const set_info[SET_COUNT] = {
  "Can cause odd behaviour when disabled",
  "The air keeps heat: hot things warm it",
  "Heavy particles pull others towards them",
  "Water levels itself out",
  "The air and its pressure, or not",
  "Which way things fall",
  "Solid: things stop at the edges; Void: they fall out",
  "Draws Newtonian gravity's field",
  "The arrows move STKM (shift: STK2)",
  "FPS, particles and all about what's under the brush"};
static const char *const air_names[] = {"On", "Pressure off", "Velocity off", "Off", "No update"};
static const char *const grav_names[] = {"Vertical", "Off", "Radial", "Custom"};
static const char *const edge_names[] = {"Void", "Solid", "Loop"};

static bool set_check(int k) {
  return k == SET_HEAT || k == SET_AHEAT || k == SET_NGRAV || k == SET_WATER || k == SET_GRAVFIELD || k == SET_DEBUG;
}
static bool set_value(int k) {
  switch (k) {
    case SET_HEAT: return settings.heat;
    case SET_AHEAT: return settings.ambient_heat;
    case SET_NGRAV: return settings.newtonian_gravity;
    case SET_WATER: return settings.water_equalization;
    case SET_GRAVFIELD: return ren.grav_field;
    case SET_DEBUG: return ui.debug;
  }
  return false;
}
static void set_change(int k, int d) {
  switch (k) {
    case SET_HEAT: settings.heat = !settings.heat; break;
    case SET_AHEAT: settings.ambient_heat = !settings.ambient_heat; break;
    case SET_NGRAV: settings.newtonian_gravity = !settings.newtonian_gravity; break;
    case SET_WATER: settings.water_equalization = !settings.water_equalization; break;
    case SET_GRAVFIELD: ren.grav_field = !ren.grav_field; break;
    case SET_DEBUG: ui.debug = !ui.debug; break;
    case SET_AIR: settings.air_mode = (uint8_t)((settings.air_mode + d + NUM_AIRMODES) % NUM_AIRMODES); break;
    case SET_GRAVITY: settings.gravity_mode = (uint8_t)((settings.gravity_mode + d + NUM_GRAVMODES) % NUM_GRAVMODES); break;
    case SET_EDGE: settings.edge_mode = (uint8_t)((settings.edge_mode + d + NUM_EDGEMODES) % NUM_EDGEMODES); break;
  }
  sim_settings_changed();
}

void settings_keys(void) {
  if (key_pressed(K_BACK) || key_pressed(K_VAR)) {
    ui.mode = MODE_OPTIONS;
    return;
  }
  if (key_repeat(K_UP)) ui.set = (uint8_t)((ui.set + SET_COUNT - 1) % SET_COUNT);
  if (key_repeat(K_DOWN)) ui.set = (uint8_t)((ui.set + 1) % SET_COUNT);
  int k = ui.set;
  if (k == SET_STKM) {
    if (key_pressed(K_OK)) {
      if (elementCount[PT_STKM] || elementCount[PT_STKM2] || elementCount[PT_SPAWN] || elementCount[PT_SPAWN2]) ui.mode = MODE_STKM;
      else ui_message("Draw STKM first (in Special)");
    }
    return;
  }
  if (key_repeat(K_LEFT)) set_change(k, set_check(k) ? 0 : -1);
  if (key_repeat(K_RIGHT) || key_pressed(K_OK)) set_change(k, 1);
}

#define SET_W 268
#define SET_ROW 14
#define SET_H (24 + SET_COUNT * SET_ROW + 22)
void settings_draw(Overlay *o) {
  int x = SIM_X + (SIM_W - SET_W) / 2, y = (SIM_H - SET_H) / 2;
  window_draw(o, x, y, SET_W, SET_H, "Settings");
  for (int k = 0; k < SET_COUNT; k++) {
    int ry = y + 23 + k * SET_ROW;
    bool sel = k == ui.set;
    if (set_check(k)) {
      /* ui::Checkbox: a 12 x 12 box, filled in the middle when checked */
      ov_rect(o, x + 10, ry + 1, 12, 12, 0xFFFFFF, sel ? 255 : 200);
      if (set_value(k)) ov_fill(o, x + 13, ry + 4, 6, 6, 0xFFFFFF, 255);
      ov_text(o, x + 28, ry + 3, set_names[k], 0xFFFFFF, sel ? 255 : 200);
    } else {
      ov_text(o, x + 10, ry + 3, set_names[k], 0xFFFFFF, sel ? 255 : 200);
      const char *v = k == SET_AIR ? air_names[settings.air_mode % NUM_AIRMODES]
                    : k == SET_GRAVITY ? grav_names[settings.gravity_mode % NUM_GRAVMODES]
                    : k == SET_EDGE ? edge_names[settings.edge_mode % NUM_EDGEMODES] : NULL;
      if (v) choice(o, x + SET_W - 10, ry + 3, v, sel);
      else ov_text(o, x + SET_W - 20, ry + 3, "\x95", 0xC0C0C0, sel ? 255 : 120);   /* (U+E015, an arrow) */
    }
    if (sel) row_frame(o, x + 4, ry, SET_W - 8, SET_ROW - 1);
  }
  /* the selected one's details (the original writes them under each), or the last message */
  ov_fill(o, x + 1, y + SET_H - 18, SET_W - 2, 1, 0xC8C8C8, 255);
  char custom[64];
  const char *info = set_info[ui.set];
  if (ui.set == SET_GRAVITY && settings.gravity_mode == GRAV_CUSTOM) {
    /* (the custom gravity's, as a save gives it: x and y) */
    char *p = custom, *e = custom + sizeof custom;
    p = text_cat(p, e, "Custom gravity: x "), p = text_cat_fixed(p, e, settings.custom_gravity_x);
    p = text_cat(p, e, ", y "), p = text_cat_fixed(p, e, settings.custom_gravity_y);
    info = custom;
  }
  if (ui.tip > 0 && ui.tip_text) ov_text(o, x + 6, y + SET_H - 13, ui.tip_text, 0xFFFFFF, 255);
  else ov_text(o, x + 6, y + SET_H - 13, info, 0xC0C0C0, 255);
}

/* ---------------------------------------------------------------- the confirm prompt */
static const char *const confirm_title[] = {"Clear the simulation", "Load a simulation", "Overwrite a save"};
static const char *const confirm_text[] = {
  "This removes every particle, wall and the air. The autosave and the slots stay.",
  "The simulation on the screen is replaced by the one saved.",
  "The slot already holds a simulation: it's replaced by this one."};
void confirm_open(int what) {
  if (ui.mode != MODE_CONFIRM) ui.prev_mode = ui.mode;
  ui.confirm = (uint8_t)what;
  ui.mode = MODE_CONFIRM;
}
void confirm_keys(void) {
  if (key_pressed(K_BACK)) {
    /* (back to where it was asked from: the saves, or the menu for Clear) */
    ui.mode = ui.prev_mode == MODE_SAVES || ui.prev_mode == MODE_OPTIONS ? ui.prev_mode : MODE_SIM;
    return;
  }
  if (key_pressed(K_OK)) {
    ui.mode = MODE_SIM;
    if (ui.confirm == CONFIRM_CLEAR) {
      sim_clear();
      render_clear_accumulation();
      ui_message("Cleared");
    } else if (ui.confirm == CONFIRM_LOAD) {
      load_slot(ui.slot);
    } else if (ui.confirm == CONFIRM_OVERWRITE) {
      save_slot(ui.slot);
    }
  }
}
void confirm_draw(Overlay *o) {
  char msg[160];
  text_wrap(msg, sizeof msg, confirm_text[ui.confirm], 250 - 16);
  int lines = font_text_lines(msg), w = 250, h = 24 + lines * FONT_H + 6 + 16;
  int x = SIM_X + (SIM_W - w) / 2, y = (SIM_H - h) / 2;
  ov_fill(o, x - 1, y - 1, w + 2, h + 2, 0x000000, 255);
  ov_rect(o, x, y, w, h, 0xC8C8C8, 255);
  ov_text(o, x + 4, y + 8, confirm_title[ui.confirm], 0xFFD820, 255);   /* WarningTitle */
  ov_text(o, x + 8, y + 26, msg, 0xFFFFFF, 255);
  /* Cancel and Continue buttons, their keys */
  int by = y + h - 16;
  ov_rect(o, x, by, w - 75, 16, 0xC8C8C8, 255);
  ov_text(o, x + 4, by + 4, "Cancel \bg(Back)", 0xFFFFFF, 255);
  ov_rect(o, x + w - 76, by, 76, 16, 0xC8C8C8, 255);
  ov_text(o, x + w - 72, by + 4, "Continue \bg(OK)", 0xFFD820, 255);
  ov_frame(o, x + w - 76, by, 76, 16, 2, 0xFFFF00);
}

/* ---------------------------------------------------------------- how to play, keys, credits */
/* how to play: a few pages, the original's element names in its teal (\bt), keys in its orange (\bo) */
static const char *const help_pages[][2] = {
  {"Draw",
   "\botoolbox\bw opens the materials: \bo" FONT_LEFT " " FONT_RIGHT "\bw pick one,\n"
   "\bo" FONT_UP " " FONT_DOWN "\bw change the category, \boOK\bw takes it.\n"
   "\boOK\bw draws it under the brush (the outline),\n"
   "\boBack\bw erases.\n"
   "\boArrows\bw move the brush, \boshift+arrows\bw faster.\n"
   "\bo+\bw and \bo-\bw change its size, \boshift\bw with them its\n"
   "shape.\n"
   "Below the sim: what \boOK\bw and \boBack\bw hold, and the\n"
   "keys that do something right now."},
  {"Things react",
   "\btSAND\bw sinks in \btWATR\bw, and \btOIL\bw floats on it.\n"
   "\btFIRE\bw burns \btWOOD\bw, \btOIL\bw and \btGAS\bw.\n"
   "\btLAVA\bw melts \btICE\bw and boils \btWATR\bw.\n"
   "\btACID\bw eats most things (not \btGLAS\bw).\n"
   "\n"
   "Each material says what it does when you pick\n"
   "it in the materials (\botoolbox\bw).\n"
   "Try things together!"},
  {"Hot and cold",
   "\btHEAT\bw and \btCOOL\bw (in Tools) warm and chill\n"
   "what's under the brush while \boOK\bw is held.\n"
   "Heat \btICE\bw: it melts to \btWATR\bw, then boils to\n"
   "\btWTRV\bw (steam). Cool it to bring it back.\n"
   "\n"
   "Over something, the brush shows its name and\n"
   "temperature at the top right.\n"
   "\bovar\bw, Display: Heat shows every temperature."},
  {"Walls",
   "\btWALL\bw (in Walls) holds everything. Other walls\n"
   "let only air, liquids, powders or gases through.\n"
   "The wall with a red X erases walls.\n"
   "\n"
   "The screen's edges are solid: things stop there.\n"
   "In \bovar\bw, Settings, Edges: Void lets them fall out,\n"
   "Loop brings them back on the other side."},
  {"Electricity",
   "Draw \btMETL\bw, then touch it with \btSPRK\bw: the spark\n"
   "runs along the metal.\n"
   "\btBTRY\bw keeps sparking the metal it touches.\n"
   "\btPSCN\bw turns a \btSWCH\bw (switch) on, \btNSCN\bw off.\n"
   "\btLCRY\bw lights up, \btINST\bw carries a spark at once.\n"
   "\n"
   "They're in Electronics and Powered Materials."},
  {"Explosives",
   "\btGUN\bw (gunpowder), \btNITR\bw and \btC-4\bw blow up when\n"
   "\btFIRE\bw or \btSPRK\bw touches them.\n"
   "\btBOMB\bw blows up what it falls on.\n"
   "\n"
   "Explosions push the air: \bovar\bw, Display: Pressure\n"
   "shows it, red pushing, blue pulling.\n"
   "They're in Explosives."},
  {"Pause, look, keep",
   "\boalpha\bw pauses; \boshift+alpha\bw moves one frame.\n"
   "\bovar\bw opens the menu:\n"
   "  Display changes the look (Fire, Heat, Fancy...),\n"
   "  Save and Load keep four simulations,\n"
   "  Settings has the original's options.\n"
   "\boHome\bw saves and quits: it's all here next time.\n"
   "\n"
   "Keys, in the menu, lists every key."},
};
#define HELP_PAGES (int)(sizeof help_pages / sizeof help_pages[0])

static const char keys_text[] =
  "\boArrows\bg  move the brush (\boshift\bg: 4 at a time)\n"
  "\boOK\bg  draws what OK holds (its border red)\n"
  "\boBack\bg  draws what Back holds (blue; ERASE)\n"
  "\boshift+OK\bg  a line from here (\bo" FONT_DEL "\bg: a box, then\n"
  "    a fill); \boOK\bg draws it, \boBack\bg cancels\n"
  "\boshift+Back\bg  the same, with what Back holds\n"
  "\botoolbox\bg  the materials: \bo" FONT_LEFT " " FONT_RIGHT "\bg one (\boshift\bg: a row\n"
  "    of them), \bo" FONT_UP " " FONT_DOWN "\bg the category, \boOK\bg for OK,\n"
  "    \boshift+OK\bg for Back, \bo" FONT_DEL "\bg a favorite or not\n"
  "\bo+ -\bg  brush size (\boshift\bg: its shape)\n"
  "\bo" FONT_DEL "\bg  OK takes what's under the brush\n"
  "\boalpha\bg  pause, \boshift+alpha\bg one frame\n"
  "\bovar\bg  the menu: help, saves, display, settings\n"
  "\boHome\bg  saves and quits\n"
  "\bgStickmen: draw \boSTKM\bg (in Special); the \boarrows\bg\n"
  "move it (\boshift\bg: STK2), \bodown\bg uses its element,\n"
  "\boBack\bg gives the brush back";
static const char credits_text[] =
  "\bl\bUNumPowder\bU\n"
  "\n"
  "\bwInspired by The Powder Toy, not affiliated with its\n"
  "developers.\n"
  "\n"
  "Elements, font and icons ported from The Powder Toy's\n"
  "source code (GPL-3.0) by Stanislaw K Skowronek, Simon\n"
  "Robertshaw and its contributors.\n"
  "\n"
  "Made by Mason Chen as part of NumPlay.\n"
  "\bghttps://powdertoy.co.uk";
#define TEXT_LINES 11
void text_open(int what) {
  ui.text = (uint8_t)what;
  ui.text_top = 0;
  ui.mode = MODE_TEXT;
}
void text_keys(void) {
  if (ui.text == TEXT_HELP) {
    if (key_repeat(K_RIGHT) || key_pressed(K_OK)) {
      if (ui.text_top + 1 < HELP_PAGES) ui.text_top++;
      else if (key_pressed(K_OK)) ui.mode = MODE_OPTIONS;
    }
    if (key_repeat(K_LEFT) && ui.text_top > 0) ui.text_top--;
  } else if (ui.text == TEXT_KEYS) {
    int lines = font_text_lines(keys_text);
    if (key_repeat(K_DOWN) && ui.text_top < lines - TEXT_LINES) ui.text_top++;
    if (key_repeat(K_UP) && ui.text_top > 0) ui.text_top--;
    if (key_pressed(K_OK)) ui.mode = MODE_OPTIONS;
  } else if (key_pressed(K_OK)) {
    ui.mode = MODE_OPTIONS;
  }
  if (key_pressed(K_BACK) || key_pressed(K_VAR)) ui.mode = MODE_OPTIONS;
}
#define TEXT_X (SIM_X + 6)
#define TEXT_Y 4
#define TEXT_W (SIM_W - 12)
#define TEXT_H 184
void text_draw(Overlay *o) {
  char title[48];
  const char *t;
  if (ui.text == TEXT_HELP) {
    int p = ui.text_top;
    strcpy(title, "How to play: ");
    strcat(title, help_pages[p][0]);
    t = help_pages[p][1];
  } else {
    strcpy(title, ui.text == TEXT_KEYS ? "Keys" : "Credits");
    t = ui.text == TEXT_KEYS ? keys_text : credits_text;
  }
  window_draw(o, TEXT_X, TEXT_Y, TEXT_W, TEXT_H, title);
  for (int k = 0; k < ui.text_top && *t && ui.text != TEXT_HELP; t++)
    if (*t == '\n') k++;
  /* (the lines shown, up to the window's bottom) */
  char buf[700];
  int n = 0, lines = 0;
  while (t[n] && n < (int)sizeof buf - 1) {
    if (t[n] == '\n' && ++lines >= TEXT_LINES) break;
    buf[n] = t[n], n++;
  }
  buf[n] = 0;
  ov_text(o, TEXT_X + 8, TEXT_Y + 27, buf, 0xFFFFFF, 255);
  ov_fill(o, TEXT_X + 1, TEXT_Y + TEXT_H - 18, TEXT_W - 2, 1, 0xC8C8C8, 255);
  if (ui.text == TEXT_HELP) {
    /* the pages: dots, the one shown white */
    char d[4];
    fmt_int(d, ui.text_top + 1);
    char s[24];
    strcpy(s, d), strcat(s, " / "), fmt_int(d, HELP_PAGES), strcat(s, d);
    ov_text(o, TEXT_X + TEXT_W - 8 - font_text_width(s), TEXT_Y + TEXT_H - 13, s, 0xC0C0C0, 255);
    for (int k = 0; k < HELP_PAGES; k++)
      ov_fill(o, TEXT_X + 8 + k * 9, TEXT_Y + TEXT_H - 10, 5, 5, k == ui.text_top ? 0xFFFFFF : 0x606060, 255);
  } else if (ui.text == TEXT_KEYS) {
    int all = font_text_lines(keys_text);
    if (all > TEXT_LINES)
      ov_text(o, TEXT_X + 8, TEXT_Y + TEXT_H - 13, ui.text_top + TEXT_LINES < all ? "\bg" FONT_DOWN " more below" : "\bg" FONT_UP " more above",
              0xC0C0C0, 255);
  }
}
