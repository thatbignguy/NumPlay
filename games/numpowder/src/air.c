/* NumPowder: The Powder Toy's air (Air.cpp): pressure and velocity, ambient heat, the edges, walls blocking air
 * and fans. Air cells are ACELL particles wide, as the original's; a wall cell is WCELL, so an air cell is
 * blocked when any of its wall cells blocks (and has a fan when any of them is one). */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O3")   /* (a hot path: a few KB of flash for speed) */
#endif
#include "simpriv.h"

float pv[YACELLS][XACELLS], vx[YACELLS][XACELLS], vy[YACELLS][XACELLS], hv[YACELLS][XACELLS];
uint8_t bmap_blockair[YACELLS][XACELLS], bmap_blockairh[YACELLS][XACELLS];
float ambientAirTemp = R_TEMP + 273.15f, edgePressure, edgeVelocityX, edgeVelocityY, vorticityCoeff = 0.1f;
int airMode = AIR_ON, convectionMode = AIRC_BOUSSINESQ;
static float kernel[9];

#define WPA (ACELL / WCELL)   /* wall cells across an air cell */

void air_make_kernel(void) {   /* used for velocity */
  float s = 0.0f;
  for (int j = -1; j < 2; j++)
    for (int i = -1; i < 2; i++) {
      kernel[(i + 1) + 3 * (j + 1)] = expf(-2.0f * (i * i + j * j));
      s += kernel[(i + 1) + 3 * (j + 1)];
    }
  s = 1.0f / s;
  for (int j = -1; j < 2; j++)
    for (int i = -1; i < 2; i++) kernel[(i + 1) + 3 * (j + 1)] *= s;
}

/* vorticity that takes bmap_blockair into account (old velocities: ovx, ovy) */
static float vorticityBmap(float (*ovx)[XACELLS], float (*ovy)[XACELLS], int y, int x) {
  if (x > 1 && x < XACELLS - 2 && y > 1 && y < YACELLS - 2) {
    float dvydx = (bmap_blockair[y][x] || bmap_blockair[y][x + 1] || bmap_blockair[y][x - 1]) ? 0.0f : ovy[y][x + 1] - ovy[y][x - 1];
    float dvxdy = (bmap_blockair[y][x] || bmap_blockair[y + 1][x] || bmap_blockair[y - 1][x]) ? 0.0f : ovx[y + 1][x] - ovx[y - 1][x];
    return (dvydx - dvxdy) * 0.5f;
  }
  return 0.0f;
}

void air_clear(void) {
  for (int y = 0; y < YACELLS; y++)
    for (int x = 0; x < XACELLS; x++) pv[y][x] = edgePressure, vx[y][x] = edgeVelocityX, vy[y][x] = edgeVelocityY;
}
void air_clear_heat(void) {
  for (int y = 0; y < YACELLS; y++)
    for (int x = 0; x < XACELLS; x++) hv[y][x] = ambientAirTemp;
}
void air_invert(void) {
  for (int y = 0; y < YACELLS; y++)
    for (int x = 0; x < XACELLS; x++) pv[y][x] = -pv[y][x], vx[y][x] = -vx[y][x], vy[y][x] = -vy[y][x];
}

static inline bool InCellBounds(int x, int y) { return x >= 0 && y >= 0 && x < XACELLS && y < YACELLS; }
static const float advDistanceMult = 0.7f;   /* when taking temperature or velocity from far away */

