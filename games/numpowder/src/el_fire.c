/* NumPowder: The Powder Toy's fire and explosives (their files in elements/), ported: FIRE (and LAVA, PLSM), CFLM,
 * COAL (and BCOL), IGNT, FUSE, FSEP, BANG, BOMB, C5, DEST, FIRW, FWRK, GBMB, LIGH, THDR, EMBR, LITH. */
#include "simpriv.h"

/* ---------------------------------------------------------------- FIRE */
static int FIRE_updateLegacy(UPDATE_FUNC_ARGS);
int Element_FIRE_update(UPDATE_FUNC_ARGS) {
  int t = parts[i].type;
  switch (t) {
  case PT_PLSM:
    if (parts[i].life <=1) {
      if (parts[i].ctype == PT_NBLE) {
        //@ PLSM(NBLE) -> NBLE
        part_change_type(i,x,y,PT_NBLE);
        parts[i].life = 0;
      } else if ((parts[i].tmp&0x3) == 3) {
        //@ PLSM + O2 + H2 -> WTRV(FIRE)
        part_change_type(i,x,y,PT_WTRV);
        parts[i].life = 0;
        parts[i].ctype = PT_FIRE;
      }
    }
    break;
  case PT_FIRE:
    if (parts[i].life <=1) {
      if ((parts[i].tmp&0x3) == 3) {
        //@ FIRE + O2 + H2 -> WTRV(FIRE)
        part_change_type(i,x,y,PT_WTRV);
        parts[i].life = 0;
        parts[i].ctype = PT_FIRE;
      } else if (p_temp(&parts[i])<625) {
        //@ FIRE -> SMKE
        part_change_type(i,x,y,PT_SMKE);
        parts[i].life = rng_between(250, 269);
      }
    }
    break;
  case PT_LAVA: {
    float pres = pv[y / ACELL][x / ACELL];
    if (parts[i].ctype == PT_ROCK) {
      if (pres <= -9) {
        //@ LAVA(ROCK) -> LAVA(STNE)
        parts[i].ctype = PT_STNE;
        break;
      }
      if (pres >= 25 && rng_chance(1, 12500)) {
        if (pres <= 50) {
          //@ LAVA(ROCK) -> LAVA(BRMT/CNCT)
          if (rng_chance(1, 2))
            parts[i].ctype = PT_BRMT;
          else
            parts[i].ctype = PT_CNCT;
        } else if (pres <= 75) {
          //@ LAVA(ROCK) -> LAVA(GOLD/QRTZ)
          if (pres >= 73 || rng_chance(1, 8))
            parts[i].ctype = PT_GOLD;
          else
            parts[i].ctype = PT_QRTZ;
        } else if (pres <= 100 && p_temp(&parts[i]) >= 5000) {
          //@ LAVA(ROCK) -> LAVA(TTAN/IRON)
          if (rng_chance(1, 5)) // 1 in 5 chance IRON to TTAN
            parts[i].ctype = PT_TTAN;
          else
            parts[i].ctype = PT_IRON;
        } else if (p_temp(&parts[i]) >= 5000 && rng_chance(1, 5)) {
          //@ LAVA(ROCK) -> LAVA(URAN/PLUT/TUNG)
          if (rng_chance(1, 5))
            parts[i].ctype = PT_URAN;
          else if (rng_chance(1, 5))
            parts[i].ctype = PT_PLUT;
          else
            parts[i].ctype = PT_TUNG;
        }
      }
    } else if (parts[i].ctype == PT_GOLD && pres < -200.0f && p_temp(&parts[i]) > elements[PT_PTNM].HighTemperature && rng_chance(1, 20000)) {
      //@ LAVA(GOLD) -> LAVA(PTNM)
      parts[i].ctype = PT_PTNM;
      pv[y/ACELL][x/ACELL] += 2.0f;
    } else if ((parts[i].ctype == PT_STNE || !parts[i].ctype) && pres >= 30.0f && (p_temp(&parts[i]) > elements[PT_ROCK].HighTemperature || pres < elements[PT_ROCK].HighPressure)) { // Form ROCK with pressure, if it will stay molten or not immediately break
      //@ LAVA -> LAVA(ROCK)
      parts[i].tmp2 = rng_between(0, 10); // Provide tmp2 for color noise
      parts[i].ctype = PT_ROCK;
    }
    break;
  }
  default:
    break;
  }
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        //THRM burning
        if (rt==PT_THRM && (t==PT_FIRE || t==PT_PLSM || t==PT_LAVA)) {
          //@ FIRE/PLSM/LAVA + THRM -> FIRE/PLSM/LAVA + LAVA(BMTL/THRM)
          if (rng_chance(1, 500)) {
            part_change_type(ID(r),x+rx,y+ry,PT_LAVA);
            parts[ID(r)].ctype = PT_BMTL;
            p_set_temp(&parts[ID(r)], 3500.0f);
            pv[(y+ry)/ACELL][(x+rx)/ACELL] += 50.0f;
          } else {
            part_change_type(ID(r),x+rx,y+ry,PT_LAVA);
            parts[ID(r)].life = 400;
            parts[ID(r)].ctype = PT_THRM;
            p_set_temp(&parts[ID(r)], 3500.0f);
            parts[ID(r)].tmp = 20;
          }
          continue;
        }
        if ((rt==PT_COAL) || (rt==PT_BCOL)) {
          if ((t==PT_FIRE || t==PT_PLSM)) {
            if (parts[ID(r)].life>100 && rng_chance(1, 500)) {
              parts[ID(r)].life = 99;
            }
          } else if (t==PT_LAVA) {
            //@ LAVA(IRON) + COAL/BCOL -> LAVA(METL)
            if (parts[i].ctype == PT_IRON && rng_chance(1, 500)) {
              parts[i].ctype = PT_METL;
              kill_part(ID(r));
              continue;
            }
            //@ LAVA + COAL/BCOL -> LAVA(SLCN)
            if ((parts[i].ctype == PT_STNE || parts[i].ctype == PT_NONE) && rng_chance(1, 60)) {
              parts[i].ctype = PT_SLCN;
              kill_part(ID(r));
              continue;
            }
          }
        }
        if (t == PT_LAVA) {
          //@ LAVA(QRTZ) + LAVA(CLST) -> 2xLAVA(CRMC)
          if (parts[i].ctype == PT_QRTZ && rt == PT_LAVA && parts[ID(r)].ctype == PT_CLST) {
            float pres = fmaxf(pv[y/ACELL][x/ACELL]*10.0f, 0.0f);
            if (p_temp(&parts[i]) >= pres+elements[PT_CRMC].HighTemperature+50.0f) {
              parts[i].ctype = PT_CRMC;
              parts[ID(r)].ctype = PT_CRMC;
            }
          } else if (rt == PT_O2 && parts[i].ctype == PT_SLCN) {
            //@ LAVA(SLCN) + O2 -> LAVA(SAND/CLST/PQRT/STNE)
            switch (rng_between(0, 2)) {
            case 0:
              parts[i].ctype = PT_SAND;
              break;
            case 1:
              parts[i].ctype = PT_CLST;
              // avoid creating CRMC.
              if (p_temp(&parts[i]) >= elements[PT_PQRT].HighTemperature * 3) {
                parts[i].ctype = PT_PQRT;
              }
              break;
            case 2:
              parts[i].ctype = PT_STNE;
              break;
            }
            parts[i].tmp = 0;
            kill_part(ID(r));
            continue;
          } else if (rt == PT_LAVA && (parts[ID(r)].ctype == PT_METL || parts[ID(r)].ctype == PT_BMTL) && parts[i].ctype == PT_SLCN) {
            //@ LAVA(SLCN) + LAVA(METL/BMTL) -> LAVA(NSCN) + LAVA(PSCN)
            parts[i].tmp = 0;
            parts[i].ctype = PT_NSCN;
            parts[ID(r)].ctype = PT_PSCN;
          } else if (parts[i].ctype == PT_SLCN && rt == PT_LAVA && parts[ID(r)].ctype == PT_SALT) {
            //@ LAVA(SLCN) + LAVA(SALT) -> LAVA(LITH)
            if (p_temp(&parts[i]) > elements[PT_LITH].HighTemperature && rng_chance(1, 1000)) {
              parts[i].ctype = PT_LITH;
              parts[i].tmp = 0;
              parts[i].tmp2 = 0;
              parts[i].life = 0;
              kill_part(ID(r));
              continue;
            }
          } else if (rt == PT_HEAC && parts[i].ctype == PT_HEAC) {
            if (p_temp(&parts[ID(r)]) > elements[PT_HEAC].HighTemperature) {
              part_change_type(ID(r), x+rx, y+ry, PT_LAVA);
              parts[ID(r)].ctype = PT_HEAC;
            }
          } else if (parts[i].ctype == PT_ROCK && rt == PT_LAVA && parts[ID(r)].ctype == PT_GOLD && parts[ID(r)].tmp == 0 &&
            pv[y / ACELL][x / ACELL] >= 50 && rng_chance(1, 10000)) { // Produce GOLD veins/clusters
            //@ LAVA(ROCK) + LAVA(GOLD) -> 2xLAVA(GOLD)
            parts[i].ctype = PT_GOLD;
            if (rx > 1 || rx < -1) // Trend veins vertical
              parts[i].tmp = 1;
          } else if (parts[i].ctype == PT_SALT && rt == PT_GLAS && parts[ID(r)].life < 234 * 120) {
            parts[ID(r)].life++;
          }
        }
        if ((surround_space || elements[rt].Explosive) &&
            elements[rt].Flammable && rng_chance((int)(elements[rt].Flammable + (pv[(y+ry)/ACELL][(x+rx)/ACELL] * 10.0f)), 1000) &&
            //exceptions, t is the thing causing the spark and rt is what's burning
            (t != PT_SPRK || (rt != PT_RBDM && rt != PT_LRBD && rt != PT_INSL)) &&
            (t != PT_PHOT || rt != PT_INSL) &&
            (rt != PT_SPNG || parts[ID(r)].life == 0)) {
          part_change_type(ID(r), x+rx, y+ry, PT_FIRE);
          p_set_temp(&parts[ID(r)], restrict_flt(elements[PT_FIRE].DefaultProperties.temp + (elements[rt].Flammable/2), MIN_TEMP, MAX_TEMP));
          parts[ID(r)].life = rng_between(180, 259);
          parts[ID(r)].tmp = parts[ID(r)].ctype = 0;
          if (elements[rt].Explosive)
            pv[y/ACELL][x/ACELL] += 0.25f * CFDS;
        }
      }
    }
  }
  if (legacy_enable && t!=PT_SPRK) // SPRK has no legacy reactions
    FIRE_updateLegacy(UPDATE_FUNC_SUBCALL_ARGS);
  return 0;
}
static int FIRE_updateLegacy(UPDATE_FUNC_ARGS) {
  int t = parts[i].type;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (bmap[(y+ry)/WCELL][(x+rx)/WCELL] && bmap[(y+ry)/WCELL][(x+rx)/WCELL]!=WL_STREAM)
          continue;
        int rt = TYP(r);
        int lpv = (int)pv[(y+ry)/ACELL][(x+rx)/ACELL];
        if (lpv < 1) lpv = 1;
        if (elements[rt].Meltable &&
                ((rt!=PT_RBDM && rt!=PT_LRBD) || t!=PT_SPRK)
                && ((t!=PT_FIRE&&t!=PT_PLSM) || (rt!=PT_METL && rt!=PT_IRON && rt!=PT_ETRD && rt!=PT_PSCN && rt!=PT_NSCN && rt!=PT_NTCT && rt!=PT_PTCT && rt!=PT_BMTL && rt!=PT_BRMT && rt!=PT_SALT && rt!=PT_INWR))
                && rng_chance(elements[rt].Meltable*lpv, 1000)) {
          if (t!=PT_LAVA || parts[i].life>0) {
            if (rt==PT_BRMT)
              parts[ID(r)].ctype = PT_BMTL;
            else if (rt==PT_SAND)
              parts[ID(r)].ctype = PT_GLAS;
            else
              parts[ID(r)].ctype = rt;
            part_change_type(ID(r),x+rx,y+ry,PT_LAVA);
            parts[ID(r)].life = rng_between(240, 359);
          } else {
            parts[i].life = 0;
            parts[i].ctype = PT_NONE;//rt;
            part_change_type(i,x,y,(parts[i].ctype)?parts[i].ctype:PT_STNE);
            return 1;
          }
        }
        if (rt==PT_ICEI || rt==PT_SNOW) {
          part_change_type(ID(r), x+rx, y+ry, PT_WATR);
          if (t==PT_FIRE) {
            kill_part(i);
            return 1;
          }
          if (t==PT_LAVA) {
            parts[i].life = 0;
            part_change_type(i,x,y,PT_STNE);
          }
        }
        if (rt==PT_WATR || rt==PT_DSTW || rt==PT_SLTW) {
          kill_part(ID(r));
          if (t==PT_FIRE) {
            kill_part(i);
            return 1;
          }
          if (t==PT_LAVA) {
            parts[i].life = 0;
            parts[i].ctype = PT_NONE;
            part_change_type(i,x,y,(parts[i].ctype)?parts[i].ctype:PT_STNE);
          }
        }
      }
    }
  }
  return 0;
}
void el_FIRE_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].life = rng_between(120, 169);
}

