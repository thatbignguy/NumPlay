/* NumPowder: The Powder Toy's special elements (their files in elements/), ported: CLNE, BCLN, CONV, NBHL, NWHL,
 * PRTI, PRTO, LIFE, MORT, TRON. */
#include "simpriv.h"

/* ---------------------------------------------------------------- CLNE */
int el_CLNE_update(UPDATE_FUNC_ARGS) {
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
            rt!=PT_BCLN && rt!=PT_STKM &&
            rt!=PT_PBCN && rt!=PT_STKM2 &&
            rt<PT_NUM) {
          parts[i].ctype = rt;
          if (rt==PT_LIFE || rt==PT_LAVA)
            parts[i].tmp = parts[ID(r)].ctype;
        }
      }
    }
  } else {
    if (parts[i].ctype==PT_LIFE) create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_LIFE, parts[i].tmp);
    else if (parts[i].ctype!=PT_LIGH || rng_chance(1, 30)) {
      int np = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), TYP(parts[i].ctype), -1);
      if (np>=0) {
        if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && elements[parts[i].tmp].HighTemperatureTransition==PT_LAVA)
          parts[np].ctype = parts[i].tmp;
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- BCLN */
#define ADVECTION 0.1f
int el_BCLN_update(UPDATE_FUNC_ARGS) {
  if (!parts[i].life && pv[y/ACELL][x/ACELL]>4.0f)
    parts[i].life = rng_between(80, 119);
  if (parts[i].life) {
    p_set_vx(&parts[i], p_vx(&parts[i]) + (ADVECTION*vx[y/ACELL][x/ACELL]));
    p_set_vy(&parts[i], p_vy(&parts[i]) + (ADVECTION*vy[y/ACELL][x/ACELL]));
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
          rt!=PT_BCLN && rt!=PT_STKM &&
          rt!=PT_PBCN && rt!=PT_STKM2 &&
          rt<PT_NUM) {
          parts[i].ctype = rt;
          if (rt==PT_LIFE || rt==PT_LAVA)
            parts[i].tmp = parts[ID(r)].ctype;
        }
      }
    }
  } else {
    if (parts[i].ctype==PT_LIFE) create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), PT_LIFE, parts[i].tmp);
    else if (parts[i].ctype!=PT_LIGH || rng_chance(1, 30)) {
      int np = create_part(-1, x + rng_between(-1, 1), y + rng_between(-1, 1), TYP(parts[i].ctype), -1);
      if (np>=0) {
        if (parts[i].ctype==PT_LAVA && parts[i].tmp>0 && parts[i].tmp<PT_NUM && elements[parts[i].tmp].HighTemperatureTransition==PT_LAVA)
          parts[np].ctype = parts[i].tmp;
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- CONV */
int el_CONV_update(UPDATE_FUNC_ARGS) {
  int ctype = TYP(parts[i].ctype);
  if (ctype<=0 || ctype>=PT_NUM || !elements[ctype].Enabled || ctype==PT_CONV) {
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        int r = PHOTAT(x+rx, y+ry);
        if (!r)
          r = PMAPAT(x+rx, y+ry);
        if (!r)
          continue;
        int rt = TYP(r);
        if (rt != PT_CLNE && rt != PT_PCLN &&
            rt != PT_BCLN && rt != PT_STKM &&
            rt != PT_PBCN && rt != PT_STKM2 &&
            rt != PT_CONV && rt < PT_NUM) {
          parts[i].ctype = rt;
          if (rt == PT_LIFE)
            parts[i].ctype |= PMAPID(parts[ID(r)].ctype);
        }
      }
    }
  } else {
    int restrictElement = IsElement(parts[i].tmp) ? parts[i].tmp : 0;
    for (int rx = -1; rx <= 1; rx++) {
      for (int ry = -1; ry <= 1; ry++) {
        if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
          int r = PHOTAT(x+rx, y+ry);
          if (!r || (restrictElement && ((TYP(r) == restrictElement) == (parts[i].tmp2 == 1))))
            r = PMAPAT(x+rx, y+ry);
          if (!r || (restrictElement && ((TYP(r) == restrictElement) == (parts[i].tmp2 == 1))))
            continue;
          if (TYP(r) != PT_CONV && TYP(r) != PT_DMND && TYP(r) != ctype) {
            create_part(ID(r), x+rx, y+ry, TYP(parts[i].ctype), ID(parts[i].ctype));
          }
        }
      }
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- NBHL */
int el_NBHL_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp) {
    gravmass[(y )/ACELL][(x)/ACELL] += restrict_flt(0.001f * parts[i].tmp, 0.1f, 51.2f);
  } else {
    gravmass[(y )/ACELL][(x)/ACELL] += 0.1f;
  }
  return 0;
}

