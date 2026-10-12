/* NumPowder: The Powder Toy's radioactive and energy elements (their files in elements/), ported: NEUT, PHOT, PLUT,
 * URAN, ISOZ, ISZS, AMTR, DEUT, WARP, SING, ELEC, PROT, GRVT, EXOT, POLO. */
#include "simpriv.h"

/* ---------------------------------------------------------------- NEUT */
static int NEUT_DeutExplosion(int n, int x, int y, float temp, int t);
int el_NEUT_update(UPDATE_FUNC_ARGS) {
  unsigned int pressureFactor = 3 + (int)pv[y/ACELL][x/ACELL];
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      int r = PMAPAT(x+rx, y+ry);
      switch (TYP(r)) {
      case PT_WATR:
        //@ NEUT + WATR -> NEUT + DSTW
        if (rng_chance(3, 20))
          part_change_type(ID(r),x+rx,y+ry,PT_DSTW);
        /* fallthrough */
      case PT_ICEI:
      case PT_SNOW:
        p_set_vx(&parts[i], p_vx(&parts[i]) * (0.995f));
        p_set_vy(&parts[i], p_vy(&parts[i]) * (0.995f));
        break;
      case PT_PLUT:
        if (rng_chance(pressureFactor, 1000)) {
          if (rng_chance(1, 3)) {
            //@ NEUT + PLUT -> NEUT + URAN/LAVA(PLUT)
            create_part(ID(r), x+rx, y+ry, rng_chance(2, 3) ? PT_LAVA : PT_URAN, -1);
            p_set_temp(&parts[ID(r)], MAX_TEMP);
            if (parts[ID(r)].type==PT_LAVA) {
              parts[ID(r)].tmp = 100;
              parts[ID(r)].ctype = PT_PLUT;
            }
          } else {
            //@ NEUT + PLUT -> 2xNEUT
            create_part(ID(r), x+rx, y+ry, PT_NEUT, -1);
            p_set_vx(&parts[ID(r)], 0.25f*p_vx(&parts[ID(r)]) + p_vx(&parts[i]));
            p_set_vy(&parts[ID(r)], 0.25f*p_vy(&parts[ID(r)]) + p_vy(&parts[i]));
          }
          pv[y/ACELL][x/ACELL] += 10.0f * CFDS; //Used to be 2, some people said nukes weren't powerful enough
          Element_FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);
        }
        break;
      case PT_DEUT:
        if (rng_chance(pressureFactor + 1 + (parts[ID(r)].life/100), 1000)) {
          //@ NEUT + DEUT -> 2xNEUT
          NEUT_DeutExplosion(parts[ID(r)].life, x+rx, y+ry, restrict_flt(p_temp(&parts[ID(r)]) + parts[ID(r)].life*500.0f, MIN_TEMP, MAX_TEMP), PT_NEUT);
          kill_part(ID(r));
        }
        break;
      case PT_GUNP:
        //@ NEUT + GUNP -> NEUT + DUST
        if (rng_chance(3, 200))
          part_change_type(ID(r),x+rx,y+ry,PT_DUST);
        break;
      case PT_DYST:
        //@ NEUT + DYST -> NEUT + YEST
        if (rng_chance(3, 200))
          part_change_type(ID(r),x+rx,y+ry,PT_YEST);
        break;
      case PT_YEST:
        //@ NEUT + YEST -> NEUT + DYST
        part_change_type(ID(r),x+rx,y+ry,PT_DYST);
        break;
      case PT_PLEX:
        //@ NEUT + PLEX -> NEUT + GOO
        if (rng_chance(3, 200))
          part_change_type(ID(r),x+rx,y+ry,PT_GOO);
        break;
      case PT_NITR:
        //@ NEUT + NITR -> NEUT + DESL
        if (rng_chance(3, 200))
          part_change_type(ID(r),x+rx,y+ry,PT_DESL);
        break;
      case PT_PLNT:
        //@ NEUT + PLNT -> NEUT + WOOD
        if (rng_chance(1, 20))
          create_part(ID(r), x+rx, y+ry, PT_WOOD, -1);
        break;
      case PT_DESL:
      case PT_OIL:
        //@ NEUT + DESL/OIL -> NEUT + GAS
        if (rng_chance(3, 200))
          part_change_type(ID(r),x+rx,y+ry,PT_GAS);
        break;
      case PT_COAL:
        //@ NEUT + COAL -> NEUT + WOOD
        if (rng_chance(1, 20))
          create_part(ID(r), x+rx, y+ry, PT_WOOD, -1);
        break;
      case PT_BCOL:
        //@ NEUT + BCOL -> NEUT + SAWD
        if (rng_chance(1, 20))
          create_part(ID(r), x+rx, y+ry, PT_SAWD, -1);
        break;
      case PT_DUST:
        //@ NEUT + DUST -> NEUT + FWRK
        if (rng_chance(1, 20))
          part_change_type(ID(r), x+rx, y+ry, PT_FWRK);
        break;
      case PT_FWRK:
        if (rng_chance(1, 20))
          parts[ID(r)].ctype = PT_DUST;
        break;
      case PT_ACID:
        //@ NEUT + ACID -> NEUT + ISOZ
        if (rng_chance(1, 20))
          create_part(ID(r), x+rx, y+ry, PT_ISOZ, -1);
        break;
      case PT_TTAN:
        if (rng_chance(1, 20)) {
          kill_part(i);
          return 1;
        }
        break;
      case PT_EXOT:
        if (rng_chance(1, 20))
          parts[ID(r)].life = 1500;
        break;
      case PT_RFRG:
        //@ NEUT + RFRG -> NEUT + GAS/CAUS
        if (rng_chance(1, 2))
          create_part(ID(r), x+rx, y+ry, PT_GAS, -1);
        else
          create_part(ID(r), x+rx, y+ry, PT_CAUS, -1);
        break;
      case PT_RSSS:
        if(!rx && !ry) {
          int ct_under, tmp_under;
          ct_under = parts[ID(r)].ctype;
          tmp_under = parts[ID(r)].tmp;
          //If there's a correct ctype set, liquefy into it
          if(ct_under > 0 && ct_under < PT_NUM) {
            //@ NEUT + RSSS(ctype) -> ctype
            create_part(ID(r), x, y, ct_under, -1);
            //If there's a correct tmp set, use it for ctype
            if((tmp_under > 0) && (tmp_under < PT_NUM) && (elements[ct_under].CarriesTypeIn & (1U << FIELD_CTYPE)))
              parts[ID(r)].ctype = tmp_under;
          } else //@ NEUT + RSSS -> RSST
            part_change_type(ID(r), x, y, PT_RSST); //Default to RSST if no ctype
          kill_part(i);
          return 1;
        }
        break;
      case PT_BASE:
        //@ NEUT + BASE -> NEUT + LRBD
        if (p_temp(&parts[ID(r)]) > (50 + 273.15) && rng_chance(1, 35))
          create_part(ID(r), x+rx, y+ry, PT_LRBD, -1);
        break;
      case PT_SEED:
        if(!rx && !ry) {
          // Flip a random gene with 1/2 chance
          switch (rng_between(0, 9)) {
            case 0:
              parts[ID(r)].ctype ^= 1 << rng_between(PLNT_COLOUR, PLNT_LIFE-1);
              break;
            case 1:
              parts[ID(r)].tmp ^= 1 << rng_between(0, PLNT_TOTAL_TMP-1);
              break;
            case 2:
              parts[ID(r)].tmp2 ^= 1 << rng_between(0, PLNT_TOTAL_TMP-1);
              break;
            case 3:
              parts[ID(r)].tmp3 ^= 1 << rng_between(0, PLNT_TOTAL_TMP-1);
              break;
            case 4:
              tmp4_set(ID(r), tmp4_get(ID(r)) ^ (1 << rng_between(0, PLNT_TOTAL_TMP-1)));
              break;
            default:
              break;
          }
        }
        break;
      default:
        break;
      }
    }
  }
  return 0;
}
void el_NEUT_create(ELEMENT_CREATE_FUNC_ARGS) {
  float r = rng_between(128, 255) / 127.0f;
  float a = rng_between(0, 359) * NP_PI / 180.0f;
  parts[i].life = rng_between(480, 959);
  p_set_vx(&parts[i], r * cosf(a));
  p_set_vy(&parts[i], r * sinf(a));
}
static int NEUT_DeutExplosion(int n, int x, int y, float temp, int t) { //testing a new deut create part
  int i;
  n = (n/50);
  if (n < 1)
    n = 1;
  else if (n > 340)
    n = 340;
  for (int c = 0; c < n; c++) {
    i = create_part(-3, x, y, t, -1);
    if (i >= 0)
      p_set_temp(&parts[i], temp);
    else if (parts_full())
      break;
  }
  pv[y/ACELL][x/ACELL] += (6.0f * CFDS)*n;
  return 0;
}

