/* NumPowder: The Powder Toy's solids (their files in elements/), ported: GOO, ICEI, WOOD, PLNT, BMTL, GLAS, GOLD,
 * IRON, TTAN, TUNG, PTNM, CRMC, HEAC, ROCK, VINE, SPNG, QRTZ (and PQRT), VIBR (and BVBR), RSSS, SHLD1 to SHLD4,
 * SPAWN, SPAWN2, FILT. */
#include "simpriv.h"

/* ---------------------------------------------------------------- GOO */
#define ADVECTION 0.1f
int el_GOO_update(UPDATE_FUNC_ARGS) {
  if (!parts[i].life && pv[y/ACELL][x/ACELL]>1.0f)
    parts[i].life = rng_between(300, 379);
  if (parts[i].life) {
    p_set_vx(&parts[i], p_vx(&parts[i]) + (ADVECTION*vx[y/ACELL][x/ACELL]));
    p_set_vy(&parts[i], p_vy(&parts[i]) + (ADVECTION*vy[y/ACELL][x/ACELL]));
  }
  return 0;
}

/* ---------------------------------------------------------------- ICEI */
int el_ICEI_update(UPDATE_FUNC_ARGS) {
  if (parts[i].ctype==PT_FRZW) { //get colder if it is from FRZW
    p_set_temp(&parts[i], restrict_flt(p_temp(&parts[i])-1.0f, MIN_TEMP, MAX_TEMP));
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SALT || TYP(r)==PT_SLTW) {
          if (p_temp(&parts[i]) > elements[PT_SLTW].LowTemperature && rng_chance(1, 200)) {
            //@ ICEI + SALT/SLTW -> 2xSLTW
            part_change_type(i,x,y,PT_SLTW);
            part_change_type(ID(r),x+rx,y+ry,PT_SLTW);
            return 0;
          }
        } else if ((TYP(r)==PT_FRZZ) && rng_chance(1, 200)) {
          //@ ICEI + FRZZ -> ICEI + ICEI(FRZW)
          part_change_type(ID(r),x+rx,y+ry,PT_ICEI);
          parts[ID(r)].ctype = PT_FRZW;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- WOOD */
int el_WOOD_update(UPDATE_FUNC_ARGS) {
  if (p_temp(&parts[i]) > 450 && p_temp(&parts[i]) > parts[i].tmp)
    parts[i].tmp = (int)p_temp(&parts[i]);
  if (p_temp(&parts[i]) > 773.0f && pv[y/ACELL][x/ACELL] <= -10.0f) {
    float temp = p_temp(&parts[i]);
    //@ WOOD -> BCOL
    create_part(i, x, y, PT_BCOL, -1);
    p_set_temp(&parts[i], temp);
  }
  return 0;
}

/* ---------------------------------------------------------------- PLNT */
// Fields used for tree growth (see also PLNT.h):
// ctype: tree or usual PLNT (1 bit), phase (2 bits), direction (3 bits), colour (6 bits), water (8 bits)
// life: used as a counter in growth
// tmp-tmp4: branching bitmask (5 bits), new phases (5x2 bits)
int el_PLNT_update(UPDATE_FUNC_ARGS) {
  // Select tree growth code or usual PLNT code
  if (parts[i].ctype & 1) {
    float temp = p_temp(&parts[i]);
    // Hot and cold temperature kills tree growth
    if (temp > 373.15f || temp < 223.15f) {
      parts[i].ctype = 0;
      parts[i].tmp = 0;
      parts[i].tmp2 = 0;
      parts[i].tmp3 = 0;
      tmp4_set(i, 0);
      return 0;
    }
    // No growth happens at bad temperatures
    if (temp > 343.15f || temp < 278.15f)
      return 0;
    // Slow things down
    if (rng_chance(9, 10))
      return 0;
    // Current phase, direction and life
    int phase = (parts[i].ctype>>PLNT_PHASE) & 0x3;
    int dir = (parts[i].ctype>>PLNT_DIR) & 0x7;
    int life = (parts[i].ctype>>PLNT_LIFE) & 0xFF;
    int prog = 0;
    // Select a program for the current phase
    switch (phase) {
      case 0: prog = parts[i].tmp; break;
      case 1: prog = parts[i].tmp2; break;
      case 2: prog = parts[i].tmp3; break;
      case 3: prog = tmp4_get(i); break;
    }
    int si = -1; // Used for create_part calls
    bool stopped = false; // Did the cell stop growing?
    int down = Element_PLNT_detectDown(x, y); // Down gravity direction (8 means no direction)
    // Do we continue growing?
    if (life > 0) {
      // Do we grow forward or branch?
      if (!parts[i].life) {
        // Branching
        int bmask = prog&0x1F;
        int phases = prog>>5;
        // Can we branch or it's a dead end?
        if (bmask) {
          // Loop through all possible branching directions
          for (int phi = 0; phi < 5; phi++) {
            if ((bmask>>phi)&0x1) {
              int ndir = (dir + phi + 6) % 8; // phi = 0 is -90 deg, phi = 1 is -45 deg etc
              int nphase = phases&0x3;
              int nlife = (2*life)/3;
              // Discourage branches which go down
              if (down < 8) {
                // Down
                if(ndir == down)
                  nlife = life/3;
                // Down-left and down-right
                if(ndir == ((down+1)%8) || ndir == ((down+7)%8))
                  nlife = life/3;
                // Sideways
                if(life > 4 && (ndir == ((down+2)%8) || ndir == ((down+6)%8)))
                  nlife = life/2;
              }
              si = create_part(-1, x+dir3x3[ndir].X, y+dir3x3[ndir].Y, PT_PLNT, -1);
              if (si >= 0) {
                parts[si].ctype = ((nlife & 0xFF) << PLNT_LIFE) | (parts[i].ctype & (0x3f << PLNT_COLOUR))
                    | ((ndir & 7) << PLNT_DIR) | ((nphase & 3) << PLNT_PHASE) | 0x1;
                // Reduce the length of diagonal branches by 1/sqrtf(2) ~= 0.7
                parts[si].life = (ndir%2 == 1) ? 7*nlife : 10*nlife;
                parts[si].tmp = parts[i].tmp;
                parts[si].tmp2 = parts[i].tmp2;
                parts[si].tmp3 = parts[i].tmp3;
                tmp4_set(si, tmp4_get(i));
              }
            }
            phases = phases>>2;
          }
        } else
          stopped = true;
      } else {
        // Grow forward
        si = create_part(-1, x+dir3x3[dir].X, y+dir3x3[dir].Y, PT_PLNT, -1);
        if (si >= 0) {
          parts[si].ctype = ((life & 0xFF) << PLNT_LIFE) | (parts[i].ctype & (0x3f << PLNT_COLOUR))
              | ((dir & 7) << PLNT_DIR) | ((phase & 3) << PLNT_PHASE) | 0x1;
          parts[si].life = parts[i].life;
          parts[si].tmp = parts[i].tmp;
          parts[si].tmp2 = parts[i].tmp2;
          parts[si].tmp3 = parts[i].tmp3;
          tmp4_set(si, tmp4_get(i));
        }
      }
      if (!stopped) {
        // Create a stem
        if (phase || life < 12) {
          //@ PLNT -> WOOD
          create_part(i, x, y, PT_WOOD, -1);
          parts[i].ctype = 0;
          parts[i].life = 0;
          parts[i].tmp = 0;
          parts[i].tmp2 = 0;
          parts[i].tmp3 = 0;
          tmp4_set(i, 0);
        } else {
          // Thick stem
          //@ PLNT -> GOO
          create_part(i, x, y, PT_GOO, -1);
          parts[i].ctype = 0;
          parts[i].life = 0;
          parts[i].tmp = 0;
          parts[i].tmp2 = 0;
          parts[i].tmp3 = 0;
          tmp4_set(i, 0);
          int left_dir = (dir+6)%8;
          int right_dir = (dir+2)%8;
          // Correction for diagonal directions
          if (dir%2) {
            left_dir = (dir+5)%8;
            right_dir = (dir+3)%8;
          }
          create_part(-1, x+dir3x3[left_dir].X, y+dir3x3[left_dir].Y, PT_WOOD, -1);
          create_part(-1, x+dir3x3[right_dir].X, y+dir3x3[right_dir].Y, PT_WOOD, -1);
        }
      }
    } else
      stopped = true;
    // Stop growing
    if (stopped) {
      // Shoot out a seed
      if (rng_chance(1, 10)) {
        //@ PLNT -> PLNT + SEED
        si = create_part(-1, x+2*dir3x3[dir].X, y+2*dir3x3[dir].Y, PT_SEED, -1);
        if (si >= 0) {
          p_set_vx(&parts[si], (float)(dir3x3[dir].X));
          p_set_vy(&parts[si], (float)(dir3x3[dir].Y));
          // Inherit genome
          parts[si].ctype = parts[i].ctype & (0x3f << PLNT_COLOUR); // Preserve only the colour
          parts[si].tmp = parts[i].tmp;
          parts[si].tmp2 = parts[i].tmp2;
          parts[si].tmp3 = parts[i].tmp3;
          tmp4_set(si, tmp4_get(i));
        }
      }
      // Turn into usual plant (but preserve color)
      // We set direction to 7 to tell graphics() that PLNT needs coloring
      parts[i].ctype = (parts[i].ctype & (0x3f << PLNT_COLOUR)) | (7 << PLNT_DIR);
      parts[i].life = 0;
      parts[i].tmp = 0;
      parts[i].tmp2 = 0;
      parts[i].tmp3 = 0;
      tmp4_set(i, 0);
    }
    return 1;
  } else
  { // Usual PLNT code
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          switch (TYP(r)) {
            case PT_WATR:
              if (rng_chance(1, 50)) {
                //@ PLNT + WATR -> 2xPLNT
                int np = create_part(ID(r),x+rx,y+ry,PT_PLNT, -1);
                if (np<0) continue;
                parts[np].life = 0;
                parts[np].ctype = parts[i].ctype & PLNT_CTYPE;//Keep the color identical
              }
              break;
            case PT_LAVA:
              if (rng_chance(1, 50)) {
                //@ PLNT + LAVA -> FIRE + LAVA
                part_change_type(i,x,y,PT_FIRE);
                parts[i].life = 4;
              }
              break;
            case PT_SMKE:
            case PT_CO2:
              if (rng_chance(1, 50)) {
                kill_part(ID(r));
                parts[i].life = rng_between(60, 119);
              }
              break;
            case PT_WOOD: {
                int rndstore = rng_gen();
                if (surround_space && !(rndstore%4) && parts[i].tmp==1) {
                  rndstore >>= 3;
                  int nnx = (rndstore%3) -1;
                  rndstore >>= 2;
                  int nny = (rndstore%3) -1;
                  if (nnx || nny) {
                    if (PMAPAT(x+rx+nnx, y+ry+nny))
                      continue;
                    int np = create_part(-1,x+rx+nnx,y+ry+nny,PT_VINE, -1);
                    if (np<0) continue;
                    p_set_temp(&parts[np], p_temp(&parts[i]));
                  }
                }
              }
              break;
            default:
              continue;
          }
        }
      }
    }
    if (parts[i].life==2) {
      for (int rx = -1; rx <= 1; rx++) {
        for (int ry = -1; ry <= 1; ry++) {
          if (rx || ry) {
            int r = PMAPAT(x+rx, y+ry);
            //@ PLNT + SMKE/CO2 -> PLNT + O2
            if (!r)
              create_part(-1,x+rx,y+ry,PT_O2, -1);
          }
        }
      }
      parts[i].life = 0;
    }
    if (p_temp(&parts[i]) > 350 && p_temp(&parts[i]) > parts[i].tmp2)
      parts[i].tmp2 = (int)p_temp(&parts[i]);
  }
  return 0;
}
// Inverse of dir3x3 table
static const int invDir[3][3] = {
    { 1, 2, 3 },
    { 0, 8, 4 },
    { 7, 6, 5 },
};
int Element_PLNT_detectDown(int x, int y) {
  float pGravX = 0;
  float pGravY = 0;
  GetGravityField(x, y, 1.0f, 1.0f, &pGravX, &pGravY);
  float gravLen = hypotf(pGravX, pGravY);
  // Normalize gravity
  if (gravLen > 0) {
    pGravX /= gravLen;
    pGravY /= gravLen;
  }
  int gravx = 1;
  int gravy = 1;
  if (pGravX > 0.5f)
    gravx = 2;
  if (pGravX < -0.5f)
    gravx = 0;
  if (pGravY > 0.5f)
    gravy = 2;
  if (pGravY < -0.5f)
    gravy = 0;
  return invDir[gravx][gravy];
}

