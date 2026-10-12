/* NumPowder: the menu sections and their tools (SimulationData's LoadMenus, GameModel::InitTools and BuildMenus)
 * and the material menu (toolbox): the sections by name with their icons, over the sim's right side; the section's
 * tool buttons (ToolButton, GameView::NotifyActiveMenuToolListChanged) and their scroll bar below the sim; the
 * selected one's description (the original's tooltip) over the sim's bottom left. */
#include "ui.h"
#include <string.h>

/* LoadMenus: the sections' icons (glyphs U+E0xx) and names */
const uint8_t section_icons[SC_TOTAL] = {0x41, 0x42, 0x56, 0x19, 0x62, 0x43, 0x45, 0x44,
                                         0x50, 0x51, 0x46, 0x4C, 0x52, 0x57, 0x67, 0x64};
const char *const section_names[SC_TOTAL] = {
  "Walls", "Electronics", "Powered Materials", "Sensors", "Force", "Explosives", "Gases", "Liquids",
  "Powders", "Solids", "Radioactive", "Special", "Game Of Life", "Tools", "Favorites", "Decoration tools"};

/* ---------------------------------------------------------------- the tools of a section, in the original's order:
 * the elements by number (NONE first, in Special), the Game of Life types, the walls, the simulation tools, SMPL */
static int menu_walk(int section, int want) {
  int n = 0;
  if (section == SC_FAVORITES) {
    for (int k = 0; k < ui.nfav; k++, n++)
      if (n == want) return ui.fav[k];
    return want < 0 ? n : -1;
  }
  for (int t = 0; t < PT_COUNT; t++)
    if (elements[t].Enabled && elements[t].MenuVisible && elements[t].MenuSection == section) {
      if (n == want) return t;
      n++;
    }
  if (section == SC_LIFE)
    for (int g = 0; g < NGOL; g++, n++)
      if (n == want) return PMAP(g, PT_LIFE);
  if (section == SC_WALL)
    for (int w = 0; w < UI_WALLCOUNT; w++, n++)
      if (n == want) return TOOL_WALLS + w;
  if (section == SC_TOOL) {
    for (int k = 0; k < TOOL_COUNT; k++, n++)
      if (n == want) return TOOL_SIMTOOLS + k;
    if (n == want) return TOOL_SAMPLE;
    n++;
  }
  return want < 0 ? n : -1;
}
int menu_count(int section) { return menu_walk(section, -1); }
int menu_tool(int section, int k) { return k < 0 ? -1 : menu_walk(section, k); }
int menu_find(int section, int tool) {
  int n = menu_count(section);
  for (int k = 0; k < n; k++)
    if (menu_tool(section, k) == tool) return k;
  return -1;
}
int tool_section(int tool) {
  if (tool & TOOL_WALLS) return SC_WALL;
  if (tool & (TOOL_SIMTOOLS | TOOL_UI)) return SC_TOOL;
  if (TYP(tool) == PT_LIFE) return SC_LIFE;
  return elements[TYP(tool)].MenuSection;
}
const char *tool_name(int tool) {
  if (tool & TOOL_WALLS) return walltypes[tool & 0xFF].name;
  if (tool & TOOL_SIMTOOLS) return simtools[tool & 0xFF].name;
  if (tool == TOOL_SAMPLE) return "SMPL";
  if (TYP(tool) == PT_LIFE && ID(tool) < NGOL) return builtinGol[ID(tool)].name;
  return elements[TYP(tool)].Name;
}
const char *tool_description(int tool) {
  if (tool & TOOL_WALLS) return walltypes[tool & 0xFF].descs;
  if (tool & TOOL_SIMTOOLS) return simtools[tool & 0xFF].description;
  if (tool == TOOL_SAMPLE) return "Sample an element on the screen.";
  if (TYP(tool) == PT_LIFE && ID(tool) < NGOL) return builtinGol[ID(tool)].description;
  return elements[TYP(tool)].Description;
}
uint32_t tool_colour(int tool) {
  if (tool & TOOL_WALLS) return walltypes[tool & 0xFF].colour;
  if (tool & TOOL_SIMTOOLS) return simtools[tool & 0xFF].colour;
  if (tool == TOOL_SAMPLE) return 0x000000;
  if (TYP(tool) == PT_LIFE && ID(tool) < NGOL) return builtinGol[ID(tool)].colour;
  return elements[TYP(tool)].Colour;
}

