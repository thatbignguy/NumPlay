/* NumPowder: The Powder Toy's Game of Life (Simulation::SimulateGoL, the built-in types of SimulationData.cpp,
 * GOLString.cpp).
 *
 * The original counts neighbours into gol[YRES][XRES][5], 32-bit lists of the kinds around each cell (128 KB
 * here). This builds each cell's list when it's looked at, from its 8 neighbours, in the order the original's
 * would have (by particle id), so the outcome is the same; the living cells that count are marked in a bitmap
 * first. (A LIFE particle under another particle isn't seen there: the original would count it.) */
#include "simpriv.h"

/* rulesets: bits 0..8 stay with that many neighbours, 9..16 begin with that many minus 8, 17..20 the number of
 * states minus 2 */
const BuiltinGOL builtinGol[NGOL] = {
  {"GOL", GT_GOL, 0x0080C, 0x0CAC00, 0x0CAC00, "Game Of Life: Begin 3/Stay 23"},
  {"HLIF", GT_HLIF, 0x0480C, 0xFF0000, 0xFF0000, "High Life: B36/S23"},
  {"ASIM", GT_ASIM, 0x038F0, 0x0000FF, 0x0000FF, "Assimilation: B345/S4567"},
  {"2X2", GT_2x2, 0x04826, 0xFFFF00, 0xFFFF00, "2X2: B36/S125"},
  {"DANI", GT_DANI, 0x1C9D8, 0x00FFFF, 0x00FFFF, "Day and Night: B3678/S34678"},
  {"AMOE", GT_AMOE, 0x0A92A, 0xFF00FF, 0xFF00FF, "Amoeba: B357/S1358"},
  {"MOVE", GT_MOVE, 0x14834, 0xFFFFFF, 0xFFFFFF, "'Move' particles. Does not move things.. it is a life type: B368/S245"},
  {"PGOL", GT_PGOL, 0x0A90C, 0xE05010, 0xE05010, "Pseudo Life: B357/S238"},
  {"DMOE", GT_DMOE, 0x1E9E0, 0x500000, 0x500000, "Diamoeba: B35678/S5678"},
  {"3-4", GT_34, 0x01818, 0x500050, 0x500050, "3-4: B34/S34"},
  {"LLIF", GT_LLIF, 0x03820, 0x505050, 0x505050, "Long Life: B345/S5"},
  {"STAN", GT_STAN, 0x1C9EC, 0x5000FF, 0x5000FF, "Stains: B3678/S235678"},
  {"SEED", GT_SEED, 0x00400, 0xFBEC7D, 0xFBEC7D, "Seeds: B2/S"},
  {"MAZE", GT_MAZE, 0x0083E, 0xA8E4A0, 0xA8E4A0, "Maze: B3/S12345"},
  {"COAG", GT_COAG, 0x189EC, 0x9ACD32, 0x9ACD32, "Coagulations: B378/S235678"},
  {"WALL", GT_WALL, 0x1F03C, 0x0047AB, 0x0047AB, "Walled cities: B45678/S2345"},
  {"GNAR", GT_GNAR, 0x00202, 0xE5B73B, 0xE5B73B, "Gnarl: B1/S1"},
  {"REPL", GT_REPL, 0x0AAAA, 0x259588, 0x259588, "Replicator: B1357/S1357"},
  {"MYST", GT_MYST, 0x139E1, 0x0C3C00, 0x0C3C00, "Mystery: B3458/S05678"},
  {"LOTE", GT_LOTE, 0x48938, 0xFF0000, 0xFFFF00, "Living on the Edge: B37/S3458/4"},
  {"FRG2", GT_FRG2, 0x20816, 0x006432, 0x00FF5A, "Like Frogs rule: B3/S124/3"},
  {"STAR", GT_STAR, 0x98478, 0x000040, 0x0000E6, "Like Star Wars rule: B278/S3456/6"},
  {"FROG", GT_FROG, 0x21806, 0x006400, 0x00FF00, "Frogs: B34/S12/3"},
  {"BRAN", GT_BRAN, 0x25440, 0xFFFF00, 0x969600, "Brian 6: B246/S6/3"},
};

/* the GoL space is BORDER narrower on each side than the simulation, and wraps around */
static inline int wrapx(int x) { return ((x + XRES - 3 * BORDER) % (XRES - 2 * BORDER)) + BORDER; }
static inline int wrapy(int y) { return ((y + YRES - 3 * BORDER) % (YRES - 2 * BORDER)) + BORDER; }

static inline bool counts(int x, int y) {
  int k = y * XRES + x;
  return sim_scratch[k >> 3] >> (k & 7) & 1;
}

