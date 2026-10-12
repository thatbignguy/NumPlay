/* NumPowder: the elements' graphics functions, A to G (src/simulation/elements/<NAME>.cpp's graphics()), and the
 * modes each may give (for which bands its drawing reaches: render.c).
 *
 * As the original's, with: `gfctx.rng` the renderer's ren_rng_*, `gfctx.sim->currentTick` currentTick,
 * `gfctx.ren->decorationLevel` ren.decorations, `cpart->dcolour` part_dcolour(cpart) (decorations aren't kept:
 * 0), the fixed-point fields through their accessors, the colour tables as 0xRRGGBB. */
#include "render.h"

const uint32_t el_graphics_modes[PT_COUNT] = {
  [PT_ACEL] = PMODE_GLOW,
  [PT_ACID] = PMODE_BLUR,
  [PT_BASE] = PMODE_BLUR | PMODE_SPARK,
  [PT_BIZR] = FIRE_ADD | PMODE_BLUR,
  [PT_BIZRG] = FIRE_ADD | PMODE_BLUR,
  [PT_BIZRS] = FIRE_ADD | PMODE_BLUR,
  [PT_BOMB] = PMODE_FLARE,
  [PT_BRAY] = PMODE_BLEND | PMODE_GLOW,
  [PT_BRCK] = FIRE_ADD,
  [PT_C5] = FIRE_ADD | PMODE_ADD,
  [PT_CFLM] = FIRE_ADD,
  [PT_DCEL] = PMODE_GLOW,
  [PT_DEST] = PMODE_LFLARE | PMODE_SPARK,
  [PT_DEUT] = PMODE_GLOW | FIRE_ADD | PMODE_BLUR,
  [PT_DMG] = PMODE_FLARE,
  [PT_ELEC] = FIRE_ADD,
  [PT_EMBR] = FIRE_ADD | PMODE_BLEND | PMODE_GLOW | PMODE_SPARK | PMODE_ADD,
  [PT_EXOT] = PMODE_FLARE | FIRE_ADD | PMODE_BLUR,
  [PT_FILT] = PMODE_BLEND,
  [PT_FIRE] = FIRE_ADD,
  [PT_FIRW] = PMODE_GLOW,
  [PT_GBMB] = PMODE_FLARE | PMODE_SPARK,
  [PT_GEL] = PMODE_BLUR,
  [PT_GLOW] = FIRE_ADD | PMODE_GLOW | PMODE_ADD | PMODE_BLUR,
  [PT_GRAV] = FIRE_ADD | PMODE_ADD | PMODE_GLOW | PMODE_FLARE,
  [PT_GRVT] = FIRE_BLEND,
  [PT_INVIS] = PMODE_BLEND,
  [PT_LAVA] = FIRE_ADD | PMODE_BLUR,
  [PT_LIGH] = PMODE_GLOW | FIRE_ADD | DECO_FIRE,
  [PT_LITH] = PMODE_FLARE | PMODE_GLOW,
  [PT_NEUT] = FIRE_ADD,
  [PT_PHOT] = FIRE_ADD | PMODE_ADD,
  /* (PIPE and PPIP draw what they carry: render.c asks that element) */
  [PT_PIPE] = PMODE_GLOW | PMODE_BLUR | PMODE_BLEND | PMODE_ADD | FIRE_ADD | FIRE_BLEND | FIRE_SPARK,
  [PT_PPIP] = PMODE_GLOW | PMODE_BLUR | PMODE_BLEND | PMODE_ADD | FIRE_ADD | FIRE_BLEND | FIRE_SPARK,
  [PT_PLSM] = PMODE_GLOW | PMODE_ADD | FIRE_ADD,
  [PT_POLO] = PMODE_GLOW,
  [PT_PROT] = FIRE_BLEND,
  [PT_PRTI] = EFFECT_DBGLINES | EFFECT_GRAVIN | PMODE_ADD,
  [PT_PRTO] = EFFECT_DBGLINES | EFFECT_GRAVOUT | PMODE_ADD,
  [PT_PTNM] = PMODE_FLARE,
  [PT_ROCK] = FIRE_ADD,
  [PT_SLCN] = PMODE_FLARE | PMODE_GLOW | PMODE_SPARK,
  [PT_SMKE] = FIRE_BLEND,
  [PT_SOAP] = EFFECT_LINES | PMODE_BLUR,
  [PT_SPRK] = FIRE_SPARK,
  [PT_STKM] = PSPEC_STICKMAN,
  [PT_STKM2] = PSPEC_STICKMAN,
  [PT_FIGH] = PSPEC_STICKMAN,
  [PT_STOR] = PMODE_GLOW,
  [PT_SWCH] = PMODE_GLOW,
  [PT_THDR] = FIRE_ADD,
  [PT_TRON] = PMODE_GLOW | FIRE_ADD | PMODE_FLARE | PMODE_BLEND,
  [PT_TUNG] = FIRE_ADD,
  [PT_VIBR] = FIRE_BLEND,
  [PT_BVBR] = FIRE_BLEND,
  [PT_VIRS] = PMODE_BLUR,
  [PT_VRSG] = FIRE_BLEND,
  [PT_WIFI] = EFFECT_DBGLINES,
};

