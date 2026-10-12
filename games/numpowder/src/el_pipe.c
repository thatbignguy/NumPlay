/* NumPowder: The Powder Toy's PIPE and PPIP (elements/PIPE.cpp, PPIP.cpp), ported.
 *
 * A pipe pixel holds a particle: its element in ctype's bits 0 to 7, its life in tmp2, its tmp in tmp3 and its ctype
 * (the original's tmp4) in ctype's bits 8 to 31 (see tmp4_get). The original's 32 bits of flags in tmp are moved to
 * fit 16 bits of tmp and 8 of flags (simpriv.h: PIPE_SINGLE...); the particle-decoration flag goes (no
 * decoration). */
#include "simpriv.h"

static const struct { int8_t X, Y; } Element_PIPE_offsets[8] = {
  {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1},
};

static unsigned int PIPE_prevColor(unsigned int flags) {
  unsigned int color = flags & PFLAG_COLORS;
  if (color == PFLAG_COLOR_RED) return PFLAG_COLOR_GREEN;
  else if (color == PFLAG_COLOR_GREEN) return PFLAG_COLOR_BLUE;
  else if (color == PFLAG_COLOR_BLUE) return PFLAG_COLOR_RED;
  return PFLAG_COLOR_RED;
}
static unsigned int PIPE_nextColor(unsigned int flags) {
  unsigned int color = flags & PFLAG_COLORS;
  if (color == PFLAG_COLOR_RED) return PFLAG_COLOR_BLUE;
  else if (color == PFLAG_COLOR_BLUE) color = PFLAG_COLOR_GREEN;
  else if (color == PFLAG_COLOR_GREEN) return PFLAG_COLOR_RED;
  return PFLAG_COLOR_GREEN;
}

static inline int pid(const Particle *p) { return (int)(p - parts); }
/* clears the particle a pipe holds: its element (the original's ctype = 0) and the rest of its ctype */
static void pipe_clear(Particle *pipe) {
  pipe->ctype = 0;
  pipe_ovf_clear(pid(pipe));
}

/* props_pipe_to_part: a pipe's (or STOR's) particle, to part */
static void props_pipe_to_part(const Particle *pipe, Particle *part, bool STOR) {
  /* STOR also moves particles to PRTI this way; it keeps its particle as PIPE does, but the element in tmp */
  int stored_ctype = tmp4_get(pid(pipe));
  if (STOR) part->type = (uint8_t)TYP(pipe->tmp);
  else part->type = (uint8_t)TYP(pipe->ctype);
  part->temp = pipe->temp;
  part->life = pipe->tmp2;
  part->tmp = pipe->tmp3;
  part->ctype = stored_ctype;
  if (!(elements[part->type].Properties & TYPE_ENERGY)) {
    part->vx = 0;
    part->vy = 0;
  }
  part->tmp2 = 0;
  part->flags = 0;
}

void Element_PIPE_transfer_pipe_to_part(Particle *pipe, Particle *part, bool STOR) {
  props_pipe_to_part(pipe, part, STOR);
  if (STOR) pipe->tmp &= ~0xFF;   /* (the original's tmp = 0: its tmp4 stays) */
  else pipe_clear(pipe);
  /* (a particle in the pipe's decoration isn't kept) */
}

void Element_PIPE_transfer_part_to_pipe(Particle *part, Particle *pipe) {
  int i = pid(pipe);
  pipe->ctype = part->type;
  if ((pipe->tmp & PFLAG_CAN_CONDUCT) == 0) {
    pipe->temp = part->temp;
  } else {
    float c_pipe = elements[pipe->type].HeatCapacity, c_part = elements[part->type].HeatCapacity;
    p_set_temp(pipe, (c_part * p_temp(part) + c_pipe * p_temp(pipe)) / (c_part + c_pipe));
  }
  pipe->tmp2 = part->life;
  pipe->tmp3 = part->tmp;
  tmp4_set(i, part->ctype);
}