void SimulateGoL(void) {
  CGOL = 0;
  /* the living cells (at full state) count for their neighbours; the others die a step */
  memset(sim_scratch, 0, (XRES * YRES + 7) / 8);
  for (int i = 0; i < parts_active; ++i) {
    Particle *part = &parts[i];
    if (part->type != PT_LIFE) continue;
    int x = (part->x + 32) / 64, y = (part->y + 32) / 64;
    if (x < BORDER || y < BORDER || x >= XRES - BORDER || y >= YRES - BORDER) continue;
    unsigned int ruleset = (unsigned int)part->ctype;
    if (ruleset < NGOL) ruleset = (unsigned int)builtinGol[ruleset].ruleset;
    if (part->tmp2 == (int)((ruleset >> 17) & 0xF) + 1) {
      if (pmap_id[y][x] == i + 1) {
        int k = y * XRES + x;
        sim_scratch[k >> 3] |= (uint8_t)(1 << (k & 7));
      }
    } else if (!(bmap[y / WCELL][x / WCELL] == WL_STASIS && emap[y / WCELL][x / WCELL] < 8)) {
      part->tmp2 -= 1;
    }
  }
  for (int y = BORDER; y < YRES - BORDER; ++y)
    for (int x = BORDER; x < XRES - BORDER; ++x) {
      int r = PMAPAT(x, y);
      if (r && TYP(r) != PT_LIFE) continue;
      /* the neighbours that count, in the order they'd have added to the list: by particle id */
      struct { int id, golnum, xx, yy; } nb[8];
      int n = 0;
      for (int yy = -1; yy <= 1; ++yy)
        for (int xx = -1; xx <= 1; ++xx) {
          if (!xx && !yy) continue;
          int sx = wrapx(x - xx), sy = wrapy(y - yy);   /* (a cell at sx, sy adds to sx + xx, sy + yy) */
          if (!counts(sx, sy)) continue;
          int id = pmap_id[sy][sx] - 1;
          unsigned golnum = (unsigned)parts[id].ctype;
          if (golnum < NGOL) golnum += 1;
          int k = n++;
          while (k > 0 && nb[k - 1].id > id) nb[k] = nb[k - 1], k--;
          nb[k].id = id, nb[k].golnum = (int)golnum, nb[k].xx = xx, nb[k].yy = yy;
        }
      if (!r && !n) continue;
      /* the original's list: up to 5 kinds, each with its count and the place of its first */
      unsigned int list[5] = {0, 0, 0, 0, 0};
      int kinds = 0;
      for (int k = 0; k < n; k++) {
        int l;
        for (l = 0; l < kinds; l++)
          if ((int)(list[l] & 0x001FFFFFU) == nb[k].golnum) {
            list[l] += 1U << 21;
            break;
          }
        if (l == kinds && kinds < 5) list[kinds++] = ((nb[k].yy & 3) << 26) | ((nb[k].xx & 3) << 24) | (unsigned)nb[k].golnum;
      }
      unsigned int neighbours = (unsigned int)n;
      if (bmap[y / WCELL][x / WCELL] == WL_STASIS && emap[y / WCELL][x / WCELL] < 8) continue;
      if (r) {
        Particle *part = &parts[ID(r)];
        unsigned int ruleset = (unsigned int)part->ctype;
        if (ruleset < NGOL) ruleset = (unsigned int)builtinGol[ruleset].ruleset;
        if (!((ruleset >> neighbours) & 1) && part->tmp2 == (int)(ruleset >> 17) + 1) part->tmp2 -= 1;   /* start dying */
      } else {
        unsigned int golnumToCreate = 0xFFFFFFFFU, createFromEntry = 0U;
        unsigned int majority = neighbours / 2 + neighbours % 2;
        for (int l = 0; l < kinds; ++l) {
          unsigned int golnum = list[l] & 0x001FFFFFU, ruleset = golnum;
          if (golnum - 1 < NGOL) {
            ruleset = (unsigned int)builtinGol[golnum - 1].ruleset;
            golnum -= 1;
          }
          if ((ruleset >> (neighbours + 8)) & 1 && ((list[l] >> 21) & 7) + 1 >= majority && golnum < golnumToCreate) {
            golnumToCreate = golnum;
            createFromEntry = list[l];
          }
        }
        if (golnumToCreate != 0xFFFFFFFFU) {
          /* 0x200000: no need to look for colours, they're set from the parent */
          int i = create_part(-1, x, y, PT_LIFE, (int)(golnumToCreate | 0x200000));
          if (i >= 0) {
            int xx = (createFromEntry >> 24) & 3, yy = (createFromEntry >> 26) & 3;
            if (xx == 3) xx = -1;
            if (yy == 3) yy = -1;
            int sample = PMAPAT(wrapx(x - xx), wrapy(y - yy));
            parts[i].tmp = parts[ID(sample)].tmp;
          }
        }
      }
    }
  for (int y = BORDER; y < YRES - BORDER; ++y)
    for (int x = BORDER; x < XRES - BORDER; ++x) {
      int r = PMAPAT(x, y);
      if (r && TYP(r) == PT_LIFE && parts[ID(r)].tmp2 <= 0) kill_part(ID(r));
    }
}

/* ---------------------------------------------------------------- rules as text (GOLString.cpp) */
int ParseGOLString(const char *it) {
  unsigned begin = 0, stay = 0, states = 2;
  if (*it == 'B') it++;   /* must begin with a B */
  else return -1;
  for (; *it >= '1' && *it <= '8'; ++it) begin |= 1U << (*it - '0');   /* 1 to 8 */
  if (!begin) return -1;
  if (it[0] == '/' && it[1] == 'S') it += 2;   /* then /S */
  else return -1;
  for (; *it >= '0' && *it <= '8'; ++it) stay |= 1U << (*it - '0');   /* 0 to 8 */
  if (*it) {   /* optionally the number of states */
    if (*it == '/') it++;
    else return -1;
    states = 0;
    if (!*it) return -1;
    for (; *it; it++) {
      if (*it < '0' || *it > '9' || states > 1000) return -1;
      states = states * 10 + (unsigned)(*it - '0');
    }
    if (states < 2 || states > 17) return -1;
  }
  return (int)(stay | (begin << 8) | ((states - 2) << 17));
}

void SerialiseGOLRule(int rule, char *out) {
  *out++ = 'B';
  for (int i = 1; i < 9; ++i)
    if ((rule >> (i + 8)) & 1) *out++ = (char)('0' + i);
  *out++ = '/';
  *out++ = 'S';
  for (int i = 0; i < 9; ++i)
    if ((rule >> i) & 1) *out++ = (char)('0' + i);
  int st = (rule >> 17) & 0xF;
  if (st) {
    *out++ = '/';
    st += 2;
    if (st >= 10) *out++ = (char)('0' + st / 10);
    *out++ = (char)('0' + st % 10);
  }
  *out = 0;
}