/* ---------------------------------------------------------------- BMTL */
int el_BMTL_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp>1) {
    parts[i].tmp--;
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if ((TYP(r)==PT_METL || TYP(r)==PT_IRON) && rng_chance(1, 100)) {
            //@ BMTL + METL/IRON -> 2xBMTL
            part_change_type(ID(r),x+rx,y+ry,PT_BMTL);
            parts[ID(r)].tmp = (parts[i].tmp<=7) ? parts[i].tmp=1 : parts[i].tmp - rng_between(0, 4);
          }
        }
      }
    }
  } else if (parts[i].tmp==1 && rng_chance(1, 1000)) {
    //@ BMTL -> BRMT
    parts[i].tmp = 0;
    part_change_type(i,x,y,PT_BRMT);
  }
  return 0;
}

/* ---------------------------------------------------------------- GLAS */
int el_GLAS_update(UPDATE_FUNC_ARGS) {
  int press = (int)(pv[y/ACELL][x/ACELL] * 64);
  int diff = press - parts[i].tmp3;
  // Determine whether the GLAS is chemically strengthened via .life setting. (250 = Max., 16 = Min.)
  int strength = (parts[i].life / 120) + 16;
  if (strength < 16)
    strength = 16;
  if (diff > strength || diff < -1 * strength) {
    //@ GLAS -> BGLA
    part_change_type(i, x, y, PT_BGLA);
  }
  parts[i].tmp3 = press;
  // Liquid nitrogen condenses on chemically strengthened glass
  if ((strength > 200) && (p_temp(&parts[i]) < 77.0f) && (hv[y/ACELL][x/ACELL] < 77.0f)
      && (pv[y/ACELL][x/ACELL] > 5.0f) && rng_chance(1, 100)) {
    // Sample 4 adjacent cells
    int adj = rng_between(0, 3);
    int rx = (1 - 2*(adj%2))*(1 - adj/2);
    int ry = (1 - 2*(adj%2))*(adj/2);
    int r = PMAPAT(x+rx, y+ry);
    // If found an empty spot around glass
    if (!r) {
      //@ GLAS -> GLAS + LNTG
      int np = create_part(-1, x+rx, y+ry, PT_LNTG, -1);
      if (np>-1) {
        pv[y/ACELL][x/ACELL] -= 1.0f;
        p_set_temp(&parts[i], p_temp(&parts[i]) + (200.0f));
      }
    }
  }
  return 0;
}
void el_GLAS_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp3 = (int)(pv[y/ACELL][x/ACELL] * 64);
}

