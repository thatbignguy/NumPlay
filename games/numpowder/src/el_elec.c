/* NumPowder: The Powder Toy's electronics and powered elements (their files in elements/), ported: SPRK, NTCT, PTCT,
 * ETRD, BTRY, SWCH, HSWC, PCLN, PBCN, LCRY, DLAY, STOR, PVOD, PUMP, GPMP, WIFI, WIRE, ARAY, CRAY, DRAY, TESC, EMP. */
#include "simpriv.h"

/* ---------------------------------------------------------------- SPRK */
/* the conduction rules from the sender (the element the spark is on) to the receiver (tryConduct) */
static bool SPRK_tryConduct(int i, int r, int receiver, int sender, int x, int y, int rx, int ry) {
  if (receiver==sender && receiver!=PT_INST && receiver!=PT_QRTZ) return true; //Everything conducts to itself, except INST.
  //Sender cases, where elements can have specific outputs
  switch (sender) {
  case PT_INST:
    if (receiver==PT_NSCN)
      return true;
    return false;
  case PT_SWCH:
    if (receiver==PT_PSCN||receiver==PT_NSCN||receiver==PT_WATR||receiver==PT_SLTW||receiver==PT_NTCT||receiver==PT_PTCT||receiver==PT_INWR)
      return false;
    break;
  case PT_ETRD:
    if (receiver==PT_METL||receiver==PT_BMTL||receiver==PT_BRMT||receiver==PT_LRBD||receiver==PT_RBDM||receiver==PT_PSCN||receiver==PT_NSCN)
      return true;
    return false;
  case PT_NTCT:
    if (receiver==PT_PSCN || (receiver==PT_NSCN && p_temp(&parts[i])>373.0f))
      return true;
    return false;
  case PT_PTCT:
    if (receiver==PT_PSCN || (receiver==PT_NSCN && p_temp(&parts[i])<373.0f))
      return true;
    return false;
  case PT_INWR:
    if (receiver==PT_NSCN || receiver==PT_PSCN)
      return true;
    return false;
  default:
    break;
  }
  //Receiving cases, where elements can have specific inputs
  switch (receiver) {
  case PT_QRTZ:
    if ((sender==PT_NSCN||sender==PT_METL||sender==PT_PSCN||sender==PT_QRTZ) && (p_temp(&parts[ID(r)])<173.15||pv[(y+ry)/ACELL][(x+rx)/ACELL]>8))
      return true;
    return false;
  case PT_NTCT:
    if (sender==PT_NSCN || (sender==PT_PSCN&&p_temp(&parts[ID(r)])>373.0f))
      return true;
    return false;
  case PT_PTCT:
    if (sender==PT_NSCN || (sender==PT_PSCN&&p_temp(&parts[ID(r)])<373.0f))
      return true;
    return false;
  case PT_INWR:
    if (sender==PT_NSCN || sender==PT_PSCN)
      return true;
    return false;
  case PT_INST:
    if (sender==PT_PSCN)
      return true;
    return false;
  case PT_NBLE:
    if (!(parts[i].tmp&0x1))
      return true;
    return false;
  case PT_PSCN:
    if (sender!=PT_NSCN)
      return true;
    return false;
  default:
    break;
  }
  return true;
}

