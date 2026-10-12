/* NumPowder: The Powder Toy's walls: their types (SimulationData.cpp's LoadWalls) and drawing them (Editing.cpp's
 * CreateWalls, CreateWallLine, CreateWallBox, FloodWalls). What they do is in the simulation (sim.c, sim_move.c,
 * air.c, gravity.c). Wall cells are WCELL particles wide. */
#include "simpriv.h"

const WallType walltypes[UI_WALLCOUNT] = {
  {0x808080, 0x000000, 0, "ERASE", "DEFAULT_WL_ERASE", "Erases walls."},
  {0xC0C0C0, 0x101010, 0, "CONDUCTIVE WALL", "DEFAULT_WL_CNDTW", "Blocks everything. Conductive."},
  {0x808080, 0x808080, 0, "EWALL", "DEFAULT_WL_EWALL", "E-Wall. Becomes transparent when electricity is connected."},
  {0xFF8080, 0xFF2008, 1, "DETECTOR", "DEFAULT_WL_DTECT", "Detector. Generates electricity when a particle is inside."},
  {0x808080, 0x000000, 0, "STREAMLINE", "DEFAULT_WL_STRM", "Streamline. Creates a line that follows air movement."},
  {0x8080FF, 0x000000, 1, "FAN", "DEFAULT_WL_FAN", "Fan. Accelerates air. Use the line tool to set direction and strength."},
  {0xC0C0C0, 0x101010, 2, "LIQUID WALL", "DEFAULT_WL_LIQD", "Allows liquids, blocks all other particles. Conductive."},
  {0x808080, 0x000000, 1, "ABSORB WALL", "DEFAULT_WL_ABSRB", "Absorbs particles but lets air currents through."},
  {0x808080, 0x000000, 3, "WALL", "DEFAULT_WL_WALL", "Basic wall, blocks everything."},
  {0x3C3C3C, 0x000000, 1, "AIRONLY WALL", "DEFAULT_WL_AIR", "Allows air, but blocks all particles."},
  {0x575757, 0x000000, 1, "POWDER WALL", "DEFAULT_WL_POWDR", "Allows powders, blocks all other particles."},
  {0xFFFF22, 0x101010, 2, "CONDUCTOR", "DEFAULT_WL_CNDTR", "Conductor. Allows all particles to pass through and conducts electricity."},
  {0x242424, 0x101010, 0, "EHOLE", "DEFAULT_WL_EHOLE", "E-Hole. absorbs particles, releases them when powered."},
  {0x579777, 0x000000, 1, "GAS WALL", "DEFAULT_WL_GAS", "Allows gases, blocks all other particles."},
  {0xFFEE00, 0xAA9900, 4, "GRAVITY WALL", "DEFAULT_WL_GRVTY", "Gravity wall. Newtonian Gravity has no effect inside a box drawn with this."},
  {0xFFAA00, 0xAA5500, 4, "ENERGY WALL", "DEFAULT_WL_ENRGY", "Allows energy particles, blocks all other particles."},
  {0xDCDCDC, 0x000000, 1, "AIRBLOCK WALL", "DEFAULT_WL_NOAIR", "Allows all particles, but blocks air."},
  {0x808080, 0x000000, 0, "ERASEALL", "DEFAULT_WL_ERASEA", "Erases walls, particles, and signs."},
  {0x800080, 0x000000, 0, "STASIS WALL", "DEFAULT_WL_STASIS", "Freezes particles inside the wall in place until powered."},
};

