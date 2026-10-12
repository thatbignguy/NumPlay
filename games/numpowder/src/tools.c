/* NumPowder: drawing on the simulation with the brush (Editing.cpp's CreateParts and the rest, the tools of
 * gui/game/tool and simulation/simtools), and the stickmen's keys (GameController). */
#include "simpriv.h"

const SimToolInfo simtools[TOOL_COUNT] = {
  {"DEFAULT_TOOL_HEAT", "HEAT", 0xFFDD00, "Heats the targeted element."},
  {"DEFAULT_TOOL_COOL", "COOL", 0x00DDFF, "Cools the targeted element."},
  {"DEFAULT_TOOL_AIR", "AIR", 0xFFFFFF, "Air, creates airflow and pressure."},
  {"DEFAULT_TOOL_VAC", "VAC", 0x303030, "Vacuum, reduces air pressure."},
  {"DEFAULT_TOOL_PGRV", "PGRV", 0xCCCCFF, "Creates a short-lasting gravity well."},
  {"DEFAULT_TOOL_NGRV", "NGRV", 0xAACCFF, "Creates a short-lasting negative gravity well."},
  {"DEFAULT_TOOL_MIX", "MIX", 0xFFD090, "Mixes particles."},
  {"DEFAULT_TOOL_CYCL", "CYCL", 0x132f5b, "Cyclone, produces swirling air currents"},
  {"DEFAULT_TOOL_AMBM", "AMBM", 0x00DDFF, "Decreases ambient air temperature."},
  {"DEFAULT_TOOL_AMBP", "AMBP", 0xFFDD00, "Increases ambient air temperature."},
  {"DEFAULT_TOOL_WIND", "WIND", 0x404040, "Creates air movement."},
};

Brush brush = {4, 4, BRUSH_CIRCLE};
float tool_strength = 1.0f;
int replaceModeSelected, replaceModeFlags;

/* ---------------------------------------------------------------- the brushes (EllipseBrush with a perfect
 * circle, RectangleBrush, TriangleBrush) */
static int clampr(int r) { return r < 0 ? 0 : r > 200 ? 200 : r; }
/* the ellipse's column i (0..2rx): its rows j (0..2ry) with bottom < j < top */
static int ellipse_top(int i, int rx, int ry) {
  /* the original's test in doubles, times 16 in integers (exact) */
  if (i > rx) i = 2 * rx - i;
  int yTop = ry + 1;
  int64_t a = 4 * (int64_t)(i - rx) * (i - rx) * (2 * ry - 1) * (2 * ry - 1), b = 4 * (int64_t)(2 * rx - 1) * (2 * rx - 1);
  int64_t r2 = 16 * (int64_t)rx * rx * ry * ry;
  while (a + (int64_t)(yTop - ry) * (yTop - ry) * b <= r2) yTop++;
  return yTop;
}
bool brush_has(int dx, int dy) {
  int rx = clampr(brush.rx), ry = clampr(brush.ry);
  if (dx < -rx || dx > rx || dy < -ry || dy > ry) return false;
  switch (brush.shape) {
  case BRUSH_SQUARE:
    return true;
  case BRUSH_TRIANGLE:
    return (abs((rx + 2 * dx) * ry + rx * dy) + abs(2 * rx * (dy - ry)) + abs((rx - 2 * dx) * ry + rx * dy)) <= (4 * rx * ry);
  default: {
    if (!rx) return true;   /* a vertical line */
    int i = dx + rx, j = dy + ry;
    if (i == rx && (j == 0 || j == 2 * ry)) return true;   /* the top and bottom middle are always in */
    int top = ellipse_top(i, rx, ry), bottom = 2 * ry - top;
    return j > bottom && j < top;
  }
  }
}
/* the brush's points around x, y, in the original's order (bottom row first, left to right) */
typedef void (*BrushFn)(int x, int y, void *u);
static void brush_each(int x, int y, BrushFn fn, void *u) {
  int rx = clampr(brush.rx), ry = clampr(brush.ry);
  if (brush.shape == BRUSH_CIRCLE && rx) {
    int16_t top[201];   /* each column's top (by symmetry, columns 0..rx) */
    for (int i = 0; i <= rx; i++) top[i] = (int16_t)ellipse_top(i, rx, ry);
    for (int dy = ry; dy >= -ry; dy--)
      for (int dx = -rx; dx <= rx; dx++) {
        int i = dx + rx, j = dy + ry, t = top[i > rx ? 2 * rx - i : i];
        if ((j > 2 * ry - t && j < t) || (i == rx && (j == 0 || j == 2 * ry))) fn(x + dx, y + dy, u);
      }
  } else {
    for (int dy = ry; dy >= -ry; dy--)
      for (int dx = -rx; dx <= rx; dx++)
        if (brush_has(dx, dy)) fn(x + dx, y + dy, u);
  }
}