/* ---------------------------------------------------------------- the buttons' look, drawn straight into a band */
static void b_rect(Band *b, int x, int y, int w, int h, uint32_t c) {
  band_fill(b, x, y, w, 1, c, 255);
  band_fill(b, x, y + h - 1, w, 1, c, 255);
  band_fill(b, x, y + 1, 1, h - 2, c, 255);
  band_fill(b, x + w - 1, y + 1, 1, h - 2, c, 255);
}
/* the selection: a thick yellow frame, outlined in black so it shows over anything */
static void b_select(Band *b, int x, int y, int w, int h) {
  b_rect(b, x - 1, y - 1, w + 2, h + 2, 0xFFFF00);
  b_rect(b, x - 2, y - 2, w + 4, h + 4, 0xFFFF00);
  b_rect(b, x - 3, y - 3, w + 6, h + 6, 0x000000);
}
static inline bool b_misses(Band *b, int y, int h) { return y >= b->y0 + b->h || y + h <= b->y0; }

/* Renderer::WallIcon's patterns, 26 x 14 */
static void wall_icon(Band *b, int x0, int y0, int wall) {
  const WallType *wt = &walltypes[wall];
  int w = 26, h = 14;
  if (wall == WL_ERASEALL) {
    /* (stripes of changing colours) */
    int r = 100, g = 150, bl = 50, rd = 1, gd = -1, bd = -1;
    for (int x = 0; x < w; x++) {
      r += 15 * rd, g += 15 * gd, bl += 15 * bd;
      if (r > 200) rd = -1;
      if (g > 200) gd = -1;
      if (bl > 200) bd = -1;
      if (r < 15) rd = 1;
      if (g < 15) gd = 1;
      if (bl < 15) bd = 1;
      int rc = r < 0 ? 0 : r > 150 ? 150 : r, gc = g < 0 ? 0 : g > 200 ? 200 : g, bc = bl < 0 ? 0 : bl > 200 ? 200 : bl;
      band_fill(b, x0 + x, y0, 1, h, (uint32_t)rc << 16 | gc << 8 | bc, 255);
    }
    font_draw_char(b, x0 + w / 2 - 10, y0 + h / 2 - 2, FONT_ICON(0x6C), 0xFF0000, 255, FONT_BLEND);
    font_draw_char(b, x0 + w / 2 + 1, y0 + h / 2 - 2, FONT_ICON(0x6C), 0xFF0000, 255, FONT_BLEND);
    return;
  }
  for (int y = 0; y < h; y++) {
    if (y0 + y < b->y0 || y0 + y >= b->y0 + b->h) continue;
    for (int x = 0; x < w; x++) {
      int32_t c = -1;
      switch (wt->drawstyle) {
        case 1: if (~y & ~(x ^ (y >> 1)) & 1) c = (int32_t)wt->colour; break;
        case 2: if (~y & ~x & 1) c = (int32_t)wt->colour; break;
        case 3: c = (int32_t)wt->colour; break;
        case 4: {
          int d = ((x - y) % 4 + 4) % 4;
          c = d == 0 ? (int32_t)wt->colour : d == 1 ? (int32_t)wt->eglow : 0x202020;
          break;
        }
      }
      switch (wall) {
        case WL_EWALL: if ((x < w / 4 + y) != (x & y & 1)) c = (int32_t)wt->colour; break;
        case WL_WALLELEC: c = (~y & ~x & 1) ? (int32_t)wt->colour : 0x808080; break;
        case WL_EHOLE:
        case WL_STASIS: if ((x < w / 4 + y) == (x & y & 1)) c = (int32_t)wt->colour; break;
        case WL_ERASE: if ((x < w / 2) ? ~y & ~(x ^ (y >> 1)) & 1 : 1) c = (int32_t)wt->colour; break;
        case WL_STREAM: if (x == 0 || y == 0 || x == w - 1 || y == h - 1) c = 0xA0A0A0; break;
      }
      if (c >= 0) band_fill(b, x0 + x, y0 + y, 1, 1, (uint32_t)c, 255);
    }
  }
  if (wall == WL_ERASE) font_draw_char(b, x0 + w / 2 - 4, y0 + h / 2 - 2, FONT_ICON(0x6C), 0xFF0000, 255, FONT_BLEND);
  if (wall == WL_STREAM) {
    font_draw_char(b, x0 + 4, y0 + 2, FONT_ICON(0x0D), 0xFFFFFF, 255, FONT_ADD);
    font_draw_char(b, x0 + 8, y0 + 2, FONT_ICON(0x6D), 0xFFFFFF, 255, FONT_ADD);
  }
}