/* ---------------------------------------------------------------- PHOT */
int el_PHOT_update(UPDATE_FUNC_ARGS) {
  if (!(parts[i].ctype&0x3FFFFFFF)) {
    kill_part(i);
    return 1;
  }
  if (p_temp(&parts[i]) > 506)
    if (rng_chance(1, 10))
      Element_FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      if (TYP(r)==PT_ISOZ || TYP(r)==PT_ISZS) {
        if (rng_chance(1, 400)) {
          p_set_vx(&parts[i], p_vx(&parts[i]) * (0.90f));
          p_set_vy(&parts[i], p_vy(&parts[i]) * (0.90f));
          //@ PHOT + ISOZ/ISZS -> 2xPHOT
          create_part(ID(r), x+rx, y+ry, PT_PHOT, -1);
          float rrr = rng_between(0, 359) * NP_PI / 180.0f;
          int rr;
          if (TYP(r) == PT_ISOZ)
            rr = (int)(rng_between(128, 255) / 127.0f);
          else
            rr = (int)(rng_between(128, 355) / 127.0f);
          p_set_vx(&parts[ID(r)], rr*cosf(rrr));
          p_set_vy(&parts[ID(r)], rr*sinf(rrr));
          pv[y/ACELL][x/ACELL] -= 15.0f * CFDS;
        }
      } else if((TYP(r) == PT_QRTZ || TYP(r) == PT_PQRT) && !ry && !rx) { //if on QRTZ
        float a = rng_between(0, 359) * NP_PI / 180.0f;
        p_set_vx(&parts[i], 3.0f*cosf(a));
        p_set_vy(&parts[i], 3.0f*sinf(a));
        if(parts[i].ctype == 0x3FFFFFFF)
          parts[i].ctype = 0x1F << rng_between(0, 25);
        if (parts[i].life)
          parts[i].life++; //Delay death
      } else if(TYP(r) == PT_BGLA && !ry && !rx) { //if on BGLA
        float a = rng_between(-50, 50) * 0.001f;
        float rx = cosf(a), ry = sinf(a), nvx, nvy;
        nvx = rx * p_vx(&parts[i]) + ry * p_vy(&parts[i]);
        nvy = rx * p_vy(&parts[i]) - ry * p_vx(&parts[i]);
        p_set_vx(&parts[i], nvx);
        p_set_vy(&parts[i], nvy);
      } else if(TYP(r) == PT_RSST && !ry && !rx) { //if on RSST, make it solid
        int ct_under, tmp_under;
        ct_under = parts[ID(r)].ctype;
        tmp_under = parts[ID(r)].tmp;
        //If there's a correct ctype set, solidify RSST into it
        if(ct_under > 0 && ct_under < PT_NUM) {
          //@ PHOT + RSST(ctype) -> ctype
          create_part(ID(r), x, y, ct_under, -1);
          //If there's a correct tmp set, use it for ctype
          if((tmp_under > 0) && (tmp_under < PT_NUM) && (elements[ct_under].CarriesTypeIn & (1U << FIELD_CTYPE)))
            parts[ID(r)].ctype = tmp_under;
        } else //@ PHOT + RSST -> RSSS
          part_change_type(ID(r), x, y, PT_RSSS); //Default to RSSS if no ctype
        kill_part(i);
        return 1;
      } else if (TYP(r) == PT_FILT && parts[ID(r)].tmp==9) {
        p_set_vx(&parts[i], p_vx(&parts[i]) + (((float)rng_between(-500, 500))/1000.0f));
        p_set_vy(&parts[i], p_vy(&parts[i]) + (((float)rng_between(-500, 500))/1000.0f));
      }
    }
  }
  return 0;
}
void el_PHOT_create(ELEMENT_CREATE_FUNC_ARGS) {
  float a = rng_between(0, 7) * 0.78540f;
  p_set_vx(&parts[i], 3.0f * cosf(a));
  p_set_vy(&parts[i], 3.0f * sinf(a));
  if (TYP(PMAPAT(x, y)) == PT_FILT)
    parts[i].ctype = Element_FILT_interactWavelengths(&parts[ID(PMAPAT(x, y))], parts[i].ctype);
}

