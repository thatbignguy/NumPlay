/* The Final Earth 2: the user interface (ui*.c), as the game's loop sees it.
 *
 * The original's GUI made for keys: the city's cursor, the build menu, the
 * city panel, windows, the story's goals and tutorial arrows, the title
 * screen and the game menu. */
#ifndef FE_UI_H
#define FE_UI_H
#include "fe.h"

typedef struct UiFrame UiFrame;   /* a frame's primitives, on game_draw's stack (uii.h) */

void ui_init(void);           /* at start: the title screen */
void ui_tick(void);           /* a tick's keys, after input_update (moves the camera too) */
void ui_draw(UiFrame *f);       /* the interface over everything */
void ui_before_city(void);      /* before a city is made or loaded */
void ui_after_load(void);       /* a city was loaded or made: the city's controls start afresh */
#endif
