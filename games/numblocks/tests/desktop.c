/* Interactive desktop player for NumBlocks (X11, no extra deps).
 *
 * HUMAN-ONLY manual playtester: opens a real window and needs a display.
 * Automated agents must NOT run it (no display, never exits on its own);
 * use the scripted headless harness instead: build/play (tests/play.c)
 *   make host && ./build/play --frames 120 --mobs --torches --rain --shots 30,60,90,119 --out /tmp/new
 * plus `make test` (build/unit). Keep this binary quiet: one help banner on
 * stdout at startup, errors only. No per-frame/per-event stderr logging.
 *
 * Reuses the game's HOST platform (src/plat_host.c): the screen goes to
 * host_fb, keys come from host_raw/host_keys, time is host_time.
 * Opens a 960x720 window (3x), maps PC keys to calculator keys, runs the
 * real game loop with real time. Start: ./build/desktop (from games/numblocks).
 * F12 saves a screenshot PPM into build/desktop-saves/.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "nb.h"

extern uint16_t host_fb[SCREEN_W * SCREEN_H];
extern uint32_t host_time, host_keys;
extern uint64_t host_raw;
extern void host_save_dir(const char *d);
extern void host_shot(const char *path);
uint32_t keys_of(uint64_t raw);
void game_init(void);
bool game_frame(void);

/* PC key -> calculator key number (see gui.c actions[] defaults).
 * Gameplay first; typing in fields/chat follows the calculator's own
 * multi-tap letters, so custom names are awkward (defaults work with OK). */
static int map_key(KeySym k) {
  if (k >= 'A' && k <= 'Z') k += 'a' - 'A';
  switch (k) {
    case XK_Left: return 0;
    case XK_Up: return 1;
    case XK_Down: return 2;
    case XK_Right: return 3;
    case XK_Return: case XK_j: return 4;    /* OK: mine, attack */
    case XK_k: return 5;                    /* Back: place, use */
    case XK_Home: case XK_h: return 6;      /* Home: save and quit */
    case XK_space: case XK_Shift_L: case XK_Shift_R: return 12; /* jump */
    case XK_c: case XK_Alt_L: case XK_Alt_R: case XK_Control_L: return 13; /* sneak */
    case XK_q: return 14;                   /* x,n,t: drop */
    case XK_e: return 15;                   /* var: inventory */
    case XK_Tab: return 16;                 /* toolbox: autocomplete */
    case XK_i: return 21;                   /* sprint */
    case XK_l: return 20;                   /* log: pick block */
    case XK_w: case XK_comma: return 22;    /* forward */
    case XK_F3: case XK_f: return 24;       /* sin: debug overlay */
    case XK_F4: case XK_g: return 25;       /* cos: with sin, the game mode switcher */
    case XK_a: return 27;                   /* strafe left */
    case XK_s: return 28;                   /* backwards */
    case XK_d: return 29;                   /* strafe right */
    case XK_7: return 30;
    case XK_8: return 31;
    case XK_9: return 32;
    case XK_4: return 36;
    case XK_5: return 37;
    case XK_6: return 38;
    case XK_t: return 39;                   /* chat */
    case XK_slash: return 40;               /* command */
    case XK_1: return 42;
    case XK_2: return 43;
    case XK_3: return 44;
    case XK_0: case XK_at: return 48;
    case XK_p: case XK_Escape: return 51;   /* ans: pause */
    case XK_u: case XK_KP_Enter: return 52; /* EXE: as OK (one item) */
    default: return -1;
  }
}

static uint64_t now_ms(struct timespec t0) {
  struct timespec n;
  clock_gettime(CLOCK_MONOTONIC, &n);
  return (uint64_t)(n.tv_sec - t0.tv_sec) * 1000 + (n.tv_nsec - t0.tv_nsec) / 1000000;
}