/* ---------------------------------------------------------------- LAVA */
void el_LAVA_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].life = rng_between(240, 359);
}

/* ---------------------------------------------------------------- PLSM */
void el_PLSM_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].life = rng_between(50, 199);
}

/* ---------------------------------------------------------------- CFLM */
void el_CFLM_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].life = rng_between(50, 199);
}

/* ---------------------------------------------------------------- COAL */
int Element_COAL_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life<=0) {
    create_part(i, x, y, PT_FIRE, -1);
    return 1;
  } else if (parts[i].life < 100) {
    parts[i].life--;
    create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_FIRE, -1);
  }
  if (parts[i].type == PT_COAL) {
    if ((pv[y/ACELL][x/ACELL] > 4.3f)&&parts[i].tmp>40)
      parts[i].tmp=39;
    else if (parts[i].tmp<40&&parts[i].tmp>0)
      parts[i].tmp--;
    else if (parts[i].tmp<=0) {
      //@ COAL -> BCOL
      part_change_type(i, x, y, PT_BCOL);
      return 1;
    }
  }
  if(p_temp(&parts[i]) > parts[i].tmp2)
    parts[i].tmp2 = (int)(p_temp(&parts[i]));
  return 0;
}

/* ---------------------------------------------------------------- IGNT */
int el_IGNT_update(UPDATE_FUNC_ARGS) {
  if(parts[i].tmp==0) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          int rt = TYP(r);
          if (rt==PT_FIRE || rt==PT_PLSM || rt==PT_SPRK || rt==PT_LIGH || (rt==PT_IGNT && parts[ID(r)].life==1)) {
            parts[i].tmp = 1;
          }
        }
      }
    }
  } else if(parts[i].life > 0) {
    if (rng_chance(2, 3)) {
      //@ IGNT -> IGNT + EMBR
      int nb = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_EMBR, -1);
      if (nb!=-1) {
        parts[nb].tmp = 0;
        parts[nb].life = 30;
        p_set_vx(&parts[nb], (float)(rng_between(-10, 10)));
        p_set_vy(&parts[nb], (float)(rng_between(-10, 10)));
        p_set_temp(&parts[nb], restrict_flt(p_temp(&parts[i])-273.15f+400.0f, MIN_TEMP, MAX_TEMP));
      }
    } else {
      //@ IGNT -> IGNT + FIRE
      create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_FIRE, -1);
    }
    parts[i].life--;
  }
  return 0;
}

