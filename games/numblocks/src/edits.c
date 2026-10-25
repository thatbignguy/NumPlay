/* The player's changes to the world, saved as they happen.
 *
 * The world is cut in regions of 4 x 4 chunks (64 x 64 columns). Each region
 * the player changed has a record in the calculator's storage, read where it
 * lies: sorted 32-bit entries, chunk (4 bits), x and z in it (8), y (8), then
 * the block (9 bits, at the top: block states are 9 bits each, nb.h). The y is
 * 8 bits because a dimension's build height can pass 127: the nether's is 200,
 * and the overworld's floor (the deepslate layer, nb.h's WORLD_MIN_Y) is -64, so
 * its records store the y based on it (0..191).
 * New changes go to a small journal in RAM, kept in the same order; when it fills
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
static char edit_dim = 'r';   /* whose records these are: 'r' overworld, 'n' nether, 'e' the End */

/* The merge: a record's entries and the journal's, in order, on their way into the
 * record. It has to hold the whole of a region's record, so it is sized for one:
 * 64 x 64 columns take 4096 entries at a row each, and a record that has outgrown
 * it cannot be merged at all - and a merge that does not happen loses the changes,
 * because the journal is then the only place they live and it holds 128. (It is a
 * buffer of its own rather than the chunk slab, which is the same memory the slab
 * is written into while a chunk is made.) */
#define MERGE_MAX 2660   /* a region's record plus the journal's entries: the merge holds
 * both, and a record that has outgrown it makes way for the journal's (edits_flush) */
static uint32_t merge[MERGE_MAX];

/* a region's record (the last one looked up is remembered until records change) */
static bool cache_ok;
static int cache_rx, cache_rz, cache_n;
static const uint8_t *cache_r;

void edits_setup(const char *world, uint32_t *buf, uint32_t n) {
  int i = 0;
  for (; world[i] && i < 7; i++) prefix[i] = world[i];
  prefix[i] = 0;
  (void)buf, (void)n;   /* (the chunk slab is the game's now: see merge, above) */
}
bool edits_full(void) { return full; }

/* the dimension the player's changes go to: the world's (DIM_OVER, DIM_NETHER).
 * The journal is flushed first: its entries are this dimension's (they are written
 * to its records), so they never travel to the other one's */
