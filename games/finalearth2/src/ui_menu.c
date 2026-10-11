/* The Final Earth 2: the title screen and the menus (the original's
 * MainMenu, gui.MainMenuGUI, gui.FreePlayConfigureWindow, gui.CreditsWindow,
 * gui.GameMenu), saves (City.handleAutoSave) and the progress kept between
 * cities (MetaGame: the scenarios won, in a small file of its own). */
#include <math.h>
#include "game.h"
#include "save.h"
#include "uii.h"
#include "view.h"

Meta meta;
#define META_FILE "fe2meta.dat"
static bool meta_unlocked_all;   /* MetaGame.unlockedAll */

void meta_load(void) {
  uint32_t len;
  const uint8_t *d = plat_load(META_FILE, &len);
  memset(&meta, 0, sizeof meta);
  meta_unlocked_all = false;
  if (!d || len < 1 + 16 + 1) return;
  meta.won = d[0];
  memcpy(meta.best, d + 1, 16);
  meta_unlocked_all = d[17] != 0;
}

void meta_save(void) {
  uint8_t d[18];
  d[0] = meta.won;
  memcpy(d + 1, meta.best, 16);
  d[17] = meta_unlocked_all;
  plat_save(META_FILE, d, sizeof d);
  save_copy();
}

/* progress.StoryLoader.hasCompletedRequirements: each scenario needs the one before it won */
bool story_requirements(int s) {
  if (s <= 0 || s >= 4) return true;   /* the first scenario and the free play maps */
  return meta_unlocked_all || (meta.won >> (s - 1) & 1);
}

/* ---------------------------------------------------------------- saving */
bool ui_save(bool tell) {
  if (ui.title) return true;
  saved_view.cx = cu.cx, saved_view.cy = cu.cy;   /* (the cursor is kept with the view) */
  bool ok = save_game();
  ui.seconds_since_save = 0;
  if (!ok) {
    win_close_all();
    win_open_text(L_keypad_save_failed_title, L_keypad_save_failed);
  } else if (tell) {
    notify_top(L_keypad_saved, NULL, 120);
  }
  return ok;
}

/* Home: saves, then the game quits. A failed save says so and the game goes on; Home again in that window quits
 * without saving */
bool ui_home(void) {
  if (ui.title || (ui.home_unsaved && win_is_open(WK_TEXT))) return true;
  saved_view.cx = cu.cx, saved_view.cy = cu.cy;
  if (save_game()) return true;
  ui_text_buf[0] = 0;
  str_cat(str_cat(str_cat(ui_text_buf, sizeof ui_text_buf, L_keypad_save_failed), sizeof ui_text_buf, " "), sizeof ui_text_buf,
          L_keypad_home_quit);
  win_close_all();
  win_open_text(L_keypad_save_failed_title, ui_text_buf);
  ui.home_unsaved = true;
  return false;
}

/* handleAutoSave: the original saves every two seconds; here, at the start of every other day */
void autosave_tick(void) {
  static int last_day;
  ui.seconds_since_save += 1.0f / FPS;
  int day = city_day();
  if (ui.title) {
    last_day = day;
    return;
  }
  if (day != last_day) {
    if (day % 2 == 0) ui_save(false);
    last_day = day;
  }
}

/* ---------------------------------------------------------------- starting a city */
static void start_story(int s) {
  ui.title = false;
  ui_before_city();
  game_start(s, NULL);
  ui_after_load();
  ui_save(false);
}

/* createNewGameState: free play maps get their options first (the map shows behind) */
void menu_new_city(int s) {
  win_close_all();
  if (stories[s].flags & ST_FREEPLAY) {
    ui.title = true;   /* not playing yet: no panel (the welcome goal waits under the options) */
    ui.setup = true;
    ui_before_city();
    game_start(s, NULL);
    ui_after_load();
    win_open(WK_FREE_PLAY, (uint16_t)s, (uint16_t)ncits);
    return;
  }
  start_story(s);
}

/* ---------------------------------------------------------------- the scenarios (MainMenuGUI.createNewScenarioWindow) */
static bool any_locked(void) {
  for (int s = 1; s < 4; s++)
    if (!story_requirements(s)) return true;
  return false;
}

static void add_story(WinOut *w, int s) {
  char desc[400], t[64];
  desc[0] = 0;
  if (s < 4 && (meta.won >> s & 1)) {
    int time = meta.best[s];
    str_cat(desc, sizeof desc, lo_3(t, sizeof t, L_best_time, LN(time / 1440), LN(time / 60 % 24), LN(time % 60)));
    str_cat(desc, sizeof desc, " \n");
  }
  str_cat(desc, sizeof desc, stories[s].description);
  /* "Free Play - " left out of the name */
  const char *name = stories[s].name;
  if (!strncmp(name, "Free Play - ", 12)) name += 12;
  win_activatable(w, false, 10 + s, name, desc, NULL, NULL);
}

