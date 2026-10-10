/* Finance: tiny compound-interest / loan / savings calculators, drawn like
 * the calculator's own apps (see launcher/src/matrices.c). */
#include <eadk.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../../common/epsilon_app.h"

#ifdef __ELF__
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Interest";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
#endif

#define C_BAR 0xFCE0
#define C_TAB 0xAD55
#define C_BG 0xEF5D
#define C_WHITE 0xFFFF
#define C_SEL 0xCE79
#define C_LINE 0xC618
#define C_TEXT 0x0000
#define C_GRAY 0x7BEF
#define C_BLUE 0x001F

#define SW 320
#define SH 240

enum {
  KEY_LEFT = 0, KEY_UP = 1, KEY_DOWN = 2, KEY_RIGHT = 3, KEY_OK = 4, KEY_BACK = 5,
  KEY_HOME = 6, KEY_ONOFF = 8, KEY_VAR = 15, KEY_TOOLBOX = 16, KEY_BACKSPACE = 17,
  KEY_MINUS = 46, KEY_DOT = 49, KEY_EXE = 52
};
/* hidden world: NumBlocks' main, renamed at compile time (-Dmain=numblocks_main) */
int numblocks_main(void);
static const int8_t digit_keys[10] = {48, 42, 43, 44, 36, 37, 38, 30, 31, 32};

static void fill(int x, int y, int w, int h, eadk_color_t c) {
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > SW) w = SW - x;
  if (y + h > SH) h = SH - y;
  if (w > 0 && h > 0) eadk_display_push_rect_uniform((eadk_rect_t){x, y, w, h}, c);
}
static void text(const char *s, int x, int y, bool big, eadk_color_t fg, eadk_color_t bg) {
  eadk_display_draw_string(s, (eadk_point_t){x, y}, big, fg, bg);
}
static int width(const char *s, bool big) {
  int n = 0;
  for (; *s; s++) n += (*s & 0xC0) != 0x80;
  return n * (big ? 10 : 7);
}
static void ctext(const char *s, int cx, int y, bool big, eadk_color_t fg, eadk_color_t bg) {
  text(s, cx - width(s, big) / 2, y, big, fg, bg);
}

/* numbers without printf (newlib nano has no float printf) */
static void fmt(float v, char *o) {
  if (v != v) { strcpy(o, "undef"); return; }
  bool neg = v < 0;
  if (neg) v = -v;
  if (v >= 1e9f) { strcpy(o, neg ? "-big" : "big"); return; }
  uint32_t ip, fp;
  if (v >= 1e6f) ip = (uint32_t)(v + 0.5f), fp = 0;
  else { uint32_t s = (uint32_t)(v * 100 + 0.5f); ip = s / 100, fp = s % 100; }
  if (fp >= 100) { ip += fp / 100; fp %= 100; }
  if (neg && (ip || fp)) *o++ = '-';
  char t[12]; int n = 0;
  do t[n++] = (char)('0' + ip % 10);
  while (ip /= 10);
  while (n) *o++ = t[--n];
  if (fp) { *o++ = '.'; *o++ = (char)('0' + fp / 10); if (fp % 10) *o++ = (char)('0' + fp % 10); }
  *o = 0;
}
static float parse(const char *s) {
  float sign = 1, v = 0, scale = 0;
  if (*s == '-') sign = -1, s++;
  for (; *s; s++) {
    if (*s == '.') scale = scale ? scale : 1;
    else if (*s >= '0' && *s <= '9') {
      if (scale) scale /= 10, v += (float)(*s - '0') * scale;
      else v = v * 10 + (float)(*s - '0');
    }
  }
  return sign * v;
}
static float pow1p(float r, int n) { float f = 1; for (int i = 0; i < n; i++) f *= (1 + r); return f; }

/* fields: decimals shown, edit buffer while typing */
typedef struct { const char *label; float v; } F;
static F *fields; static int nfields, fsel;
static char edit[12];
static bool editing(void) { return edit[0] != 0; }

static void status_bar(const char *title) {
  fill(0, 0, SW, 18, C_BAR);
  ctext(title, 160, 2, false, C_WHITE, C_BAR);
}
#define C_NAVY 0x18E3
#define C_PALE 0xD69A
#define C_HINT 0x39C7
#define C_YEL 0xFFE0