/* ---------------------------------------------------------------- FUSE */
int el_FUSE_update(UPDATE_FUNC_ARGS) {
  //@ FUSE -> FUSE + PLSM
  if (parts[i].life<=0) {
    int r = create_part(i, x, y, PT_PLSM, -1);
    if (r>-1)
      parts[r].life = 50;
    return 1;
  } else if (parts[i].life < 40) {
    parts[i].life--;
    if (rng_chance(1, 100)) {
      int r = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_PLSM, -1);
      if (r>-1)
        parts[r].life = 50;
    }
  }
  if ((pv[y/ACELL][x/ACELL] > 2.7f) && parts[i].tmp>40)
    parts[i].tmp=39;
  else if (parts[i].tmp<=0) {
    //@ FUSE -> FSEP
    create_part(i, x, y, PT_FSEP, -1);
    return 1;
  } else if (parts[i].tmp<40)
    parts[i].tmp--;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SPRK || (p_temp(&parts[i])>=(273.15+700.0f) && rng_chance(1, 20))) {
          if (parts[i].life > 40)
            parts[i].life = 39;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- FSEP */
int el_FSEP_update(UPDATE_FUNC_ARGS) {
  //@ FSEP -> FSEP + PLSM
  if (parts[i].life<=0) {
    int r = create_part(i, x, y, PT_PLSM, -1);
    if (r!=-1)
      parts[r].life = 50;
    return 1;
  } else if (parts[i].life < 40) {
    parts[i].life--;
    if (rng_chance(1, 10)) {
      int r = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_PLSM, -1);
      if (r>-1)
        parts[r].life = 50;
    }
  } else {
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if ((TYP(r)==PT_SPRK || (p_temp(&parts[i])>=(273.15+400.0f))) && parts[i].life>40 && rng_chance(1, 15)) {
            parts[i].life = 39;
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- BANG */
int el_BANG_update(UPDATE_FUNC_ARGS) {
  if(parts[i].tmp==0) {
    if(p_temp(&parts[i])>=673.0f)
      parts[i].tmp = 1;
    else {
      for (int rx = -1; rx <= 1; rx++) {
        for (int ry = -1; ry <= 1; ry++) {
          if (rx || ry) {
            int r = PMAPAT(x+rx, y+ry);
            if (!r)
              continue;
            if (TYP(r)==PT_FIRE || TYP(r)==PT_PLSM || TYP(r)==PT_SPRK || TYP(r)==PT_LIGH) {
              parts[i].tmp = 1;
            }
          }
        }
      }
    }
  } else if(parts[i].tmp==1) {
    if (PMAPAT(x, y) && ID(PMAPAT(x, y)) == i) {
      flood_prop_int(x, y, FIELD_TMP, 2);
    }
    parts[i].tmp = 2;
  } else if(parts[i].tmp==2) {
    parts[i].tmp = 3;
  } else {
    float otemp = p_temp(&parts[i])-273.15f;
    //Explode!!
    pv[y/ACELL][x/ACELL] += 0.5f;
    parts[i].tmp = 0;
    if (rng_chance(1, 3)) {
      if (rng_chance(1, 2)) {
        //@ BANG -> FIRE
        create_part(i, x, y, PT_FIRE, -1);
      } else {
        //@ BANG -> SMKE
        create_part(i, x, y, PT_SMKE, -1);
        parts[i].life = rng_between(500, 549);
      }
      p_set_temp(&parts[i], restrict_flt((MAX_TEMP/4)+otemp, MIN_TEMP, MAX_TEMP));
    } else {
      if (rng_chance(1, 15)) {
        //@ BANG -> EMBR
        create_part(i, x, y, PT_EMBR, -1);
        parts[i].tmp = 0;
        parts[i].life = 50;
        p_set_temp(&parts[i], restrict_flt((MAX_TEMP/3)+otemp, MIN_TEMP, MAX_TEMP));
        p_set_vx(&parts[i], (float)(rng_between(-10, 10)));
        p_set_vy(&parts[i], (float)(rng_between(-10, 10)));
      } else {
        kill_part(i);
      }
    }
    return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------- BOMB */
int el_BOMB_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt!=PT_BOMB && rt!=PT_EMBR && rt!=PT_DMND && rt!=PT_CLNE && rt!=PT_PCLN && rt!=PT_BCLN && rt!=PT_VIBR) {
          const int rad = 8;
          kill_part(i);
          for (int nxj =-rad; nxj<=rad; nxj++) {
            for (int nxi =-rad; nxi<=rad; nxi++) {
              if ((nxi*nxi + nxj*nxj) <= rad*rad) {
                int ynxj = y+nxj, xnxi = x+nxi;
                if ((ynxj < 0) || (ynxj >= YRES) || (xnxi < 0) || (xnxi >= XRES))
                  continue;
                int nt = TYP(PMAPAT(xnxi, ynxj));
                if (nt!=PT_DMND && nt!=PT_CLNE && nt!=PT_PCLN && nt!=PT_BCLN && nt!=PT_VIBR) {
                  if (nt)
                    kill_part(ID(PMAPAT(xnxi, ynxj)));
                  pv[ynxj/ACELL][xnxi/ACELL] += 0.1f;
                  //@ BOMB -> EMBR
                  int nb = create_part(-3, xnxi, ynxj, PT_EMBR, -1);
                  if (nb!=-1) {
                    parts[nb].tmp = 2;
                    parts[nb].life = 2;
                    p_set_temp(&parts[nb], MAX_TEMP);
                  }
                }
              }
            }
          }
          for (int nxj =-(rad+1); nxj<=(rad+1); nxj++) {
            for (int nxi =-(rad+1); nxi<=(rad+1); nxi++) {
              if ((nxi*nxi + nxj*nxj) <= (rad+1)*(rad+1)) {
                int ynxj = y+nxj, xnxi = x+nxi;
                if ((ynxj < 0) || (ynxj >= YRES) || (xnxi < 0) || (xnxi >= XRES))
                  continue;
                if (TYP(PMAPAT(xnxi, ynxj)))
                  continue;
                //@ BOMB -> EMBR
                int nb = create_part(-3, xnxi, ynxj, PT_EMBR, -1);
                if (nb!=-1) {
                  parts[nb].tmp = 0;
                  parts[nb].life = 50;
                  p_set_temp(&parts[nb], MAX_TEMP);
                  p_set_vx(&parts[nb], (float)(rng_between(-20, 20)));
                  p_set_vy(&parts[nb], (float)(rng_between(-20, 20)));
                }
              }
            }
          }
          return 1;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- C5 */
int el_C5_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if ((TYP(r)!=PT_C5 && p_temp(&parts[ID(r)])<100 && !IsHeatInsulator(&parts[ID(r)])) || TYP(r)==PT_CFLM) {
          if (rng_chance(1, 6)) {
            //@ C5 -> CFLM
            part_change_type(i,x,y,PT_CFLM);
            p_set_temp(&parts[i], 0);
            p_set_temp(&parts[ID(r)], 0);
            parts[i].life = rng_between(50, 199);
            pv[y/ACELL][x/ACELL] += 1.5;
          }
        }
      }
    }
  }
  if (parts[i].ctype && !parts[i].life) {
    /* the photon's velocity (the original's tmp, 16 bits each) is in tmp and tmp3, its place in tmp2 */
    float pvx = parts[i].tmp / 255.0f;
    float pvy = parts[i].tmp3 / 255.0f;
    float dx = (int8_t)(parts[i].tmp2 & 0xFF) / 255.0f;
    float dy = (parts[i].tmp2 >> 8) / 255.0f;
    int r = create_part(-3, x, y, PT_PHOT, -1);
    if (r != -1) {
      parts[r].ctype = parts[i].ctype;
      p_set_x(&parts[r], p_x(&parts[r]) + (dx));
      p_set_y(&parts[r], p_y(&parts[r]) + (dy));
      p_set_vx(&parts[r], pvx);
      p_set_vy(&parts[r], pvy);
      p_set_temp(&parts[r], p_temp(&parts[i]));
    }
    parts[i].ctype = 0;
    parts[i].tmp = 0;
    parts[i].tmp2 = 0;
    parts[i].tmp3 = 0;
  }
  return 0;
}

/* ---------------------------------------------------------------- DEST */
int el_DEST_update(UPDATE_FUNC_ARGS) {
  int rx = rng_between(-2, 2);
  int ry = rng_between(-2, 2);
  int r = PMAPAT(x+rx, y+ry);
  if (!r)
    return 0;
  int rt = TYP(r);
  if (rt == PT_DEST || rt == PT_DMND || rt == PT_BCLN  || rt == PT_CLNE  || rt == PT_PCLN  || rt == PT_PBCN)
    return 0;
  if (parts[i].life<=0 || parts[i].life>37) {
    parts[i].life = rng_between(30, 49);
    pv[y/ACELL][x/ACELL]+=60.0f;
  }
  if (rt == PT_PLUT || rt == PT_DEUT) {
    pv[y/ACELL][x/ACELL]+=20.0f;
    if (rng_chance(1, 2)) {
      //@ DEST + PLUT/DEUT -> DEST + NEUT
      create_part(ID(r), x+rx, y+ry, PT_NEUT, -1);
      p_set_temp(&parts[ID(r)], MAX_TEMP);
      pv[y/ACELL][x/ACELL] += 10.0f;
      parts[i].life-=4;
    }
  } else if (rt == PT_INSL) {
    //@ DEST + INSL -> DEST + PLSM
    create_part(ID(r), x+rx, y+ry, PT_PLSM, -1);
  } else if (rng_chance(1, 3)) {
    kill_part(ID(r));
    parts[i].life -= 4*((elements[rt].Properties&TYPE_SOLID)?3:1);
    if (parts[i].life<=0)
      parts[i].life=1;
  } else if (!IsHeatInsulator(&parts[ID(r)]))
    p_set_temp(&parts[ID(r)], MAX_TEMP);
  p_set_temp(&parts[i], MAX_TEMP);
  pv[y/ACELL][x/ACELL] = restrict_flt(pv[y/ACELL][x/ACELL] + 80.0f, MIN_PRESSURE, MAX_PRESSURE);
  return 0;
}

/* ---------------------------------------------------------------- FIRW */
/* the renderer's firwTable (Gradient, 200 colours): the embers' colour in their ctype */
static uint32_t FIRW_colour(int index) {
  static const struct { uint32_t c; float point; } stops[] = {
    {0xFF00FF, 0.00f}, {0x0000FF, 0.20f}, {0x00FFFF, 0.40f}, {0x00FF00, 0.60f}, {0xFFFF00, 0.80f}, {0xFF0000, 1.00f}};
  if (index < 0) index = 0;
  if (index > 199) index = 199;
  float point = index / 200.0f;
  int stop = -1;
  while (stop < 5 && stops[stop + 1].point <= point) ++stop;
  if (stop < 0 || stop >= 5) return 0;
  float f = (point - stops[stop].point) / (stops[stop + 1].point - stops[stop].point);
  unsigned a = (uint8_t)(f * 0xFF);
  uint32_t l = stops[stop].c, r = stops[stop + 1].c, out = 0;
  if (a == 0xFF) return r;
  for (int sh = 0; sh <= 16; sh += 8)
    out |= (uint32_t)(((a * ((r >> sh) & 0xFF) + (0xFF - a) * ((l >> sh) & 0xFF)) & 0xFFFF) / 0xFF) << sh;
  return out;
}
int el_FIRW_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp<=0) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          int rt = TYP(r);
          if (rt==PT_FIRE||rt==PT_PLSM||rt==PT_THDR) {
            float gx, gy, multiplier;
            GetGravityField(x, y, elements[PT_FIRW].Gravity, 1.0f, &gx, &gy);
            if (gx*gx+gy*gy < 0.001f) {
              float angle = rng_between(0, 6283) * 0.001f;//(in radians, between 0 and 2*pi)
              gx += sinf(angle)*elements[PT_FIRW].Gravity*0.5f;
              gy += cosf(angle)*elements[PT_FIRW].Gravity*0.5f;
            }
            parts[i].tmp = 1;
            parts[i].life = rng_between(20, 29);
            multiplier = (parts[i].life+20)*0.2f/sqrtf(gx*gx+gy*gy);
            p_set_vx(&parts[i], p_vx(&parts[i]) - (gx*multiplier));
            p_set_vy(&parts[i], p_vy(&parts[i]) - (gy*multiplier));
            return 0;
          }
        }
      }
    }
  } else if (parts[i].tmp==1) {
    if (parts[i].life<=0) {
      parts[i].tmp=2;
    } else {
      parts[i].flags &= (uint8_t)~FLAG_STAGNANT;
    }
  } else { //if (parts[i].tmp>=2)
    unsigned col = FIRW_colour(rng_between(0, 199));
    for (int n=0; n<40; n++) {
      //@ FIRW -> EMBR
      int np = create_part(-3, x, y, PT_EMBR, -1);
      if (np>-1) {
        float magnitude = rng_between(40, 99) * 0.05f;
        float angle = rng_between(0, 6283) * 0.001f;//(in radians, between 0 and 2*pi)
        p_set_vx(&parts[np], p_vx(&parts[i])*0.5f + cosf(angle)*magnitude);
        p_set_vy(&parts[np], p_vy(&parts[i])*0.5f + sinf(angle)*magnitude);
        parts[np].ctype = col;
        parts[np].tmp = 1;
        parts[np].life = rng_between(70, 109);
        p_set_temp(&parts[np], (float)(rng_between(5750, 6249)));
      }
    }
    pv[y/ACELL][x/ACELL] += 8.0f;
    kill_part(i);
    return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------- FWRK */
int el_FWRK_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life == 0 && ((surround_space && p_temp(&parts[i])>400 && rng_chance((int)(9+p_temp(&parts[i])/40), 100000)) || parts[i].ctype == PT_DUST)) {
    float gx, gy, multiplier, gmax;
    int randTmp;
    GetGravityField(x, y, elements[PT_FWRK].Gravity, 1.0f, &gx, &gy);
    if (gx*gx+gy*gy < 0.001f) {
      float angle = rng_between(0, 6283) * 0.001f;//(in radians, between 0 and 2*pi)
      gx += sinf(angle)*elements[PT_FWRK].Gravity*0.5f;
      gy += cosf(angle)*elements[PT_FWRK].Gravity*0.5f;
    }
    gmax = fmaxf(fabsf(gx), fabsf(gy));
    if (eval_move(PT_FWRK, (int)(x-(gx/gmax)+0.5f), (int)(y-(gy/gmax)+0.5f), NULL)) {
      multiplier = 15.0f/sqrtf(gx*gx+gy*gy);
      //Some variation in speed parallel to gravity direction
      randTmp = rng_between(-100, 100);
      gx += gx*randTmp*0.002f;
      gy += gy*randTmp*0.002f;
      //and a bit more variation in speed perpendicular to gravity direction
      randTmp = rng_between(-100, 100);
      gx += -gy*randTmp*0.005f;
      gy += gx*randTmp*0.005f;
      parts[i].life = rng_between(18, 27);
      parts[i].ctype=0;
      p_set_vx(&parts[i], p_vx(&parts[i]) - (gx*multiplier));
      p_set_vy(&parts[i], p_vy(&parts[i]) - (gy*multiplier));
      return 0;
    }
  }
  if (parts[i].life<3&&parts[i].life>0) {
    int r = rng_between(11, 255);
    int g = rng_between(11, 255);
    int b = rng_between(11, 255);
    int n;
    float angle, magnitude;
    unsigned col = (r<<16) | (g<<8) | b;
    for (n=0; n<40; n++) {
      //@ FWRK -> EMBR
      int np = create_part(-3, x, y, PT_EMBR, -1);
      if (np>-1) {
        magnitude = rng_between(40, 99) * 0.05f;
        angle = rng_between(0, 6283) * 0.001f;//(in radians, between 0 and 2*pi)
        p_set_vx(&parts[np], p_vx(&parts[i])*0.5f + cosf(angle)*magnitude);
        p_set_vy(&parts[np], p_vy(&parts[i])*0.5f + sinf(angle)*magnitude);
        parts[np].ctype = col;
        parts[np].tmp = 1;
        parts[np].life = rng_between(70, 109);
        p_set_temp(&parts[np], (float)(rng_between(5750, 6249)));
      }
    }
    pv[y/ACELL][x/ACELL] += 8.0f;
    kill_part(i);
    return 1;
  }
  if (parts[i].life>=45)
    parts[i].life=0;
  return 0;
}