int el_SPRK_update(UPDATE_FUNC_ARGS) {
  int ct = parts[i].ctype;
  Element_FIRE_update(UPDATE_FUNC_SUBCALL_ARGS);
  if (parts[i].life<=0) {
    if (ct==PT_WATR||ct==PT_SLTW||ct==PT_PSCN||ct==PT_NSCN||ct==PT_ETRD||ct==PT_INWR)
      p_set_temp(&parts[i], R_TEMP + 273.15f);
    if (ct<=0 || ct>=PT_NUM || !elements[parts[i].ctype].Enabled)
      ct = PT_METL;
    parts[i].ctype = PT_NONE;
    parts[i].life = 4;
    if (ct == PT_WATR)
      parts[i].life = 64;
    else if (ct == PT_SLTW)
      parts[i].life = 54;
    else if (ct == PT_SWCH)
      parts[i].life = 14;
    else if (ct == PT_RSST) { //RSST disappears at the end of its spark cycle
      kill_part(i);
      return 1;
    }
    if (part_change_type(i,x,y,ct))
      return 1;
    return 0;
  }
  //Some functions of SPRK based on ctype (what it is on)
  switch(ct) {
  case PT_SPRK:
    kill_part(i);
    return 1;
  case PT_NTCT:
  case PT_PTCT:
    Element_NTCT_update(UPDATE_FUNC_SUBCALL_ARGS);
    break;
  case PT_ETRD:
    if (parts[i].life==1) {
      int nearp = Element_ETRD_nearestSparkablePart(i);
      if (nearp == -1)
        break;
      int pavg = parts_avg(i, nearp, PT_INSL);
      if (pavg != PT_INSL && pavg != PT_RSSS) {
        //@ ETRD -> ETRD + PLSM
        CreateLine(x, y, (int)(p_x(&parts[nearp])+0.5f), (int)(p_y(&parts[nearp])+0.5f), PT_PLSM);
        parts[i].life = 20;
        part_change_type(i,x,y,ct);
        ct = parts[i].ctype = PT_NONE;
        part_change_type(nearp,(int)(p_x(&parts[nearp])+0.5f),(int)(p_y(&parts[nearp])+0.5f),PT_SPRK);
        parts[nearp].life = 9;
        parts[nearp].ctype = PT_ETRD;
      }
    }
    break;
  case PT_NBLE:
    if (parts[i].life<=1 && !(parts[i].tmp&0x1)) {
      //@ NBLE -> PLSM(NBLE)
      parts[i].life = rng_between(50, 199);
      part_change_type(i,x,y,PT_PLSM);
      parts[i].ctype = PT_NBLE;
      if (p_temp(&parts[i]) > 5273.15)
        parts[i].tmp |= 0x4;
      p_set_temp(&parts[i], 3500);
      pv[y/ACELL][x/ACELL] += 1;
    }
    break;
  case PT_TESC:
    if (parts[i].tmp>300)
      parts[i].tmp=300;
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (r)
            continue;
          if (parts[i].tmp>4 && rng_chance(1, parts[i].tmp*parts[i].tmp/20+6)) {
            //@ TESC -> TESC + LIGH
            int p = create_part(-1, x+rx*2, y+ry*2, PT_LIGH, -1);
            if (p!=-1) {
              parts[p].life = rng_between(0, 2+parts[i].tmp/15) + parts[i].tmp/7;
              if (parts[i].life>60)
                parts[i].life=60;
              p_set_temp(&parts[p], parts[p].life*parts[i].tmp/2.5);
              parts[p].tmp2=1;
              parts[p].tmp=(int)(atan2f(-ry, (float)(rx))*360/NP_PI);
              p_set_temp(&parts[i], restrict_flt(p_temp(&parts[i]) - (parts[i].tmp * 2 + p_temp(&parts[i]) / 5), MIN_TEMP, MAX_TEMP)); // slight self-cooling
              if (fabsf(pv[y/ACELL][x/ACELL])!=0.0f) {
                if (fabsf(pv[y/ACELL][x/ACELL])<=0.5f)
                  pv[y/ACELL][x/ACELL]=0;
                else
                  pv[y/ACELL][x/ACELL]-=(pv[y/ACELL][x/ACELL]>0)?0.5:-0.5;
              }
            }
          }
        }
      }
    }
    break;
  case PT_IRON:
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r)==PT_DSTW || TYP(r)==PT_SLTW || TYP(r)==PT_WATR) {
            int rndstore = rng_gen()%100;
            //@ IRON + DSTW/SLTW/WATR -> IRON + O2/H2
            if (!rndstore)
              part_change_type(ID(r),x+rx,y+ry,PT_O2);
            else if (3 > rndstore)
              part_change_type(ID(r),x+rx,y+ry,PT_H2);
          }
        }
      }
    }
    break;
  case PT_TUNG:
    if(p_temp(&parts[i]) < 3595.0){
      p_set_temp(&parts[i], p_temp(&parts[i]) + (rng_between(-4, 15)));
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
        int receiver = TYP(r);
        int sender = ct;
        int pavg = parts_avg(ID(r), i,PT_INSL);
        //receiver is the element SPRK is trying to conduct to
        //sender is the element the SPRK is on
        //First, some checks usually for (de)activation of elements
        switch (receiver) {
        case PT_SWCH:
          if (pavg!=PT_INSL && pavg!=PT_RSSS && parts[i].life<4) {
            if(sender==PT_PSCN && parts[ID(r)].life<10) {
              parts[ID(r)].life = 10;
            } else if (sender==PT_NSCN) {
              parts[ID(r)].ctype = PT_NONE;
              parts[ID(r)].life = 9;
            }
          }
          break;
        case PT_SPRK:
          if (pavg!=PT_INSL && pavg!=PT_RSSS && parts[i].life<4) {
            if (parts[ID(r)].ctype==PT_SWCH) {
              if (sender==PT_NSCN) {
                part_change_type(ID(r),x+rx,y+ry,PT_SWCH);
                parts[ID(r)].ctype = PT_NONE;
                parts[ID(r)].life = 9;
              }
            } else if(parts[ID(r)].ctype==PT_NTCT||parts[ID(r)].ctype==PT_PTCT)
              if (sender==PT_METL) {
                p_set_temp(&parts[ID(r)], 473.0f);
              }
          }
          continue;
        case PT_PUMP: case PT_GPMP: case PT_HSWC: case PT_PBCN:
          if (parts[i].life<4) { // PROP_PTOGGLE, Maybe? We seem to use 2 different methods for handling actived elements, this one seems better. Yes, use this one for new elements, PCLN is different for compatibility with existing saves
            if (sender==PT_PSCN) parts[ID(r)].life = 10;
            else if (sender==PT_NSCN && parts[ID(r)].life>=10) parts[ID(r)].life = 9;
          }
          continue;
        case PT_LCRY:
          if (abs(rx)<2&&abs(ry)<2 && parts[i].life<4) {
            if (sender==PT_PSCN && parts[ID(r)].tmp == 0) parts[ID(r)].tmp = 2;
            else if (sender==PT_NSCN && parts[ID(r)].tmp == 3) parts[ID(r)].tmp = 1;
          }
          continue;
        case PT_PPIP:
          if (parts[i].life == 3 && pavg!=PT_INSL && pavg!=PT_RSSS) {
            if (sender == PT_NSCN || sender == PT_PSCN || sender == PT_INST)
              Element_PPIP_flood_trigger(x+rx, y+ry, sender);
          }
          continue;
        case PT_NTCT: case PT_PTCT: case PT_INWR:
          if (sender==PT_METL && pavg!=PT_INSL && pavg!=PT_RSSS && parts[i].life<4) {
            p_set_temp(&parts[ID(r)], 473.0f);
            if (receiver==PT_NTCT||receiver==PT_PTCT)
              continue;
          }
          break;
        case PT_EMP:
          if (!parts[ID(r)].life && parts[i].life > 0 && parts[i].life < 4) {
            emp_trigger_count++;
            emp_decor += 3;
            if (emp_decor > 40)
              emp_decor = 40;
            parts[ID(r)].life = 220;
          }
          continue;
        }
        if ((pavg == PT_INSL) || (pavg == PT_RSSS)) continue; //Insulation blocks everything past here
        if (!((elements[receiver].Properties&PROP_CONDUCTS)||receiver==PT_INST||receiver==PT_QRTZ)) continue; //Stop non-conducting receivers, allow INST and QRTZ as special cases
        if (abs(rx)+abs(ry)>=4 &&sender!=PT_SWCH&&receiver!=PT_SWCH) continue; //Only switch conducts really far
        if (!SPRK_tryConduct(i, r, receiver, sender, x, y, rx, ry)) {
          continue;
        }
        //Yay, passed normal conduction rules, check a few last things and change receiver to spark
        if (receiver==PT_WATR||receiver==PT_SLTW) {
          if (parts[ID(r)].life==0 && parts[i].life<3) {
            part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
            if (receiver==PT_WATR) parts[ID(r)].life = 6;
            else parts[ID(r)].life = 5;
            parts[ID(r)].ctype = receiver;
          }
        } else if (receiver==PT_INST) {
          if (parts[ID(r)].life==0 && parts[i].life<4) {
            FloodINST(x+rx,y+ry);//spark the wire
          }
        } else if (receiver==PT_RSST) {
          if (parts[ID(r)].life==0 && parts[i].life<4) {
            part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
            parts[ID(r)].life = 5;
            parts[ID(r)].ctype = receiver;
          }
        } else if (parts[ID(r)].life==0 && parts[i].life<4) {
          parts[ID(r)].life = 4;
          parts[ID(r)].ctype = receiver;
          part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
          if (p_temp(&parts[ID(r)])+10.0f<673.0f&&!legacy_enable&&(receiver==PT_METL||receiver==PT_BMTL||receiver==PT_BRMT||receiver==PT_PSCN||receiver==PT_NSCN||receiver==PT_ETRD||receiver==PT_NBLE||receiver==PT_IRON))
            p_set_temp(&parts[ID(r)], p_temp(&parts[ID(r)])+10.0f);
        } else if (!parts[ID(r)].life && sender==PT_ETRD && parts[i].life==5) { //ETRD is odd and conducts to others only at life 5, this could probably be somewhere else
          part_change_type(i,x,y,sender);
          parts[i].ctype = PT_NONE;
          parts[i].life = 20;
          parts[ID(r)].life = 4;
          parts[ID(r)].ctype = receiver;
          part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- NTCT */
int Element_NTCT_update(UPDATE_FUNC_ARGS) {
  if (p_temp(&parts[i])>295.0f)
    p_set_temp(&parts[i], p_temp(&parts[i]) - (2.5f));
  return 0;
}

/* ---------------------------------------------------------------- PTCT */
int el_PTCT_update(UPDATE_FUNC_ARGS) {
  if (p_temp(&parts[i])>295.0f)
    p_set_temp(&parts[i], p_temp(&parts[i]) - (2.5f));
  return 0;
}

/* ---------------------------------------------------------------- ETRD */
void el_ETRD_changeType(ELEMENT_CHANGETYPE_FUNC_ARGS) {
  if (etrd_count_valid) {
    if (from == PT_ETRD && parts[i].life == 0)
      etrd_life0_count--;
    if (to == PT_ETRD && parts[i].life == 0)
      etrd_life0_count++;
  }
}
/* the places within 12 (|x| + |y|) and their distances, nearest first (ETRD's deltaPos, stably sorted) */
static const struct { int8_t x, y, length; } ETRD_deltaPos[313] = {
  {0, 0, 0}, {-1, -1, 1}, {0, -1, 1}, {1, -1, 1}, {-1, 0, 1}, {1, 0, 1}, {-1, 1, 1}, {0, 1, 1},
  {1, 1, 1}, {-2, -2, 2}, {-1, -2, 2}, {0, -2, 2}, {1, -2, 2}, {2, -2, 2}, {-2, -1, 2}, {2, -1, 2},
  {-2, 0, 2}, {2, 0, 2}, {-2, 1, 2}, {2, 1, 2}, {-2, 2, 2}, {-1, 2, 2}, {0, 2, 2}, {1, 2, 2},
  {2, 2, 2}, {-2, -3, 3}, {-1, -3, 3}, {0, -3, 3}, {1, -3, 3}, {2, -3, 3}, {-3, -2, 3}, {3, -2, 3},
  {-3, -1, 3}, {3, -1, 3}, {-3, 0, 3}, {3, 0, 3}, {-3, 1, 3}, {3, 1, 3}, {-3, 2, 3}, {3, 2, 3},
  {-2, 3, 3}, {-1, 3, 3}, {0, 3, 3}, {1, 3, 3}, {2, 3, 3}, {-2, -4, 4}, {-1, -4, 4}, {0, -4, 4},
  {1, -4, 4}, {2, -4, 4}, {-3, -3, 4}, {3, -3, 4}, {-4, -2, 4}, {4, -2, 4}, {-4, -1, 4}, {4, -1, 4},
  {-4, 0, 4}, {4, 0, 4}, {-4, 1, 4}, {4, 1, 4}, {-4, 2, 4}, {4, 2, 4}, {-3, 3, 4}, {3, 3, 4},
  {-2, 4, 4}, {-1, 4, 4}, {0, 4, 4}, {1, 4, 4}, {2, 4, 4}, {-3, -5, 5}, {-2, -5, 5}, {-1, -5, 5},
  {0, -5, 5}, {1, -5, 5}, {2, -5, 5}, {3, -5, 5}, {-4, -4, 5}, {-3, -4, 5}, {3, -4, 5}, {4, -4, 5},
  {-5, -3, 5}, {-4, -3, 5}, {4, -3, 5}, {5, -3, 5}, {-5, -2, 5}, {5, -2, 5}, {-5, -1, 5}, {5, -1, 5},
  {-5, 0, 5}, {5, 0, 5}, {-5, 1, 5}, {5, 1, 5}, {-5, 2, 5}, {5, 2, 5}, {-5, 3, 5}, {-4, 3, 5},
  {4, 3, 5}, {5, 3, 5}, {-4, 4, 5}, {-3, 4, 5}, {3, 4, 5}, {4, 4, 5}, {-3, 5, 5}, {-2, 5, 5},
  {-1, 5, 5}, {0, 5, 5}, {1, 5, 5}, {2, 5, 5}, {3, 5, 5}, {-3, -6, 6}, {-2, -6, 6}, {-1, -6, 6},
  {0, -6, 6}, {1, -6, 6}, {2, -6, 6}, {3, -6, 6}, {-4, -5, 6}, {4, -5, 6}, {-5, -4, 6}, {5, -4, 6},
  {-6, -3, 6}, {6, -3, 6}, {-6, -2, 6}, {6, -2, 6}, {-6, -1, 6}, {6, -1, 6}, {-6, 0, 6}, {6, 0, 6},
  {-6, 1, 6}, {6, 1, 6}, {-6, 2, 6}, {6, 2, 6}, {-6, 3, 6}, {6, 3, 6}, {-5, 4, 6}, {5, 4, 6},
  {-4, 5, 6}, {4, 5, 6}, {-3, 6, 6}, {-2, 6, 6}, {-1, 6, 6}, {0, 6, 6}, {1, 6, 6}, {2, 6, 6},
  {3, 6, 6}, {-3, -7, 7}, {-2, -7, 7}, {-1, -7, 7}, {0, -7, 7}, {1, -7, 7}, {2, -7, 7}, {3, -7, 7},
  {-5, -6, 7}, {-4, -6, 7}, {4, -6, 7}, {5, -6, 7}, {-6, -5, 7}, {-5, -5, 7}, {5, -5, 7}, {6, -5, 7},
  {-6, -4, 7}, {6, -4, 7}, {-7, -3, 7}, {7, -3, 7}, {-7, -2, 7}, {7, -2, 7}, {-7, -1, 7}, {7, -1, 7},
  {-7, 0, 7}, {7, 0, 7}, {-7, 1, 7}, {7, 1, 7}, {-7, 2, 7}, {7, 2, 7}, {-7, 3, 7}, {7, 3, 7},
  {-6, 4, 7}, {6, 4, 7}, {-6, 5, 7}, {-5, 5, 7}, {5, 5, 7}, {6, 5, 7}, {-5, 6, 7}, {-4, 6, 7},
  {4, 6, 7}, {5, 6, 7}, {-3, 7, 7}, {-2, 7, 7}, {-1, 7, 7}, {0, 7, 7}, {1, 7, 7}, {2, 7, 7},
  {3, 7, 7}, {-4, -8, 8}, {-3, -8, 8}, {-2, -8, 8}, {-1, -8, 8}, {0, -8, 8}, {1, -8, 8}, {2, -8, 8},
  {3, -8, 8}, {4, -8, 8}, {-5, -7, 8}, {-4, -7, 8}, {4, -7, 8}, {5, -7, 8}, {-6, -6, 8}, {6, -6, 8},
  {-7, -5, 8}, {7, -5, 8}, {-8, -4, 8}, {-7, -4, 8}, {7, -4, 8}, {8, -4, 8}, {-8, -3, 8}, {8, -3, 8},
  {-8, -2, 8}, {8, -2, 8}, {-8, -1, 8}, {8, -1, 8}, {-8, 0, 8}, {8, 0, 8}, {-8, 1, 8}, {8, 1, 8},
  {-8, 2, 8}, {8, 2, 8}, {-8, 3, 8}, {8, 3, 8}, {-8, 4, 8}, {-7, 4, 8}, {7, 4, 8}, {8, 4, 8},
  {-7, 5, 8}, {7, 5, 8}, {-6, 6, 8}, {6, 6, 8}, {-5, 7, 8}, {-4, 7, 8}, {4, 7, 8}, {5, 7, 8},
  {-4, 8, 8}, {-3, 8, 8}, {-2, 8, 8}, {-1, 8, 8}, {0, 8, 8}, {1, 8, 8}, {2, 8, 8}, {3, 8, 8},
  {4, 8, 8}, {-3, -9, 9}, {-2, -9, 9}, {-1, -9, 9}, {0, -9, 9}, {1, -9, 9}, {2, -9, 9}, {3, -9, 9},
  {-9, -3, 9}, {9, -3, 9}, {-9, -2, 9}, {9, -2, 9}, {-9, -1, 9}, {9, -1, 9}, {-9, 0, 9}, {9, 0, 9},
  {-9, 1, 9}, {9, 1, 9}, {-9, 2, 9}, {9, 2, 9}, {-9, 3, 9}, {9, 3, 9}, {-3, 9, 9}, {-2, 9, 9},
  {-1, 9, 9}, {0, 9, 9}, {1, 9, 9}, {2, 9, 9}, {3, 9, 9}, {-2, -10, 10}, {-1, -10, 10}, {0, -10, 10},
  {1, -10, 10}, {2, -10, 10}, {-10, -2, 10}, {10, -2, 10}, {-10, -1, 10}, {10, -1, 10}, {-10, 0, 10}, {10, 0, 10},
  {-10, 1, 10}, {10, 1, 10}, {-10, 2, 10}, {10, 2, 10}, {-2, 10, 10}, {-1, 10, 10}, {0, 10, 10}, {1, 10, 10},
  {2, 10, 10}, {-1, -11, 11}, {0, -11, 11}, {1, -11, 11}, {-11, -1, 11}, {11, -1, 11}, {-11, 0, 11}, {11, 0, 11},
  {-11, 1, 11}, {11, 1, 11}, {-1, 11, 11}, {0, 11, 11}, {1, 11, 11}, {0, -12, 12}, {-12, 0, 12}, {12, 0, 12},
  {0, 12, 12},
};
#define ETRD_NDELTA 313
int Element_ETRD_nearestSparkablePart(int targetId) {
  if (!elementCount[PT_ETRD])
    return -1;
  if (etrd_count_valid && etrd_life0_count <= 0)
    return -1;
  if (parts[targetId].tmp2 && parts[targetId].tmp > parts[targetId].tmp2) // Invalid range if max is set
    return -1;
  const int maxDistance = (int)(hypotf((float)XRES, (float)YRES));
  int foundDistance = parts[targetId].tmp2 ? imin(parts[targetId].tmp2, maxDistance) : maxDistance; // tmp2 sets max distance
  int foundI = -1;
  int targetX = (int)(p_x(&parts[targetId])), targetY = (int)(p_y(&parts[targetId]));
  if (etrd_count_valid) {
    // countLife0 doesn't need recalculating, so just focus on finding the nearest particle
    // If the simulation contains lots of particles, check near the target position first since going through all particles will be slow.
    // Threshold = number of positions checked, *2 because it's likely to access memory all over the place (less cache friendly) and there's extra logic needed
    // TODO: probably not optimal if excessive stacking is used
    if (parts_active > ETRD_NDELTA*2) {
      for (int k = 0; k < ETRD_NDELTA; k++) {
        int checkX = targetX + ETRD_deltaPos[k].x, checkY = targetY + ETRD_deltaPos[k].y;
        int checkDistance = ETRD_deltaPos[k].length;
        if (parts[targetId].tmp >= checkDistance) { // tmp sets min distance
          continue;
        }
        if (foundDistance < checkDistance) {
          // deltaPos is sorted in order of ascending length, so foundDistance < checkDistance means all later items are further away.
          break;
        }
        if (InBounds(checkX, checkY) && checkDistance <= foundDistance) {
          int r = PMAPAT(checkX, checkY);
          if (r && TYP(r) == PT_ETRD && !parts[ID(r)].life && ID(r) != targetId && checkDistance < foundDistance) {
            foundDistance = checkDistance;
            foundI = ID(r);
          }
        }
      }
    }
    // If neighbor search didn't find a suitable particle, search all particles
    if (foundI < 0) {
      for (int i = 0; i < parts_active; i++) {
        if (parts[i].type == PT_ETRD && !parts[i].life) {
          int checkX = (int)(p_x(&parts[i]))-targetX, checkY = (int)(p_y(&parts[i]))-targetY;
          int checkDistance = (int)(hypotf((float)checkX, (float)checkY));
          if (checkDistance < foundDistance && checkDistance > parts[targetId].tmp && i != targetId) { // tmp sets min distance
            foundDistance = checkDistance;
            foundI = i;
          }
        }
      }
    }
  } else {
    // Recalculate countLife0, and search for the closest suitable particle
    int countLife0 = 0;
    for (int i = 0; i < parts_active; i++) {
      if (parts[i].type == PT_ETRD && !parts[i].life) {
        countLife0++;
        int checkX = (int)(p_x(&parts[i]))-targetX, checkY = (int)(p_y(&parts[i]))-targetY;
        int checkDistance = (int)(hypotf((float)checkX, (float)checkY));
        if (checkDistance < foundDistance && checkDistance > parts[targetId].tmp && i != targetId) { // tmp sets min distance
          foundDistance = checkDistance;
          foundI = i;
        }
      }
    }
    etrd_life0_count = countLife0;
    etrd_count_valid = true;
  }
  return foundI;
}

/* ---------------------------------------------------------------- BTRY */
int el_BTRY_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if ((rx || ry) && abs(rx)+abs(ry)<4) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        int pavg = parts_avg(i,ID(r),PT_INSL);
        if (pavg!=PT_INSL && pavg!=PT_RSSS) {
          if ((elements[rt].Properties&PROP_CONDUCTS) && !(rt==PT_WATR||rt==PT_SLTW||rt==PT_NTCT||rt==PT_PTCT||rt==PT_INWR) && parts[ID(r)].life==0) {
            parts[ID(r)].life = 4;
            parts[ID(r)].ctype = rt;
            part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SWCH */
static bool SWCH_isRedBRAY(int xc, int yc) {
  return TYP(PMAPAT(xc, yc)) == PT_BRAY && parts[ID(PMAPAT(xc, yc))].tmp == 2;
}
int el_SWCH_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life>0 && parts[i].life!=10)
    parts[i].life--;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int pavg = parts_avg(i,ID(r),PT_INSL);
        if (pavg!=PT_INSL && pavg!=PT_RSSS) {
          int rt = TYP(r);
          if (rt==PT_SWCH) {
            if (parts[i].life>=10&&parts[ID(r)].life<10&&parts[ID(r)].life>0)
              parts[i].life = 9;
            else if (parts[i].life==0&&parts[ID(r)].life>=10) {
              //Set to other particle's life instead of 10, otherwise spark loops form when SWCH is sparked while turning on
              parts[i].life = parts[ID(r)].life;
            }
          } else if (rt==PT_SPRK && parts[i].life==10 && parts[ID(r)].life>0 && parts[ID(r)].ctype!=PT_PSCN && parts[ID(r)].ctype!=PT_NSCN) {
            part_change_type(i,x,y,PT_SPRK);
            parts[i].ctype = PT_SWCH;
            parts[i].life = 4;
          }
        }
      }
    }
  }
  //turn SWCH on/off from two red BRAYS. There must be one either above or below, and one either left or right to work, and it can't come from the side, it must be a diagonal beam
  if (!TYP(PMAPAT(x-1, y-1)) && !TYP(PMAPAT(x+1, y-1)) && (SWCH_isRedBRAY(x, y-1) || SWCH_isRedBRAY(x, y+1)) && (SWCH_isRedBRAY(x+1, y) || SWCH_isRedBRAY(x-1, y))) {
    if (parts[i].life == 10)
      parts[i].life = 9;
    else if (parts[i].life <= 5)
      parts[i].life = 14;
  }
  return 0;
}

