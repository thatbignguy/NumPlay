/* The Final Earth 2: the user interface's main parts (the original's GUI,
 * CityGUI.update and the game states' switching): which screen shows, where
 * the keys go, and the order things are drawn in. */
#include "game.h"
#include "save.h"
#include "uii.h"
#include "view.h"

UiState ui;

const WinDef windefs[WK_COUNT] = {
  [WK_TEXT] = {text_content, NULL, 0},
  [WK_BUILDING] = {bwin_content, bwin_action, 0},
  [WK_CITIZEN] = {cwin_content, cwin_action, WD_TOP},
  [WK_FOLLOW_LIST] = {flist_content, flist_action, WD_TOP},
  [WK_MATERIAL] = {info_material, NULL, WD_OPAQUE},
  [WK_POPULATION] = {info_population, info_population_action, 0},
  [WK_HAPPINESS] = {info_happiness, info_happiness_action, 0},
  [WK_HOUSES] = {info_houses, info_houses_action, 0},
  [WK_WORKERS] = {info_workers, info_workers_action, 0},
  [WK_BIRTH] = {info_birth, info_birth_action, 0},
  [WK_ENTERTAINMENT] = {info_entertainment, NULL, 0},
  [WK_FIND] = {info_find, info_find_action, 0},
  [WK_UPGRADES] = {info_upgrades, info_upgrades_action, 0},
  [WK_INVITE] = {info_invite, info_invite_action, 0},
  [WK_GOAL] = {story_goal_content, story_goal_action, 0},
  [WK_MISSIONS] = {story_missions_content, NULL, 0},
  [WK_GAME_MENU] = {menu_game_content, menu_game_action, 0},
  [WK_RESTART] = {menu_restart_content, menu_restart_action, 0},
  [WK_SCENARIOS] = {menu_scenarios_content, menu_scenarios_action, 0},
  [WK_FREE_PLAY] = {menu_free_play_content, menu_free_play_action, WD_OWN_CLOSE},
  [WK_CREDITS] = {menu_credits_content, NULL, 0},
  [WK_DESTROY] = {destroy_content, destroy_action, 0},
  [WK_REPLACE] = {replace_content, replace_action, 0},
  [WK_REPLACE_TO] = {replace_to_content, replace_to_action, 0},
  [WK_CITY_NAME] = {city_name_content, city_name_action, 0},
  [WK_COLOR] = {color_content, color_action, 0},
};

void ui_init(void) {
  meta_load();
  title_open();
}

/* before a city is made or loaded: the old city's windows and tutorial go */
void ui_before_city(void) {
  win_close_all();
  story_ui_reset();
  ui.paused_for_window = false;
}

/* a city was loaded or made (its story may have opened its first goal's window): the city's controls start
 * afresh */
void ui_after_load(void) {
  view_edges(&city_edges);
  city_ui_reset();
  if (saved_view.valid) {
    /* the cursor where it was saved */
    cu.cx = saved_view.cx, cu.cy = saved_view.cy;
    saved_view.valid = false;
  }
  ui.seconds_since_save = 0;
}

void ui_tick(void) {
  if (ui.title) {
    title_input();
    story_ui_tick();
    return;
  }
  story_ui_tick();
  autosave_tick();
  if (nwins) win_input();
  else city_input();
  /* the category shown keeps no "new" marker (createBuildingCategoryButtons: stopNotifyCategoryUnlock) */
  if (!ui.title && cu.category >= 0) unlocks.notifyCategory &= (uint16_t)~(1u << cu.category);
  /* while a text is typed, the keys that type don't also move the focus (input.c) */
  input.typing = win_is_open(WK_CITY_NAME);
}

void ui_draw(UiFrame *f) {
  ui_frame_begin(f);
  if (!ui.title) {
    city_draw_overlay();
  }
  if (ui.title) {
    title_draw();
  } else {
    city_draw_ui();
  }
  win_draw();
  /* the original's stages from here: help arrows, tooltips, notifications */
  ui_noclip();
  story_ui_draw_help();
  if (!ui.title) city_draw_tips();
  win_draw_tip();
  story_ui_draw_notes();
  ui_frame_end();
}
