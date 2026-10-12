/* NumPowder: The Powder Toy's sensors (their files in elements/), ported: DTEC, TSNS, PSNS, LSNS, VSNS, LDTC, INVIS. */
#include "simpriv.h"

/* ---------------------------------------------------------------- DTEC */
int el_DTEC_update(UPDATE_FUNC_ARGS) {
  int rd = parts[i].tmp2;
  if (rd > 25) parts[i].tmp2 = rd = 25;
  if (parts[i].life) {
    parts[i].life = 0;
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          int rt = TYP(r);
          int pavg = parts_avg(i,ID(r),PT_INSL);
          if (pavg != PT_INSL && pavg != PT_RSSS) {
            if ((elements[rt].Properties&PROP_CONDUCTS) && !(rt==PT_WATR||rt==PT_SLTW||rt==PT_NTCT||rt==PT_PTCT||rt==PT_INWR) && parts[ID(r)].life==0) {
              parts[ID(r)].life = 4;
              parts[ID(r)].ctype = rt;
              part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
            }
          }
        }
      }
    }
  }
  bool setFilt = false;
  int photonWl = 0;
  for (int rx =-rd; rx<rd+1; rx++) {
    for (int ry =-rd; ry<rd+1; ry++) {
      if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry)) {
        int r = PMAPAT(x+rx, y+ry);
        if(!r)
          r = PHOTAT(x+rx, y+ry);
        if(!r)
          continue;
        if (TYP(r) == parts[i].ctype && (parts[i].ctype != PT_LIFE || parts[i].tmp == parts[ID(r)].ctype || !parts[i].tmp))
          parts[i].life = 1;
        if (TYP(r) == PT_PHOT || (TYP(r) == PT_BRAY && parts[ID(r)].tmp!=2) || TYP(r) == PT_BIZR || TYP(r) == PT_BIZRG || TYP(r) == PT_BIZRS) {
          setFilt = true;
          photonWl = parts[ID(r)].ctype;
        }
      }
    }
  }
  if (setFilt) {
    int nx, ny;
    for (int rx =-1; rx<2; rx++) {
      for (int ry =-1; ry<2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          nx = x+rx;
          ny = y+ry;
          while (TYP(r)==PT_FILT) {
            parts[ID(r)].ctype = photonWl;
            nx += rx;
            ny += ry;
            if (nx<0 || ny<0 || nx>=XRES || ny>=YRES)
              break;
            r = PMAPAT(nx, ny);
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- TSNS */
int el_TSNS_update(UPDATE_FUNC_ARGS) {
  int rd = parts[i].tmp2;
  if (rd > 25)
    parts[i].tmp2 = rd = 25;
  if (parts[i].life) {
    parts[i].life = 0;
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            r = PHOTAT(x+rx, y+ry);
          if (!r)
            continue;
          int rt = TYP(r);
          int pavg = parts_avg(i, ID(r), PT_INSL);
          if (pavg != PT_INSL && pavg != PT_RSSS) {
            if ((elements[rt].Properties&PROP_CONDUCTS) && !(rt == PT_WATR || rt == PT_SLTW || rt == PT_NTCT || rt == PT_PTCT || rt == PT_INWR) && parts[ID(r)].life == 0) {
              parts[ID(r)].life = 4;
              parts[ID(r)].ctype = rt;
              part_change_type(ID(r), x+rx, y+ry, PT_SPRK);
            }
          }
        }
      }
    }
  }
  bool setFilt = false;
  int photonWl = 0;
  for (int rx = -rd; rx <= rd; rx++)
    for (int ry = -rd; ry <= rd; ry++)
      if (x + rx >= 0 && y + ry >= 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          r = PHOTAT(x+rx, y+ry);
        if (!r)
          continue;
        if (parts[i].tmp == 0 && TYP(r) != PT_TSNS && TYP(r) != PT_METL && p_temp(&parts[ID(r)]) > p_temp(&parts[i]))
          parts[i].life = 1;
        if (parts[i].tmp == 2 && TYP(r) != PT_TSNS && TYP(r) != PT_METL && p_temp(&parts[ID(r)]) < p_temp(&parts[i]))
          parts[i].life = 1;
        if (parts[i].tmp == 1 && TYP(r) != PT_TSNS && TYP(r) != PT_FILT) {
          setFilt = true;
          photonWl = (int)(p_temp(&parts[ID(r)]));
        }
      }
  if (setFilt) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          int nx = x + rx;
          int ny = y + ry;
          while (TYP(r) == PT_FILT) {
            parts[ID(r)].ctype = 0x10000000 + photonWl;
            nx += rx;
            ny += ry;
            if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
              break;
            r = PMAPAT(nx, ny);
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- PSNS */
int el_PSNS_update(UPDATE_FUNC_ARGS) {
  if ((parts[i].tmp == 0 && pv[y/ACELL][x/ACELL] > p_temp(&parts[i])-273.15f) || (parts[i].tmp == 2 && pv[y/ACELL][x/ACELL] < p_temp(&parts[i])-273.15f)) {
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          int pavg = parts_avg(i,ID(r),PT_INSL);
          if (pavg != PT_INSL && pavg != PT_RSSS) {
            int rt = TYP(r);
            if ((elements[rt].Properties&PROP_CONDUCTS) && !(rt==PT_WATR||rt==PT_SLTW||rt==PT_NTCT||rt==PT_PTCT||rt==PT_INWR) && parts[ID(r)].life==0) {
              parts[ID(r)].life = 4;
              parts[ID(r)].ctype = rt;
              part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
            }
          }
        }
      }
    }
  }
  if (parts[i].tmp == 1) {
    bool setFilt = true;
    float photonWl = pv[y / ACELL][x / ACELL];
    if (setFilt) {
      for (int rx = -1; rx <= 1; rx++) {
        for (int ry = -1; ry <= 1; ry++) {
          if (rx || ry) {
            int r = PMAPAT(x + rx, y + ry);
            if (!r)
              continue;
            int nx = x + rx;
            int ny = y + ry;
            while (TYP(r) == PT_FILT) {
              parts[ID(r)].ctype = 0x10000000 + (int)(roundf(photonWl) - MIN_PRESSURE);
              nx += rx;
              ny += ry;
              if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
                break;
              r = PMAPAT(nx, ny);
            }
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- LSNS */
int el_LSNS_update(UPDATE_FUNC_ARGS) {
  int rd = parts[i].tmp2;
  if (rd > 25) parts[i].tmp2 = rd = 25;
  if (parts[i].life) {
    parts[i].life = 0;
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x + rx, y + ry);
          if (!r)
            r = PHOTAT(x + rx, y + ry);
          if (!r)
            continue;
          int rt = TYP(r);
          int pavg = parts_avg(i, ID(r), PT_INSL);
          if (pavg != PT_INSL && pavg != PT_RSSS) {
            if ((elements[rt].Properties&PROP_CONDUCTS) && !(rt == PT_WATR || rt == PT_SLTW || rt == PT_NTCT || rt == PT_PTCT || rt == PT_INWR) && parts[ID(r)].life == 0) {
              parts[ID(r)].life = 4;
              parts[ID(r)].ctype = rt;
              part_change_type(ID(r), x + rx, y + ry, PT_SPRK);
            }
          }
        }
      }
    }
  }
  bool doSerialization = false;
  bool doDeserialization = false;
  int life = 0;
  for (int rx = -rd; rx < rd + 1; rx++) {
    for (int ry = -rd; ry < rd + 1; ry++) {
      if (x + rx >= 0 && y + ry >= 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
        int r = PMAPAT(x + rx, y + ry);
        if (!r)
          r = PHOTAT(x + rx, y + ry);
        if (!r)
          continue;
        switch (parts[i].tmp) {
        case 1:
          // .life serialization into FILT
          if (TYP(r) != PT_LSNS && TYP(r) != PT_FILT && parts[ID(r)].life >= 0) {
            doSerialization = true;
            life = parts[ID(r)].life;
          }
          break;
        case 3:
          // .life deserialization
          if (TYP(r) == PT_FILT) {
            doDeserialization = true;
            life = parts[ID(r)].ctype;
          }
          break;
        case 2:
          // Invert mode
          if (TYP(r) != PT_METL && parts[ID(r)].life <= p_temp(&parts[i]) - 273.15)
            parts[i].life = 1;
          break;
        default:
          // Normal mode
          if (TYP(r) != PT_METL && parts[ID(r)].life > p_temp(&parts[i]) - 273.15)
            parts[i].life = 1;
          break;
        }
      }
    }
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x + rx, y + ry);
        if (!r)
          r = PHOTAT(x + rx, y + ry);
        if (!r)
          continue;
        int nx = x + rx;
        int ny = y + ry;
        // .life serialization.
        if (doSerialization) {
          while (TYP(r) == PT_FILT) {
            parts[ID(r)].ctype = 0x10000000 + life;
            nx += rx;
            ny += ry;
            if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
              break;
            r = PMAPAT(nx, ny);
          }
        }
        // .life deserialization.
        else if (doDeserialization) {
          if (TYP(r) != PT_FILT)
            parts[ID(r)].life = life - 0x10000000;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- VSNS */
int el_VSNS_update(UPDATE_FUNC_ARGS) {
  int rd = parts[i].tmp2;
  if (rd > 25) parts[i].tmp2 = rd = 25;
  if (parts[i].life) {
    parts[i].life = 0;
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x + rx, y + ry);
          if (!r)
            continue;
          int rt = TYP(r);
          int pavg = parts_avg(i, ID(r), PT_INSL);
          if (pavg != PT_INSL && pavg != PT_RSSS) {
            if ((elements[rt].Properties &PROP_CONDUCTS) && !(rt == PT_WATR || rt == PT_SLTW || rt == PT_NTCT || rt == PT_PTCT || rt == PT_INWR) && parts[ID(r)].life == 0) {
              parts[ID(r)].life = 4;
              parts[ID(r)].ctype = rt;
              part_change_type(ID(r), x + rx, y + ry, PT_SPRK);
            }
          }
        }
      }
    }
  }
  bool doSerialization = false;
  bool doDeserialization = false;
  float Vs = 0;
  for (int rx = -rd; rx < rd + 1; rx++)
    for (int ry = -rd; ry < rd + 1; ry++)
      if (x + rx >= 0 && y + ry >= 0 && x + rx < XRES && y + ry < YRES && (rx || ry)) {
        int r = PMAPAT(x + rx, y + ry);
        if (!r)
          r = PHOTAT(x + rx, y + ry);
        if (!r)
          continue;
        float Vx = p_vx(&parts[ID(r)]);
        float Vy = p_vy(&parts[ID(r)]);
        float Vm = sqrtf(Vx*Vx + Vy*Vy);
        switch (parts[i].tmp) {
        case 1:
          // serialization
          if (TYP(r) != PT_VSNS && TYP(r) != PT_FILT && !(elements[TYP(r)].Properties & TYPE_SOLID)) {
            doSerialization = true;
            Vs = Vm;
          }
          break;
        case 3:
          // deserialization
          if (TYP(r) == PT_FILT) {
            int vel = parts[ID(r)].ctype - 0x10000000;
            if (vel >= 0 && vel < MAX_VELOCITY) {
              doDeserialization = true;
              Vs = (float)(vel);
            }
          }
          break;
        case 2:
          // Invert mode
          if (!(elements[TYP(r)].Properties & TYPE_SOLID) && Vm <= p_temp(&parts[i]) - 273.15)
            parts[i].life = 1;
          break;
        default:
          // Normal mode
          if (!(elements[TYP(r)].Properties & TYPE_SOLID) && Vm > p_temp(&parts[i]) - 273.15)
            parts[i].life = 1;
          break;
        }
      }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x + rx, y + ry);
        if (!r)
          r = PHOTAT(x + rx, y + ry);
        if (!r)
          continue;
        int nx = x + rx;
        int ny = y + ry;
        //Serialization.
        if (doSerialization) {
          while (TYP(r) == PT_FILT) {
            parts[ID(r)].ctype = 0x10000000 + (int)(Vs + 0.5f);
            nx += rx;
            ny += ry;
            if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
              break;
            r = PMAPAT(nx, ny);
          }
        }
        //Deserialization.
        if (doDeserialization) {
          if (TYP(r) != PT_FILT && !(elements[TYP(r)].Properties & TYPE_SOLID)) {
            float Vx = p_vx(&parts[ID(r)]);
            float Vy = p_vy(&parts[ID(r)]);
            float Vm = sqrtf(Vx*Vx + Vy*Vy);
            if (Vm > 0) {
              p_set_vx(&parts[ID(r)], p_vx(&parts[ID(r)]) * (Vs / Vm));
              p_set_vy(&parts[ID(r)], p_vy(&parts[ID(r)]) * (Vs / Vm));
            }
            break;
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- LDTC */
enum { FLAG_INVERT_FILTER = 0x1, FLAG_IGNORE_ENERGY = 0x2, FLAG_NO_COPY_COLOR = 0x4, FLAG_KEEP_SEARCHING = 0x8 };
//NOTES:
// ctype is used to store the target element, if any. (NONE is treated as a wildcard)
// life is used for the amount of pixels to skip before starting the scan. Starts just in front of the LDTC if 0.
// tmp is the number of particles that will be scanned before scanning stops. Unbounded if 0.
// tmp2 is used for settings (binary flags). The flags are as follows:
// 0x01: Inverts the CTYPE filter so that the element in ctype is the only thing that doesn't trigger LDTC, instead of the opposite.
// 0x02: Ignore energy particles
// 0x04: Ignore FILT (do not use color copying mode)
// 0x08: Keep searching even after finding a particle
/* Returns true for particles that activate the special FILT color copying mode */
static bool LDTC_phot_data_type(int rt) {
  return rt == PT_FILT || rt == PT_PHOT || rt == PT_BRAY || rt == PT_BIZR || rt == PT_BIZRG || rt == PT_BIZRS;
}
/* Returns true for particles that start a ray search ("dtec" mode)
 */
static bool LDTC_accepted_conductor(int r) {
  int rt = TYP(r);
  return (elements[rt].Properties & PROP_CONDUCTS) &&
    !(rt == PT_WATR || rt == PT_SLTW || rt == PT_NTCT ||
    rt == PT_PTCT || rt == PT_INWR) &&
    parts[ID(r)].life == 0;
}
int el_LDTC_update(UPDATE_FUNC_ARGS) {
  int ctype = TYP(parts[i].ctype), ctypeExtra = ID(parts[i].ctype), detectLength = parts[i].tmp, detectSpaces = parts[i].tmp2;
  bool copyColor = !(parts[i].tmp2 & FLAG_NO_COPY_COLOR);
  bool ignoreEnergy = parts[i].tmp2 & FLAG_IGNORE_ENERGY;
  bool invertFilter = parts[i].tmp2 & FLAG_INVERT_FILTER;
  bool keepSearching = parts[i].tmp2 & FLAG_KEEP_SEARCHING;
  if (detectSpaces < 0)
    detectSpaces = parts[i].tmp2 = 0;
  if (detectLength < 0)
    detectLength = parts[i].tmp = 0;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        bool boolMode = LDTC_accepted_conductor(r);
        bool filtMode = copyColor && TYP(r) == PT_FILT;
        if (!boolMode && !filtMode)
          continue;
        int maxRange = parts[i].life + parts[i].tmp;
        int xStep = rx * -1, yStep = ry * -1;
        int xCurrent = x + (xStep * (parts[i].life + 1)), yCurrent = y + (yStep * (parts[i].life + 1));
        for (; !parts[i].tmp ||
          (xStep * (xCurrent - x) <= maxRange &&
          yStep * (yCurrent - y) <= maxRange);
          xCurrent += xStep, yCurrent += yStep) {
          if (!(xCurrent>=0 && yCurrent>=0 && xCurrent<XRES && yCurrent<YRES))
            break; // We're out of bounds! Oops!
          int rr = PMAPAT(xCurrent, yCurrent);
          if (!rr && !ignoreEnergy)
            rr = PHOTAT(xCurrent, yCurrent);
          if (!rr)
            continue;
          // If ctype isn't set (no type restriction), or ctype matches what we found
          // Can use .tmp2 flag to invert this
          bool matchesCtype = ctype == TYP(rr) && (ctype != PT_LIFE || parts[ID(rr)].ctype == ctypeExtra);
          bool matchesFilter = !ctype || (invertFilter ^ (int)matchesCtype);
          if (!matchesFilter) {
            if (keepSearching)
              continue;
            else
              break;
          }
          // room for more conditions here.
          if (boolMode) {
            parts[ID(r)].life = 4;
            parts[ID(r)].ctype = TYP(r);
            part_change_type(ID(r), x + rx, y + ry, PT_SPRK);
            break;
          }
          if (filtMode) {
            if (!LDTC_phot_data_type(TYP(rr)))
              continue;
            int nx = x + rx, ny = y + ry;
            int photonWl = TYP(rr) == PT_FILT ?
              Element_FILT_getWavelengths(&parts[ID(rr)]) :
              parts[ID(rr)].ctype;
            while (TYP(r) == PT_FILT) {
              parts[ID(r)].ctype = photonWl;
              nx += rx;
              ny += ry;
              if (nx < 0 || ny < 0 || nx >= XRES || ny >= YRES)
                break;
              r = PMAPAT(nx, ny);
            }
            break;
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- INVIS */
int el_INVIS_update(UPDATE_FUNC_ARGS) {
  float pressureResistance = 0.0f;
  if (parts[i].tmp > 0)
    pressureResistance = (float) parts[i].tmp;
  else
    pressureResistance = 4.0f;
  if (pv[y/ACELL][x/ACELL] < -pressureResistance || pv[y/ACELL][x/ACELL] > pressureResistance)
    parts[i].tmp2 = 1;
  else
    parts[i].tmp2 = 0;
  return 0;
}