static void transfer_pipe_to_pipe(Particle *src, Particle *dest, bool STOR) {
  int t4 = tmp4_get(pid(src));
  if (STOR) {   /* STOR to PIPE */
    dest->ctype = src->tmp & 0xFF;
    src->tmp &= ~0xFF;
  } else {
    dest->ctype = src->ctype & 0xFF;
    pipe_clear(src);
  }
  if ((dest->tmp & PFLAG_CAN_CONDUCT) == 0) {
    dest->temp = src->temp;
  } else {
    int src_ctype = src->ctype & 0xFF;   /* (0 for a pipe now; a STOR's filter, as in the original) */
    float c_src = (0 < src_ctype && src_ctype < PT_NUM) ? elements[src_ctype].HeatCapacity : 1.0f;
    float c_dest = elements[dest->type].HeatCapacity;
    p_set_temp(dest, (c_src * p_temp(src) + c_dest * p_temp(dest)) / (c_src + c_dest));
  }
  dest->tmp2 = src->tmp2;
  dest->tmp3 = src->tmp3;
  tmp4_set(pid(dest), t4);
}

/* pushes a pipe's particle along (up to 2 a frame) */
static void pushParticle(int i, int count, int original) {
  unsigned int notctype = PIPE_nextColor((unsigned)parts[i].tmp);
  if (!TYP(parts[i].ctype) || count >= 2)   /* nothing there, or the max speed of 2 a frame */
    return;
  int x = (parts[i].x + 32) / 64, y = (parts[i].y + 32) / 64;
  if (!(parts[i].tmp & PIPE_FWD_SINGLE)) {
    /* normal random push: 5 tries in a random number, 3 used */
    int rndstore = (int)rng_gen();
    for (int q = 0; q < 3; q++) {   /* try to push 3 times */
      int rnd = rndstore & 7;
      rndstore = rndstore >> 3;
      int rx = Element_PIPE_offsets[rnd].X, ry = Element_PIPE_offsets[rnd].Y;
      int r = PMAPAT(x + rx, y + ry);
      if (!r) {
        continue;
      } else if ((TYP(r) == PT_PIPE || TYP(r) == PT_PPIP) && (parts[ID(r)].tmp & PFLAG_COLORS) != (int)notctype && !TYP(parts[ID(r)].ctype)) {
        transfer_pipe_to_pipe(parts + i, parts + ID(r), false);
        if (ID(r) > original) parts[ID(r)].flags |= PFLAG_NORMALSPEED;   /* skip its push, normalizes speed */
        count++;
        pushParticle(ID(r), count, original);
      } else if (TYP(r) == PT_PRTI) {   /* into PRTI for a pipe speed increase */
        int portaltmp = parts[ID(r)].tmp;
        if (portaltmp >= CHANNELS) portaltmp = CHANNELS - 1;
        else if (portaltmp < 0) portaltmp = 0;
        if (portal_free(portaltmp, count)) {
          Particle p;
          memset(&p, 0, sizeof p);
          Element_PIPE_transfer_pipe_to_part(parts + i, &p, false);
          portal_put(portaltmp, count, &p);
          count++;
        }
      }
    }
  } else {   /* predefined 1 pixel thick pipe movement */
    int coords = 7 - ((parts[i].tmp >> PIPE_FWD_SHIFT) & 7);
    int r = PMAPAT(x + Element_PIPE_offsets[coords].X, y + Element_PIPE_offsets[coords].Y);
    if ((TYP(r) == PT_PIPE || TYP(r) == PT_PPIP) && (parts[ID(r)].tmp & PFLAG_COLORS) != (int)notctype && !TYP(parts[ID(r)].ctype)) {
      transfer_pipe_to_pipe(parts + i, parts + ID(r), false);
      if (ID(r) > original) parts[ID(r)].flags |= PFLAG_NORMALSPEED;
      count++;
      pushParticle(ID(r), count, original);
    } else if (TYP(r) == PT_PRTI) {
      int portaltmp = parts[ID(r)].tmp;
      if (portaltmp >= CHANNELS) portaltmp = CHANNELS - 1;
      else if (portaltmp < 0) portaltmp = 0;
      if (portal_free(portaltmp, count)) {
        Particle p;
        memset(&p, 0, sizeof p);
        Element_PIPE_transfer_pipe_to_part(parts + i, &p, false);
        portal_put(portaltmp, count, &p);
        count++;
      }
    } else if (!r) {   /* out of the pipe at its end, much faster */
      int rx = Element_PIPE_offsets[coords].X, ry = Element_PIPE_offsets[coords].Y;
      int np = create_part(-1, x + rx, y + ry, TYP(parts[i].ctype), -1);
      if (np != -1) Element_PIPE_transfer_pipe_to_part(parts + i, parts + np, false);
    }
  }
}

