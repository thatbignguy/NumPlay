/* NumPowder: The Powder Toy's stickmen (elements/STKM.cpp, STKM2.cpp, FIGH.cpp), ported. The original keeps 100
 * fighters; MAX_FIGHTERS here. */
#include "simpriv.h"

/* ---------------------------------------------------------------- STKM */
static void STKM_die(playerst *playerp, int i);
static void FIGH_Free(unsigned char i);
static inline bool InWallBounds(int x, int y) { return x >= 0 && y >= 0 && x < XWCELLS && y < YWCELLS; }
int el_STKM_update(UPDATE_FUNC_ARGS) {
  Element_STKM_run_stickman(&player, UPDATE_FUNC_SUBCALL_ARGS);
  return 0;
}
void el_STKM_create(ELEMENT_CREATE_FUNC_ARGS) {
  int spawnID = create_part(-3, x, y, PT_SPAWN, -1);
  if (spawnID >= 0)
    player.spawnID = spawnID;
}
bool el_STKM_createAllowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS) {
  return elementCount[PT_STKM] <= 0 && !player.spwn;
}
void el_STKM_changeType(ELEMENT_CHANGETYPE_FUNC_ARGS) {
  if (to == PT_STKM) {
    Element_STKM_init_legs(&player, i);
    player.spwn = 1;
  } else
    player.spwn = 0;
}
static void STKM_die(playerst *playerp, int i) {
  if (playerp->fan) {
    int x = (int)(p_x(&parts[i]) + 0.5f) / ACELL;
    int y = (int)(p_y(&parts[i]) + 0.5f) / ACELL;
    pv[y][x] += 64;
  } else {
    int x = (int)(p_x(&parts[i]) + 0.5f);
    int y = (int)(p_y(&parts[i]) + 0.5f);
    for (int r = -2; r <= 1; r++) {
      create_part(-1, x + r, y - 2, playerp->elem, -1);
      create_part(-1, x + r + 1, y + 2, playerp->elem, -1);
      create_part(-1, x - 2, y + r + 1, playerp->elem, -1);
      create_part(-1, x + 2, y + r, playerp->elem, -1);
    }
  }
  kill_part(i);  //Kill him
}
int Element_STKM_run_stickman(playerst *playerp, UPDATE_FUNC_ARGS) {
  int r, rx, ry;
  int t = parts[i].type;
  float pp, d;
  float dt = 0.9f;///(FPSB*FPSB);  //Delta time in square
  float gvx, gvy;
  float gx, gy, dl, dr;
  float rocketBootsHeadEffect = 0.35f;
  float rocketBootsFeetEffect = 0.15f;
  float rocketBootsHeadEffectV = 0.3f;// stronger acceleration vertically, to counteract gravity
  float rocketBootsFeetEffectV = 0.45f;
  if (!playerp->fan && parts[i].ctype && IsElementOrNone(parts[i].ctype))
    Element_STKM_set_element(playerp, parts[i].ctype);
  playerp->frames++;
  //Temperature handling
  if (p_temp(&parts[i])<243)
    parts[i].life -= 1;
  if ((p_temp(&parts[i])<309.6f) && (p_temp(&parts[i])>=243))
    p_set_temp(&parts[i], p_temp(&parts[i]) + (1));
  //Death
  if (parts[i].life<1 || (pv[y/ACELL][x/ACELL]>=4.5f && !playerp->fan) ) { //If his HP is less than 0 or there is very big wind...
    STKM_die(playerp, i);
    return 1;
  }
  //Follow gravity
  gvx = gvy = 0.0f;
  switch (gravityMode) {
    default:
    case GRAV_VERTICAL:
      gvy = 1;
      break;
    case GRAV_OFF:
      gvy = gvx = 0.0f;
      break;
    case GRAV_RADIAL: {
        float gravd;
        gravd = 0.01f - hypotf((p_x(&parts[i]) - XCNTR), (p_y(&parts[i]) - YCNTR));
        gvx = ((float)(p_x(&parts[i]) - XCNTR) / gravd);
        gvy = ((float)(p_y(&parts[i]) - YCNTR) / gravd);
      }
      break;
    case GRAV_CUSTOM:
      gvx = customGravityX;
      gvy = customGravityY;
      break;
  }
  gvx += gravx[((int)(p_y(&parts[i])) )/ACELL][((int)(p_x(&parts[i])))/ACELL];
  gvy += gravy[((int)(p_y(&parts[i])) )/ACELL][((int)(p_x(&parts[i])))/ACELL];
  float mvx = gvx;
  float mvy = gvy;
  bool rbLowGrav = false;
  float tmp = fabsf(mvx) > fabsf(mvy)?fabsf(mvx):fabsf(mvy);
  if (tmp < 0.001f) {
    rbLowGrav = true;
    mvx = -p_vx(&parts[i]);
    mvy = -p_vy(&parts[i]);
    tmp = fabsf(mvx) > fabsf(mvy)?fabsf(mvx):fabsf(mvy);
  }
  if (tmp < 0.001f) {
    mvx = 0;
    mvy = 1.0f;
    tmp = 1.0f;
  } else
    tmp = 1.0f/sqrtf(mvx*mvx+mvy*mvy);
  mvx *= tmp;// scale to a unit vector
  mvy *= tmp;
  if (rbLowGrav) {
    rocketBootsHeadEffectV = rocketBootsHeadEffect;
    rocketBootsFeetEffectV = rocketBootsFeetEffect;
  }
  p_set_vx(&parts[i], p_vx(&parts[i]) - (gvx*dt));  //Head up!
  p_set_vy(&parts[i], p_vy(&parts[i]) - (gvy*dt));
  //Verlet integration
  pp = 2*playerp->legs[0]-playerp->legs[2]+playerp->accs[0]*dt*dt;
  playerp->legs[2] = playerp->legs[0];
  playerp->legs[0] = pp;
  pp = 2*playerp->legs[1]-playerp->legs[3]+playerp->accs[1]*dt*dt;
  playerp->legs[3] = playerp->legs[1];
  playerp->legs[1] = pp;
  pp = 2*playerp->legs[4]-playerp->legs[6]+(playerp->accs[2]+gvx)*dt*dt;
  playerp->legs[6] = playerp->legs[4];
  playerp->legs[4] = pp;
  pp = 2*playerp->legs[5]-playerp->legs[7]+(playerp->accs[3]+gvy)*dt*dt;
  playerp->legs[7] = playerp->legs[5];
  playerp->legs[5] = pp;
  pp = 2*playerp->legs[8]-playerp->legs[10]+playerp->accs[4]*dt*dt;
  playerp->legs[10] = playerp->legs[8];
  playerp->legs[8] = pp;
  pp = 2*playerp->legs[9]-playerp->legs[11]+playerp->accs[5]*dt*dt;
  playerp->legs[11] = playerp->legs[9];
  playerp->legs[9] = pp;
  pp = 2*playerp->legs[12]-playerp->legs[14]+(playerp->accs[6]+gvx)*dt*dt;
  playerp->legs[14] = playerp->legs[12];
  playerp->legs[12] = pp;
  pp = 2*playerp->legs[13]-playerp->legs[15]+(playerp->accs[7]+gvy)*dt*dt;
  playerp->legs[15] = playerp->legs[13];
  playerp->legs[13] = pp;
  //Setting acceleration to 0
  playerp->accs[0] = 0;
  playerp->accs[1] = 0;
  playerp->accs[2] = 0;
  playerp->accs[3] = 0;
  playerp->accs[4] = 0;
  playerp->accs[5] = 0;
  playerp->accs[6] = 0;
  playerp->accs[7] = 0;
  gx = (playerp->legs[4] + playerp->legs[12])/2 - gvy;
  gy = (playerp->legs[5] + playerp->legs[13])/2 + gvx;
  dl = powf(gx - playerp->legs[4], 2) + powf(gy - playerp->legs[5], 2);
  dr = powf(gx - playerp->legs[12], 2) + powf(gy - playerp->legs[13], 2);
  //Go left
  if (((int)(playerp->comm)&0x01) == 0x01) {
    bool moved = false;
    if (dl>dr) {
      if (InBounds((int)(playerp->legs[4]), (int)(playerp->legs[5])) && !eval_move(t, (int)(playerp->legs[4]), (int)(playerp->legs[5]), NULL)) {
        playerp->accs[2] = -3*mvy-3*mvx;
        playerp->accs[3] = 3*mvx-3*mvy;
        playerp->accs[0] = -mvy;
        playerp->accs[1] = mvx;
        moved = true;
      }
    } else {
      if (InBounds((int)(playerp->legs[12]), (int)(playerp->legs[13])) && !eval_move(t, (int)(playerp->legs[12]), (int)(playerp->legs[13]), NULL)) {
        playerp->accs[6] = -3*mvy-3*mvx;
        playerp->accs[7] = 3*mvx-3*mvy;
        playerp->accs[0] = -mvy;
        playerp->accs[1] = mvx;
        moved = true;
      }
    }
    if (!moved && playerp->rocketBoots) {
      p_set_vx(&parts[i], p_vx(&parts[i]) - (rocketBootsHeadEffect*mvy));
      p_set_vy(&parts[i], p_vy(&parts[i]) + (rocketBootsHeadEffect*mvx));
      playerp->accs[2] -= rocketBootsFeetEffect*mvy;
      playerp->accs[6] -= rocketBootsFeetEffect*mvy;
      playerp->accs[3] += rocketBootsFeetEffect*mvx;
      playerp->accs[7] += rocketBootsFeetEffect*mvx;
      for (int leg=0; leg<2; leg++) {
        if (leg==1 && (((int)(playerp->comm)&0x02) == 0x02))
          continue;
        int footX = (int)(playerp->legs[leg*8+4]), footY = (int)(playerp->legs[leg*8+5]);
        int np = create_part(-1, footX, footY, PT_PLSM, -1);
        if (np>=0) {
          p_set_vx(&parts[np], p_vx(&parts[i])+mvy*25);
          p_set_vy(&parts[np], p_vy(&parts[i])-mvx*25);
          parts[np].life += 30;
        }
      }
    }
  }
  //Go right
  if (((int)(playerp->comm)&0x02) == 0x02) {
    bool moved = false;
    if (dl<dr) {
      if (InBounds((int)(playerp->legs[4]), (int)(playerp->legs[5])) && !eval_move(t, (int)(playerp->legs[4]), (int)(playerp->legs[5]), NULL)) {
        playerp->accs[2] = 3*mvy-3*mvx;
        playerp->accs[3] = -3*mvx-3*mvy;
        playerp->accs[0] = mvy;
        playerp->accs[1] = -mvx;
        moved = true;
      }
    } else {
      if (InBounds((int)(playerp->legs[12]), (int)(playerp->legs[13])) && !eval_move(t, (int)(playerp->legs[12]), (int)(playerp->legs[13]), NULL)) {
        playerp->accs[6] = 3*mvy-3*mvx;
        playerp->accs[7] = -3*mvx-3*mvy;
        playerp->accs[0] = mvy;
        playerp->accs[1] = -mvx;
        moved = true;
      }
    }
    if (!moved && playerp->rocketBoots) {
      p_set_vx(&parts[i], p_vx(&parts[i]) + (rocketBootsHeadEffect*mvy));
      p_set_vy(&parts[i], p_vy(&parts[i]) - (rocketBootsHeadEffect*mvx));
      playerp->accs[2] += rocketBootsFeetEffect*mvy;
      playerp->accs[6] += rocketBootsFeetEffect*mvy;
      playerp->accs[3] -= rocketBootsFeetEffect*mvx;
      playerp->accs[7] -= rocketBootsFeetEffect*mvx;
      for (int leg=0; leg<2; leg++) {
        if (leg==0 && (((int)(playerp->comm)&0x01) == 0x01))
          continue;
        int footX = (int)(playerp->legs[leg*8+4]), footY = (int)(playerp->legs[leg*8+5]);
        int np = create_part(-1, footX, footY, PT_PLSM, -1);
        if (np>=0) {
          p_set_vx(&parts[np], p_vx(&parts[i])-mvy*25);
          p_set_vy(&parts[np], p_vy(&parts[i])+mvx*25);
          parts[np].life += 30;
        }
      }
    }
  }
  if (playerp->rocketBoots && ((int)(playerp->comm)&0x03) == 0x03) {
    // Pressing left and right simultaneously with rocket boots on slows the stickman down
    // Particularly useful in zero gravity
    p_set_vx(&parts[i], p_vx(&parts[i]) * (0.5f));
    p_set_vy(&parts[i], p_vy(&parts[i]) * (0.5f));
    playerp->accs[2] = playerp->accs[6] = 0;
    playerp->accs[3] = playerp->accs[7] = 0;
  }
  //Jump
  if (((int)(playerp->comm)&0x04) == 0x04) {
    if (playerp->rocketBoots) {
      p_set_vx(&parts[i], p_vx(&parts[i]) - (rocketBootsHeadEffectV*mvx));
      p_set_vy(&parts[i], p_vy(&parts[i]) - (rocketBootsHeadEffectV*mvy));
      playerp->accs[2] -= rocketBootsFeetEffectV*mvx;
      playerp->accs[6] -= rocketBootsFeetEffectV*mvx;
      playerp->accs[3] -= rocketBootsFeetEffectV*mvy;
      playerp->accs[7] -= rocketBootsFeetEffectV*mvy;
      for (int leg=0; leg<2; leg++) {
        int footX = (int)(playerp->legs[leg*8+4]), footY = (int)(playerp->legs[leg*8+5]);
        int np = create_part(-1, footX, footY+1, PT_PLSM, -1);
        if (np>=0) {
          p_set_vx(&parts[np], p_vx(&parts[i])+mvx*30);
          p_set_vy(&parts[np], p_vy(&parts[i])+mvy*30);
          parts[np].life += 10;
        }
      }
    } else if ((InBounds((int)(playerp->legs[4]), (int)(playerp->legs[5])) && !eval_move(t, (int)(playerp->legs[4]), (int)(playerp->legs[5]), NULL)) ||
         (InBounds((int)(playerp->legs[12]), (int)(playerp->legs[13])) && !eval_move(t, (int)(playerp->legs[12]), (int)(playerp->legs[13]), NULL))) {
      p_set_vx(&parts[i], p_vx(&parts[i]) - (4*mvx));
      p_set_vy(&parts[i], p_vy(&parts[i]) - (4*mvy));
      playerp->accs[2] -= mvx;
      playerp->accs[6] -= mvx;
      playerp->accs[3] -= mvy;
      playerp->accs[7] -= mvy;
    }
  }
  //Charge detector wall if foot inside
  if (InWallBounds((int)(playerp->legs[4]+0.5f)/WCELL, (int)(playerp->legs[5]+0.5f)/WCELL) &&
         bmap[(int)(playerp->legs[5]+0.5f)/WCELL][(int)(playerp->legs[4]+0.5f)/WCELL]==WL_DETECT)
    set_emap((int)playerp->legs[4]/WCELL, (int)playerp->legs[5]/WCELL);
  if (InWallBounds((int)(playerp->legs[12]+0.5f)/WCELL, (int)(playerp->legs[13]+0.5f)/WCELL) &&
          bmap[(int)(playerp->legs[13]+0.5f)/WCELL][(int)(playerp->legs[12]+0.5f)/WCELL]==WL_DETECT)
    set_emap((int)(playerp->legs[12]+0.5f)/WCELL, (int)(playerp->legs[13]+0.5f)/WCELL);
  //Searching for particles near head
  for (rx=-2; rx<3; rx++)
    for (ry=-2; ry<3; ry++)
      if (x+rx>=0 && y+ry>0 && x+rx<XRES && y+ry<YRES && (rx || ry)) {
        r = PMAPAT(x+rx, y+ry);
        if (!r)
          r = PHOTAT(x+rx, y+ry);
        if (!r && !bmap[(y+ry)/WCELL][(x+rx)/WCELL])
          continue;
        Element_STKM_set_element(playerp, TYP(r));
        if (TYP(r) == PT_PLNT && parts[i].life<100) { //Plant gives him 5 HP
          if (parts[i].life<=95)
            parts[i].life += 5;
          else
            parts[i].life = 100;
          kill_part(ID(r));
        }
        if (TYP(r) == PT_NEUT) {
          if (parts[i].life<=100) parts[i].life -= (102-parts[i].life)/2;
          else parts[i].life = (int)(parts[i].life * 0.9f);
          kill_part(ID(r));
        }
        if (bmap[(ry+y)/WCELL][(rx+x)/WCELL]==WL_FAN)
          playerp->fan = true;
        else if (bmap[(ry+y)/WCELL][(rx+x)/WCELL]==WL_EHOLE)
          playerp->rocketBoots = false;
        else if (bmap[(ry+y)/WCELL][(rx+x)/WCELL]==WL_GRAV /* && parts[i].type!=PT_FIGH */)
          playerp->rocketBoots = true;
        if (TYP(r)==PT_PRTI)
          Element_STKM_interact(playerp, i, rx, ry);
        if (!parts[i].type)//STKM_interact may kill STKM
          return 1;
      }
  //Head position
  rx = x + 3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01);
  ry = y - 3*(playerp->pcomm == 0);
  //Spawn
  if (((int)(playerp->comm)&0x08) == 0x08) {
    ry -= 2 * rng_between(0, 1) + 1;
    /* (the head near the edge: the place can be off the grid, where there's nothing, and nothing is made) */
    r = InBounds(rx, ry) ? PMAPAT(rx, ry) : 0;
    if (elements[TYP(r)].Properties&TYPE_SOLID) {
      create_part(-1, rx, ry, PT_SPRK, -1);
      playerp->frames = 0;
    } else {
      int np = -1;
      if (playerp->fan) {
        for(int j = -4; j < 5; j++)
          for (int k = -4; k < 5; k++) {
            int airx = rx + 3*((((int)playerp->pcomm)&0x02) == 0x02) - 3*((((int)playerp->pcomm)&0x01) == 0x01)+j;
            int airy = ry+k;
            if (airx < 0 || airy < 0 || airx >= XRES || airy >= YRES)
              continue;   /* (the original writes outside the air there) */
            pv[airy/ACELL][airx/ACELL] += 0.03f;
            if (airy + ACELL < YRES)
              pv[airy/ACELL+1][airx/ACELL] += 0.03f;
            if (airx + ACELL < XRES) {
              pv[airy/ACELL][airx/ACELL+1] += 0.03f;
              if (airy + ACELL < YRES)
                pv[airy/ACELL+1][airx/ACELL+1] += 0.03f;
            }
          }
      } else if (playerp->elem==PT_LIGH && playerp->frames<30)//limit lightning creation rate
        np = -1;
      else
        np = create_part(-1, rx, ry, playerp->elem, -1);
      if ( (np < NPART) && np>=0) {
        if (playerp->elem == PT_PHOT) {
          int random = abs((rng_between(-1, 1)))*3;
          if (random==0) {
            kill_part(np);
          } else {
            p_set_vy(&parts[np], 0);
            if (((int)playerp->pcomm)&(0x01|0x02))
              p_set_vx(&parts[np], (float)((((((int)playerp->pcomm)&0x02) == 0x02) - (((int)(playerp->pcomm)&0x01) == 0x01))*random));
            else
              p_set_vx(&parts[np], (float)(random));
          }
        } else if (playerp->elem == PT_LIGH) {
          int angle;
          int power = 100;
          if (gvx!=0 || gvy!=0)
            angle = (int)(atan2f(mvx, mvy)*180.0f/NP_PI);
          else
            angle = rng_between(0, 359);
          if (((int)playerp->pcomm)&0x01)
            angle += 180;
          if (angle>360)
            angle-=360;
          if (angle<0)
            angle+=360;
          parts[np].tmp = angle;
          parts[np].life = rng_between(0, 1+power/15) + power/7;
          p_set_temp(&parts[np], parts[np].life*power/2.5);
          parts[np].tmp2 = 1;
        } else if (!playerp->fan) {
          p_set_vx(&parts[np], p_vx(&parts[np]) - (-mvy*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01))));
          p_set_vy(&parts[np], p_vy(&parts[np]) - (mvx*(5*((((int)playerp->pcomm)&0x02) == 0x02) - 5*(((int)(playerp->pcomm)&0x01) == 0x01))));
          p_set_vx(&parts[i], p_vx(&parts[i]) - ((elements[(int)playerp->elem].Weight*p_vx(&parts[np]))/1000));
        }
        playerp->frames = 0;
      }
    }
  }
  //Simulation of joints
  d = 25/(powf((playerp->legs[0]-playerp->legs[4]), 2) + powf((playerp->legs[1]-playerp->legs[5]), 2)+25) - 0.5;  //Fast distance
  playerp->legs[4] -= (playerp->legs[0]-playerp->legs[4])*d;
  playerp->legs[5] -= (playerp->legs[1]-playerp->legs[5])*d;
  playerp->legs[0] += (playerp->legs[0]-playerp->legs[4])*d;
  playerp->legs[1] += (playerp->legs[1]-playerp->legs[5])*d;
  d = 25/(powf((playerp->legs[8]-playerp->legs[12]), 2) + powf((playerp->legs[9]-playerp->legs[13]), 2)+25) - 0.5;
  playerp->legs[12] -= (playerp->legs[8]-playerp->legs[12])*d;
  playerp->legs[13] -= (playerp->legs[9]-playerp->legs[13])*d;
  playerp->legs[8] += (playerp->legs[8]-playerp->legs[12])*d;
  playerp->legs[9] += (playerp->legs[9]-playerp->legs[13])*d;
  d = 36/(powf((playerp->legs[0]-p_x(&parts[i])), 2) + powf((playerp->legs[1]-p_y(&parts[i])), 2)+36) - 0.5;
  p_set_vx(&parts[i], p_vx(&parts[i]) - ((playerp->legs[0]-p_x(&parts[i]))*d));
  p_set_vy(&parts[i], p_vy(&parts[i]) - ((playerp->legs[1]-p_y(&parts[i]))*d));
  playerp->legs[0] += (playerp->legs[0]-p_x(&parts[i]))*d;
  playerp->legs[1] += (playerp->legs[1]-p_y(&parts[i]))*d;
  d = 36/(powf((playerp->legs[8]-p_x(&parts[i])), 2) + powf((playerp->legs[9]-p_y(&parts[i])), 2)+36) - 0.5;
  p_set_vx(&parts[i], p_vx(&parts[i]) - ((playerp->legs[8]-p_x(&parts[i]))*d));
  p_set_vy(&parts[i], p_vy(&parts[i]) - ((playerp->legs[9]-p_y(&parts[i]))*d));
  playerp->legs[8] += (playerp->legs[8]-p_x(&parts[i]))*d;
  playerp->legs[9] += (playerp->legs[9]-p_y(&parts[i]))*d;
  if (InBounds((int)(playerp->legs[4]), (int)(playerp->legs[5])) && !eval_move(t, (int)(playerp->legs[4]), (int)(playerp->legs[5]), NULL)) {
    playerp->legs[4] = playerp->legs[6];
    playerp->legs[5] = playerp->legs[7];
  }
  if (InBounds((int)(playerp->legs[12]), (int)(playerp->legs[13])) && !eval_move(t, (int)(playerp->legs[12]), (int)(playerp->legs[13]), NULL)) {
    playerp->legs[12] = playerp->legs[14];
    playerp->legs[13] = playerp->legs[15];
  }
  //This makes stick man "pop" from obstacles
  if (InBounds((int)(playerp->legs[4]), (int)(playerp->legs[5])) && !eval_move(t, (int)(playerp->legs[4]), (int)(playerp->legs[5]), NULL)) {
    float t;
    t = playerp->legs[4]; playerp->legs[4] = playerp->legs[6]; playerp->legs[6] = t;
    t = playerp->legs[5]; playerp->legs[5] = playerp->legs[7]; playerp->legs[7] = t;
  }
  if (InBounds((int)(playerp->legs[12]), (int)(playerp->legs[13])) && !eval_move(t, (int)(playerp->legs[12]), (int)(playerp->legs[13]), NULL)) {
    float t;
    t = playerp->legs[12]; playerp->legs[12] = playerp->legs[14]; playerp->legs[14] = t;
    t = playerp->legs[13]; playerp->legs[13] = playerp->legs[15]; playerp->legs[15] = t;
  }
  //Keeping legs distance
  if ((powf((playerp->legs[4] - playerp->legs[12]), 2) + powf((playerp->legs[5]-playerp->legs[13]), 2))<16) {
    float tvx, tvy;
    tvx = -gvy;
    tvy = gvx;
    if (tvx || tvy) {
      playerp->accs[2] -= 0.2*tvx/hypotf(tvx, tvy);
      playerp->accs[3] -= 0.2*tvy/hypotf(tvx, tvy);
      playerp->accs[6] += 0.2*tvx/hypotf(tvx, tvy);
      playerp->accs[7] += 0.2*tvy/hypotf(tvx, tvy);
    }
  }
  if ((powf((playerp->legs[0] - playerp->legs[8]), 2) + powf((playerp->legs[1]-playerp->legs[9]), 2))<16) {
    float tvx, tvy;
    tvx = -gvy;
    tvy = gvx;
    if (tvx || tvy) {
      playerp->accs[0] -= 0.2*tvx/hypotf(tvx, tvy);
      playerp->accs[1] -= 0.2*tvy/hypotf(tvx, tvy);
      playerp->accs[4] += 0.2*tvx/hypotf(tvx, tvy);
      playerp->accs[5] += 0.2*tvy/hypotf(tvx, tvy);
    }
  }
  //If legs touch something
  Element_STKM_interact(playerp, i, (int)(playerp->legs[4]+0.5), (int)(playerp->legs[5]+0.5));
  Element_STKM_interact(playerp, i, (int)(playerp->legs[12]+0.5), (int)(playerp->legs[13]+0.5));
  Element_STKM_interact(playerp, i, (int)(playerp->legs[4]+0.5), (int)playerp->legs[5]);
  Element_STKM_interact(playerp, i, (int)(playerp->legs[12]+0.5), (int)playerp->legs[13]);
  if (!parts[i].type)
    return 1;
  parts[i].ctype = playerp->elem;
  return 0;
}
void Element_STKM_interact(playerst *playerp, int i, int x, int y) {
  int r;
  if (x<0 || y<0 || x>=XRES || y>=YRES || !parts[i].type)
    return;
  r = PMAPAT(x, y);
  if (r) {
    int damage = 0;
    if (TYP(r)==PT_SPRK && playerp->elem!=PT_LIGH) { //If on charge
      damage += rng_between(32, 51);
    }
    if (!IsHeatInsulator(&parts[ID(r)]) && ((playerp->elem!=PT_LIGH && p_temp(&parts[ID(r)])>=323) || p_temp(&parts[ID(r)])<=243) && (!playerp->rocketBoots || TYP(r)!=PT_PLSM)) {
      damage += 2;
      playerp->accs[3] -= 1;
    }
    if (elements[TYP(r)].Properties&PROP_DEADLY)
      switch (TYP(r)) {
        case PT_ACID:
          damage += 5;
          break;
        default:
          damage++;
          break;
      }
    if (elements[TYP(r)].Properties&PROP_RADIOACTIVE)
      damage++;
    if (damage) {
      if (damage > parts[i].life) {
        STKM_die(playerp, i);
        return;
      }
      parts[i].life -= damage;
    }
    if (TYP(r)==PT_PRTI && parts[i].type) {
      int nnx, count=1;//gives rx=0, ry=1 in update_PRTO
      parts[ID(r)].tmp = (int)((p_temp(&parts[ID(r)])-73.15f)/100+1);
      if (parts[ID(r)].tmp>=CHANNELS) parts[ID(r)].tmp = CHANNELS-1;
      else if (parts[ID(r)].tmp<0) parts[ID(r)].tmp = 0;
      (void)nnx;
      int type = parts[i].type;
      if (portal_put(parts[ID(r)].tmp, count, &parts[i])) {
        kill_part(i);
        //stop new STKM/fighters being created to replace the ones in the portal:
        playerp->spwn = 1;
        if (type==PT_FIGH)
          fighcount++;
      }
    }
    if ((TYP(r)==PT_BHOL || TYP(r)==PT_NBHL) && parts[i].type) {
      if (!legacy_enable) {
        p_set_temp(&parts[ID(r)], restrict_flt(p_temp(&parts[ID(r)])+p_temp(&parts[i])/2, MIN_TEMP, MAX_TEMP));
      }
      kill_part(i);
    }
    if ((TYP(r)==PT_VOID || (TYP(r)==PT_PVOD && parts[ID(r)].life==10)) && (!parts[ID(r)].ctype || (parts[ID(r)].ctype==parts[i].type)!=(parts[ID(r)].tmp&1)) && parts[i].type) {
      kill_part(i);
    }
  }
}
void Element_STKM_init_legs(playerst *playerp, int i) {
  int x, y;
  x = (int)(p_x(&parts[i])+0.5f);
  y = (int)(p_y(&parts[i])+0.5f);
  playerp->legs[0] = (float)(x-1);
  playerp->legs[1] = (float)(y+6);
  playerp->legs[2] = (float)(x-1);
  playerp->legs[3] = (float)(y+6);
  playerp->legs[4] = (float)(x-3);
  playerp->legs[5] = (float)(y+12);
  playerp->legs[6] = (float)(x-3);
  playerp->legs[7] = (float)(y+12);
  playerp->legs[8] = (float)(x+1);
  playerp->legs[9] = (float)(y+6);
  playerp->legs[10] = (float)(x+1);
  playerp->legs[11] = (float)(y+6);
  playerp->legs[12] = (float)(x+3);
  playerp->legs[13] = (float)(y+12);
  playerp->legs[14] = (float)(x+3);
  playerp->legs[15] = (float)(y+12);
  for (int i = 0; i < 8; i++)
    playerp->accs[i] = 0;
  playerp->comm = 0;
  playerp->pcomm = 0;
  playerp->frames = 0;
  playerp->spwn = 0;
  playerp->fan = false;
  playerp->rocketBoots = false;
}
void Element_STKM_set_element(playerst *playerp, int element) {
  if (elements[element].Falldown != 0
      || elements[element].Properties&TYPE_GAS
      || elements[element].Properties&TYPE_LIQUID
      || elements[element].Properties&TYPE_ENERGY
      || element == PT_LOLZ || element == PT_LOVE) {
    if (!playerp->rocketBoots || element != PT_PLSM) {
      playerp->elem = element;
      playerp->fan = false;
    }
  }
  if (element == PT_TESC || element == PT_LIGH) {
    playerp->elem = PT_LIGH;
    playerp->fan = false;
  }
}

