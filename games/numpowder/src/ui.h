/* NumPowder: the interface on the keypad: the original's simulation, element buttons, font and windows, without
 * its desktop chrome. The sim's area at the top; below it, a panel saying what OK and Back draw with, the brush, and
 * a line of the keys that do something now. toolbox opens the materials (the original's menu sections, by name, and
 * their buttons), var a short menu (how to play, keys, saves, the display, the settings).
 *
 * ui.c: the modes and the keys; ui_menu.c: the material menu; ui_draw.c: the panel, the brush, the hover info and
 * the debug HUD, the welcome card; ui_opts.c: the menu, the settings, the windows (confirm, text pages); ui_save.c:
 * the save slots and their copy in a Python script. */
#ifndef NP_UI_H
#define NP_UI_H
#include "input.h"
#include "render.h"

/* the interface's own tools, besides the core's (elements, TOOL_WALLS + WL_*, TOOL_SIMTOOLS + TOOL_*) */
#define TOOL_UI 0x10000000
#define TOOL_SAMPLE (TOOL_UI + 0)   /* SMPL: picks the element under the cursor */
static inline bool tool_is_element(int tool) { return !(tool & (TOOL_WALLS | TOOL_SIMTOOLS | TOOL_UI)); }

enum {
  MODE_SIM,       /* drawing on the sim */
  MODE_SHAPE,     /* a line, box or fill being placed (shift + OK) */
  MODE_STKM,      /* the arrows move the stickmen */
  MODE_MENU,      /* the material menu (toolbox) */
  MODE_OPTIONS,   /* the menu (var) */
  MODE_SETTINGS,  /* its settings */
  MODE_CONFIRM,   /* a question: OK yes, Back no */
  MODE_SAVES,     /* the save slots */
  MODE_TEXT,      /* how to play, keys, credits */
};
enum { SHAPE_LINE, SHAPE_BOX, SHAPE_FILL, NUM_SHAPES };
enum { CONFIRM_CLEAR, CONFIRM_LOAD, CONFIRM_OVERWRITE };
enum { TEXT_HELP, TEXT_KEYS, TEXT_CREDITS };

#define NFAV 12
typedef struct {
  uint8_t mode, prev_mode;
  int cx, cy;                 /* the cursor, in particles */
  int lx, ly;                 /* where the held tool drew last */
  int8_t held;                /* the tool held down: 0 (OK) or 1 (Back), -1 none */
  int tool[2];                /* the left (OK) and right (Back) tools */
  uint8_t section;            /* the element menu's section */
  int sel;                    /* its selected tool */
  int16_t scroll;             /* the element buttons' scroll, pixels */
  uint8_t shape, shape_side;  /* MODE_SHAPE: what, and with which tool */
  int sx, sy;                 /* its start */
  uint8_t opt;                /* MODE_OPTIONS: the selected row */
  uint8_t set;                /* MODE_SETTINGS: the selected row */
  uint8_t confirm;            /* MODE_CONFIRM: the question */
  uint8_t slot, saving;       /* MODE_SAVES: the selected slot; saving or loading */
  uint8_t text;               /* MODE_TEXT: which */
  int16_t text_top;           /* its scroll, or how to play's page */
  bool debug;                 /* the original's HUD: FPS, the particles' count, all of what's under the brush */
  bool step;                  /* a frame to step, paused */
  int16_t intro;              /* the intro text's time left (the original's introText) */
  int16_t tip;                /* a message's time left */
  const char *tip_text;
  int fav[NFAV];              /* the favourites (SC_FAVORITES) */
  uint8_t nfav;
  uint32_t fps100;            /* frames a second, x100 */
} Ui;
extern Ui ui;

void ui_init(void);
bool ui_tick(void);           /* the keys, a frame: false to quit */
int ui_draw(Overlay *o, bool *panel);   /* the width of the sim's rows to show again (panel: all of them, and the panel) */
void ui_message(const char *text);
bool ui_home(void);           /* Home: autosaves; true to quit */

/* ui_menu.c */
int menu_count(int section);
int menu_tool(int section, int k);
int menu_find(int section, int tool);      /* the tool's index in a section, -1 */
int tool_section(int tool);                /* the section a tool is in */
const char *tool_name(int tool);           /* its button's text */
const char *tool_description(int tool);
uint32_t tool_colour(int tool);
void menu_open(void);
void menu_show_tool(int tool);
void menu_keys(void);
void menu_draw(Overlay *o, bool full);     /* over the sim: the sections (when full: they don't change), the description */
#define MENU_LIST_X (SCREEN_W - 112)       /* the sections' list */
void menu_row(Overlay *o);                 /* below it: the section's buttons */
extern const char *const section_names[SC_TOTAL];
extern const uint8_t section_icons[SC_TOTAL];

/* ui_draw.c */
#define PANEL_Y SIM_H              /* the panel below the sim */
#define KEYS_Y (SCREEN_H - 14)      /* its line of keys (the text's y) */
void hud_draw(Overlay *o);
void brush_draw(Overlay *o);
void panel_draw(Overlay *o);               /* what OK and Back hold, or the material menu's buttons; the keys */
void keys_line(Overlay *o, const char *text);
void intro_draw(Overlay *o);
char *text_cat(char *p, const char *end, const char *s);      /* (ends it with a 0) */
char *text_cat_fixed(char *p, const char *end, float v);    /* (2 decimals) */
void stickman_hp(Overlay *o);
void particle_info(char *out, int n, int x, int y, bool line2);
int fmt_fixed(char *out, float v, int decimals);
int fmt_int(char *out, int v);
void text_wrap(char *out, int n, const char *s, int width);
void sample_at(int x, int y);              /* the original's sample: the left tool from what's there */

/* ui_opts.c */
void options_open(void);
void options_keys(void);
void options_draw(Overlay *o);
void settings_keys(void);
void settings_draw(Overlay *o);
void confirm_open(int what);
void confirm_keys(void);
void confirm_draw(Overlay *o);
void text_open(int what);
void text_keys(void);
void text_draw(Overlay *o);
/* the TPT window look: a dark box with a border, its title */
void window_draw(Overlay *o, int x, int y, int w, int h, const char *title);
const char *mode_keys(void);               /* the keys line for the mode */

/* ui_save.c */
#define NSLOTS 4
void saves_open(bool saving);
void saves_keys(void);
void saves_draw(Overlay *o);
enum { SAVE_OK, SAVE_NOROOM, SAVE_UNAVAILABLE };
int save_slot(int slot);                   /* 0 the autosave, 1..NSLOTS: SAVE_* */
bool load_slot(int slot);
bool slot_used(int slot, int *nparts);
void saves_restore(void);                  /* the slots from their copy, after the app was installed again */
void config_save(void);                    /* the settings and the interface's choices */
void config_load(void);
#endif
