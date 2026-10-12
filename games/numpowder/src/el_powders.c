/* NumPowder: The Powder Toy's powders (their files in elements/), ported: SNOW, FRZZ, RIME, YEST, BRMT, BREC, CLST,
 * SLCN, GRAV, ANAR, SEED. */
#include "simpriv.h"

/* ---------------------------------------------------------------- SNOW */
int el_SNOW_update(UPDATE_FUNC_ARGS) {
  if (parts[i].ctype==PT_FRZW) { //get colder if it is from FRZW
    p_set_temp(&parts[i], restrict_flt(p_temp(&parts[i])-1.0f, MIN_TEMP, MAX_TEMP));
  }
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        //@ SNOW + SALT/SLTW -> 2xSLTW
        if ((TYP(r)==PT_SALT || TYP(r)==PT_SLTW) && rng_chance(1, 333)) {
          part_change_type(i,x,y,PT_SLTW);
          part_change_type(ID(r),x+rx,y+ry,PT_SLTW);
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- FRZZ */
int el_FRZZ_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_WATR && rng_chance(1, 20)) {
          //@ FRZZ + WATR -> FRZW
          part_change_type(ID(r),x+rx,y+ry,PT_FRZW);
          parts[ID(r)].life = 100;
          kill_part(i);
          return 1;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- RIME */
int el_RIME_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SPRK) {
          //@ RIME -> FOG
          part_change_type(i,x,y,PT_FOG);
          parts[i].life = rng_between(60, 119);
        }
        // GAS increases acidity
        else if (TYP(r) == PT_GAS && parts[i].tmp < 10) {
          kill_part(ID(r));
          if (parts[i].ctype == PT_DSTW)
            parts[i].ctype = 0;
          else
            parts[i].tmp++;
        } else if (TYP(r)==PT_FOG&&parts[ID(r)].life>0) {
          //@ RIME + FOG -> 2xFOG
          part_change_type(i,x,y,PT_FOG);
          parts[i].life = parts[ID(r)].life;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- YEST */
int el_YEST_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_DYST && rng_chance(1, 6) && !legacy_enable) {
          //@ YEST -> DYST
          part_change_type(i,x,y,PT_DYST);
        }
      }
    }
  }
  if (p_temp(&parts[i]) > 303 && p_temp(&parts[i]) < 317) {
    create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_YEST, -1);
  }
  return 0;
}

/* ---------------------------------------------------------------- BRMT */
int el_BRMT_update(UPDATE_FUNC_ARGS) {
  if (p_temp(&parts[i]) > 523.15f) { //250.0f+273.15f
    float tempFactor = (int)(1000 - ((523.15f-p_temp(&parts[i]))*2));
    if(tempFactor < 2)
      tempFactor = 2;
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (rx || ry) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r)==PT_BREC && rng_chance(1, tempFactor)) {
            if (rng_chance(1, 2)) {
              //@ BRMT + BREC -> BRMT + THRM
              create_part(ID(r), x+rx, y+ry, PT_THRM, -1);
            } else //@ BRMT + BREC -> THRM + BREC
              create_part(i, x, y, PT_THRM, -1);
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- BREC */
int el_BREC_update(UPDATE_FUNC_ARGS) {
  if (parts[i].life) {
    if (pv[y/ACELL][x/ACELL]>10.0f) {
      if (p_temp(&parts[i])>9000 && pv[y/ACELL][x/ACELL]>30.0f && rng_chance(1, 200)) {
        //@ BREC -> EXOT
        part_change_type(i, x, y, PT_EXOT);
        parts[i].life = 1000;
      }
      p_set_temp(&parts[i], p_temp(&parts[i]) + ((pv[y/ACELL][x/ACELL])/8));
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- CLST */
int el_CLST_update(UPDATE_FUNC_ARGS) {
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_WATR) {
          if (rng_chance(1, 1500)) {
            //@ CLST + WATR -> PSTS
            create_part(i, x, y, PT_PSTS, -1);
            kill_part(ID(r));
          }
        } else if (TYP(r)==PT_NITR) {
          //@ CLST + NITR -> 2xBANG
          create_part(i, x, y, PT_BANG, -1);
          create_part(ID(r), x+rx, y+ry, PT_BANG, -1);
        } else if (TYP(r)==PT_CLST) {
          float cxy;
          if(p_temp(&parts[i]) <195)
            cxy = 0.05f;
          else if(p_temp(&parts[i]) <295)
            cxy = 0.015f;
          else if(p_temp(&parts[i]) <350)
            cxy = 0.01f;
          else
            cxy = 0.005f;
          p_set_vx(&parts[i], p_vx(&parts[i]) + (cxy*rx));
          p_set_vy(&parts[i], p_vy(&parts[i]) + (cxy*ry));//These two can be set not to calculate over 350 later. They do virtually nothing over 0.005.
        }
      }
    }
  }
  return 0;
}
void el_CLST_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].tmp = rng_between(0, 6);
}