/* ---------------------------------------------------------------- the element tools (Editing.cpp) */
static int CreatePartFlags(int p, int x, int y, int c, int flags) {
  if (x < 0 || y < 0 || x >= XRES || y >= YRES) return 0;
  int pm = PMAPAT(x, y), ph = PHOTAT(x, y);
  if (flags & REPLACE_MODE) {
    /* replace anything there, or a particle of the type on top */
    if ((!replaceModeSelected && (ph || pm)) || (!ph && pm && TYP(pm) == replaceModeSelected) || (ph && TYP(ph) == replaceModeSelected)) {
      if (c) create_part(ph ? ID(ph) : ID(pm), x, y, TYP(c), ID(c));
      else delete_part(x, y);
    }
    return 0;
  } else if (!c) {
    delete_part(x, y);
    return 0;
  } else if (flags & SPECIFIC_DELETE) {
    if ((!replaceModeSelected && (ph || pm)) || (!ph && pm && TYP(pm) == replaceModeSelected) || (ph && TYP(ph) == replaceModeSelected))
      delete_part(x, y);
    return 0;
  }
  return create_part(p, x, y, TYP(c), ID(c)) == -1;
}

typedef struct { int p, c, flags; } PartsCtx;
static void create_at(int x, int y, void *u) {
  PartsCtx *k = u;
  if (x >= 0 && y >= 0 && x < XRES && y < YRES) CreatePartFlags(k->p, x, y, k->c, k->flags);
}
/* CreateParts with the brush */
int CreateParts(int p, int x, int y, int c, int flags) {
  if (flags == -1) flags = replaceModeFlags;
  int rx = brush.rx, ry = brush.ry;
  if (c == PT_LIGH) {   /* special case for LIGH */
    if (currentTick < lightningRecreate) return 1;
    int newlife = rx + ry;
    if (newlife > 55) newlife = 55;
    c = PMAP(newlife, c);
    lightningRecreate = currentTick + imax(newlife / 4, 1);
    return CreatePartFlags(p, x, y, c, flags);
  } else if (c == PT_TESC) {
    int newtmp = rx * 4 + ry * 4 + 7;
    if (newtmp > 300) newtmp = 300;
    c = PMAP(newtmp, c);
  }
  PartsCtx k = {p, c, flags};
  brush_each(x, y, create_at, &k);
  return 0;
}
/* CreateParts in a rectangle of radii rx, ry */
int CreatePartsR(int p, int x, int y, int rx, int ry, int c, int flags) {
  bool created = false;
  if (flags == -1) flags = replaceModeFlags;
  if (c == PT_LIGH) {
    if (currentTick < lightningRecreate) return 1;
    int newlife = rx + ry;
    if (newlife > 55) newlife = 55;
    c = PMAP(newlife, c);
    lightningRecreate = currentTick + imax(newlife / 4, 1);
    rx = ry = 0;
  } else if (c == PT_TESC) {
    int newtmp = rx * 4 + ry * 4 + 7;
    if (newtmp > 300) newtmp = 300;
    c = PMAP(newtmp, c);
  }
  for (int j = -ry; j <= ry; j++)
    for (int i = -rx; i <= rx; i++)
      if (CreatePartFlags(p, x + i, y + j, c, flags)) created = true;
  return !created;
}

void CreateLineBrush(int x1, int y1, int x2, int y2, int c, int flags) {
  int x, y, dx, dy, sy, rx = brush.rx, ry = brush.ry;
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
    if (reverseXY) CreateParts(-2, y, x, c, flags);
    else CreateParts(-2, x, y, c, flags);
    e += de;
    if (e >= 0.5f) {
      y += sy;
      if (!(rx + ry) && ((y1 < y2) ? (y <= y2) : (y >= y2))) {
        if (reverseXY) CreateParts(-2, y, x, c, flags);
        else CreateParts(-2, x, y, c, flags);
      }
      e -= 1.0f;
    }
  }
}