/* ToolButton::Draw: 30 x 18, the tool's colour (or icon) inside, its name, the border showing which side has it
 * (red the left tool, blue the right one) */
static void tool_button_band(Band *b, int x, int y, int tool, int side) {
  int w = 30, h = 18;
  if (b_misses(b, y - 3, h + 6) || x >= b->x0 + b->w || x + w <= b->x0) return;
  uint32_t colour = tool_colour(tool);
  bool icon = true;
  if (tool & TOOL_WALLS) {
    wall_icon(b, x + 2, y + 2, tool & 0xFF);
  } else if (tool == TOOL_SAMPLE) {
    b_rect(b, x + 2, y + 2, 26, 14, 0xA0A0A0);
    font_draw_char(b, x + 2 + 13 - 5, y + 2 + 7 - 5, FONT_ICON(0x66), 0xFFFFFF, 255, FONT_BLEND);
    font_draw_char(b, x + 2 + 13 - 5, y + 2 + 7 - 5, FONT_ICON(0x6B), 0x64B4FF, 255, FONT_BLEND);
  } else if (tool == PT_NONE) {
    /* NONE's icon: U+E06C in red */
    font_draw_char(b, x + 2 + 13 - 4, y + 2 + 7 - 2, FONT_ICON(0x6C), 0xFF0000, 255, FONT_BLEND);
  } else {
    band_fill(b, x + 2, y + 2, w - 4, h - 4, colour, 255);
    icon = false;
  }
  b_rect(b, x, y, w, h, side == 0 ? 0xFF0000 : side == 1 ? 0x0000FF : 0x000000);
  for (int k = 0; k < ui.nfav; k++)
    if (ui.fav[k] == tool) font_draw_char(b, x, y, FONT_ICON(0x68), 0xFFFF00, 255, FONT_BLEND);
  if (!icon) {
    const char *name = tool_name(tool);
    char txt[8];
    int n = 0;
    while (name[n] && n < 7) txt[n] = name[n], n++;   /* (ButtonText.Substr(0, 7)) */
    txt[n] = 0;
    int r = colour >> 16 & 255, g = colour >> 8 & 255, bl = colour & 255;
    int total = bl + 3 * g + 2 * r;
    /* Component::TextPosition, centred within its margins (1 above and below, 4 at the sides) */
    int tw = font_text_width(txt) - 1, tx = 4 + ((w - 8) - tw) / 2, ty = 1 + ((h - 2) - (FONT_H - 2 - 3)) / 2;
    font_draw(b, x + tx, y + ty, txt, total < 544 ? 0xFFFFFF : 0x000000, 255, FONT_BLEND);
  }
}
/* ---------------------------------------------------------------- the material menu */
static uint8_t section_sel[SC_TOTAL];   /* each section's last selected tool */
#define BTN_X0 (SCREEN_W - 6 - 30)   /* the first tool's button, at the right as the original's */
#define BTN_Y (PANEL_Y + 2)
#define BTN_STEP 31
#define CAT_X MENU_LIST_X             /* the sections' list, over the sim's right side */
#define CAT_W (SCREEN_W - CAT_X)
#define CAT_ROW 12

static void menu_scroll_to(int k) {
  int x = BTN_X0 - BTN_STEP * k + ui.scroll;
  int n = menu_count(ui.section);
  int lo = k + 1 < n ? BTN_STEP / 2 : 1;   /* (some of the next one shows) */
  if (x < lo) ui.scroll += lo - x;
  if (x > BTN_X0) ui.scroll -= x - BTN_X0;
  if (ui.scroll < 0) ui.scroll = 0;
}

/* the row scrolled to show a tool of its section (after sampling) */
void menu_show_tool(int tool) {
  int k = menu_find(ui.section, tool);
  ui.scroll = 0;
  if (k >= 0) menu_scroll_to(k);
}

