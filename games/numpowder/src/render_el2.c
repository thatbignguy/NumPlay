/* NumPowder: the elements' graphics functions, H to Z (src/simulation/elements/<NAME>.cpp's graphics()), as in
 * render_el.c. */
#include "render.h"

int el_HSWC_graphics(GRAPHICS_FUNC_ARGS) {
  int lifemod = ((cpart->life > 10 ? 10 : cpart->life) * 19);
  *colr += lifemod;
  return 0;
}

int el_INVIS_graphics(GRAPHICS_FUNC_ARGS) {
  /* pv[ny/CELL][nx/CELL]>4.0f || pv[ny/CELL][nx/CELL]<-4.0f */
  if (cpart->tmp2) {
    *cola = 100;
    *colr = 15;
    *colg = 0;
    *colb = 150;
    *pixel_mode = PMODE_BLEND;
  }
  return 0;
}

int el_LAVA_graphics(GRAPHICS_FUNC_ARGS) {
  *colr = cpart->life * 2 + 0xE0;
  *colg = cpart->life * 1 + 0x50;
  *colb = cpart->life / 2 + 0x10;
  if (*colr > 255) *colr = 255;
  if (*colg > 192) *colg = 192;
  if (*colb > 128) *colb = 128;
  *firea = 40;
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  *pixel_mode |= FIRE_ADD;
  *pixel_mode |= PMODE_BLUR;
  return 0;
}

int el_LCRY_graphics(GRAPHICS_FUNC_ARGS) {
  uint32_t dc = part_dcolour(cpart);
  if (ren.decorations && dc && (dc & 0xFF000000)) {
    *colr = (dc >> 16) & 0xFF;
    *colg = (dc >> 8) & 0xFF;
    *colb = (dc)&0xFF;
    if (cpart->tmp2 < 10) {
      *colr /= 10 - cpart->tmp2;
      *colg /= 10 - cpart->tmp2;
      *colb /= 10 - cpart->tmp2;
    }
  } else {
    *colr = *colg = *colb = 0x50 + ((cpart->tmp2 > 10 ? 10 : cpart->tmp2) * 10);
  }
  *pixel_mode |= NO_DECO;
  return 0;
}

/* LIFE: the built-in types' two colours (the original keeps them in dcolour and tmp, which the calculator's
 * particle doesn't: they're the type's own here, as the original draws them with decorations off) */
int el_LIFE_graphics(GRAPHICS_FUNC_ARGS) {
  uint32_t colour1 = 0xFFFFFF, colour2 = 0x000000;
  int ruleset = cpart->ctype;
  if (ruleset >= 0 && ruleset < NGOL) {
    colour1 = builtinGol[ruleset].colour;
    colour2 = builtinGol[ruleset].colour2;
    ruleset = builtinGol[ruleset].ruleset;
  }
  int states = ((ruleset >> 17) & 0xF) + 2;
  if (states == 2) {
    *colr = colour1 >> 16 & 255;
    *colg = colour1 >> 8 & 255;
    *colb = colour1 & 255;
  } else {
    float mul = (cpart->tmp2 - 1) / (float)(states - 2);
    *colr = (int)((colour1 >> 16 & 255) * mul + (colour2 >> 16 & 255) * (1.f - mul));
    *colg = (int)((colour1 >> 8 & 255) * mul + (colour2 >> 8 & 255) * (1.f - mul));
    *colb = (int)((colour1 & 255) * mul + (colour2 & 255) * (1.f - mul));
  }
  *pixel_mode |= NO_DECO;
  return 0;
}

int el_LIGH_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 120;
  *firer = *colr = 235;
  *fireg = *colg = 245;
  *fireb = *colb = 255;
  *pixel_mode |= PMODE_GLOW | FIRE_ADD | DECO_FIRE;
  return 1;
}