/* ---------------------------------------------------------------- NWHL */
int el_NWHL_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp) {
    gravmass[(y )/ACELL][(x)/ACELL] -= restrict_flt(0.001f * parts[i].tmp, 0.1f, 51.2f);
  } else {
    gravmass[(y )/ACELL][(x)/ACELL] -= 0.1f;
  }
  return 0;
}

/* ---------------------------------------------------------------- PRTI */
/* the orbits drawn around portals (orbitalparts.h): 4 distances in the original's 32-bit life, here in life (two)
 * and tmp2 (two), and 4 places in ctype */
static void orbitalparts_get(int i, int resblock1[], int resblock2[]) {
  uint32_t block1 = (uint16_t)parts[i].life | (uint32_t)(uint16_t)parts[i].tmp2 << 16, block2 = (uint32_t)parts[i].ctype;
  for (int k = 0; k < 4; k++) {
    resblock1[k] = (int)((block1 >> (8 * k)) & 0xFF);
    resblock2[k] = (int)((block2 >> (8 * k)) & 0xFF);
  }
}
static void orbitalparts_set(int i, int resblock1[], int resblock2[]) {
  uint32_t block1 = 0, block2 = 0;
  for (int k = 0; k < 4; k++) {
    block1 |= (uint32_t)(resblock1[k] & 0xFF) << (8 * k);
    block2 |= (uint32_t)(resblock2[k] & 0xFF) << (8 * k);
  }
  parts[i].life = (int16_t)(block1 & 0xFFFF);
  parts[i].tmp2 = (int16_t)(block1 >> 16);
  parts[i].ctype = (int32_t)block2;
}
static bool orbit_life_zero(int i) { return !parts[i].life && !parts[i].tmp2; }
static void orbit_life_set(int i, uint32_t v) {
  parts[i].life = (int16_t)(v & 0xFFFF);
  parts[i].tmp2 = (int16_t)(v >> 16);
}
/*these are the count values of where the particle gets stored, depending on where it came from
   0 1 2
   7 . 3
   6 5 4
   PRTO does (count+4)%8, so that it will come out at the opposite place to where it came in
   PRTO does +/-1 to the count, so it doesn't jam as easily
*/
int el_PRTI_update(UPDATE_FUNC_ARGS) {
  int fe = 0;
  parts[i].tmp = (int)((p_temp(&parts[i])-73.15f)/100+1);
  if (parts[i].tmp >= CHANNELS)
    parts[i].tmp = CHANNELS-1;
  else if (parts[i].tmp < 0)
    parts[i].tmp = 0;
  for (int count = 0; count < 8; count++) {
    int rx = portal_rx[count];
    int ry = portal_ry[count];
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r || TYP(r) == PT_STOR)
        fe = 1;
      if (!r || (!(elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY)) && TYP(r)!=PT_SPRK && TYP(r)!=PT_STOR)) {
        r = PHOTAT(x+rx, y+ry);
        if (!r)
          continue;
      }
      if (TYP(r)==PT_STKM || TYP(r)==PT_STKM2 || TYP(r)==PT_FIGH)
        continue;// Handling these is a bit more complicated, and is done in STKM_interact()
      if (TYP(r) == PT_SOAP)
        Element_SOAP_detach(ID(r));
      /* the first free place of the 80 of this channel and direction */
      if (portal_free(parts[i].tmp, count)) {
        if (TYP(r) == PT_STOR) {
          int stored = parts[ID(r)].tmp & 0xFF;
          if (IsElement(stored) && (elements[stored].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY))) {
            // STOR uses same format as PIPE, so we can use this function to do the transfer
            Particle p;
            memset(&p, 0, sizeof p);
            Element_PIPE_transfer_pipe_to_part(parts+(ID(r)), &p, true);
            portal_put(parts[i].tmp, count, &p);
          }
        } else {
          portal_put(parts[i].tmp, count, &parts[ID(r)]);
          if (TYP(r) == PT_SPRK)
            part_change_type(ID(r),x+rx,y+ry,parts[ID(r)].ctype);
          else
            kill_part(ID(r));
          fe = 1;
        }
      }
    }
  }
  if (fe) {
    int orbd[4] = {0, 0, 0, 0};  //Orbital distances
    int orbl[4] = {0, 0, 0, 0};  //Orbital locations
    if (orbit_life_zero(i)) orbit_life_set(i, rng_gen());
    if (!parts[i].ctype) parts[i].ctype = (int32_t)rng_gen();
    orbitalparts_get(i, orbd, orbl);
    for (int r = 0; r < 4; r++) {
      if (orbd[r]>1) {
        orbd[r] -= 12;
        if (orbd[r]<1) {
          orbd[r] = rng_between(128, 255);
          orbl[r] = rng_between(0, 254);
        } else {
          orbl[r] += 2;
          orbl[r] = orbl[r]%255;
        }
      } else {
        orbd[r] = rng_between(128, 255);
        orbl[r] = rng_between(0, 254);
      }
    }
    orbitalparts_set(i, orbd, orbl);
  } else {
    orbit_life_set(i, 0);
    parts[i].ctype = 0;
  }
  return 0;
}