/* the walls in a square of wall cells around x, y (particles): the brush's radius, or rx, ry */
int CreateWalls(int x, int y, int rx, int ry, int wall, bool withBrush) {
  if (withBrush) {
    rx = brush.rx;
    ry = brush.ry;
  }
  ry = ry / WCELL;
  rx = rx / WCELL;
  x = x / WCELL;
  y = y / WCELL;
  x -= rx;
  y -= ry;
  for (int wallX = x; wallX <= x + rx + rx; wallX++)
    for (int wallY = y; wallY <= y + ry + ry; wallY++)
      if (wallX >= 0 && wallX < XWCELLS && wallY >= 0 && wallY < YWCELLS) {
        if (wall == WL_FAN) {
          fvx[wallY * WCELL / ACELL][wallX * WCELL / ACELL] = 0.0f;
          fvy[wallY * WCELL / ACELL][wallX * WCELL / ACELL] = 0.0f;
        } else if (wall == WL_STREAM) {
          wallX = x + rx;
          wallY = y + ry;
          /* streamlines can't be drawn next to each other */
          for (int tempY = wallY - 1; tempY < wallY + 2; tempY++)
            for (int tempX = wallX - 1; tempX < wallX + 2; tempX++)
              if (tempX >= 0 && tempX < XWCELLS && tempY >= 0 && tempY < YWCELLS && bmap[tempY][tempX] == WL_STREAM) return 1;
        }
        if (wall == WL_GRAV || bmap[wallY][wallX] == WL_GRAV) gravWallChanged = true;
        if (wall == WL_ERASEALL) {
          for (int i = 0; i < WCELL; i++)
            for (int j = 0; j < WCELL; j++) delete_part(wallX * WCELL + i, wallY * WCELL + j);
          bmap[wallY][wallX] = 0;
        } else {
          bmap[wallY][wallX] = (unsigned char)wall;
        }
      }
  return 1;
}

void CreateWallLine(int x1, int y1, int x2, int y2, int rx, int ry, int wall, bool withBrush) {
  int x, y, dx, dy, sy;
  bool reverseXY = abs(y2 - y1) > abs(x2 - x1);
  float e = 0.0f, de;
  if (reverseXY) {
    y = x1, x1 = y1, y1 = y;
    y = x2, x2 = y2, y2 = y;
  }
  if (x1 > x2) {
    y = x1, x1 = x2, x2 = y;
    y = y1, y1 = y2, y2 = y;
  }
  dx = x2 - x1;
  dy = abs(y2 - y1);
  de = dx ? dy / (float)dx : 0.0f;
  y = y1;
  sy = (y1 < y2) ? 1 : -1;
  for (x = x1; x <= x2; x++) {
    if (reverseXY) CreateWalls(y, x, rx, ry, wall, withBrush);
    else CreateWalls(x, y, rx, ry, wall, withBrush);
    e += de;
    if (e >= 0.5f) {
      y += sy;
      if ((y1 < y2) ? (y <= y2) : (y >= y2)) {
        if (reverseXY) CreateWalls(y, x, rx, ry, wall, withBrush);
        else CreateWalls(x, y, rx, ry, wall, withBrush);
      }
      e -= 1.0f;
    }
  }
}

void CreateWallBox(int x1, int y1, int x2, int y2, int wall) {
  int i, j;
  if (x1 > x2) i = x2, x2 = x1, x1 = i;
  if (y1 > y2) j = y2, y2 = y1, y1 = j;
  for (j = y1; j <= y2; j++)
    for (i = x1; i <= x2; i++) CreateWalls(i, j, 0, 0, wall, false);
}

/* fills the cells of wall bm (or none) connected to x, y (particles) with wall */
int FloodWalls(int x, int y, int wall, int bm) {
  int x1, x2, dy = WCELL;
  if (bm == -1) {
    if (wall == WL_ERASE || wall == WL_ERASEALL) {
      bm = bmap[y / WCELL][x / WCELL];
      if (!bm) return 0;
    } else {
      bm = 0;
    }
  }
  if (bmap[y / WCELL][x / WCELL] != bm) return 1;
  /* go left as far as possible */
  x1 = x2 = x;
  while (x1 >= WCELL) {
    if (bmap[y / WCELL][(x1 - 1) / WCELL] != bm) break;
    x1--;
  }
  while (x2 < XRES - WCELL) {
    if (bmap[y / WCELL][(x2 + 1) / WCELL] != bm) break;
    x2++;
  }
  /* fill span */
  for (x = x1; x <= x2; x++)
    if (!CreateWalls(x, y, 0, 0, wall, false)) return 0;
  /* fill children */
  if (y >= WCELL)
    for (x = x1; x <= x2; x++)
      if (bmap[(y - dy) / WCELL][x / WCELL] == bm)
        if (!FloodWalls(x, y - dy, wall, bm)) return 0;
  if (y < YRES - WCELL)
    for (x = x1; x <= x2; x++)
      if (bmap[(y + dy) / WCELL][x / WCELL] == bm)
        if (!FloodWalls(x, y + dy, wall, bm)) return 0;
  return 1;
}