void air_update_heat(void) {
  float (*ohv)[XACELLS] = (float (*)[XACELLS])(void *)sim_scratch;
  for (int i = 0; i < YACELLS; i++)   /* the edges' air temperature, every frame */
    hv[i][0] = hv[i][1] = hv[i][XACELLS - 2] = hv[i][XACELLS - 1] = ambientAirTemp;
  for (int i = 0; i < XACELLS; i++)
    hv[0][i] = hv[1][i] = hv[YACELLS - 2][i] = hv[YACELLS - 1][i] = ambientAirTemp;
  for (int y = 0; y < YACELLS; y++)
    for (int x = 0; x < XACELLS; x++) {
      float dh = 0.0f, dx = 0.0f, dy = 0.0f;
      for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
          float f = kernel[i + 1 + (j + 1) * 3];
          if (y + j > 0 && y + j < YACELLS - 1 && x + i > 0 && x + i < XACELLS - 1 && !(bmap_blockairh[y + j][x + i] & 0x8)) {
            dh += hv[y + j][x + i] * f;
            dx += vx[y + j][x + i] * f;
            dy += vy[y + j][x + i] * f;
          } else {
            dh += hv[y][x] * f;
            dx += vx[y][x] * f;
            dy += vy[y][x] * f;
          }
        }
      /* air temperature from far away, as the velocity in air_update */
      float tx = x - dx * advDistanceMult, ty = y - dy * advDistanceMult;
      if ((fabsf(dx * advDistanceMult) > 1.0f || fabsf(dy * advDistanceMult) > 1.0f) && (tx >= 2 && tx < XACELLS - 2 && ty >= 2 && ty < YACELLS - 2)) {
        float stepX, stepY;
        int stepLimit;
        if (fabsf(dx) > fabsf(dy)) {
          stepX = (dx < 0.0f) ? 1.f : -1.f;
          stepY = -dy / fabsf(dx);
          stepLimit = (int)(fabsf(dx * advDistanceMult));
        } else {
          stepY = (dy < 0.0f) ? 1.f : -1.f;
          stepX = -dx / fabsf(dy);
          stepLimit = (int)(fabsf(dy * advDistanceMult));
        }
        tx = (float)x;
        ty = (float)y;
        int step = 0;
        for (; step < stepLimit; ++step) {
          tx += stepX;
          ty += stepY;
          if (!InCellBounds((int)(tx + 0.5f), (int)(ty + 0.5f)) || bmap_blockairh[(int)(ty + 0.5f)][(int)(tx + 0.5f)] & 0x8) {
            tx -= stepX;
            ty -= stepY;
            break;
          }
        }
        if (step == stepLimit) {   /* no wall found */
          tx = x - dx * advDistanceMult;
          ty = y - dy * advDistanceMult;
        }
      }
      int i = (int)tx, j = (int)ty;
      tx -= i;
      ty -= j;
      if (!(bmap_blockairh[y][x] & 0x8) && i >= 0 && i < XACELLS - 1 && j >= 0 && j < YACELLS - 1 && tx >= 0.0f && ty >= 0.0f) {
        float odh = dh;
        dh *= 1.0f - AIR_VADV;
        dh += AIR_VADV * (1.0f - tx) * (1.0f - ty) * ((bmap_blockairh[j][i] & 0x8) ? odh : hv[j][i]);
        dh += AIR_VADV * tx * (1.0f - ty) * ((bmap_blockairh[j][i + 1] & 0x8) ? odh : hv[j][i + 1]);
        dh += AIR_VADV * (1.0f - tx) * ty * ((bmap_blockairh[j + 1][i] & 0x8) ? odh : hv[j + 1][i]);
        dh += AIR_VADV * tx * ty * ((bmap_blockairh[j + 1][i + 1] & 0x8) ? odh : hv[j + 1][i + 1]);
      }
      if (bmap_blockairh[y][x] & 0x8) dh = hv[y][x];   /* a cell that blocks ambient heat doesn't change */
      if (dh > MAX_TEMP) dh = MAX_TEMP;
      if (dh < MIN_TEMP) dh = MIN_TEMP;
      ohv[y][x] = dh;

      /* air convection (the velocities change in place, as in the original) */
      float dvx = vx[y][x], dvy = vy[y][x];
      if (x >= 2 && x < XACELLS - 2 && y >= 2 && y < YACELLS - 2) {
        float convGravX, convGravY;
        GetGravityField(x * ACELL, y * ACELL, -1.0f, -1.0f, &convGravX, &convGravY);
        switch (convectionMode) {
        case AIRC_LEGACY: {   /* before 99.0 */
          float weight = ((hv[y][x] - hv[y][x - 1]) * convGravX + (hv[y][x] - hv[y - 1][x]) * convGravY) / 5000.0f;
          if (weight > 0 && !(bmap_blockairh[y - 1][x] & 0x8)) {
            dvx += weight * convGravX;
            dvy += weight * convGravY;
          }
          break;
        }
        case AIRC_BOUSSINESQ: {
          /* density changes only in the gravity term, linearly with the difference to the ambient temperature */
          float gravMagn = sqrtf(convGravX * convGravX + convGravY * convGravY);
          if (gravMagn > 10.0f) {   /* cap the gravity field */
            convGravX /= 0.1f * gravMagn;
            convGravY /= 0.1f * gravMagn;
          }
          float weight = (hv[y][x] - ambientAirTemp) / 10000.0f;
          if (weight > 0.01f) weight = 0.01f;   /* best when the difference is small */
          dvx += weight * convGravX;
          dvy += weight * convGravY;
          break;
        }
        default:
          break;
        }
      }
      if (dvx > MAX_PRESSURE) dvx = MAX_PRESSURE;
      if (dvx < MIN_PRESSURE) dvx = MIN_PRESSURE;
      if (dvy > MAX_PRESSURE) dvy = MAX_PRESSURE;
      if (dvy < MIN_PRESSURE) dvy = MIN_PRESSURE;
      vx[y][x] = dvx;
      vy[y][x] = dvy;
    }
  memcpy(hv, ohv, sizeof hv);
}

