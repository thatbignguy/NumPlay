/* NumPowder: The Powder Toy's liquids (their files in elements/), ported: WATR, DSTW, SLTW, CBNW, ACID, GLOW, MERC,
 * GEL, SOAP, FRZW, BIZR (and BIZRG, BIZRS), RFRG (and RFGL), RSST, BASE, VIRS (and VRSS, VRSG). */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("O3")   /* (a hot path: a few KB of flash for speed) */
#endif
#include "simpriv.h"

/* ---------------------------------------------------------------- WATR */
int el_WATR_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SALT && rng_chance(1, 50)) {
          //@ WATR + SALT -> SLTW + SALT
          part_change_type(i,x,y,PT_SLTW);
          // on average, convert 3 WATR to SLTW before SALT turns into SLTW
          if (rng_chance(1, 3))
            part_change_type(ID(r),x+rx,y+ry,PT_SLTW);
        } else if ((TYP(r)==PT_RBDM||TYP(r)==PT_LRBD) && (legacy_enable||p_temp(&parts[i])>(273.15f+12.0f)) && rng_chance(1, 100)) {
          //@ WATR + RBDM/LRBD -> FIRE + RBDM/LRBD
          part_change_type(i,x,y,PT_FIRE);
          parts[i].life = 4;
          parts[i].ctype = PT_WATR;
        } else if (TYP(r)==PT_FIRE && parts[ID(r)].ctype!=PT_WATR) {
          kill_part(ID(r));
          if (rng_chance(1, 30)) {
            kill_part(i);
            return 1;
          }
        } else if (TYP(r)==PT_SLTW && rng_chance(1, 2000)) {
          //@ WATR + SLTW -> 2xSLTW
          part_change_type(i,x,y,PT_SLTW);
        } else if (TYP(r)==PT_ROCK && fabsf(p_vx(&parts[i]))+fabsf(p_vy(&parts[i])) >= 0.5 && rng_chance(1, 1000)) { // ROCK erosion
          //@ WATR + ROCK -> WATR + SAND/STNE
          if (rng_chance(1,3))
            part_change_type(ID(r),x+rx,y+ry,PT_SAND);
          else
            part_change_type(ID(r),x+rx,y+ry,PT_STNE);
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- DSTW */
int el_DSTW_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        switch (TYP(r)) {
        case PT_SALT:
          if (rng_chance(1, 50)) {
            //@ DSTW + SALT -> 2xSLTW
            part_change_type(i,x,y,PT_SLTW);
            // on average, convert 3 DSTW to SLTW before SALT turns into SLTW
            if (rng_chance(1, 3))
              part_change_type(ID(r),x+rx,y+ry,PT_SLTW);
          }
          break;
        case PT_SLTW:
          if (rng_chance(1, 2000)) {
            //@ DSTW + SLTW -> 2xSLTW
            part_change_type(i,x,y,PT_SLTW);
            break;
          }
          /* fallthrough */
        case PT_WATR:
          if (rng_chance(1, 100)) {
            //@ DSTW + WATR -> 2xWATR
            part_change_type(i,x,y,PT_WATR);
          }
          break;
        case PT_RBDM:
        case PT_LRBD:
          if ((legacy_enable||p_temp(&parts[i])>12.0f) && rng_chance(1, 100)) {
            //@ DSTW + RBDM/LRBD -> FIRE + RBDM/LRBD
            part_change_type(i,x,y,PT_FIRE);
            parts[i].life = 4;
          }
          break;
        case PT_FIRE:
          kill_part(ID(r));
          if (rng_chance(1, 30)) {
            kill_part(i);
            return 1;
          }
          break;
        case PT_SMKE:
          if (p_temp(&parts[ID(r)]) > (40 + 273.15f) && p_temp(&parts[ID(r)]) < (60 + 273.15f) &&
            p_temp(&parts[i]) > (40 + 273.15f) && p_temp(&parts[i]) < (60 + 273.15f)) {
            if (rng_chance(1, 100)) {
              //@ DSTW + SMKE -> BASE
              part_change_type(i,x,y,PT_BASE);
              parts[i].life = 1;
              kill_part(ID(r));
            }
          }
          break;
        default:
          continue;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SLTW */
int el_SLTW_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        switch (TYP(r)) {
        case PT_SALT:
          //@ SLTW + SALT -> 2xSLTW
          if (rng_chance(1, 2000))
            part_change_type(ID(r),x+rx,y+ry,PT_SLTW);
          break;
        case PT_PLNT:
          if (rng_chance(1, 40))
            kill_part(ID(r));
          break;
        case PT_RBDM:
        case PT_LRBD:
          //@ SLTW + RBDM/LRBD -> FIRE + RBDM/LRBD
          if ((legacy_enable||p_temp(&parts[i])>(273.15f+12.0f)) && rng_chance(1, 100)) {
            part_change_type(i,x,y,PT_FIRE);
            parts[i].life = 4;
            parts[i].ctype = PT_WATR;
          }
          break;
        case PT_FIRE:
          if (parts[ID(r)].ctype!=PT_WATR) {
            kill_part(ID(r));
            if (rng_chance(1, 30)) {
              kill_part(i);
              return 1;
            }
          }
          break;
        case PT_NONE:
          break;
        default:
          continue;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- CBNW */
int el_CBNW_update(UPDATE_FUNC_ARGS) {
  if (pv[y/ACELL][x/ACELL]<=3) {
    if (pv[y/ACELL][x/ACELL] <= -0.5 || rng_chance(1, 4000)) {
      //@ CBNW -> CO2
      part_change_type(i,x,y,PT_CO2);
      parts[i].ctype = 5;
      pv[y/ACELL][x/ACELL] += 0.5f;
    }
  }
  if (parts[i].tmp2!=20) {
    parts[i].tmp2 -= (parts[i].tmp2>20)?1:-1;
  } else if (rng_chance(1, 200)) {
    parts[i].tmp2 = rng_between(0, 39);
  }
  if(parts[i].tmp>0) {
    //Explode
    if(parts[i].tmp==1 && rng_chance(3, 4)) {
      //@ CBNW -> CO2
      part_change_type(i,x,y,PT_CO2);
      parts[i].ctype = 5;
      pv[y/ACELL][x/ACELL] += 0.2f;
    }
    parts[i].tmp--;
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if ((elements[TYP(r)].Properties&TYPE_PART) && parts[i].tmp == 0 && rng_chance(1, 83)) {
          //Start explode
          parts[i].tmp = rng_between(0, 24);
        } else if((elements[TYP(r)].Properties&TYPE_SOLID) && TYP(r)!=PT_DMND && TYP(r)!=PT_GLAS && parts[i].tmp == 0 && rng_chance((int)(2 - pv[y/ACELL][x/ACELL]), 6667)) {
          //@ CBNW -> CO2
          part_change_type(i,x,y,PT_CO2);
          parts[i].ctype = 5;
          pv[y/ACELL][x/ACELL] += 0.2f;
        }
        if (TYP(r)==PT_CBNW) {
          if(!parts[i].tmp) {
            if (parts[ID(r)].tmp) {
              parts[i].tmp = parts[ID(r)].tmp;
              if((ID(r))>i) //If the other particle hasn't been life updated
                parts[i].tmp--;
            }
          } else if(!parts[ID(r)].tmp) {
            parts[ID(r)].tmp = parts[i].tmp;
            if((ID(r))>i) //If the other particle hasn't been life updated
              parts[ID(r)].tmp++;
          }
        } else if (TYP(r)==PT_RBDM||TYP(r)==PT_LRBD) {
          if ((legacy_enable||p_temp(&parts[i])>(273.15f+12.0f)) && rng_chance(1, 166)) {
            //@ CBNW + RBDM/LRBD -> CBNW + FIRE
            part_change_type(i,x,y,PT_FIRE);
            parts[i].life = 4;
            parts[i].ctype = PT_WATR;
          }
        } else if (TYP(r)==PT_FIRE && parts[ID(r)].ctype!=PT_WATR){
          kill_part(ID(r));
          if (rng_chance(1, 50)) {
            kill_part(i);
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- ACID */
int el_ACID_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt != PT_ACID && rt != PT_CAUS) {
          if (rt == PT_PLEX || rt == PT_NITR || rt == PT_GUNP || rt == PT_RBDM || rt == PT_LRBD) {
            //@ ACID + PLEX/NITR/GUNP/RBDM/LRBD -> 2xFIRE
            part_change_type(i,x,y,PT_FIRE);
            part_change_type(ID(r),x+rx,y+ry,PT_FIRE);
            parts[i].life = 4;
            parts[ID(r)].life = 4;
          } else if (rt == PT_WTRV) {
            if (rng_chance(1, 250)) {
              //@ ACID + WTRV -> CAUS
              part_change_type(i, x, y, PT_CAUS);
              parts[i].life = rng_between(25, 74);
              kill_part(ID(r));
            }
          } else if (rt != PT_CLNE && rt != PT_PCLN && ((rt != PT_FOG && rt != PT_RIME) || parts[ID(r)].tmp <= 5) && parts[i].life > 50 && rng_chance(elements[rt].Hardness, 1000)) {
            if (parts_avg(i, ID(r),PT_GLAS)!= PT_GLAS) { //GLAS protects stuff from acid
              float newtemp = ((60.0f-(float)elements[rt].Hardness))*7.0f;
              if(newtemp < 0){
                newtemp = 0;
              }
              p_set_temp(&parts[i], p_temp(&parts[i]) + (newtemp));
              parts[i].life--;
              switch (rt) {
              case PT_LITH:
                //@ ACID + LITH -> ACID + H2
                part_change_type(ID(r), x + rx, y + ry, PT_H2);
                break;
              default:
                kill_part(ID(r));
                break;
              }
            }
          } else if (parts[i].life<=50) {
            kill_part(i);
            return 1;
          }
        }
      }
    }
  }
  for (int trade = 0; trade<2; trade++) {
    int rx = rng_between(-2, 2);
    int ry = rng_between(-2, 2);
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      if (TYP(r) == PT_ACID && (parts[i].life > parts[ID(r)].life) && parts[i].life>0) { //diffusion
        int temp = parts[i].life - parts[ID(r)].life;
        if (temp == 1) {
          parts[ID(r)].life++;
          parts[i].life--;
        } else if (temp>0) {
          parts[ID(r)].life += temp/2;
          parts[i].life -= temp/2;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- GLOW */
int el_GLOW_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_WATR && rng_chance(1, 400)) {
          //@ GLOW + WATR -> DEUT
          kill_part(i);
          part_change_type(ID(r),x+rx,y+ry,PT_DEUT);
          parts[ID(r)].life = 10;
          return 1;
        } else if (TYP(r) == PT_GEL) {
          //@ GLOW + GEL -> RSST
          kill_part(i);
          part_change_type(ID(r),x+rx,y+ry,PT_RSST);
          parts[ID(r)].tmp = 0;
          return 1;
        }
      }
    }
  }
  int ctype = (int)(pv[y/ACELL][x/ACELL]*16);
  if (ctype < 0)
    ctype = 0;
  parts[i].ctype = ctype;
  parts[i].tmp = abs((int)((vx[y/ACELL][x/ACELL]+vy[y/ACELL][x/ACELL])*16.0f)) + abs((int)((p_vx(&parts[i])+p_vy(&parts[i]))*64.0f));
  return 0;
}

/* ---------------------------------------------------------------- MERC */
int el_MERC_update(UPDATE_FUNC_ARGS) {
  // Max number of particles that can be condensed into one
  const int absorbScale = 10000;
  p_set_temp(&parts[i], restrict_flt(p_temp(&parts[i]), MIN_TEMP, MAX_TEMP));
  int maxtmp = (int)(absorbScale/(p_temp(&parts[i]) + 1))-1;
  int it1 = (int)(p_temp(&parts[i])) + 1;
  if (it1 < 1)
    it1 = 1;
  if (rng_chance(absorbScale % it1, it1))
    maxtmp ++;
  if (parts[i].tmp < 0) {
    parts[i].tmp = 0;
  }
  if (parts[i].tmp > absorbScale) {
    parts[i].tmp = absorbScale;
  }
  if (parts[i].tmp < maxtmp) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r || (parts[i].tmp >=maxtmp))
            continue;
          if (TYP(r)==PT_MERC&& rng_chance(1, 3)) {
            if ((parts[i].tmp + parts[ID(r)].tmp + 1) <= maxtmp) {
              parts[i].tmp += parts[ID(r)].tmp + 1;
              kill_part(ID(r));
            }
          }
        }
      }
    }
  } else {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (parts[i].tmp<=maxtmp)
            continue;
          if ((!r)&&parts[i].tmp>=1) { //if nothing then create MERC
            int np = create_part(-1,x+rx,y+ry,PT_MERC, -1);
            if (np<0) continue;
            parts[i].tmp--;
            p_set_temp(&parts[np], p_temp(&parts[i]));
            parts[np].tmp = 0;
          }
        }
      }
    }
  }
  for (int trade = 0; trade<4; trade ++) {
    int rx = rng_between(-2, 2);
    int ry = rng_between(-2, 2);
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      if (TYP(r)==PT_MERC&&(parts[i].tmp>parts[ID(r)].tmp)&&parts[i].tmp>0) { //diffusion
        int temp = parts[i].tmp - parts[ID(r)].tmp;
        if (temp ==1) {
          parts[ID(r)].tmp ++;
          parts[i].tmp --;
        } else if (temp>0) {
          parts[ID(r)].tmp += temp/2;
          parts[i].tmp -= temp/2;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- GEL */
int el_GEL_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp > 100)
    parts[i].tmp = 100;
  if (parts[i].tmp < 0)
    parts[i].tmp = 0;
  int absorbChanceDenom = parts[i].tmp * 10 + 500;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        bool gel = false;
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        //Desaturation
        switch (rt) {
        case PT_WATR:
        case PT_DSTW:
        case PT_FRZW:
          if (parts[i].tmp<100 && rng_chance(500, absorbChanceDenom)) {
            parts[i].tmp++;
            kill_part(ID(r));
          }
          break;
        case PT_PSTE:
          if (parts[i].tmp<100 && rng_chance(20, absorbChanceDenom)) {
            parts[i].tmp++;
            //@ GEL + PSTE -> GEL + CLST
            create_part(ID(r), x+rx, y+ry, PT_CLST, -1);
          }
          break;
        case PT_SLTW:
          if (parts[i].tmp<100 && rng_chance(50, absorbChanceDenom)) {
            parts[i].tmp++;
            //@ GEL + SLTW -> GEL + SALT
            if (rng_chance(3, 4))
              kill_part(ID(r));
            else
              part_change_type(ID(r), x+rx, y+ry, PT_SALT);
          }
          break;
        case PT_CBNW:
          if (parts[i].tmp < 100 && rng_chance(100, absorbChanceDenom)) {
            parts[i].tmp++;
            //@ GEL + CBNW -> GEL + CO2
            part_change_type(ID(r), x+rx, y+ry, PT_CO2);
          }
          break;
        case PT_SPNG:
          // Concentration diffusion
          if (parts[ID(r)].life>0 && parts[i].tmp<100 && ((parts[ID(r)].life+1)>parts[i].tmp)) {
            // SPNG -> GEL
            parts[ID(r)].life--;
            parts[i].tmp++;
          } else if (parts[i].tmp>0 && (parts[ID(r)].life+1)<parts[i].tmp) {
            // SPNG <- GEL (saturation limit of SPNG is ignored here)
            parts[ID(r)].life++;
            parts[i].tmp--;
          }
          break;
        case PT_GEL:
          if ((parts[ID(r)].tmp+1)<parts[i].tmp) {
            parts[ID(r)].tmp++;
            parts[i].tmp--;
          }
          gel = true;
          break;
        case PT_BASE:
          // Base absorbs water from gel
          if (parts[i].tmp > 0 && parts[ID(r)].life > 1) {
            // BASE <- GEL
            parts[ID(r)].life--;
            parts[i].tmp--;
          }
          break;
        default:
          break;
        }
        float dx, dy;
        dx = p_x(&parts[i]) - p_x(&parts[ID(r)]);
        dy = p_y(&parts[i]) - p_y(&parts[ID(r)]);
        //Stickiness
        if ((dx*dx + dy*dy)>1.5 && (gel || !elements[rt].Falldown || (fabsf((float)rx)<2 && fabsf((float)ry)<2))) {
          float per, nd;
          nd = dx*dx + dy*dy - 0.5;
          per = 5*(1 - parts[i].tmp/100)*(nd/(dx*dx + dy*dy + nd) - 0.5);
          if (elements[rt].Properties&TYPE_LIQUID)
            per *= 0.1f;
          dx *= per; dy *= per;
          p_set_vx(&parts[i], p_vx(&parts[i]) + (dx));
          p_set_vy(&parts[i], p_vy(&parts[i]) + (dy));
          if ((elements[rt].Properties&TYPE_PART) || rt==PT_GOO) {
            p_set_vx(&parts[ID(r)], p_vx(&parts[ID(r)]) - (dx));
            p_set_vy(&parts[ID(r)], p_vy(&parts[ID(r)]) - (dy));
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SOAP */
/* the shortest way between two particles when the edges loop (Element_SOAP_neighourLoop) */
void SOAP_neighbourLoop(float *dx, float *dy) {
  if (edgeMode == EDGE_LOOP) {
    if (*dx >= (XRES / 2 - BORDER)) *dx -= XRES - 2 * BORDER;
    if (*dy >= (YRES / 2 - BORDER)) *dy -= YRES - 2 * BORDER;
    if (*dx < -(XRES / 2 - BORDER)) *dx += XRES - 2 * BORDER;
    if (*dy < -(YRES / 2 - BORDER)) *dy += YRES - 2 * BORDER;
  }
}
static bool SOAP_validIndex(int i) {
  return i >= 0 && i < NPART;
}
void Element_SOAP_detach(int i) {
  if ((parts[i].ctype&2) == 2 && SOAP_validIndex(parts[i].tmp) && parts[parts[i].tmp].type == PT_SOAP) {
    if ((parts[parts[i].tmp].ctype&4) == 4)
      parts[parts[i].tmp].ctype ^= 4;
  }
  if ((parts[i].ctype&4) == 4 && SOAP_validIndex(parts[i].tmp2) && parts[parts[i].tmp2].type == PT_SOAP) {
    if ((parts[parts[i].tmp2].ctype&2) == 2)
      parts[parts[i].tmp2].ctype ^= 2;
  }
  parts[i].ctype = 0;
}
static void SOAP_attach(int i1, int i2) {
  if (!(parts[i2].ctype&4)) {
    parts[i1].ctype |= 2;
    parts[i1].tmp = i2;
    parts[i2].ctype |= 4;
    parts[i2].tmp2 = i1;
  } else if (!(parts[i2].ctype&2)) {
    parts[i1].ctype |= 4;
    parts[i1].tmp2= i2;
    parts[i2].ctype |= 2;
    parts[i2].tmp = i1;
  }
}
#define FREEZING 248.15f
int el_SOAP_update(UPDATE_FUNC_ARGS) {
  //0x01 - bubble on/off
  //0x02 - first mate yes/no
  //0x04 - "back" mate yes/no
  if (parts[i].ctype&1) {
    // reset invalid SOAP links
    if (!SOAP_validIndex(parts[i].tmp) || !SOAP_validIndex(parts[i].tmp2)) {
      parts[i].tmp = parts[i].tmp2 = parts[i].ctype = 0;
      return 0;
    }
    if (p_temp(&parts[i])>FREEZING) {
      if (parts[i].life<=0) {
        //if only connected on one side
        if ((parts[i].ctype&6) != 6 && (parts[i].ctype&6)) {
          int target = i;
          //break entire bubble in a loop
          while((parts[target].ctype&6) != 6 && (parts[target].ctype&6) && parts[target].type == PT_SOAP) {
            if (parts[target].ctype&2) {
              target = parts[target].tmp;
              if (!SOAP_validIndex(target)) {
                break;
              }
              Element_SOAP_detach(target);
            }
            if (parts[target].ctype&4) {
              target = parts[target].tmp2;
              if (!SOAP_validIndex(target)) {
                break;
              }
              Element_SOAP_detach(target);
            }
          }
        }
        if ((parts[i].ctype&6) != 6)
          parts[i].ctype = 0;
        if (SOAP_validIndex(parts[i].tmp) && (parts[i].ctype&6) == 6 && (parts[parts[i].tmp].ctype&6) == 6 && parts[parts[i].tmp].tmp == i)
          Element_SOAP_detach(i);
      }
      p_set_vy(&parts[i], (p_vy(&parts[i])-0.1f)*0.5f);
      p_set_vx(&parts[i], p_vx(&parts[i]) * (0.5f));
    }
    if(!(parts[i].ctype&2)) {
      for (int rx = -2; rx <= 2; rx++) {
        for (int ry = -2; ry <= 2; ry++) {
          if (rx || ry) {
            int r = PMAPAT(x+rx, y+ry);
            if (!r)
              continue;
            if ((parts[ID(r)].type == PT_SOAP) && (parts[ID(r)].ctype&1) && !(parts[ID(r)].ctype&4))
              SOAP_attach(i, ID(r));
          }
        }
      }
    } else {
      if (parts[i].life<=0) {
        for (int rx = -2; rx <= 2; rx++) {
          for (int ry = -2; ry <= 2; ry++) {
            if (rx || ry) {
              int r = PMAPAT(x+rx, y+ry);
              if (!r && !bmap[(y+ry)/WCELL][(x+rx)/WCELL])
                continue;
              if (p_temp(&parts[i])>FREEZING) {
                if (bmap[(y+ry)/WCELL][(x+rx)/WCELL]
                  || (r && !(elements[TYP(r)].Properties&TYPE_GAS)
                    && TYP(r) != PT_SOAP && TYP(r) != PT_GLAS)) {
                  Element_SOAP_detach(i);
                  continue;
                }
              }
              if (TYP(r) == PT_SOAP) {
                if (parts[ID(r)].ctype == 1) {
                  int buf = parts[i].tmp;
                  parts[i].tmp = ID(r);
                  if (SOAP_validIndex(buf) && parts[buf].type == PT_SOAP)
                    parts[buf].tmp2 = ID(r);
                  parts[ID(r)].tmp2 = i;
                  parts[ID(r)].tmp = buf;
                  parts[ID(r)].ctype = 7;
                } else if (parts[ID(r)].ctype == 7 && parts[i].tmp != ID(r) && parts[i].tmp2 != ID(r)) {
                  if (SOAP_validIndex(parts[i].tmp) && parts[parts[i].tmp].type == PT_SOAP)
                    parts[parts[i].tmp].tmp2 = parts[ID(r)].tmp2;
                  if (SOAP_validIndex(parts[ID(r)].tmp2) && parts[parts[ID(r)].tmp2].type == PT_SOAP)
                    parts[parts[ID(r)].tmp2].tmp = parts[i].tmp;
                  parts[ID(r)].tmp2 = i;
                  parts[i].tmp = ID(r);
                }
              }
            }
          }
        }
      }
    }
    if(parts[i].ctype&2 && SOAP_validIndex(parts[i].tmp)) {
      float d, dx, dy;
      dx = p_x(&parts[i]) - p_x(&parts[parts[i].tmp]);
      dy = p_y(&parts[i]) - p_y(&parts[parts[i].tmp]);
      SOAP_neighbourLoop(&dx, &dy);
      d = 9/(powf(dx, 2)+powf(dy, 2)+9)-0.5;
      p_set_vx(&parts[parts[i].tmp], p_vx(&parts[parts[i].tmp]) - (dx*d));
      p_set_vy(&parts[parts[i].tmp], p_vy(&parts[parts[i].tmp]) - (dy*d));
      p_set_vx(&parts[i], p_vx(&parts[i]) + (dx*d));
      p_set_vy(&parts[i], p_vy(&parts[i]) + (dy*d));
      if ((parts[parts[i].tmp].ctype&2) && (parts[parts[i].tmp].ctype&1)
          && SOAP_validIndex(parts[parts[i].tmp].tmp)
          && (parts[parts[parts[i].tmp].tmp].ctype&2) && (parts[parts[parts[i].tmp].tmp].ctype&1)) {
        int ii = parts[parts[parts[i].tmp].tmp].tmp;
        if (SOAP_validIndex(ii)) {
          dx = p_x(&parts[ii]) - p_x(&parts[parts[i].tmp]);
          dy = p_y(&parts[ii]) - p_y(&parts[parts[i].tmp]);
          SOAP_neighbourLoop(&dx, &dy);
          d = 81/(powf(dx, 2)+powf(dy, 2)+81)-0.5;
          p_set_vx(&parts[parts[i].tmp], p_vx(&parts[parts[i].tmp]) - (dx*d*0.5f));
          p_set_vy(&parts[parts[i].tmp], p_vy(&parts[parts[i].tmp]) - (dy*d*0.5f));
          p_set_vx(&parts[ii], p_vx(&parts[ii]) + (dx*d*0.5f));
          p_set_vy(&parts[ii], p_vy(&parts[ii]) + (dy*d*0.5f));
        }
      }
    }
  } else {
    if (pv[y/ACELL][x/ACELL]>0.5f || pv[y/ACELL][x/ACELL]<(-0.5f)) {
      parts[i].ctype = 1;
      parts[i].life = 10;
    }
    //SOAP+OIL foam effect
    for (int rx =-2; rx<3; rx++)
      for (int ry =-2; ry<3; ry++)
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r) == PT_OIL) {
            float ax, ay, gx, gy;
            GetGravityField(x, y, elements[PT_SOAP].Gravity, 1.0f, &gx, &gy);
            ax = ((p_vx(&parts[i])-gx)*0.5f + p_vx(&parts[ID(r)]))/2;
            ay = ((p_vy(&parts[i])-gy)*0.5f + p_vy(&parts[ID(r)]))/2;
            p_set_vx(&parts[ID(r)], ax);
            p_set_vx(&parts[i], ax);
            p_set_vy(&parts[ID(r)], ay);
            p_set_vy(&parts[i], ay);
          }
        }
  }
  /* (then the original fades the decoration of what touches it: decoration isn't kept) */
  return 0;
}
void el_SOAP_changeType(ELEMENT_CHANGETYPE_FUNC_ARGS) {
  if (from == PT_SOAP && to != PT_SOAP) {
    Element_SOAP_detach(i);
  }
}

/* ---------------------------------------------------------------- FRZW */
int el_FRZW_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_WATR && rng_chance(1, 14)) {
          //@ FRZW + WATR -> 2xFRZW
          part_change_type(ID(r),x+rx,y+ry,PT_FRZW);
        }
      }
    }
  }
  if ((parts[i].life==0 && rng_chance(1, 192)) || rng_chance(100-parts[i].life, 50000)) {
    //@ FRZW -> ICEI(FRZW)
    part_change_type(i,x,y,PT_ICEI);
    parts[i].ctype=PT_FRZW;
    p_set_temp(&parts[i], restrict_flt(p_temp(&parts[i])-200.0f, MIN_TEMP, MAX_TEMP));
  }
  return 0;
}