/* ---------------------------------------------------------------- GOLD */
int el_GOLD_update(UPDATE_FUNC_ARGS) {
  static int checkCoordsX[] = { -4, 4, 0, 0 };
  static int checkCoordsY[] = { 0, 0, -4, 4 };
  //Find nearby rusted iron (BMTL with tmp 1+)
  for(int j = 0; j < 8; j++) {
    int rndstore = rng_gen();
    int rx = (rndstore % 9)-4;
    rndstore >>= 4;
    int ry = (rndstore % 9)-4;
    if ((!rx != !ry)) {
      int r = PMAPAT(x+rx, y+ry);
      if(!r) continue;
      if(TYP(r)==PT_BMTL && parts[ID(r)].tmp) {
        //@ GOLD + BMTL -> GOLD + IRON
        parts[ID(r)].tmp = 0;
        part_change_type(ID(r), x+rx, y+ry, PT_IRON);
      }
    }
  }
  //Find sparks
  if(!parts[i].life) {
    for(int j = 0; j < 4; j++) {
      int rx = checkCoordsX[j];
      int ry = checkCoordsY[j];
      int r = PMAPAT(x+rx, y+ry);
      if(!r) continue;
      if(TYP(r)==PT_SPRK && parts[ID(r)].life && parts[ID(r)].life<4) {
        part_change_type(i, x, y, PT_SPRK);
        parts[i].life = 4;
        parts[i].ctype = PT_GOLD;
      }
    }
  }
  if (TYP(PHOTAT(x, y)) == PT_NEUT) {
    if (rng_chance(1, 7)) {
      kill_part(ID(PHOTAT(x, y)));
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- IRON */
int el_IRON_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life)
    return 0;
  bool tryBreak = false;
  for (bool once = true; once; once = false) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          switch (TYP(r)) {
          case PT_SALT:
            //@ IRON + SALT -> BMTL + SALT
            if (rng_chance(1, 47))
              { tryBreak = true; goto done; }
            break;
          case PT_SLTW:
            //@ IRON + SLTW -> BMTL + SLTW
            if (rng_chance(1, 67))
              { tryBreak = true; goto done; }
            break;
          case PT_WATR:
            //@ IRON + WATR -> BMTL + WATR
            if (rng_chance(1, 1200))
              { tryBreak = true; goto done; }
            break;
          case PT_O2:
            //@ IRON + O2 -> BMTL + O2
            if (rng_chance(1, 250))
              { tryBreak = true; goto done; }
            break;
          case PT_LO2:
            //@ IRON + LO2 -> BMTL + LO2
            tryBreak = true;
            goto done;
          default:
            break;
          }
        }
      }
    }
  }
done:
  if (tryBreak) {
    part_change_type(i,x,y,PT_BMTL);
    parts[i].tmp = rng_between(20, 29);
  }
  return 0;
}

/* ---------------------------------------------------------------- TTAN */
int el_TTAN_update(UPDATE_FUNC_ARGS) {
  int ttan = 0;
  if (nt <= 2)
    ttan = 2;
  else if (parts[i].tmp)
    ttan = 2;
  else if (nt <= 6) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (!rx != !ry) {
          if (TYP(PMAPAT(x+rx, y+ry)) == PT_TTAN)
            ttan++;
        }
      }
    }
  }
  if (ttan >= 2) {
    bmap_blockair[y/ACELL][x/ACELL] = 1;
    bmap_blockairh[y/ACELL][x/ACELL] = 0x8;
  }
  return 0;
}

/* ---------------------------------------------------------------- TUNG */
int el_TUNG_update(UPDATE_FUNC_ARGS) {
  bool splode = false;
  const float MELTING_POINT = elements[PT_TUNG].HighTemperature;
  if(p_temp(&parts[i]) > 2400.0) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if(TYP(r) == PT_O2) {
            splode = true;
          }
        }
      }
    }
  }
  if((p_temp(&parts[i]) > MELTING_POINT && rng_chance(1, 20)) || splode) {
    if (rng_chance(1, 50)) {
      pv[y/ACELL][x/ACELL] += 50.0f;
    } else if (rng_chance(1, 100)) {
      //@ TUNG -> FIRE
      part_change_type(i, x, y, PT_FIRE);
      parts[i].life = rng_between(0, 499);
      return 1;
    } else {
      //@ TUNG -> LAVA(TUNG)
      part_change_type(i, x, y, PT_LAVA);
      parts[i].ctype = PT_TUNG;
      return 1;
    }
    if(splode) {
      p_set_temp(&parts[i], restrict_flt(MELTING_POINT + rng_between(200, 799), MIN_TEMP, MAX_TEMP));
    }
    p_set_vx(&parts[i], p_vx(&parts[i]) + (rng_between(-50, 50)));
    p_set_vy(&parts[i], p_vy(&parts[i]) + (rng_between(-50, 50)));
    return 1;
  }
  int press = (int)(pv[y/ACELL][x/ACELL] * 64);
  int diff = press - parts[i].tmp3;
  if (diff > 32 || diff < -32) {
    //@ TUNG -> BRMT(TUNG)
    part_change_type(i,x,y,PT_BRMT);
    parts[i].ctype = PT_TUNG;
    return 1;
  }
  parts[i].tmp3 = press;
  return 0;
}
void el_TUNG_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp3 = (int)(pv[y/ACELL][x/ACELL] * 64);
}