int el_LITH_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->life >= 1000) { /* Exploding lith */
    *colr = 0xFF;
    *colg = 0xA0;
    *colb = 0x40;
    *pixel_mode |= PMODE_FLARE | PMODE_GLOW;
  } else if (cpart->ctype > 0) { /* Charged lith */
    int mult = ren_rng_between(cpart->ctype / 3, cpart->ctype) / 15;
    if (mult > 6) mult = 6;
    *colr -= 30 * mult;
    *colb += 20 * mult;
    *pixel_mode |= PMODE_FLARE | PMODE_GLOW;
  }
  return 0;
}

int el_NEUT_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 120;
  *firer = 10;
  *fireg = 80;
  *fireb = 120;
  *pixel_mode |= FIRE_ADD;
  return 1;
}

int el_PBCN_graphics(GRAPHICS_FUNC_ARGS) {
  int lifemod = ((cpart->life > 10 ? 10 : cpart->life) * 10);
  *colr += lifemod;
  *colg += lifemod / 2;
  return 0;
}

int el_PCLN_graphics(GRAPHICS_FUNC_ARGS) {
  int lifemod = ((cpart->life > 10 ? 10 : cpart->life) * 10);
  *colr += lifemod;
  *colg += lifemod;
  return 0;
}

/* PIPE and PPIP: what they carry is drawn as its own element (props_pipe_to_part: its type in ctype, its life in
 * tmp2, its tmp in tmp3, its ctype in tmp4) */
__attribute__((weak)) int tmp4_get(int i) { (void)i; return 0; }
int Element_PIPE_graphics(GRAPHICS_FUNC_ARGS) {
  int t = TYP(cpart->ctype);
  if (t > 0 && t < PT_NUM && elements[t].Enabled) {
    if (t == PT_STKM || t == PT_STKM2 || t == PT_FIGH) return 0;
    uint32_t colour = elements[t].Colour;
    *colr = colour >> 16 & 255;
    *colg = colour >> 8 & 255;
    *colb = colour & 255;
    GraphicsFn g = element_graphics(t);
    if (g) {
      Particle tpart;
      memset(&tpart, 0, sizeof tpart);
      tpart.type = (uint8_t)t;
      tpart.temp = cpart->temp;
      tpart.life = cpart->tmp2;
      tpart.tmp = cpart->tmp3;
      ptrdiff_t id = cpart - parts;
      tpart.ctype = id >= 0 && id < npart_max ? tmp4_get((int)id) : 0;
      tpart.x = cpart->x, tpart.y = cpart->y;
      g(&tpart, nx, ny, pixel_mode, cola, colr, colg, colb, firea, firer, fireg, fireb);
    }
  } else {
    switch (cpart->tmp & PFLAG_COLORS) {
      case PFLAG_COLOR_RED: *colr = 50, *colg = 1, *colb = 1; break;
      case PFLAG_COLOR_GREEN: *colr = 1, *colg = 50, *colb = 1; break;
      case PFLAG_COLOR_BLUE: *colr = 1, *colg = 1, *colb = 50; break;
      default: break;
    }
  }
  return 0;
}

/* PLNT.cpp's leafColor */
static const uint8_t leafColor[8][3] = {
  {243, 246, 244}, /* White */
  {255, 223, 50},  /* Yellow */
  {255, 183, 197}, /* Pink */
  {250, 0, 25},    /* Red */
  {128, 206, 196}, /* Cyan / light blue */
  {127, 255, 0},   /* Bright green */
  {0, 74, 178},    /* Blue */
  {12, 172, 0},    /* Usual PLNT green */
};
#define PLNT_DIR 3
#define PLNT_COLOUR 6
#define PLNT_LIFE 12
int el_PLNT_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->ctype == 0) {
    float maxtemp = (float)cpart->tmp2 > p_temp(cpart) ? (float)cpart->tmp2 : p_temp(cpart);
    if (maxtemp > 300) {
      *colr += (int)restrict_flt((maxtemp - 300) / 5, 0, 58);
      *colg -= (int)restrict_flt((maxtemp - 300) / 2, 0, 102);
      *colb += (int)restrict_flt((maxtemp - 300) / 5, 0, 70);
    }
    if (maxtemp < 273) {
      *colg += (int)restrict_flt((273 - maxtemp) / 4, 0, 255);
      *colb += (int)restrict_flt((273 - maxtemp) / 1.5f, 0, 255);
    }
  } else {
    /* Set tree-grown PLNT color */
    int colour = (cpart->ctype >> PLNT_COLOUR) & 0x3f;
    int cyan = (colour & 0x30) != 0, magenta = (colour & 0x0C) != 0, yellow = (colour & 0x03) != 0;
    int ind = 4 * cyan + 2 * magenta + yellow;
    if (0 <= ind && ind < 8) {
      *colr = leafColor[ind][0];
      *colg = leafColor[ind][1];
      *colb = leafColor[ind][2];
    }
  }
  return 0;
}