/* ---------------------------------------------------------------- BIZR */
/* BIZR, BIZRG and BIZRS blend their decoration colour into what touches them: decoration isn't kept */
int Element_BIZR_update(UPDATE_FUNC_ARGS) {
  return 0;
}

/* ---------------------------------------------------------------- RFRG */
int Element_RFRG_update(UPDATE_FUNC_ARGS) {
  /* the last pressure, a float in the original's 32-bit tmp: here its bits in tmp (low half) and tmp2 */
  float new_pressure = pv[y/ACELL][x/ACELL];
  union { uint32_t u; float f; } old;
  old.u = (uint16_t)parts[i].tmp | (uint32_t)(uint16_t)parts[i].tmp2 << 16;
  float *old_pressure = &old.f;
  if (isnan(*old_pressure)) {
    *old_pressure = new_pressure;
    parts[i].tmp = (int16_t)(old.u & 0xFFFF);
    parts[i].tmp2 = (int16_t)(old.u >> 16);
    return 0;
  }
  // * 0 bar seems to be pressure value -256 in TPT, see Air.cpp. Also, 1 bar seems to be pressure value 0.
  //   With those two values we can set up our pressure scale which states that ... the highest pressure
  //   we can achieve in TPT is 2 bar. That's not particularly realistic, but good enough for TPT.
  p_set_temp_d(&parts[i], restrict_flt(p_temp(&parts[i]) * ((new_pressure + 257.f) / (*old_pressure + 257.f)), 0, MAX_TEMP));   /* (small changes: dithered) */
  *old_pressure = new_pressure;
  parts[i].tmp = (int16_t)(old.u & 0xFFFF);
  parts[i].tmp2 = (int16_t)(old.u >> 16);
  return 0;
}

