/* The End: ChunkProviderEnd, from NumPlay 1.17's gen_dims.h, adapted to this
 * fork's 9-bit slab. It shares gen.c's noise, Java random and scratch memory; the
 * End's own octave states live in the summary cache (the End has no summaries). */
#ifndef NB_GEN_END_H
#define NB_GEN_END_H

/* ---------------------------------------------------------------- noise: NoiseGeneratorOctaves, any size */
/* n octaves of a NoiseGeneratorOctaves (their states st) at grid origin (bx, by, bz), xs x ys x zs samples
 * (ys 1: the 2D noise) spaced sx, sy, sz (Q32), added into out[(x * zs + z) * ys + y] */
static void gd_octaves(const Octave *oc, int n, float *out, i64 bx, i64 by, i64 bz, int xs, int ys, int zs, u64 sx,
                       u64 sy, u64 sz) {
    uint8_t P[256];
    int xc[16], zc[16], yc[33];
    float xf[16], zf[16], yf[33];
    for (int o = 0; o < n; o++) {
        i64 off[3];
        perm_build(oc[o].st, P, off);
        float amp = (float)(1 << o);
        for (int k = 0; k < xs; k++) coord_split(coord_q32(bx, sx, o, k, off[0]), &xc[k], &xf[k]);
        for (int k = 0; k < zs; k++) coord_split(coord_q32(bz, sz, o, k, off[2]), &zc[k], &zf[k]);
        if (ys == 1) {
            /* NoiseGeneratorImproved's 2D path (no y) */
            int idx = 0;
            for (int x = 0; x < xs; x++) {
                float fx = xf[x], ux = fade(fx);
                int i3 = xc[x];
                for (int z = 0; z < zs; z++, idx++) {
                    float fz = zf[z], uz = fade(fz);
                    int l1 = zc[z];
                    int j3 = P[P[i3] & 255] + l1, i1 = P[P[(i3 + 1) & 255] & 255] + l1;
                    float a = lerp(ux, grad3(P[j3 & 255], fx, 0, fz), grad3(P[i1 & 255], fx - 1, 0, fz));
                    float b = lerp(ux, grad3(P[(j3 + 1) & 255], fx, 0, fz - 1), grad3(P[(i1 + 1) & 255], fx - 1, 0, fz - 1));
                    out[idx] += lerp(uz, a, b) * amp;
                }
            }
            continue;
        }
        for (int k = 0; k < ys; k++) coord_split(coord_q32(by, sy, o, k, off[1]), &yc[k], &yf[k]);
        int idx = 0;
        for (int x = 0; x < xs; x++) {
            float fx = xf[x], ux = fade(fx), gx1 = fx - 1;
            int pa = P[xc[x]], pb = P[(xc[x] + 1) & 255];
            for (int z = 0; z < zs; z++) {
                float fz = zf[z], uz = fade(fz), gz1 = fz - 1;
                int j4 = zc[z], cur = -1;
                float d16 = 0, d7 = 0, d17 = 0, d8 = 0;
                for (int y = 0; y < ys; y++, idx++) {
                    /* Java's stale gradients: recomputed only when the y cell changes (and at a column's start),
                     * with the y fraction of the first sample in the cell */
                    if (y == 0 || yc[y] != cur) {
                        cur = yc[y];
                        float fy = yf[y], gy1 = fy - 1;
                        int j5 = pa + cur, i6 = pb + cur;
                        int k5 = P[j5 & 255] + j4, l5 = P[(j5 + 1) & 255] + j4;
                        int i2 = P[i6 & 255] + j4, j6 = P[(i6 + 1) & 255] + j4;
                        d16 = lerp(ux, grad3(P[k5 & 255], fx, fy, fz), grad3(P[i2 & 255], gx1, fy, fz));
                        d7 = lerp(ux, grad3(P[l5 & 255], fx, gy1, fz), grad3(P[j6 & 255], gx1, gy1, fz));
                        d17 = lerp(ux, grad3(P[(k5 + 1) & 255], fx, fy, gz1), grad3(P[(i2 + 1) & 255], gx1, fy, gz1));
                        d8 = lerp(ux, grad3(P[(l5 + 1) & 255], fx, gy1, gz1), grad3(P[(j6 + 1) & 255], gx1, gy1, gz1));
                    }
                    float uy = fade(yf[y]);
                    out[idx] += lerp(uz, lerp(uy, d16, d7), lerp(uy, d17, d8)) * amp;
                }
            }
        }
    }
}