/* ---------------------------------------------------------------- PRTO */
/*these are the count values of where the particle gets stored, depending on where it came from
  0 1 2
  7 . 3
  6 5 4
  PRTO does (count+4)%8, so that it will come out at the opposite place to where it came in
  PRTO does +/-1 to the count, so it doesn't jam as easily
*/
int el_PRTO_update(UPDATE_FUNC_ARGS) {
  int fe = 0;
  parts[i].tmp = (int)((p_temp(&parts[i])-73.15f)/100+1);
  if (parts[i].tmp>=CHANNELS) parts[i].tmp = CHANNELS-1;
  else if (parts[i].tmp<0) parts[i].tmp = 0;
  for (int count =0; count<8; count++) {
    int rx = portal_rx[count];
    int ry = portal_ry[count];
    if (rx || ry) {
      int r = PMAPAT(x+rx, y+ry);
      if (!r) {
        fe = 1;
        if (!portal_any(parts[i].tmp)) {
          for (int nnx = 0; nnx < 80; nnx++) rng_between(-1, 1);   /* (nothing to find, but the same random numbers) */
          continue;
        }
        for (int nnx =0 ; nnx<80; nnx++) {
          int randomness = (count + rng_between(-1, 1) + 4) % 8;//add -1,0,or 1 to count
          int slot = portal_find(parts[i].tmp, randomness, nnx);
          if (slot < 0)
            continue;
          Particle *pp = &portalp[slot].p;
          if (pp->type==PT_SPRK) { // TODO: make it look better, spark creation
            create_part(-1,x+1,y,PT_SPRK, -1);
            create_part(-1,x+1,y+1,PT_SPRK, -1);
            create_part(-1,x+1,y-1,PT_SPRK, -1);
            create_part(-1,x,y-1,PT_SPRK, -1);
            create_part(-1,x,y+1,PT_SPRK, -1);
            create_part(-1,x-1,y+1,PT_SPRK, -1);
            create_part(-1,x-1,y,PT_SPRK, -1);
            create_part(-1,x-1,y-1,PT_SPRK, -1);
            portal_take(slot);
            break;
          } else if (pp->type) {
            if (pp->type==PT_STKM)
              player.spwn = 0;
            if (pp->type==PT_STKM2)
              player2.spwn = 0;
            if (pp->type==PT_FIGH) {
              fighcount--;
              if ((unsigned char)pp->tmp < MAX_FIGHTERS) fighters[(unsigned char)pp->tmp].spwn = 0;
            }
            int np = create_part(-1, x+rx, y+ry, pp->type, -1);
            if (np<0) {
              if (pp->type==PT_STKM)
                player.spwn = 1;
              if (pp->type==PT_STKM2)
                player2.spwn = 1;
              if (pp->type==PT_FIGH) {
                fighcount++;
                if ((unsigned char)pp->tmp < MAX_FIGHTERS) fighters[(unsigned char)pp->tmp].spwn = 1;
              }
              continue;
            }
            if (parts[np].type==PT_FIGH) {
              // Release the fighters[] element allocated by create_part, the one reserved when the fighter went into the portal will be used
              if ((unsigned char)parts[np].tmp < MAX_FIGHTERS) fighters[(unsigned char)parts[np].tmp].spwn = 0;
              if ((unsigned char)pp->tmp < MAX_FIGHTERS) fighters[(unsigned char)pp->tmp].spwn = 1;
            }
            if (pp->vx == 0 && pp->vy == 0) {
              // particles that have passed from PIPE into PRTI have lost their velocity, so use the velocity of the newly created particle if the particle in the portal has no velocity
              int16_t tmp_vx = parts[np].vx, tmp_vy = parts[np].vy;
              parts[np] = *pp;
              parts[np].vx = tmp_vx;
              parts[np].vy = tmp_vy;
            } else
              parts[np] = *pp;
            p_set_x(&parts[np], (float)(x+rx));
            p_set_y(&parts[np], (float)(y+ry));
            portal_take(slot);
            break;
          }
        }
      }
    }
  }
  if (fe) {
    int orbd[4] = {0, 0, 0, 0};  //Orbital distances
    int orbl[4] = {0, 0, 0, 0};  //Orbital locations
    if (orbit_life_zero(i)) orbit_life_set(i, rng_gen());
    if (!parts[i].ctype) parts[i].ctype = (int32_t)rng_gen();
    orbitalparts_get(i, orbd, orbl);
    for (int r = 0; r < 4; r++) {
      if (orbd[r]<254) {
        orbd[r] += 16;
        if (orbd[r]>254) {
          orbd[r] = 0;
          orbl[r] = rng_between(0, 254);
        } else {
          orbl[r] += 1;
          orbl[r] = orbl[r]%255;
        }
      } else {
        orbd[r] = 0;
        orbl[r] = rng_between(0, 254);
      }
    }
    orbitalparts_set(i, orbd, orbl);
  } else {
    orbit_life_set(i, 0);
    parts[i].ctype = 0;
  }
  return 0;
}

