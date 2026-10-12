/* The player's changes to the world, saved as they happen.
 *
 * The world is cut in regions of 4 x 4 chunks (64 x 64 columns). Each region
 * the player changed has a record in the calculator's storage, read where it
 * lies: sorted 32-bit entries, chunk (4 bits), x and z in it (8), y (7), then
 * the block (9 bits, at the top: block states are 9 bits each, nb.h). New
 * changes go to a small journal in RAM, kept in the same order; when it fills
 * up (and when the game saves), its entries are merged into the records.
 *
 * The two dimensions keep their own records: the region's name says which
 * ("nb1r-3_12.nbe" overworld, "nb1n-3_12.nbe" nether), so the nether's edits
 * never mix with the overworld's. */
#include "edits.h"
#include "nb.h"
#pragma GCC optimize("Os")   /* (not where the time goes: small) */

typedef struct {
  int16_t rx, rz;
  uint32_t e;   /* the entry, as in the records */
} Journal;

#define J_MAX 128   /* 128 x 8 B = 1 KB; flushed to storage when full */
static Journal jn[J_MAX];
static int nj;
static char prefix[8] = "nb1";
static bool full;   /* a record could not grow: storage is full */
static uint32_t *scratch;
static uint32_t scratch_n;
static char edit_dim = 'r';   /* whose records these are: 'r' overworld, 'n' nether */

/* a region's record (the last one looked up is remembered until records change) */
static bool cache_ok;
static int cache_rx, cache_rz, cache_n;
static const uint8_t *cache_r;

void edits_setup(const char *world, uint32_t *buf, uint32_t n) {
  int i = 0;
  for (; world[i] && i < 7; i++) prefix[i] = world[i];
  prefix[i] = 0;
  scratch = buf;
  scratch_n = n;
}
bool edits_full(void) { return full; }

/* the dimension the player's changes go to: the world's (DIM_OVER, DIM_NETHER).
 * The journal is flushed first: its entries are this dimension's (they are written
 * to its records), so they never travel to the other one's */
void edits_set_dim(int dim) {
  if (nj) edits_flush();
  edit_dim = dim ? 'n' : 'r';
  cache_ok = false;
}

static inline int floordiv(int a, int d) { return a >= 0 ? a / d : -((-a + d - 1) / d); }

/* "nb1r-3_12.nbe": the world's prefix, the dimension's mark, then the region */
static void region_name_d(char *out, int rx, int rz, char dim) {
  char *p = out;
  for (const char *s = prefix; *s;) *p++ = *s++;
  *p++ = dim;
  int v[2] = {rx, rz};
  for (int k = 0; k < 2; k++) {
    if (k) *p++ = '_';
    int n = v[k];
    if (n < 0) *p++ = '-', n = -n;
    char t[8];
    int m = 0;
    do t[m++] = (char)('0' + n % 10), n /= 10;
    while (n);
    while (m) *p++ = t[--m];
  }
  const char *ext = ".nbe";
  while (*ext) *p++ = *ext++;
  *p = 0;
}
static void region_name(char *out, int rx, int rz) { region_name_d(out, rx, rz, edit_dim); }

static const uint8_t *region(int rx, int rz, int *n) {
  if (!cache_ok || rx != cache_rx || rz != cache_rz) {
    char name[32];
    region_name(name, rx, rz);
    uint32_t len = 0;
    const uint8_t *d = plat_load(name, &len);
    cache_r = d;
    cache_n = d ? (int)(len / 4) : 0;
    cache_rx = rx, cache_rz = rz, cache_ok = true;
  }
  *n = cache_n;
  return cache_r;
}

void edits_clear(void) { nj = 0, full = false, cache_ok = false; }
void edits_forget(void) { cache_ok = false; }

/* records are not aligned in storage: entries are read a byte at a time */
static inline uint32_t rd32(const uint8_t *p) {
  uint32_t v;
  memcpy(&v, p, 4);
  return v;
}
#define R(i) rd32(r + 4 * (i))