/* ---------------------------------------------------------------- PTNM */
static void PTNM_wtrv_reactions(int wtrv1_id, UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x + rx, y + ry);
        if (!r || ID(r) == wtrv1_id)
          continue;
        int rt = TYP(r);
        //@ PTNM + WTRV + BCOL -> PTNM + OIL
        if (rt == PT_BCOL && p_temp(&parts[ID(r)]) > 200.0f + 273.15f && p_temp(&parts[wtrv1_id]) > 200.0f + 273.15f && pv[(y + ry) / ACELL][(x + rx) / ACELL] > 7.f) {
          part_change_type(ID(r), x + rx, y + ry, PT_OIL);
          kill_part(wtrv1_id);
          return;
        }
      }
    }
  }
}
static void PTNM_hygn_reactions(int hygn1_id, UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x + rx, y + ry);
        if (!r || ID(r) == hygn1_id)
          continue;
        int rt = TYP(r);
        //@ PTNM + H2 + DESL -> PTNM + OIL + WATR
        if (rt == PT_DESL) {
          part_change_type(ID(r), x + rx, y + ry, PT_WATR);
          part_change_type(hygn1_id, (int)(p_x(&parts[hygn1_id]) + 0.5f), (int)(p_y(&parts[hygn1_id]) + 0.5f), PT_OIL);
          return;
        }
        //@ PTNM + H2 + O2 -> PTNM + 2xDSTW
        if (rt == PT_O2 && !parts[i].life) {
          part_change_type(ID(r), x + rx, y + ry, PT_DSTW);
          part_change_type(hygn1_id, (int)(p_x(&parts[hygn1_id]) + 0.5f), (int)(p_y(&parts[hygn1_id]) + 0.5f), PT_DSTW);
          p_set_temp(&parts[ID(r)], p_temp(&parts[ID(r)]) + (5.0f));
          p_set_temp(&parts[hygn1_id], p_temp(&parts[hygn1_id]) + (5.0f));
          parts[i].ctype = PT_PTNM;
          parts[i].life = 4;
          part_change_type(i, x, y, PT_SPRK);
          return;
        }
        // Cold fusion: 2 hydrogen > 500 C has a chance to fuse
        if (rt == PT_H2 && rng_chance(1, 1000) && p_temp(&parts[ID(r)]) > 500.0f + 273.15f && p_temp(&parts[hygn1_id]) > 500.0f + 273.15f) {
          //@ PTNM + 2xH2 -> PTNM + NBLE + NEUT + PHOT + maybe ELEC
          part_change_type(ID(r), x + rx, y + ry, PT_NBLE);
          part_change_type(hygn1_id, (int)(p_x(&parts[hygn1_id]) + 0.5f), (int)(p_y(&parts[hygn1_id]) + 0.5f), PT_NEUT);
          p_set_temp(&parts[ID(r)], p_temp(&parts[ID(r)]) + (1000.0f));
          p_set_temp(&parts[hygn1_id], p_temp(&parts[hygn1_id]) + (1000.0f));
          pv[(y + ry) / ACELL][(x + rx) / ACELL] += 10.0f;
          int j = create_part(-3, x + rx, y + ry, PT_PHOT, -1);
          if (j > -1) {
            parts[j].ctype = 0x7C0000;
            p_set_temp(&parts[j], p_temp(&parts[ID(r)]));
            parts[j].tmp = 0x1;
          }
          if (rng_chance(1, 10)) {
            int j = create_part(-3, x + rx, y + ry, PT_ELEC, -1);
            if (j > -1)
              p_set_temp(&parts[j], p_temp(&parts[ID(r)]));
          }
          return;
        }
      }
    }
  }
}
int el_PTNM_update(UPDATE_FUNC_ARGS) {
  int hygn1_id = -1; // Id of a hydrogen particle for hydrogen multi-particle reactions
  int wtrv1_id = -1; // same but wtrv
  // Fast conduction (like GOLD)
  if (!parts[i].life) {
    for (int j = 0; j < 4; j++) {
      static const int checkCoordsX[] = { -4, 4, 0, 0 };
      static const int checkCoordsY[] = { 0, 0, -4, 4 };
      int rx = checkCoordsX[j];
      int ry = checkCoordsY[j];
      int r = PMAPAT(x + rx, y + ry);
      if (r && TYP(r) == PT_SPRK && parts[ID(r)].life && parts[ID(r)].life < 4) {
        part_change_type(i, x, y, PT_SPRK);
        parts[i].life = 4;
        parts[i].ctype = PT_PTNM;
      }
    }
  }
  // Single element reactions
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x + rx, y + ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt == PT_H2 && hygn1_id < 0)
          hygn1_id = ID(r);
        if (rt == PT_WTRV && wtrv1_id < 0)
          wtrv1_id = ID(r);
        // These reactions will occur instantly in contact with PTNM
        // --------------------------------------------------------
        // Shield instantly grows (even without SPRK)
        if (!parts[ID(r)].life && (rt == PT_SHLD1 || rt == PT_SHLD2 || rt == PT_SHLD3)) {
          int next = PT_SHLD1;
          switch (rt) {
          case PT_SHLD1: next = PT_SHLD2; break;
          case PT_SHLD2: next = PT_SHLD3; break;
          case PT_SHLD3: next = PT_SHLD4; break;
          }
          part_change_type(ID(r), x + rx, y + ry, next);
          parts[ID(r)].life = 7;
          continue;
        }
        //@ PTNM + ISZS/ISOZ -> PTNM + PLUT + PHOT
        if (rt == PT_ISZS || rt == PT_ISOZ) {
          part_change_type(ID(r), x + rx, y + ry, PT_PLUT);
          create_part(-3, x + rx, y + ry, PT_PHOT, -1);
          continue;
        }
        // These reactions are dependent on temperature
        // Probability goes quadratically from 0% / frame to 100% / frame from 0 C to 1500 C
        // --------------------------------------------------------
        float prob = fminf(1.0f, p_temp(&parts[i]) / (273.15f + 1500.0f));
        prob *= prob;
        if (rng_uniform01() <= prob) {
          switch (rt) {
          case PT_GAS: // GAS + > 2 pressure + >= 200 C -> INSL
            if (p_temp(&parts[ID(r)]) >= 200.0f + 273.15f && pv[(y + ry) / ACELL][(x + rx) / ACELL] > 2.0f) {
              //@ PTNM + GAS -> PTNM + INSL
              part_change_type(ID(r), x + rx, y + ry, PT_INSL);
              p_set_temp(&parts[i], p_temp(&parts[i]) + (60.0f)); // Other part is INSL, adding temp is useless
            }
            break;
          case PT_BREC: // BREL + > 1000 C + > 50 pressure -> EXOT
            if (p_temp(&parts[ID(r)]) > 1000.0f + 273.15f && pv[(y + ry) / ACELL][(x + rx) / ACELL] > 50.0f) {
              //@ PTNM + BREC -> PTNM + EXOT
              part_change_type(ID(r), x + rx, y + ry, PT_EXOT);
              p_set_temp(&parts[ID(r)], p_temp(&parts[ID(r)]) - (30.0f));
              p_set_temp(&parts[i], p_temp(&parts[i]) - (30.0f));
            }
            break;
          case PT_SMKE: //@ PTNM + SMKE -> PTNM + CO2
            part_change_type(ID(r), x + rx, y + ry, PT_CO2);
            break;
          case PT_RSST: //@ PTNM + RSST -> PTNM + BIZR
            create_part(ID(r), x + rx, y + ry, PT_BIZR, -1);
            break;
          }
        }
      }
    }
  }
  // Hydrogen reactions
  if (hygn1_id >= 0) {
    PTNM_hygn_reactions(hygn1_id, UPDATE_FUNC_SUBCALL_ARGS);
  }
  // WTRV reactions
  if (wtrv1_id >= 0) {
    PTNM_wtrv_reactions(wtrv1_id, UPDATE_FUNC_SUBCALL_ARGS);
  }
  return 0;
}
void el_PTNM_create(ELEMENT_CREATE_FUNC_ARGS) {
  if (rng_chance(1, 15))
    parts[i].tmp = 1;
}

