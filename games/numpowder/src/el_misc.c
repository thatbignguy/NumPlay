/* NumPowder: The Powder Toy's element functions shared by several elements (Element.cpp): drawing an element
 * on one to set its ctype, and the update for when the heat simulation is off (legacyUpdate). */
#include "simpriv.h"

bool basicCtypeDraw(CTYPEDRAW_FUNC_ARGS) {
  if (parts[i].type == t || elements[t].Properties & PROP_NOCTYPEDRAW) return false;
  parts[i].ctype = t;
  return true;
}

/* CLNE, BCLN, DTEC...: LIFE's rule goes in tmp */
bool ctypeDrawVInTmp(CTYPEDRAW_FUNC_ARGS) {
  if (!basicCtypeDraw(CTYPEDRAW_FUNC_SUBCALL_ARGS)) return false;
  if (t == PT_LIFE) parts[i].tmp = (int16_t)v;   /* (the built-in rules' numbers: custom rules aren't offered) */
  return true;
}

bool ctypeDrawVInCtype(CTYPEDRAW_FUNC_ARGS) {
  if (!basicCtypeDraw(CTYPEDRAW_FUNC_SUBCALL_ARGS)) return false;
  if (t == PT_LIFE) parts[i].ctype |= PMAPID(v);
  return true;
}

/* when the heat simulation is off: water boils near fire, ice melts... (Element::legacyUpdate) */
int Element_legacyUpdate(UPDATE_FUNC_ARGS) {
  int r, rx, ry;
  int t = parts[i].type;
  if (t == PT_WTRV) {
    for (rx = -2; rx < 3; rx++)
      for (ry = -2; ry < 3; ry++)
        if (x + rx >= 0 && y + ry > 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
          r = PMAPAT(x + rx, y + ry);
          if (!r) continue;
          if ((TYP(r) == PT_WATR || TYP(r) == PT_DSTW || TYP(r) == PT_SLTW) && rng_chance(1, 1000)) {
            part_change_type(i, x, y, PT_WATR);
            part_change_type(ID(r), x + rx, y + ry, PT_WATR);
          }
          if ((TYP(r) == PT_ICEI || TYP(r) == PT_SNOW) && rng_chance(1, 1000)) {
            part_change_type(i, x, y, PT_WATR);
            if (rng_chance(1, 1000)) part_change_type(ID(r), x + rx, y + ry, PT_WATR);
          }
        }
  } else if (t == PT_WATR) {
    for (rx = -2; rx < 3; rx++)
      for (ry = -2; ry < 3; ry++)
        if (x + rx >= 0 && y + ry > 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
          r = PMAPAT(x + rx, y + ry);
          if (!r) continue;
          if ((TYP(r) == PT_FIRE || TYP(r) == PT_LAVA) && rng_chance(1, 10)) part_change_type(i, x, y, PT_WTRV);
        }
  } else if (t == PT_SLTW) {
    for (rx = -2; rx < 3; rx++)
      for (ry = -2; ry < 3; ry++)
        if (x + rx >= 0 && y + ry > 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
          r = PMAPAT(x + rx, y + ry);
          if (!r) continue;
          if ((TYP(r) == PT_FIRE || TYP(r) == PT_LAVA) && rng_chance(1, 10)) {
            if (rng_chance(1, 4)) part_change_type(i, x, y, PT_SALT);
            else part_change_type(i, x, y, PT_WTRV);
          }
        }
  } else if (t == PT_DSTW) {
    for (rx = -2; rx < 3; rx++)
      for (ry = -2; ry < 3; ry++)
        if (x + rx >= 0 && y + ry > 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
          r = PMAPAT(x + rx, y + ry);
          if (!r) continue;
          if ((TYP(r) == PT_FIRE || TYP(r) == PT_LAVA) && rng_chance(1, 10)) part_change_type(i, x, y, PT_WTRV);
        }
  } else if (t == PT_ICEI) {
    for (rx = -2; rx < 3; rx++)
      for (ry = -2; ry < 3; ry++)
        if (x + rx >= 0 && y + ry > 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
          r = PMAPAT(x + rx, y + ry);
          if (!r) continue;
          if ((TYP(r) == PT_WATR || TYP(r) == PT_DSTW) && rng_chance(1, 1000)) {
            part_change_type(i, x, y, PT_ICEI);
            part_change_type(ID(r), x + rx, y + ry, PT_ICEI);
          }
        }
  } else if (t == PT_SNOW) {
    for (rx = -2; rx < 3; rx++)
      for (ry = -2; ry < 3; ry++)
        if (x + rx >= 0 && y + ry > 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
          r = PMAPAT(x + rx, y + ry);
          if (!r) continue;
          if ((TYP(r) == PT_WATR || TYP(r) == PT_DSTW) && rng_chance(1, 1000)) {
            part_change_type(i, x, y, PT_ICEI);
            part_change_type(ID(r), x + rx, y + ry, PT_ICEI);
          }
          if ((TYP(r) == PT_WATR || TYP(r) == PT_DSTW) && rng_chance(3, 200)) part_change_type(i, x, y, PT_WATR);
        }
  }
  if (t == PT_WTRV && pv[y / ACELL][x / ACELL] > 4.0f) part_change_type(i, x, y, PT_DSTW);
  if (t == PT_OIL && pv[y / ACELL][x / ACELL] < -6.0f) part_change_type(i, x, y, PT_GAS);
  if (t == PT_GAS && pv[y / ACELL][x / ACELL] > 6.0f) part_change_type(i, x, y, PT_OIL);
  if (t == PT_DESL && pv[y / ACELL][x / ACELL] > 12.0f) {
    part_change_type(i, x, y, PT_FIRE);
    parts[i].life = (int16_t)rng_between(120, 169);
  }
  return 0;
}