/* ---------------------------------------------------------------- HSWC */
int el_HSWC_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life!=10) {
    if (parts[i].life>0)
      parts[i].life--;
  } else {
    bool deserializeTemp = parts[i].tmp == 1;
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            r = PHOTAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r) == PT_HSWC) {
            if (parts[ID(r)].life<10&&parts[ID(r)].life>0)
              parts[i].life = 9;
            else if (parts[ID(r)].life==0)
              parts[ID(r)].life = 10;
          }
          if (deserializeTemp && TYP(r) == PT_FILT) {
            if (rx >= -1 && rx <= 1 && ry >= -1 && ry <= 1) {
              int newTemp = parts[ID(r)].ctype - 0x10000000;
              if (newTemp >= MIN_TEMP && newTemp <= MAX_TEMP)
                p_set_temp(&parts[i], (float)(parts[ID(r)].ctype - 0x10000000));
            }
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- PCLN */
int el_PCLN_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life>0 && parts[i].life!=10)
    parts[i].life--;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SPRK) {
          if (parts[ID(r)].life>0 && parts[ID(r)].life<4) {
            if (parts[ID(r)].ctype==PT_PSCN)
              parts[i].life = 10;
            else if (parts[ID(r)].ctype==PT_NSCN)
              parts[i].life = 9;
          }
        } else if (TYP(r)==PT_PCLN) {
          if (parts[i].life==10&&parts[ID(r)].life<10&&parts[ID(r)].life>0)
            parts[i].life = 9;
          else if (parts[i].life==0&&parts[ID(r)].life==10)
            parts[i].life = 10;
        }
      }
    }
  }
  if (parts[i].ctype<=0 || parts[i].ctype>=PT_NUM || !elements[parts[i].ctype].Enabled) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        int r = PHOTAT(x+rx, y+ry);
        if (!r)
          r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt!=PT_CLNE && rt!=PT_PCLN &&
            rt!=PT_BCLN &&  rt!=PT_SPRK &&
            rt!=PT_NSCN && rt!=PT_PSCN &&
            rt!=PT_STKM && rt!=PT_STKM2 &&
            rt!=PT_PBCN && rt<PT_NUM) {
          parts[i].ctype = rt;
          if (rt==PT_LIFE || rt==PT_LAVA)
            parts[i].tmp = parts[ID(r)].ctype;
        }
      }
    }
  }
  if (parts[i].ctype>0 && parts[i].ctype<PT_NUM && elements[parts[i].ctype].Enabled && parts[i].life==10) {
    if (parts[i].ctype==PT_PHOT) {//create photons a different way
      for (int rx = -1; rx <= 1; rx++) {
        for (int ry = -1; ry <= 1; ry++) {
          if (rx || ry) {
            int r = create_part(-1, x + rx, y + ry, PT_PHOT, -1);
            if (r != -1) {
              p_set_vx(&parts[r], (float)(rx * 3));
              p_set_vy(&parts[r], (float)(ry * 3));
              if (r>i) {
                // Make sure movement doesn't happen until next frame, to avoid gaps in the beams of photons produced
                parts[r].flags |= FLAG_SKIPMOVE;
              }
            }
          }
        }
      }
    } else if (parts[i].ctype==PT_LIFE) { //create life a different way
      for (int rx = -1; rx <= 1; rx++) {
        for (int ry = -1; ry <= 1; ry++) {
          create_part(-1, x+rx, y+ry, PT_LIFE, parts[i].tmp);
        }
      }
    } else if (parts[i].ctype != PT_LIGH || rng_chance(1, 30)) {
      int np = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), TYP(parts[i].ctype), -1);
      if (np>=0) {
        if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && elements[parts[i].tmp].HighTemperatureTransition==PT_LAVA)
          parts[np].ctype = parts[i].tmp;
      }
    }
  }
  return 0;
}
bool Element_PCLN_ctypeDraw(CTYPEDRAW_FUNC_ARGS) {
  if (t == PT_PSCN || t == PT_NSCN || t == PT_SPRK) {
    return false;
  }
  return ctypeDrawVInTmp(CTYPEDRAW_FUNC_SUBCALL_ARGS);
}