void menu_scenarios_content(WinOut *w) {
  win_title(w, L_start_scenario);
  for (int s = 0; s < nstories; s++)
    if (!(stories[s].flags & ST_FREEPLAY) && story_requirements(s)) add_story(w, s);
  if (any_locked()) {
    /* the "Unlock All" button: a ContainerButton (padding 3, 3, 3, 1) with a text */
    win_text_button(w, L_unlock_all, 1, BX_BUTTON, true, false);
    w->y += 1;
  }
  win_space(w, 6);
  char t[32] = "";
  str_cat(str_cat(t, sizeof t, L_free_play), sizeof t, ":");
  win_para(w, FONT_STANDARD, t, 250, C_BLACK);
  for (int s = 0; s < nstories; s++)
    if ((stories[s].flags & ST_FREEPLAY) && story_requirements(s)) add_story(w, s);
  win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
}

void menu_scenarios_action(Win *wn, int a) {
  (void)wn;
  if (a == 1) {
    meta_unlocked_all = true;
    meta_save();
  } else if (a >= 10 && a < 10 + nstories) {
    menu_new_city(a - 10);
  }
}

/* ---------------------------------------------------------------- free play (FreePlayConfigureWindow) */
static void set_citizens(int n) {
  int w = 0;
  for (int i = 1; i < MAX_CITS; i++)
    if (cits[i].flags & CF_ALIVE) {
      w = cits[i].onWorld;
      break;
    }
  for (int i = MAX_CITS - 1; i >= 1 && ncits > n; i--)
    if (cits[i].flags & CF_ALIVE) {
      cit_try_remove(&cits[i], true);
      city.materials.m[M_FOOD] -= 3;
    }
  while (ncits < n) {
    if (!cit_new(w, (float)rnd_int(worlds[w].w - 2), rnd_range(18, 40), NONE)) break;
    city.materials.m[M_FOOD] += 3;
  }
}

void menu_free_play_content(WinOut *w) {
  Win *wn = w->win;
  int s = wn->arg, standard = wn->arg2;
  win_title(w, L_customize_free_play);
  win_space(w, 2);
  win_para(w, FONT_15, L_starting_resources, 250, C_BLACK);
  bool extra = city.materials.m[M_WOOD] >= 1000;
  win_text_button(w, L_normal_resources, 1, BX_ACTIVABLE | (!extra ? BXF_ACTIVE : 0), true, false);
  if (!(stories[s].flags & ST_NO_EXTRA_RESOURCES)) {
    win_space(w, 2);
    win_text_button(w, L_extra_resources, 2, BX_ACTIVABLE | (extra ? BXF_ACTIVE : 0), true, false);
  }
  win_space(w, 6);
  win_para(w, FONT_15, L_starting_citizens, 250, C_BLACK);
  win_row_begin(w);
  const int counts[] = {standard, 10, 25, 100};
  for (int i = 0; i < 4; i++) {
    char n[8];
    fmt_int(n, sizeof n, counts[i]);
    win_text_button(w, n, 10 + i, BX_ACTIVABLE | (ncits == counts[i] ? BXF_ACTIVE : 0), false, false);
    w->rx += 2;
  }
  win_row_end(w);
  win_space(w, 6);
  const char *label = L_back;
  int a = 3;
  win_bottom(w, &label, &a, 1, L_continue, 4);
}

void menu_free_play_action(Win *wn, int a) {
  int standard = wn->arg2;
  static const float extra[M_COUNT] = {[M_WOOD] = 1000, [M_STONE] = 1000, [M_FOOD] = 1000, [M_MACHINEPARTS] = 100, [M_KNOWLEDGE] = 500};
  if (a == 1 || a == 2) {
    memset(&city.materials, 0, sizeof city.materials);
    for (int m = 0; m < M_COUNT; m++)
      city.materials.m[m] = a == 2 ? extra[m] : stories[wn->arg].initialMaterials[m];
    city.materials.m[M_FOOD] += 3.0 * (ncits - standard);
  } else if (a >= 10 && a < 14) {
    const int counts[] = {standard, 10, 25, 100};
    set_citizens(counts[a - 10]);
  } else if (a == 3 || a == ACT_CLOSE || a == ACT_BACK) {
    /* back (onCancel): to the title */
    win_close_all();
    title_open();
  } else if (a == 4) {
    /* the city starts: its welcome goal's window shows */
    win_close();
    ui.title = ui.setup = false;
    ui_after_load();
    ui_save(false);
  }
}

