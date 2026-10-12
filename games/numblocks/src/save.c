/* Saving: each world's record (its name, seed, the time, the weather, the player, chests and
 * furnaces) next to the records of its changed blocks (edits.c), and the options. A world
 * saves every 45 seconds of play, as Minecraft does, and when you quit; block changes reach
 * storage as they fill the journal. Worlds live in slots: the records of slot 3 are
 * "nb3.nbw" and "nb3r..." (its regions). */
#include <stddef.h>
#include "nb.h"
#pragma GCC optimize("Os")   /* (not where the time goes: small) */
#include "edits.h"

#define OPTIONS_REC "numblocks.cfg"

/* the options' defaults, positional (the record is the struct as it comes):
 * difficulty, fancy, look speed, clouds, bobbing, key sheet version, last world,
 * plays, the keys, debug overlay, render distance, sprint toggle, brightness
 * (8, just over the middle of Moody..Bright: a dark room can be made out at once) */
Options opt = {2, 0, 100, 1, 1, 0, 0, 0, {0}, 0, 2, 0, 8};
int64_t world_seed;
uint8_t world_type;
uint8_t world_dim;   /* the dimension (DIM_*): the overworld, or the nether */
char world_preset[FLAT_MAX + 1] = FLAT_DEFAULT;

/* the record's head: version 1 had no name (it was "New World") */
typedef struct {
  char magic[4];
  uint16_t player_size, tiles_size;
  int64_t seed;
  uint32_t time;
  Weather weather;
} Head1;
typedef struct {
  Head1 h;
  char name[WORLD_NAME + 1];
  uint32_t played;
} Head2;   /* version 2 had no cheats or game rules */
typedef struct {
  Head2 h2;
  uint8_t cheats, rules, type, pad;   /* (type: 0 in worlds from before there were others) */
} Head;
typedef struct {
  Head h3;
  char flat_preset[FLAT_MAX + 1];   /* version 4: superflat preset (default for older worlds) */
} Head4;
typedef struct {
  Head4 h4;
  uint8_t dim, pad;                  /* the dimension the player is in (DIM_*) */
  int32_t ret_x, ret_y, ret_z;      /* where the nether's portal leads back to (0,0,0: none) */
} Head5;

extern uint32_t game_time;
void *tiles_data(uint32_t *len);

uint8_t game_rules = GR_DEFAULT;
bool world_cheats, lan_open, lan_cheats;
/* where the portal the player came through leads back to (the overworld), and
 * whether the world has one at all: the nether's return position */
int32_t nb_ret_x, nb_ret_y, nb_ret_z;
bool nb_ret_ok;
static int slot = 1;                   /* the world being played */
static char cur_name[WORLD_NAME + 1];  /* its name */
static uint32_t cur_played;

static void prefix_of(int s, char *out) { out[0] = 'n', out[1] = 'b', out[2] = (char)('0' + s), out[3] = 0; }
static void record_of(int s, char *out) {
  prefix_of(s, out);
  out[3] = '.', out[4] = 'n', out[5] = 'b', out[6] = 'w', out[7] = 0;
}
static void copy_name(char *d, const char *s) {
  int i = 0;
  for (; s[i] && i < WORLD_NAME; i++) d[i] = s[i];
  d[i] = 0;
}
static void copy_preset(char *d, const char *s) {
  if (!s || !s[0]) s = FLAT_DEFAULT;
  int i = 0;
  for (; s[i] && i < FLAT_MAX; i++) d[i] = s[i];
  d[i] = 0;
  /* invalid presets fall back to the default (never crash on a bad save) */
  if (!flat_validate(d, NULL, 0)) {
    const char *f = FLAT_DEFAULT;
    int k = 0;
    for (; f[k] && k < FLAT_MAX; k++) d[k] = f[k];
    d[k] = 0;
  }
}