/* ---------------------------------------------------------------- PLUT */
int el_PLUT_update(UPDATE_FUNC_ARGS) {
  if (rng_chance(1, 100) && rng_chance((int)(5.0f*pv[y/ACELL][x/ACELL]), 1000)) {
    //@ PLUT -> NEUT
    create_part(i, x, y, PT_NEUT, -1);
  }
  return 0;
}

/* ---------------------------------------------------------------- URAN */
int el_URAN_update(UPDATE_FUNC_ARGS) {
  if (!legacy_enable && pv[y/ACELL][x/ACELL]>0.0f) {
    if (p_temp(&parts[i]) == MIN_TEMP) {
      p_set_temp_d(&parts[i], p_temp(&parts[i]) + (.01f));
    } else {
      p_set_temp_d(&parts[i], restrict_flt((p_temp(&parts[i])*(1 + (pv[y / ACELL][x / ACELL] / 2000))) + MIN_TEMP, MIN_TEMP, MAX_TEMP));
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- ISOZ */
int el_ISOZ_update(UPDATE_FUNC_ARGS) {
  float rr, rrr;
  if (rng_chance(1, 200) && rng_chance((int)(-4.0f * pv[y/ACELL][x/ACELL]), 1000)) {
    //@ ISOZ -> PHOT
    create_part(i, x, y, PT_PHOT, -1);
    rr = rng_between(128, 355) / 127.0f;
    rrr = rng_between(0, 359) * NP_PI / 180.0f;
    p_set_vx(&parts[i], rr*cosf(rrr));
    p_set_vy(&parts[i], rr*sinf(rrr));
  }
  return 0;
}

/* ---------------------------------------------------------------- ISZS */
int el_ISZS_update(UPDATE_FUNC_ARGS) {
  float rr, rrr;
  if (rng_chance(1, 200) && rng_chance((int)(-4.0f * pv[y/ACELL][x/ACELL]), 1000)) {
    //@ ISZS -> PHOT
    create_part(i, x, y, PT_PHOT, -1);
    rr = rng_between(128, 355) / 127.0f;
    rrr = rng_between(0, 359) * NP_PI / 180.0f;
    p_set_vx(&parts[i], rr*cosf(rrr));
    p_set_vy(&parts[i], rr*sinf(rrr));
  }
  return 0;
}

/* ---------------------------------------------------------------- AMTR */
int el_AMTR_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt!=PT_AMTR && rt!=PT_DMND && rt!=PT_CLNE && rt!=PT_PCLN && rt!=PT_VOID && rt!=PT_BHOL && rt!=PT_NBHL && rt!=PT_PRTI && rt!=PT_PRTO) {
          parts[i].life++;
          if (parts[i].life==4) {
            kill_part(i);
            return 1;
          }
          //@ AMTR + anything -> AMTR + PHOT
          if (rng_chance(1, 10))
            create_part(ID(r), x+rx, y+ry, PT_PHOT, -1);
          else
            kill_part(ID(r));
          pv[y/ACELL][x/ACELL] -= 2.0f;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- DEUT */
int el_DEUT_update(UPDATE_FUNC_ARGS) {
  float gravtot = fabsf(gravx[y/ACELL][x/ACELL]) + fabsf(gravy[y/ACELL][x/ACELL]);
  // Prevent division by 0
  float temp = fmaxf(1.0f, (p_temp(&parts[i]) + 1));
  int maxlife = (int)(((10000/(temp + 1))-1));
  if (rng_chance(10000 % (int)(temp + 1), (int)(temp + 1)))
    maxlife++;
  // Compress when Newtonian gravity is applied
  // multiplier=1 when gravtot=0, multiplier -> 5 as gravtot -> inf
  maxlife = maxlife*(int)(5.0f - 8.0f/(gravtot+2.0f));
  if (parts[i].life < maxlife) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r || (parts[i].life >=maxlife))
            continue;
          if (TYP(r)==PT_DEUT&& rng_chance(1, 3)) {
            // If neighbour life+1 fits in the free capacity for this particle, absorb neighbour
            // Condition is written in this way so that large neighbour life values don't cause integer overflow
            if (parts[ID(r)].life <= maxlife - parts[i].life - 1) {
              parts[i].life += parts[ID(r)].life + 1;
              kill_part(ID(r));
            }
          }
        }
      }
    }
  } else {
    /* tryExpand */
      for (int rx = -1; rx <= 1; rx++) {
        for (int ry = -1; ry <= 1; ry++) {
          if (rx || ry) {
            //Leave if there is nothing to do
            if (parts[i].life <= maxlife) {
              goto expanded;
            }
            int r = PMAPAT(x+rx, y+ry);
            if ((!r)&&parts[i].life>=1) { //if nothing then create deut
              int np = create_part(-1,x+rx,y+ry,PT_DEUT, -1);
              if (np<0) continue;
              parts[i].life--;
              p_set_temp(&parts[np], p_temp(&parts[i]));
              parts[np].life = 0;
            }
          }
        }
      }
    expanded:;
  }
  for (int trade = 0; trade<4; trade ++) {
    int rx = rng_between(-2, 2);
    int ry = rng_between(-2, 2);
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      if (TYP(r)==PT_DEUT&&(parts[i].life>parts[ID(r)].life)&&parts[i].life>0) { //diffusion
        int temp = parts[i].life - parts[ID(r)].life;
        if (temp ==1) {
          parts[ID(r)].life ++;
          parts[i].life --;
        } else if (temp>0) {
          parts[ID(r)].life += temp/2;
          parts[i].life -= temp/2;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- WARP */
int el_WARP_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp2 > 2000) {
    p_set_temp(&parts[i], 10000);   /* (the original's 10000 K: MAX_TEMP here, simdefs.h) */
    pv[y/ACELL][x/ACELL] = restrict_flt(pv[y/ACELL][x/ACELL] + (parts[i].tmp2 / 5000) * CFDS, MIN_PRESSURE, MAX_PRESSURE);
    //@ WARP -> WARP + ELEC
    if (rng_chance(1, 50))
      create_part(-3, x, y, PT_ELEC, -1);
  }
  for (int trade = 0; trade < 5; trade ++) {
    int rx = rng_between(-1, 1);
    int ry = rng_between(-1, 1);
    if (rx || ry) {
      int r = PMAPAT(x + rx, y + ry);
      if (!r)
        continue;
      if (TYP(r) != PT_WARP && TYP(r) != PT_STKM && TYP(r) != PT_STKM2 && TYP(r) != PT_DMND && TYP(r) != PT_CLNE && TYP(r) != PT_BCLN && TYP(r) != PT_PCLN) {
        p_set_x(&parts[i], p_x(&parts[ID(r)]));
        p_set_y(&parts[i], p_y(&parts[ID(r)]));
        p_set_x(&parts[ID(r)], (float)(x));
        p_set_y(&parts[ID(r)], (float)(y));
        p_set_vx(&parts[ID(r)], rng_between(-2, 1) + 0.5f);
        p_set_vy(&parts[ID(r)], (float)(rng_between(-2, 1)));
        parts[i].life = (int16_t)(parts[i].life <= 32767 - 4 ? parts[i].life + 4 : 32767);   /* (16 bits: kept at the top) */
        PMAPSET(x, y, r);
        PMAPSET(x + rx, y + ry, PMAP(i, parts[i].type));
        trade = 5;
      }
    }
  }
  return 0;
}
void el_WARP_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].life = rng_between(70, 164);
}