void CreateBox(int p, int x1, int y1, int x2, int y2, int c, int flags) {
  int i, j;
  if (x1 > x2) i = x2, x2 = x1, x1 = i;
  if (y1 > y2) j = y2, y2 = y1, y1 = j;
  for (j = y2; j >= y1; j--)
    for (i = x1; i <= x2; i++) CreatePartsR(p, i, j, 0, 0, c, flags);
}

/* the flood fill's bitmap and stack in the scratch RAM (as sim.c's) */
#define FLOOD_BITMAP ((XRES * YRES + 7) / 8)
static inline bool fb_get(int x, int y) {
  int k = y * XRES + x;
  return sim_scratch[k >> 3] >> (k & 7) & 1;
}
static inline void fb_set(int x, int y) {
  int k = y * XRES + x;
  sim_scratch[k >> 3] |= (uint8_t)(1 << (k & 7));
}

int FloodParts(int x, int y, int fullc, int cm, int flags) {
  int c = TYP(fullc);
  int x1, x2, dy = 1, created_something = 0;
  if (cm == -1) {
    /* outside: nothing */
    if (c != 0 && (x < BORDER || x >= XRES - BORDER || y < BORDER || y >= YRES - BORDER || c == PT_SPRK)) return 1;
    else if (x < 0 || x >= XRES || y < 0 || y >= YRES) return 1;
    if (c == 0) {
      cm = TYP(PMAPAT(x, y));
      if (!cm) {
        cm = TYP(PHOTAT(x, y));
        if (!cm) {
          if (bmap[y / WCELL][x / WCELL]) return FloodWalls(x, y, WL_ERASE, -1);
          return -1;
        }
      }
    } else {
      cm = 0;
    }
  }
  if (c != 0 && IsWallBlocking(x, y, c)) return 1;
  if (!FloodFillPmapCheck(x, y, cm)) return 1;
  memset(sim_scratch, 0, FLOOD_BITMAP);
  flood_stack_init(FLOOD_BITMAP);
  flood_stack_push(x, y);
  do {
    flood_stack_pop(&x, &y);
    x1 = x2 = x;
    /* go left as far as possible */
    while (c ? x1 > BORDER : x1 > 0) {
      if (fb_get(x1 - 1, y) || !FloodFillPmapCheck(x1 - 1, y, cm) || (c != 0 && IsWallBlocking(x1 - 1, y, c))) break;
      x1--;
    }
    while (c ? x2 < XRES - BORDER - 1 : x2 < XRES - 1) {
      if (fb_get(x2 + 1, y) || !FloodFillPmapCheck(x2 + 1, y, cm) || (c != 0 && IsWallBlocking(x2 + 1, y, c))) break;
      x2++;
    }
    /* fill span */
    for (x = x1; x <= x2; x++) {
      if (!fullc) {
        if (elements[cm].Properties & TYPE_ENERGY) {
          if (photons_id[y][x]) {
            kill_part(photons_id[y][x] - 1);
            created_something = 1;
          }
        } else if (pmap_id[y][x]) {
          kill_part(pmap_id[y][x] - 1);
          created_something = 1;
        }
      } else if (CreatePartsR(-2, x, y, 0, 0, fullc, flags)) {
        created_something = 1;
      }
      fb_set(x, y);
    }
    if (c ? y >= BORDER + dy : y >= dy)
      for (x = x1; x <= x2; x++)
        if (!fb_get(x, y - dy) && FloodFillPmapCheck(x, y - dy, cm) && (c == 0 || !IsWallBlocking(x, y - dy, c)))
          if (!flood_stack_push(x, y - dy)) return -1;
    if (c ? y < YRES - BORDER - dy : y < YRES - dy)
      for (x = x1; x <= x2; x++)
        if (!fb_get(x, y + dy) && FloodFillPmapCheck(x, y + dy, cm) && (c == 0 || !IsWallBlocking(x, y + dy, c)))
          if (!flood_stack_push(x, y + dy)) return -1;
  } while (flood_stack_size() > 0);
  return created_something;
}