int el_PLSM_graphics(GRAPHICS_FUNC_ARGS) {
  uint32_t color = plasma_table_at(cpart->life);
  *colr = color >> 16 & 255;
  *colg = color >> 8 & 255;
  *colb = color & 255;
  *firea = 255;
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  *pixel_mode = PMODE_GLOW | PMODE_ADD; /* Clear default, don't draw pixel */
  *pixel_mode |= FIRE_ADD;
  return 0;
}

#define POLO_LIMIT 5
int el_POLO_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp >= POLO_LIMIT) {
    *colr = 0x70;
    *colg = 0x70;
    *colb = 0x70;
  } else {
    *pixel_mode |= PMODE_GLOW;
  }
  return 0;
}

int el_PROT_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 7;
  *firer = 250;
  *fireg = 170;
  *fireb = 170;
  *pixel_mode |= FIRE_BLEND;
  return 1;
}

int el_PRTI_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 8;
  *firer = 255;
  *fireg = 0;
  *fireb = 0;
  *pixel_mode |= EFFECT_DBGLINES;
  *pixel_mode |= EFFECT_GRAVIN;
  *pixel_mode &= ~PMODE;
  *pixel_mode |= PMODE_ADD;
  return 1;
}

int el_PRTO_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 8;
  *firer = 0;
  *fireg = 0;
  *fireb = 255;
  *pixel_mode |= EFFECT_DBGLINES;
  *pixel_mode |= EFFECT_GRAVOUT;
  *pixel_mode &= ~PMODE;
  *pixel_mode |= PMODE_ADD;
  return 1;
}

int el_PSTN_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->life) {
    *colr -= 60;
    *colg -= 60;
  }
  return 0;
}

int el_PTNM_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp) *pixel_mode |= PMODE_FLARE;
  return 0;
}

int el_PUMP_graphics(GRAPHICS_FUNC_ARGS) {
  int lifemod = ((cpart->life > 10 ? 10 : cpart->life) * 19);
  *colb += lifemod;
  return 0;
}

int el_PVOD_graphics(GRAPHICS_FUNC_ARGS) {
  int lifemod = ((cpart->life > 10 ? 10 : cpart->life) * 16);
  *colr += lifemod;
  return 0;
}

/* QRTZ and PQRT */
int Element_QRTZ_graphics(GRAPHICS_FUNC_ARGS) {
  int z = (cpart->tmp2 - 5) * 16; /* speckles! */
  *colr += z;
  *colg += z;
  *colb += z;
  return 0;
}

int el_ROCK_graphics(GRAPHICS_FUNC_ARGS) {
  int z = (cpart->tmp2 - 7) * 6; /* Randomized color noise based on tmp2 */
  *colr += z;
  *colg += z;
  *colb += z;
  if (p_temp(cpart) >= 810.15f) { /* Glows when hot, right before melting becomes bright */
    *pixel_mode |= FIRE_ADD;
    *firea = (int)((p_temp(cpart) - 810.15f) / 45);
    *firer = *colr;
    *fireg = *colg;
    *fireb = *colb;
  }
  return 0;
}