static void hint_bar(void) {
  fill(0, SH - 16, SW, 16, C_HINT);
  text("left/right:tabs up/down:choose OK:edit", 8, SH - 14, false, C_WHITE, C_HINT);
}
static void tabs(int tab) {
  static const char *const names[3] = {"Growth", "Loan", "Save"};
  for (int t = 0; t < 3; t++) {
    eadk_color_t bg = t == tab ? C_WHITE : C_TAB, fg = t == tab ? C_TAB : C_WHITE;
    int x = t == 0 ? 0 : t == 1 ? 107 : 214, w = t == 2 ? 106 : 107;
    fill(x, 18, w, 22, bg);
    ctext(names[t], x + w / 2, 22, false, fg, bg);
  }
}
static void field_rows(void) {
  fill(0, 40, SW, SH - 40, C_BG);
  for (int i = 0; i < nfields; i++) {
    int y = 44 + i * 30;
    bool sel = i == fsel;
    eadk_color_t bg = sel ? C_WHITE : C_WHITE;
    /* card with margin + left accent when selected */
    fill(8, y, SW - 16, 26, sel ? C_SEL : C_LINE);
    fill(8 + (sel ? 3 : 0), y + (sel ? 1 : 1), SW - 16 - (sel ? 4 : 2), 26 - 2, bg);
    if (sel) fill(8, y, 3, 26, C_BLUE);
    text(fields[i].label, 16, y + 7, false, sel ? C_TEXT : C_GRAY, bg);
    char s[14];
    if (i == fsel && editing()) {
      strcpy(s, edit);
      text(s, SW - 16 - width(s, false), y + 7, false, C_BLUE, bg);
      fill(SW - 15, y + 5, 1, 14, C_BLUE);
    } else {
      fmt(fields[i].v, s);
      text(s, SW - 16 - width(s, true), y + 5, true, C_TEXT, bg);
    }
  }
}
/* graph card with area fill, then white result row below it */
static void results(const char *rlabel, const char *rsub, float result, float *series, int n) {
  int gy = 44 + nfields * 30 + 2, gh = 58, gx = 14, gw = SW - 28;
  int ry = gy + gh + 3;
  /* result row first so the value always shows even if n < 2 */
  char s[16];
  fmt(result, s);
  fill(8, ry, SW - 16, 27, C_LINE);
  fill(9, ry + 1, SW - 18, 25, C_WHITE);
  text(rlabel, 16, ry + 2, false, C_GRAY, C_WHITE);
  text(rsub, 16, ry + 14, false, C_GRAY, C_WHITE);
  text(s, SW - 16 - width(s, true), ry + 5, true, C_TEXT, C_WHITE);
  if (n < 2) { fill(8, gy, SW - 16, gh, C_WHITE); return; }
  float mx = series[0], mn = series[0];
  for (int i = 1; i < n; i++) { if (series[i] > mx) mx = series[i]; if (series[i] < mn) mn = series[i]; }
  if (mx - mn < 1) mx = mn + 1;
  fill(8, gy, SW - 16, gh, C_WHITE);
  /* one y per pixel column, clamped on screen */
  for (int c = 0; c <= gw; c++) {
    int i = c * (n - 1) / gw;
    int i2 = i + 1 < n ? i + 1 : i;
    int f = c * (n - 1) % gw;
    float v = series[i] + (series[i2] - series[i]) * (float)f / (float)gw;
    int y = gy + gh - 4 - (int)((v - mn) / (mx - mn) * (gh - 8));
    if (y < gy + 1) y = gy + 1;
    if (y > gy + gh - 3) y = gy + gh - 3;
    fill(gx + c, y, 1, gy + gh - 3 - y, C_PALE); /* area */
  }
  for (int g = 0; g <= 2; g++) { int yy = gy + 2 + (gh - 6) * g / 2; fill(gx, yy, gw, 1, C_LINE); }
  for (int c = 0; c <= gw; c++) {
    int i = c * (n - 1) / gw;
    int i2 = i + 1 < n ? i + 1 : i;
    int f = c * (n - 1) % gw;
    float v = series[i] + (series[i2] - series[i]) * (float)f / (float)gw;
    int y = gy + gh - 4 - (int)((v - mn) / (mx - mn) * (gh - 8));
    if (y < gy) y = gy;
    if (y > gy + gh - 2) y = gy + gh - 2;
    fill(gx + c, y, 1, 2, C_BLUE); /* curve */
  }
}

/* per-tab data */
static F FG[3] = {{"Principal", 1000}, {"Rate %", 5}, {"Years", 10}};
static F FL[3] = {{"Amount", 10000}, {"APR %", 6}, {"Years", 5}};
static F FS[3] = {{"Goal", 5000}, {"Per month", 200}, {"Rate %", 3}};
static float series[33];

static float calc_growth(int *n, float *paid) {
  float b = FG[0].v, r = FG[1].v / 100;
  int yrs = (int)FG[2].v; if (yrs < 1) yrs = 1; if (yrs > 32) yrs = 32;
  *n = yrs + 1;
  for (int i = 0; i < *n; i++) { series[i] = b; b = b * (1 + r); }
  *paid = FG[0].v;
  return series[yrs];
}
static float calc_loan(int *n, float *paid) {
  int yrs = (int)FL[2].v; if (yrs < 1) yrs = 1; if (yrs > 30) yrs = 30;
  int mo = yrs * 12, steps = mo > 32 ? 32 : mo;
  float r = FL[1].v / 100 / 12, pmt;
  if (r <= 0) pmt = FL[0].v / mo;
  else { float f = pow1p(r, mo); pmt = FL[0].v * r * f / (f - 1); }
  float bal = FL[0].v;
  *n = steps + 1;
  for (int i = 0; i <= steps; i++) { series[i] = bal < 0 ? 0 : bal; bal = bal * (1 + r) - pmt; }
  *paid = pmt * mo;
  return pmt;
}
static float calc_save(int *n, float *paid) {
  float r = FS[2].v / 100 / 12, b = 0; int m = 0;
  while (b < FS[0].v && m < 360) { b = b * (1 + r) + FS[1].v; m++; }
  int steps = m > 32 ? 32 : m;
  float bb = 0;
  for (int i = 0; i <= steps; i++) { series[i] = bb; bb = bb * (1 + r) + FS[1].v; }
  *n = steps + 1; *paid = FS[1].v * m;
  return (float)m;
}