/* ---------------------------------------------------------------- SING */
int el_SING_update(UPDATE_FUNC_ARGS) {
  int singularity = -parts[i].life;
  if (pv[y/ACELL][x/ACELL]<singularity)
    pv[y/ACELL][x/ACELL] += 0.1f*(singularity-pv[y/ACELL][x/ACELL]);
  if (pv[y/ACELL+1][x/ACELL]<singularity)
    pv[y/ACELL+1][x/ACELL] += 0.1f*(singularity-pv[y/ACELL+1][x/ACELL]);
  if (pv[y/ACELL-1][x/ACELL]<singularity)
    pv[y/ACELL-1][x/ACELL] += 0.1f*(singularity-pv[y/ACELL-1][x/ACELL]);
  pv[y/ACELL][x/ACELL+1]   = restrict_flt(pv[y/ACELL][x/ACELL+1]   + 0.1f*(singularity-pv[y/ACELL][x/ACELL+1]),   MIN_PRESSURE, MAX_PRESSURE);
  pv[y/ACELL+1][x/ACELL+1] = restrict_flt(pv[y/ACELL+1][x/ACELL+1] + 0.1f*(singularity-pv[y/ACELL+1][x/ACELL+1]), MIN_PRESSURE, MAX_PRESSURE);
  pv[y/ACELL][x/ACELL-1]   = restrict_flt(pv[y/ACELL][x/ACELL-1]   + 0.1f*(singularity-pv[y/ACELL][x/ACELL-1]),   MIN_PRESSURE, MAX_PRESSURE);
  pv[y/ACELL-1][x/ACELL-1] = restrict_flt(pv[y/ACELL-1][x/ACELL-1] + 0.1f*(singularity-pv[y/ACELL-1][x/ACELL-1]), MIN_PRESSURE, MAX_PRESSURE);
  if (parts[i].life<1) {
    //Pop!
    for (int rx = -1; rx <= 1; rx++) {
      int crx = (x/ACELL)+rx;
      for (int ry = -1; ry <= 1; ry++) {
        int cry = (y/ACELL)+ry;
        if (cry >= 0 && crx >= 0 && crx < XACELLS && cry < YACELLS) {
          pv[cry][crx] = restrict_flt(pv[cry][crx] + (float)parts[i].tmp, MIN_PRESSURE, MAX_PRESSURE);
        }
      }
    }
    int spawncount = abs(parts[i].tmp);
    spawncount = spawncount>255 ? 3019 : (int)((float)((spawncount/8)*(spawncount/8))*NP_PI);   /* (the same as in doubles for these) */
    for (int j = 0;j < spawncount; j++) {
      int nb = -1;
      switch (rng_gen() % 3) {
        case 0:
          //@ SING -> PHOT
          nb = create_part(-3, x, y, PT_PHOT, -1);
          break;
        case 1:
          //@ SING -> NEUT
          nb = create_part(-3, x, y, PT_NEUT, -1);
          break;
        case 2:
          //@ SING -> ELEC
          nb = create_part(-3, x, y, PT_ELEC, -1);
          break;
      }
      if (nb!=-1) {
        parts[nb].life = rng_between(0, 299);
        p_set_temp(&parts[nb], MAX_TEMP / 2);
        float angle = rng_uniform01() * 2.0f * NP_PI;
        float v = rng_uniform01() * 5.0f;
        p_set_vx(&parts[nb], v * cosf(angle));
        p_set_vy(&parts[nb], v * sinf(angle));
      } else if (parts_full())
        break;//if we've run out of particles, stop trying to create them - saves a lot of lag on "sing bomb" saves
    }
    kill_part(i);
    return 1;
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)!=PT_DMND&& rng_chance(1, 3)) {
          if (TYP(r)==PT_SING && parts[ID(r)].life >10) {
            if (parts[i].life+parts[ID(r)].life > 255)
              continue;
            parts[i].life += parts[ID(r)].life;
          } else {
            if (parts[i].life+3 > 255) {
              if (parts[ID(r)].type!=PT_SING && rng_chance(1, 1000)) {
                int np;
                np = create_part(ID(r),x+rx,y+ry,PT_SING, -1);
                parts[np].life = rng_between(60, 109);
              }
              continue;
            }
            parts[i].life += 3;
            parts[i].tmp++;
          }
          p_set_temp(&parts[i], restrict_flt(p_temp(&parts[ID(r)])+p_temp(&parts[i]), MIN_TEMP, MAX_TEMP));
          kill_part(ID(r));
        }
      }
    }
  }
  return 0;
}
void el_SING_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].life = rng_between(60, 109);
}

