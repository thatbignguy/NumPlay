/* NumPowder: The Powder Toy's Newtonian gravity (gravity/Fft.cpp, Simulation's gravity functions). The original
 * convolves the masses with a point mass's field by FFT, on a thread that hands back the result a frame later;
 * on 25 x 16 cells, a direct sum over the cells with mass is cheap. As in the original, the field used in a frame
 * is that of the masses two frames before (one frame in the thread, one to take its result), and it's only
 * computed again when the masses or the gravity walls change. */
#include "simpriv.h"

float gravx[YACELLS][XACELLS], gravy[YACELLS][XACELLS];   /* gravOut */
float gravmass[YACELLS][XACELLS];                          /* gravIn.mass */
uint8_t gravmask[YACELLS][XACELLS];                        /* gravIn.mask (1: 0xFFFFFFFF, 0: 0) */
/* the worker's input (the masses and mask handed over last frame), and whether it has a result to take */
float grav_work_mass[YACELLS][XACELLS];
uint8_t grav_work_mask[YACELLS][XACELLS];
bool grav_work_pending, grav_force_recalc = true;
#define work_mass grav_work_mass
#define work_mask grav_work_mask
#define work_pending grav_work_pending
#define force_recalc grav_force_recalc

/* M_GRAV / d^3 for d = (dx, dy), dx 0..XACELLS-1, dy 0..YACELLS-1 (the kernel, by symmetry) */
static float kern[YACELLS][XACELLS];
static bool kern_done;
static void make_kernel(void) {
  for (int dy = 0; dy < YACELLS; dy++)
    for (int dx = 0; dx < XACELLS; dx++) {
      float d = hypotf((float)dx, (float)dy);
      kern[dy][dx] = (dx || dy) ? M_GRAV / (d * d * d) : 0.0f;
    }
  kern_done = true;
}

/* the worker's job (GravityImpl::Work): the field of the masses in the mask, zero outside the mask */
static void work(void) {
  if (!kern_done) make_kernel();
  memset(gravx, 0, sizeof gravx);
  memset(gravy, 0, sizeof gravy);
  for (int qy = 0; qy < YACELLS; qy++)
    for (int qx = 0; qx < XACELLS; qx++) {
      float m = work_mask[qy][qx] ? work_mass[qy][qx] : 0.f;
      if (m == 0.f) continue;
      for (int y = 0; y < YACELLS; y++) {
        int dy = y - qy, ady = dy < 0 ? -dy : dy;
        for (int x = 0; x < XACELLS; x++) {
          int dx = x - qx, adx = dx < 0 ? -dx : dx;
          float k = m * kern[ady][adx];   /* the force at (x, y) points to the mass */
          gravx[y][x] -= k * (float)dx;
          gravy[y][x] -= k * (float)dy;
        }
      }
    }
  for (int y = 0; y < YACELLS; y++)
    for (int x = 0; x < XACELLS; x++)
      if (!work_mask[y][x]) gravx[y][x] = gravy[y][x] = 0;
}

/* Gravity::Exchange, at the start of a frame: take the last result, hand over the new masses if they changed */
void grav_exchange(void) {
  if (!ngrav_enable) return;
  if (work_pending) {
    work();
    work_pending = false;
  }
  if (force_recalc || memcmp(work_mass, gravmass, sizeof work_mass) || memcmp(work_mask, gravmask, sizeof work_mask)) {
    memcpy(work_mass, gravmass, sizeof work_mass);
    memcpy(work_mask, gravmask, sizeof work_mask);
    work_pending = true;
  }
  force_recalc = false;
}

/* ResetNewtonianGravity({}, {}) */
void grav_reset(void) {
  memset(gravmass, 0, sizeof gravmass);
  memset(gravmask, 1, sizeof gravmask);
  memset(gravx, 0, sizeof gravx);
  memset(gravy, 0, sizeof gravy);
  memset(work_mass, 0, sizeof work_mass);
  memset(work_mask, 1, sizeof work_mask);
  work_pending = false;
  force_recalc = true;
  gravWallChanged = true;
}

/* EnableNewtonianGravity */
void grav_enable(bool enable) {
  if (ngrav_enable && !enable) {
    memset(gravx, 0, sizeof gravx);
    memset(gravy, 0, sizeof gravy);
    work_pending = false;
    force_recalc = true;
  }
  if (!ngrav_enable && enable) {
    memset(work_mass, 0, sizeof work_mass);
    memset(work_mask, 1, sizeof work_mask);
    force_recalc = true;
  }
  ngrav_enable = enable;
  settings.newtonian_gravity = enable;
}

/* the cells inside boxes of gravity walls get no gravity (UpdateGravityMask): filled from the edges through the
 * cells without a gravity wall (an air cell has one when any of its wall cells is one) */
static bool grav_wall(int x, int y) {
  for (int j = 0; j < ACELL / WCELL; j++)
    for (int i = 0; i < ACELL / WCELL; i++)
      if (bmap[y * (ACELL / WCELL) + j][x * (ACELL / WCELL) + i] == WL_GRAV) return true;
  return false;
}
void UpdateGravityMask(void) {
  memset(gravmask, 0, sizeof gravmask);
  uint16_t *stack = (uint16_t *)(void *)sim_scratch;   /* (each cell pushes at most 4) */
  _Static_assert(XACELLS * YACELLS * 4 * 2 <= SCRATCH, "the gravity mask's stack fits the scratch RAM");
  int n = 0;
#define CHECK(px, py)                                                                         \
  do {                                                                                        \
    int cx_ = (px), cy_ = (py);                                                               \
    if (!(grav_wall(cx_, cy_) || gravmask[cy_][cx_])) {                                       \
      gravmask[cy_][cx_] = 1;                                                                 \
      if (cx_ > 0) stack[n++] = (uint16_t)(cy_ * XACELLS + cx_ - 1);                          \
      if (cx_ < XACELLS - 1) stack[n++] = (uint16_t)(cy_ * XACELLS + cx_ + 1);                \
      if (cy_ > 0) stack[n++] = (uint16_t)((cy_ - 1) * XACELLS + cx_);                        \
      if (cy_ < YACELLS - 1) stack[n++] = (uint16_t)((cy_ + 1) * XACELLS + cx_);              \
    }                                                                                         \
  } while (0)
  for (int x = 0; x < XACELLS; ++x) {
    CHECK(x, 0);
    CHECK(x, YACELLS - 1);
  }
  for (int y = 1; y < YACELLS - 1; ++y) {
    CHECK(0, y);
    CHECK(XACELLS - 1, y);
  }
  while (n > 0) {
    int c = stack[--n];
    CHECK(c % XACELLS, c / XACELLS);
  }
#undef CHECK
}