/* ---------------------------------------------------------------- the simulation tools (simtools/) */
static int tool_perform(int tool, Particle *cpart, int x, int y, int brushX, int brushY, float strength) {
  int ax = x / ACELL, ay = y / ACELL;
  switch (tool) {
  case TOOL_HEAT:
  case TOOL_COOL: {
    if (!cpart) return 0;
    float d = (cpart->type == PT_PUMP || cpart->type == PT_GPMP) ? strength * .1f : strength * 2.0f;
    /* (kept within 0 and MAX_TEMP; dithered: PUMP's and GPMP's 0.1 K a time is under a step of 1/4 K) */
    p_set_temp_d(cpart, p_temp(cpart) + (tool == TOOL_HEAT ? d : -d));
    return 1;
  }
  case TOOL_AIR:
  case TOOL_VAC:
    pv[ay][ax] += (tool == TOOL_AIR ? strength : -strength) * 0.05f;
    if (pv[ay][ax] > MAX_PRESSURE) pv[ay][ax] = MAX_PRESSURE;
    else if (pv[ay][ax] < MIN_PRESSURE) pv[ay][ax] = MIN_PRESSURE;
    return 1;
  case TOOL_PGRV:
    gravmass[ay][ax] = strength * 5.0f;
    return 1;
  case TOOL_NGRV:
    gravmass[ay][ax] = strength * -5.0f;
    return 1;
  case TOOL_MIX: {
    int thisPart = PMAPAT(x, y);
    if (!thisPart) return 0;
    if (rng_u32() % 100 != 0) return 0;
    int distance = (int)(powf(strength, .5f) * 10);
    if (distance <= 0) return 0;
    if (!(elements[TYP(thisPart)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS))) return 0;
    int newX = x + (int)(rng_u32() % (unsigned)distance) - (distance / 2);
    int newY = y + (int)(rng_u32() % (unsigned)distance) - (distance / 2);
    if (newX < 0 || newY < 0 || newX >= XRES || newY >= YRES) return 0;
    int thatPart = PMAPAT(newX, newY);
    if (!thatPart) return 0;
    if ((elements[TYP(thisPart)].Properties & STATE_FLAGS) != (elements[TYP(thatPart)].Properties & STATE_FLAGS)) return 0;
    PMAPSET(x, y, thatPart);
    parts[ID(thatPart)].x = (int16_t)(x * 64);
    parts[ID(thatPart)].y = (int16_t)(y * 64);
    PMAPSET(newX, newY, thisPart);
    parts[ID(thisPart)].x = (int16_t)(newX * 64);
    parts[ID(thisPart)].y = (int16_t)(newY * 64);
    return 1;
  }
  case TOOL_CYCL:
    /* once per air cell (less laggy): the air turns around the brush's middle */
    if ((x % ACELL) == 0 && (y % ACELL) == 0) {
      if (brushX == x && brushY == y) return 1;
      float dvx = (float)(brushX - x), dvy = (float)(brushY - y);
      float invsqr = 1 / sqrtf(dvx * dvx + dvy * dvy);
      vx[ay][ax] -= (strength / 16) * (-dvy) * invsqr;
      vy[ay][ax] -= (strength / 16) * dvx * invsqr;
      if (vx[ay][ax] > MAX_PRESSURE) vx[ay][ax] = MAX_PRESSURE;
      else if (vx[ay][ax] < MIN_PRESSURE) vx[ay][ax] = MIN_PRESSURE;
      if (vy[ay][ax] > MAX_PRESSURE) vy[ay][ax] = MAX_PRESSURE;
      else if (vy[ay][ax] < MIN_PRESSURE) vy[ay][ax] = MIN_PRESSURE;
    }
    return 1;
  case TOOL_AMBM:
  case TOOL_AMBP:
    if (!aheat_enable) return 0;
    hv[ay][ax] += (tool == TOOL_AMBP ? strength : -strength) * 2.0f;
    if (hv[ay][ax] > MAX_TEMP) hv[ay][ax] = MAX_TEMP;
    if (hv[ay][ax] < MIN_TEMP) hv[ay][ax] = MIN_TEMP;
    return 1;
  default:   /* (WIND has no Perform) */
    return 0;
  }
}
/* SimTool::CallPerform */
static void call_perform(int tool, int x, int y, int brushX, int brushY, float strength) {
  Particle *cpart = NULL;
  if (pmap_id[y][x]) cpart = &parts[pmap_id[y][x] - 1];
  else if (photons_id[y][x]) cpart = &parts[photons_id[y][x] - 1];
  tool_perform(tool, cpart, x, y, brushX, brushY, strength);
}
typedef struct { int tool, bx, by; float strength; } PerformCtx;
static void perform_at(int x, int y, void *u) {
  PerformCtx *k = u;
  if (x >= 0 && y >= 0 && x < XRES && y < YRES) call_perform(k->tool, x, y, k->bx, k->by, k->strength);
}
static void tool_draw(int tool, int x, int y, float strength) {   /* defaultPerformDraw */
  PerformCtx k = {tool, x, y, strength};
  brush_each(x, y, perform_at, &k);
}
typedef struct { float dx, dy; } WindCtx;
static void wind_at(int x, int y, void *u) {
  WindCtx *k = u;
  if (x >= 0 && y >= 0 && x < XRES && y < YRES) {
    vx[y / ACELL][x / ACELL] += k->dx;
    vy[y / ACELL][x / ACELL] += k->dy;
  }
}
static void tool_draw_line(int tool, int x1, int y1, int x2, int y2, bool dragging, float strength) {
  if (tool == TOOL_WIND) {   /* WIND's performDrawLine */
    float s = (dragging ? 0.01f : 0.002f) * strength;
    WindCtx k = {(x2 - x1) * s, (y2 - y1) * s};
    brush_each(x1, y1, wind_at, &k);
    return;
  }
  /* defaultPerformDrawLine */
  int x, y, dx, dy, sy, rx = brush.rx, ry = brush.ry;
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
    if (reverseXY) tool_draw(tool, y, x, strength);
    else tool_draw(tool, x, y, strength);
    e += de;
    if (e >= 0.5f) {
      y += sy;
      if (!(rx + ry) && ((y1 < y2) ? (y <= y2) : (y >= y2))) {
        if (reverseXY) tool_draw(tool, y, x, strength);
        else tool_draw(tool, x, y, strength);
      }
      e -= 1.0f;
    }
  }
}
static void tool_draw_rect(int tool, int x1, int y1, int x2, int y2, float strength) {   /* defaultPerformDrawRect */
  int brushX = (x1 + x2) / 2, brushY = (y1 + y2) / 2;
  int tlx = imin(x1, x2), tly = imin(y1, y2), brx = imax(x1, x2), bry = imax(y1, y2);
  for (int y = tly; y <= bry; y++)
    for (int x = tlx; x <= brx; x++)
      if (x >= 0 && y >= 0 && x < XRES && y < YRES) call_perform(tool, x, y, brushX, brushY, strength);
}

