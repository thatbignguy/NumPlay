/* The Final Earth 2: windows that pick a value with the keys: the colour
 * picker (gui.ColorPickerWindow and gui.ColorPicker, at the mobile version's
 * scale) and the texts the original asks for in a text box or with
 * window.prompt (gui.ChangeCityNameWindow, the colour picker's colour code).
 * A text is typed with the calculator's own letters (alpha, then a key; shift
 * and alpha for capitals) or picked from letters on screen with the arrows. */
#include <math.h>
#include "uii.h"

/* ---------------------------------------------------------------- typing (gui.TextInput) */
enum { AL_OFF, AL_ONCE, AL_LOCK, AL_CAPS_ONCE, AL_CAPS_LOCK };   /* the alpha key's state */
static uint8_t alpha_mode;
static bool shift_armed;      /* shift pressed: the next alpha gives capitals */
static bool grid_caps;        /* the on-screen letters are capitals */
static char code_buf[12];     /* the colour code being typed */

/* each typing key's character without alpha (0: none) and with it (NumWorks' alpha layout) */
static const char plain_chars[TK_ANS + 1] = {
  [22] = ',', [30] = '7', [31] = '8', [32] = '9', [33] = '(', [34] = ')', [36] = '4', [37] = '5', [38] = '6', [40] = '/',
  [42] = '1', [43] = '2', [44] = '3', [45] = '+', [46] = '-', [48] = '0', [49] = '.'};
static const char alpha_chars[TK_ANS + 1] = {
  [18] = 'a', [19] = 'b', [20] = 'c', [21] = 'd', [22] = 'e', [23] = 'f', [24] = 'g', [25] = 'h', [26] = 'i', [27] = 'j',
  [28] = 'k', [29] = 'l', [30] = 'm', [31] = 'n', [32] = 'o', [33] = 'p', [34] = 'q', [36] = 'r', [37] = 's', [38] = 't',
  [39] = 'u', [40] = 'v', [42] = 'w', [43] = 'x', [44] = 'y', [45] = 'z', [46] = ' ', [48] = '?', [49] = '!'};

static char *text_of(int which, int *size) {
  if (which == TX_CITY_NAME) {
    *size = (int)sizeof city.cityName;
    return city.cityName;
  }
  *size = (int)sizeof code_buf;
  return code_buf;
}

static void text_add(int which, char c) {
  int size;
  char *t = text_of(which, &size);
  int n = (int)strlen(t);
  if (n < size - 1) t[n] = c, t[n + 1] = 0;
  /* the on-screen letters: capitals to start a word */
  grid_caps = c == ' ';
}

static void text_delete(int which) {
  int size;
  char *t = text_of(which, &size);
  int n = (int)strlen(t);
  if (n) t[n - 1] = 0;
  grid_caps = n <= 1 || t[n - 2] == ' ';
}

void text_window_open(int which) {
  alpha_mode = AL_OFF;
  shift_armed = false;
  if (which == TX_COLOR_CODE) code_buf[0] = 0;
  int size;
  char *t = text_of(which, &size);
  int n = (int)strlen(t);
  grid_caps = !n || t[n - 1] == ' ';
  win_open(WK_CITY_NAME, (uint16_t)which, 0);
}

/* the calculator's keys while typing: alpha (and shift) choose letters, the keys type, backspace deletes */
static void text_keys(Win *wn) {
  if (key_pressed(K_SHIFT)) shift_armed = !shift_armed;
  if (key_pressed(K_ALPHA)) {
    if (shift_armed || key_held(K_SHIFT)) alpha_mode = alpha_mode == AL_CAPS_ONCE ? AL_CAPS_LOCK : AL_CAPS_ONCE;
    else alpha_mode = alpha_mode == AL_ONCE ? AL_LOCK : alpha_mode == AL_OFF ? AL_ONCE : AL_OFF;
    shift_armed = false;
  }
  if (key_repeat(K_BACKSPACE)) text_delete(wn->arg);
  int k = input.text_key;
  if (k < 0 || k > TK_ANS) return;
  char c = alpha_mode != AL_OFF || !plain_chars[k] ? alpha_chars[k] : plain_chars[k];
  if (!c) return;
  if ((alpha_mode == AL_CAPS_ONCE || alpha_mode == AL_CAPS_LOCK) && c >= 'a' && c <= 'z') c = (char)(c - 32);
  if (alpha_mode == AL_ONCE || alpha_mode == AL_CAPS_ONCE) alpha_mode = AL_OFF;
  text_add(wn->arg, c);
}

