/* The Final Earth 2: the interface's hooks, doing nothing, for the tests that run the game without it
 * (simtest, storytest, savetest). */
#include "../src/story.h"
#include "../src/win.h"

void win_title(WinOut *w, const char *s) { (void)w, (void)s; }
void win_text(WinOut *w, const char *s) { (void)w, (void)s; }
void win_info(WinOut *w, uint16_t icon, const char *s) { (void)w, (void)icon, (void)s; }
void win_button(WinOut *w, uint16_t icon, const char *label, int action, bool enabled) {
  (void)w, (void)icon, (void)label, (void)action, (void)enabled;
}
void win_cost(WinOut *w, const float cost[M_COUNT]) { (void)w, (void)cost; }
void win_para(WinOut *w, uint8_t font, const char *s, int max_w, uint16_t color) {
  (void)w, (void)font, (void)s, (void)max_w, (void)color;
}
void win_setting(WinOut *w, bool checked, int action, const char *text) { (void)w, (void)checked, (void)action, (void)text; }
void win_later(WinOut *w) { (void)w; }
void win_space(WinOut *w, int h) { (void)w, (void)h; }
void win_heading(WinOut *w, const char *title, int one_action, int all_action, bool all, bool max, const char *one_tip,
                 const char *all_tip) {
  (void)w, (void)title, (void)one_action, (void)all_action, (void)all, (void)max, (void)one_tip, (void)all_tip;
}
void win_bottom_button(WinOut *w, const char *label, int action, const char *tip_header, const char *tip) {
  (void)w, (void)label, (void)action, (void)tip_header, (void)tip;
}
void ui_show_goal_text(int goal) { (void)goal; }
void ui_show_tutorial(int tutorial) { (void)tutorial; }
void ui_show_message(const char *text) { (void)text; }
void ui_show_simple(const char *text) { (void)text; }
bool ui_story_window_open(void) { return false; }
void meta_win_scenario(int s, float time) { (void)s, (void)time; }
void ui_pause_for_window(void) {}