/* ---------------------------------------------------------------- GBMB */
int el_GBMB_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life<=0) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        int r = PMAPAT(x+rx, y+ry);
        if(!r)
          continue;
        if(TYP(r)!=PT_BOMB && TYP(r)!=PT_GBMB &&
           TYP(r)!=PT_CLNE && TYP(r)!=PT_PCLN &&
           TYP(r)!=PT_DMND) {
          parts[i].life=60;
          break;
        }
      }
    }
  }
  if (parts[i].life > 20) {
    gravmass[(y )/ACELL][(x)/ACELL] = 20;
  } else if (parts[i].life >= 1) {
    gravmass[(y )/ACELL][(x)/ACELL] = -80;
  }
  return 0;
}

/* ---------------------------------------------------------------- LIGH */
#define LIGHTING_POWER 0.65f
static void LIGH_create_line_par(int x1, int y1, int x2, int y2, int c, float temp, int life, int tmp, int tmp2, int i);
int el_LIGH_update(UPDATE_FUNC_ARGS) {
  /*
   * tmp2:
   * 0 - bending
   * 1 - bending (particle order deferred)
   * 2 - branching
   * 3 - branching (particle order deferred)
   * 4 - first pixel
   * 5+  normal segment. Starts at 8, counts down and is removed at 5
   *
   * life - power of lightning, influences reaction strength and segment length
   *
   * tmp - angle of lighting, measured in degrees counterclockwise from the positive x direction
   */
  int powderful = (int)(p_temp(&parts[i])*(1+parts[i].life/40)*LIGHTING_POWER);   /* (an int: its divisions are whole, as the original's) */
  //Element_FIRE::update(UPDATE_FUNC_SUBCALL_ARGS);
  if (aheat_enable) {
    hv[y/ACELL][x/ACELL] += powderful/50;
    if (hv[y/ACELL][x/ACELL] > MAX_TEMP)
      hv[y/ACELL][x/ACELL] = MAX_TEMP;
    // If the LIGH was so powerful that it overflowed hv, set to max temp
    else if (hv[y/ACELL][x/ACELL] < 0)
      hv[y/ACELL][x/ACELL] = MAX_TEMP;
  }
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if ((surround_space || elements[rt].Explosive) &&
            (rt!=PT_SPNG || parts[ID(r)].life==0) &&
          elements[rt].Flammable && rng_chance(elements[rt].Flammable + (int)(pv[(y+ry)/ACELL][(x+rx)/ACELL] * 10.0f), 1000)) {
          //@ LIGH + flammable -> LIGH + FIRE
          part_change_type(ID(r),x+rx,y+ry,PT_FIRE);
          p_set_temp(&parts[ID(r)], restrict_flt(elements[PT_FIRE].DefaultProperties.temp + (elements[rt].Flammable/2), MIN_TEMP, MAX_TEMP));
          parts[ID(r)].life = rng_between(180, 259);
          parts[ID(r)].tmp = parts[ID(r)].ctype = 0;
          if (elements[rt].Explosive)
            pv[y/ACELL][x/ACELL] += 0.25f * CFDS;
        }
        switch (rt) {
        case PT_LIGH:
        case PT_TESC:
          continue;
        case PT_CLNE:
        case PT_THDR:
        case PT_DMND:
        case PT_FIRE:
          p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)])+powderful/10, MIN_TEMP, MAX_TEMP));
          continue;
        case PT_DEUT:
        case PT_PLUT:
          p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)])+powderful, MIN_TEMP, MAX_TEMP));
          pv[y/ACELL][x/ACELL] = restrict_flt(pv[y/ACELL][x/ACELL] + powderful/35, MIN_PRESSURE, MAX_PRESSURE);
          if (rng_chance(1, 3)) {
            //@ LIGH + DEUT/PLUT -> LIGH + NEUT
            part_change_type(ID(r),x+rx,y+ry,PT_NEUT);
            parts[ID(r)].life = rng_between(480, 959);
            p_set_vx(&parts[ID(r)], (float)(rng_between(-5, 5)));
            p_set_vy(&parts[ID(r)], (float)(rng_between(-5, 5)));
          }
          break;
        case PT_COAL:
        case PT_BCOL:
          if (parts[ID(r)].life>100)
            parts[ID(r)].life = 99;
          break;
        case PT_STKM:
          if (player.elem!=PT_LIGH)
            parts[ID(r)].life-=powderful/100;
          break;
        case PT_STKM2:
          if (player2.elem!=PT_LIGH)
            parts[ID(r)].life-=powderful/100;
          break;
        case PT_HEAC:
          p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)])+powderful/10, MIN_TEMP, MAX_TEMP));
          if (p_temp(&parts[ID(r)]) > elements[PT_HEAC].HighTemperature) {
            //@ LIGH + HEAC -> LIGH + LAVA(HEAC)
            part_change_type(ID(r), x+rx, y+ry, PT_LAVA);
            parts[ID(r)].ctype = PT_HEAC;
          }
          break;
        default:
          break;
        }
        if ((elements[TYP(r)].Properties&PROP_CONDUCTS) && parts[ID(r)].life==0)
          create_part(ID(r),x+rx,y+ry,PT_SPRK, -1);
        pv[y/ACELL][x/ACELL] = restrict_flt(pv[y/ACELL][x/ACELL] + powderful/400, MIN_PRESSURE, MAX_PRESSURE);
        if (!IsHeatInsulator(&parts[ID(r)])) p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)])+powderful/1.3, MIN_TEMP, MAX_TEMP));
      }
    }
  }
  // Deferred branch or bend; or in removal countdown stage
  if (parts[i].tmp2 == 1 || parts[i].tmp2 == 3 || (parts[i].tmp2 >= 6 && parts[i].tmp2 <= 8)) {
    // Probably set via console, make sure it doesn't stick around forever
    if (parts[i].tmp2 >= 9)
      parts[i].tmp2 = 7;
    else
      parts[i].tmp2--;
    return 0;
  }
  if (parts[i].tmp2 == 5 || parts[i].life <= 1) {
    kill_part(i);
    return 1;
  }
  float angle = (float)((parts[i].tmp + rng_between(-30, 30)) % 360);
  float multipler = (int)(parts[i].life * 1.5) + rng_between(0, parts[i].life);
  int rx = (int)(cosf(angle * NP_PI / 180) * multipler);
  int ry = (int)(-sinf(angle * NP_PI / 180) * multipler);
  LIGH_create_line_par(x, y, x+rx, y+ry, PT_LIGH, p_temp(&parts[i]), parts[i].life, (int)(angle), parts[i].tmp2, i);
  if (parts[i].tmp2 == 2) { // && pNear == -1)
    float angle2 = (float)(((int)angle + rng_between(-100, 100)) % 360);
    rx = (int)(cosf(angle2 * NP_PI / 180) * multipler);
    ry = (int)(-sinf(angle2 * NP_PI / 180) * multipler);
    LIGH_create_line_par(x, y, x+rx, y+ry, PT_LIGH, p_temp(&parts[i]), parts[i].life, (int)(angle2), parts[i].tmp2, i);
  }
  parts[i].tmp2 = 7;
  return 0;
}
static bool LIGH_create_LIGH(int x, int y, int c, float temp, int life, int tmp, int tmp2, bool last, int i) {
  int p = create_part(-1, x, y,c, -1);
  if (p != -1) {
    p_set_temp(&parts[p], (float)(temp));
    parts[p].tmp = tmp;
    if (last) {
      int nextSegmentLife = (int)(life/1.5 - rng_between(0, 1));
      parts[p].life = nextSegmentLife;
      if (nextSegmentLife > 1) {
        // Decide whether to branch or to bend
        bool doBranch = rng_chance(7, 10);
        parts[p].tmp2 = (doBranch ? 2 : 0) + (p > i && tmp2 != 4 ? 1 : 0);
      }
      // Not enough energy to continue
      else {
        parts[p].tmp2 = 7 + (p > i ? 1 : 0);
      }
    } else {
      parts[p].life = life;
      parts[p].tmp2 = 7 + (p > i ? 1 : 0);
    }
  } else if (x >= 0 && x < XRES && y >= 0 && y < YRES) {
    int r = PMAPAT(x, y);
    if (((TYP(r)==PT_VOID || (TYP(r)==PT_PVOD && parts[ID(r)].life >= 10)) && (!parts[ID(r)].ctype || (parts[ID(r)].ctype==c)!=(parts[ID(r)].tmp&1))) || TYP(r)==PT_BHOL || TYP(r)==PT_NBHL) // VOID, PVOD, VACU, and BHOL eat LIGH here
      return true;
  } else return true;
  return false;
}
static void LIGH_create_line_par(int x1, int y1, int x2, int y2, int c, float temp, int life, int tmp, int tmp2, int i) {
  bool reverseXY = abs(y2-y1) > abs(x2-x1), back = false;
  int x, y, dx, dy, Ystep;
  float e = 0.0f, de;
  if (reverseXY) {
    y = x1;
    x1 = y1;
    y1 = y;
    y = x2;
    x2 = y2;
    y2 = y;
  }
  if (x1 > x2)
    back = 1;
  dx = x2 - x1;
  dy = abs(y2 - y1);
  if (dx)
    de = dy/(float)dx;
  else
    de = 0.0f;
  y = y1;
  Ystep = (y1<y2) ? 1 : -1;
  if (!back) {
    for (x = x1; x <= x2; x++) {
      bool ret;
      if (reverseXY)
        ret = LIGH_create_LIGH(y, x, c, temp, life, tmp, tmp2,x==x2, i);
      else
        ret = LIGH_create_LIGH(x, y, c, temp, life, tmp, tmp2,x==x2, i);
      if (ret)
        return;
      e += de;
      if (e >= 0.5f) {
        y += Ystep;
        e -= 1.0f;
      }
    }
  } else {
    for (x = x1; x >= x2; x--) {
      bool ret;
      if (reverseXY)
        ret = LIGH_create_LIGH(y, x, c, temp, life, tmp, tmp2,x==x2, i);
      else
        ret = LIGH_create_LIGH(x, y, c, temp, life, tmp, tmp2,x==x2, i);
      if (ret)
        return;
      e += de;
      if (e <= -0.5f) {
        y += Ystep;
        e += 1.0f;
      }
    }
  }
}
void el_LIGH_create(ELEMENT_CREATE_FUNC_ARGS) {
  float gx, gy, gsize;
  if (v >= 0) {
    if (v > 55)
      v = 55;
    parts[i].life = v;
  } else
    parts[i].life = 30;
  p_set_temp(&parts[i], parts[i].life * 150.0f); // temperature of the lightning shows the power of the lightning
  GetGravityField(x, y, 1.0f, 1.0f, &gx, &gy);
  gsize = gx * gx + gy * gy;
  if (gsize < 0.0016f) {
    float angle = rng_between(0, 6283) * 0.001f; //(in radians, between 0 and 2*pi)
    gsize = sqrtf(gsize);
    // randomness in weak gravity fields (more randomness with weaker fields)
    gx += cosf(angle) * (0.04f - gsize);
    gy += sinf(angle) * (0.04f - gsize);
  }
  parts[i].tmp = ((int)(atan2f(-gy, gx) * 180.0f / NP_PI) + rng_between(-20, 20) + 360) % 360;
  parts[i].tmp2 = 4;
}