/* entries are sorted by their low 19 bits (the position: chunk, x and z in it, y);
 * the block state is the 9 bits above them, so a 32-bit entry holds both */
#define KEY(e) ((e) & 0x7FFFF)
#define BLK(e) ((int)((e) >> 19) & 511)
#define ENTRY(key, b) ((key) | (uint32_t)(b) << 19)
static inline uint32_t entry_key(int x, int y, int z) {
  int cx = floordiv(x, 16), cz = floordiv(z, 16);
  int chunk = (cx & 3) | (cz & 3) << 2;
  int xz = (x & 15) | (z & 15) << 4;
  return (uint32_t)(chunk << 15 | xz << 7 | (y & 127));
}

static int jcmp(int rx, int rz, uint32_t key, const Journal *j) {
  if (rz != j->rz) return rz < j->rz ? -1 : 1;
  if (rx != j->rx) return rx < j->rx ? -1 : 1;
  uint32_t k = KEY(j->e);
  return key < k ? -1 : key > k ? 1 : 0;
}
static int jlower(int rx, int rz, uint32_t key) {
  int lo = 0, hi = nj;
  while (lo < hi) {
    int m = (lo + hi) / 2;
    if (jcmp(rx, rz, key, &jn[m]) > 0) lo = m + 1;
    else hi = m;
  }
  return lo;
}
static int rlower(const uint8_t *r, int n, uint32_t key) {
  int lo = 0, hi = n;
  while (lo < hi) {
    int m = (lo + hi) / 2;
    if (KEY(R(m)) < key) lo = m + 1;
    else hi = m;
  }
  return lo;
}

/* the journal's entries go into their regions' records */
bool edits_flush(void) {
  bool ok = true;
  int i = 0;
  while (i < nj) {
    int rx = jn[i].rx, rz = jn[i].rz, j = i;
    while (j < nj && jn[j].rx == rx && jn[j].rz == rz) j++;
    int n;
    const uint8_t *r = region(rx, rz, &n);
    /* merge the record and the journal's entries for it into the scratch buffer */
    uint32_t m = 0;
    int a = 0, b = i;
    while ((a < n || b < j) && m < scratch_n) {
      if (b >= j || (a < n && KEY(R(a)) < KEY(jn[b].e))) scratch[m++] = R(a), a++;
      else {
        if (a < n && KEY(R(a)) == KEY(jn[b].e)) a++;
        scratch[m++] = jn[b++].e;
      }
    }
    char name[32];
    region_name(name, rx, rz);
    cache_ok = false;
    if (a < n || b < j || !plat_save(name, scratch, m * 4)) {
      /* no room: these changes stay in the journal */
      ok = false;
      full = true;
      i = j;
      continue;
    }
    memmove(&jn[i], &jn[j], (size_t)(nj - j) * sizeof jn[0]);
    nj -= j - i;
  }
  return ok;
}

bool edits_put(int x, int y, int z, int b) {
  int rx = floordiv(x, 64), rz = floordiv(z, 64);
  uint32_t key = entry_key(x, y, z);
  int i = jlower(rx, rz, key);
  if (i < nj && jcmp(rx, rz, key, &jn[i]) == 0) {
    jn[i].e = ENTRY(key, b);
    return true;
  }
  if (nj >= J_MAX) {
    edits_flush();
    if (nj >= J_MAX) return false;
    i = jlower(rx, rz, key);
  }
  memmove(&jn[i + 1], &jn[i], (size_t)(nj - i) * sizeof jn[0]);
  jn[i] = (Journal){(int16_t)rx, (int16_t)rz, ENTRY(key, b)};
  nj++;
  return true;
}