/* a slot's record, read as the latest head (a version 1 record named "New World"); NULL: none */
static const uint8_t *read_head(int s, Head4 *h, uint8_t *dim, int32_t ret[3], uint32_t *len, uint32_t *head_len) {
  char rec[8];
  record_of(s, rec);
  const uint8_t *d = plat_load(rec, len);
  if (!d || *len < sizeof(Head1)) return NULL;
  memset(h, 0, sizeof *h);
  *dim = 0, ret[0] = ret[1] = ret[2] = 0;
  copy_preset(h->flat_preset, FLAT_DEFAULT);
  if (!memcmp(d, "NBW1", 4)) {
    memcpy(&h->h3.h2.h, d, sizeof(Head1));
    copy_name(h->h3.h2.name, "New World");
    *head_len = sizeof(Head1);
  } else if (!memcmp(d, "NBW2", 4) && *len >= sizeof(Head2)) {
    memcpy(&h->h3.h2, d, sizeof(Head2));
    *head_len = sizeof(Head2);
  } else if (!memcmp(d, "NBW3", 4) && *len >= sizeof(Head)) {
    memcpy(&h->h3, d, sizeof(Head));
    *head_len = sizeof(Head);
  } else if (!memcmp(d, "NBW4", 4) && *len >= sizeof(Head4)) {
    memcpy(h, d, sizeof(Head4));
    h->flat_preset[FLAT_MAX] = 0;
    if (!flat_validate(h->flat_preset, NULL, 0)) copy_preset(h->flat_preset, FLAT_DEFAULT);
    *head_len = sizeof(Head4);
  } else if (!memcmp(d, "NBW5", 4) && *len >= sizeof(Head5)) {
    Head5 h5;
    memcpy(&h5, d, sizeof h5);
    *h = h5.h4;
    h->flat_preset[FLAT_MAX] = 0;
    if (!flat_validate(h->flat_preset, NULL, 0)) copy_preset(h->flat_preset, FLAT_DEFAULT);
    *dim = h5.dim;
    memcpy(ret, &h5.ret_x, 3 * sizeof(int32_t));   /* (ret is a parameter: sizeof it is the pointer's) */
    *head_len = sizeof(Head5);
  } else
    return NULL;
  h->h3.h2.name[WORLD_NAME] = 0;
  if (h->h3.h2.h.player_size != sizeof pl || *len < *head_len + sizeof pl) return NULL;
  if (*head_len < sizeof(Head) + sizeof(h->flat_preset)) {
    /* (older worlds: cheats as a new world of their mode would have them, the rules as they start) */
    h->h3.cheats = d[*head_len + offsetof(Player, mode)] != 0;
    h->h3.rules = GR_DEFAULT;
  }
  return d;
}

bool world_info(int s, WorldInfo *w) {
  Head4 h;
  uint8_t dim;
  int32_t ret[3];
  uint32_t len, hl;
  const uint8_t *d = read_head(s, &h, &dim, ret, &len, &hl);
  if (!d) return false;
  Player p;
  memcpy(&p, d + hl, sizeof p);
  copy_name(w->name, h.h3.h2.name);
  w->mode = p.mode;
  w->cheats = h.h3.cheats;
  w->type = h.h3.type;
  w->seed = h.h3.h2.h.seed;
  w->played = h.h3.h2.played;
  copy_preset(w->preset, h.flat_preset);
  return true;
}

int world_free_slot(void) {
  WorldInfo w;
  for (int s = 1; s <= MAX_WORLDS; s++)
    if (!world_info(s, &w)) return s;
  return 0;
}

static bool name_taken(const char *n) {
  WorldInfo w;
  for (int s = 1; s <= MAX_WORLDS; s++)
    if (world_info(s, &w) && !strcmp(w.name, n)) return true;
  return false;
}

void world_unique_name(const char *base, char *out) {
  char b[WORLD_NAME + 1];
  copy_name(b, base[0] ? base : "New World");
  copy_name(out, b);
  for (int k = 2; name_taken(out) && k < 100; k++) {
    /* "New World (2)": the number kept whole, the name cut to fit */
    char sfx[8] = {' ', '(', 0};
    int n = 2;
    if (k >= 10) sfx[n++] = (char)('0' + k / 10);
    sfx[n++] = (char)('0' + k % 10), sfx[n++] = ')', sfx[n] = 0;
    int bl = (int)strlen(b), room = WORLD_NAME - n;
    if (bl > room) bl = room;
    memcpy(out, b, (size_t)bl);
    memcpy(out + bl, sfx, (size_t)n + 1);
  }
}

