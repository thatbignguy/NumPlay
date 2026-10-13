/* NumBlocks: the game loop. Game ticks run 20 times a second, as in
 * Minecraft; frames are drawn as fast as the calculator can, with the camera
 * placed between the last two ticks so it moves smoothly. */
#include <math.h>
#include "nb.h"
#include "gen.h"
#pragma GCC optimize("Os")   /* (not where the time goes: small) */

#ifndef HOST
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "NumBlocks";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
#endif

uint32_t perf_frames __attribute__((used));   /* read by tools/emu.py */
uint32_t perf_max __attribute__((used));      /* (and the slowest frame in a world, ms) */
uint32_t ticks_run;   /* ticks played (what waits a few ticks counts these: the time of day can stop or jump) */
uint32_t game_time = 1000;                    /* Minecraft's time of day: 0 sunrise, 6000 noon */
bool start_in_world;                          /* tests: straight into a new world */
int64_t start_seed;                           /* (its seed) */
int start_type;                               /* (and type) */

static uint32_t last, acc, keys_held, autosave;
static float px, py, pz;
static bool in_world;      /* playing; else the title screens, the world turning behind */
static float title_yaw;
/* The title's backdrop comes from Personalisation's Panorama pick (opt.panorama). This
 * is the whole state of it, packed into one byte: the pick showing (PAN_WORLD..
 * PAN_NETHER) and the setting it was picked for (Random re-picks each entry). */
static struct {
  uint8_t show : 3, pick : 3;
} title_pan;
/* The fixed backdrop world: one seed, its overworld and its nether, the camera always
 * at the same spot (the feet; the same block the pick was noted in). */
#define TITLE_SEED 123
static const float title_spot[2][3] = {
  {138.5f, 63.0f, 548.5f},   /* (PAN_OVER) the overworld */
  {4.5f, 33.0f, 106.5f},     /* (PAN_NETHER) the same world's nether */
};

void camera_reset(void) {
  px = pl.x, py = pl.y, pz = pl.z;
  player_look(pl.x, pl.y + 1.62f, pl.z, pl.yaw, pl.pitch);
}

/* The title's backdrop (Personalisation's Panorama): a fixed pick keeps itself every
 * visit; Random rolls one of the three. The roll mixes the clock - never the world's
 * rnd() stream, which the tests replay - with plat_entropy(), which says where this
 * run began, so even a run's first pick is a roll rather than always the same one.
 * It happens on every entry, so Random shows a different one on different visits. */
static void title_pick(void) {
  uint8_t p = opt.panorama <= PAN_NETHER ? opt.panorama : PAN_RANDOM;
  if (p == PAN_RANDOM) {
    uint64_t r = (uint64_t)(plat_millis() ^ plat_entropy() * 2654435761u) * 0x9E3779B97F4A7C15ull + perf_frames;
    r ^= r >> 30;
    r *= 0xBF58476D1CE4E5B9ull;
    r ^= r >> 31;
    title_pan.show = (uint8_t)(PAN_WORLD + r % 3);
  } else title_pan.show = p;
  title_pan.pick = p;
}

/* the backdrop world of the pick: the world played last (the player where they left
 * off), else a world of seed 0; or the fixed world - TITLE_SEED, the overworld or its
 * nether - with the camera at the spot above, so a fresh install has a world to show
 * without one being played, and the nether can be seen without walking to a portal */
static void title_backdrop(void) {
  if (title_pan.show == PAN_WORLD) {
    if (!opt.last_world || !load_world(opt.last_world)) {
      world_type = WT_DEFAULT;
      memcpy(world_preset, FLAT_DEFAULT, sizeof FLAT_DEFAULT);
      world_new(0, "nbt");
      player_spawn();
    }
  } else {
    /* (the fixed world: never flat, whichever world was played last) */
    world_type = WT_DEFAULT;
    memcpy(world_preset, FLAT_DEFAULT, sizeof FLAT_DEFAULT);
    world_new(TITLE_SEED, "nbt");
    if (title_pan.show == PAN_NETHER) world_switch_dim(DIM_NETHER);
    const float *s = title_spot[title_pan.show - PAN_OVER];
    pl.x = s[0], pl.y = s[1], pl.z = s[2];
    pl.vx = pl.vy = pl.vz = 0;
  }
  world_follow(pl.x, pl.y, pl.z);
  camera_reset();
}