/* onto a chunk from gen_slab: the record's entries, then the journal's */
void edits_apply(int cx, int cz, int y0, int h, uint8_t *slab) {
  int rx = floordiv(cx, 4), rz = floordiv(cz, 4), chunk = (cx & 3) | (cz & 3) << 2;
  uint32_t k0 = (uint32_t)chunk << 15, k1 = k0 + (1 << 15);
  int n;
  const uint8_t *r = region(rx, rz, &n);
  for (int i = rlower(r, n, k0); i < n && KEY(R(i)) < k1; i++) {
    uint32_t e = R(i);
    int y = (int)(e & 127) - y0, xz = (int)(e >> 7) & 255;
    if (y >= 0 && y < h) b9_set(slab, (y * 16 + (xz >> 4)) * 16 + (xz & 15), BLK(e));
  }
  for (int i = jlower(rx, rz, k0); i < nj && jn[i].rx == rx && jn[i].rz == rz && KEY(jn[i].e) < k1; i++) {
    int y = (int)(jn[i].e & 127) - y0, xz = (int)(jn[i].e >> 7) & 255;
    if (y >= 0 && y < h) b9_set(slab, (y * 16 + (xz >> 4)) * 16 + (xz & 15), BLK(jn[i].e));
  }
}

/* the column's sky top (1 + the highest opaque y) with the changes */
int edits_top(int x, int z, int top) {
  int rx = floordiv(x, 64), rz = floordiv(z, 64);
  uint32_t k0 = entry_key(x, 0, z), k1 = k0 + 128;
  int n;
  const uint8_t *r = region(rx, rz, &n);
  for (int i = rlower(r, n, k0); i < n && KEY(R(i)) < k1; i++) {
    uint32_t e = R(i);
    if ((blk_flags[BLK(e)] & BF_OPAQUE) && (int)(e & 127) + 1 > top) top = (int)(e & 127) + 1;
  }
  for (int i = jlower(rx, rz, k0); i < nj && jn[i].rx == rx && jn[i].rz == rz && KEY(jn[i].e) < k1; i++)
    if ((blk_flags[BLK(jn[i].e)] & BF_OPAQUE) && (int)(jn[i].e & 127) + 1 > top) top = (int)(jn[i].e & 127) + 1;
  return top;
}

int edits_count(void) { return nj; }

/* the merge buffer, lent out between merges (n bytes at most) */
void *edits_scratch(uint32_t n) { return n <= scratch_n * 4 ? (void *)scratch : NULL; }

/* ---------------------------------------------------------------- old records
 * The records a world saved before the states were 9 bits (its head is NBW1..4)
 * hold the position in 24 bits and the block in the top byte. They are read
 * again and written back with the block at bits 19.., once per world (the head
 * is then saved as NBW5, which is what says it has been done). */
#define ECONV_MAX 32
bool edits_convert(void) {
  bool ok = true;
  char names[ECONV_MAX][32];
  int nn = 0;
  int pl = (int)strlen(prefix);
  char rec[40];
  for (int i = 0; i < 64 && nn < ECONV_MAX; i++) {
    if (!plat_record(i, rec, sizeof rec)) break;
    /* the world's own: prefix + "r<region>.nbe" (the overworld's; the nether's are
     * written 9-bit from the start, so they are left alone) */
    if (strncmp(rec, prefix, (size_t)pl) || rec[pl] != 'r') continue;
    int k = pl + 1;
    bool shape = rec[k] == '-' || (rec[k] >= '0' && rec[k] <= '9');
    while (rec[k] && (rec[k] == '-' || (rec[k] >= '0' && rec[k] <= '9') || rec[k] == '_')) k++;
    if (shape && !strcmp(rec + k, ".nbe")) memcpy(names[nn++], rec, 32);
  }
  for (int i = 0; i < nn; i++) {
    uint32_t len = 0;
    const uint8_t *r = plat_load(names[i], &len);
    if (!r || !len || len % 4 || len / 4 > scratch_n) continue;
    int n = (int)(len / 4);
    for (int k = 0; k < n; k++) {
      uint32_t e = rd32(r + 4 * k);
      uint32_t key = e & 0xFFFFFF, blk = e >> 24;   /* the old shape: 24-bit position, block on top */
      if (key >> 19 || blk >= NSTATES) {
        ok = false;   /* not an old entry: leave this record alone */
        break;
      }
      scratch[k] = ENTRY(key, blk);
    }
    if (!plat_save(names[i], scratch, len)) ok = false;
  }
  edits_forget();
  return ok;
}