int main(void) {
  mkdir("build/desktop-saves", 0755);
  host_save_dir("build/desktop-saves");

  Display *d = XOpenDisplay(NULL);
  if (!d) {
    fprintf(stderr, "desktop: cannot open X display (DISPLAY=%s)\n", getenv("DISPLAY"));
    return 1;
  }
  int scr = DefaultScreen(d);
  const int S = 3, W = SCREEN_W * S, H = SCREEN_H * S;
  Window win = XCreateSimpleWindow(d, RootWindow(d, scr), 0, 0, W, H, 0, 0, 0);
  XStoreName(d, win, "NumBlocks desktop");
  Atom del = XInternAtom(d, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(d, win, &del, 1);
  XSelectInput(d, win, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);
  XMapWindow(d, win);
  XImage *img = XCreateImage(d, DefaultVisual(d, scr), (unsigned)DefaultDepth(d, scr), ZPixmap, 0,
                             malloc((size_t)W * H * 4), W, H, 32, 0);
  if (!img || !img->data) {
    fprintf(stderr, "desktop: XCreateImage failed\n");
    return 1;
  }

  game_init();
  printf("NumBlocks desktop: arrows look | WASD move | Space jump | J mine/OK | K place/use\n"
         "  E inventory | 1-9 hotbar | P/Esc pause | L pick block | Q drop | T chat | Home/H save+quit | F12 shot\n");

  static int cnt[64];
  static int kc_raw[256];
  static bool kc_down[256];
  for (int i = 0; i < 256; i++) kc_raw[i] = -1;

  struct timespec t0;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  uint64_t last = 0;
  bool running = true;
  int frame = 0;
  while (running) {
    uint64_t now = now_ms(t0);
    uint32_t el = (uint32_t)(now - last);
    last = now;
    if (el > 250) el = 250;
    host_time += el;

    while (XPending(d)) {
      XEvent e;
      XNextEvent(d, &e);
      if (e.type == ClientMessage) {
        running = false; /* window X: save via Home below */
        break;
      } else if (e.type == KeyPress || e.type == KeyRelease) {
        int kc = e.xkey.keycode;
        if (kc < 0 || kc > 255) continue;
        if (e.type == KeyPress) {
          KeySym ks = XLookupKeysym(&e.xkey, 0);
          if (ks == XK_F12) {
            char p[128];
            snprintf(p, sizeof p, "build/desktop-saves/shot_%04d.ppm", frame);
            host_shot(p);
            printf("desktop: shot %s\n", p);
            fflush(stdout);
            continue;
          }
          if (kc_down[kc]) continue; /* autorepeat */
          int r = map_key(ks);
          kc_down[kc] = true;
          kc_raw[kc] = r;
          if (r >= 0) cnt[r]++;
        } else {
          if (!kc_down[kc]) continue;
          int r = kc_raw[kc];
          kc_down[kc] = false;
          kc_raw[kc] = -1;
          if (r >= 0 && cnt[r] > 0) cnt[r]--;
        }
      }
    }
    if (!running) break;

    uint64_t raw = 0;
    for (int i = 0; i < 64; i++)
      if (cnt[i]) raw |= 1ULL << i;
    host_raw = raw;
    host_keys = keys_of(raw);

    if (!game_frame()) break; /* Home: saved, quit */

    for (int y = 0; y < SCREEN_H; y++)
      for (int x = 0; x < SCREEN_W; x++) {
        uint16_t v = host_fb[y * SCREEN_W + x];
        uint32_t px = (uint32_t)(((v >> 11) * 255 / 31) << 16 | (((v >> 5) & 63) * 255 / 63) << 8 | ((v & 31) * 255 / 31));
        for (int dy = 0; dy < S; dy++)
          for (int dx = 0; dx < S; dx++)
            ((uint32_t *)img->data)[(y * S + dy) * W + x * S + dx] = px;
      }
    XPutImage(d, win, DefaultGC(d, scr), img, 0, 0, 0, 0, W, H);
    XFlush(d);
    frame++;

    uint64_t spent = now_ms(t0) - now;
    if (spent < 16) usleep((useconds_t)((16 - spent) * 1000));
  }

  /* window closed: quit through Home so the world saves */
  host_raw |= 1ULL << 6;
  host_keys = keys_of(host_raw);
  for (int i = 0; i < 600 && game_frame(); i++) {
  }
  printf("desktop: bye (saves in build/desktop-saves/)\n");
  XCloseDisplay(d);
  return 0;
}