int Element_PIPE_update(UPDATE_FUNC_ARGS) {
  if (TYP(parts[i].ctype) && !elements[TYP(parts[i].ctype)].Enabled) pipe_clear(&parts[i]);
  if (parts[i].flags & PPIP_TMPFLAG_TRIGGERS) {
    int pause_changed = 0;
    if (parts[i].flags & PPIP_TMPFLAG_TRIGGER_ON) {   /* TRIGGER_ON overrides TRIGGER_OFF */
      if (parts[i].tmp & PPIP_TMPFLAG_PAUSED) pause_changed = 1;
      parts[i].tmp &= ~PPIP_TMPFLAG_PAUSED;
    } else if (parts[i].flags & PPIP_TMPFLAG_TRIGGER_OFF) {
      if (!(parts[i].tmp & PPIP_TMPFLAG_PAUSED)) pause_changed = 1;
      parts[i].tmp |= PPIP_TMPFLAG_PAUSED;
    }
    if (pause_changed) {
      for (int rx = -2; rx <= 2; rx++)
        for (int ry = -2; ry <= 2; ry++)
          if (rx || ry) {
            int r = PMAPAT(x + rx, y + ry);
            if (TYP(r) == PT_BRCK) {
              if (parts[i].tmp & PPIP_TMPFLAG_PAUSED) parts[ID(r)].tmp = 0;
              else parts[ID(r)].tmp = 1;   /* make surrounding BRCK glow */
            }
          }
    }
    if (parts[i].flags & PPIP_TMPFLAG_TRIGGER_REVERSE) {
      parts[i].tmp ^= PPIP_TMPFLAG_REVERSED;
      /* switch colours so it goes in reverse */
      if ((parts[i].tmp & PFLAG_COLORS) != PFLAG_COLOR_GREEN) parts[i].tmp ^= PFLAG_COLOR_GREEN;
      if (parts[i].tmp & PIPE_SINGLE) {   /* switch the one pixel pipe's directions */
        int coords = (parts[i].tmp >> 6) & 0xF;    /* the reverse group (the original's tmp >> 13) */
        int coords2 = (parts[i].tmp >> 2) & 0xF;   /* the forward group (tmp >> 9) */
        parts[i].tmp &= ~(PIPE_FWD_GROUP | PIPE_REV_GROUP);
        parts[i].tmp |= (int16_t)(coords << 2);
        parts[i].tmp |= (int16_t)(coords2 << 6);
      }
    }
    parts[i].flags &= (uint8_t)~PPIP_TMPFLAG_TRIGGERS;
  }
  if ((parts[i].tmp & PFLAG_COLORS) && !(parts[i].tmp & PPIP_TMPFLAG_PAUSED)) {
    if (parts[i].life == 3) {
      int lastneighbor = -1, neighborcount = 0, count = 0;
      bool heatPipe = false;
      /* make automatic pipe pattern */
      for (int rx = -1; rx <= 1; rx++)
        for (int ry = -1; ry <= 1; ry++)
          if (rx || ry) {
            count++;
            int r = PMAPAT(x + rx, y + ry);
            if (!r) continue;
            if (TYP(r) == PT_HEAC) {
              heatPipe = true;
              continue;
            }
            if (TYP(r) != PT_PIPE && TYP(r) != PT_PPIP) continue;
            unsigned int next = PIPE_nextColor((unsigned)parts[i].tmp), prev = PIPE_prevColor((unsigned)parts[i].tmp);
            if (parts[ID(r)].tmp & PFLAG_INITIALIZING) {
              parts[ID(r)].tmp |= (int16_t)next;
              parts[ID(r)].tmp &= ~PFLAG_INITIALIZING;
              parts[ID(r)].life = 6;
              if (parts[i].tmp & PIPE_SINGLE) {   /* a single pixel pipe */
                parts[ID(r)].tmp |= PIPE_FWD_SINGLE;   /* it transfers to a single pixel pipe */
                parts[ID(r)].tmp |= (int16_t)((count - 1) << PIPE_FWD_SHIFT);   /* where it came from */
                parts[i].tmp |= (int16_t)((8 - count) << PIPE_REV_SHIFT);
                parts[i].tmp |= PIPE_REV_SINGLE;
              }
              neighborcount++;
              lastneighbor = ID(r);
            } else if ((parts[ID(r)].tmp & PFLAG_COLORS) != (int)prev) {
              neighborcount++;
              lastneighbor = ID(r);
            }
          }
      if (neighborcount == 1) parts[lastneighbor].tmp |= PIPE_SINGLE;
      if (heatPipe) Element_PPIP_flood_trigger(x, y, PT_HEAC);
    } else {
      if (parts[i].flags & PFLAG_NORMALSPEED) {   /* skip the push, so more particles don't speed it up */
        parts[i].flags &= (uint8_t)~PFLAG_NORMALSPEED;
      } else {
        pushParticle(i, 0, i);
      }
      if (nt) {   /* something besides PIPE around */
        int rndstore = (int)rng_gen();
        int rnd = rndstore & 7;
        int rx = Element_PIPE_offsets[rnd].X, ry = Element_PIPE_offsets[rnd].Y;
        int r = PMAPAT(x + rx, y + ry);
        if (!r) r = PHOTAT(x + rx, y + ry);
        if (surround_space && !r && TYP(parts[i].ctype)) {   /* creating at the end */
          int np = create_part(-1, x + rx, y + ry, TYP(parts[i].ctype), -1);
          if (np != -1) Element_PIPE_transfer_pipe_to_part(parts + i, parts + np, false);
        }
        /* eating a particle at the entrance */
        else if (!TYP(parts[i].ctype) && (elements[TYP(r)].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY))) {
          if (TYP(r) == PT_SOAP) Element_SOAP_detach(ID(r));
          Element_PIPE_transfer_part_to_pipe(parts + ID(r), parts + i);
          kill_part(ID(r));
        } else if (!TYP(parts[i].ctype) && TYP(r) == PT_STOR && IsElement(parts[ID(r)].tmp & 0xFF) &&
                   (elements[parts[ID(r)].tmp & 0xFF].Properties & (TYPE_PART | TYPE_LIQUID | TYPE_GAS | TYPE_ENERGY))) {
          /* STOR stores properties in the same places as PIPE does */
          transfer_pipe_to_pipe(parts + ID(r), parts + i, true);
        }
      }
    }
  } else if (!(parts[i].tmp & (PFLAG_COLORS | PFLAG_INITIALIZING)) && parts[i].life <= 10) {
    /* make a border of BRCK */
    for (int rx = -2; rx <= 2; rx++)
      for (int ry = -2; ry <= 2; ry++)
        if (rx || ry) {
          int r = PMAPAT(x + rx, y + ry);
          if (!r) {
            int index = create_part(-1, x + rx, y + ry, PT_BRCK, -1);
            if (parts[i].type == PT_PPIP && index != -1) parts[index].tmp = 1;
          }
        }
    if (parts[i].life <= 1) parts[i].tmp |= PFLAG_INITIALIZING;
  }
  /* wait for empty space before starting to generate the automatic pipe pattern */
  else if (parts[i].tmp & PFLAG_INITIALIZING) {
    if (!parts[i].life) {
      for (int rx = -1; rx <= 1; rx++)
        for (int ry = -1; ry <= 1; ry++)
          if (rx || ry) {
            int b = bmap[(y + ry) / WCELL][(x + rx) / WCELL];
            if (!PMAPAT(x + rx, y + ry) && b != WL_ALLOWAIR && b != WL_WALL && b != WL_WALLELEC &&
                (b != WL_EWALL || emap[(y + ry) / WCELL][(x + rx) / WCELL]))
              parts[i].life = 50;
          }
    } else if (parts[i].life == 5) {   /* the start of a single pixel pipe? */
      int issingle = 1;
      for (int rx = -1; rx <= 1; rx++)
        for (int ry = -1; ry <= 1; ry++)
          if (rx || ry) {
            int r = PMAPAT(x + rx, y + ry);
            if ((TYP(r) == PT_PIPE || TYP(r) == PT_PPIP) && parts[i].life) issingle = 0;
          }
      if (issingle) parts[i].tmp |= PIPE_SINGLE;
    } else if (parts[i].life == 2) {
      parts[i].tmp |= PFLAG_COLOR_RED;
      parts[i].tmp &= ~PFLAG_INITIALIZING;
      parts[i].life = 6;
    }
  }
  return 0;
}