/* the on-screen letters: three rows of 13, then capitals, space and delete */
static const char grid_rows[3][14] = {"ABCDEFGHIJKLM", "NOPQRSTUVWXYZ", "0123456789-'."};
enum { TA_CAPS = 1, TA_SPACE, TA_DELETE, TA_CONFIRM, TA_CHAR = 100 };
#define CELL_W 19
#define CELL_H 15

static void grid_key(WinOut *w, int x, int wd, const char *label, int action) {
  bool f = win_small_button(w, x, wd, CELL_H, action);
  int bx, by;
  if (win_area(w, 0, 0, &bx, &by)) {
    bool on = action == TA_CAPS && grid_caps;
    ui_box(bx + x, by, wd, CELL_H, on || (f && key_held(K_OK)) ? BX_BUTTON_DOWN : f ? BX_BUTTON_HOVER : BX_BUTTON);
    ui_text(FONT_STANDARD, bx + x + (wd - ui_text_w(FONT_STANDARD, label)) / 2, by + 1, label, C_BLACK);
  }
}

void city_name_content(WinOut *w) {
  int which = w->win->arg, size;
  char *t = text_of(which, &size);
  if (which == TX_CITY_NAME) {
    win_title(w, L_change_city_name);
    win_para(w, FONT_STANDARD, L_give_city_name, 250, C_BLACK);
  } else {
    win_para(w, FONT_STANDARD, L_color_code_enter, 250, C_BLACK);
  }
  win_space(w, 4);
  /* the text box (TextInput: 250 x 20), its caret, and alpha's state at its right ("a", "A"; underlined: locked) */
  int x, y;
  if (win_area(w, 250, 20, &x, &y)) {
    ui_box(x, y, 250, 20, BX_BUTTON);
    int tw = 0;
    if (*t) tw = ui_text(FONT_STANDARD, x + 3, y + 4, t, C_BLACK);
    else if (which == TX_CITY_NAME) ui_text(FONT_STANDARD, x + 3, y + 4, L_city_name, 0x8410);
    if ((int)cu.gui_timer % 40 < 24) ui_rect(x + 3 + tw + 1, y + 4, 1, 12, C_BLACK, 255);
    if (alpha_mode != AL_OFF) {
      const char *a = alpha_mode >= AL_CAPS_ONCE ? "A" : "a";
      int aw = ui_text_w(FONT_STANDARD, a);
      ui_text(FONT_STANDARD, x + 250 - 4 - aw, y + 4, a, C_BLUE);
      if (alpha_mode == AL_LOCK || alpha_mode == AL_CAPS_LOCK) ui_rect(x + 250 - 4 - aw, y + 16, aw, 1, C_BLUE, 255);
    }
  }
  win_space(w, 3);
  win_para(w, FONT_10, L_keypad_typing_hint, 250, C_BLACK);
  win_space(w, 2);
  /* the letters (capitals or not), the digits and signs, then capitals, space and delete */
  for (int r = 0; r < 3; r++) {
    /* (a row is one primitive: each key is still a button of its own for the arrows) */
    char row[13];
    int focused = 255;
    for (int i = 0; i < 13; i++) {
      char c = grid_rows[r][i];
      if (!grid_caps && c >= 'A' && c <= 'Z') c = (char)(c + 32);
      row[i] = c;
      if (win_small_button(w, i * CELL_W, CELL_W, CELL_H, TA_CHAR + (uint8_t)c)) focused = i;
    }
    if (win_area(w, 13 * CELL_W, 0, &x, &y)) ui_keys(x, y, CELL_W, CELL_H, row, 13, focused, key_held(K_OK));
    w->y += CELL_H;
  }
  grid_key(w, 0, 3 * CELL_W, L_keypad_caps, TA_CAPS);
  grid_key(w, 3 * CELL_W, 7 * CELL_W, L_keypad_space, TA_SPACE);
  grid_key(w, 10 * CELL_W, 3 * CELL_W, L_keypad_delete, TA_DELETE);
  w->y += CELL_H;
  if (w->maxw < 250) w->maxw = 250;
  win_space(w, 4);
  if (which == TX_COLOR_CODE) {
    const char *label = L_confirm;
    int a = TA_CONFIRM;
    win_bottom(w, &label, &a, 1, L_close, ACT_CLOSE);
  } else {
    win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
  }
}

static bool parse_color(const char *s, int32_t *rgb);
static void color_confirm(Win *picker, int32_t col);