/* the colour of a photon's wavelengths (PHOT, BRAY, C5, BIZR, FILT) */
static void wavelength_colour(int wl, int *colr, int *colg, int *colb) {
  int x;
  *colr = *colg = *colb = 0;
  for (x = 0; x < 12; x++) {
    *colr += (wl >> (x + 18)) & 1;
    *colb += (wl >> x) & 1;
  }
  for (x = 0; x < 12; x++) *colg += (wl >> (x + 9)) & 1;
  x = 624 / (*colr + *colg + *colb + 1);
  *colr *= x;
  *colg *= x;
  *colb *= x;
}

int el_ACEL_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp) *pixel_mode |= PMODE_GLOW;
  return 0;
}

int el_ACID_graphics(GRAPHICS_FUNC_ARGS) {
  int s = cpart->life;
  if (s > 75) s = 75; /* These two should not be here. */
  if (s < 49) s = 49;
  s = (s - 49) * 3;
  if (s == 0) s = 1;
  *colr += s * 4;
  *colg += s * 1;
  *colb += s * 2;
  *pixel_mode |= PMODE_BLUR;
  return 0;
}

int el_AMTR_graphics(GRAPHICS_FUNC_ARGS) {
  /* don't render AMTR as a gas: this function just overrides the default graphics */
  return 1;
}

int el_BASE_graphics(GRAPHICS_FUNC_ARGS) {
  int s = cpart->life;
  if (s <= 25) *colr = 0x33, *colg = 0x4C, *colb = 0xD8;
  else if (s <= 50) *colr = 0x58, *colg = 0x83, *colb = 0xE8;
  else if (s <= 75) *colr = 0x7D, *colg = 0xBA, *colb = 0xF7;
  *pixel_mode |= PMODE_BLUR;
  if (cpart->tmp == 1) *pixel_mode |= PMODE_SPARK;
  return 0;
}

/* BIZR, BIZRG, BIZRS */
int Element_BIZR_graphics(GRAPHICS_FUNC_ARGS) {
  float brightness = fabsf(p_vx(cpart)) + fabsf(p_vy(cpart));
  if (cpart->ctype & 0x3FFFFFFF) wavelength_colour(cpart->ctype, colr, colg, colb);
  if (brightness > 0) {
    brightness /= 5;
    *firea = 255;
    *fireg = (int)(*colg * brightness);
    *fireb = (int)(*colb * brightness);
    *firer = (int)(*colr * brightness);
    *pixel_mode |= FIRE_ADD;
  }
  *pixel_mode |= PMODE_BLUR;
  return 0;
}

int el_BOMB_graphics(GRAPHICS_FUNC_ARGS) {
  *pixel_mode |= PMODE_FLARE;
  return 1;
}