/* ---------------------------------------------------------------- the tools as the interface uses them */
static bool plop_tool(int c) { return c == PT_STKM || c == PT_STKM2 || c == PT_FIGH; }

void sim_tool_points(int tool, int x1, int y1, int x2, int y2, bool held) {
  if (tool & TOOL_WALLS) {
    int wall = tool & 0xFF;
    if (!held) CreateWalls(x2, y2, 1, 1, wall, true);
    else CreateWallLine(x1, y1, x2, y2, 1, 1, wall, true);
  } else if (tool & TOOL_SIMTOOLS) {
    int t = tool & 0xFF;
    if (!held) tool_draw(t, x2, y2, tool_strength);
    else tool_draw_line(t, x1, y1, x2, y2, true, tool_strength);
  } else if (plop_tool(tool)) {
    /* PlopTool: one, where clicked */
    if (!held) create_part(-2, x2, y2, TYP(tool), ID(tool));
  } else if (tool == PT_LIGH) {
    /* Element_LIGH_Tool: drawn while dragging, in the brush's rectangle */
    if (!held) CreateParts(-2, x2, y2, tool, -1);
    else CreatePartsR(-2, x1, y1, brush.rx, brush.ry, PT_LIGH, -1);
  } else {
    if (!held) CreateParts(-2, x2, y2, tool, -1);
    else CreateLineBrush(x1, y1, x2, y2, tool, -1);
  }
}