/* ---------------------------------------------------------------- PBCN */
#define ADVECTION 0.1f
int el_PBCN_update(UPDATE_FUNC_ARGS) {
  if (!parts[i].tmp2 && pv[y/ACELL][x/ACELL]>4.0f)
    parts[i].tmp2 = rng_between(80, 119);
  if (parts[i].tmp2) {
    p_set_vx(&parts[i], p_vx(&parts[i]) + (ADVECTION*vx[y/ACELL][x/ACELL]));
    p_set_vy(&parts[i], p_vy(&parts[i]) + (ADVECTION*vy[y/ACELL][x/ACELL]));
    parts[i].tmp2--;
    if(!parts[i].tmp2){
      kill_part(i);
      return 1;
    }
  }
  if (parts[i].ctype<=0 || parts[i].ctype>=PT_NUM || !elements[parts[i].ctype].Enabled) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        int r = PHOTAT(x+rx, y+ry);
        if (!r)
          r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt!=PT_CLNE && rt!=PT_PCLN &&
            rt!=PT_BCLN &&  rt!=PT_SPRK &&
            rt!=PT_NSCN && rt!=PT_PSCN &&
            rt!=PT_STKM && rt!=PT_STKM2 &&
            rt!=PT_PBCN && rt<PT_NUM) {
          parts[i].ctype = rt;
          if (rt==PT_LIFE || rt==PT_LAVA)
            parts[i].tmp = parts[ID(r)].ctype;
        }
      }
    }
  }
  if (parts[i].life!=10) {
    if (parts[i].life>0)
      parts[i].life--;
  } else {
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r)==PT_PBCN) {
            if (parts[ID(r)].life<10&&parts[ID(r)].life>0)
              parts[i].life = 9;
            else if (parts[ID(r)].life==0)
              parts[ID(r)].life = 10;
          }
        }
      }
    }
    if (parts[i].ctype>0 && parts[i].ctype<PT_NUM && elements[parts[i].ctype].Enabled) {
      if (parts[i].ctype==PT_PHOT) {//create photons a different way
        for (int rx = -1; rx <= 1; rx++) {
          for (int ry = -1; ry <= 1; ry++) {
            if (rx || ry) {
              int r = create_part(-1, x + rx, y + ry, PT_PHOT, -1);
              if (r != -1) {
                p_set_vx(&parts[r], (float)(rx * 3));
                p_set_vy(&parts[r], (float)(ry * 3));
                if (r>i) {
                  // Make sure movement doesn't happen until next frame, to avoid gaps in the beams of photons produced
                  parts[r].flags |= FLAG_SKIPMOVE;
                }
              }
            }
          }
        }
      } else if (parts[i].ctype==PT_LIFE) { //create life a different way
        for (int rx = -1; rx <= 1; rx++) {
          for (int ry = -1; ry <= 1; ry++) {
            create_part(-1, x+rx, y+ry, PT_LIFE, parts[i].tmp);
          }
        }
      } else if (parts[i].ctype!=PT_LIGH || rng_chance(1, 30)) {
        int np = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), TYP(parts[i].ctype), -1);
        if (np>-1) {
          if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && elements[parts[i].tmp].HighTemperatureTransition==PT_LAVA)
            parts[np].ctype = parts[i].tmp;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- LCRY */