/* ---------------------------------------------------------------- credits */
void menu_credits_content(WinOut *w) {
  win_title(w, L_credits);
  win_para(w, FONT_15, L_keypad_credits_1, 250, C_BLACK);
  win_para(w, FONT_STANDARD, L_keypad_credits_2, 250, C_BLACK);
  win_space(w, 6);
  win_para(w, FONT_STANDARD, L_keypad_credits_3, 250, C_BLACK);
  win_para(w, FONT_STANDARD, L_credits_additional_sprites, 250, C_BLACK);
  win_space(w, 6);
  win_para(w, FONT_15, L_keypad_credits_4, 250, C_BLACK);
  win_space(w, 6);
  win_para(w, FONT_STANDARD, L_thanks_for_playing, 250, C_BLACK);
  win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
}

/* ---------------------------------------------------------------- the game menu (gui.GameMenu) */
enum { GM_SAVE = 1, GM_NEW, GM_RESTART, GM_NEXT, GM_TITLE, GM_CREDITS };

void menu_game_content(WinOut *w) {
  char t[64], num[16];
  win_title(w, L_game_menu);
  ui_float_fmt(num, sizeof num, ui.seconds_since_save, 1);
  win_para(w, FONT_STANDARD, lo_1(t, sizeof t, L_last_saved, LS(num)), 250, C_BLACK);
  win_row_begin(w);
  win_text_button(w, L_save, GM_SAVE, BX_BUTTON, false, false);
  win_row_end(w);
  win_space(w, 6);
  /* New City (filling the row), then Restart Scenario or Next Scenario */
  const StoryGoal *win_goal = NULL;
  for (int g = 0; g < stories[story.index].ngoals; g++)
    if (!strcmp(stories[story.index].goals[g].name, "Win")) win_goal = &stories[story.index].goals[g];
  bool next = story.goal < 0 && win_goal && win_goal->nextStory >= 0 && story_requirements(win_goal->nextStory);
  const char *second = next ? L_next_scenario : L_restart_scenario;
  int sw = ui_text_w(FONT_STANDARD, second) + 6;
  int nw = w->cw - sw - 2;
  int min_nw = ui_text_w(FONT_STANDARD, L_new_city) + 6;
  if (nw < min_nw) nw = min_nw;
  win_row_begin(w);
  int x0 = w->rx;
  /* a TextButton made wider (extraWidth): its text in the middle */
  int save_cw = w->cw;
  w->cw = nw;
  win_text_button(w, L_new_city, GM_NEW, BX_BUTTON, true, true);
  w->cw = save_cw;
  w->rx = x0 + nw + 2;
  win_text_button(w, second, next ? GM_NEXT : GM_RESTART, BX_BUTTON, false, false);
  win_row_end(w);
  win_space(w, 6);
  /* the credits button: an ImageButton (spr_button_small) with spr_credits tinted black */
  {
    bool f = win_small_button(w, 0, 18, 18, GM_CREDITS);
    int bx, by;
    if (win_area(w, 18, 18, &bx, &by)) {
      ui_frame(SPR_BUTTON_SMALL, 18, f ? (key_held(K_OK) ? 2 : 1) : 0, bx, by, 0, 255);
      ui_spr_ex(SPR_CREDITS, 0, 0, 14, 14, bx + 2, by + 2, 0, 255, 0x0000);
    }
  }
  win_space(w, 6);
  const char *label = L_back_to_title;
  int a = GM_TITLE;
  win_bottom(w, &label, &a, 1, L_close, ACT_CLOSE);
}

void menu_game_action(Win *wn, int a) {
  (void)wn;
  switch (a) {
  case GM_SAVE:
    ui_save(true);
    break;
  case GM_NEW: win_open(WK_SCENARIOS, 0, 0); break;
  case GM_RESTART: win_open(WK_RESTART, 0, 0); break;
  case GM_NEXT: {
    for (int g = 0; g < stories[story.index].ngoals; g++)
      if (!strcmp(stories[story.index].goals[g].name, "Win")) {
        win_close_all();
        menu_new_city(stories[story.index].goals[g].nextStory);
        return;
      }
    break;
  }
  case GM_TITLE:
    win_close_all();
    if (ui_save(false)) title_open();
    break;
  case GM_CREDITS: win_open(WK_CREDITS, 0, 0); break;
  }
}

void menu_restart_content(WinOut *w) {
  win_title(w, L_restart_scenario_question);
  win_para(w, FONT_STANDARD, L_sure_restart, 250, C_BLACK);
  const char *label = L_restart;
  int a = 1;
  win_bottom(w, &label, &a, 1, L_close, ACT_CLOSE);
}