int el_BRAY_graphics(GRAPHICS_FUNC_ARGS) {
  int trans = 255;
  if (cpart->tmp == 0) {
    trans = cpart->life * 7;
    if (trans > 255) trans = 255;
    if (cpart->ctype & 0x3FFFFFFF) wavelength_colour(cpart->ctype, colr, colg, colb);
  } else if (cpart->tmp == 1) {
    trans = cpart->life / 4;
    if (trans > 255) trans = 255;
    if (cpart->ctype & 0x3FFFFFFF) wavelength_colour(cpart->ctype, colr, colg, colb);
  } else if (cpart->tmp == 2) {
    trans = cpart->life * 100;
    if (trans > 255) trans = 255;
    *colr = 255;
    *colg = 150;
    *colb = 50;
  }
  *cola = trans;
  *pixel_mode &= ~PMODE;
  *pixel_mode |= PMODE_BLEND | PMODE_GLOW;
  return 0;
}

int el_BRCK_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp == 1) {
    *pixel_mode |= FIRE_ADD;
    *colb += 100;
    *firea = 40;
    *firer = *colr;
    *fireg = *colg;
    *fireb = *colb;
  }
  return 0;
}

int el_C5_graphics(GRAPHICS_FUNC_ARGS) {
  if (!cpart->ctype) return 0;
  wavelength_colour(cpart->ctype, colr, colg, colb);
  *firea = 100;
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  *pixel_mode &= ~PMODE_FLAT;
  *pixel_mode |= FIRE_ADD | PMODE_ADD | NO_DECO;
  return 0;
}

int el_CBNW_graphics(GRAPHICS_FUNC_ARGS) {
  int z = cpart->tmp2 - 20; /* speckles! */
  *colr += z * 1;
  *colg += z * 2;
  *colb += z * 8;
  return 0;
}

int el_CFLM_graphics(GRAPHICS_FUNC_ARGS) {
  uint32_t color = clfm_table_at(cpart->life / 2);
  *colr = color >> 16 & 255;
  *colg = color >> 8 & 255;
  *colb = color & 255;
  *firea = 255;
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  *pixel_mode = PMODE_NONE; /* Clear default, don't draw pixel */
  *pixel_mode |= FIRE_ADD;
  return 0;
}

int el_CLST_graphics(GRAPHICS_FUNC_ARGS) {
  int z = (cpart->tmp - 5) * 16; /* speckles! */
  *colr += z;
  *colg += z;
  *colb += z;
  return 0;
}

/* COAL and BCOL */
#define COAL_FREQUENCY (3.14159265f / (2 * 300.0f - (300.0f - 200.0f)))
int Element_COAL_graphics(GRAPHICS_FUNC_ARGS) {
  *colr += (int)((cpart->tmp2 - 295.15f) / 3);
  if (*colr > 170) *colr = 170;
  if (*colr < *colg) *colr = *colg;
  *colg = *colb = *colr;
  /* ((cpart->temp-295.15f) > 300.0f-200.0f) */
  if (p_temp(cpart) > 395.15f) {
    int q = (int)((p_temp(cpart) > 595.15f) ? 200.0f : p_temp(cpart) - 395.15f);
    *colr += (int)(sinf(COAL_FREQUENCY * q) * 226);
    *colg += (int)(-sinf(COAL_FREQUENCY * q * 4.55f) * 34);
    *colb += (int)(-sinf(COAL_FREQUENCY * q * 2.22f) * 64);
  }
  return 0;
}

int el_CRMC_graphics(GRAPHICS_FUNC_ARGS) {
  int z = (cpart->tmp2 - 2) * 8;
  *colr += z;
  *colg += z;
  *colb += z;
  return 0;
}

int el_DCEL_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp) *pixel_mode |= PMODE_GLOW;
  return 0;
}

int el_DEST_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->life) *pixel_mode |= PMODE_LFLARE;
  else *pixel_mode |= PMODE_SPARK;
  return 0;
}

