/* The End's fight (end.c): the dragon, its crystals, the exit portal and the boss bar. */
#ifndef NB_END_H
#define NB_END_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t dragon_dead, pad[3];
  float dragon_hp;             /* the dragon's health while it lives (0: not met yet, full) */
  uint32_t crystals_gone[2];   /* the End's crystals destroyed, by pillar */
} DimsState;
extern DimsState dst;

void dragon_enter(void);
void end_tick(void);
float dragon_bar(void);
bool dragon_beam(float *from, float *to);
void dragon_offsets(int i, float pt, float *yaw, float *y);
float dragon_anim(float pt);
void dragon_tick(Entity *e);
void dragon_hurt(Entity *e, float dmg);
void crystal_gone(Entity *e);
#endif