/* ---------------------------------------------------------------- ELEC */
int el_ELEC_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        r = PHOTAT(x+rx, y+ry);
      if (!r)
        continue;
      int rt = TYP(r);
      switch (rt) {
      case PT_GLAS:
        for (int rrx =-1; rrx<=1; rrx++) {
          for (int rry =-1; rry<=1; rry++) {
            if (x+rx+rrx>=0 && y+ry+rry>=0 && x+rx+rrx<XRES && y+ry+rry<YRES) {
              //@ ELEC + GLAS -> EMBR + GLAS
              int nb = create_part(-1, x+rx+rrx, y+ry+rry, PT_EMBR, -1);
              if (nb!=-1) {
                parts[nb].tmp = 0;
                parts[nb].life = 50;
                p_set_temp(&parts[nb], p_temp(&parts[i])*0.8f);
                p_set_vx(&parts[nb], (float)(rng_between(-10, 10)));
                p_set_vy(&parts[nb], (float)(rng_between(-10, 10)));
              }
            }
          }
        }
        kill_part(i);
        return 1;
      case PT_LCRY:
        parts[ID(r)].tmp2 = rng_between(5, 9);
        break;
      case PT_WATR:
      case PT_DSTW:
      case PT_SLTW:
      case PT_CBNW:
        //@ ELEC + WATR/DSTW/SLTW/CBNW -> O2/H2
        if (rng_chance(1, 3))
          create_part(ID(r), x+rx, y+ry, PT_O2, -1);
        else
          create_part(ID(r), x+rx, y+ry, PT_H2, -1);
        kill_part(i);
        return 1;
      case PT_PROT: // this is the correct reaction, not NEUT, but leaving NEUT in anyway
        if (parts[ID(r)].tmp2 & 0x1)
          break;
        /* fallthrough */
      case PT_NEUT:
        //@ ELEC + PROT/NEUT -> H2
        part_change_type(ID(r), x+rx, y+ry, PT_H2);
        parts[ID(r)].life = 0;
        parts[ID(r)].ctype = 0;
        kill_part(i);
        break;
      case PT_DEUT:
        if(parts[ID(r)].life < 6000)
          parts[ID(r)].life += 1;
        p_set_temp(&parts[ID(r)], 0);
        kill_part(i);
        return 1;
      case PT_EXOT:
        parts[ID(r)].tmp2 += 5;
        parts[ID(r)].life = 1000;
        break;
      case PT_RSST: //Destroy RSST
        if(!rx && !ry) {
          kill_part(ID(r));
          kill_part(i);
          return 1;
        }
        break;
      case PT_NONE: //seems to speed up ELEC even if it isn't used
        break;
      default:
        if ((elements[rt].Properties & PROP_CONDUCTS) && (rt!=PT_NBLE||p_temp(&parts[i])<2273.15)) {
          create_part(-1, x+rx, y+ry, PT_SPRK, -1);
          kill_part(i);
          return 1;
        }
        break;
      }
    }
  }
  return 0;
}
void el_ELEC_create(ELEMENT_CREATE_FUNC_ARGS) {
  float a = rng_between(0, 359) * NP_PI / 180.0f;
  parts[i].life = 680;
  p_set_vx(&parts[i], 2.0f * cosf(a));
  p_set_vy(&parts[i], 2.0f * sinf(a));
}