/* ---------------------------------------------------------------- SLCN */
/* the sparkles' state (the original's 32-bit tmp): bits 0 to 15 in tmp, 16 to 31 in tmp2 */
static uint32_t SLCN_get(int i) { return (uint16_t)parts[i].tmp | (uint32_t)(uint16_t)parts[i].tmp2 << 16; }
static void SLCN_set(int i, uint32_t v) {
  parts[i].tmp = (int16_t)(v & 0xFFFF);
  parts[i].tmp2 = (int16_t)(v >> 16);
}
static void SLCN_initSparkles(int i) {
  // bits 31-20: phase increment (randomised to a value between 1 and 9)
  // bits 19-16: next colour index
  // bits 15-12: current colour index
  // bits 11-00: phase
  SLCN_set(i, (uint32_t)rng_between(0x100000, 0x9FFFFF));
}
int el_SLCN_update(UPDATE_FUNC_ARGS) {
  if (!SLCN_get(i)) {
    SLCN_initSparkles(i);
  }
  uint32_t tmp = SLCN_get(i);
  int phase = (tmp & 0xFFF) + ((tmp >> 20) & 0xFFF);
  if (phase & 0x1000) {
    // discard current, current <- next, next <- random, wrap phase
    SLCN_set(i, (tmp & 0xFFF00000) | (phase & 0xFFF) | ((uint32_t)rng_between(0, 15) << 16) | ((tmp >> 4) & 0xF000));
  } else {
    // update phase
    SLCN_set(i, (tmp & 0xFFFFF000) | (uint32_t)phase);
  }
  if (parts[i].life == 0 && p_temp(&parts[i]) < 373.15f) {
    for (int j = 0; j < 4; j++) {
      static const int check_coords_x[] = { -4, 4, 0, 0 };
      static const int check_coords_y[] = { 0, 0, -4, 4 };
      int n = PMAPAT(x + check_coords_x[j], y + check_coords_y[j]);
      if (n && TYP(n) == PT_SPRK) {
        Particle *neighbour = &parts[ID(n)];
        if (neighbour->life != 0 && neighbour->life < 4) {
          part_change_type(i, x, y, PT_SPRK);
          parts[i].life = 4;
          parts[i].ctype = PT_SLCN;
        }
      }
    }
  }
  return 0;
}
void el_SLCN_create(ELEMENT_CREATE_FUNC_ARGS) {
  SLCN_initSparkles(i);
}

/* ---------------------------------------------------------------- GRAV */
int el_GRAV_update(UPDATE_FUNC_ARGS) {
  if (p_vx(&parts[i])*p_vx(&parts[i]) + p_vy(&parts[i])*p_vy(&parts[i]) >= 0.1f && rng_chance(1, 512)) {
    if (!parts[i].life)
      parts[i].life = 48;
  }
  return 0;
}

