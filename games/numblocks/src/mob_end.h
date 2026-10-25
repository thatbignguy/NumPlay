/* The End's creatures (included at the end of mob.c): the enderman, the ender crystal
 * and the Ender Dragon's model and per-tick state. The dragon's flight, health and
 * portal are end.c's; this file draws them and gives the enderman its blink. */
#ifndef NB_MOB_END_H
#define NB_MOB_END_H
#include <math.h>

static void end_parts(const Entity *e, float pt, float f, float a, float pitch, float age) {
  float sw = cosf(f * 0.6662f), swo = cosf(f * 0.6662f + PI);
  switch (e->type) {
  case E_ENDERMAN: {
    /* ModelEnderman: a biped 14 pixels taller, its limbs long and thin */
    bool angry = e->panic > 0;
    float hy = -13 - (angry ? 5 : 0);
    box(-4, -8, -4, 8, 8, 8, 0, 0, 0, hy, 0, SKIN_ENDERMAN)->rx = pitch;
    Part *jaw = box(-4, -8, -4, 8, 8, 8, 0, 16, 0, -13, 0, SKIN_ENDERMAN);
    jaw->grow = -0.5f, jaw->rx = pitch;
    box(-4, 0, -2, 8, 12, 4, 32, 16, 0, -14, 0, SKIN_ENDERMAN);
    float ar = swo * a * 0.5f, al = sw * a * 0.5f, lr = sw * 1.4f * a * 0.5f, ll = swo * 1.4f * a * 0.5f;
#define CL(v) ((v) > 0.4f ? 0.4f : (v) < -0.4f ? -0.4f : (v))
    Part *rA = box(-1, -2, -1, 2, 30, 2, 56, 0, -3, -12, 0, SKIN_ENDERMAN);
    Part *lA = box(-1, -2, -1, 2, 30, 2, 56, 0, 5, -12, 0, SKIN_ENDERMAN);
    rA->rx = CL(ar), lA->rx = CL(al), lA->mirror = 1;
    box(-1, 0, -1, 2, 30, 2, 56, 0, -2, -5, 0, SKIN_ENDERMAN)->rx = CL(lr);
    Part *lL = box(-1, 0, -1, 2, 30, 2, 56, 0, 2, -5, 0, SKIN_ENDERMAN);
    lL->rx = CL(ll), lL->mirror = 1;
#undef CL
    break;
  }
  case E_DRAGON: {
    /* A compact ModelDragon: the long body, the head and jaw, two wings and a tail. */
    float flap = sinf(age * 0.35f) * 0.35f;
    box(-6, -4, -36, 12, 10, 16, 0, 0, 0, -8, 0, SKIN_DRAGON)->rx = pitch;
    Part *jaw = box(-5, -1, -38, 10, 4, 14, 0, 32, 0, -8, 0, SKIN_DRAGON);
    jaw->rx = 0.25f + sinf(age * 0.2f) * 0.08f;
    box(-12, 0, -20, 24, 20, 48, 0, 0, 0, 8, 0, SKIN_DRAGON);
    Part *rw = box(-56, -2, -4, 54, 4, 8, 32, 0, 0, 8, 0, SKIN_DRAGON);
    Part *lw = box(-56, -2, -4, 54, 4, 8, 32, 0, 0, 8, 0, SKIN_DRAGON);
    rw->rz = flap, lw->rz = -flap, lw->mirror = 1;
    box(-4, 2, 20, 8, 8, 16, 0, 16, 0, 8, 0, SKIN_DRAGON)->rx = sinf(age * 0.2f) * 0.1f;
    box(-3, 3, 32, 6, 6, 14, 0, 16, 0, 8, 0, SKIN_DRAGON)->rx = sinf(age * 0.2f + 0.5f) * 0.1f;
    break;
  }
  case E_CRYSTAL: {
    /* ModelEnderCrystal: its base and the three glass cubes inside, turning and bobbing */
    float t = e->age + pt, bob = sinf(t * 0.2f) / 2 + 0.5f;
    bob = (bob * bob + bob) * 0.2f;
    box(-6, 0, -6, 12, 4, 12, 0, 16, 0, 0, 0, SKIN_CRYSTAL);
    for (int k = 0; k < 3; k++) {
      Part *p = box(-4, -4, -4, 8, 8, 8, (k == 2 ? 0 : 32), 0, 0, 8 - bob * 8, 0, SKIN_CRYSTAL);
      p->grow = -(float)k * 0.45f;
      p->ry = t * 0.05f + (float)k;
    }
    break;
  }
  default: break;
  }
}

/* the enderman's blink: a short random teleport, 32 blocks around (EntityEnderman.teleportRandomly) */
static void enderman_tp(Entity *e) {
  for (int k = 0; k < 16; k++) {
    float x = e->x + (rndf() - 0.5f) * 64, y = e->y + (float)(rnd(64) - 32), z = e->z + (rndf() - 0.5f) * 64;
    if (!world_loaded((int)x, (int)y, (int)z)) continue;
    if (blk_flags[world_get((int)x, (int)y, (int)z)] & BF_SOLID) continue;
    if (blk_flags[world_get((int)x, (int)y + 1, (int)z)] & BF_SOLID) continue;
    e->x = x, e->y = y, e->z = z, e->vx = e->vy = e->vz = 0;
    return;
  }
}

#endif