/* ---------------------------------------------------------------- LIFE */
void el_LIFE_create(ELEMENT_CREATE_FUNC_ARGS) {
  if (v == -1)
    v = 0;
  // * 0x200000: No need to look for colours, they'll be set later anyway.
  bool skipLookup = v & 0x200000;
  v &= 0x1FFFFF;
  parts[i].ctype = v;
  if (v < NGOL) {
    /* (the first colour, the original's dcolour, isn't kept: it's builtinGol[ctype].colour; the second, the
     * original's 24-bit tmp, is kept in tmp as RGB565) */
    uint32_t c2 = builtinGol[v].colour2;
    parts[i].tmp = (int16_t)(((c2 >> 8) & 0xF800) | ((c2 >> 5) & 0x07E0) | ((c2 >> 3) & 0x001F));
    v = builtinGol[v].ruleset;
  } else if (!skipLookup) {
    /* (custom types' colours aren't kept) */
  }
  parts[i].tmp2 = ((v >> 17) & 0xF) + 1;
}

/* ---------------------------------------------------------------- MORT */
int el_MORT_update(UPDATE_FUNC_ARGS) {
  create_part(-1, x, y-1, PT_SMKE, -1);
  return 0;
}

/* ---------------------------------------------------------------- TRON */
/* TRON element is meant to resemble a tron bike (or worm) moving around and trying to avoid obstacles itself.
 * It has four direction each turn to choose from, 0 (left) 1 (up) 2 (right) 3 (down).
 * Each turn has a small random chance to randomly turn one way (so it doesn't do the exact same thing in a large room)
 * If the place it wants to move isn't a barrier, it will try and 'see' in front of itself to determine its safety.
 * For now the tron can only see its own body length in pixels ahead of itself (and around corners)
 *  - - - - - - - - - -
 *  - - - - + - - - - -
 *  - - - + + + - - - -
 *  - - +<--+-->+ - - -
 *  - +<----+---->+ - -
 *  - - - - H - - - - -
 * Where H is the head with tail length 4, it checks the + area to see if it can hit any of the edges, then it is called safe, or picks the biggest area if none safe.
 * .tmp bit values: 1st head, 2nd no tail growth, 3rd wait flag, 4th Nodie, 5th Dying, 6th & 7th is direction, 8th - 16th hue, 17th Norandom
 * .tmp2 is tail length (gets longer every few hundred frames)
 * .life is the timer that kills the end of the tail (the head uses life for how often it grows longer)
 * .ctype Contains the colour, lost on save, regenerated using hue tmp (bits 7 - 16)
 */