static inline float Mix(float a, float b, float f) { return a + (b - a) * f; }

static bool fan_at(int y, int x) {
  for (int j = 0; j < WPA; j++)
    for (int i = 0; i < WPA; i++)
      if (bmap[y * WPA + j][x * WPA + i] == WL_FAN) return true;
  return false;
}

void air_update(void) {
  if (airMode == AIR_NOUPDATE) return;
  for (int i = 0; i < YACELLS; i++) {   /* pressure and velocity fade on the edges every frame */
    pv[i][0] = Mix(edgePressure, pv[i][0], 0.8f);
    pv[i][1] = Mix(edgePressure, pv[i][1], 0.8f);
    pv[i][XACELLS - 2] = Mix(edgePressure, pv[i][XACELLS - 2], 0.8f);
    pv[i][XACELLS - 1] = Mix(edgePressure, pv[i][XACELLS - 1], 0.8f);
    vx[i][0] = Mix(edgeVelocityX, vx[i][0], 0.9f);
    vx[i][1] = Mix(edgeVelocityX, vx[i][1], 0.9f);
    vx[i][XACELLS - 2] = Mix(edgeVelocityX, vx[i][XACELLS - 2], 0.9f);
    vx[i][XACELLS - 1] = Mix(edgeVelocityX, vx[i][XACELLS - 1], 0.9f);
    vy[i][0] = Mix(edgeVelocityY, vy[i][0], 0.9f);
    vy[i][1] = Mix(edgeVelocityY, vy[i][1], 0.9f);
    vy[i][XACELLS - 2] = Mix(edgeVelocityY, vy[i][XACELLS - 2], 0.9f);
    vy[i][XACELLS - 1] = Mix(edgeVelocityY, vy[i][XACELLS - 1], 0.9f);
  }
  for (int i = 0; i < XACELLS; i++) {
    pv[0][i] = Mix(edgePressure, pv[0][i], 0.8f);
    pv[1][i] = Mix(edgePressure, pv[1][i], 0.8f);
    pv[YACELLS - 2][i] = Mix(edgePressure, pv[YACELLS - 2][i], 0.8f);
    pv[YACELLS - 1][i] = Mix(edgePressure, pv[YACELLS - 1][i], 0.8f);
    vx[0][i] = Mix(edgeVelocityX, vx[0][i], 0.9f);
    vx[1][i] = Mix(edgeVelocityX, vx[1][i], 0.9f);
    vx[YACELLS - 2][i] = Mix(edgeVelocityX, vx[YACELLS - 2][i], 0.9f);
    vx[YACELLS - 1][i] = Mix(edgeVelocityX, vx[YACELLS - 1][i], 0.9f);
    vy[0][i] = Mix(edgeVelocityY, vy[0][i], 0.9f);
    vy[1][i] = Mix(edgeVelocityY, vy[1][i], 0.9f);
    vy[YACELLS - 2][i] = Mix(edgeVelocityY, vy[YACELLS - 2][i], 0.9f);
    vy[YACELLS - 1][i] = Mix(edgeVelocityY, vy[YACELLS - 1][i], 0.9f);
  }
  for (int j = 1; j < YACELLS - 1; j++)   /* no velocity near walls */
    for (int i = 1; i < XACELLS - 1; i++)
      if (bmap_blockair[j][i]) {
        vx[j][i] = vx[j][i - 1] = vx[j][i + 1] = 0.0f;
        vy[j][i] = vy[j - 1][i] = vy[j + 1][i] = 0.0f;
      }
  for (int y = 1; y < YACELLS - 1; y++)   /* pressure from velocity */
    for (int x = 1; x < XACELLS - 1; x++) {
      float dp = 0.0f;
      dp += vx[y][x - 1] - vx[y][x + 1];
      dp += vy[y - 1][x] - vy[y + 1][x];
      pv[y][x] = Mix(edgePressure, pv[y][x], AIR_PLOSS);
      pv[y][x] += dp * AIR_TSTEPP * 0.5f;
    }
  for (int y = 1; y < YACELLS - 1; y++)   /* velocity from pressure */
    for (int x = 1; x < XACELLS - 1; x++) {
      float dx = 0.0f, dy = 0.0f;
      dx += pv[y][x - 1] - pv[y][x + 1];
      dy += pv[y - 1][x] - pv[y + 1][x];
      vx[y][x] = Mix(edgeVelocityX, vx[y][x], AIR_VLOSS);
      vy[y][x] = Mix(edgeVelocityY, vy[y][x], AIR_VLOSS);
      vx[y][x] += dx * AIR_TSTEPV * 0.5f;
      vy[y][x] += dy * AIR_TSTEPV * 0.5f;
      if (bmap_blockair[y][x - 1] || bmap_blockair[y][x] || bmap_blockair[y][x + 1]) vx[y][x] = 0;
      if (bmap_blockair[y - 1][x] || bmap_blockair[y][x] || bmap_blockair[y + 1][x]) vy[y][x] = 0;
    }

  /* velocity and pressure: the next values of vx and vy go to the scratch RAM, those of pv to two rows there
   * (a row is written back once the next one no longer reads it) */
  float (*ovx)[XACELLS] = (float (*)[XACELLS])(void *)sim_scratch;
  float (*ovy)[XACELLS] = ovx + YACELLS;
  float (*opv)[XACELLS] = ovy + YACELLS;   /* 2 rows */
  for (int y = 0; y < YACELLS; y++) {
    float *prow = opv[y & 1];
    for (int x = 0; x < XACELLS; x++) {
      float dx = 0.0f, dy = 0.0f, dp = 0.0f;
      for (int j = -1; j < 2; j++)
        for (int i = -1; i < 2; i++) {
          float f = kernel[i + 1 + (j + 1) * 3];
          if (y + j > 0 && y + j < YACELLS - 1 && x + i > 0 && x + i < XACELLS - 1 && !bmap_blockair[y + j][x + i]) {
            dx += vx[y + j][x + i] * f;
            dy += vy[y + j][x + i] * f;
            dp += pv[y + j][x + i] * f;
          } else {
            dx += vx[y][x] * f;
            dy += vy[y][x] * f;
            dp += pv[y][x] * f;
          }
        }
      float tx = x - dx * advDistanceMult, ty = y - dy * advDistanceMult;
      if ((fabsf(dx * advDistanceMult) > 1.0f || fabsf(dy * advDistanceMult) > 1.0f) && (tx >= 2 && tx < XACELLS - 2 && ty >= 2 && ty < YACELLS - 2)) {
        /* velocity from far away: look for a wall in the way, stepping a cell at a time along x or y */
        float stepX, stepY;
        int stepLimit;
        if (fabsf(dx) > fabsf(dy)) {
          stepX = (dx < 0.0f) ? 1.f : -1.f;
          stepY = -dy / fabsf(dx);
          stepLimit = (int)(fabsf(dx * advDistanceMult));
        } else {
          stepY = (dy < 0.0f) ? 1.f : -1.f;
          stepX = -dx / fabsf(dy);
          stepLimit = (int)(fabsf(dy * advDistanceMult));
        }
        tx = (float)x;
        ty = (float)y;
        int step = 0;
        for (; step < stepLimit; ++step) {
          tx += stepX;
          ty += stepY;
          if (!InCellBounds((int)(tx + 0.5f), (int)(ty + 0.5f)) || bmap_blockair[(int)(ty + 0.5f)][(int)(tx + 0.5f)]) {
            tx -= stepX;
            ty -= stepY;
            break;
          }
        }
        if (step == stepLimit) {   /* no wall found */
          tx = x - dx * advDistanceMult;
          ty = y - dy * advDistanceMult;
        }
      }
      int i = (int)tx, j = (int)ty;
      tx -= i;
      ty -= j;
      if (!bmap_blockair[y][x] && i >= 2 && i < XACELLS - 3 && j >= 2 && j < YACELLS - 3 && tx >= 0.0f && ty >= 0.0f) {
        dx *= 1.0f - AIR_VADV;
        dy *= 1.0f - AIR_VADV;
        dx += AIR_VADV * (1.0f - tx) * (1.0f - ty) * vx[j][i];
        dy += AIR_VADV * (1.0f - tx) * (1.0f - ty) * vy[j][i];
        dx += AIR_VADV * tx * (1.0f - ty) * vx[j][i + 1];
        dy += AIR_VADV * tx * (1.0f - ty) * vy[j][i + 1];
        dx += AIR_VADV * (1.0f - tx) * ty * vx[j + 1][i];
        dy += AIR_VADV * (1.0f - tx) * ty * vy[j + 1][i];
        dx += AIR_VADV * tx * ty * vx[j + 1][i + 1];
        dy += AIR_VADV * tx * ty * vy[j + 1][i + 1];
      }
      /* vorticity confinement */
      if (vorticityCoeff > 0.0f && x > 1 && x < XACELLS - 2 && y > 1 && y < YACELLS - 2) {
        float dwx = (fabsf(vorticityBmap(vx, vy, y, x + 1)) - fabsf(vorticityBmap(vx, vy, y, x - 1))) * 0.5f;
        float dwy = (fabsf(vorticityBmap(vx, vy, y + 1, x)) - fabsf(vorticityBmap(vx, vy, y - 1, x))) * 0.5f;
        float norm = hypotf(dwx, dwy);
        float w = vorticityBmap(vx, vy, y, x);
        dx += vorticityCoeff / 5.0f * dwy / (norm + 0.001f) * w;
        dy += vorticityCoeff / 5.0f * (-dwx) / (norm + 0.001f) * w;
      }
      if (fan_at(y, x)) {
        dx += fvx[y][x];
        dy += fvy[y][x];
      }
      if (dp > MAX_PRESSURE) dp = MAX_PRESSURE;
      if (dp < MIN_PRESSURE) dp = MIN_PRESSURE;
      if (dx > MAX_PRESSURE) dx = MAX_PRESSURE;
      if (dx < MIN_PRESSURE) dx = MIN_PRESSURE;
      if (dy > MAX_PRESSURE) dy = MAX_PRESSURE;
      if (dy < MIN_PRESSURE) dy = MIN_PRESSURE;
      switch (airMode) {
      default:
      case AIR_ON: break;
      case AIR_PRESSUREOFF: dp = 0.0f; break;
      case AIR_VELOCITYOFF: dx = 0.0f, dy = 0.0f; break;
      case AIR_OFF: dx = 0.0f, dy = 0.0f, dp = 0.0f; break;
      case AIR_NOUPDATE: break;
      }
      ovx[y][x] = dx;
      ovy[y][x] = dy;
      prow[x] = dp;
    }
    if (y > 0) memcpy(pv[y - 1], opv[(y - 1) & 1], sizeof pv[0]);
  }
  memcpy(pv[YACELLS - 1], opv[(YACELLS - 1) & 1], sizeof pv[0]);
  memcpy(vx, ovx, sizeof vx);
  memcpy(vy, ovy, sizeof vy);
}