int el_LCRY_update(UPDATE_FUNC_ARGS) {
  int check, setto;
  switch (parts[i].tmp) {
  case 1:
    if(parts[i].life<=0)
      parts[i].tmp = 0;
    else {
      parts[i].life-=2;
      if(parts[i].life < 0)
        parts[i].life = 0;
      parts[i].tmp2 = parts[i].life;
    }
    /* fallthrough */
  case 0:
    check=3;
    setto=1;
    break;
  case 2:
    if(parts[i].life>=10)
      parts[i].tmp = 3;
    else {
      parts[i].life+=2;
      if(parts[i].life > 10)
        parts[i].life = 10;
      parts[i].tmp2 = parts[i].life;
    }
    /* fallthrough */
  case 3:
    check=0;
    setto=2;
    break;
  default:
    parts[i].tmp = 0;
    parts[i].life = 0;
    return 0;
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_LCRY && parts[ID(r)].tmp == check) {
          parts[ID(r)].tmp = setto;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- DLAY */
int el_DLAY_update(UPDATE_FUNC_ARGS) {
  int oldl = parts[i].life;
  if (parts[i].life>0)
    parts[i].life--;
  if (p_temp(&parts[i])<= 1.0f+273.15f)
    p_set_temp(&parts[i], 1.0f+273.15f);
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int pavg = parts_avg(ID(r), i, PT_INSL);
        if (pavg==PT_INSL || pavg==PT_RSSS)
          continue;
        if (TYP(r)==PT_SPRK && parts[i].life==0 && parts[ID(r)].life>0 && parts[ID(r)].life<4 && parts[ID(r)].ctype==PT_PSCN) {
          parts[i].life = (int)(p_temp(&parts[i])-273.15f+0.5f);
        } else if (TYP(r)==PT_DLAY) {
          if (!parts[i].life) {
            if (parts[ID(r)].life) {
              parts[i].life = parts[ID(r)].life;
              if((ID(r))>i) //If the other particle hasn't been life updated
                parts[i].life--;
            }
          } else if (!parts[ID(r)].life) {
            parts[ID(r)].life = parts[i].life;
            if((ID(r))>i) //If the other particle hasn't been life updated
              parts[ID(r)].life++;
          }
        } else if(TYP(r)==PT_NSCN && oldl==1) {
          create_part(-1, x+rx, y+ry, PT_SPRK, -1);
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- STOR */
/* the stored element is in tmp's bits 0 to 7, the filter in ctype's (their other bits keep the stored particle's
 * ctype, the original's tmp4: see tmp4_get) */
int el_STOR_update(UPDATE_FUNC_ARGS) {
  if (!IsElementOrNone(parts[i].tmp & 0xFF))
    parts[i].tmp &= ~0xFF;
  if(parts[i].life && !(parts[i].tmp & 0xFF))
    parts[i].life--;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          r= PHOTAT(x+rx, y+ry);
        if (!r)
          continue;
        if (!(parts[i].tmp & 0xFF) && !parts[i].life && TYP(r)!=PT_STOR && !(elements[TYP(r)].Properties&TYPE_SOLID) && (!(parts[i].ctype & 0xFF) || TYP(r)==(parts[i].ctype & 0xFF))) {
          if (TYP(r) == PT_SOAP)
            Element_SOAP_detach(ID(r));
          parts[i].tmp = (int16_t)((parts[i].tmp & ~0xFF) | parts[ID(r)].type);
          p_set_temp(&parts[i], p_temp(&parts[ID(r)]));
          parts[i].tmp2 = parts[ID(r)].life;
          parts[i].tmp3 = parts[ID(r)].tmp;
          tmp4_set(i, parts[ID(r)].ctype);
          kill_part(ID(r));
        }
        if((parts[i].tmp & 0xFF) && TYP(r)==PT_SPRK && parts[ID(r)].ctype==PT_PSCN && parts[ID(r)].life>0 && parts[ID(r)].life<4) {
          for(int ry1 = 1; ry1 >= -1; ry1--) {
            for(int rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1-rx1+1) { // Oscillate the X starting at 0, 1, -1, 3, -5, etc (Though stop at -1)
              int np = create_part(-1,x+rx1,y+ry1,TYP(parts[i].tmp), -1);
              if (np!=-1) {
                p_set_temp(&parts[np], p_temp(&parts[i]));
                parts[np].life = parts[i].tmp2;
                parts[np].tmp = parts[i].tmp3;
                parts[np].ctype = tmp4_get(i);
                parts[i].tmp &= ~0xFF;
                parts[i].life = 10;
                break;
              }
            }
          }
        }
      }
    }
  }
  return 0;
}
bool el_STOR_ctypeDraw(CTYPEDRAW_FUNC_ARGS) {
  if (elements[t].Properties & TYPE_SOLID) {
    return false;
  }
  int keep = parts[i].ctype & ~0xFF;
  if (!basicCtypeDraw(CTYPEDRAW_FUNC_SUBCALL_ARGS))
    return false;
  parts[i].ctype |= keep;
  return true;
}

/* ---------------------------------------------------------------- PVOD */
int el_PVOD_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life>0 && parts[i].life!=10)
    parts[i].life--;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SPRK) {
          if (parts[ID(r)].life>0 && parts[ID(r)].life<4) {
            if (parts[ID(r)].ctype==PT_PSCN)
              parts[i].life = 10;
            else if (parts[ID(r)].ctype==PT_NSCN)
              parts[i].life = 9;
          }
        } else if (TYP(r)==PT_PVOD) {
          if (parts[i].life==10&&parts[ID(r)].life<10&&parts[ID(r)].life>0)
            parts[i].life = 9;
          else if (parts[i].life==0&&parts[ID(r)].life==10)
            parts[i].life = 10;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- PUMP */
int el_PUMP_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life != 10) {
    if (parts[i].life>0)
      parts[i].life--;
  } else {
    if (p_temp(&parts[i]) >= MAX_PRESSURE+273.15f)
      p_set_temp(&parts[i], MAX_PRESSURE+273.15f);
    if (p_temp(&parts[i]) <= MIN_PRESSURE+273.15f)
      p_set_temp(&parts[i], MIN_PRESSURE+273.15f);
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (parts[i].tmp != 1) {
          if (!(rx && ry))
            pv[(y/ACELL)+ry][(x/ACELL)+rx] += 0.1f*((p_temp(&parts[i])-273.15)-pv[(y/ACELL)+ry][(x/ACELL)+rx]);
        } else {
          int r = PMAPAT(x+rx, y+ry);
          if (TYP(r) == PT_FILT) {
            int newPressure = parts[ID(r)].ctype - 0x10000000;
            if (newPressure >= 0 && newPressure <= MAX_PRESSURE - MIN_PRESSURE) {
              pv[(y + ry) / ACELL][(x + rx) / ACELL] = (float)(newPressure) + MIN_PRESSURE;
            }
          }
        }
      }
    }
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r) == PT_PUMP) {
            if (parts[ID(r)].life < 10 && parts[ID(r)].life > 0)
              parts[i].life = 9;
            else if (parts[ID(r)].life == 0)
              parts[ID(r)].life = 10;
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- GPMP */
int el_GPMP_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life!=10) {
    if (parts[i].life>0)
      parts[i].life--;
  } else {
    if (p_temp(&parts[i])>=256.0f+273.15f)
      p_set_temp(&parts[i], 256.0f+273.15f);
    if (p_temp(&parts[i])<= -256.0f+273.15f)
      p_set_temp(&parts[i], -256.0f+273.15f);
    gravmass[(y )/ACELL][(x)/ACELL] = 0.2f * (p_temp(&parts[i]) - 273.15);
    for (int rx = -2; rx <= 2; rx++) {
      for (int ry = -2; ry <= 2; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r)==PT_GPMP) {
            if (parts[ID(r)].life<10&&parts[ID(r)].life>0)
              parts[i].life = 9;
            else if (parts[ID(r)].life==0)
              parts[ID(r)].life = 10;
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- WIFI */
int el_WIFI_update(UPDATE_FUNC_ARGS) {
  parts[i].tmp = (int)((p_temp(&parts[i])-73.15f)/100+1);
  if (parts[i].tmp>=CHANNELS) parts[i].tmp = CHANNELS-1;
  else if (parts[i].tmp<0) parts[i].tmp = 0;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        // wireless[][0] - whether channel is active on this frame
        // wireless[][1] - whether channel should be active on next frame
        if (wireless[parts[i].tmp][0]) {
          if ((TYP(r)==PT_NSCN||TYP(r)==PT_PSCN||TYP(r)==PT_INWR)&&parts[ID(r)].life==0 && wireless[parts[i].tmp][0]) {
            parts[ID(r)].ctype = TYP(r);
            part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
            parts[ID(r)].life = 4;
          }
        }
        if (TYP(r)==PT_SPRK && parts[ID(r)].ctype!=PT_NSCN && parts[ID(r)].life>=3) {
          wireless[parts[i].tmp][1] = 1;
          ISWIRE = 2;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- WIRE */
int el_WIRE_update(UPDATE_FUNC_ARGS) {
  int count=0;
  /*
    0:  wire
    1:  spark head
    2:  spark tail
    tmp is previous state, ctype is current state
  */
  //parts[i].tmp=parts[i].ctype;
  parts[i].ctype=0;
  if (parts[i].tmp==1) {
    parts[i].ctype=2;
  } else if (parts[i].tmp==2) {
    parts[i].ctype=0;
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SPRK && parts[ID(r)].life==3 && parts[ID(r)].ctype==PT_PSCN) {
          parts[i].ctype=1;
          return 0;
        } else if (TYP(r)==PT_NSCN && parts[i].tmp==1)
          create_part(-1, x+rx, y+ry, PT_SPRK, -1);
        else if (TYP(r)==PT_WIRE && parts[ID(r)].tmp==1 && !parts[i].tmp)
          count++;
      }
    }
  }
  if (count==1 || count==2)
    parts[i].ctype=1;
  return 0;
}

/* ---------------------------------------------------------------- ARAY */
int el_ARAY_update(UPDATE_FUNC_ARGS) {
  int short_bray_life = parts[i].life > 0 ? parts[i].life : 30;
  int long_bray_life = parts[i].life > 0 ? parts[i].life : 1020;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r) == PT_SPRK && parts[ID(r)].life == 3) {
          int destroy = (parts[ID(r)].ctype==PT_PSCN) ? 1 : 0;
          int nostop = (parts[ID(r)].ctype==PT_INST) ? 1 : 0;
          int colored = 0, rt;
          for (int docontinue = 1, nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1; docontinue; nyy+=nyi, nxx+=nxi) {
            if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0))
              break;
            r = PMAPAT(x+nxi+nxx, y+nyi+nyy);
            rt = TYP(r);
            r = ID(r);
            if (!rt) {
              int nr = create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_BRAY, -1);
              if (nr != -1) {
                // if it came from PSCN
                if (destroy) {
                  parts[nr].tmp = 2;
                  parts[nr].life = 2;
                } else {
                  parts[nr].ctype = colored;
                  parts[nr].life = short_bray_life;
                }
                p_set_temp(&parts[nr], p_temp(&parts[i]));
              }
            } else if (!destroy) {
              if (rt == PT_BRAY) {
                // cases for hitting different BRAY modes
                switch(parts[r].tmp) {
                // normal white
                case 0:
                  if (nyy != 0 || nxx !=0) {
                    parts[r].life = long_bray_life; // makes it last a while
                    parts[r].tmp = 1;
                    if (!parts[r].ctype) // and colors it if it isn't already
                      parts[r].ctype = colored;
                  }
                // red bray or any other random tmp mode, stop
                  /* fallthrough */
                case 2:
                default:
                  docontinue = 0;
                  break;
                // long life, reset it
                case 1:
                  parts[r].life = long_bray_life;
                  //docontinue = 1;
                  break;
                }
              }
              // get color if passed through FILT
              else if (rt == PT_FILT) {
                if (parts[r].tmp != 6) {
                  colored = Element_FILT_interactWavelengths(&parts[r], colored);
                  if (!colored)
                    break;
                }
                /* (decoration isn't kept: never black) */
                parts[r].life = 4;
              } else if (rt == PT_STOR) {
                if (parts[r].tmp & 0xFF) {
                  //Cause STOR to release
                  for (int ry1 = 1; ry1 >= -1; ry1--) {
                    for (int rx1 = 0; rx1 >= -1 && rx1 <= 1; rx1 = -rx1 - rx1 + 1) {
                      int np = create_part(-1, x + nxi + nxx + rx1, y + nyi + nyy + ry1, TYP(parts[r].tmp), -1);
                      if (np != -1) {
                        p_set_temp(&parts[np], p_temp(&parts[r]));
                        parts[np].life = parts[r].tmp2;
                        parts[np].tmp = parts[r].tmp3;
                        parts[np].ctype = tmp4_get(r);
                        parts[r].tmp &= ~0xFF;
                        parts[r].life = 10;
                        break;
                      }
                    }
                  }
                } else {
                  parts[r].life = 10;
                }
              // this if prevents BRAY from stopping on certain materials
              } else if (rt != PT_INWR && (rt != PT_SPRK || parts[r].ctype != PT_INWR) && rt != PT_ARAY && rt != PT_WIFI && !(rt == PT_SWCH && parts[r].life >= 10)) {
                if (nyy!=0 || nxx!=0)
                  create_part(-1, x+nxi+nxx, y+nyi+nyy, PT_SPRK, -1);
                if (!(nostop && parts[r].type==PT_SPRK && parts[r].ctype >= 0 && parts[r].ctype < PT_NUM && (elements[parts[r].ctype].Properties&PROP_CONDUCTS)))
                  docontinue = 0;
                else
                  docontinue = 1;
              }
            } else if (destroy) {
              if (rt == PT_BRAY) {
                parts[r].tmp = 2;
                parts[r].life = 1;
                docontinue = 1;
              //this if prevents red BRAY from stopping on certain materials
              } else if (rt==PT_STOR || rt==PT_INWR || (rt==PT_SPRK && parts[r].ctype==PT_INWR) || rt==PT_ARAY || rt==PT_WIFI || rt==PT_FILT || (rt==PT_SWCH && parts[r].life>=10)) {
                if (rt == PT_STOR) {
                  parts[r].tmp &= ~0xFF;
                  parts[r].life = 0;
                } else if (rt == PT_FILT) {
                  /* (decoration isn't kept: never black) */
                  parts[r].life = 2;
                }
                docontinue = 1;
              } else {
                docontinue = 0;
              }
            }
          }
        }
        //parts[i].life = 4;
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- CRAY */
int el_CRAY_update(UPDATE_FUNC_ARGS) {
  int nxx, nyy, docontinue, nxi, nyi;
  // set ctype to things that touch it if it doesn't have one already
  if (parts[i].ctype<=0 || !elements[TYP(parts[i].ctype)].Enabled) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        int r = PHOTAT(x+rx, y+ry);
        if (!r)
          r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)!=PT_CRAY && TYP(r)!=PT_PSCN && TYP(r)!=PT_INST && TYP(r)!=PT_METL && TYP(r)!=PT_SPRK && TYP(r)<PT_NUM) {
          parts[i].ctype = TYP(r);
          p_set_temp(&parts[i], p_temp(&parts[ID(r)]));
        }
      }
    }
  } else {
    for (int rx =-1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r)==PT_SPRK && parts[ID(r)].life==3) { //spark found, start creating
            bool destroy = parts[ID(r)].ctype==PT_PSCN;
            bool nostop = parts[ID(r)].ctype==PT_INST;
            bool createSpark = (parts[ID(r)].ctype==PT_INWR);
            int partsRemaining = 255;
            if (parts[i].tmp) //how far it shoots
              partsRemaining = parts[i].tmp;
            int spacesRemaining = parts[i].tmp2;
            for (docontinue = 1, nxi = rx*-1, nyi = ry*-1, nxx = spacesRemaining*nxi, nyy = spacesRemaining*nyi; docontinue; nyy+=nyi, nxx+=nxi) {
              if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0)) {
                break;
              }
              r = PMAPAT(x+nxi+nxx, y+nyi+nyy);
              if (!IsWallBlocking(x+nxi+nxx, y+nyi+nyy, TYP(parts[i].ctype)) && (!PMAPAT(x+nxi+nxx, y+nyi+nyy) || createSpark)) { // create, also set color if it has passed through FILT
                int nr = create_part(-1, x+nxi+nxx, y+nyi+nyy, TYP(parts[i].ctype), ID(parts[i].ctype));
                if (nr!=-1) {
                  p_set_temp(&parts[nr], p_temp(&parts[i]));
                  if (parts[i].life>0)
                    parts[nr].life = parts[i].life;
                  if(!--partsRemaining)
                    docontinue = 0;
                }
              } else if (TYP(r)==PT_FILT) { // get color if passed through FILT
                /* (the decoration colour it would give what it makes isn't kept) */
                parts[ID(r)].life = 4;
              } else if (TYP(r) == PT_CRAY || nostop) {
                docontinue = 1;
              } else if(destroy && r && (TYP(r) != PT_DMND)) {
                kill_part(ID(r));
                if(!--partsRemaining)
                  docontinue = 0;
              } else
                docontinue = 0;
              if(!partsRemaining)
                docontinue = 0;
            }
          }
        }
      }
    }
  }
  return 0;
}
bool el_CRAY_ctypeDraw(CTYPEDRAW_FUNC_ARGS) {
  if (!ctypeDrawVInCtype(CTYPEDRAW_FUNC_SUBCALL_ARGS)) {
    return false;
  }
  if (t == PT_LIGH) {
    parts[i].ctype |= PMAPID(30);
  }
  p_set_temp(&parts[i], elements[t].DefaultProperties.temp);
  return true;
}