#define TRON_HEAD 0x00000001
#define TRON_NOGROW 0x00000002
#define TRON_WAIT 0x00000004 //it was just created, so WAIT a frame
#define TRON_NODIE 0x00000008
#define TRON_DEATH 0x00000010 //Crashed, now dying
/* the original's TRON_NORANDOM is tmp's bit 16: here ctype's bit 30 (ctype is copied to new heads, as the flag) */
#define TRON_NORANDOM_CT 0x40000000
static const int tron_rx[4] = {-1, 0, 1, 0};
static const int tron_ry[4] = { 0,-1, 0, 1};
static int TRON_trymovetron(int x, int y, int dir, int i, int len);
static bool TRON_canmovetron(int r, int len);
static int TRON_new_tronhead(int x, int y, int i, int direction);
int el_TRON_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp&TRON_WAIT) {
    parts[i].tmp &= ~TRON_WAIT;
    return 0;
  }
  if (parts[i].tmp&TRON_HEAD) {
    int firstdircheck = 0, seconddir = 0, seconddircheck = 0, lastdir = 0, lastdircheck = 0;
    int direction = (parts[i].tmp>>5 & 0x3);
    int originaldir = direction;
    //random turn
    int random = rng_between(0, 339);
    if ((random==1 || random==3) && !(parts[i].ctype & TRON_NORANDOM_CT)) {
      //randomly turn left(3) or right(1)
      direction = (direction + random)%4;
    }
    //check in front
    //do sight check
    firstdircheck = TRON_trymovetron(x,y,direction,i,parts[i].tmp2);
    if (firstdircheck < parts[i].tmp2) {
      if (parts[i].ctype & TRON_NORANDOM_CT) {
        seconddir = (direction + 1)%4;
        lastdir = (direction + 3)%4;
      } else if (originaldir != direction) { //if we just tried a random turn, don't pick random again
        seconddir = originaldir;
        lastdir = (direction + 2)%4;
      } else {
        seconddir = (direction + (rng_between(0, 1)*2)+1)% 4;
        lastdir = (seconddir + 2)%4;
      }
      seconddircheck = TRON_trymovetron(x,y,seconddir,i,parts[i].tmp2);
      lastdircheck = TRON_trymovetron(x,y,lastdir,i,parts[i].tmp2);
    }
    //find the best move
    if (seconddircheck > firstdircheck)
      direction = seconddir;
    if (lastdircheck > seconddircheck && lastdircheck > firstdircheck)
      direction = lastdir;
    //now try making new head, even if it fails
    if (TRON_new_tronhead(x + tron_rx[direction],y + tron_ry[direction],i,direction) == -1) {
      //ohgod crash
      parts[i].tmp |= TRON_DEATH;
      //trigger tail death for TRON_NODIE, or is that mode even needed? just set a high tail length(but it still won't start dying when it crashes)
    }
    //set own life and clear .tmp (it dies if it can't move anyway)
    parts[i].life = parts[i].tmp2;
    parts[i].tmp = (int16_t)(parts[i].tmp & (parts[i].tmp&0xF818));
  } else { // fade tail deco, or prevent tail from dying
    if (parts[i].tmp&TRON_NODIE)
      parts[i].life++;
  }
  return 0;
}
void el_TRON_create(ELEMENT_CREATE_FUNC_ARGS) {
  int randhue = rng_between(0, 359);
  int randomdir = rng_between(0, 3);
  // Set as a head and a direction
  parts[i].tmp = (int16_t)(1 | (randomdir << 5) | (randhue << 7));
  // Tail
  parts[i].tmp2 = 4;
  parts[i].life = 5;
}
static int TRON_new_tronhead(int x, int y, int i, int direction) {
  int np = create_part(-1, x , y ,PT_TRON, -1);
  if (np==-1)
    return -1;
  if (parts[i].life >= 100) { // increase tail length
    if (!(parts[i].tmp&TRON_NOGROW))
      parts[i].tmp2++;
    parts[i].life = 5;
  }
  //give new head our properties
  parts[np].tmp = (int16_t)(1 | direction<<5 | (parts[i].tmp&(TRON_NOGROW|TRON_NODIE)) | (parts[i].tmp&0xF800));
  if (np > i)
    parts[np].tmp |= TRON_WAIT;
  parts[np].ctype = parts[i].ctype;
  parts[np].tmp2 = parts[i].tmp2;
  parts[np].life = parts[i].life + 2;
  return 1;
}
static int TRON_trymovetron(int x, int y, int dir, int i, int len) {
  int k,j,r,rx,ry,tx,ty,count;
  count = 0;
  rx = x;
  ry = y;
  for (k = 1; k <= len; k ++) {
    rx += tron_rx[dir];
    ry += tron_ry[dir];
    if (ry >= BORDER && rx >= BORDER && ry < YRES-BORDER && rx < XRES-BORDER && (r = PMAPAT(rx, ry), TRON_canmovetron(r, k-1)) && !bmap[(ry)/WCELL][(rx)/WCELL]) {
      count++;
      for (tx = rx - tron_ry[dir] , ty = ry - tron_rx[dir], j=1; abs(tx-rx) < (len-k) && abs(ty-ry) < (len-k); tx-=tron_ry[dir],ty-=tron_rx[dir],j++) {
        if (ty >= BORDER && tx >= BORDER && ty < YRES-BORDER && tx < XRES-BORDER && (r = PMAPAT(tx, ty), TRON_canmovetron(r, j+k-1)) && !bmap[(ty)/WCELL][(tx)/WCELL]) {
          if (j == (len-k))//there is a safe path, so we can break out
            return len+1;
          count++;
        } else //we hit a block so no need to check farther here
          break;
      }
      for (tx = rx + tron_ry[dir] , ty = ry + tron_rx[dir], j=1; abs(tx-rx) < (len-k) && abs(ty-ry) < (len-k); tx+=tron_ry[dir],ty+=tron_rx[dir],j++) {
        if (ty >= BORDER && tx >= BORDER && ty < YRES-BORDER && tx < XRES-BORDER && (r = PMAPAT(tx, ty), TRON_canmovetron(r, j+k-1)) && !bmap[(ty)/WCELL][(tx)/WCELL]) {
          if (j == (len-k))
            return len+1;
          count++;
        } else
          break;
      }
    } else //a block infront, no need to continue
      break;
  }
  return count;
}
static bool TRON_canmovetron(int r, int len) {
  if (!r || (TYP(r) == PT_SWCH && parts[ID(r)].life >= 10) || (TYP(r) == PT_INVIS && parts[ID(r)].tmp2 == 1))
    return true;
  if ((((elements[TYP(r)].Properties & PROP_LIFE_KILL_DEC) && parts[ID(r)].life > 0)|| ((elements[TYP(r)].Properties & PROP_LIFE_KILL) && (elements[TYP(r)].Properties & PROP_LIFE_DEC))) && parts[ID(r)].life < len)
    return true;
  return false;
}