/* the title's backdrop pick, and the world behind it */
static void title(void) {
  in_world = false;
  title_pick();
  title_backdrop();
  gui_menu(GUI_TITLE);
}

/* the loading screen, drawn once before the world is built */
static void loading(void) {
  gui = GUI_LOADING;
  render_frame(NULL, game_time);
  world_follow(pl.x, pl.y, pl.z);
  /* the camera starts free of any entity's eyes: what it rode was not saved */
  spectate = -1, spec_eye = 0;
  /* standing inside blocks (the world changed under a saved player): up to the first free space
   * (a spectator goes through them: it stays where it was saved) */
  if (pl.mode != 2)
    for (int k = 0; k < dim_height() - dim_min_y(); k++) {
      int x = (int)floorf(pl.x), y = (int)floorf(pl.y), z = (int)floorf(pl.z);
      if (!(blk_flags[world_get(x, y, z)] & BF_SOLID) && !(blk_flags[world_get(x, y + 1, z)] & BF_SOLID)) break;
      pl.y = (float)(y + 1);
      world_follow(pl.x, pl.y, pl.z);
    }
  camera_reset();
  gui = GUI_NONE;
  in_world = true;
  autosave = 0;
  acc = 0;
  last = plat_millis();
}

/* GuiCreateWorld: the seed typed, a number (Long.parseLong) or else words (String.hashCode);
 * none, or 0: random */
static int64_t parse_seed(void) {
  const char *s = seed_text;
  bool neg = *s == '-';
  int n = 0;
  uint64_t v = 0;
  bool num = s[neg] != 0;
  for (const char *c = s + neg; *c && num; c++, n++) {
    if (*c < '0' || *c > '9' || n >= 19) num = false;   /* (19 digits: no more than a long holds) */
    else v = v * 10 + (uint64_t)(*c - '0');
  }
  if (num && v > (uint64_t)INT64_MAX + neg) num = false;
  if (s[0] && !num) {
    int32_t h = 0;
    for (const char *c = s; *c; c++) h = (int32_t)((uint32_t)h * 31u + (uint8_t)*c);
    return h;
  }
  if (num && v) return neg ? (int64_t)(0 - v) : (int64_t)v;
  /* random: the time, mixed */
  uint64_t r = (uint64_t)plat_millis() * 0x9E3779B97F4A7C15ull + perf_frames;
  r ^= r >> 29;
  r *= 0xBF58476D1CE4E5B9ull;
  return (int64_t)(r ^ r >> 32);
}

void game_init(void) {
  /* just installed again? the saves come back from their copy; and the copy is made if there is none */
  copy_restore();
  copy_write();
  load_options();
#ifdef BENCH
  /* (a build for timing: straight into a new world of seed BENCH) */
  start_in_world = true, start_seed = BENCH, opt.keys_seen = KEY_SHEET;
#endif
  last = plat_millis();
  if (start_in_world) {
    new_world(1, start_seed, 0, false, start_type, "New World", start_type == WT_FLAT ? FLAT_DEFAULT : FLAT_DEFAULT);
    game_time = 1000;
#ifdef BENCH_FALL
    pl.mode = 1, pl.y += BENCH_FALL;   /* (timing a fall from that high, in Creative: no harm) */
#endif
    loading();
    return;
  }
  title();
  if (opt.keys_seen != KEY_SHEET) gui_menu(GUI_CONTROLS);   /* the first time, or new keys: the keys */
}