int el_DEUT_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->life >= 240) {
    *firea = 60;
    *firer = *colr += 255;
    *fireg = *colg += 255;
    *fireb = *colb += 255;
    *pixel_mode |= PMODE_GLOW | FIRE_ADD;
  } else if (cpart->life > 0) {
    *colr += cpart->life * 1;
    *colg += cpart->life * 2;
    *colb += cpart->life * 3;
    *pixel_mode |= PMODE_BLUR;
  } else {
    *pixel_mode |= PMODE_BLUR;
  }
  return 0;
}

int el_DLAY_graphics(GRAPHICS_FUNC_ARGS) {
  int stage = (int)(((float)cpart->life / (p_temp(cpart) - 273.15f)) * 100.0f);
  *colr += stage;
  *colg += stage;
  *colb += stage;
  return 0;
}

int el_DMG_graphics(GRAPHICS_FUNC_ARGS) {
  *pixel_mode |= PMODE_FLARE;
  return 1;
}

int el_ELEC_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 70;
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  *pixel_mode |= FIRE_ADD;
  return 0;
}

int el_EMBR_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->ctype & 0xFFFFFF) {
    int maxComponent;
    *colr = (cpart->ctype & 0xFF0000) >> 16;
    *colg = (cpart->ctype & 0x00FF00) >> 8;
    *colb = (cpart->ctype & 0x0000FF);
    maxComponent = *colr;
    if (*colg > maxComponent) maxComponent = *colg;
    if (*colb > maxComponent) maxComponent = *colb;
    if (maxComponent < 60) { /* make sure it isn't too dark to see */
      float multiplier = 60.0f / maxComponent;
      *colr = (int)(*colr * multiplier);
      *colg = (int)(*colg * multiplier);
      *colb = (int)(*colb * multiplier);
    }
  } else if (cpart->tmp != 0) {
    *colr = *colg = *colb = 255;
  }
  uint32_t dc = part_dcolour(cpart);
  if (ren.decorations && dc && (dc & 0xFF000000)) {
    int a = (dc >> 24) & 0xFF;
    *colr = (a * ((dc >> 16) & 0xFF) + (255 - a) * *colr) >> 8;
    *colg = (a * ((dc >> 8) & 0xFF) + (255 - a) * *colg) >> 8;
    *colb = (a * ((dc)&0xFF) + (255 - a) * *colb) >> 8;
  }
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  if (cpart->tmp == 1) {
    *pixel_mode = FIRE_ADD | PMODE_BLEND | PMODE_GLOW;
    *firea = (cpart->life - 15) * 4;
    *cola = (cpart->life + 15) * 4;
  } else if (cpart->tmp == 2) {
    *pixel_mode = PMODE_FLAT | FIRE_ADD;
    *firea = 255;
  } else {
    *pixel_mode = PMODE_SPARK | PMODE_ADD;
    if (cpart->life < 64) *cola = 4 * cpart->life;
  }
  return 0;
}

int el_EMP_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->life) {
    *colr = (int)(cpart->life * 1.5f);
    *colg = (int)(cpart->life * 1.5f);
    *colb = 200 - (cpart->life);
  }
  return 0;
}