/* ---------------------------------------------------------------- RSST */
int el_RSST_update(UPDATE_FUNC_ARGS) {
  for(int rx = -1; rx < 2; rx++) {
    for(int ry = -1; ry < 2; ry++) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      //@ RSST + GUNP -> FIRW
      if(TYP(r) == PT_GUNP) {
        create_part(i, x, y, PT_FIRW, -1);
        kill_part(ID(r));
        return 1;
      }
      //@ RSST + BCOL -> FSEP
      if(TYP(r) == PT_BCOL) {
        create_part(i, x, y, PT_FSEP, -1);
        parts[i].life = 50;
        kill_part(ID(r));
        return 1;
      }
      // Set RSST ctype from nearby clone
      if((TYP(r) == PT_CLNE) || (TYP(r) == PT_PCLN)) {
        if(parts[ID(r)].ctype != PT_RSST)
          parts[i].ctype = parts[ID(r)].ctype;
      }
      // Set RSST tmp from nearby breakable clone
      if((TYP(r) == PT_BCLN) || (TYP(r) == PT_PBCN)) {
        if(parts[ID(r)].ctype != PT_RSST)
          parts[i].tmp = parts[ID(r)].ctype;
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- BASE */
int el_BASE_update(UPDATE_FUNC_ARGS) {
  //Reset spark effect
  parts[i].tmp = 0;
  if (parts[i].life < 1)
    parts[i].life = 1;
  if (parts[i].life > 100)
    parts[i].life = 100;
  float pres = pv[y/ACELL][x/ACELL];
  //Base evaporates into BOYL or increases concentration
  if (parts[i].life < 100 && pres < 10.0f && p_temp(&parts[i]) > (120.0f + 273.15f)) {
    //Slow down boiling
    if (rng_chance(1, 20)) {
      //This way we preserve the total amount of concentrated BASE in the solution
      if (rng_chance(1, parts[i].life+1)) {
        //@ BASE -> BOYL
        createPartTempVel(i, x, y, PT_BOYL);
        return 1;
      } else {
        parts[i].life++;
        //Enthalpy of vaporization
        p_set_temp_d(&parts[i], p_temp(&parts[i]) - (20.0f / ((float)parts[i].life)));   /* (small changes: dithered) */
      }
    }
  }
  //Base's freezing point lowers with its concentration
  if (p_temp(&parts[i]) < (273.15f - ((float)parts[i].life)/4.0f)) {
    //@ BASE -> ICE(BASE)
    //We don't save base's concentration, so ICEI(BASE) will unfreeze into life = 0
    createPartTempVel(i, x, y, PT_ICEI);
    parts[i].ctype = PT_BASE;
    parts[i].life = 0;
    return 1;
  }
  //Reactions
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        //Don't react with some elements
        if (rt != PT_BASE && rt != PT_SALT && rt != PT_SLTW && rt != PT_BOYL && rt != PT_MERC &&
                   rt != PT_BMTL && rt != PT_BRMT && rt != PT_SOAP && rt != PT_CLNE && rt != PT_PCLN &&
            !(rt == PT_ICEI && parts[ID(r)].ctype == PT_BASE) && !(rt == PT_SNOW && parts[ID(r)].ctype == PT_BASE) &&
            !(rt == PT_SPRK && parts[ID(r)].ctype == PT_BMTL) && !(rt == PT_SPRK && parts[ID(r)].ctype == PT_BRMT)) {
          //Base is diluted by water
          if (parts[i].life > 1 && (rt == PT_WATR || rt == PT_DSTW || rt == PT_CBNW)) {
                   if (rng_chance(1, 20)) {
              int saturh = parts[i].life/2;
              //@ BASE + WATR/DSTW/CBNW -> 2xBASE
              createPartTempVel(ID(r), x+rx, y+ry, PT_BASE);
              parts[ID(r)].life = saturh;
              p_set_temp_d(&parts[ID(r)], p_temp(&parts[ID(r)]) + (((float)saturh)/10.0f));
              parts[i].life -= saturh;
            }
          } //Base neutralizes acid
          else if (rt == PT_ACID) {
            if (parts[ID(r)].life > 50 && parts[i].life > 0) {
              parts[ID(r)].life--;
              parts[i].life--;
            }
            //@ BASE + ACID -> 2xSLTW
            if (parts[ID(r)].life <= 50)
              createPartTempVel(ID(r), x+rx, y+ry, PT_SLTW);
            if (parts[i].life <= 0) {
              createPartTempVel(i, x, y, PT_SLTW);
              return 1;
            }
          } //Base neutralizes CAUS
          else if (rt == PT_CAUS) {
            if (parts[ID(r)].life > 50 && parts[i].life > 0) {
              parts[ID(r)].life--;
              parts[i].life--;
            }
            //@ BASE + CAUS -> SLTW
            if (parts[ID(r)].life <= 50)
              kill_part(ID(r));
            if (parts[i].life <= 0) {
              createPartTempVel(i, x, y, PT_SLTW);
              return 1;
            }
          } //@ BASE + OIL -> SOAP
          else if (parts[i].life >= 70 && rt == PT_OIL) {
            createPartTempVel(i, x, y, PT_SOAP);
            kill_part(ID(r));
            return 1;
          } //@ BASE + GOO -> BASE + GEL
          else if (parts[i].life > 1 && rt == PT_GOO) {
            createPartTempVel(ID(r), x+rx, y+ry, PT_GEL);
            parts[i].life--;
          } //@ BASE + BCOL -> BASE + GUNP
          else if (parts[i].life > 1 && rt == PT_BCOL) {
            createPartTempVel(ID(r), x+rx, y+ry, PT_GUNP);
            parts[i].life--;
          } //@ BASE + LAVA(ROCK) -> MERC
                else if (rt == PT_LAVA && parts[ID(r)].ctype == PT_ROCK && pres >= 10.0f && rng_chance(1, 1000)) {
            createPartTempVel(i, x, y, PT_MERC);
            kill_part(ID(r));
            return 1;
          } //Base rusts conductive solids
          else if (parts[i].life >= 10 &&
                     (elements[rt].Properties & (TYPE_SOLID|PROP_CONDUCTS)) == (TYPE_SOLID|PROP_CONDUCTS) && rng_chance(1, 10)) {
            //@ BASE + conductive solid -> BASE + BMTL
            createPartTempVel(ID(r), x+rx, y+ry, PT_BMTL);
            parts[ID(r)].tmp = rng_between(20, 29);
            parts[i].life--;
            //Draw a spark effect
            parts[i].tmp = 1;
          } //Base destroys a substance slower if acid destroys it faster
          else if (elements[rt].Hardness > 0 && elements[rt].Hardness < 50 &&
              parts[i].life >= (2*elements[rt].Hardness) && rng_chance(50-elements[rt].Hardness, 1000)) {
            kill_part(ID(r));
            parts[i].life -= 2;
            //Draw a spark
            parts[i].tmp = 1;
          }
        }
      }
    }
  }
  //Diffusion
  for (int trade = 0; trade<2; trade++) {
    int rx = rng_between(-1, 1);
    int ry = rng_between(-1, 1);
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      if (TYP(r) == PT_BASE && (parts[i].life > parts[ID(r)].life) && parts[i].life>1) {
        int temp = parts[i].life - parts[ID(r)].life;
        if (temp == 1) {
          parts[ID(r)].life++;
          parts[i].life--;
        } else if (temp>0) {
          parts[ID(r)].life += temp/2;
          parts[i].life -= temp/2;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- VIRS */
/* tmp3: frames until it's cured, when above 0 (being cured); tmp4, frames until it dies, is minus tmp3 when that
 * isn't above 0 (the original's tmp4 matters only while tmp3 is 0) */
int Element_VIRS_update(UPDATE_FUNC_ARGS) {
  int rndstore = rng_gen();
  if (parts[i].tmp3 > 0) {
    parts[i].tmp3 -= (rndstore & 0x1) ? 0:1;
    //has been cured, so change back into the original element
    if (!parts[i].tmp3) {
      part_change_type(i,x,y,parts[i].tmp2);
      parts[i].tmp2 = 0;
      parts[i].tmp3 = 0;
      tmp4_clear(i);   /* (the element it's back to: a PIPE's, STOR's, PLNT's... tmp4 is 0, as the original's) */
    }
    return 0;
    //cured virus is never in below code
  }
  int tmp4 = -parts[i].tmp3;
  //decrease tmp4 so it slowly dies
  if (tmp4) {
    if (!(rndstore & 0x7) && --tmp4 <= 0) {
      kill_part(i);
      return 1;
    }
    parts[i].tmp3 = (int16_t)-tmp4;
    rndstore >>= 3;
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        //spread "being cured" state
        if (parts[ID(r)].tmp3 > 0 && (TYP(r) == PT_VIRS || TYP(r) == PT_VRSS || TYP(r) == PT_VRSG)) {
          parts[i].tmp3 = parts[ID(r)].tmp3 + ((rndstore & 0x3) ? 2:1);
          return 0;
        }
        //soap cures virus
        else if (TYP(r) == PT_SOAP) {
          parts[i].tmp3 = 10;   /* (the original's tmp3 += 10, from 0) */
          if (!(rndstore & 0x3))
            kill_part(ID(r));
          return 0;
        } else if (TYP(r) == PT_PLSM) {
          if (surround_space && rng_chance(10 + (int)(pv[(y+ry)/ACELL][(x+rx)/ACELL]), 100)) {
            //@ VIRS/VRSS/VRSG + PLSM -> 2xPLSM
            create_part(i, x, y, PT_PLSM, -1);
            return 1;
          }
        }
        //transforms things into virus here
        else if (TYP(r) != PT_VIRS && TYP(r) != PT_VRSS && TYP(r) != PT_VRSG && TYP(r) != PT_DMND && TYP(r) != PT_BASE) {
          if (!(rndstore & 0x7)) {
            parts[ID(r)].tmp2 = TYP(r);
            /* tmp3 = 0, tmp4 = this one's + 1 (or 0) */
            parts[ID(r)].tmp3 = (int16_t)-(tmp4 ? imin(tmp4 + 1, 32767) : 0);
            if (p_temp(&parts[ID(r)]) < 305.0f)
              part_change_type(ID(r), x+rx, y+ry, PT_VRSS);
            else if (p_temp(&parts[ID(r)]) > 673.0f)
              part_change_type(ID(r), x+rx, y+ry, PT_VRSG);
            else
              part_change_type(ID(r), x+rx, y+ry, PT_VIRS);
          }
          rndstore >>= 3;
        }
        //protons make VIRS last forever
        else if (TYP(PHOTAT(x+rx, y+ry)) == PT_PROT) {
          tmp4 = 0;
          parts[i].tmp3 = 0;
        }
      }
      //reset rndstore only once, halfway through
      else if (!rx && !ry)
        rndstore = rng_gen();
    }
  }
  return 0;
}