/* ---------------------------------------------------------------- CRMC */
int el_CRMC_update(UPDATE_FUNC_ARGS) {
  float origTemp = p_temp(&parts[i]);
  //@ CRMC -> CLST
  if (pv[y/ACELL][x/ACELL] < -30.0f)
    create_part(i, x, y, PT_CLST, -1);
  p_set_temp(&parts[i], origTemp);
  return 0;
}
void el_CRMC_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp2 = rng_between(0, 4);
}

/* ---------------------------------------------------------------- HEAC */
/* is there a heat insulator on the line (CheckLine with HEAC's predicate) */
static bool HEAC_insulated(int x1, int y1, int x2, int y2) {
  bool reverseXY = abs(y2-y1) > abs(x2-x1);
  int x, y, dx, dy, sy, p;
  float e, de;
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
  e = 0.0f;
  de = dx ? dy/(float)dx : 0.0f;
  y = y1;
  sy = (y1<y2) ? 1 : -1;
  for (x=x1; x<=x2; x++) {
    p = reverseXY ? PMAPAT(y, x) : PMAPAT(x, y);
    if (p && IsHeatInsulator(&parts[ID(p)])) return true;
    e += de;
    if (e >= 0.5f) {
      y += sy;
      if ((y1<y2) ? (y<=y2) : (y>=y2)) {
        p = reverseXY ? PMAPAT(y, x) : PMAPAT(x, y);
        if (p && IsHeatInsulator(&parts[ID(p)])) return true;
      }
      e -= 1.0f;
    }
  }
  return false;
}
int el_HEAC_update(UPDATE_FUNC_ARGS) {
  const int rad = 4;
  int rry, rrx, r;
  float c_heat = 0.0f;
  float hc_total = 0.0f;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      rry = ry * rad;
      rrx = rx * rad;
      if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !HEAC_insulated(x, y, x+rrx, y+rry)) {
        r = PMAPAT(x+rrx, y+rry);
        if (r && !IsHeatInsulator(&parts[ID(r)])) {
          float hc = HeatCapacityOf(&parts[ID(r)]);
          c_heat += p_temp(&parts[ID(r)]) * hc;
          hc_total += hc;
        }
        r = PHOTAT(x+rrx, y+rry);
        if (r && !IsHeatInsulator(&parts[ID(r)])) {
          float hc = HeatCapacityOf(&parts[ID(r)]);
          c_heat += p_temp(&parts[ID(r)]) * hc;
          hc_total += hc;
        }
      }
    }
  }
  if (hc_total > 0.0f) {
    float pt = restrict_flt(c_heat / hc_total, MIN_TEMP, MAX_TEMP);
    p_set_temp(&parts[i], pt);
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        rry = ry * rad;
        rrx = rx * rad;
        if (x+rrx >= 0 && x+rrx < XRES && y+rry >= 0 && y+rry < YRES && !HEAC_insulated(x, y, x+rrx, y+rry)) {
          r = PMAPAT(x+rrx, y+rry);
          if (r && !IsHeatInsulator(&parts[ID(r)]))
            p_set_temp(&parts[ID(r)], pt);
          r = PHOTAT(x+rrx, y+rry);
          if (r && !IsHeatInsulator(&parts[ID(r)]))
            p_set_temp(&parts[ID(r)], pt);
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- ROCK */
void el_ROCK_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp2 = rng_between(0, 10);
}

/* ---------------------------------------------------------------- VINE */
int el_VINE_update(UPDATE_FUNC_ARGS) {
  int rndstore = rng_gen();
  int rx = (rndstore % 3) - 1;
  rndstore >>= 2;
  int ry = (rndstore % 3) - 1;
  rndstore >>= 2;
  if (rx || ry) {
    int r = PMAPAT(x+rx, y+ry);
    //@ VINE -> PLNT
    if (!(rndstore % 15))
      part_change_type(i, x, y, PT_PLNT);
    else if (!r) {
      int np = create_part(-1,x+rx,y+ry,PT_VINE, -1);
      if (np<0) return 0;
      p_set_temp(&parts[np], p_temp(&parts[i]));
      part_change_type(i,x,y,PT_PLNT);
    }
  }
  if (p_temp(&parts[i]) > 350 && p_temp(&parts[i]) > parts[i].tmp2)
    parts[i].tmp2 = (int)p_temp(&parts[i]);
  return 0;
}

/* ---------------------------------------------------------------- SPNG */
int el_SPNG_update(UPDATE_FUNC_ARGS) {
  int limit = 50;
  if (parts[i].life<limit && pv[y/ACELL][x/ACELL]<=3&&pv[y/ACELL][x/ACELL]>=-3&&p_temp(&parts[i])<=374.0f) {
    int absorbChanceDenom = parts[i].life*10000/limit + 500;
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          switch (TYP(r)) {
          case PT_WATR:
          case PT_DSTW:
          case PT_FRZW:
            if (parts[i].life<limit && rng_chance(500, absorbChanceDenom)) {
              parts[i].life++;
              kill_part(ID(r));
            }
            break;
          case PT_SLTW:
            if (parts[i].life<limit && rng_chance(50, absorbChanceDenom)) {
              parts[i].life++;
              if (rng_chance(3, 4))
                kill_part(ID(r));
              else //@ SPNG + SLTW -> SPNG + SALT
                part_change_type(ID(r), x+rx, y+ry, PT_SALT);
            }
            break;
          case PT_CBNW:
            if (parts[i].life<limit && rng_chance(100, absorbChanceDenom)) {
              parts[i].life++;
              //@ SPNG + CBNW -> SPNG + CO2
              part_change_type(ID(r), x+rx, y+ry, PT_CO2);
            }
            break;
          case PT_PSTE:
            if (parts[i].life<limit && rng_chance(20, absorbChanceDenom)) {
              parts[i].life++;
              //@ SPNG + PSTE -> SPNG + CLST
              create_part(ID(r), x+rx, y+ry, PT_CLST, -1);
            }
            break;
          default:
            continue;
          }
        }
      }
    }
  } else {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if ((!r)&&parts[i].life>=1) { //if nothing then create water
            //@ SPNG -> SPNG + WATR
            int np = create_part(-1,x+rx,y+ry,PT_WATR, -1);
            if (np>-1) parts[i].life--;
          }
        }
      }
    }
  }
  for (int trade = 0; trade<9; trade ++) {
    int rx = rng_between(-2, 2);
    int ry = rng_between(-2, 2);
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      if (TYP(r)==PT_SPNG&&(parts[i].life>parts[ID(r)].life)&&parts[i].life>0) { //diffusion
        int tmp = parts[i].life - parts[ID(r)].life;
        if (tmp ==1) {
          parts[ID(r)].life ++;
          parts[i].life --;
          trade = 9;
        } else if (tmp>0) {
          parts[ID(r)].life += tmp/2;
          parts[i].life -= tmp/2;
          trade = 9;
        }
      }
    }
  }
  int tmp = 0;
  if (parts[i].life>0) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r)==PT_FIRE) {
            tmp++;
            if (parts[ID(r)].life>60)
              parts[ID(r)].life -= parts[ID(r)].life/60;
            else if (parts[ID(r)].life>2)
              parts[ID(r)].life--;
          }
        }
      }
    }
  }
  if (tmp && parts[i].life>3)
    parts[i].life -= parts[i].life/3;
  if (tmp>1)
    tmp = tmp/2;
  if (tmp || p_temp(&parts[i])>=374) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if ((!r)&&parts[i].life>=1) { //if nothing then create steam
            //@ SPNG -> SPNG + WTRV
            int np = create_part(-1,x+rx,y+ry,PT_WTRV, -1);
            if (np>-1) {
              p_set_temp(&parts[np], p_temp(&parts[i]));
              tmp--;
              parts[i].life--;
              p_set_temp(&parts[i], restrict_flt(p_temp(&parts[i]) - 20.0f, MIN_TEMP, MAX_TEMP));
            }
          }
        }
      }
    }
  }
  if (tmp>0) {
    if (parts[i].life>tmp)
      parts[i].life -= tmp;
    else
      parts[i].life = 0;
  }
  return 0;
}