/* ---------------------------------------------------------------- PROT */
static int PROT_DeutImplosion(int n, int x, int y, float temp, int t);
/* the energy of its collisions (the original's tmp, up to 500000): tmp (low half) and tmp3 */
static int PROT_tmp(int i) { return (int)((uint16_t)parts[i].tmp | (uint32_t)(uint16_t)parts[i].tmp3 << 16); }
static void PROT_set_tmp(int i, int v) {
  parts[i].tmp = (int16_t)(v & 0xFFFF);
  parts[i].tmp3 = (int16_t)((uint32_t)v >> 16);
}
int el_PROT_update(UPDATE_FUNC_ARGS) {
  pv[y/ACELL][x/ACELL] -= .003f;
  int under = PMAPAT(x, y);
  int utype = TYP(under);
  int uID = ID(under);
  switch (utype) {
  case PT_SPRK: {
    //remove active sparks
    int sparked = parts[uID].ctype;
    if (!part_change_type(uID, x, y, sparked)) {
      parts[uID].life = 44 + parts[uID].life;
      parts[uID].ctype = 0;
    } else
      utype = 0;
    break;
  }
  case PT_DEUT:
    if (rng_chance(-((int)pv[y / ACELL][x / ACELL] - 4) + (parts[uID].life / 100), 200)) {
      PROT_DeutImplosion(parts[uID].life, x, y, restrict_flt(p_temp(&parts[uID]) + parts[uID].life * 500, MIN_TEMP, MAX_TEMP), PT_PROT);
      kill_part(uID);
    }
    break;
  case PT_LCRY:
    //Powered LCRY reaction: PROT->PHOT
    if (parts[uID].life > 5 && rng_chance(1, 10)) {
      //@ PROT + LCRY -> PHOT + LCRY
      part_change_type(i, x, y, PT_PHOT);
      parts[i].life *= 2;
      parts[i].ctype = 0x3FFFFFFF;
    }
    break;
  case PT_EXOT:
    parts[uID].ctype = PT_PROT;
    break;
  case PT_WIFI: {
    float change;
    if (p_temp(&parts[i]) < 173.15f) change = -1000.0f;
    else if (p_temp(&parts[i]) < 273.15f) change = -100.0f;
    else if (p_temp(&parts[i]) > 473.15f) change = 1000.0f;
    else if (p_temp(&parts[i]) > 373.15f) change = 100.0f;
    else change = 0.0f;
    p_set_temp(&parts[uID], restrict_flt(p_temp(&parts[uID]) + change, MIN_TEMP, MAX_TEMP));
    break;
  }
  case PT_RSSS: { //Destroy RSSS
      kill_part(uID);
      kill_part(i);
      return 1;
    }
    break;
  case PT_SEED: // Mutate seeds
    if (rng_chance(1, 2)) {
      int tmps[4] = {parts[uID].tmp, parts[uID].tmp2, parts[uID].tmp3, tmp4_get(uID)};
      int j = rng_between(0, 3);
      int k = rng_between(0, 3);
      int old_tmpj = tmps[j];
      int old_tmpk = tmps[k];
      // Swap genes for j-th and k-th phases
      tmps[j] = old_tmpk;
      tmps[k] = old_tmpj;
      parts[uID].tmp = (int16_t)tmps[0], parts[uID].tmp2 = (int16_t)tmps[1], parts[uID].tmp3 = (int16_t)tmps[2];
      tmp4_set(uID, tmps[3]);
    }
    break;
  case PT_NONE:
    //slowly kill if it's not inside an element
    if (parts[i].life) {
      if (!--parts[i].life) {
        kill_part(i);
        return 1;
      }
    }
    break;
  default:
    //set off explosives (only when hot because it wasn't as fun when it made an entire save explode)
    if (p_temp(&parts[i]) > 273.15f + 500.0f && (elements[utype].Flammable || elements[utype].Explosive || utype == PT_BANG)) {
      create_part(uID, x, y, PT_FIRE, -1);
      p_set_temp(&parts[uID], p_temp(&parts[uID]) + (restrict_flt((float)(elements[utype].Flammable * 5), MIN_TEMP, MAX_TEMP)));
      pv[y / ACELL][x / ACELL] += 1.00f;
    }
    //prevent inactive sparkable elements from being sparked
    else if ((elements[utype].Properties&PROP_CONDUCTS) && parts[uID].life <= 4) {
      parts[uID].life = 40 + parts[uID].life;
    }
    break;
  }
  //make temp of other things closer to it's own temperature. This will change temp of things that don't conduct, and won't change the PROT's temperature
  if (utype && utype != PT_WIFI)
    p_set_temp(&parts[uID], restrict_flt(p_temp(&parts[uID])-(p_temp(&parts[uID])-p_temp(&parts[i]))/4.0f, MIN_TEMP, MAX_TEMP));
  //if this proton has collided with another last frame, change it into a heavier element
  if (PROT_tmp(i)) {
    int newID, element, ptmp = PROT_tmp(i);
    if (ptmp > 500000) {
      if (utype == PT_TUNG)
        element = PT_AMTR; //@ PROT + TUNG -> AMTR
      else //@ PROT -> SING
        element = PT_SING; //particle accelerators are known to create earth-destroying black holes
    } else if (ptmp > 700)
      element = PT_PLUT; //@ PROT -> PLUT
    else if (ptmp > 420)
      element = PT_URAN; //@ PROT -> URAN
    else if (ptmp > 310)
      element = PT_POLO; //@ PROT -> POLO
    else if (ptmp > 250)
      element = PT_PLSM; //@ PROT -> PLSM
    else if (ptmp > 100)
      element = PT_O2; //@ PROT -> O2
    else if (ptmp > 50)
      element = PT_CO2; //@ PROT -> CO2
    else
      element = PT_NBLE; //@ PROT -> NBLE
    newID = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), element, -1);
    if (newID >= 0)
      p_set_temp(&parts[newID], restrict_flt(100.0f*ptmp, MIN_TEMP, MAX_TEMP));
    kill_part(i);
    return 1;
  }
  //collide with other protons to make heavier materials
  int ahead = PHOTAT(x, y);
  if (ID(ahead) != i && TYP(ahead) == PT_PROT) {
    float velocity1 = powf(p_vx(&parts[i]), 2.0f)+powf(p_vy(&parts[i]), 2.0f);
    float velocity2 = powf(p_vx(&parts[ID(ahead)]), 2.0f)+powf(p_vy(&parts[ID(ahead)]), 2.0f);
    float direction1 = atan2f(-p_vy(&parts[i]), p_vx(&parts[i]));
    float direction2 = atan2f(-p_vy(&parts[ID(ahead)]), p_vx(&parts[ID(ahead)]));
    float difference = direction1 - direction2; if (difference < 0) difference += 6.28319f;
    if (difference > 3.12659f && difference < 3.15659f && velocity1 + velocity2 > 10.0f) {
      PROT_set_tmp(ID(ahead), PROT_tmp(ID(ahead)) + (int)(velocity1 + velocity2));
      kill_part(i);
      return 1;
    }
  }
  return 0;
}
static int PROT_DeutImplosion(int n, int x, int y, float temp, int t) {
  int i;
  n = (n/50);
  if (n < 1)
    n = 1;
  else if (n > 340)
    n = 340;
  for (int c = 0; c < n; c++) {
    i = create_part(-3, x, y, t, -1);
    if (i >= 0)
      p_set_temp(&parts[i], temp);
    else if (parts_full())
      break;
  }
  pv[y/ACELL][x/ACELL] -= (6.0f * CFDS)*n;
  return 0;
}
void el_PROT_create(ELEMENT_CREATE_FUNC_ARGS) {
  float a = rng_between(0, 35) * 0.17453f;
  parts[i].life = 680;
  p_set_vx(&parts[i], 2.0f * cosf(a));
  p_set_vy(&parts[i], 2.0f * sinf(a));
}