typedef struct {
    Octave oc[40];   /* noiseGen1 (16), noiseGen2 (16), noiseGen3 (8) */
} EndMem;
_Static_assert(sizeof(EndMem) <= GEN_CACHE_BYTES, "the End's state fits in the summary cache");
#define EM ((EndMem *)(void *)sumc)
/* (the pillars live after the End's octaves in the same summary cache: gen_spikes is
 * called from end_tick, where the generator's scratch is not on the stack) */
static u64 SC_E1XZ, SC_E3XZ, SC_E3Y;

static void end_init(void) {
    EndMem *m = EM;
    memset(m, 0, sizeof *m);
    JRand r;
    jr_seed(&r, g_seed);
    for (int i = 0; i < 40; i++) m->oc[i].st = r.s, perm_skip(&r);
    SC_E1XZ = FX(684.412 * 2.0);
    SC_E3XZ = FX(684.412 * 2.0 / 80.0);
    SC_E3Y = FX(684.412 / 160.0);
}

/* initializeNoiseField: chunk (cx, cz)'s 3 x 33 x 3 densities (cells of 8 blocks across, 4 up), with the
 * island's falloff from (0, 0) */
static void end_grid(int cx, int cz, float *g) {
    EndMem *m = EM;
    float n1[297], n2[297], n3[297];
    memset(n1, 0, sizeof n1);
    memset(n2, 0, sizeof n2);
    memset(n3, 0, sizeof n3);
    gd_octaves(m->oc + 32, 8, n3, cx * 2, 0, cz * 2, 3, 33, 3, SC_E3XZ, SC_E3Y, SC_E3XZ);
    gd_octaves(m->oc, 16, n1, cx * 2, 0, cz * 2, 3, 33, 3, SC_E1XZ, FX(684.412), SC_E1XZ);
    gd_octaves(m->oc + 16, 16, n2, cx * 2, 0, cz * 2, 3, 33, 3, SC_E1XZ, FX(684.412), SC_E1XZ);
    int i = 0;
    for (int j = 0; j < 3; j++)
        for (int k = 0; k < 3; k++) {
            float f = (float)(j + cx * 2), f1 = (float)(k + cz * 2);
            float f2 = 100.0f - sqrtf(f * f + f1 * f1) * 8.0f;
            if (f2 > 80.0f) f2 = 80.0f;
            if (f2 < -100.0f) f2 = -100.0f;
            for (int l = 0; l < 33; l++, i++) {
                float d3 = n1[i] / 512.0f, d4 = n2[i] / 512.0f, d5 = (n3[i] / 10.0f + 1.0f) / 2.0f, d2;
                if (d5 < 0.0f) d2 = d3;
                else if (d5 > 1.0f) d2 = d4;
                else d2 = d3 + (d4 - d3) * d5;
                d2 -= 8.0f;
                d2 += f2;
                if (l > 33 / 2 - 2) {
                    float d6 = (float)(l - (33 / 2 - 2)) / 64.0f;
                    d6 = d6 < 0 ? 0 : d6 > 1 ? 1 : d6;
                    d2 = d2 * (1.0f - d6) + -3000.0f * d6;
                }
                if (l < 8) {
                    float d7 = (float)(8 - l) / 7.0f;
                    d2 = d2 * (1.0f - d7) + -30.0f * d7;
                }
                g[i] = d2;
            }
        }
}

static void end_decorate(int cx, int cz, int y0, int h, uint8_t *out);