void edits_set_dim(int dim) {
  if (nj) edits_flush();
  edit_dim = dim == DIM_OVER ? 'r' : dim == DIM_NETHER ? 'n' : 'e';
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

/* entries are sorted by their low 20 bits (the position: chunk, x and z in it, y);
 * the block state is the 9 bits above them, so a 32-bit entry holds both */
#define KEY(e) ((e) & 0xFFFFF)
#define BLK(e) ((int)((e) >> 20) & 511)
#define ENTRY(key, b) ((key) | (uint32_t)(b) << 20)
/* The overworld's deepslate floor (nb.h's WORLD_MIN_Y): its records store the y based on
 * it, so the byte reads 0..191 (y -64..127) and stays ordered. The nether has no deep
 * layer below 0, so it stores the y as it is. */
static inline int y_base(void) { return edit_dim == 'r' ? WORLD_MIN_Y : 0; }

static inline uint32_t entry_key(int x, int y, int z) {
  int cx = floordiv(x, 16), cz = floordiv(z, 16);
  int chunk = (cx & 3) | (cz & 3) << 2;
  int xz = (x & 15) | (z & 15) << 4;
  return (uint32_t)(chunk << 16 | xz << 8 | ((y - y_base()) & 255));
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
    /* Merge the record and the journal's entries for it, in key order. The journal's
     * always go in: they are the player's, and they are at most J_MAX. A record that
     * has outgrown the room makes way for them by losing its tail - the entries with
     * the largest keys - which is the one thing here that can be cut, and cutting it
     * is what keeps the merge possible. The alternative is worse: a record that cannot
     * be merged at all leaves the journal holding the player's changes with nowhere to
     * put them, so every change after that one is thrown away, and the structures come
     * back whole. (Nothing makes a record that big any more - the showcase's liquids
     * stand where they are put - so this is the old damage healing itself.) */
    int room = MERGE_MAX - (j - i);   /* what is left of it for the record's own entries */
    uint32_t m = 0;
    int a = 0, b = i;
    while (a < n || b < j) {
      if (b >= j || (a < n && KEY(R(a)) < KEY(jn[b].e))) {
        if (a >= room) {   /* the record's tail makes way for the journal's */
          a++;
          continue;
        }
        merge[m++] = R(a), a++;
      } else {
        if (a < n && KEY(R(a)) == KEY(jn[b].e)) a++;
        merge[m++] = jn[b++].e;
      }
    }
    char name[32];
    region_name(name, rx, rz);
    cache_ok = false;
    if (b < j || !plat_save(name, merge, m * 4)) {
      /* the write failed (the storage is full): these changes stay in the journal */
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
    if (nj >= J_MAX) return false;   /* the storage is full: this change is not kept */
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
  uint32_t k0 = (uint32_t)chunk << 16, k1 = k0 + (1 << 16);
  int n;
  const uint8_t *r = region(rx, rz, &n);
  for (int i = rlower(r, n, k0); i < n && KEY(R(i)) < k1; i++) {
    uint32_t e = R(i);
    int y = (int)(e & 255) + y_base() - y0, xz = (int)(e >> 8) & 255;
    if (y >= 0 && y < h) b9_set(slab, (y * 16 + (xz >> 4)) * 16 + (xz & 15), BLK(e));
  }
  for (int i = jlower(rx, rz, k0); i < nj && jn[i].rx == rx && jn[i].rz == rz && KEY(jn[i].e) < k1; i++) {
    int y = (int)(jn[i].e & 255) + y_base() - y0, xz = (int)(jn[i].e >> 8) & 255;
    if (y >= 0 && y < h) b9_set(slab, (y * 16 + (xz >> 4)) * 16 + (xz & 15), BLK(jn[i].e));
  }
}

/* The change recorded at (x, y, z), or -1: the journal's (the newest) then the record's,
 * as edits_apply reads them. */
static int edit_block(int x, int y, int z) {
  int rx = floordiv(x, 64), rz = floordiv(z, 64);
  uint32_t k = entry_key(x, y, z);
  int n;
  const uint8_t *r = region(rx, rz, &n);
  int j = jlower(rx, rz, k);
  if (j < nj && jn[j].rx == rx && jn[j].rz == rz && KEY(jn[j].e) == k) return BLK(jn[j].e);
  int i = rlower(r, n, k);
  if (i < n && KEY(R(i)) == k) return BLK(R(i));
  return -1;
}

/* The column's sky top (1 + the highest opaque y) with the changes. A block placed above
 * raises it; a block dug out of the top lowers it, down through the changes, until a cell
 * with no change - the ground itself, opaque below the generator's surface - or an opaque
 * one. (Without the lowering a dug shaft whose surface has left the cache read as if the
 * ground were still overhead, and the whole column went dark.) */
int edits_top(int x, int z, int top) {
  int rx = floordiv(x, 64), rz = floordiv(z, 64);
  uint32_t k0 = entry_key(x, y_base(), z), k1 = k0 + 256;   /* the whole column, all y */
  int n;
  const uint8_t *r = region(rx, rz, &n);
  for (int i = rlower(r, n, k0); i < n && KEY(R(i)) < k1; i++) {
    uint32_t e = R(i);
    int ey = (int)(e & 255) + y_base();
    if ((blk_flags[BLK(e)] & BF_OPAQUE) && ey + 1 > top) top = ey + 1;
  }
  for (int i = jlower(rx, rz, k0); i < nj && jn[i].rx == rx && jn[i].rz == rz && KEY(jn[i].e) < k1; i++) {
    int ey = (int)(jn[i].e & 255) + y_base();
    if ((blk_flags[BLK(jn[i].e)] & BF_OPAQUE) && ey + 1 > top) top = ey + 1;
  }
  while (top > WORLD_MIN_Y) {
    int b = edit_block(x, top - 1, z);
    if (b < 0 || (blk_flags[b] & BF_OPAQUE)) break;
    top--;
  }
  return top;
}

int edits_count(void) { return nj; }

/* the merge buffer, lent out between merges (n bytes at most): the world's own save
 * is built in it too (save.c), which is nothing to do with a merge in progress */
void *edits_scratch(uint32_t n) { return n <= sizeof merge ? (void *)merge : NULL; }

/* ---------------------------------------------------------------- old records
 * Records written before the states were 9 bits hold 8-bit blocks over a 24-bit
 * position (a head says NBW1..4: from 0); the records written while the position
 * packed y in 7 bits hold 9-bit blocks over 19 (a head says NBW5: from 1); NBW6
 * holds the position 20 bits (y 8 of them) with the block at bits 20... All three
 * are read again and written back in the current shape (NBW7): NBW6's, with the
 * overworld's y based on the deepslate floor (nb.h's WORLD_MIN_Y) so the byte reads
 * 0..191 and -64..127 fits 8 bits. Once per world. Every entry is still 4 bytes, so
 * each record is rewritten in place and the records keep their places. */
bool edits_convert(const char *world, int from) {
  bool ok = true;
  int pl = (int)strlen(world);
  char rec[40];
  for (int i = 0; plat_record(i, rec, sizeof rec); i++) {
    /* the world's own: the world's prefix, then "r" (the overworld) or "n" (the
     * nether), then the region */
    if (strncmp(rec, world, (size_t)pl) || (rec[pl] != 'r' && rec[pl] != 'n')) continue;
    int k = pl + 1;
    bool shape = rec[k] == '-' || (rec[k] >= '0' && rec[k] <= '9');
    while (rec[k] && (rec[k] == '-' || (rec[k] >= '0' && rec[k] <= '9') || rec[k] == '_')) k++;
    if (!shape || strcmp(rec + k, ".nbe")) continue;
    int base = rec[pl] == 'r' ? WORLD_MIN_Y : 0;   /* the overworld's y moved onto its floor */
    uint32_t len = 0;
    const uint8_t *r = plat_load(rec, &len);
    if (!r || !len || len % 4 || len / 4 > MERGE_MAX) continue;
    int n = (int)(len / 4), good = 1;
    for (int j = 0; j < n && good; j++) {
      uint32_t e = rd32(r + 4 * j), key, blk;
      if (from == 0) {
        key = e & 0xFFFFFF, blk = e >> 24;   /* the old shape: 24-bit position, block on top */
        if (key >> 19 || blk >= NSTATES) good = 0;   /* not an 8-bit entry: leave this record alone */
        else merge[j] = ENTRY(((key >> 7) << 8) | (key & 0x7F), (int)blk);
      } else if (from == 1) {
        key = e & 0x7FFFF, blk = (e >> 19) & 511;
        if (e >> 28) good = 0;   /* not a 19-bit entry: already converted, leave it */
        else merge[j] = ENTRY(((key >> 7) << 8) | (key & 0x7F), (int)blk);
      } else {
        blk = (e >> 20) & 511;
        if (e >> 29 || blk >= NSTATES) good = 0;   /* not a 20-bit entry: leave it */
        else merge[j] = e;
      }
      if (good) {   /* the stored y: y - base (the overworld moves down 64) */
        int by = (int)(merge[j] & 255) - base;
        merge[j] = (merge[j] & ~0xFFu) | (uint32_t)(by & 255);
      }
    }
    if (!good || !plat_save(rec, merge, len)) ok = false;
  }
  edits_forget();
  return ok;
}