/* ---------------------------------------------------------------- GRVT */
int el_GRVT_update(UPDATE_FUNC_ARGS) {
  //at higher tmps they just go completely insane
  if (parts[i].tmp >= 100)
    parts[i].tmp = 100;
  if (parts[i].tmp <= -100)
    parts[i].tmp = -100;
  int under = PMAPAT(x, y);
  int utype = TYP(under);
  //Randomly kill GRVT inside RSSS
  if((utype == PT_RSSS) && rng_chance(1, 5)) {
    kill_part(i);
    return 1;
  }
  gravmass[(y )/ACELL][(x)/ACELL] = 0.2f * parts[i].tmp;
  return 0;
}
void el_GRVT_create(ELEMENT_CREATE_FUNC_ARGS) {
  float a = rng_between(0, 359) * NP_PI / 180.0f;
  parts[i].life = 250 + rng_between(0, 199);
  p_set_vx(&parts[i], 2.0f * cosf(a));
  p_set_vy(&parts[i], 2.0f * sinf(a));
}

/* ---------------------------------------------------------------- EXOT */
int el_EXOT_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt == PT_WARP) {
          if (parts[ID(r)].tmp2>2000 && rng_chance(1, 100)) {
            parts[i].tmp2 += 100;
          }
        } else if (rt == PT_EXOT) {
          if (parts[ID(r)].ctype == PT_PROT)
            parts[i].ctype = PT_PROT;
          if (parts[ID(r)].life == 1500 && rng_chance(1, 1000))
            parts[i].life = 1500;
        } else if (rt == PT_LAVA) {
          //turn molten TTAN or molten GOLD to molten VIBR
          if (parts[ID(r)].ctype == PT_TTAN || parts[ID(r)].ctype == PT_GOLD) {
            if (rng_chance(1, 10)) {
              //@ EXOT + LAVA(TTAN/GOLD) -> LAVA(VIBR)
              parts[ID(r)].ctype = PT_VIBR;
              kill_part(i);
              return 1;
            }
          }
          //molten VIBR will kill the leftover EXOT though, so the VIBR isn't killed later
          else if (parts[ID(r)].ctype == PT_VIBR) {
            if (rng_chance(1, 1000)) {
              kill_part(i);
              return 1;
            }
          }
        }
        if (parts[i].tmp > 245 && parts[i].life > 1337)
          if (rt!=PT_EXOT && rt!=PT_BREC && rt!=PT_DMND && rt!=PT_CLNE && rt!=PT_PRTI && rt!=PT_PRTO && rt!=PT_PCLN && rt!=PT_VOID && rt!=PT_NBHL && rt!=PT_WARP) {
            if (create_part(i, x, y, rt, -1) != -1) {
              return 1;
            }
          }
      }
    }
  }
  parts[i].tmp--;
  parts[i].tmp2--;
  //reset tmp every 250 frames, gives EXOT it's slow flashing effect
  if (parts[i].tmp < 1 || parts[i].tmp > 250)
    parts[i].tmp = 250;
  if (parts[i].tmp2 < 1)
    parts[i].tmp2 = 1;
  else if (parts[i].tmp2 > 6000) {
    parts[i].tmp2 = 10000;
    if (parts[i].life < 1001) {
      //@ EXOT -> WARP
      part_change_type(i, x, y, PT_WARP);
      return 1;
    }
  } else if(parts[i].life < 1001)
    pv[y/ACELL][x/ACELL] += (parts[i].tmp2*CFDS)/160000;
  if (pv[y/ACELL][x/ACELL]>200 && p_temp(&parts[i])>9000 && parts[i].tmp2>200) {
    parts[i].tmp2 = 6000;
    //@ EXOT -> WARP
    part_change_type(i, x, y, PT_WARP);
    return 1;
  }
  if (parts[i].tmp2 > 100) {
    for (int trade = 0; trade < 9; trade++) {
      int rx = rng_between(-2, 2);
      int ry = rng_between(-2, 2);
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_EXOT && (parts[i].tmp2 > parts[ID(r)].tmp2) && parts[ID(r)].tmp2 >= 0) { //diffusion
          int tym = parts[i].tmp2 - parts[ID(r)].tmp2;
          if (tym == 1) {
            parts[ID(r)].tmp2++;
            parts[i].tmp2--;
            break;
          }
          if (tym > 0) {
            parts[ID(r)].tmp2 += tym/2;
            parts[i].tmp2 -= tym/2;
            break;
          }
        }
      }
    }
  }
  if (parts[i].ctype == PT_PROT) {
    if (p_temp(&parts[i]) < 50.0f) {
      //@ EXOT(PROT) -> CFLM
      if (create_part(i, x, y, PT_CFLM, -1) != -1) { // I don't see how this could fail but whatever
        return 1;
      }
    } else
      p_set_temp(&parts[i], p_temp(&parts[i]) - (1.0f));
  } else if (p_temp(&parts[i]) < 273.15f) {
    p_set_vx(&parts[i], 0);
    p_set_vy(&parts[i], 0);
    pv[y/ACELL][x/ACELL] -= 0.01f;
    parts[i].tmp--;
  }
  return 0;
}