/* ---------------------------------------------------------------- PPIP */
/* the triggers of a powered pipe and the pipes connected to it, for the next frame (the original's tmp << 3, here
 * flags << PPIP_LATE_SHIFT); HEAC next to a pipe makes it conduct heat */
static bool ppip_has(int id, int prop, bool tmpProp) {
  return tmpProp ? (parts[id].tmp & prop) != 0 : (parts[id].flags & prop) != 0;
}
void Element_PPIP_flood_trigger(int x, int y, int sparkedBy) {
  int x1, x2, t = TYP(PMAPAT(x, y));
  if (t != PT_PIPE && t != PT_PPIP) return;
  /* separate flags for on and off, so PSCN beats NSCN on the same frame whatever the particle order */
  int prop = 0;
  bool tmpProp = false;
  if (sparkedBy == PT_PSCN) prop = PPIP_TMPFLAG_TRIGGER_ON << PPIP_LATE_SHIFT;
  else if (sparkedBy == PT_NSCN) prop = PPIP_TMPFLAG_TRIGGER_OFF << PPIP_LATE_SHIFT;
  else if (sparkedBy == PT_INST) prop = PPIP_TMPFLAG_TRIGGER_REVERSE << PPIP_LATE_SHIFT;
  else if (sparkedBy == PT_HEAC) prop = PFLAG_CAN_CONDUCT, tmpProp = true;   /* special case for HEAC near pipe */
  if (prop == 0 || (t != PT_PPIP && sparkedBy != PT_HEAC) || ppip_has(ID(PMAPAT(x, y)), prop, tmpProp)) return;
  flood_stack_init(0);
  flood_stack_push(x, y);
  do {
    flood_stack_pop(&x, &y);
    x1 = x2 = x;
    while (x1 >= BORDER) {   /* go left as far as possible */
      if (TYP(PMAPAT(x1 - 1, y)) != t) break;
      x1--;
    }
    while (x2 < XRES - BORDER) {
      if (TYP(PMAPAT(x2 + 1, y)) != t) break;
      x2++;
    }
    for (x = x1; x <= x2; x++) {   /* fill span */
      int id = ID(PMAPAT(x, y));
      if (!ppip_has(id, prop, tmpProp) && sparkedBy != PT_HEAC) Element_PPIP_ppip_changed = 1;
      if (tmpProp) parts[id].tmp |= (int16_t)prop;
      else parts[id].flags |= (uint8_t)prop;
    }
    /* adjacent pixels, diagonals too (x limits already within the edge) */
    if (y >= BORDER + 1)
      for (x = x1 - 1; x <= x2 + 1; x++)
        if (TYP(PMAPAT(x, y - 1)) == t && !ppip_has(ID(PMAPAT(x, y - 1)), prop, tmpProp))
          if (!flood_stack_push(x, y - 1)) return;
    if (y < YRES - BORDER - 1)
      for (x = x1 - 1; x <= x2 + 1; x++)
        if (TYP(PMAPAT(x, y + 1)) == t && !ppip_has(ID(PMAPAT(x, y + 1)), prop, tmpProp))
          if (!flood_stack_push(x, y + 1)) return;
  } while (flood_stack_size() > 0);
}