int el_EXOT_graphics(GRAPHICS_FUNC_ARGS) {
  float q = p_temp(cpart);
  int b = cpart->tmp, c = cpart->tmp2;
  if (cpart->life < 1001) {
    if (ren_rng_chance(cpart->tmp2 - 1, 1000)) {
      float frequency = 0.04045f;
      *colr = (int)(sinf(frequency * c + 4) * 127 + 150);
      *colg = (int)(sinf(frequency * c + 6) * 127 + 150);
      *colb = (int)(sinf(frequency * c + 8) * 127 + 150);
      *firea = 100;
      *firer = 0;
      *fireg = 0;
      *fireb = 0;
      *pixel_mode |= PMODE_FLAT;
      *pixel_mode |= PMODE_FLARE;
    } else {
      float frequency = 0.00045f;
      *colr = (int)(sinf(frequency * q + 4) * 127 + (b / 1.7f));
      *colg = (int)(sinf(frequency * q + 6) * 127 + (b / 1.7f));
      *colb = (int)(sinf(frequency * q + 8) * 127 + (b / 1.7f));
      *cola = cpart->tmp / 6;
      *firea = *cola;
      *firer = *colr;
      *fireg = *colg;
      *fireb = *colb;
      *pixel_mode |= FIRE_ADD;
      *pixel_mode |= PMODE_BLUR;
    }
  } else {
    float frequency = 0.01300f;
    *colr = (int)(sinf(frequency * q + 6.00f) * 127 + ((b / 2.9f) + 80));
    *colg = (int)(sinf(frequency * q + 6.00f) * 127 + ((b / 2.9f) + 80));
    *colb = (int)(sinf(frequency * q + 6.00f) * 127 + ((b / 2.9f) + 80));
    *cola = cpart->tmp / 6;
    *firea = *cola;
    *firer = *colr;
    *fireg = *colg;
    *fireb = *colb;
    *pixel_mode |= FIRE_ADD;
    *pixel_mode |= PMODE_BLUR;
  }
  return 0;
}

/* Element_FILT_getWavelengths */
static int filt_wavelengths(const Particle *cpart) {
  if (cpart->ctype & 0x3FFFFFFF) return cpart->ctype;
  int temp_bin = (int)((p_temp(cpart) - 273.0f) * 0.025f);
  if (temp_bin < 0) temp_bin = 0;
  if (temp_bin > 25) temp_bin = 25;
  return 0x1F << temp_bin;
}
int el_FILT_graphics(GRAPHICS_FUNC_ARGS) {
  wavelength_colour(filt_wavelengths(cpart), colr, colg, colb);
  if (cpart->life > 0 && cpart->life <= 4) *cola = 127 + cpart->life * 30;
  else *cola = 127;
  *pixel_mode &= ~PMODE;
  *pixel_mode |= PMODE_BLEND;
  return 0;
}

int el_FIRE_graphics(GRAPHICS_FUNC_ARGS) {
  uint32_t color = flame_table_at(cpart->life);
  *colr = color >> 16 & 255;
  *colg = color >> 8 & 255;
  *colb = color & 255;
  *firea = 255;
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  *pixel_mode = PMODE_NONE; /* Clear default, don't draw pixel */
  *pixel_mode |= FIRE_ADD;
  return 0;
}

int el_FIRW_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp > 0) *pixel_mode |= PMODE_GLOW;
  return 0;
}

int el_FRME_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->tmp) {
    *colr += 30;
    *colg += 30;
    *colb += 30;
  }
  return 0;
}

int el_GBMB_graphics(GRAPHICS_FUNC_ARGS) {
  if (cpart->life <= 0) *pixel_mode |= PMODE_FLARE;
  else *pixel_mode |= PMODE_SPARK;
  return 0;
}

int el_GEL_graphics(GRAPHICS_FUNC_ARGS) {
  int q = cpart->tmp;
  *colr = q * (32 - 255) / 120 + 255;
  *colg = q * (48 - 186) / 120 + 186;
  *colb = q * 208 / 120;
  *pixel_mode |= PMODE_BLUR;
  return 0;
}

int el_GLOW_graphics(GRAPHICS_FUNC_ARGS) {
  *firer = 16 + (int)(restrict_flt(p_temp(cpart) - (273.15f + 34.0f), 0, 128) / 2.0f);
  *fireg = 16 + (int)(restrict_flt((float)cpart->ctype, 0, 128) / 2.0f);
  *fireb = 16 + (int)(restrict_flt((float)cpart->tmp, 0, 128) / 2.0f);
  *firea = 64;
  *colr = (int)restrict_flt(64.0f + p_temp(cpart) - (273.15f + 34.0f), 0, 255);
  *colg = (int)restrict_flt(64.0f + cpart->ctype, 0, 255);
  *colb = (int)restrict_flt(64.0f + cpart->tmp, 0, 255);
  int rng = ren_rng_between(1, 32);
  if (((*colr) + (*colg) + (*colb)) > (256 + rng)) {
    *colr -= 54;
    *colg -= 54;
    *colb -= 54;
    *pixel_mode |= FIRE_ADD;
    *pixel_mode |= PMODE_GLOW | PMODE_ADD;
    *pixel_mode &= ~PMODE_FLAT;
  } else {
    *pixel_mode |= PMODE_BLUR;
  }
  return 0;
}