/* ---------------------------------------------------------------- POLO */
#define COOLDOWN 15
#define LIMIT 5
int el_POLO_update(UPDATE_FUNC_ARGS) {
  int r = PHOTAT(x, y);
  if (parts[i].tmp < LIMIT && !parts[i].life) {
    if (rng_chance(1, 10000) && !parts[i].tmp) {
      //@ POLO -> POLO + NEUT
      int s = create_part(-3, x, y, PT_NEUT, -1);
      if (s >= 0) {
        parts[i].life = COOLDOWN;
        parts[i].tmp++;
        p_set_temp(&parts[i], ((p_temp(&parts[i]) + p_temp(&parts[s])) + 600.0f) / 2.0f);
        p_set_temp(&parts[s], p_temp(&parts[i]));
      }
    }
    if (r && rng_chance(1, 100)) {
      //@ POLO -> POLO + NEUT
      int s = create_part(-3, x, y, PT_NEUT, -1);
      if (s >= 0) {
        p_set_temp(&parts[i], ((p_temp(&parts[i]) + p_temp(&parts[ID(r)]) + p_temp(&parts[ID(r)])) + 600.0f) / 3.0f);
        parts[i].life = COOLDOWN;
        parts[i].tmp++;
        p_set_temp(&parts[ID(r)], p_temp(&parts[i]));
        p_set_temp(&parts[s], p_temp(&parts[i]));
        p_set_vx(&parts[s], p_vx(&parts[ID(r)]));
        p_set_vy(&parts[s], p_vy(&parts[ID(r)]));
      }
    }
  }
  if (parts[i].tmp2 >= 10) {
    //@ POLO -> PLUT
    part_change_type(i,x,y,PT_PLUT);
    p_set_temp(&parts[i], (p_temp(&parts[i])+600.0f)/2.0f);
    return 1;
  }
  if (TYP(r) == PT_PROT) {
    parts[i].tmp2++;
    kill_part(ID(r));
  }
  if (p_temp(&parts[i]) < 388.15f) {
    p_set_temp_d(&parts[i], p_temp(&parts[i]) + (0.2f));
  }
  return 0;
}