/* the density at one place of the grid g of its chunk, summed as func_180520_a sums it: end stone or not */
static int end_point(const float *g, int lx, int lz, int y) {
    int j1 = lx >> 3, k1 = lz >> 3, l1 = y >> 2;
    float d1 = g[((j1 + 0) * 3 + k1 + 0) * 33 + l1], d2 = g[((j1 + 0) * 3 + k1 + 1) * 33 + l1];
    float d3 = g[((j1 + 1) * 3 + k1 + 0) * 33 + l1], d4 = g[((j1 + 1) * 3 + k1 + 1) * 33 + l1];
    float d5 = (g[((j1 + 0) * 3 + k1 + 0) * 33 + l1 + 1] - d1) * 0.25f;
    float d6 = (g[((j1 + 0) * 3 + k1 + 1) * 33 + l1 + 1] - d2) * 0.25f;
    float d7 = (g[((j1 + 1) * 3 + k1 + 0) * 33 + l1 + 1] - d3) * 0.25f;
    float d8 = (g[((j1 + 1) * 3 + k1 + 1) * 33 + l1 + 1] - d4) * 0.25f;
    for (int i2 = 0; i2 < (y & 3); i2++) d1 += d5, d2 += d6, d3 += d7, d4 += d8;
    float d10 = d1, d11 = d2, d12 = (d3 - d1) * 0.125f, d13 = (d4 - d2) * 0.125f;
    for (int j2 = 0; j2 < (lx & 7); j2++) d10 += d12, d11 += d13;
    float d15 = d10, d16 = (d11 - d10) * 0.125f;
    for (int k2 = 0; k2 < (lz & 7); k2++) d15 += d16;
    return d15 > 0.0f;
}
/* the End's terrain at one place (a grid kept for its chunk in g, gx, gz) */
static int end_solid(float *g, int *gc, int x, int y, int z) {
    if (y < 0 || y >= 128) return 0;
    int cx = x >> 4, cz = z >> 4;
    if (cx * cx + cz * cz > 30 * 30) return 0;
    if (gc[2] != 1 || gc[0] != cx || gc[1] != cz) end_grid(cx, cz, g), gc[0] = cx, gc[1] = cz, gc[2] = 1;
    return end_point(g, x & 15, z & 15, y);
}

/* BiomeEndDecorator: a chunk in five has an obsidian pillar (WorldGenSpikes) where its decoration lands on
 * end stone: 6 to 37 high, 1 to 4 across, bedrock on top (its crystal: end.c). 1.8.8 decorates the End with
 * the world's random; here each chunk's is its own, so the pillars are the same every time: listed once,
 * in U (the End has no other use for it) */
typedef struct {
    int n, ok;
    EndSpike s[64];
} SpikeList;
#define SPK ((SpikeList *)(void *)((uint8_t *)sumc + sizeof(EndMem)))
const EndSpike *gen_spikes(int *n) {
    SpikeList *l = SPK;
    if (!l->ok) {
        l->ok = 1, l->n = 0;
        float g[297];
        int gc[3] = {0, 0, 0};
        for (int cz = -8; cz <= 8; cz++)
            for (int cx = -8; cx <= 8; cx++) {
                JRand r;
                jr_seed(&r, ((i64)cx * 341873128712LL + (i64)cz * 132897987541LL) ^ g_seed);
                if (jr_int(&r, 5) != 0) continue;
                int x = cx * 16 + jr_int(&r, 16) + 8, z = cz * 16 + jr_int(&r, 16) + 8, y = 127;
                while (y >= 0 && !end_solid(g, gc, x, y, z)) y--;
                if (y < 0 || ++y >= 127) continue;
                int h = jr_int(&r, 32) + 6, rad = jr_int(&r, 4) + 1, ok = 1;
                for (int k = x - rad; k <= x + rad && ok; k++)
                    for (int l = z - rad; l <= z + rad && ok; l++)
                        if ((k - x) * (k - x) + (l - z) * (l - z) <= rad * rad + 1 && !end_solid(g, gc, k, y - 1, l)) ok = 0;
                if (!ok || l->n >= 64) continue;
                if (y + h > 127) h = 127 - y;
                l->s[l->n++] = (EndSpike){(int16_t)x, (int16_t)z, (uint8_t)y, (uint8_t)h, (uint8_t)rad, 0};
            }
    }
    *n = l->n;
    return l->s;
}
/* what the End has there as made (end stone, a pillar, air) */
int gen_end_block(int x, int y, int z) {
    int n;
    const EndSpike *s = gen_spikes(&n);
    for (int i = 0; i < n; i++) {
        int dx = x - s[i].x, dz = z - s[i].z;
        if (dx * dx + dz * dz <= s[i].r * s[i].r + 1 && y >= s[i].y && y <= s[i].y + s[i].h)
            return y == s[i].y + s[i].h ? B_BEDROCK : B_OBSIDIAN;
    }
    float g[297];
    int gc[3] = {0, 0, 0};
    return end_solid(g, gc, x, y, z) ? B_END_STONE : B_AIR;
}

