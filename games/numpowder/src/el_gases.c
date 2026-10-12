/* NumPowder: The Powder Toy's gases: WTRV, O2, CO2, H2, NBLE, BOYL, CAUS, FOG (their elements/ files), ported: each one's update and the rest but graphics. */
#include "simpriv.h"

/* ---------------------------------------------------------------- WTRV */
int el_WTRV_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if ((TYP(r)==PT_RBDM||TYP(r)==PT_LRBD) && !legacy_enable && p_temp(&parts[i])>(273.15f+12.0f) && rng_chance(1, 100)) {
          //@ WTRV + RBDM/LRBD -> FIRE + RBDM/LRBD
          part_change_type(i,x,y,PT_FIRE);
          parts[i].life = 4;
          parts[i].ctype = PT_WATR;
        }
      }
    }
  }
  if(p_temp(&parts[i])>1273&&parts[i].ctype==PT_FIRE)
    p_set_temp(&parts[i], p_temp(&parts[i]) - (p_temp(&parts[i])/1000));
  return 0;
}

/* ---------------------------------------------------------------- O2 */
int el_O2_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_FIRE) {
          p_set_temp(&parts[ID(r)], p_temp(&parts[ID(r)]) + (rng_between(0, 99)));
          if (parts[ID(r)].tmp & 0x01)
            p_set_temp(&parts[ID(r)], 3473);
          parts[ID(r)].tmp |= 2;
          //@ O2 + FIRE -> 2xFIRE
          create_part(i,x,y,PT_FIRE, -1);
          p_set_temp(&parts[i], p_temp(&parts[i]) + (rng_between(0, 99)));
          parts[i].tmp |= 2;
        } else if (TYP(r)==PT_PLSM && !(parts[ID(r)].tmp&4)) {
          //@ O2 + PLSM -> FIRE + PLSM
          create_part(i,x,y,PT_FIRE, -1);
          p_set_temp(&parts[i], p_temp(&parts[i]) + (rng_between(0, 99)));
          parts[i].tmp |= 2;
        }
      }
    }
  }
  if (p_temp(&parts[i]) > 9973.15 && pv[y/ACELL][x/ACELL] > 250.0f) {
    float gx = gravx[y/ACELL][x/ACELL];
    float gy = gravy[y/ACELL][x/ACELL];
    //@ O2 -> BRMT + NEUT + PHOT + GRVT + sometimes PLSM
    if (gx*gx + gy*gy > 400) {
      if (rng_chance(1, 5)) {
        int j;
        create_part(i,x,y,PT_BRMT, -1);
        j = create_part(-3,x,y,PT_NEUT, -1);
        if (j != -1)
          p_set_temp(&parts[j], MAX_TEMP);
        j = create_part(-3,x,y,PT_PHOT, -1);
        if (j != -1) {
          p_set_temp(&parts[j], MAX_TEMP);
          parts[j].tmp = 0x1;
        }
        int rx = x + rng_between(-1, 1), ry = y + rng_between(-1, 1), r = TYP(PMAPAT(rx, ry));
        if (can_move(PT_PLSM, r) || r == PT_O2) {
          j = create_part(-3,rx,ry,PT_PLSM, -1);
          if (j > -1) {
            p_set_temp(&parts[j], MAX_TEMP);
            parts[j].tmp |= 4;
          }
        }
        j = create_part(-3,x,y,PT_GRVT, -1);
        if (j != -1)
          p_set_temp(&parts[j], MAX_TEMP);
        p_set_temp(&parts[i], MAX_TEMP);
        pv[y/ACELL][x/ACELL] = MAX_PRESSURE;
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- CO2 */
int el_CO2_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r) {
          if (parts[i].ctype==5 && rng_chance(1, 2000)) {
            //@ CO2 -> WATR
            if (create_part(-1, x+rx, y+ry, PT_WATR, -1)>=0)
              parts[i].ctype = 0;
          }
          continue;
        }
        if (TYP(r)==PT_FIRE) {
          kill_part(ID(r));
          if (rng_chance(1, 30)) {
            kill_part(i);
            return 1;
          }
        } else if ((TYP(r)==PT_WATR || TYP(r)==PT_DSTW) && rng_chance(1, 50)) {
          part_change_type(ID(r), x+rx, y+ry, PT_CBNW);
          if (parts[i].ctype==5) { //conserve number of water particles - ctype=5 means this CO2 hasn't released the water particle from BUBW yet
            //@ CO2 + WATR/DSTW -> WATR + CBNW
            create_part(i, x, y, PT_WATR, -1);
            return 0;
          } else {
            //@ CO2 + WATR/DSTW -> CBNW
            kill_part(i);
            return 1;
          }
        }
      }
    }
  }
  if (p_temp(&parts[i]) > 9773.15 && pv[y/ACELL][x/ACELL] > 200.0f) {
    if (rng_chance(1, 5)) {
      int j;
      //@ CO2 -> O2 + NEUT
      create_part(i,x,y,PT_O2, -1);
      j = create_part(-3,x,y,PT_NEUT, -1);
      if (j != -1)
        p_set_temp(&parts[j], MAX_TEMP);
      if (rng_chance(1, 50)) {
        //@ CO2 -> O2 + NEUT + ELEC
        j = create_part(-3,x,y,PT_ELEC, -1);
        if (j != -1)
          p_set_temp(&parts[j], MAX_TEMP);
      }
      p_set_temp(&parts[i], MAX_TEMP);
      pv[y/ACELL][x/ACELL] += 100;
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- H2 */
int el_H2_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (pv[y/ACELL][x/ACELL] > 8.0f && rt == PT_DESL) {
          //@ H2 + DESL -> OIL + WATR
          part_change_type(ID(r),x+rx,y+ry,PT_WATR);
          part_change_type(i,x,y,PT_OIL);
          return 1;
        }
        if (pv[y/ACELL][x/ACELL] > 45.0f) {
          if (p_temp(&parts[ID(r)]) > 2273.15)
            continue;
        } else {
          if (rt==PT_FIRE) {
            if(parts[ID(r)].tmp&0x02)
              p_set_temp(&parts[ID(r)], 3473.0f);
            else
              p_set_temp(&parts[ID(r)], 2473.15f);
            parts[ID(r)].tmp |= 1;
            //@ H2 + FIRE -> 2xFIRE
            create_part(i,x,y,PT_FIRE, -1);
            p_set_temp(&parts[i], p_temp(&parts[i]) + (rng_between(0, 99)));
            parts[i].tmp |= 1;
            return 1;
          } else if ((rt==PT_PLSM && !(parts[ID(r)].tmp&4)) || (rt==PT_LAVA && parts[ID(r)].ctype != PT_BMTL)) {
            //@ H2 + PLSM/LAVA -> FIRE + PLSM/LAVA
            create_part(i,x,y,PT_FIRE, -1);
            p_set_temp(&parts[i], p_temp(&parts[i]) + (rng_between(0, 99)));
            parts[i].tmp |= 1;
            return 1;
          }
        }
      }
    }
  }
  if (p_temp(&parts[i]) > 2273.15 && pv[y/ACELL][x/ACELL] > 50.0f) {
    if (rng_chance(1, 5)) {
      int j;
      float temp = p_temp(&parts[i]);
      create_part(i,x,y,PT_NBLE, -1);
      parts[i].tmp = 0x1;
      //@ H2 -> NBLE + NEUT + PHOT
      j = create_part(-3,x,y,PT_NEUT, -1);
      if (j>-1)
        p_set_temp(&parts[j], temp);
      if (rng_chance(1, 10)) {
        //@ H2 -> NBLE + NEUT + PHOT + ELEC
        j = create_part(-3,x,y,PT_ELEC, -1);
        if (j>-1)
          p_set_temp(&parts[j], temp);
      }
      j = create_part(-3,x,y,PT_PHOT, -1);
      if (j>-1) {
        parts[j].ctype = 0x7C0000;
        p_set_temp(&parts[j], temp);
        parts[j].tmp = 0x1;
      }
      int rx = x + rng_between(-1, 1), ry = y + rng_between(-1, 1), rt = TYP(PMAPAT(rx, ry));
      if (can_move(PT_PLSM, rt) || rt == PT_H2) {
        //@ H2 -> NBLE + NEUT + PHOT + ELEC + PLSM
        j = create_part(-3,rx,ry,PT_PLSM, -1);
        if (j>-1) {
          p_set_temp(&parts[j], temp);
          parts[j].tmp |= 4;
        }
      }
      p_set_temp(&parts[i], temp + rng_between(750, 1249));
      pv[y/ACELL][x/ACELL] += 30;
      return 1;
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- NBLE */
int el_NBLE_update(UPDATE_FUNC_ARGS) {
  if (p_temp(&parts[i]) > 5273.15 && pv[y/ACELL][x/ACELL] > 100.0f) {
    parts[i].tmp |= 0x1;
    //@ NBLE -> CO2 + NEUT + PHOT + sometimes ELEC + PLSM
    if (rng_chance(1, 5)) {
      int j;
      float temp = p_temp(&parts[i]);
      create_part(i,x,y,PT_CO2, -1);
      j = create_part(-3,x,y,PT_NEUT, -1);
      if (j != -1)
        p_set_temp(&parts[j], temp);
      if (rng_chance(1, 25)) {
        j = create_part(-3,x,y,PT_ELEC, -1);
        if (j != -1)
          p_set_temp(&parts[j], temp);
      }
      j = create_part(-3,x,y,PT_PHOT, -1);
      if (j != -1) {
        parts[j].ctype = 0xF800000;
        p_set_temp(&parts[j], temp);
        parts[j].tmp = 0x1;
      }
      int rx = x + rng_between(-1, 1), ry = y + rng_between(-1, 1), rt = TYP(PMAPAT(rx, ry));
      if (can_move(PT_PLSM, rt) || rt == PT_NBLE) {
        j = create_part(-3,rx,ry,PT_PLSM, -1);
        if (j != -1) {
          p_set_temp(&parts[j], temp);
          parts[j].tmp |= 4;
        }
      }
      p_set_temp(&parts[i], temp + 1750 + rng_between(0, 499));
      pv[y/ACELL][x/ACELL] += 50;
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- BOYL */
int el_BOYL_update(UPDATE_FUNC_ARGS) {
  float limit = p_temp(&parts[i]) / 100;
  if (pv[y / ACELL][x / ACELL] < limit)
    pv[y / ACELL][x / ACELL] += 0.001f*(limit - pv[y / ACELL][x / ACELL]);
  if (pv[y / ACELL + 1][x / ACELL] < limit)
    pv[y / ACELL + 1][x / ACELL] += 0.001f*(limit - pv[y / ACELL + 1][x / ACELL]);
  if (pv[y / ACELL - 1][x / ACELL] < limit)
    pv[y / ACELL - 1][x / ACELL] += 0.001f*(limit - pv[y / ACELL - 1][x / ACELL]);
  pv[y / ACELL][x / ACELL + 1]  += 0.001f*(limit - pv[y / ACELL][x / ACELL + 1]);
  pv[y / ACELL + 1][x / ACELL + 1] += 0.001f*(limit - pv[y / ACELL + 1][x / ACELL + 1]);
  pv[y / ACELL][x / ACELL - 1]  += 0.001f*(limit - pv[y / ACELL][x / ACELL - 1]);
  pv[y / ACELL - 1][x / ACELL - 1] += 0.001f*(limit - pv[y / ACELL - 1][x / ACELL - 1]);
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_WATR) {
          //@ BOYL + WATR -> BOYL + FOG
          if (rng_chance(1, 30))
            part_change_type(ID(r),x+rx,y+ry,PT_FOG);
        } else if (TYP(r)==PT_O2) {
          //@ BOYL + O2 -> WATR
          if (rng_chance(1, 9)) {
            kill_part(ID(r));
            part_change_type(i,x,y,PT_WATR);
            pv[y/ACELL][x/ACELL] += 4.0;
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- CAUS */
int el_CAUS_update(UPDATE_FUNC_ARGS) {
  bool converted = false;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r) == PT_GAS) {
          if (pv[(y+ry)/ACELL][(x+rx)/ACELL] > 3) {
            //@ CAUS + GAS -> 2xRFRG
            part_change_type(ID(r), x+rx, y+ry, PT_RFRG);
            part_change_type(i, x, y, PT_RFRG);
            converted = true;
          }
        } else if (TYP(r) != PT_ACID && TYP(r) != PT_CAUS && TYP(r) != PT_RFRG && TYP(r) != PT_RFGL) {
          if ((TYP(r) != PT_CLNE && TYP(r) != PT_PCLN && ((TYP(r) != PT_FOG && TYP(r) != PT_RIME) || parts[ID(r)].tmp <= 5) && rng_chance(elements[TYP(r)].Hardness, 1000)) && parts[i].life > 50) {
            // GLAS protects stuff from acid
            if (parts_avg(i, ID(r),PT_GLAS) != PT_GLAS) {
              float newtemp = ((60.0f - (float)elements[TYP(r)].Hardness)) * 7.0f;
              if (newtemp < 0)
                newtemp = 0;
              p_set_temp(&parts[i], p_temp(&parts[i]) + (newtemp));
              parts[i].life--;
              kill_part(ID(r));
            }
          } else if (parts[i].life <= 50) {
            kill_part(i);
            return 1;
          }
        }
      }
    }
  }
  return converted;
}

/* ---------------------------------------------------------------- FOG */
int el_FOG_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if ((elements[TYP(r)].Properties&TYPE_SOLID) && rng_chance(1, 10) && parts[i].life==0 && !(TYP(r)==PT_CLNE || TYP(r)==PT_PCLN || TYP(r)==PT_BCLN || TYP(r)==PT_PBCN)) {
          //@ FOG -> RIME
          part_change_type(i,x,y,PT_RIME);
        }
        if (TYP(r)==PT_SPRK) {
          parts[i].life += rng_between(0, 19);
        }
        // GAS increases acidity
        if (TYP(r) == PT_GAS && parts[i].tmp < 10) {
          kill_part(ID(r));
          if (parts[i].ctype == PT_DSTW)
            parts[i].ctype = 0;
          else
            parts[i].tmp++;
        }
      }
    }
  }
  return 0;
}