/* walls block air and ambient heat (BeforeSim, after the walls' electricity faded) */
void air_walls_block(void) {
  for (int y = 0; y < YACELLS; y++)
    for (int x = 0; x < XACELLS; x++) {
      uint8_t block = 0, grav = 0;
      for (int j = 0; j < WPA; j++)
        for (int i = 0; i < WPA; i++) {
          int b = bmap[y * WPA + j][x * WPA + i];
          block |= b == WL_WALL || b == WL_WALLELEC || b == WL_BLOCKAIR || (b == WL_EWALL && !emap[y * WPA + j][x * WPA + i]);
          grav |= b == WL_GRAV;
        }
      bmap_blockair[y][x] = block;
      bmap_blockairh[y][x] = (block || grav) ? 0x8 : 0;
    }
}

/* after loading a save, so that nothing "leaks" the first frame (ApproximateBlockAirMaps) */
void air_approximate_block_maps(void) {
  air_walls_block();
  for (int i = 0; i < parts_active; i++) {
    int type = parts[i].type;
    if (!type) continue;
    int x = ((parts[i].x + 32) / 64) / ACELL, y = ((parts[i].y + 32) / 64) / ACELL;
    if (!InCellBounds(x, y)) continue;
    /* real TTAN only blocks when there's enough, but blocking for a frame is fine */
    if (type == PT_TTAN || type == PT_RSSS) {
      bmap_blockair[y][x] = 1;
      bmap_blockairh[y][x] = 0x8;
    } else if (IsHeatInsulator(&parts[i]) || elements[type].HeatConduct <= (rng_u32() % 250)) {
      /* mostly accurate insulators (but GEL) */
      if (!(bmap_blockairh[y][x] & 0x8)) bmap_blockairh[y][x]++;
    }
  }
}