/* func_180520_a: end stone where the density is above 0 (the surface pass changes nothing there) */
static void end_slab(int cx, int cz, int y0, int h, uint8_t *out) {
    float g[297];
    if (cx * cx + cz * cz > 30 * 30) {   /* (far out, the falloff leaves the void: no noise needed) */
        memset(out, 0, (size_t)h * 256 / 8 * 9);
        return;
    }
    end_grid(cx, cz, g);
    for (int j1 = 0; j1 < 2; j1++)
        for (int k1 = 0; k1 < 2; k1++)
            for (int l1 = 0; l1 < 32; l1++) {
                float d1 = g[((j1 + 0) * 3 + k1 + 0) * 33 + l1], d2 = g[((j1 + 0) * 3 + k1 + 1) * 33 + l1];
                float d3 = g[((j1 + 1) * 3 + k1 + 0) * 33 + l1], d4 = g[((j1 + 1) * 3 + k1 + 1) * 33 + l1];
                float d5 = (g[((j1 + 0) * 3 + k1 + 0) * 33 + l1 + 1] - d1) * 0.25f;
                float d6 = (g[((j1 + 0) * 3 + k1 + 1) * 33 + l1 + 1] - d2) * 0.25f;
                float d7 = (g[((j1 + 1) * 3 + k1 + 0) * 33 + l1 + 1] - d3) * 0.25f;
                float d8 = (g[((j1 + 1) * 3 + k1 + 1) * 33 + l1 + 1] - d4) * 0.25f;
                for (int i2 = 0; i2 < 4; i2++) {
                    float d10 = d1, d11 = d2, d12 = (d3 - d1) * 0.125f, d13 = (d4 - d2) * 0.125f;
                    int y = l1 * 4 + i2 - y0;
                    for (int j2 = 0; j2 < 8; j2++) {
                        float d15 = d10, d16 = (d11 - d10) * 0.125f;
                        for (int k2 = 0; k2 < 8; k2++) {
                            if (y >= 0 && y < h)
                                b9_set(out, (y * 16 + k2 + k1 * 8) * 16 + j2 + j1 * 8,
                                       d15 > 0.0f ? B_END_STONE : B_AIR);
                            d15 += d16;
                        }
                        d10 += d12, d11 += d13;
                    }
                    d1 += d5, d2 += d6, d3 += d7, d4 += d8;
                }
            }
    end_decorate(cx, cz, y0, h, out);
}

static void end_decorate(int cx, int cz, int y0, int h, uint8_t *out) {
    int n;
    const EndSpike *s = gen_spikes(&n);
    for (int i = 0; i < n; i++) {
        const EndSpike *p = &s[i];
        if (p->x + p->r < cx * 16 || p->x - p->r > cx * 16 + 15 || p->z + p->r < cz * 16 || p->z - p->r > cz * 16 + 15) continue;
        for (int y = p->y; y <= p->y + p->h; y++) {
            if (y < y0 || y >= y0 + h) continue;
            for (int z = p->z - p->r; z <= p->z + p->r; z++)
                for (int x = p->x - p->r; x <= p->x + p->r; x++) {
                    int lx = x - cx * 16, lz = z - cz * 16;
                    if ((unsigned)lx > 15 || (unsigned)lz > 15) continue;
                    if ((x - p->x) * (x - p->x) + (z - p->z) * (z - p->z) > p->r * p->r + 1) continue;
                    b9_set(out, ((y - y0) * 16 + lz) * 16 + lx, y == p->y + p->h ? B_BEDROCK : B_OBSIDIAN);
                }
        }
    }
}


#endif