/* ---------------------------------------------------------------- QRTZ */
int Element_QRTZ_update(UPDATE_FUNC_ARGS) {
  int t = parts[i].type;
  if (t == PT_QRTZ) {
    int press = (int)(pv[y/ACELL][x/ACELL] * 64);
    float diffTolerance = p_temp(&parts[i]) * 1.0666f;
    if (press - parts[i].tmp3 > diffTolerance || press - parts[i].tmp3 < -diffTolerance) {
      //@ QRTZ -> PQRT
      part_change_type(i,x,y,PT_PQRT);
      parts[i].life = 5; //timer before it can grow or diffuse again
    }
    parts[i].tmp3 = press;
  }
  if (parts[i].life>5)
    parts[i].life = 5;
  // absorb SLTW
  if (parts[i].tmp != -1) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          else if (TYP(r)==PT_SLTW && rng_chance(1, 500)) {
            kill_part(ID(r));
            parts[i].tmp++;
          }
        }
      }
    }
  }
  // grow and diffuse
  if (parts[i].tmp > 0 && (p_vx(&parts[i])*p_vx(&parts[i]) + p_vy(&parts[i])*p_vy(&parts[i]))<0.2f && parts[i].life<=0) {
    bool stopgrow = false;
    for (int trade = 0; trade < 9; trade++) {
      int rnd = rng_gen() % 0x3FF;
      int rx = (rnd%5)-2;
      int srx = (rnd%3)-1;
      rnd >>= 3;
      int ry = (rnd%5)-2;
      int sry = (rnd%3)-1;
      if (rx || ry) {
        if (!stopgrow) { //try to grow
          if (!PMAPAT(x+srx, y+sry) && parts[i].tmp!=0) {
            //@ PQRT/QRTZ -> 2xQRTZ
            int np = create_part(-1,x+srx,y+sry,PT_QRTZ, -1);
            if (np>-1) {
              p_set_temp(&parts[np], p_temp(&parts[i]));
              parts[np].tmp2 = parts[i].tmp2;
              if (rng_chance(1, 2)) {
                parts[np].tmp2 = iclamp(parts[np].tmp2 + rng_between(-1, 1), 0, 10);
              }
              parts[i].tmp--;
              if (t == PT_PQRT) {
                // If PQRT is stationary and has started growing particles of QRTZ, the PQRT is basically part of a new QRTZ crystal. So turn it back into QRTZ so that it behaves more like part of the crystal.
                part_change_type(i,x,y,PT_QRTZ);
              }
              if (rng_chance(1, 2)) {
                parts[np].tmp=-1;//dead qrtz
              } else if (!parts[i].tmp && rng_chance(1, 15)) {
                parts[i].tmp=-1;
              }
              stopgrow=true;
            }
          }
        }
        //diffusion
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        else if (TYP(r)==PT_QRTZ && (parts[i].tmp>parts[ID(r)].tmp) && parts[ID(r)].tmp>=0) {
          int tmp = parts[i].tmp - parts[ID(r)].tmp;
          if (tmp ==1) {
            parts[ID(r)].tmp++;
            parts[i].tmp--;
            break;
          }
          if (tmp>0) {
            parts[ID(r)].tmp += tmp/2;
            parts[i].tmp -= tmp/2;
            break;
          }
        }
      }
    }
  }
  return 0;
}
void el_QRTZ_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp2 = rng_between(0, 10);
  parts[i].tmp3 = (int)(pv[y/ACELL][x/ACELL] * 64);
}

/* ---------------------------------------------------------------- PQRT */
void el_PQRT_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp2 = rng_between(0, 10);
}