static void redraw(int tab) {
  status_bar("INTEREST");
  tabs(tab);
  field_rows();
  int n; float tot, res;
  char sub[20], num[14];
  if (tab == 0) {
    res = calc_growth(&n, &tot);
    fmt(tot, num); sub[0] = 'i'; sub[1] = 'n'; sub[2] = ' ';
    int i; for (i = 0; num[i] && i < 15; i++) sub[3 + i] = num[i]; sub[3 + i] = 0;
    results("FINAL VALUE", sub, res, series, n);
  } else if (tab == 1) {
    res = calc_loan(&n, &tot);
    fmt(tot, num); strcpy(sub, "tot ");
    int i; for (i = 0; num[i] && i < 15; i++) sub[4 + i] = num[i]; sub[4 + i] = 0;
    results("PER MONTH", sub, res, series, n);
  } else {
    res = calc_save(&n, &tot);
    fmt(tot, num); strcpy(sub, "paid ");
    int i; for (i = 0; num[i] && i < 14; i++) sub[5 + i] = num[i]; sub[5 + i] = 0;
    results("MONTHS TO GOAL", sub, res, series, n);
  }
  hint_bar();
}

/* full state reset: NumBlocks' np_app_end() zeroes our RAM when it quits,
 * so everything (including default values in .data) must be restored */
static int tab;
static void init_state(void) {
  tab = 0;
  fields = FG; nfields = 3; fsel = 0; edit[0] = 0;
  FG[0] = (F){"Principal", 1000}; FG[1] = (F){"Rate %", 5}; FG[2] = (F){"Years", 10};
  FL[0] = (F){"Amount", 10000}; FL[1] = (F){"APR %", 6}; FL[2] = (F){"Years", 5};
  FS[0] = (F){"Goal", 5000}; FS[1] = (F){"Per month", 200}; FS[2] = (F){"Rate %", 3};
}

int main(void) {
  np_app_begin();
  init_state();
  redraw(tab);
  uint64_t prev = eadk_keyboard_scan();
  for (;;) {
    uint64_t k = eadk_keyboard_scan(), hit = k & ~prev;
    prev = k;
#define HIT(key) (((hit) >> (key)) & 1)
    if (HIT(KEY_HOME) || HIT(KEY_ONOFF)) break;
    if (HIT(KEY_VAR)) {
      numblocks_main();   /* the hidden world; returns on quit */
      np_app_begin();     /* re-lock Home, wiped by NumBlocks' cleanup */
      init_state();
      redraw(tab);
      prev = eadk_keyboard_scan();
      continue;
    }
    if (!hit) { eadk_timing_msleep(16); continue; }
    bool dirty = false;
    if (HIT(KEY_LEFT) && !editing()) { tab = (tab + 2) % 3; fsel = 0; edit[0] = 0; dirty = true; }
    else if (HIT(KEY_RIGHT) && !editing()) { tab = (tab + 1) % 3; fsel = 0; edit[0] = 0; dirty = true; }
    else if (HIT(KEY_UP)) { if (editing()) { edit[0] = 0; } else fsel = (fsel + nfields - 1) % nfields; dirty = true; }
    else if (HIT(KEY_DOWN)) { if (editing()) { fields[fsel].v = parse(edit); edit[0] = 0; } else fsel = (fsel + 1) % nfields; dirty = true; }
    else if (HIT(KEY_OK) || HIT(KEY_EXE)) { if (editing()) { fields[fsel].v = parse(edit); edit[0] = 0; } dirty = true; }
    else if (HIT(KEY_BACK)) { if (editing()) edit[0] = 0; dirty = true; }
    else {
      size_t len = strlen(edit);
      for (int d = 0; d < 10; d++)
        if (HIT(digit_keys[d]) && len < 9) { edit[len++] = (char)('0' + d); edit[len] = 0; dirty = true; }
      if (HIT(KEY_DOT) && len < 9 && !strchr(edit, '.')) { edit[len++] = '.'; edit[len] = 0; dirty = true; }
      if (HIT(KEY_MINUS) && len == 0) { edit[len++] = '-'; edit[len] = 0; dirty = true; }
      if (HIT(KEY_BACKSPACE)) {
        if (len) edit[len - 1] = 0;
        else fields[fsel].v = 0;
        dirty = true;
      }
    }
    if (tab == 0) fields = FG; else if (tab == 1) fields = FL; else fields = FS;
    if (dirty) redraw(tab);
  }
  return np_app_end();
}