int el_GOLD_graphics(GRAPHICS_FUNC_ARGS) {
  int rndstore = (int)ren_rng_gen();
  *colr += (rndstore % 10) - 5;
  rndstore >>= 4;
  *colg += (rndstore % 10) - 5;
  rndstore >>= 4;
  *colb += (rndstore % 10) - 5;
  return 0;
}

int el_GPMP_graphics(GRAPHICS_FUNC_ARGS) {
  int lifemod = ((cpart->life > 10 ? 10 : cpart->life) * 19);
  *colg += lifemod;
  *colb += lifemod;
  return 0;
}

int el_GRAV_graphics(GRAPHICS_FUNC_ARGS) {
  int GRAV_R, GRAV_B, GRAV_G, GRAV_R2, GRAV_B2, GRAV_G2;
  GRAV_R = abs((currentTick % 120) - 60);
  GRAV_G = abs(((currentTick + 60) % 120) - 60);
  GRAV_B = abs(((currentTick + 120) % 120) - 60);
  GRAV_R2 = abs((currentTick % 60) - 30);
  GRAV_G2 = abs(((currentTick + 30) % 60) - 30);
  GRAV_B2 = abs(((currentTick + 60) % 60) - 30);
  float vx_ = p_vx(cpart), vy_ = p_vy(cpart);
  *colr = 20;
  *colg = 20;
  *colb = 20;
  if (vx_ > 0) {
    *colr += (int)(vx_ * GRAV_R);
    *colg += (int)(vx_ * GRAV_G);
    *colb += (int)(vx_ * GRAV_B);
  }
  if (vy_ > 0) {
    *colr += (int)(vy_ * GRAV_G);
    *colg += (int)(vy_ * GRAV_B);
    *colb += (int)(vy_ * GRAV_R);
  }
  if (vx_ < 0) {
    *colr -= (int)(vx_ * GRAV_B);
    *colg -= (int)(vx_ * GRAV_R);
    *colb -= (int)(vx_ * GRAV_G);
  }
  if (vy_ < 0) {
    *colr -= (int)(vy_ * GRAV_R2);
    *colg -= (int)(vy_ * GRAV_G2);
    *colb -= (int)(vy_ * GRAV_B2);
  }
  if (cpart->life) {
    *pixel_mode = FIRE_ADD | PMODE_ADD | PMODE_GLOW | PMODE_FLARE;
    *firer = *colr * 3 < 255 ? *colr * 3 : 255;
    *fireg = *colg * 3 < 255 ? *colg * 3 : 255;
    *fireb = *colb * 3 < 255 ? *colb * 3 : 255;
    *firea = (cpart->life + 15) * 4;
    *cola = (cpart->life + 15) * 4;
  } else {
    *pixel_mode = PMODE_ADD;
  }
  return 0;
}

int el_GRVT_graphics(GRAPHICS_FUNC_ARGS) {
  *firea = 5;
  *firer = 0;
  *fireg = 250;
  *fireb = 170;
  *pixel_mode |= FIRE_BLEND;
  return 1;
}

int el_PHOT_graphics(GRAPHICS_FUNC_ARGS) {
  wavelength_colour(cpart->ctype, colr, colg, colb);
  *firea = 100;
  *firer = *colr;
  *fireg = *colg;
  *fireb = *colb;
  *pixel_mode &= ~PMODE_FLAT;
  *pixel_mode |= FIRE_ADD | PMODE_ADD | NO_DECO;
  if (cpart->flags & FLAG_PHOTDECO) *pixel_mode &= ~NO_DECO;
  return 0;
}