bool save_world(void) {
  bool ok = edits_flush();
  uint32_t tl;
  void *t = tiles_data(&tl);
  Head5 h;
  memset(&h, 0, sizeof h);
  h.h4.h3.h2.h = (Head1){{'N', 'B', 'W', '5'}, (uint16_t)sizeof pl, (uint16_t)tl, world_seed, game_time, weather};
  copy_name(h.h4.h3.h2.name, cur_name);
  h.h4.h3.h2.played = cur_played;
  h.h4.h3.cheats = world_cheats, h.h4.h3.rules = game_rules, h.h4.h3.type = world_type;
  copy_preset(h.h4.flat_preset, world_preset);
  h.dim = world_dim;
  h.ret_x = nb_ret_x, h.ret_y = nb_ret_y, h.ret_z = nb_ret_z;
  /* one record: the head, the player, the tiles (written from a buffer made in the merge
   * scratch) and, after them, the monster spawners that have been set */
  uint32_t sp = (uint32_t)spawners_save(NULL);
  uint32_t n = sizeof h + sizeof pl + tl + sp;
  uint8_t *b = (uint8_t *)edits_scratch(n);
  if (!b) return false;
  memcpy(b, &h, sizeof h);
  memcpy(b + sizeof h, &pl, sizeof pl);
  memcpy(b + sizeof h + sizeof pl, t, tl);
  spawners_save(b + sizeof h + sizeof pl + tl);
  char rec[8];
  record_of(slot, rec);
  ok = plat_save(rec, b, n) && ok;
  edits_forget();
  copy_write();
  return ok;
}

/* the world becoming the one played: the latest in the list */
static void played(int s) {
  opt.last_world = (uint8_t)s;
  cur_played = ++opt.plays;
  save_options();
}

bool load_world(int s) {
  Head4 h;
  uint8_t dim;
  int32_t ret[3];
  uint32_t len, hl, tl;
  const uint8_t *d = read_head(s, &h, &dim, ret, &len, &hl);
  void *t = tiles_data(&tl);
  /* the spawners are the bytes after the tiles: a count, then 10 for each (0 for a world
   * saved before spawners could be set) */
  uint32_t sp = len - hl - sizeof pl - tl;
  if (!d || h.h3.h2.h.tiles_size != tl || sp % 10 != 1 || sp / 10 > SPAWNERS_MAX) return false;
  slot = s;
  copy_name(cur_name, h.h3.h2.name);
  cur_played = h.h3.h2.played;
  world_seed = h.h3.h2.h.seed;
  game_time = h.h3.h2.h.time;
  world_cheats = h.h3.cheats, lan_open = lan_cheats = false, game_rules = h.h3.rules, world_type = h.h3.type;
  copy_preset(world_preset, h.flat_preset);
  /* World.calculateInitialWeather */
  weather = h.h3.h2.h.weather;
  rain_str = weather.raining ? 1 : 0;
  thunder_str = weather.thundering ? 1 : 0;
  memcpy(&pl, d + hl, sizeof pl);
  memcpy(t, d + hl + sizeof pl, tl);
  spawners_load(d + hl + sizeof pl + tl, (int)(len - hl - sizeof pl - tl));
  memset(ents, 0, sizeof ents);
  char pre[4];
  prefix_of(s, pre);
  world_new(world_seed, pre);
  /* the dimension the player saved in: world_new starts in the overworld, so
   * the nether's generator and records are put back when that is where the
   * save is (the cache is still empty, so this is the whole volume again in
   * the right dimension). Without this the title's backdrop and the spawn
   * after loading are in the overworld at the nether's coordinates. */
  world_dim = dim;
  nb_ret_x = ret[0], nb_ret_y = ret[1], nb_ret_z = ret[2], nb_ret_ok = (ret[0] | ret[1] | ret[2]) != 0;
  if (dim != DIM_OVER) {
    gen_set_dim(dim);
    edits_set_dim(dim);
  }
  if (hl < sizeof(Head5)) {
    /* a world from before the states were 9 bits: its records are re-encoded once
     * (the record is written as NBW5 from now on, which says it has been) */
    edits_convert();
  }
  if (s != opt.last_world || h.h3.h2.played != opt.plays) played(s);
  return true;
}

