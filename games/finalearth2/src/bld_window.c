/* The Final Earth 2: building windows' texts: the original's lang_en.csv
 * lines with their "[0]" and "[1]" filled in (common.Localize.lo), numbers
 * written as JavaScript writes them. */
#include "bldi.h"

/* a number as JavaScript's String(n) for the values windows show (integers, or a few decimals) */
static int put_num(char *out, int size, double v) {
  char tmp[32];
  int n = 0;
  if (v != v) v = 0;
  bool neg = v < 0;
  if (neg) v = -v;
  double ip = floor(v);
  double frac = v - ip;
  /* up to 6 decimals, trailing zeros dropped */
  long long scaled = (long long)(frac * 1e6 + 0.5);
  if (scaled >= 1000000) ip += 1, scaled = 0;
  unsigned long long whole = ip > 9e17 ? 900000000000000000ull : (unsigned long long)ip;
  char digits[24];
  int nd = 0;
  do {
    digits[nd++] = (char)('0' + whole % 10);
    whole /= 10;
  } while (whole);
  if (neg && (ip > 0 || scaled > 0)) tmp[n++] = '-';
  while (nd) tmp[n++] = digits[--nd];
  if (scaled > 0) {
    char fd[6];
    for (int i = 5; i >= 0; i--) fd[i] = (char)('0' + scaled % 10), scaled /= 10;
    int last = 5;
    while (last >= 0 && fd[last] == '0') last--;
    tmp[n++] = '.';
    for (int i = 0; i <= last; i++) tmp[n++] = fd[i];
  }
  int k = 0;
  for (; k < n && k < size - 1; k++) out[k] = tmp[k];
  out[k] = 0;
  return k;
}

static const char *lo(char *buf, int size, const char *text, int nv, const double *v) {
  int o = 0;
  for (const char *s = text; *s && o < size - 1;) {
    if (s[0] == '[' && s[1] >= '0' && s[1] <= '9' && s[2] == ']' && s[1] - '0' < nv) {
      o += put_num(buf + o, size - o, v[s[1] - '0']);
      s += 3;
    } else {
      buf[o++] = *s++;
    }
  }
  buf[o] = 0;
  return buf;
}
const char *lo1(char *buf, int size, const char *text, double v0) { return lo(buf, size, text, 1, &v0); }
const char *lo2(char *buf, int size, const char *text, double v0, double v1) {
  double v[2] = {v0, v1};
  return lo(buf, size, text, 2, v);
}
void win_line(WinOut *w, const char *text) { win_info(w, 0, text); }
void win_line1(WinOut *w, const char *text, double v0) {
  char b[160];
  win_info(w, 0, lo1(b, sizeof b, text, v0));
}
void win_line2(WinOut *w, const char *text, double v0, double v1) {
  char b[160];
  win_info(w, 0, lo2(b, sizeof b, text, v0, v1));
}