int el_SEED_graphics(GRAPHICS_FUNC_ARGS) {
  int water = (cpart->ctype >> PLNT_LIFE) & 0xFF;
  if (water > 3) { /* Ready for growth */
    *colr -= 50;
    *colg -= 50;
    *colb -= 50;
  } else if (water > 0) { /* Had a sip of water */
    *colr -= 25;
    *colg -= 25;
    *colb -= 25;
  }
  return 0;
}

/* SLCN.cpp's SLCN_COLOUR */
static const uint32_t SLCN_COLOUR[16] = {
  0x5A6679, 0x6878A1, 0xABBFDD, 0x838490, 0xBCCDDF, 0x82A0D2, 0x5B6680, 0x232C3B,
  0x485067, 0x8B9AB6, 0xADB1C1, 0xC3C6D1, 0x8594AD, 0x262F47, 0xA9AEBC, 0xC2E1F7,
};
/* (its 32-bit sparkle state, the original's tmp: bits 0 to 15 in tmp, 16 to 31 in tmp2, see el_powders.c) */
#define SLCN_STATE(p) ((uint32_t)(uint16_t)(p)->tmp | (uint32_t)(uint16_t)(p)->tmp2 << 16)
int el_SLCN_graphics(GRAPHICS_FUNC_ARGS) {
  uint32_t st = SLCN_STATE(cpart);
  uint32_t curr = SLCN_COLOUR[(st >> 12) & 15];
  int r = curr >> 16 & 255, g = curr >> 8 & 255, b = curr & 255;
  if (st & 0x800) { /* mix with next colour if phase is at least halfway there */
    uint32_t next = SLCN_COLOUR[(st >> 16) & 15];
    r = (r + (int)(next >> 16 & 255)) / 2;
    g = (g + (int)(next >> 8 & 255)) / 2;
    b = (b + (int)(next & 255)) / 2;
  }
  *colr = r;
  *colg = g;
  *colb = b;
  int rnd = (int)((st & 0xFFFF) * ((st >> 16) & 0xFFFF));
  if (!(rnd % 887)) *pixel_mode |= PMODE_FLARE | PMODE_GLOW;
  if (!(rnd % 593)) *pixel_mode |= PMODE_SPARK;
  return 0;
}

int el_SMKE_graphics(GRAPHICS_FUNC_ARGS) {
  *colr = 55;
  *colg = 55;
  *colb = 55;
  *firea = 75;
  *firer = 55;
  *fireg = 55;
  *fireb = 55;
  *pixel_mode = PMODE_NONE; /* Clear default, don't draw pixel */
  *pixel_mode |= FIRE_BLEND;
  return 1;
}

int el_SOAP_graphics(GRAPHICS_FUNC_ARGS) {
  *pixel_mode |= EFFECT_LINES | PMODE_BLUR;
  return 1;
}

int el_SPNG_graphics(GRAPHICS_FUNC_ARGS) {
  *colr -= cpart->life * 15;
  *colg -= cpart->life * 15;
  *colb -= cpart->life * 15;
  if (*colr <= 50) *colr = 50;
  if (*colg <= 50) *colg = 50;
  if (*colb <= 20) *colb = 20;
  return 0;
}

int el_SPRK_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 60;
  *firer = *colr / 2;
  *fireg = *colg / 2;
  *fireb = *colb / 2;
  *pixel_mode |= FIRE_SPARK;
  return 1;
}

/* STKM, STK2 and FIGH: drawn with their legs (render.c) */
int Element_STKM_graphics(GRAPHICS_FUNC_ARGS) {
  *colr = *colg = *colb = *cola = 0;
  *pixel_mode = PSPEC_STICKMAN;
  return 1;
}

int el_STOR_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp & 0xFF) {   /* (the stored element: tmp's bits 8 to 15 keep part of its tmp4, sim.c) */
    *pixel_mode |= PMODE_GLOW;
    *colr = 0x50;
    *colg = 0xDF;
    *colb = 0xDF;
  } else {
    *colr = 0x20;
    *colg = 0xAF;
    *colb = 0xAF;
  }
  return 0;
}