void city_name_action(Win *wn, int a) {
  int which = wn->arg;
  if (a >= TA_CHAR) text_add(which, (char)(a - TA_CHAR));
  else if (a == TA_SPACE) text_add(which, ' ');
  else if (a == TA_DELETE) text_delete(which);
  else if (a == TA_CAPS) grid_caps = !grid_caps;
  else if (a == TA_CONFIRM) {
    /* the colour code: the picker's colour if it reads as one, else the original's alert */
    int32_t col;
    bool ok = parse_color(code_buf, &col);
    win_close();
    Win *picker = win_top();
    if (!ok) win_open_text(NULL, L_color_not_parsed);
    else if (picker && picker->kind == WK_COLOR) color_confirm(picker, col);
  }
}

/* ColorParser.parseHex: "RRGGBB" (or "AARRGGBB"), or after "#" or "0x": RGB, RGBA (doubled), RRGGBB, AARRGGBB */
static bool parse_color(const char *s, int32_t *rgb) {
  char h[9];
  int n = 0;
  bool prefixed = s[0] == '#' || (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'));
  const char *d = s[0] == '#' ? s + 1 : prefixed ? s + 2 : s;
  int len = (int)strlen(d);
  if (len > 8) return false;
  for (int i = 0; i < len; i++) {
    char c = d[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
  }
  if (!prefixed && len != 6 && len != 8) return false;
  if (s[0] == '#' && (len == 3 || len == 4)) {
    for (int i = 0; i < len; i++) h[n++] = d[i], h[n++] = d[i];
  } else {
    for (int i = 0; i < len; i++) h[n++] = d[i];
  }
  if (n != 6 && n != 8) return false;
  h[n] = 0;
  uint32_t v = 0;
  for (int i = n == 8 ? 2 : 0; i < n; i++) {
    char c = h[i];
    v = v << 4 | (uint32_t)(c <= '9' ? c - '0' : (c | 32) - 'a' + 10);
  }
  *rgb = (int32_t)(v & 0xFFFFFF);
  return true;
}

/* ---------------------------------------------------------------- the colour picker */
int32_t color_picked = 0xFFFFFF;         /* gui.ColorPicker.colorPicked */
int32_t color_picked_on_menu_open = -1;  /* gui.ColorPicker.colorPickedOnLastMultiMenuOpen */
static float pick_h, pick_s, pick_v;     /* its hue (degrees), saturation and value */
#define CIRCLE_W 120                     /* spr_colorcircle at 0.6: 120 x 133.8 */
#define CIRCLE_H 134

/* thx.color: Rgbx.toHsv (grey has hue -1) and Hsv.toRgbx, then Rgbx.toRgb */
static void rgb_to_hsv(int32_t rgb, float *h, float *s, float *v) {
  float r = (rgb >> 16 & 255) / 255.0f, g = (rgb >> 8 & 255) / 255.0f, b = (rgb & 255) / 255.0f;
  float mn = fminf(fminf(r, g), b), mx = fmaxf(fmaxf(r, g), b), delta = mx - mn;
  *v = mx;
  if (delta == 0) {
    *s = 0, *h = -1;
    return;
  }
  *s = delta / mx;
  float hh = r == mx ? (g - b) / delta : g == mx ? 2 + (b - r) / delta : 4 + (r - g) / delta;
  hh *= 60;
  *h = hh < 0 ? hh + 360 : hh;
}
static int32_t hsv_rgb(float h, float s, float v) {
  float r, g, b;
  if (s == 0) {
    r = g = b = v;
  } else {
    float hh = h / 60, f;
    int i = (int)floorf(hh);
    f = hh - i;
    float p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    switch (i) {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    default: r = v, g = p, b = q;
    }
  }
  return (int32_t)(((int)roundf(r * 255) & 255) << 16 | ((int)roundf(g * 255) & 255) << 8 | ((int)roundf(b * 255) & 255));
}

/* ColorPickerWindow.createWindow: for a decoration's colour (arg: the permanent) or the build menu's any-colour
 * look (arg: the class) */
void color_picker_open(int target, uint16_t arg) {
  if (target == COLOR_FOR_BUILDING) {
    int32_t c = bld_decoration_color(P(arg));
    if (c >= 0) color_picked = c;
    win_open(WK_COLOR, (uint16_t)target, arg);
  } else {
    win_open_only(WK_COLOR, (uint16_t)target, arg);
  }
  rgb_to_hsv(color_picked, &pick_h, &pick_s, &pick_v);
}

static void update_color(void) { color_picked = hsv_rgb(pick_h, pick_s, pick_v); }

/* onConfirm, then goPreviousWindow */
static void color_confirm(Win *picker, int32_t col) {
  int target = picker->arg;
  uint16_t arg = picker->arg2;
  if (target == COLOR_FOR_BUILDING) {
    Perm *p = P(arg);
    if (p && (p->flags & PF_USED)) bld_customize_color(p, col);
    win_close();
  } else {
    /* the builder for the class's last look in that colour */
    const ClsText *ct = &cls_text[arg];
    int looks = (spr_w(pcls[arg].sprite) + 2) / 22;
    win_close();
    builder_start_ex(BK_BUILDING, arg, looks - 1, (ct->flags & CT_MIRRORABLE) && cu.mirrored, col);
    color_picked = col;
  }
}

enum { CP_CIRCLE = 1, CP_VALUE, CP_CONFIRM, CP_CODE };

void color_content(WinOut *w) {
  win_title_tint(w, L_choose_color, SPR_COLORPICKER_COLORPICKED, rgb565_near((uint32_t)color_picked));
  /* the circle (hue and saturation) with its bar (value) under it, the handles, and the colour (15 high) */
  int y0 = w->y, x, y;
  /* (the part the arrows change is framed as any focused button) */
  win_small_button(w, 5, CIRCLE_W, CIRCLE_W, CP_CIRCLE);
  w->y = y0 + CIRCLE_W;
  win_small_button(w, 5, CIRCLE_W, CIRCLE_H - CIRCLE_W + 12, CP_VALUE);
  w->y = y0;
  if (win_area(w, CIRCLE_W + 10, CIRCLE_H + 10 + 25, &x, &y)) {
    ui_spr_small(SPR_COLORCIRCLE, x + 5, y);
    float a = (pick_h < 0 ? 0 : pick_h) / 360 * 2 * (float)M_PI, r = pick_s * CIRCLE_W / 2;
    int cx = x + 5 + CIRCLE_W / 2 + (int)lroundf(r * cosf(a)), cy = y + CIRCLE_W / 2 + (int)lroundf(r * sinf(a));
    ui_spr(SPR_COLORPICKER_CIRCLE, cx - 6, cy - 6);
    ui_spr(SPR_COLORPICKER_TRIANGLE, x + 5 + (int)lroundf(pick_v * CIRCLE_W) - 6, y + CIRCLE_H - 1);
    ui_rect(x, y + CIRCLE_H + 20, CIRCLE_W + 10, 15, rgb565_near((uint32_t)color_picked), 255);
  }
  win_space(w, 4);
  const char *labels[2] = {L_confirm, L_color_code};
  int acts[2] = {CP_CONFIRM, CP_CODE};
  win_bottom(w, labels, acts, 2, L_close, ACT_CLOSE);
}

void color_action(Win *wn, int a) {
  if (a == CP_CONFIRM || a == CP_CIRCLE || a == CP_VALUE) color_confirm(wn, color_picked);
  else if (a == CP_CODE) text_window_open(TX_COLOR_CODE);
}

/* the arrows over the circle move its handle (a pixel a press, two a tick when held); down from its lowest point
 * goes to the bar, whose left and right change the value. False: the window's usual keys */
static int step_of(int bit) { return input.held_ticks[bit] > 8 ? 2 : (input.pressed >> bit & 1) ? 1 : 0; }
static bool color_keys(Win *wn) {
  if (key_held(K_SHIFT)) return false;
  int dx = step_of(1) - step_of(0), dy = step_of(3) - step_of(2);
  if (wn->focus == 0) {
    if (!dx && !dy) return false;
    float a = (pick_h < 0 ? 0 : pick_h) / 360 * 2 * (float)M_PI, r = pick_s * CIRCLE_W / 2;
    float px = r * cosf(a), py = r * sinf(a), nx = px + dx, ny = py + dy, len = sqrtf(nx * nx + ny * ny);
    if (len > CIRCLE_W / 2) nx *= CIRCLE_W / 2 / len, ny *= CIRCLE_W / 2 / len, len = CIRCLE_W / 2;
    if (dy > 0 && !dx && ny <= py + 0.01f) return false;   /* the lowest point: on to the bar */
    pick_s = fminf(len / (CIRCLE_W / 2), 1);
    pick_h = fmodf(atan2f(ny, nx) / (2 * (float)M_PI) * 360 + 360, 360);
    update_color();
    return true;
  }
  if (wn->focus == 1 && dx) {
    pick_v = fminf(1, fmaxf(0, pick_v + dx / (float)CIRCLE_W));
    update_color();
    return true;
  }
  return false;
}

/* the keys of these windows before the usual ones (win_input) */
bool pick_keys(Win *wn) {
  if (wn->kind == WK_CITY_NAME) {
    text_keys(wn);
    return false;
  }
  if (wn->kind == WK_COLOR) return color_keys(wn);
  return false;
}