void sim_tool_line(int tool, int x1, int y1, int x2, int y2) {
  if (tool & TOOL_WALLS) {
    int wall = tool & 0xFF;
    int wallX = x1 / WCELL, wallY = y1 / WCELL;
    if (wall == WL_FAN && bmap[wallY][wallX] == WL_FAN) {
      /* the fans connected to the start get the line's direction and strength */
      float newFanVelX = (x2 - x1) * 0.005f, newFanVelY = (y2 - y1) * 0.005f;
      FloodWalls(x1, y1, WL_FLOODHELPER, WL_FAN);
      for (int j = 0; j < YWCELLS; j++)
        for (int i = 0; i < XWCELLS; i++)
          if (bmap[j][i] == WL_FLOODHELPER) {
            fvx[j * WCELL / ACELL][i * WCELL / ACELL] = newFanVelX;
            fvy[j * WCELL / ACELL][i * WCELL / ACELL] = newFanVelY;
            bmap[j][i] = WL_FAN;
          }
    } else {
      CreateWallLine(x1, y1, x2, y2, 1, 1, wall, true);
    }
  } else if (tool & TOOL_SIMTOOLS) {
    tool_draw_line(tool & 0xFF, x1, y1, x2, y2, false, 1.0f);
  } else if (plop_tool(tool)) {
  } else if (tool == PT_LIGH) {
    /* (not dragging: nothing) */
  } else {
    CreateLineBrush(x1, y1, x2, y2, tool, -1);
  }
}

void sim_tool_box(int tool, int x1, int y1, int x2, int y2) {
  if (tool & TOOL_WALLS) CreateWallBox(x1, y1, x2, y2, tool & 0xFF);
  else if (tool & TOOL_SIMTOOLS) tool_draw_rect(tool & 0xFF, x1, y1, x2, y2, 1.0f);
  else if (plop_tool(tool)) return;
  else if (tool == PT_TESC) CreateBox(-2, x1, y1, x2, y2, tool | PMAPID(brush.rx * 4 + brush.ry * 4 + 7), -1);
  else CreateBox(-2, x1, y1, x2, y2, tool, -1);
}

void sim_tool_fill(int tool, int x, int y) {
  if (tool & TOOL_WALLS) {
    if ((tool & 0xFF) != WL_STREAM) FloodWalls(x, y, tool & 0xFF, -1);
  } else if (tool & TOOL_SIMTOOLS) {
    /* (no fill) */
  } else if (plop_tool(tool)) {
  } else if (tool == PT_TESC) {
    FloodParts(x, y, tool | PMAPID(brush.rx * 4 + brush.ry * 4 + 7), -1, -1);
  } else {
    FloodParts(x, y, tool, -1, -1);
  }
}

/* ---------------------------------------------------------------- the stickmen's keys (GameController) */
void stkm_key(int who, int key, bool down) {
  playerst *pl = who ? &player2 : &player;
  if (down) {
    if (key == STKM_RIGHT) pl->comm = (char)(pl->comm | 0x02);
    else if (key == STKM_LEFT) pl->comm = (char)(pl->comm | 0x01);
    else if (key == STKM_DOWN && (pl->comm & 0x08) != 0x08) pl->comm = (char)(pl->comm | 0x08);
    else if (key == STKM_UP && (pl->comm & 0x04) != 0x04) pl->comm = (char)(pl->comm | 0x04);
  } else {
    if (key == STKM_RIGHT || key == STKM_LEFT) {
      pl->pcomm = pl->comm;   /* the last movement */
      pl->comm = (char)(pl->comm & 12);
    } else if (key == STKM_UP) {
      pl->comm = (char)(pl->comm & 11);
    } else if (key == STKM_DOWN) {
      pl->comm = (char)(pl->comm & 7);
    }
  }
}

void stkm_frame(int rightElement) {
  if (!player.spwn || !player2.spwn) {
    int rightSelected = PT_DUST;
    if (rightElement && IsElementOrNone(rightElement)) rightSelected = rightElement;
    if (!player.spwn) Element_STKM_set_element(&player, rightSelected);
    if (!player2.spwn) Element_STKM_set_element(&player2, rightSelected);
  }
}