/* (opening: on what OK holds, in its section) */
void menu_open(void) {
  ui.mode = MODE_MENU;
  if (menu_find(ui.section, ui.tool[0]) < 0 && ui.tool[0] != PT_NONE) ui.section = (uint8_t)tool_section(ui.tool[0]);
  int k = menu_find(ui.section, ui.tool[0]);
  if (k < 0) k = section_sel[ui.section];
  int n = menu_count(ui.section);
  ui.sel = n ? (k < n ? k : n - 1) : 0;
  ui.scroll = 0;
  menu_scroll_to(ui.sel);
}

static void menu_section(int d) {
  section_sel[ui.section] = (uint8_t)ui.sel;
  ui.section = (uint8_t)((ui.section + d + SC_TOTAL) % SC_TOTAL);
  int n = menu_count(ui.section), k = menu_find(ui.section, ui.tool[0]);
  if (k < 0) k = section_sel[ui.section];
  ui.sel = n ? (k < n ? k : n - 1) : 0;
  ui.scroll = 0;
  menu_scroll_to(ui.sel);
}

static void fav_toggle(int tool) {
  for (int k = 0; k < ui.nfav; k++)
    if (ui.fav[k] == tool) {
      memmove(&ui.fav[k], &ui.fav[k + 1], sizeof(int) * (size_t)(ui.nfav - k - 1));
      ui.nfav--;
      ui_message("Removed from favorites");
      return;
    }
  if (ui.nfav >= NFAV) {
    ui_message("Favorites are full");
    return;
  }
  ui.fav[ui.nfav++] = tool;
  ui_message("Added to favorites");
}

void menu_keys(void) {
  int n = menu_count(ui.section);
  if (key_pressed(K_BACK) || key_pressed(K_TOOLBOX)) {
    section_sel[ui.section] = (uint8_t)ui.sel;
    ui.mode = MODE_SIM;
    return;
  }
  if (key_pressed(K_VAR)) {
    section_sel[ui.section] = (uint8_t)ui.sel;
    options_open();
    return;
  }
  if (key_repeat(K_UP)) menu_section(-1);
  if (key_repeat(K_DOWN)) menu_section(1);
  n = menu_count(ui.section);
  if (n) {
    /* (the first tool is at the right, as the original's; with shift, a row's worth of buttons at a time, up to
     * the ends) */
    int page = SCREEN_W / BTN_STEP;
    if (key_repeat(K_LEFT)) ui.sel = key_held(K_SHIFT) ? imin(ui.sel + page, n - 1) : (ui.sel + 1) % n;
    if (key_repeat(K_RIGHT)) ui.sel = key_held(K_SHIFT) ? imax(ui.sel - page, 0) : (ui.sel + n - 1) % n;
    menu_scroll_to(ui.sel);
    int tool = menu_tool(ui.section, ui.sel);
    if (key_pressed(K_OK)) {
      /* (OK: for OK; shift+OK: for Back) */
      int side = key_held(K_SHIFT) ? 1 : 0;
      ui.tool[side] = tool;
      section_sel[ui.section] = (uint8_t)ui.sel;
      ui.mode = MODE_SIM;
    }
    if (key_pressed(K_BACKSPACE) && ui.section != SC_FAVORITES) fav_toggle(tool);
    else if (key_pressed(K_BACKSPACE)) {
      fav_toggle(tool);
      n = menu_count(ui.section);
      if (ui.sel >= n) ui.sel = n ? n - 1 : 0;
    }
  }
}

/* the sections, by name with the original's icons, a row each over the sim's right side: the one shown white (the
 * original's pressed MenuButton) in a thick frame */