int el_SWCH_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->life >= 10) {
    *colr = 17;
    *colg = 217;
    *colb = 24;
    *pixel_mode |= PMODE_GLOW;
  }
  return 0;
}

int el_THDR_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 160;
  *fireg = 192;
  *fireb = 255;
  *firer = 144;
  *pixel_mode |= FIRE_ADD;
  return 1;
}

/* TRON.cpp's tron_colours (MakeTronColors: HSV_to_RGB(i << 4, 255, 255)) */
static const uint32_t tron_colours[32] = {
  0xFF0000, 0xFF4300, 0xFF8800, 0xFFCC00, 0xEDFF00, 0xA9FF00, 0x65FF00, 0x21FF00, 0x00FF22, 0x00FF66, 0x00FFAA,
  0x00FFEE, 0x00CBFF, 0x0087FF, 0x0043FF, 0x0000FF, 0x4400FF, 0x8700FF, 0xCC00FF, 0xFF00EE, 0xFF00A9, 0xFF0066,
  0xFF0021, 0xFF0021, 0xFF0021, 0xFF0021, 0xFF0021, 0xFF0021, 0xFF0021, 0xFF0021, 0xFF0021, 0xFF0021,
};
#define TRON_HEAD 0x00000001
#define TRON_DEATH 0x00000010
int el_TRON_graphics(GRAPHICS_FUNC_ARGS) {
  uint32_t col = tron_colours[(cpart->tmp & 0xF800) >> 11];
  if (cpart->tmp & TRON_HEAD) *pixel_mode |= PMODE_GLOW;
  *colr = (col & 0xFF0000) >> 16;
  *colg = (col & 0x00FF00) >> 8;
  *colb = (col & 0x0000FF);
  if (cpart->tmp & TRON_DEATH) {
    *pixel_mode |= FIRE_ADD | PMODE_FLARE;
    *firer = *colr;
    *fireg = *colg;
    *fireb = *colb;
    *firea = 255;
  }
  if (cpart->life < cpart->tmp2 && !(cpart->tmp & TRON_HEAD)) {
    *pixel_mode |= PMODE_BLEND;
    *pixel_mode &= ~PMODE_FLAT;
    *cola = (int)((((float)cpart->life) / ((float)cpart->tmp2)) * 255.0f);
  }
  return 0;
}

int el_TUNG_graphics(GRAPHICS_FUNC_ARGS) {
  const float MELTING_POINT = elements[PT_TUNG].HighTemperature;
  float startTemp = MELTING_POINT - 1500.0f;
  float tempOver = (((p_temp(cpart) - startTemp) / 1500.0f) * 3.14159265f) - (3.14159265f / 2.0f);
  if (tempOver > -(3.14159265f / 2.0f)) {
    if (tempOver > 3.14159265f / 2.0f) tempOver = 3.14159265f / 2.0f;
    float gradv = sinf(tempOver) + 1.0f;
    *firer = (int)(gradv * 258.0f);
    *fireg = (int)(gradv * 156.0f);
    *fireb = (int)(gradv * 112.0f);
    *firea = 30;
    *colr += *firer;
    *colg += *fireg;
    *colb += *fireb;
    *pixel_mode |= FIRE_ADD;
  }
  return 0;
}

/* VIBR and BVBR */
int Element_VIBR_graphics(GRAPHICS_FUNC_ARGS) {
  int gradient = cpart->tmp / 10;
  if (gradient >= 100 || cpart->life) {
    *colr = (int)(fabsf(sinf(expf((750.0f - cpart->life) / 170))) * 200.0f);
    if (cpart->tmp2) {
      *colg = *colr;
      *colb = 255;
    } else {
      *colg = 255;
      *colb = *colr;
    }
    *firea = 90;
    *firer = *colr;
    *fireg = *colg;
    *fireb = *colb;
    *pixel_mode = PMODE_NONE;
    *pixel_mode |= FIRE_BLEND;
  } else if (gradient < 100) {
    *colr += (int)restrict_flt(gradient * 2.0f, 0, 255);
    *colg += (int)restrict_flt(gradient * 2.0f, 0, 175);
    *colb += (int)restrict_flt(gradient * 2.0f, 0, 255);
    *firea = (int)restrict_flt(gradient * .6f, 0, 60);
    *firer = *colr / 2;
    *fireg = *colg / 2;
    *fireb = *colb / 2;
    *pixel_mode |= FIRE_BLEND;
  }
  return 0;
}

