/* NumPowder: The Powder Toy's force elements (their files in elements/), ported: ACEL, DCEL, RPEL, FRAY, PSTN, DMG. */
#include "simpriv.h"

/* ---------------------------------------------------------------- ACEL */
int el_ACEL_update(UPDATE_FUNC_ARGS) {
  float multiplier;
  if (parts[i].life!=0) {
    int change = parts[i].life > 1000 ? 1000 : (parts[i].life < 0 ? 0 : parts[i].life);
    multiplier = 1.0f+(change/100.0f);
  } else {
    multiplier = 1.1f;
  }
  parts[i].tmp = 0;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (!rx != !ry) {
        int r = PMAPAT(x+rx, y+ry);
        if(!r)
          r = PHOTAT(x+rx, y+ry);
        if (!r)
          continue;
        if(elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)) {
          float pvx = p_vx(&parts[ID(r)]);
          float pvy = p_vy(&parts[ID(r)]);
          pvx *= multiplier;
          pvy *= multiplier;
          // Restrict velocity and try to preserve direction
          float mv = fmaxf(fabsf(pvx), fabsf(pvy));
          if (mv > MAX_VELOCITY) {
            pvx *= MAX_VELOCITY/mv;
            pvy *= MAX_VELOCITY/mv;
          }
          p_set_vx(&parts[ID(r)], pvx);
          p_set_vy(&parts[ID(r)], pvy);
          parts[i].tmp = 1;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- DCEL */
int el_DCEL_update(UPDATE_FUNC_ARGS) {
  float multiplier = 1.0f/1.1f;
  if (parts[i].life!=0) {
    multiplier = 1.0f - ((parts[i].life > 100 ? 100 : (parts[i].life < 0 ? 0 : parts[i].life)) / 100.0f);
  }
  parts[i].tmp = 0;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if ((rx || ry) && !(rx && ry)) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          r = PHOTAT(x+rx, y+ry);
        if (!r)
          continue;
        if (elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)) {
          p_set_vx(&parts[ID(r)], p_vx(&parts[ID(r)]) * (multiplier));
          p_set_vy(&parts[ID(r)], p_vy(&parts[ID(r)]) * (multiplier));
          parts[i].tmp = 1;
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- RPEL */
int el_RPEL_update(UPDATE_FUNC_ARGS) {
  int r, rx, ry, ri;
  for(ri = 0; ri <= 10; ri++) {
    rx = rng_between(-10, 10);
    ry = rng_between(-10, 10);
    if (x+rx >= 0 && x+rx < XRES && y+ry >= 0 && y+ry < YRES && (rx || ry)) {
      r = PMAPAT(x+rx, y+ry);
      if (!r)
        r = PHOTAT(x+rx, y+ry);
      if (r && !(elements[TYP(r)].Properties & TYPE_SOLID)) {
        if (!parts[i].ctype || parts[i].ctype == parts[ID(r)].type) {
          p_set_vx(&parts[ID(r)], restrict_flt(p_vx(&parts[ID(r)]) + isign(rx)*((p_temp(&parts[i])-273.15)/10.0f), -MAX_VELOCITY, MAX_VELOCITY));
          p_set_vy(&parts[ID(r)], restrict_flt(p_vy(&parts[ID(r)]) + isign(ry)*((p_temp(&parts[i])-273.15)/10.0f), -MAX_VELOCITY, MAX_VELOCITY));
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- FRAY */
int el_FRAY_update(UPDATE_FUNC_ARGS) {
  int curlen;
  if (parts[i].tmp > 0)
    curlen = parts[i].tmp;
  else
    curlen = 10;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)==PT_SPRK) {
          for (int nxx = 0, nyy = 0, nxi = rx*-1, nyi = ry*-1, len = 0; ; nyy+=nyi, nxx+=nxi, len++) {
            if (!(x+nxi+nxx<XRES && y+nyi+nyy<YRES && x+nxi+nxx >= 0 && y+nyi+nyy >= 0) || len>curlen) {
              break;
            }
            r = PMAPAT(x+nxi+nxx, y+nyi+nyy);
            if (!r)
              r = PHOTAT(x+nxi+nxx, y+nyi+nyy);
            if (r && !(elements[TYP(r)].Properties & TYPE_SOLID)){
              float coef = (p_temp(&parts[i])-273.15f)/10.0f;
              p_set_vx(&parts[ID(r)], restrict_flt(p_vx(&parts[ID(r)]) + nxi*coef, -MAX_VELOCITY, MAX_VELOCITY));
              p_set_vy(&parts[ID(r)], restrict_flt(p_vy(&parts[ID(r)]) + nyi*coef, -MAX_VELOCITY, MAX_VELOCITY));
            }
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- PSTN */
typedef struct { int pushed, spaces; } StackData;
static StackData StackData_make(int pushed, int spaces) { return (StackData){pushed, spaces}; }
enum { PISTON_INACTIVE = 0x00, PISTON_RETRACT = 0x01, PISTON_EXTEND = 0x02, MAX_FRAME = 0x0F, DEFAULT_LIMIT = 0x1F,
       DEFAULT_ARM_LIMIT = 0xFF };
// tmp3 as a bitfield
enum {
  FLAG_EXT_OBSTACLE = 0x01, // xxx1 : Don't extend if blocked by obstacles
  FLAG_EXT_ARMLIMIT = 0x02, // xx1x : Don't extend if blocked by arm limit
  FLAG_RET_OBSTACLE = 0x04, // x1xx : Don't retract if frme hinders movement
  FLAG_RET_TOOSHORT = 0x08, // 1xxx : Don't retract if arm is shorter than retraction length
};
static StackData PSTN_CanMoveStack(int stackX, int stackY, int directionX, int directionY, int maxSize, int amount, bool retract, int block);
static int PSTN_MoveStack(int stackX, int stackY, int directionX, int directionY, int maxSize, int amount, bool retract, int block, bool sticky, int callDepth, bool cancelOnObstacle);
int el_PSTN_update(UPDATE_FUNC_ARGS) {
   if (parts[i].life)
     return 0;
   int maxSize = parts[i].tmp ? parts[i].tmp : DEFAULT_LIMIT;
   int armLimit = parts[i].tmp2 ? parts[i].tmp2 : DEFAULT_ARM_LIMIT;
   int state = PISTON_INACTIVE;
  for (int rx = -2; rx <= 2; rx++) {
    for (int ry = -2; ry <= 2; ry++) {
      if ((rx || ry) && (!rx || !ry)) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r) == PT_SPRK && parts[ID(r)].life == 3) {
          if (parts[ID(r)].ctype == PT_PSCN)
            state = PISTON_EXTEND;
          else
            state = PISTON_RETRACT;
        }
      }
    }
  }
  if (state == PISTON_EXTEND || state == PISTON_RETRACT) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if ((rx || ry) && (!rx || !ry)) {
          int r = PMAPAT(x+rx, y+ry);
          if (!r)
            continue;
          if (TYP(r) == PT_PSTN && !parts[ID(r)].life) {
            bool movedPiston = false;
            bool foundEnd = false;
            int pistonEndX, pistonEndY;
            int pistonCount = -1;// number of PSTN particles minus 1
            int armCount = 0;
            int directionX = rx;
            int directionY = ry;
            int nxi = directionX, nyi = directionY;
            for (int nxx = 0, nyy = 0; ; nyy += nyi, nxx += nxi) {
              if (!(x+nxx<XRES && y+nyy<YRES && x+nxx >= 0 && y+nyy >= 0)) {
                break;
              }
              r = PMAPAT(x+nxx, y+nyy);
              if (TYP(r) == PT_PSTN) {
                if (parts[ID(r)].life)
                  armCount++;
                else if (armCount) {
                  pistonEndX = x+nxx;
                  pistonEndY = y+nyy;
                  foundEnd = true;
                  break;
                } else {
                  pistonCount += (int)(floorf((p_temp(&parts[ID(r)]) - 268.15f) / 10));// How many tens of degrees above 0 C, rounded to nearest ten degrees. Can be negative.
                }
              } else if (nxx == 0 && nyy == 0) {
                // compatibility with BAD THINGS: starting PSTN layered underneath other particles
                // (in v90, it started scanning from the neighbouring particle, so could not break out of loop at offset=(0,0))
                pistonCount += (int)(floorf((p_temp(&parts[i]) - 268.15f) / 10));
                continue;
              } else {
                pistonEndX = x+nxx;
                pistonEndY = y+nyy;
                foundEnd = true;
                break;
              }
            }
            if (foundEnd) {
              if (state == PISTON_EXTEND) {
                if (armCount + pistonCount > armLimit) {
                  // If extending piston would exceed the armLimit, skip extension entirely
                  if (parts[i].tmp3 & FLAG_EXT_ARMLIMIT)
                    continue;
                  pistonCount = armLimit - armCount;
                }
                if (pistonCount > 0) {
                  bool cancelOnObstacle = parts[i].tmp3 & FLAG_EXT_OBSTACLE;
                  int newSpace = PSTN_MoveStack(pistonEndX, pistonEndY, directionX, directionY, maxSize, pistonCount, false, parts[i].ctype, true, 0, cancelOnObstacle);
                  if (newSpace) {
                    // Create new piston section
                    for (int j = 0; j < newSpace; j++) {
                      int nr = create_part(-3, pistonEndX + (nxi * j), pistonEndY + (nyi * j), PT_PSTN, -1);
                      if (nr > -1) {
                        parts[nr].life = 1;
                      }
                    }
                    movedPiston =  true;
                  }
                }
              } else if (state == PISTON_RETRACT) {
                if (pistonCount > armCount) {
                  // Arm too short to retract the full amount
                  if (parts[i].tmp3 & FLAG_RET_TOOSHORT)
                    continue;
                  pistonCount = armCount;
                }
                if (armCount && pistonCount > 0) {
                  bool cancelOnObstacle = parts[i].tmp3 & FLAG_RET_OBSTACLE; // Only retract if no obstacles hinder frme movement
                  PSTN_MoveStack(pistonEndX, pistonEndY, directionX, directionY,
                    maxSize, pistonCount, true, parts[i].ctype, true, 0, cancelOnObstacle);
                  movedPiston = true;
                }
              }
            }
            if (movedPiston)
              return 0;
          }
        }
      }
    }
  }
  return 0;
}
static StackData PSTN_CanMoveStack(int stackX, int stackY, int directionX, int directionY, int maxSize, int amount, bool retract, int block) {
  int posX, posY, spaces = 0, currentPos = 0;
  if (amount <= 0)
    return StackData_make(0, 0);
  for (posX = stackX, posY = stackY; currentPos < maxSize + amount && currentPos < XRES - 1; posX += directionX, posY += directionY) {
    if (!(posX < XRES && posY < YRES && posX >= 0 && posY >= 0))
      break;
    int r = PMAPAT(posX, posY);
    if (IsWallBlocking(posX, posY, 0) || (block && TYP(r) == block))
      return StackData_make(currentPos - spaces, spaces);
    if (!r) {
      spaces++;
      Element_PSTN_tempParts[currentPos++] = -1;   /* (100 at most here: currentPos < XRES - 1) */
      if (spaces >= amount)
        break;
    } else {
      if (currentPos - spaces < maxSize && (!retract || (TYP(r) == PT_FRME && posX == stackX && posY == stackY)))
        Element_PSTN_tempParts[currentPos++] = ID(r);
      else
        return StackData_make(currentPos - spaces, spaces);
    }
  }
  return StackData_make(currentPos - spaces, spaces);
}
static int PSTN_MoveStack(int stackX, int stackY, int directionX, int directionY, int maxSize, int amount, bool retract, int block, bool sticky, int callDepth, bool cancelOnObstacle) {
  int posX, posY, r;
  r = PMAPAT(stackX, stackY);
  if (!callDepth && TYP(r) == PT_FRME) {
    int newY = !!directionX, newX = !!directionY;
    int realDirectionX = retract ? -directionX : directionX;
    int realDirectionY = retract ? -directionY : directionY;
    int maxRight = MAX_FRAME, maxLeft = MAX_FRAME;
    //check if we can push all the FRME
    for (int c = retract; c < MAX_FRAME; c++) {
      posY = stackY + (c * newY);
      posX = stackX + (c * newX);
      if (posX < XRES && posY < YRES && posX >= 0 && posY >= 0 && TYP(PMAPAT(posX, posY)) == PT_FRME) {
        int spaces = PSTN_CanMoveStack(posX, posY, realDirectionX, realDirectionY, maxSize, amount, retract, block).spaces;
        if (spaces < amount) {
          // Obstacle encountered, tmp3 flags are set such that shortened movement is not allowed, abort before moving anything
          if (cancelOnObstacle)
            return 0;
          amount = spaces;
        }
      } else {
        maxRight = c;
        break;
      }
    }
    for (int c = 1; c < MAX_FRAME; c++) {
      posY = stackY - (c*newY);
      posX = stackX - (c*newX);
      if (posX < XRES && posY < YRES && posX >= 0 && posY >= 0 && TYP(PMAPAT(posX, posY)) == PT_FRME) {
        int spaces = PSTN_CanMoveStack(posX, posY, realDirectionX, realDirectionY, maxSize, amount, retract, block).spaces;
        if (spaces < amount) {
          // Obstacle encountered, tmp3 flags are set such that shortened movement is not allowed, abort before moving anything
          if (cancelOnObstacle)
            return 0;
          amount = spaces;
        }
      } else {
        maxLeft = c;
        break;
      }
    }
    //If the piston is pushing frame, iterate out from the centre to the edge and push everything resting on frame
    for (int c = 1; c < maxRight; c++) {
      posY = stackY + (c*newY);
      posX = stackX + (c*newX);
      PSTN_MoveStack(posX, posY, directionX, directionY, maxSize, amount, retract, block, !parts[ID(PMAPAT(posX, posY))].tmp, 1, cancelOnObstacle);
    }
    for (int c = 1; c < maxLeft; c++) {
      posY = stackY - (c*newY);
      posX = stackX - (c*newX);
      PSTN_MoveStack(posX, posY, directionX, directionY, maxSize, amount, retract, block, !parts[ID(PMAPAT(posX, posY))].tmp, 1, cancelOnObstacle);
    }
    //Remove arm section if retracting with FRME
    if (retract)
      for (int j = 1; j <= amount; j++)
        kill_part(ID(PMAPAT(stackX+(directionX * -j), stackY+(directionY * -j))));
    return PSTN_MoveStack(stackX, stackY, directionX, directionY, maxSize, amount, retract, block, !parts[ID(PMAPAT(stackX, stackY))].tmp, 1, cancelOnObstacle);
  }
  if (retract) {
    bool foundParts = false;
    //Remove arm section if retracting without FRME
    if (!callDepth)
      for (int j = 1; j <= amount; j++)
        kill_part(ID(PMAPAT(stackX + (directionX * -j), stackY + (directionY * -j))));
    int currentPos = 0;
    for (posX = stackX, posY = stackY; currentPos < maxSize && currentPos < XRES-1; posX += directionX, posY += directionY) {
      if (!(posX < XRES && posY < YRES && posX >= 0 && posY >= 0)) {
        break;
      }
      r = PMAPAT(posX, posY);
      if (!r || TYP(r) == block || (!sticky && TYP(r) != PT_FRME)) {
        break;
      } else {
        foundParts = true;
        Element_PSTN_tempParts[currentPos++] = ID(r);
      }
    }
    if (foundParts) {
      //Move particles
      for (int j = 0; j < currentPos; j++) {
        int jP = Element_PSTN_tempParts[j];
        int srcX = (int)(p_x(&parts[jP]) + 0.5f), srcY = (int)(p_y(&parts[jP]) + 0.5f);
        int destX = srcX-directionX*amount, destY = srcY-directionY*amount;
        PMAPSET(srcX, srcY, 0);
        p_set_x(&parts[jP], (float)(destX));
        p_set_y(&parts[jP], (float)(destY));
        PMAPSET(destX, destY, PMAP(jP, parts[jP].type));
      }
      return amount;
    }
  } else {
    StackData stackData = PSTN_CanMoveStack(stackX, stackY, directionX, directionY, maxSize, amount, retract, block);
    int currentPos = stackData.pushed + stackData.spaces;
    if (currentPos) {
      //Move particles
      int possibleMovement = 0;
      for (int j = currentPos-1; j >= 0; j--) {
        int jP = Element_PSTN_tempParts[j];
        if (jP < 0) {
          possibleMovement++;
          continue;
        }
        if (!possibleMovement)
          continue;
        int srcX = (int)(p_x(&parts[jP]) + 0.5f), srcY = (int)(p_y(&parts[jP]) + 0.5f);
        int destX = srcX+directionX*possibleMovement, destY = srcY+directionY*possibleMovement;
        PMAPSET(srcX, srcY, 0);
        p_set_x(&parts[jP], (float)(destX));
        p_set_y(&parts[jP], (float)(destY));
        PMAPSET(destX, destY, PMAP(jP, parts[jP].type));
      }
      return possibleMovement;
    }
  }
  return 0;
}
bool el_PSTN_ctypeDraw(CTYPEDRAW_FUNC_ARGS) {
  if (t == PT_FRME) {
    return false;
  }
  return basicCtypeDraw(CTYPEDRAW_FUNC_SUBCALL_ARGS);
}

/* ---------------------------------------------------------------- DMG */
int el_DMG_update(UPDATE_FUNC_ARGS) {
  int rad = 25;
  for (int rx = -1; rx <= 1; rx++) {
    for (int ry = -1; ry <= 1; ry++) {
      if (rx || ry) {
        int r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        if (TYP(r)!=PT_DMG && TYP(r)!=PT_EMBR && TYP(r)!=PT_DMND && TYP(r)!=PT_CLNE && TYP(r)!=PT_PCLN && TYP(r)!=PT_BCLN) {
          kill_part(i);
          for (int nxj =-rad; nxj<=rad; nxj++) {
            for (int nxi =-rad; nxi<=rad; nxi++) {
              if (x+nxi>=0 && y+nxj>=0 && x+nxi<XRES && y+nxj<YRES && (nxi || nxj)) {
                int dist = (int)(sqrtf(powf(nxi, 2.0f)+powf(nxj, 2.0f)));//;(powf((float)nxi,2))/(powf((float)rad,2))+(powf((float)nxj,2))/(powf((float)rad,2));
                if (!dist || (dist <= rad)) {
                  int rr = PMAPAT(x+nxi, y+nxj);
                  if (rr) {
                    float angle = atan2f((float)nxj, nxi);
                    float fx = cosf(angle) * 7.0f;
                    float fy = sinf(angle) * 7.0f;
                    p_set_vx(&parts[ID(rr)], p_vx(&parts[ID(rr)]) + (fx));
                    p_set_vy(&parts[ID(rr)], p_vy(&parts[ID(rr)]) + (fy));
                    vx[(y+nxj)/ACELL][(x+nxi)/ACELL] += fx;
                    vy[(y+nxj)/ACELL][(x+nxi)/ACELL] += fy;
                    pv[(y+nxj)/ACELL][(x+nxi)/ACELL] += 1.0f;
                    int t = TYP(rr);
                    if (t && elements[t].HighPressureTransition>-1 && elements[t].HighPressureTransition<PT_NUM)
                      part_change_type(ID(rr), x+nxi, y+nxj, elements[t].HighPressureTransition);
                    else if (t == PT_BMTL) //@ DMG + BMTL -> BRMT
                      part_change_type(ID(rr), x+nxi, y+nxj, PT_BRMT);
                    else if (t == PT_GLAS) //@ DMG + GLAS -> BGLA
                      part_change_type(ID(rr), x+nxi, y+nxj, PT_BGLA);
                    else if (t == PT_COAL) //@ DMG + COAL -> BCOL
                      part_change_type(ID(rr), x+nxi, y+nxj, PT_BCOL);
                    else if (t == PT_QRTZ) //@ DMG + QRTZ -> PQRT
                      part_change_type(ID(rr), x+nxi, y+nxj, PT_PQRT);
                    else if (t == PT_TUNG) { //@ DMG + TUNG -> BRMT(TUNG)
                      part_change_type(ID(rr), x+nxi, y+nxj, PT_BRMT);
                      parts[ID(rr)].ctype = PT_TUNG;
                    } else if (t == PT_WOOD) //@ DMG + WOOD -> SAWD
                      part_change_type(ID(rr), x+nxi, y+nxj, PT_SAWD);
                  }
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

