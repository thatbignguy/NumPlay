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
  case E_WSKELETON: {
    /* the skeleton's model, its black bones: the wither skeleton */
    box(-4, -8, -4, 8, 8, 8, 0, 0, 0, 0, 0, SKIN_WITHER_SKELETON)->rx = pitch;
    box(-4, 0, -2, 8, 12, 4, 16, 16, 0, 0, 0, SKIN_WITHER_SKELETON);
    Part *ra = box(-1, -2, -1, 2, 12, 2, 40, 16, -5, 2, 0, SKIN_WITHER_SKELETON);
    Part *la = box(-1, -2, -1, 2, 12, 2, 40, 16, 5, 2, 0, SKIN_WITHER_SKELETON);
    Part *rl = box(-1, 0, -1, 2, 12, 2, 0, 16, -2, 12, 0, SKIN_WITHER_SKELETON);
    Part *ll = box(-1, 0, -1, 2, 12, 2, 0, 16, 2, 12, 0, SKIN_WITHER_SKELETON);
    ra->rx = la->rx = -PI / 2 + sinf(age * 0.067f) * 0.05f;
    ra->rz = cosf(age * 0.09f) * 0.05f + 0.05f;
    la->rz = -ra->rz;
    la->mirror = ll->mirror = 1;
    rl->rx = sw * 1.4f * a, ll->rx = swo * 1.4f * a;
    break;
  }
  case E_BLAZE: {
    /* ModelBlaze: the head and twelve rods in three rings */
    Part *h = box(-4, -4, -4, 8, 8, 8, 0, 0, 0, 0, 0, SKIN_BLAZE);
    h->rx = pitch;
    float g = age * PI * -0.1f;
    for (int i = 0; i < 4; i++, g++)
      box(0, 0, 0, 2, 8, 2, 0, 16, cosf(g) * 9, -2 + cosf(((float)(i * 2) + age) * 0.25f), sinf(g) * 9, SKIN_BLAZE);
    g = PI / 4 + age * PI * 0.03f;
    for (int j = 4; j < 8; j++, g++)
      box(0, 0, 0, 2, 8, 2, 0, 16, cosf(g) * 7, 2 + cosf(((float)(j * 2) + age) * 0.25f), sinf(g) * 7, SKIN_BLAZE);
    g = 0.47123894f + age * PI * -0.05f;
    for (int k = 8; k < 12; k++, g++)
      box(0, 0, 0, 2, 8, 2, 0, 16, cosf(g) * 5, 11 + cosf(((float)k * 1.5f + age) * 0.5f), sinf(g) * 5, SKIN_BLAZE);
    break;
  }
  case E_MAGMA: {
    /* ModelMagmaCube: eight slices pulled apart as it squishes, and its core */
    float sq = e->on_ground ? 0 : 0.6f;
    for (int i = 0; i < 8; i++) {
      int u = 0, v = i;
      if (i == 2) u = 24, v = 10;
      else if (i == 3) u = 24, v = 19;
      box(-4, (float)(16 + i), -4, 8, 1, 8, u, v, 0, (float)(-(4 - i)) * sq * 1.7f, 0, SKIN_MAGMACUBE);
    }
    box(-2, 18, -2, 4, 4, 4, 0, 16, 0, 0, 0, SKIN_MAGMACUBE);
    break;
  }
  case E_GHAST: {
    /* ModelGhast: a 16-pixel cube and nine tentacles */
    box(-8, -8, -8, 16, 16, 16, 0, 0, 0, 8 + 9.6f, 0, SKIN_GHAST);
    for (int i = 0; i < 9; i++) {
      float fx = (((float)(i % 3) - (float)(i / 3 % 2) * 0.5f + 0.25f) / 2.0f * 2.0f - 1.0f) * 5.0f;
      float fz = ((float)(i / 3) / 2.0f * 2.0f - 1.0f) * 5.0f;
      box(-1, 0, -1, 2, 8, 2, 0, 0, fx, 15 + 9.6f, fz, SKIN_GHAST)->rx = 0.2f * sinf(age * 0.3f + (float)i) + 0.4f;
    }
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