int el_VINE_graphics(GRAPHICS_FUNC_ARGS) {
  float maxtemp = (float)cpart->tmp2 > p_temp(cpart) ? (float)cpart->tmp2 : p_temp(cpart);
  if (maxtemp > 300) {
    *colr += (int)restrict_flt((maxtemp - 300) / 5, 0, 58);
    *colg -= (int)restrict_flt((maxtemp - 300) / 2, 0, 102);
    *colb += (int)restrict_flt((maxtemp - 300) / 5, 0, 70);
  }
  if (maxtemp < 273) {
    *colg += (int)restrict_flt((273 - maxtemp) / 4, 0, 255);
    *colb += (int)restrict_flt((273 - maxtemp) / 1.5f, 0, 255);
  }
  return 0;
}

int el_VIRS_graphics(GRAPHICS_FUNC_ARGS) {
  *pixel_mode |= PMODE_BLUR;
  *pixel_mode |= NO_DECO;
  return 1;
}

int el_VRSG_graphics(GRAPHICS_FUNC_ARGS) {
  *pixel_mode &= ~PMODE;
  *pixel_mode |= FIRE_BLEND;
  *firer = *colr / 2;
  *fireg = *colg / 2;
  *fireb = *colb / 2;
  *firea = 125;
  *pixel_mode |= NO_DECO;
  return 1;
}

int el_VRSS_graphics(GRAPHICS_FUNC_ARGS) {
  *pixel_mode |= NO_DECO;
  return 1;
}

int el_WARP_graphics(GRAPHICS_FUNC_ARGS) {
  *colr = *colg = *colb = *cola = 0;
  *pixel_mode |= NO_DECO;
  return 0;
}

#define WIFI_FREQUENCY 0.0628f
int el_WIFI_graphics(GRAPHICS_FUNC_ARGS) {
  int q = (int)((p_temp(cpart) - 73.15f) / 100 + 1);
  *colr = (int)(sinf(WIFI_FREQUENCY * q + 0) * 127 + 128);
  *colg = (int)(sinf(WIFI_FREQUENCY * q + 2) * 127 + 128);
  *colb = (int)(sinf(WIFI_FREQUENCY * q + 4) * 127 + 128);
  *pixel_mode |= EFFECT_DBGLINES;
  return 0;
}

int el_WIRE_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->ctype == 0) {
    *colr = 255;
    *colg = 204;
    *colb = 0;
    return 0;
  }
  if (cpart->ctype == 1) {
    *colr = 50;
    *colg = 100;
    *colb = 255;
    return 0;
  }
  if (cpart->ctype == 2) {
    *colr = 255;
    *colg = 100;
    *colb = 50;
    return 0;
  }
  return 0;
}

int el_WOOD_graphics(GRAPHICS_FUNC_ARGS) {
  float maxtemp = (float)cpart->tmp > p_temp(cpart) ? (float)cpart->tmp : p_temp(cpart);
  if (maxtemp > 400) {
    *colr -= (int)restrict_flt((maxtemp - 400) / 3, 0, 172);
    *colg -= (int)restrict_flt((maxtemp - 400) / 4, 0, 140);
    *colb -= (int)restrict_flt((maxtemp - 400) / 20, 0, 44);
  }
  if (maxtemp < 273) {
    *colr -= (int)restrict_flt((273 - maxtemp) / 5, 0, 40);
    *colg += (int)restrict_flt((273 - maxtemp) / 4, 0, 40);
    *colb += (int)restrict_flt((273 - maxtemp) / 1.5f, 0, 150);
  }
  return 0;
}