/* ---------------------------------------------------------------- ANAR */
int el_ANAR_update(UPDATE_FUNC_ARGS) {
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_CFLM && rng_chance(1, 4)) {
          //@ ANAR + CFLM -> 2xCFLM
          part_change_type(i,x,y,PT_CFLM);
          parts[i].life = rng_between(50, 199);
          p_set_temp(&parts[i], 0);
          p_set_temp(&parts[ID(r)], 0);
          pv[y/ACELL][x/ACELL] -= 0.5;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- SEED */
// Fields used (see also PLNT.cpp):
// ctype, tmp-tmp4 have the same meaning as in PLNT,
// but the first bit of ctype is used to determine whether a seed bred with another seed or not.
// life is used as a timer.
int el_SEED_update(UPDATE_FUNC_ARGS) {
  float temp = p_temp(&parts[i]);
  // Turn into liquid wax under high temperature and pressure
  if (temp > 320.0f && pv[y/ACELL][x/ACELL] > 50.0f && rng_chance(1, 150)) {
    //@ SEED -> MWAX
    create_part(i, x, y, PT_MWAX, -1);
    parts[i].ctype = 0;
    parts[i].life = 0;
    parts[i].tmp = 0;
    parts[i].tmp2 = 0;
    parts[i].tmp3 = 0;
    tmp4_set(i, 0);
    return 0;
  }
  // Don't do anything at bad temperatures
  if (temp > 343.15f || temp < 278.15f) {
    parts[i].ctype &= ~(0xFF << PLNT_LIFE); // Set water to zero
    parts[i].life = 0;
    return 0;
  }
  int water = (parts[i].ctype >> PLNT_LIFE) & 0xFF;
  // Check if we can grow
  if (water > 3) {
    int down = Element_PLNT_detectDown(x, y); // Down gravity direction
    // Set a default direction if can't decide using gravity
    if (down < 0 || down > 7)
      down = 4;
    int up = (down+4)%8;
    // Check if there's SAND/SPNG under the seed particle and empty space above
    int downPart = PMAPAT(x+dir3x3[down].X, y+dir3x3[down].Y);
    if ((TYP(downPart) == PT_SAND || TYP(downPart) == PT_SPNG) && !TYP(PMAPAT(x+dir3x3[up].X, y+dir3x3[up].Y))) {
      if (parts[i].life > 200) {
        // Start growth
        //@ SEED -> PLNT
        part_change_type(i, x, y, PT_PLNT);
        parts[i].ctype &= ~((7 << PLNT_DIR) | (3 << PLNT_PHASE)); // Clear direction and phase
        parts[i].ctype |= ((up & 7) << PLNT_DIR) | 1; // Set initial growth direction and growth bit
        parts[i].life = 15*water; // Make the first branch longer
        return 1;
      } else
        parts[i].life++;
    } else
      parts[i].life = 0; // Reset growth counter if we lost contact with ground
  }
  for (int rx = -1; rx <= 1; rx++)
    for (int ry = -1; ry <= 1; ry++)
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        switch (TYP(r)) {
          case PT_WATR: // Drink water
            if (water < 31) {
              kill_part(ID(r));
              water++;
              parts[i].life = 0;
            }
            break;
          case PT_DEUT: // Heavy water allows extra growth
            if (water < 255) {
              kill_part(ID(r));
              water++;
              parts[i].life = 0;
            }
            break;
          case PT_CBNW:
          case PT_DSTW: // Distilled or carbonated water is allowed only in very limited amount
            if (water < 3) {
              kill_part(ID(r));
              water++;
              parts[i].life = 0;
            }
            break;
          case PT_SLTW: // Salty water is harmful
            if (water > 0) {
              kill_part(ID(r));
              water--;
              parts[i].life = 0;
            }
            break;
          case PT_SPNG: // Allow absorbing some water from SPNG
            if (water < 15 && parts[ID(r)].life > 0) {
              parts[ID(r)].life--;
              water++;
              parts[i].life = 0;
            }
            break;
          case PT_SEED: // Breed with another seed
            if (water > 0 && !(parts[i].ctype & 1) && !(parts[ID(r)].ctype & 1) && rng_chance(1, 10)) {
              int ac = parts[i].ctype;
              int bc = parts[ID(r)].ctype;
              int old_tmpsi[4] = {parts[i].tmp, parts[i].tmp2, parts[i].tmp3, tmp4_get(i)};
              int old_tmpsr[4] = {parts[ID(r)].tmp, parts[ID(r)].tmp2, parts[ID(r)].tmp3, tmp4_get(ID(r))};
              int new_tmpsi[4] = {old_tmpsi[0], old_tmpsi[1], old_tmpsi[2], old_tmpsi[3]};
              int new_tmpsr[4] = {old_tmpsr[0], old_tmpsr[1], old_tmpsr[2], old_tmpsr[3]};
              // We set breeding bits, so these seeds will no longer breed with others
              parts[i].ctype = 1 | (parts[i].ctype & ~PLNT_CTYPE);
              parts[ID(r)].ctype = 1 | (parts[ID(r)].ctype & ~PLNT_CTYPE);
              // Inherit colour
              for(int ind = 0; ind < 3; ind++) {
                int bits[2][2];
                for(int kid = 0; kid < 2; kid++) {
                  // Decide which allele is chosen from the first parent
                  if (rng_chance(1, 2))
                    bits[kid][0] = (ac & (1 << (2*ind + PLNT_COLOUR))) != 0;
                  else
                    bits[kid][0] = (ac & (1 << (1 + 2*ind + PLNT_COLOUR))) != 0;
                  // ... from the second parent
                  if (rng_chance(1, 2))
                    bits[kid][1] = (bc & (1 << (2*ind + PLNT_COLOUR))) != 0;
                  else
                    bits[kid][1] = (bc & (1 << (1 + 2*ind + PLNT_COLOUR))) != 0;
                }
                // Set the corresponding genes
                parts[i].ctype |= bits[0][0] << (2*ind + PLNT_COLOUR);
                parts[i].ctype |= bits[0][1] << (1 + 2*ind + PLNT_COLOUR);
                parts[ID(r)].ctype |= bits[1][0] << (2*ind + PLNT_COLOUR);
                parts[ID(r)].ctype |= bits[1][1] << (1 + 2*ind + PLNT_COLOUR);
              }
              // Inherit the branching part of tmp - tmp4
              for(int ind = 0; ind < 5; ind++) {
                // Loop over tmp - tmp4
                for(int field = 0; field < 4; field++) {
                  new_tmpsi[field] |= rng_chance(1, 2) ?
                    (old_tmpsi[field] & (1 << ind)) : (old_tmpsr[field] & (1 << ind));
                  new_tmpsr[field] |= rng_chance(1, 2) ?
                    (old_tmpsi[field] & (1 << ind)) : (old_tmpsr[field] & (1 << ind));
                }
              }
              // Inherit the phases part of tmp - tmp4
              for(int ind = 5; ind < PLNT_TOTAL_TMP; ind += 2) {
                // Loop over tmp - tmp4
                for(int field = 0; field < 4; field++) {
                  new_tmpsi[field] |= rng_chance(1, 2) ?
                    (old_tmpsi[field] & (3 << ind)) : (old_tmpsr[field] & (3 << ind));
                  new_tmpsr[field] |= rng_chance(1, 2) ?
                    (old_tmpsi[field] & (3 << ind)) : (old_tmpsr[field] & (3 << ind));
                }
              }
              parts[i].tmp = (int16_t)new_tmpsi[0], parts[i].tmp2 = (int16_t)new_tmpsi[1], parts[i].tmp3 = (int16_t)new_tmpsi[2];
              tmp4_set(i, new_tmpsi[3]);
              parts[ID(r)].tmp = (int16_t)new_tmpsr[0], parts[ID(r)].tmp2 = (int16_t)new_tmpsr[1], parts[ID(r)].tmp3 = (int16_t)new_tmpsr[2];
              tmp4_set(ID(r), new_tmpsr[3]);
            }
            break;
          default:
            continue;
        }
      }
  // Update water counter
  parts[i].ctype &= ~(0xFF << PLNT_LIFE);
  parts[i].ctype |= (water & 0xFF) << PLNT_LIFE;
  return 0;
}
void el_SEED_create(ELEMENT_CREATE_FUNC_ARGS) {
  parts[i].ctype = 0x760;   /* 0b111011'000'00'0 */
  int down = Element_PLNT_detectDown(x, y); // Down gravity direction
  // Use random down direction if can't decide using gravity
  if (down < 0 || down > 7)
    down = rng_between(0, 7);
  int up = (down+4)%8; // Up gravity direction
  parts[i].ctype |= (up & 7) << PLNT_DIR;
  parts[i].tmp = 0;
  parts[i].tmp2 = 0;
  parts[i].tmp3 = 0;
  int tmp4 = 0;
  for(int ind = 0; ind < PLNT_TOTAL_TMP; ind++) {
    if(rng_chance(1, 2))
      parts[i].tmp |= (1<<ind);
    if(rng_chance(1, 2))
      parts[i].tmp2 |= (1<<ind);
    if(rng_chance(1, 2))
      parts[i].tmp3 |= (1<<ind);
    if(rng_chance(1, 2))
      tmp4 |= (1<<ind);
  }
  tmp4_set(i, tmp4);
}