void new_world(int s, int64_t seed, int mode, bool cheats, int type, const char *name, const char *preset) {
  char pre[4];
  prefix_of(s, pre);
  plat_remove_prefix(pre);
  slot = s;
  copy_name(cur_name, name);
  uint32_t tl;
  void *t = tiles_data(&tl);   /* (first: the length is only known after the call) */
  memset(t, 0, tl);
  memset(ents, 0, sizeof ents);
  world_seed = seed;
  game_time = 0;
  world_cheats = cheats, lan_open = lan_cheats = false, game_rules = GR_DEFAULT, world_type = (uint8_t)type;
  copy_preset(world_preset, type == WT_FLAT ? preset : FLAT_DEFAULT);
  memset(&weather, 0, sizeof weather);
  rain_str = thunder_str = 0;
  spawners_clear();
  world_new(seed, pre);
  nb_ret_x = nb_ret_y = nb_ret_z = 0, nb_ret_ok = false;   /* a new world has no return portal */
  player_spawn();
  pl.mode = (uint8_t)mode;
  played(s);
}

bool rename_world(int s, const char *name) {
  Head4 h;
  uint8_t dim;
  int32_t ret[3];
  uint32_t len, hl;
  const uint8_t *d = read_head(s, &h, &dim, ret, &len, &hl);
  if (!d) return false;
  /* the record again with the new head (made in the merge scratch, as saving does).
   * An older record gains the current head, which is what brings the dimension
   * and the return position with it */
  Head5 h5;
  memset(&h5, 0, sizeof h5);
  h5.h4 = h;
  memcpy(h5.h4.h3.h2.h.magic, "NBW5", 4);
  h5.dim = dim;
  memcpy(&h5.ret_x, ret, sizeof ret);
  copy_name(h5.h4.h3.h2.name, name);
  uint32_t body = len - hl, n = sizeof h5 + body;
  uint8_t *b = (uint8_t *)edits_scratch(n);
  if (!b) return false;
  memcpy(b + sizeof h5, d + hl, body);   /* (first: the scratch may be where nothing else is) */
  memcpy(b, &h5, sizeof h5);
  char rec[8];
  record_of(s, rec);
  bool ok = plat_save(rec, b, n);
  edits_forget();
  if (s == slot) copy_name(cur_name, name);
  copy_write();
  return ok;
}

void delete_world(int s) {
  char pre[4];
  prefix_of(s, pre);
  plat_remove_prefix(pre);
  if (opt.last_world == s) opt.last_world = 0;
  save_options();   /* (and the copy, without that world) */
}

/* ---------------------------------------------------------------- the copy
 * Installing an app from the NumWorks website empties the calculator's files but the Python scripts.
 * So NumBlocks keeps its saves in one too, as comment lines "#>name:base64" (the way NumPlay keeps its
 * games' saves, and in NumPlay it leaves that to NumPlay), and when none of its saves are left, they
 * come back from it. */
#define COPY_REC "numblocks_saves.py"
static const char copy_head[] =
    "# NumBlocks keeps a copy of your worlds here, so that installing\n"
    "# it again doesn't erase them. If you delete this file, NumBlocks\n"
    "# writes it again: your worlds stay either way.\n";
static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static bool ours(const char *n) { return !strcmp(n, OPTIONS_REC) || (n[0] == 'n' && n[1] == 'b' && n[2] >= '1' && n[2] <= '9'); }