/* ---------------------------------------------------------------- VIBR */
static const int orbit_rx[8] = { -1,  0,  1,  1,  1,  0, -1, -1 };
static const int orbit_ry[8] = { -1, -1, -1,  0,  1,  1,  1,  0 };
int Element_VIBR_update(UPDATE_FUNC_ARGS) {
  int rndstore = 0;
  if (!parts[i].life) { //if not exploding
    //Heat absorption code
    if (p_temp(&parts[i]) > 274.65f) {
      parts[i].tmp++;
      p_set_temp(&parts[i], p_temp(&parts[i]) - (3));
    } else if (p_temp(&parts[i]) < 271.65f) {
      parts[i].tmp--;
      p_set_temp(&parts[i], p_temp(&parts[i]) + (3));
    }
    //Pressure absorption code
    if (pv[y/ACELL][x/ACELL] > 2.5) {
      parts[i].tmp += 7;
      pv[y/ACELL][x/ACELL]--;
    } else if (pv[y/ACELL][x/ACELL] < -2.5) {
      parts[i].tmp -= 2;
      pv[y/ACELL][x/ACELL]++;
    }
    //initiate explosion counter
    if (parts[i].tmp > 1000)
      parts[i].life = 750;
  } else { //if it is exploding
    //Release sparks before explode
    rndstore = rng_gen();
    if (parts[i].life < 300) {
      int rx = orbit_rx[rndstore & 7];
      int ry = orbit_ry[rndstore & 7];
      rndstore = rndstore >> 3;
      int r = PMAPAT(x+rx, y+ry);
      if (TYP(r) && TYP(r) != PT_BREC && (elements[TYP(r)].Properties&PROP_CONDUCTS) && !parts[ID(r)].life) {
        parts[ID(r)].life = 4;
        parts[ID(r)].ctype = TYP(r);
        part_change_type(ID(r),x+rx,y+ry,PT_SPRK);
      }
    }
    //Release all heat
    if (parts[i].life < 500) {
      int rx = rndstore%7-3;
      int ry = (rndstore>>3)%7-3;
      int r = PMAPAT(x+rx, y+ry);
      if (TYP(r) && TYP(r) != PT_VIBR && TYP(r) != PT_BVBR && (!IsHeatInsulator(&parts[ID(r)]))) {
        p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)]) + parts[i].tmp * 3, MIN_TEMP, MAX_TEMP));
        parts[i].tmp = 0;
      }
    }
    //Explosion code
    if (parts[i].life == 1) {
      if (!parts[i].tmp2) {
        //@ VIBR/BVBR -> EXOT + ELEC + PHOT + BREC
        rndstore = rng_gen();
        int index = create_part(-3, x + (orbit_rx[rndstore & 7]), y + (orbit_ry[rndstore & 7]), PT_ELEC, -1);
        if (index != -1)
          p_set_temp(&parts[index], 7000);
        index = create_part(-3, x + (orbit_rx[(rndstore >> 3) & 7]), y + (orbit_ry[(rndstore >> 3) & 7]), PT_PHOT, -1);
        if (index != -1)
          p_set_temp(&parts[index], 7000);
        index = create_part(-1, x + (orbit_rx[(rndstore >> 6) & 7]), y + (orbit_ry[(rndstore >> 6) & 7]), PT_BREC, -1);
        if (index != -1)
          p_set_temp(&parts[index], 7000);
        create_part(i, x, y, PT_EXOT, -1);
        parts[i].tmp2 = (rndstore >> 9) % 1000;
        p_set_temp(&parts[i], 9000);
        pv[y/ACELL][x/ACELL] += 50;
        return 1;
      } else {
        parts[i].tmp2 = 0;
        p_set_temp(&parts[i], 273.15f);
        parts[i].tmp = 0;
      }
    }
  }
  //Neighbor check loop
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (parts[i].life) {
          //Makes EXOT around it get tmp to start exploding too
          if ((TYP(r)==PT_VIBR  || TYP(r)==PT_BVBR)) {
            if (!parts[ID(r)].life)
              parts[ID(r)].tmp += 45;
            else if (parts[i].tmp2 && parts[i].life > 75 && rng_chance(1, 2)) {
              parts[ID(r)].tmp2 = 1;
              parts[i].tmp = 0;
            }
          } else if (TYP(r)==PT_CFLM) {
            parts[i].tmp2 = 1;
            parts[i].tmp = 0;
          }
        } else {
          //Melts into EXOT
          if (TYP(r) == PT_EXOT && rng_chance(1, 25)) {
            //@ VIBR/BVBR + EXOT -> 2xEXOT
            part_change_type(i, x, y, PT_EXOT);
            return 1;
          }
        }
        //@ VIBR + ANAR -> BVBR
        if (parts[i].type != PT_BVBR && TYP(r) == PT_ANAR) {
          part_change_type(i,x,y,PT_BVBR);
          pv[y/ACELL][x/ACELL] -= 1;
        }
      }
    }
  }
  for (int trade = 0; trade < 9; trade++) {
    if (!(trade%2))
      rndstore = rng_gen();
    int rx = rndstore%7-3;
    rndstore >>= 3;
    int ry = rndstore%7-3;
    rndstore >>= 3;
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (TYP(r) != PT_VIBR && TYP(r) != PT_BVBR)
        continue;
      if (parts[i].tmp > parts[ID(r)].tmp) {
        int transfer = parts[i].tmp - parts[ID(r)].tmp;
        parts[ID(r)].tmp += transfer/2;
        parts[i].tmp -= transfer/2;
        break;
      }
    }
  }
  parts[i].tmp = (int16_t)iclamp(parts[i].tmp, 0, 32767);   /* (the original: 0 to 1 << 15) */
  return 0;
}

/* ---------------------------------------------------------------- RSSS */
int el_RSSS_update(UPDATE_FUNC_ARGS) {
  for(int rx = -1; rx < 2; rx++) {
    for(int ry = -1; ry < 2; ry++) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r)
        continue;
      // Set RSSS ctype from nearby clone
      if((TYP(r) == PT_CLNE) || (TYP(r) == PT_PCLN)) {
        if(parts[ID(r)].ctype != PT_RSSS)
          parts[i].ctype = parts[ID(r)].ctype;
      }
      // Set RSSS tmp from nearby breakable clone
      if((TYP(r) == PT_BCLN) || (TYP(r) == PT_PBCN)) {
        if(parts[ID(r)].ctype != PT_RSSS)
          parts[i].tmp = parts[ID(r)].ctype;
      }
    }
  }
  //Block air like TTAN
  bmap_blockair[y/ACELL][x/ACELL] = 1;
  bmap_blockairh[y/ACELL][x/ACELL] = 0x8;
  return 0;
}

