/* The Final Earth 2: the game's loop (game.c). */
#ifndef FE_GAME_H
#define FE_GAME_H
#include "fe.h"
#include "story.h"

#include "bg.h"

extern bool game_running;
extern uint32_t game_ticks;
extern float frame_time_mod;   /* the last frame's length in ticks, 1 to 2 */
extern BgEdges city_edges;   /* City.getCityEdges, kept up to date for the clouds and the camera */
void game_init(void);
void game_start(int story, const NewCityOptions *opt);
void game_after_load(void);   /* a save was read: its sky, clouds and view */
void game_tick(void);
void game_draw(void);
#endif