/* ---------------------------------------------------------------- DRAY */
int el_DRAY_update(UPDATE_FUNC_ARGS) {
  int ctype = TYP(parts[i].ctype), ctypeExtra = ID(parts[i].ctype), copyLength = parts[i].tmp, copySpaces = parts[i].tmp2;
  if (copySpaces < 0)
    copySpaces = parts[i].tmp2 = 0;
  if (copyLength < 0)
    copyLength = parts[i].tmp = 0;
  else if (copyLength > 0)
    copySpaces++; //strange hack
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (TYP(r) == PT_SPRK && parts[ID(r)].life == 3) { //spark found, start creating
          bool overwrite = parts[ID(r)].ctype == PT_PSCN;
          int partsRemaining = copyLength, xCopyTo = -1, yCopyTo = -1; //positions where the line will start being copied at
          int localCopyLength = copyLength;
          if (parts[ID(r)].ctype == PT_INWR && rx && ry) // INWR doesn't spark from diagonals
            continue;
          //figure out where the copying will start/end
          bool foundParticle = false;
          bool isEnergy = false;
          for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; ; xCurrent+=xStep, yCurrent+=yStep) {
            // Out of bounds, stop looking and don't copy anything
            if (!InBounds(xCurrent, yCurrent))
              break;
            int rr;
            // haven't found a particle yet, keep looking for one
            // the first particle it sees decides whether it will copy energy particles or not
            if (!foundParticle) {
              rr = PMAPAT(xCurrent, yCurrent);
              if (!rr) {
                rr = PHOTAT(xCurrent, yCurrent);
                if (rr)
                  foundParticle = isEnergy = true;
              } else
                foundParticle = true;
            }
            // now that it knows what kind of particle it is copying, do some extra stuff here so we can determine when to stop
            if ((ctype && elements[ctype].Properties&TYPE_ENERGY) || isEnergy)
              rr = PHOTAT(xCurrent, yCurrent);
            else
              rr = PMAPAT(xCurrent, yCurrent);
            // Checks for when to stop:
            //  1: if .tmp isn't set, and the element in this spot is the ctype, then stop
            //  2: if .tmp is set, stop when the length limit reaches 0
            if ((!localCopyLength && TYP(rr) == ctype && (ctype != PT_LIFE || parts[ID(rr)].ctype == ctypeExtra))
                || !--partsRemaining) {
              localCopyLength -= partsRemaining;
              xCopyTo = xCurrent + xStep*copySpaces;
              yCopyTo = yCurrent + yStep*copySpaces;
              break;
            }
          }
          // now, actually copy the particles
          partsRemaining = localCopyLength + 1;
          int type, p;
          for (int xStep = rx*-1, yStep = ry*-1, xCurrent = x+xStep, yCurrent = y+yStep; InBounds(xCopyTo, yCopyTo) && --partsRemaining; xCurrent+=xStep, yCurrent+=yStep, xCopyTo+=xStep, yCopyTo+=yStep) {
            // get particle to copy
            if (isEnergy)
              type = TYP(PHOTAT(xCurrent, yCurrent));
            else
              type = TYP(PMAPAT(xCurrent, yCurrent));
            // if sparked by PSCN, overwrite whatever is in the target location, instead of just ignoring it
            if (overwrite) {
              if (isEnergy) {
                if (PHOTAT(xCopyTo, yCopyTo))
                  kill_part(ID(PHOTAT(xCopyTo, yCopyTo)));
              } else {
                if (PMAPAT(xCopyTo, yCopyTo))
                  kill_part(ID(PMAPAT(xCopyTo, yCopyTo)));
              }
            }
            if (type == PT_SPRK) // spark hack
              p = create_part(-1, xCopyTo, yCopyTo, PT_METL, -1);
            else if (type == PT_STKM || type == PT_STKM2)
              continue; // do not try to copy these non-copyable particles
            else if (type)
              p = create_part(-1, xCopyTo, yCopyTo, type, -1);
            else
              continue;
            // if new particle was created successfully
            if (p >= 0) {
              if (type == PT_SPRK) // spark hack
                part_change_type(p, xCopyTo, yCopyTo, PT_SPRK);
              int src = isEnergy ? ID(PHOTAT(xCurrent, yCurrent)) : ID(PMAPAT(xCurrent, yCurrent));
              if (isEnergy || type != PT_FIGH) {
                parts[p] = parts[src];
                if (type == PT_PIPE || type == PT_PPIP)
                  tmp4_set(p, tmp4_get(src));   /* (the part of tmp4 kept by particle id) */
              } else {
                // FIGH needs special rules
                const Particle *other = &parts[src];
                // need to keep .tmp consistent: it points to fighter metadata
                int old_tmp = parts[p].tmp;
                parts[p] = *other;
                parts[p].tmp = (int16_t)old_tmp;
                // Update the new fighter's metadata
                // Do not attempt to copy kinematics of legs - overwritten next frame anyway
                if (other->tmp >= 0 && other->tmp < MAX_FIGHTERS) {
                  const playerst *source_pst = &fighters[other->tmp];
                  playerst *this_pst = &fighters[old_tmp];
                  this_pst->rocketBoots = source_pst->rocketBoots;
                  this_pst->fan = source_pst->fan;
                  this_pst->elem = source_pst->elem;
                }
              }
              p_set_x(&parts[p], (float)(xCopyTo));
              p_set_y(&parts[p], (float)(yCopyTo));
            }
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- TESC */
void el_TESC_create(ELEMENT_CREATE_FUNC_ARGS) {
  if (v >= 0) {
    parts[i].tmp = v;
    if (parts[i].tmp > 300)
      parts[i].tmp = 300;
  }
}

/* ---------------------------------------------------------------- EMP */
/* Probability.cpp: P(X >= 1) for X ~ binomial(n, p), and binomial numbers up to a small maximum */
static float binomial_gte1(int n, float p) { return 1.0f - powf(1.0f - p, (float)n); }
typedef struct { float stepSize; unsigned maxK; float cdf[10]; } DeltaTempGenerator;
static void DeltaTempGenerator_init(DeltaTempGenerator *g, unsigned n, float p, float tempStep) {
  g->stepSize = tempStep;
  /* hardcoded limit of 10, to avoid massive lag if someone adds a few zeroes to MAX_TEMP */
  g->maxK = (MAX_TEMP / tempStep < 10) ? ((unsigned)(MAX_TEMP / tempStep) + 1) : 10;
  float pTerm = powf(1.0f - p, (float)n);   /* the p^k * (1-p)^(n-k) term */
  unsigned coeffN = 1, coeffD = 1;          /* n!/(k!(n-k)!) as coeffN / coeffD */
  float sum = 0;
  for (unsigned k = 0; k < g->maxK; k++) {
    sum += pTerm * ((float)coeffN / coeffD);
    g->cdf[k] = sum;
    pTerm *= p / (1.0f - p);
    coeffN *= n - k;
    coeffD *= k + 1;
  }
}
static void DeltaTempGenerator_apply(const DeltaTempGenerator *g, Particle *p) {
  float randFloat = rng_uniform01();
  unsigned k = 0;
  while (k < g->maxK && !(randFloat < g->cdf[k])) k++;
  p_set_temp(p, restrict_flt(p_temp(p) + k * g->stepSize, MIN_TEMP, MAX_TEMP));
}
void Element_EMP_Trigger(int triggerCount) {
  /* Known differences from original one-particle-at-a-time version:
   * - SPRK that disappears during a frame (such as SPRK with life==0 on that frame) will not cause destruction around it.
   * - SPRK neighbour effects are calculated assuming the SPRK exists and causes destruction around it for the entire frame (so was not turned into BREL/NTCT partway through). This means mass EMP will be more destructive.
   * - The chance of a METL particle near sparked semiconductor turning into BRMT within 1 frame is different if triggerCount>2. See comment for prob_breakMETLMore.
   * - Probability of centre isElec particle breaking is slightly different (1/48 instead of 1-(1-1/80)*(1-1/120) = just under 1/48).
   */
  float prob_changeCenter = binomial_gte1(triggerCount, 1.0f/48);
  DeltaTempGenerator temp_center, temp_metal, temp_SWCH;
  DeltaTempGenerator_init(&temp_center, (unsigned)triggerCount, 1.0f/100, 3000.0f);
  float prob_breakMETL = binomial_gte1(triggerCount, 1.0f/300);
  float prob_breakBMTL = binomial_gte1(triggerCount, 1.0f/160);
  DeltaTempGenerator_init(&temp_metal, (unsigned)triggerCount, 1.0f/280, 3000.0f);
  /* Probability of breaking from BMTL to BRMT, given that the particle has just broken from METL to BMTL. There is no mathematical reasoning for the numbers used, other than:
   * - larger triggerCount should make this more likely, so it should depend on triggerCount instead of being a constant probability
   * - triggerCount==1 should make this a chance of 0 (matching previous behaviour)
   * - triggerCount==2 should make this a chance of 1/160 (matching previous behaviour)
   */
  // TODO: work out in a more mathematical way what this should be?
  float prob_breakMETLMore = binomial_gte1(triggerCount/2, 1.0f/160);
  float prob_randWIFI = binomial_gte1(triggerCount, 1.0f/8);
  float prob_breakWIFI = binomial_gte1(triggerCount, 1.0f/16);
  float prob_breakSWCH = binomial_gte1(triggerCount, 1.0f/100);
  DeltaTempGenerator_init(&temp_SWCH, (unsigned)triggerCount, 1.0f/100, 2000.0f);
  float prob_breakARAY = binomial_gte1(triggerCount, 1.0f/60);
  float prob_randDLAY = binomial_gte1(triggerCount, 1.0f/70);
  for (int r = 0; r < parts_active; r++) {
    int t = parts[r].type;
    int rx = (int)(p_x(&parts[r]));
    int ry = (int)(p_y(&parts[r]));
    if (t==PT_SPRK || (t==PT_SWCH && parts[r].life!=0 && parts[r].life!=10) || (t==PT_WIRE && parts[r].ctype>0)) {
      bool is_elec = false;
      if (parts[r].ctype==PT_PSCN || parts[r].ctype==PT_NSCN || parts[r].ctype==PT_PTCT ||
          parts[r].ctype==PT_NTCT || parts[r].ctype==PT_INST || parts[r].ctype==PT_SWCH || t==PT_WIRE || t==PT_SWCH) {
        is_elec = true;
        DeltaTempGenerator_apply(&temp_center, &parts[r]);
        //@ EMP + PSCN/NSCN/PTCT/NTCT/INST/SWCH/WIRE -> EMP + BREC/NTCT
        if (rng_uniform01() < prob_changeCenter) {
          if (rng_chance(2, 5))
            part_change_type(r, rx, ry, PT_BREC);
          else
            part_change_type(r, rx, ry, PT_NTCT);
        }
      }
      for (int nx =-2; nx <= 2; nx++)
        for (int ny =-2; ny <= 2; ny++)
          if (rx+nx>=0 && ry+ny>=0 && rx+nx<XRES && ry+ny<YRES && (rx || ry)) {
            int n = PMAPAT(rx+nx, ry+ny);
            if (!n)
              continue;
            int ntype = TYP(n);
            n = ID(n);
            //Some elements should only be affected by wire/swch, or by a spark on inst/semiconductor
            //So not affected by spark on metl, watr etc
            if (is_elec) {
              switch (ntype) {
              case PT_METL:
                DeltaTempGenerator_apply(&temp_metal, &parts[n]);
                if (rng_uniform01() < prob_breakMETL) {
                  //@ EMP + METL -> EMP + BMTL/BRMT
                  part_change_type(n, rx+nx, ry+ny, PT_BMTL);
                  if (rng_uniform01() < prob_breakMETLMore) {
                    part_change_type(n, rx+nx, ry+ny, PT_BRMT);
                    p_set_temp(&parts[n], restrict_flt(p_temp(&parts[n])+1000.0f, MIN_TEMP, MAX_TEMP));
                  }
                }
                break;
              case PT_BMTL:
                //@ EMP + BMTL -> EMP + BRMT
                DeltaTempGenerator_apply(&temp_metal, &parts[n]);
                if (rng_uniform01() < prob_breakBMTL) {
                  part_change_type(n, rx+nx, ry+ny, PT_BRMT);
                  p_set_temp(&parts[n], restrict_flt(p_temp(&parts[n])+1000.0f, MIN_TEMP, MAX_TEMP));
                }
                break;
              case PT_WIFI:
                if (rng_uniform01() < prob_randWIFI) {
                  // Randomize channel
                  p_set_temp(&parts[n], (float)(rng_between(0, (int)(MAX_TEMP)-1)));
                }
                //@ EMP + WIFI -> EMP + BREC
                if (rng_uniform01() < prob_breakWIFI) {
                  create_part(n, rx+nx, ry+ny, PT_BREC, -1);
                  p_set_temp(&parts[n], restrict_flt(p_temp(&parts[n])+1000.0f, MIN_TEMP, MAX_TEMP));
                }
                continue;
              default:
                break;
              }
            }
            switch (ntype) {
            case PT_SWCH:
              //@ EMP + SWCH -> EMP + BREC
              if (rng_uniform01() < prob_breakSWCH)
                part_change_type(n, rx+nx, ry+ny, PT_BREC);
              DeltaTempGenerator_apply(&temp_SWCH, &parts[n]);
              break;
            case PT_ARAY:
              //@ EMP + ARAY -> EMP + BREC
              if (rng_uniform01() < prob_breakARAY) {
                create_part(n, rx+nx, ry+ny, PT_BREC, -1);
                p_set_temp(&parts[n], restrict_flt(p_temp(&parts[n])+1000.0f, MIN_TEMP, MAX_TEMP));
              }
              break;
            case PT_DLAY:
              if (rng_uniform01() < prob_randDLAY) {
                // Randomize delay
                p_set_temp(&parts[n], rng_between(0, 255) + 273.15f);
              }
              break;
            default:
              break;
            }
          }
    }
  }
}