/* ---------------------------------------------------------------- THDR */
int el_THDR_update(UPDATE_FUNC_ARGS) {
  bool kill=false;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if ((elements[TYP(r)].Properties&PROP_CONDUCTS) && parts[ID(r)].life==0 && !(rt==PT_WATR||rt==PT_SLTW) && parts[ID(r)].ctype!=PT_SPRK) {
          parts[ID(r)].ctype = parts[ID(r)].type;
          part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
          parts[ID(r)].life = 4;
          kill=true;
        } else if (rt!=PT_CLNE&&rt!=PT_THDR&&rt!=PT_SPRK&&rt!=PT_DMND&&rt!=PT_FIRE) {
          pv[y/ACELL][x/ACELL] = restrict_flt(pv[y/ACELL][x/ACELL] + 100.0f, MIN_PRESSURE, MAX_PRESSURE);
          if (legacy_enable && rng_chance(1, 200)) {
            //@ THDR -> FIRE
            parts[i].life = rng_between(120, 169);
            part_change_type(i,x,y,PT_FIRE);
          } else
            kill=true;
        }
      }
    }
  }
  if (kill) {
    kill_part(i);
    return 1;
  }
  return 0;
}

/* ---------------------------------------------------------------- EMBR */
int el_EMBR_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if ((elements[TYP(r)].Properties & (TYPE_SOLID | TYPE_PART | TYPE_LIQUID)) && !(elements[TYP(r)].Properties & PROP_SPARKSETTLE)) {
          kill_part(i);
          return 1;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- LITH */
/*
tmp2:  carbonation factor
life:  burn timer above 1000, spark cooldown timer otherwise
tmp:   hydrogenation factor
ctype: absorbed energy
For game reasons, baseline LITH has the reactions of both its pure form and
its hydroxide, and also has basic li-ion battery-like behavior.
It absorbs CO2 like its hydroxide form, but can only be converted into GLAS
after having absorbed CO2.
*/
int el_LITH_update(UPDATE_FUNC_ARGS) {
  Particle *self = &parts[i];
#define hydrogenationFactor (self->tmp)
#define burnTimer (self->life)
#define carbonationFactor (self->tmp2)
#define storedEnergy (self->ctype)
  if (storedEnergy < 0) {
    storedEnergy = 0;
  }
  bool charged = false;
  bool discharged = false;
  for (int rx = -2; rx <= 2; ++rx) {
    for (int ry = -2; ry <= 2; ++ry) {
      if (rx || ry) {
        int neighborData = PMAPAT(x + rx, y + ry);
        if (!neighborData) {
          if (burnTimer > 1012 && rng_chance(1, 10)) {
            //@ LITH -> LITH + FIRE
            create_part(-1, x + rx, y + ry, PT_FIRE, -1);
          }
          continue;
        }
        Particle *neighbor = &parts[ID(neighborData)];
        int pavg = parts_avg(i, ID(neighborData), PT_INSL);
        switch (TYP(neighborData)) {
        case PT_SLTW:
        case PT_WTRV:
        case PT_WATR:
        case PT_DSTW:
        case PT_CBNW:
          if (burnTimer > 1016) {
            //@ LITH + SLTW/WATR/DSTW/CBNW -> LITH + WTRV
            part_change_type(ID(neighborData), x + rx, y + ry, PT_WTRV);
            p_set_temp(neighbor, 440.f);
            continue;
          }
          if (hydrogenationFactor + carbonationFactor >= 10) {
            continue;
          }
          //@ LITH + SLTW/WTRV/WATR/DSTW/CBNW -> LITH + H2
          if (p_temp(self) > 440.f) {
            burnTimer = 1024 + (storedEnergy > 24 ? 96 : storedEnergy * 4);
            part_change_type(ID(neighborData), x + rx, y + ry, PT_H2);
            hydrogenationFactor = 10;
          } else {
            p_set_temp(self, restrict_flt(p_temp(self) + 20.365f + storedEnergy * storedEnergy * 1.5f, MIN_TEMP, MAX_TEMP));
            part_change_type(ID(neighborData), x + rx, y + ry, PT_H2);
            hydrogenationFactor += 1;
          }
          break;
        case PT_CO2:
          if (hydrogenationFactor + carbonationFactor >= 10) {
            continue;
          }
          kill_part(ID(neighborData));
          carbonationFactor += 1;
          break;
        case PT_SPRK:
          if (pavg == PT_INSL || pavg == PT_RSSS) {
            break;
          }
          if (hydrogenationFactor + carbonationFactor >= 5) {
            continue; // too impure to do battery things.
          }
          if (neighbor->ctype == PT_PSCN && neighbor->life == 3 && !charged && !burnTimer) {
            charged = true;
          }
          break;
        case PT_NSCN:
          if (pavg == PT_INSL || pavg == PT_RSSS) {
            break;
          }
          if (neighbor->life == 0 && storedEnergy > 0 && !burnTimer) {
            part_change_type(ID(neighborData), x + rx, y + ry, PT_SPRK);
            neighbor->life = 4;
            neighbor->ctype = PT_NSCN;
            discharged = true;
          }
          break;
        case PT_FIRE:
          if (p_temp(self) > 440.f && rng_chance(1, 40) && hydrogenationFactor < 6) {
            burnTimer = 1013;
            hydrogenationFactor += 1;
          }
          break;
        case PT_O2:
          if (burnTimer > 1000 && rng_chance(1, 10)) {
            //@ LITH + O2 -> 2xPLSM
            part_change_type(i, x, y, PT_PLSM);
            part_change_type(ID(neighborData), x + rx, y + ry, PT_PLSM);
            pv[y / ACELL][x / ACELL] += 4.0;
            return 0;
          }            
          break;
        }
      }
    }
  }
  if (charged) {
    storedEnergy += 1;
    burnTimer = 8;
  }
  if (discharged) {
    storedEnergy -= 1;
    burnTimer = 8;
  }
  for (int trade = 0; trade < 9; ++trade) {
    int rx = rng_between(-3, 3);
    int ry = rng_between(-3, 3);
    if (rx || ry) {
      int neighborData = PMAPAT(x + rx, y + ry);
      if (TYP(neighborData) != PT_LITH) {
        continue;
      }
      Particle *neighbor = &parts[ID(neighborData)];
#define neighborStoredEnergy (neighbor->ctype)
      // Transfer overcharge explosion status to nearby LITH
      if (burnTimer < 1000 && storedEnergy > 90 && neighbor->life > 1000)
        burnTimer = 1024;
      if (storedEnergy > neighborStoredEnergy) {
        int transfer = storedEnergy - neighborStoredEnergy;
        transfer -= transfer / 2;
        neighborStoredEnergy += transfer;
        storedEnergy -= transfer;
        break;
      }
#undef neighborStoredEnergy
    }
  }
  // Overcharged - begin explosion
  if (burnTimer < 1000 && storedEnergy >= 100)
    burnTimer = 1024;
  if (burnTimer == 1000) {
    burnTimer = 0;
    part_change_type(i, x, y, PT_LAVA);
    if (carbonationFactor < 3) {
      p_set_temp(self, restrict_flt(500.f + storedEnergy * 10, MIN_TEMP, MAX_TEMP));
      //@ LITH -> LAVA(LITH)
      self->ctype = PT_LITH;
    } else {
      p_set_temp(self, restrict_flt(2000.f + storedEnergy * 10, MIN_TEMP, MAX_TEMP));
      //@ LITH -> LAVA(GLAS)
      self->ctype = PT_GLAS;
    }
  }
  return 0;
#undef hydrogenationFactor
#undef burnTimer
#undef carbonationFactor
#undef storedEnergy
}