/* ---------------------------------------------------------------- STKM2 */
int el_STKM2_update(UPDATE_FUNC_ARGS) {
  Element_STKM_run_stickman(&player2, UPDATE_FUNC_SUBCALL_ARGS);
  return 0;
}
void el_STKM2_create(ELEMENT_CREATE_FUNC_ARGS) {
  int spawnID = create_part(-3, x, y, PT_SPAWN2, -1);
  if (spawnID >= 0)
    player2.spawnID = spawnID;
}
bool el_STKM2_createAllowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS) {
  return elementCount[PT_STKM2] <= 0 && !player2.spwn;
}
void el_STKM2_changeType(ELEMENT_CHANGETYPE_FUNC_ARGS) {
  if (to == PT_STKM2) {
    Element_STKM_init_legs(&player2, i);
    player2.spwn = 1;
  } else
    player2.spwn = 0;
}

/* ---------------------------------------------------------------- FIGH */
int el_FIGH_update(UPDATE_FUNC_ARGS) {
  if (parts[i].tmp < 0 || parts[i].tmp >= MAX_FIGHTERS) {
    kill_part(i);
    return 1;
  }
  playerst* figh = &fighters[(unsigned char)parts[i].tmp];
  int tarx = 0, tary = 0;
  parts[i].tmp2 = 0; //0 - stay in place, 1 - seek a stick man
  //Set target cords
  if (player2.spwn) {
    if (player.spwn && (powf((float)player.legs[2]-x, 2) + powf((float)player.legs[3]-y, 2))<=
       (powf((float)player2.legs[2]-x, 2) + powf((float)player2.legs[3]-y, 2))) {
      tarx = (int)player.legs[2];
      tary = (int)player.legs[3];
    } else {
      tarx = (int)player2.legs[2];
      tary = (int)player2.legs[3];
    }
    parts[i].tmp2 = 1;
  } else if (player.spwn) {
    tarx = (int)player.legs[2];
    tary = (int)player.legs[3];
    parts[i].tmp2 = 1;
  }
  switch (parts[i].tmp2) {
  case 1:
    if ((powf((float)(tarx-x), 2) + powf((float)(tary-y), 2))<600) {
      if (figh->elem == PT_LIGH || figh->elem == PT_NEUT
          || elements[figh->elem].Properties & (PROP_DEADLY | PROP_RADIOACTIVE)
          || elements[figh->elem].DefaultProperties.temp >= 323 || elements[figh->elem].DefaultProperties.temp <= 243)
        figh->comm = (int)figh->comm | 0x08;
    } else if (tarx<x) {
      if(figh->rocketBoots || !(eval_move(PT_FIGH, (int)(figh->legs[4])-10, (int)(figh->legs[5])+6, NULL)
           && eval_move(PT_FIGH, (int)(figh->legs[4])-10, (int)(figh->legs[5])+3, NULL)))
        figh->comm = 0x01;
      else
        figh->comm = 0x02;
      if (figh->rocketBoots) {
        if (tary<y)
          figh->comm = (int)figh->comm | 0x04;
      } else if (!eval_move(PT_FIGH, (int)(figh->legs[4])-4, (int)(figh->legs[5])-1, NULL)
          || !eval_move(PT_FIGH, (int)(figh->legs[12])-4, (int)(figh->legs[13])-1, NULL)
          || eval_move(PT_FIGH, 2*(int)(figh->legs[4])-(int)(figh->legs[6]), (int)(figh->legs[5])+5, NULL))
        figh->comm = (int)figh->comm | 0x04;
    } else {
      if (figh->rocketBoots || !(eval_move(PT_FIGH, (int)(figh->legs[12])+10, (int)(figh->legs[13])+6, NULL)
            && eval_move(PT_FIGH, (int)(figh->legs[12])+10, (int)(figh->legs[13])+3, NULL)))
        figh->comm = 0x02;
      else
        figh->comm = 0x01;
      if (figh->rocketBoots) {
        if (tary<y)
          figh->comm = (int)figh->comm | 0x04;
      } else if (!eval_move(PT_FIGH, (int)(figh->legs[4])+4, (int)(figh->legs[5])-1, NULL)
          || !eval_move(PT_FIGH, (int)(figh->legs[4])+4, (int)(figh->legs[5])-1, NULL)
          || eval_move(PT_FIGH, 2*(int)(figh->legs[12])-(int)(figh->legs[14]), (int)(figh->legs[13])+5, NULL))
        figh->comm = (int)figh->comm | 0x04;
    }
    break;
  default:
    figh->comm = 0;
    break;
  }
  figh->pcomm = figh->comm;
  Element_STKM_run_stickman(figh, UPDATE_FUNC_SUBCALL_ARGS);
  return 0;
}
bool el_FIGH_createAllowed(ELEMENT_CREATE_ALLOWED_FUNC_ARGS) {
  return Element_FIGH_CanAlloc();
}
void el_FIGH_changeType(ELEMENT_CHANGETYPE_FUNC_ARGS) {
  if (to == PT_FIGH) {
    parts[i].tmp = Element_FIGH_Alloc();
    if (parts[i].tmp >= 0)
      Element_FIGH_NewFighter(parts[i].tmp, i, PT_DUST);
  } else {
    FIGH_Free((unsigned char)parts[i].tmp);
  }
}
bool Element_FIGH_CanAlloc(void) {
  return fighcount < MAX_FIGHTERS;
}
int Element_FIGH_Alloc(void) {
  if (fighcount >= MAX_FIGHTERS)
    return -1;
  int i = 0;
  while (i < MAX_FIGHTERS && fighters[i].spwn==1)
    i++;
  if (i < MAX_FIGHTERS) {
    fighters[i].spwn = 1;
    fighters[i].elem = PT_DUST;
    fighcount++;
    return i;
  } else return -1;
}
static void FIGH_Free(unsigned char i) {
  if (i >= MAX_FIGHTERS) return;   /* (the original's array has room for any unsigned char: 100) */
  if (fighters[i].spwn) {
    fighters[i].spwn = 0;
    fighcount--;
  }
}
void Element_FIGH_NewFighter(int fighterID, int i, int elem) {
  Element_STKM_init_legs(&fighters[fighterID], i);
  if (elem > 0 && elem < PT_NUM)
    fighters[fighterID].elem = elem;
  fighters[fighterID].spwn = 1;
}