void copy_write(void) {
  if (plat_in_launcher()) return;
  char name[40];
  uint32_t total = 1 + sizeof copy_head - 1 + 1, len;
  for (int i = 0; plat_record(i, name, sizeof name); i++)
    if (ours(name) && plat_load(name, &len)) total += 2 + (uint32_t)strlen(name) + 1 + (len + 2) / 3 * 4 + 1;
  uint8_t *o = plat_save_open(COPY_REC, total), *end = o + total;
  if (!o) return;   /* (no room: the copy there stays) */
  *o++ = 0;         /* the script's status byte: not imported by the shell */
  memcpy(o, copy_head, sizeof copy_head - 1);
  o += sizeof copy_head - 1;
  for (int i = 0; plat_record(i, name, sizeof name); i++) {
    const uint8_t *d = ours(name) ? plat_load(name, &len) : NULL;
    uint32_t nl = (uint32_t)strlen(name);
    if (!d || o + 2 + nl + 1 + (len + 2) / 3 * 4 + 1 >= end) continue;
    *o++ = '#', *o++ = '>';
    memcpy(o, name, nl);
    o += nl;
    *o++ = ':';
    for (uint32_t k = 0; k < len; k += 3, o += 4) {
      /* (written out: GCC 15.2 for the calculator got a loop over the four wrong, see launcher/src/storage.c) */
      uint32_t left = len - k, v = (uint32_t)d[k] << 16 | (left > 1 ? d[k + 1] << 8 : 0) | (left > 2 ? d[k + 2] : 0);
      o[0] = (uint8_t)b64[v >> 18 & 63];
      o[1] = (uint8_t)b64[v >> 12 & 63];
      o[2] = left > 1 ? (uint8_t)b64[v >> 6 & 63] : '=';
      o[3] = left > 2 ? (uint8_t)b64[v & 63] : '=';
    }
    *o++ = '\n';
  }
  while (o < end) *o++ = 0;
  plat_save_close(COPY_REC);
}

static int b64_value(uint8_t c) {
  for (int i = 0; i < 64; i++)
    if ((uint8_t)b64[i] == c) return i;
  return -1;
}
void copy_restore(void) {
  char name[40];
  for (int i = 0; plat_record(i, name, sizeof name); i++)
    if (ours(name)) return;   /* (saves are here: they are the newest) */
  uint32_t n, at = 1;
  const uint8_t *c = plat_load(COPY_REC, &n);
  while (c && at < n) {
    /* a line: "#>name:base64" */
    const uint8_t *l = c + at, *e = l;
    while (e < c + n && *e != '\n' && *e) e++;
    uint32_t next = (uint32_t)(e - c) + 1;
    const uint8_t *colon = l;
    while (colon < e && *colon != ':') colon++;
    uint32_t nl = (uint32_t)(colon - l - 2), len = (uint32_t)(e - colon - 1);
    if (e - l > 3 && l[0] == '#' && l[1] == '>' && colon < e && nl < sizeof name && len && !(len % 4)) {
      memcpy(name, l + 2, nl), name[nl] = 0;
      uint32_t size = len / 4 * 3 - (colon[len] == '=') - (colon[len - 1] == '='), off = (uint32_t)(colon + 1 - c);
      uint8_t *out = ours(name) ? plat_save_open(name, size) : NULL;
      c = plat_load(COPY_REC, &n);   /* (the files moved about) */
      if (out && c) {
        const uint8_t *data = c + off;
        for (uint32_t i = 0, k = 0; i < len; i += 4) {
          int v[4];
          for (int q = 0; q < 4; q++) v[q] = data[i + q] == '=' ? 0 : b64_value(data[i + q]);
          uint32_t w = (uint32_t)((v[0] & 63) << 18 | (v[1] & 63) << 12 | (v[2] & 63) << 6 | (v[3] & 63));
          for (int q = 0; q < 3 && k < size; q++) out[k++] = (uint8_t)(w >> (16 - 8 * q));
        }
        plat_save_close(name);
        c = plat_load(COPY_REC, &n);
      }
    }
    at = next;
  }
}

void load_options(void) {
  keys_reset();
  uint32_t len = 0;
  const uint8_t *d = plat_load(OPTIONS_REC, &len);
  /* (an older, shorter record: what it has; the rest stays as it was) */
  if (d && len <= sizeof opt) memcpy(&opt, d, len);
  keys_update(opt.keys_seen);
  /* (older records end in padding where distance now lives: 0 means "never set") */
  if (opt.distance < 1 || opt.distance > 2) opt.distance = 2;
  if (opt.sprint_toggle) opt.sprint_toggle = 1;
  /* (an older record is shorter than the struct: the fields it has no room for stay
   * as they were initialised above; a value past what the field holds is never set) */
  if (opt.brightness > 15) opt.brightness = 0;
}
void save_options(void) {
  plat_save(OPTIONS_REC, &opt, sizeof opt);
  copy_write();
}