static void sections_fn(Band *b, const void *ctx) {
  (void)ctx;
  if (b->y0 >= SIM_H) return;
  for (int s = 0; s < SC_TOTAL; s++) {
    int y = s * CAT_ROW;
    if (b_misses(b, y, CAT_ROW)) continue;
    bool on = s == ui.section;
    band_fill(b, CAT_X, y, CAT_W, CAT_ROW, on ? 0xFFFFFF : 0x000000, 255);
    band_fill(b, CAT_X, y, 1, CAT_ROW, 0xC8C8C8, 255);
    uint8_t g = (uint8_t)FONT_ICON(section_icons[s]);
    uint32_t fg = on ? 0x000000 : 0xFFFFFF;
    font_draw_char(b, CAT_X + 4 + (12 - font_char_width(g)) / 2, y + 1, g, fg, 255, FONT_BLEND);
    font_draw(b, CAT_X + 18, y + 2, section_names[s], fg, 255, FONT_BLEND);
  }
  /* (the frame inside the list: its right side is the screen's edge) */
  int y = ui.section * CAT_ROW;
  b_rect(b, CAT_X + 1, y, CAT_W - 1, CAT_ROW, 0xFFFF00);
  b_rect(b, CAT_X + 2, y + 1, CAT_W - 3, CAT_ROW - 2, 0xFFFF00);
  b_rect(b, CAT_X, y - 1, CAT_W, CAT_ROW + 2, 0x000000);
}

/* the section's tool buttons below the sim, the first at the right (GameView::NotifyActiveMenuToolListChanged) */
static void row_fn(Band *b, const void *ctx) {
  (void)ctx;
  if (b->y0 + b->h <= SIM_H || b_misses(b, BTN_Y - 3, 18 + 6 + 6)) return;
  int n = menu_count(ui.section), scroll = ui.scroll;
  for (int k = 0; k < n; k++) {
    int x = BTN_X0 - BTN_STEP * k + scroll;
    if (x + 30 <= 0) break;
    if (x >= SCREEN_W) continue;
    int tool = menu_tool(ui.section, k);
    tool_button_band(b, x, BTN_Y, tool, tool == ui.tool[0] ? 0 : tool == ui.tool[1] ? 1 : -1);
  }
  if (n) {
    /* the selection's frame: thick, yellow, outlined in black (its top on the panel's first rows) */
    int x = BTN_X0 - BTN_STEP * ui.sel + ui.scroll;
    b_select(b, x, BTN_Y + 1, 30, 17);
    band_fill(b, x - 2, BTN_Y - 2, 34, 3, 0xFFFF00, 255);
  }
  /* the scroll bar (GameView::updateToolButtonScroll), when they don't fit */
  int total = (30 + 1) * n;
  if (total > SCREEN_W - 15) {
    int bar = SCREEN_W - 17, size = bar * bar / total;
    int maxscroll = total - BTN_X0 - 30 > 1 ? total - BTN_X0 - 30 : 1;
    int pos = (SCREEN_W - size) - (int)((long)scroll * (SCREEN_W - size) / maxscroll);
    if (pos < 0) pos = 0;
    band_fill(b, pos, BTN_Y + 21, size - 1, 2, 0xC8C8C8, 255);
  }
}

/* below the sim (with the panel): the buttons */
void menu_row(Overlay *o) {
  if (menu_count(ui.section)) ov_call(o, row_fn, NULL);
  else
    ov_text(o, 8, BTN_Y + 5, ui.section == SC_DECO ? "Decorations aren't kept on the calculator"
                                                  : "No favorites yet: " FONT_DEL " in another section adds one", 0xC0C0C0, 255);
}

/* over the sim (every frame): the sections (only when they change: the screen keeps them), the selected tool's name
 * and description */
void menu_draw(Overlay *o, bool full) {
  if (full) ov_call(o, sections_fn, NULL);
  int n = menu_count(ui.section);
  if (!n) return;
  int tool = menu_tool(ui.section, ui.sel);
  char text[300], wrapped[320];
  char *p = text, *end = text + sizeof text;
  p = text_cat(p, end, "\bo"), p = text_cat(p, end, tool == PT_NONE ? "NONE (erases)" : tool_name(tool));
  p = text_cat(p, end, "\bw  "), p = text_cat(p, end, tool_description(tool));
  p = text_cat(p, end, "\n\bgshift+OK: for Back");
  int w = CAT_X - SIM_X - 10;
  text_wrap(wrapped, sizeof wrapped, text, w - 8);
  int lines = font_text_lines(wrapped), h = lines * FONT_H + 6, y = SIM_H - 3 - h;
  ov_fill(o, SIM_X + 3, y, w, h, 0x000000, 200);
  ov_rect(o, SIM_X + 3, y, w, h, 0x808080, 255);
  ov_text(o, SIM_X + 7, y + 5, wrapped, 0xFFFFFF, 255);
}