/* ---------------------------------------------------------------- SHLD1 */
int el_SHLD1_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        else if (TYP(r)==PT_SPRK&&parts[i].life==0) {
          if (rng_chance(11, 40)) {
            part_change_type(i,x,y,PT_SHLD2);
            parts[i].life = 7;
          }
          for (int nnx = -1; nnx <= 1; nnx++) {
            for (int nny = -1; nny <= 1; nny++) {
              if (!PMAPAT(x+rx+nnx, y+ry+nny)) {
                create_part(-1,x+rx+nnx,y+ry+nny,PT_SHLD1, -1);
                //parts[ID(PMAPAT(x+nx+nnx, y+ny+nny))].life=7;
              }
            }
          }
        } else if (TYP(r) == PT_SHLD3 && rng_chance(2, 5)) {
          part_change_type(i,x,y,PT_SHLD2);
          parts[i].life = 7;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SHLD2 */
int el_SHLD2_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r) {
          if (parts[i].life>0)
            create_part(-1,x+rx,y+ry,PT_SHLD1, -1);
          continue;
        } else if (TYP(r)==PT_SPRK&&parts[i].life==0) {
          if (rng_chance(1, 8)) {
            part_change_type(i,x,y,PT_SHLD3);
            parts[i].life = 7;
          }
          for (int nnx = -1; nnx <= 1; nnx++) {
            for (int nny = -1; nny <= 1; nny++) {
              if (!PMAPAT(x+rx+nnx, y+ry+nny)) {
                int np = create_part(-1,x+rx+nnx,y+ry+nny,PT_SHLD1, -1);
                if (np<0) continue;
                parts[np].life=7;
              }
            }
          }
        } else if (TYP(r) == PT_SHLD4 && rng_chance(2, 5)) {
          part_change_type(i,x,y,PT_SHLD3);
          parts[i].life = 7;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SHLD3 */
int el_SHLD3_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r) {
          if (rng_chance(1, 2500)) {
            int np = create_part(-1,x+rx,y+ry,PT_SHLD1, -1);
            if (np<0) continue;
            parts[np].life=7;
            part_change_type(i,x,y,PT_SHLD2);
          }
          continue;
        }
        if (TYP(r)==PT_SHLD1 && parts[i].life>3) {
          part_change_type(ID(r),x+rx,y+ry,PT_SHLD2);
          parts[ID(r)].life=7;
        } else if (TYP(r)==PT_SPRK&&parts[i].life==0) {
          if (rng_chance(3, 500)) {
            part_change_type(i,x,y,PT_SHLD4);
            parts[i].life = 7;
          }
          for (int nnx = -1; nnx <= 1; nnx++) {
            for (int nny = -1; nny <= 1; nny++) {
              if (!PMAPAT(x+rx+nnx, y+ry+nny)) {
                int np = create_part(-1,x+rx+nnx,y+ry+nny,PT_SHLD1, -1);
                if (np<0) continue;
                parts[np].life=7;
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SHLD4 */
int el_SHLD4_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r) {
          if (rng_chance(1, 5500)) {
            int np = create_part(-1,x+rx,y+ry,PT_SHLD1, -1);
            if (np<0) continue;
            parts[np].life=7;
            part_change_type(i,x,y,PT_SHLD2);
          }
          continue;
        }
        if (TYP(r)==PT_SHLD2 && parts[i].life>3) {
          part_change_type(ID(r),x+rx,y+ry,PT_SHLD3);
          parts[ID(r)].life = 7;
        } else if (TYP(r)==PT_SPRK&&parts[i].life==0) {
          for (int nnx = -1; nnx <= 1; nnx++) {
            for (int nny = -1; nny <= 1; nny++) {
              if (!PMAPAT(x+rx+nnx, y+ry+nny)) {
                int np = create_part(-1,x+rx+nnx,y+ry+nny,PT_SHLD1, -1);
                if (np<0) continue;
                parts[np].life=7;
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SPAWN */
bool el_SPAWN_createAllowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS) {
  return player.spawnID == -1;
}
void el_SPAWN_changeType(ELEMENT_CHANGETYPE_FUNC_ARGS) {
  if (to == PT_SPAWN) {
    if (player.spawnID == -1)
      player.spawnID = i;
  } else {
    if (player.spawnID == i)
      player.spawnID = -1;
  }
}

/* ---------------------------------------------------------------- SPAWN2 */
bool el_SPAWN2_createAllowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS) {
  return player2.spawnID == -1;
}
void el_SPAWN2_changeType(ELEMENT_CHANGETYPE_FUNC_ARGS) {
  if (to == PT_SPAWN2) {
    if (player2.spawnID == -1)
      player2.spawnID = i;
  } else {
    if (player2.spawnID == i)
      player2.spawnID = -1;
  }
}

/* ---------------------------------------------------------------- FILT */
void el_FILT_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp = v;
}
// Returns the wavelengths in a particle after FILT interacts with it (e.g. a photon)
// cpart is the FILT particle, origWl the original wavelengths in the interacting particle
int Element_FILT_interactWavelengths(Particle* cpart, int origWl) {
  const int mask = 0x3FFFFFFF;
  int filtWl = Element_FILT_getWavelengths(cpart);
  switch (cpart->tmp) {
    case 0:
      return filtWl; //Assign Colour
    case 1:
      return origWl & filtWl; //Filter Colour
    case 2:
      return origWl | filtWl; //Add Colour
    case 3:
      return origWl & (~filtWl); //Filter Colour the other way
    case 4: {
      int shift = (int)((p_temp(cpart)-273.0f)*0.025f);
      if (shift<=0) shift = 1;
      return (int)(((unsigned)origWl << (shift & 31)) & mask); // red shift (a count over 31 taken mod 32, as on x86)
    }
    case 5: {
      int shift = (int)((p_temp(cpart)-273.0f)*0.025f);
      if (shift<=0) shift = 1;
      return (origWl >> (shift & 31)) & mask; // blue shift
    }
    case 6:
      return origWl; // No change
    case 7:
      return origWl ^ filtWl; // XOR colours
    case 8:
      return (~origWl) & mask; // Invert colours
    case 9: {
      int t1 = (origWl & 0x0000FF) + rng_between(-2, 2);
      int t2 = ((origWl & 0x00FF00)>>8) + rng_between(-2, 2);
      int t3 = ((origWl & 0xFF0000)>>16) + rng_between(-2, 2);
      return (origWl & 0xFF000000) | (t3<<16) | (t2<<8) | t1;
    }
    case 10: {
      long long int lsb = filtWl & (-(long long)filtWl);
      return (int)((origWl * lsb) & 0x3FFFFFFF); //red shift
    }
    case 11: {
      long long int lsb = filtWl & (-(long long)filtWl);
      return lsb ? (int)((origWl / lsb) & 0x3FFFFFFF) : 0; // blue shift
    }
    default:
      return filtWl;
  }
}
int Element_FILT_getWavelengths(const Particle* cpart) {
  if (cpart->ctype&0x3FFFFFFF) {
    return cpart->ctype;
  } else {
    int temp_bin = (int)((p_temp(cpart)-273.0f)*0.025f);
    if (temp_bin < 0) temp_bin = 0;
    if (temp_bin > 25) temp_bin = 25;
    return (0x1F << temp_bin);
  }
}