void menu_restart_action(Win *wn, int a) {
  (void)wn;
  if (a == 1) {
    win_close_all();
    start_story(story.index);
  }
}

/* ---------------------------------------------------------------- the title screen (MainMenu) */
enum { TB_CONTINUE, TB_PLAY, TB_PLAYED_BEFORE, TB_NEW, TB_CREDITS, TB_COUNT };
static uint8_t title_focus;
static float title_time;

static int title_buttons(uint8_t *list) {
  int n = 0;
  if (save_exists()) {
    list[n++] = TB_CONTINUE;
    list[n++] = TB_NEW;
  } else {
    list[n++] = TB_PLAY;
    list[n++] = TB_PLAYED_BEFORE;
  }
  list[n++] = TB_CREDITS;
  return n;
}

/* the display city: the first scenario's island at night (the menu keeps its time at 0) */
void title_open(void) {
  ui.title = true;
  ui.setup = false;
  ui_before_city();
  game_start(0, NULL);
  ui_after_load();
  ui.title = true;
  /* displayCity's view: its bottom at 270 */
  camera.zoom = 1;
  camera.x = 220 - SCREEN_W / 2.0f;
  camera.y = 270 - SCREEN_H;
  title_focus = 0;
  title_time = 0;
}

void title_input(void) {
  title_time += TIMEMOD;
  if (!ui.setup) city.timeSinceStart = 0;   /* MainMenu.updateDisplayCity */
  if (nwins) {
    win_input();
    return;
  }
  uint8_t list[TB_COUNT];
  int n = title_buttons(list);
  if (title_focus >= n) title_focus = 0;
  if (key_repeat(K_DOWN) || key_repeat(K_RIGHT)) title_focus = (uint8_t)((title_focus + 1) % n);
  if (key_repeat(K_UP) || key_repeat(K_LEFT)) title_focus = (uint8_t)((title_focus + n - 1) % n);
  if (input.released & K_OK) {
    switch (list[title_focus]) {
    case TB_CONTINUE:
      ui_before_city();
      if (load_game()) {
        ui.title = false;
        game_after_load();
        ui_after_load();
      } else {
        win_open_text(NULL, L_keypad_load_failed);
      }
      break;
    case TB_PLAY: start_story(0); break;
    case TB_PLAYED_BEFORE:
    case TB_NEW: win_open(WK_SCENARIOS, 0, 0); break;
    case TB_CREDITS: win_open(WK_CREDITS, 0, 0); break;
    }
  }
}

void title_draw(void) {
  ui_noclip();
  if (nwins) return;
  /* the title, its lines down to "A game by Florian van Strien" (the music credit left out) */
  ui_spr_ex(SPR_TITLE, 0, 0, SPR_TITLE_W, 42, SCREEN_W / 2 - SPR_TITLE_W / 2, 10, 0, 255, GFX_NO_TINT);
  uint8_t list[TB_COUNT];
  int n = title_buttons(list), ntext = n - 1;
  int big = list[0] == TB_PLAY ? 1 : 0;
  int y = SCREEN_H - 10 - 16 * ntext - 3 * (big ? 0 : 0);
  for (int i = 0; i < ntext; i++) {
    const char *t;
    char buf[48] = "- ";
    switch (list[i]) {
    case TB_CONTINUE: t = L_continue; break;
    case TB_PLAY: t = L_click_here_to_play; break;
    case TB_PLAYED_BEFORE: t = L_played_before; break;
    default: t = L_new_city; break;
    }
    str_cat(str_cat(buf, sizeof buf, t), sizeof buf, " -");
    uint8_t font = list[i] == TB_PLAY ? FONT_18 : FONT_16;
    float a = i == title_focus ? 1.0f : 0.7f;
    if (list[i] == TB_PLAY && i != title_focus) a = 0.7f + 0.3f * sinf(title_time * 0.08f);
    uint16_t col = list[i] == TB_PLAYED_BEFORE ? rgb565(0xA0A0B0) : C_WHITE;
    int w = ui_text_w(font, buf);
    ui_text_n(font, SCREEN_W / 2 - w / 2, y, buf, -1, col, (uint8_t)(255 * a), false);
    y += font == FONT_18 ? 19 : 16;
  }
  /* the credits button at the bottom left (spr_credits) */
  bool f = list[title_focus] == TB_CREDITS;
  ui_spr_ex(SPR_CREDITS, 0, 0, 14, 14, 10, SCREEN_H - 24, 0, f ? 255 : 178, GFX_NO_TINT);
  if (f) {
    Tip tip = {0};
    tip.text = L_credits;
    ui_tooltip(&tip, 10, SCREEN_H - 24, false);
  }
}