/* one frame: input, the ticks due, the picture; false once the player quits */
bool game_frame(void) {
  uint32_t now = plat_millis();
  uint32_t dt = now - last;
  last = now;
  if (in_world && gui == GUI_NONE && dt > perf_max) perf_max = dt;
  if (dt > 250) dt = 250;
  if (dt > 0) {
    float inst = 1000.0f / (float)dt;
    if (dbg_fps <= 0) dbg_fps = inst;
    else dbg_fps += (inst - dbg_fps) * 0.05f;
  }
  uint32_t k = plat_keys();
  uint32_t pressed = k & ~keys_held;
  keys_held = k;
  static bool pick_held, debug_held, pair_held, pair_used;
  /* one scan a frame for both (each scan is a call into the calculator) */
  uint64_t raw = plat_scan();
  bool pick = opt.keys[A_PICK] < 64 && (raw >> opt.keys[A_PICK] & 1);
  if (pick && !pick_held && in_world && gui == GUI_NONE && !pl.dead) player_pick_block();
  pick_held = pick;
  /* sin + cos: the game mode switcher, as Minecraft's F3 + F4. The pair wins over the
   * debug overlay sin alone gives, so sin's own toggle waits until it is let go, and is
   * dropped if cos was pressed while it was held. */
  uint32_t dbg_key = opt.keys[A_DEBUG] < 64 ? opt.keys[A_DEBUG] : 64;
  bool sin = opt.keys[A_DEBUG] < 64 && (raw >> opt.keys[A_DEBUG] & 1);
  bool cos = dbg_key + 1 < 64 && (raw >> (dbg_key + 1) & 1);
  if (sin && cos && !pair_held) {
    /* the pair's first frame: cos pressed with sin held (or the other way round) */
    if (gui == GUI_MODE) mode_next();
    else if (in_world && gui == GUI_NONE) mode_open();
    pair_used = true;
  }
  pair_held = sin && cos;
  if (debug_held && !sin) {
    if (gui == GUI_MODE) mode_use();   /* sin let go: the mode outlined */
    else if (in_world && gui == GUI_NONE && !pair_used) {
      opt.debug = (uint8_t)!opt.debug;
      save_options();
    }
    pair_used = false;
  }
  debug_held = sin;
  if (k & K_HOME) {
    /* Home: save and quit, from anywhere */
    if (in_world) {
      if (gui != GUI_NONE && gui < GUI_PAUSE) gui_close();
      save_world();
    }
    return false;
  }
  int gui_before = gui;
  gui_input(k, pressed);
  if (menu_choice) {
    int c = menu_choice;
    menu_choice = 0;
    switch (c) {
      case ACT_PLAY:
        if (load_world(play_slot)) loading();
        break;
      case ACT_NEW: {
        /* a free slot, the name made unique ("New World (2)") */
        int slot = world_free_slot();
        if (!slot) break;
        char name[WORLD_NAME + 1];
        int a = 0, b = (int)strlen(name_text);
        while (name_text[a] == ' ') a++;
        while (b > a && name_text[b - 1] == ' ') b--;
        name_text[b] = 0;
        world_unique_name(name_text + a, name);
        new_world(slot, parse_seed(), create_mode, create_cheats, create_type, name, create_preset);
        game_time = 0;
        loading();
        save_world();
        break;
      }
      case ACT_QUIT_APP: return false;
      case ACT_SAVE_QUIT:
      case ACT_TITLE:
        save_world();
        title();
        break;
      case ACT_RESPAWN:
        player_respawn();
        gui = GUI_NONE;
        world_follow(pl.x, pl.y, pl.z);
        camera_reset();
        break;
    }
  }
  if (!in_world) {
    /* the title: its backdrop turns slowly (a world, played or fixed), nothing moves */
    if (opt.panorama != title_pan.pick) {
      /* the setting changed under the title (Options, Personalisation): pick again */
      title_pick();
      title_backdrop();
    }
    title_yaw += dt * 0.006f;
    Camera c = {pl.x, pl.y + 1.62f, pl.z, title_yaw, 8};
    render_frame(gui == GUI_TITLE ? &c : NULL, game_time);
    perf_frames++;
    return true;
  }
  bool in_game = gui == GUI_NONE;
  /* keys held while a screen is open (or as it closes: OK on "Back to Game", Back out of the
   * inventory) do nothing in the world until they are let go, so they don't place or break */
  static uint32_t latched;
  if (!in_game || gui_before != GUI_NONE) latched = k & ~(K_FWD | K_BACKW | K_STRAFE_L | K_STRAFE_R);
  else latched &= k;
  k &= ~latched, pressed &= ~latched;
  /* looking around: arrows, 150 degrees a second sideways, 100 up and down (x the look speed) */
  if (in_game) {
    float turn = dt / 1000.0f * opt.look / 100.0f;
    if (k & K_LEFT) pl.yaw -= 150 * turn;
    if (k & K_RIGHT) pl.yaw += 150 * turn;
    if (pl.yaw >= 180) pl.yaw -= 360;   /* (kept small, for the floats' precision) */
    if (pl.yaw < -180) pl.yaw += 360;
    if (k & K_UP) pl.pitch -= 100 * turn;
    if (k & K_DOWN) pl.pitch += 100 * turn;
    if (pl.pitch > 90) pl.pitch = 90;
    if (pl.pitch < -90) pl.pitch = -90;
  }
  bool paused = gui == GUI_PAUSE || gui == GUI_MODE || gui == GUI_OPTIONS || gui == GUI_CTRLMENU || gui == GUI_VIDEO || gui == GUI_CONTROLS || gui == GUI_LAN || gui == GUI_KEYS || gui == GUI_MODS || gui == GUI_PERSONAL;
  if (!paused) acc += dt;
  /* a key pressed on a frame between ticks waits for the next tick (else that press is lost) */
  static uint32_t game_pressed;
  uint32_t game_keys = in_game ? k : 0;
  game_pressed = in_game ? game_pressed | pressed : 0;
  while (acc >= 50) {
    px = pl.x, py = pl.y, pz = pl.z;
    player_tick(game_keys | game_pressed, game_pressed);
    game_pressed = 0;
    ents_tick();
    world_tick();
    gui_tick();
    hand_tick();
    ticks_run++;
    if (rule(GR_DAYLIGHT_CYCLE)) game_time++;
    acc -= 50;
    /* Minecraft saves every 45 seconds */
    if (++autosave >= 900) {
      autosave = 0;
      save_world();
    }
  }
  if (pl.dead && gui != GUI_DEATH) {
    if (gui != GUI_NONE && gui < GUI_PAUSE) gui_close();
    gui_menu(GUI_DEATH);
  }
  world_follow(pl.x, pl.y, pl.z);
  float t = acc / 50.0f;
  tick_frac = t;
  /* the eyes: a spectator riding an entity has that entity's height, its own 1.62
   * otherwise (1.54 sneaking, as Minecraft's) */
  float eye = spec_eye ? spec_eye : pl.sneaking ? 1.54f : 1.62f;
  Camera c = {px + (pl.x - px) * t, py + (pl.y - py) * t + eye, pz + (pl.z - pz) * t, pl.yaw, pl.pitch};
  if (opt.bobbing && !pl.flying) {
    /* EntityRenderer.setupViewBobbing: the eyes sway side to side and dip with each step */
    float w = (pl.prev_walked + (pl.walked - pl.prev_walked) * t) * 3.14159265f;
    float b = pl.prev_bob + (pl.bob - pl.prev_bob) * t;
    float side = sinf(w) * b * 0.5f, dip = -fabsf(cosf(w) * b);
    float yr = pl.yaw * 0.017453292f;
    c.x += -cosf(yr) * side, c.z += -sinf(yr) * side, c.y += dip;
    c.pitch += fabsf(cosf(w - 0.2f) * b) * 5;
  }
  /* what the crosshair is on: the ray through the middle of this picture */
  if (!pl.dead) player_look(c.x, c.y, c.z, c.yaw, c.pitch);
  if (pl.dead) c.y = pl.y + 0.3f;
  if (pl.sleep_timer) c.y = pl.y + 0.3f;
  render_frame(&c, game_time);
  perf_frames++;
  return true;
}

#ifndef HOST
int main(void) {
  /* the world generator's summary cache, here on the stack: some calculator software gives apps less RAM */
  uint32_t gen_cache[GEN_CACHE_BYTES / 4];
  gen_set_cache(gen_cache);
  plat_begin();
  game_init();
  while (game_frame()) {}
  return plat_end();
}
#endif
