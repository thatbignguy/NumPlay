/* The Final Earth 2: what a window shows, said by the code that knows it
 * (a building's class, a citizen, the city's menus) and laid out by the
 * user interface (ui*.c). Texts are UTF-8, in the original's words. */
#ifndef FE_WIN_H
#define FE_WIN_H
#include "fe.h"

typedef struct WinOut WinOut;
void win_title(WinOut *w, const char *s);
void win_text(WinOut *w, const char *s);                      /* a paragraph, wrapped */
void win_info(WinOut *w, uint16_t icon, const char *s);       /* a line with an icon (SPR_*, 0 none) */
void win_space(WinOut *w, int h);                             /* GUISpacing: h pixels */
/* a button of the window's owner: pressing it calls its window_action(action). Alone it is a FullSizeTextButton;
 * followed by win_text (a description) and win_cost, it is UpgradeWindowParts.createActivatableButton. Not
 * enabled: with a description and no cost, the option chosen (the active box, not a button); with a cost, one
 * the city can't pay (a button all the same, its cost in red, that does nothing); alone, greyed out */
void win_button(WinOut *w, uint16_t icon, const char *label, int action, bool enabled);
/* materials, as the original's cost lines (icons and amounts) */
void win_cost(WinOut *w, const float cost[M_COUNT]);
/* UpgradeWindowParts.createHeader (Arial15); with one_action >= 0 also addOneAndMaxButtons on its row: One
 * (pressed unless all) and All (Max when max), with their tooltips */
void win_heading(WinOut *w, const char *title, int one_action, int all_action, bool all, bool max, const char *one_tip,
                 const char *all_tip);
/* a button of the window's bottom row, left of its Close (windowAddBottomButtons' extra buttons, at most two),
 * with its tooltip */
void win_bottom_button(WinOut *w, const char *label, int action, const char *tip_header, const char *tip);
/* a paragraph in one of the original's fonts (FONT_*, data.h), wrapped at max_w */
void win_para(WinOut *w, uint8_t font, const char *s, int max_w, uint16_t color);
/* gui.CheckboxButton.createSettingButton: a checkbox and its text in a button */
void win_setting(WinOut *w, bool checked, int action, const char *text);
/* what a building's window shows after its upgrades, modes and policies (the original's createMainWindowPart after
 * the Building's, and createWindowAddBottomButtons): what follows this call goes there */
void win_later(WinOut *w);
#endif
