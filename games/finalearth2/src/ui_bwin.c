/* The Final Earth 2: a permanent's window (Permanent.createWindow,
 * Building.createWindowAddBottomButtons, the House, Work and School lines,
 * gui.CreateBuildingUpgrades, gui.CreateCityUpgrades, gui.CreatePolicies),
 * the lists of its residents and workers (gui.MultiFollowWindow) and a
 * citizen's window (gui.FollowingCitizen, cityActions.ChangeCitizenVitalBuildings). */
#include "game.h"
#include "uii.h"
#include "view.h"

enum {
  BW_DESTROY = ACT_DESTROY,
  BW_RESIDENTS = 1000, BW_WORKERS, BW_ONE, BW_ALL, BW_MODE_ONE, BW_MODE_ALL,
  BW_OPTION = 2000,   /* + the option's index */
  BW_LOOK = 3000,     /* + the option's index: change_variant */
  BW_SMART_LABS = 3900
};

/* a class's name in the original's words */
static const char *perm_title(const Perm *p) { return cls_text[p->cls].name; }

/* Building.tryDestroy, with its warnings */
void try_destroy(uint16_t id) {
  if (cu.selected == id) cu.selected = 0;
  switch (building_try_destroy(id)) {
  case DR_PROTECTED: win_open_text(NULL, L_secret_society_forbidden); break;
  case DR_INVALID_STRUCTURE: win_open_text(NULL, L_invalid_structure); break;
  case DR_INVALID_STRUCTURE_HOLE: win_open_text(NULL, L_invalid_structure_2); break;
  default: view_edges(&city_edges); break;
  }
}

/* ---------------------------------------------------------------- upgrades (CreateBuildingUpgrades...) */
#define MAX_OPTS 24

static void upgrades_part(WinOut *w, Perm *p, bool global, int base) {
  UpgradeOption opts[MAX_OPTS];
  int n = upgrade_options(p, opts, MAX_OPTS);
  bool all = (w->win->flags & WF2_ALL) || global;
  int kinds[] = {UO_UPGRADE, UO_MODE, UO_CITY, UO_POLICY};
  const char *headers[] = {L_upgrades, L_building_modes, L_city_upgrades, L_city_policies};
  for (int k = 0; k < 4; k++) {
    int kind = kinds[k], count = 0;
    bool any_new = false, any_cost = false;
    for (int i = 0; i < n; i++) {
      if (opts[i].kind != kind) continue;
      if (global && opts[i].owned && kind != UO_POLICY && kind != UO_MODE) {
        /* the global window shows only what isn't bought yet */
        float total[M_COUNT];
        bool ca;
        if (kind != UO_UPGRADE || upgrade_all_count(p, opts[i].upgrade, total, &ca) == 0) continue;
      }
      count++;
      if (!opts[i].owned) {
        any_new = true;
        for (int m = 0; m < M_COUNT; m++)
          if (m != M_KNOWLEDGE && opts[i].price[m] > 0) any_cost = true;
      }
    }
    if (!count) continue;
    if (!global) {
      if ((kind == UO_UPGRADE || kind == UO_MODE) && any_new) {
        int count_type = 0;
        for (int q = 1; q < MAX_PERMS; q++)
          if ((perms[q].flags & PF_USED) && perms[q].cls == p->cls) count_type++;
        const char *all_text = kind == UO_UPGRADE ? (any_cost ? L_upgrade_afford : L_upgrade_all_explain)
                             : (any_cost ? L_building_mode_afford : L_building_mode_all_explain);
        lo_1(w->tip_buf, sizeof w->tip_buf, all_text, LN(count_type));
        win_heading(w, headers[k], kind == UO_UPGRADE ? BW_ONE : BW_MODE_ONE, kind == UO_UPGRADE ? BW_ALL : BW_MODE_ALL,
                    all, any_cost, kind == UO_UPGRADE ? L_upgrade_only_this : L_building_mode_only_this, w->tip_buf);
      } else {
        win_header(w, headers[k]);
      }
    }
    for (int i = 0; i < n; i++) {
      UpgradeOption *o = &opts[i];
      if (o->kind != kind) continue;
      if (kind == UO_POLICY) {
        win_checkbox(w, o->owned, base + i, o->title, o->text, NULL);
        continue;
      }
      if (global && o->owned && kind != UO_MODE) {
        float total[M_COUNT];
        bool ca;
        if (kind != UO_UPGRADE || upgrade_all_count(p, o->upgrade, total, &ca) == 0) continue;
      }
      if (o->owned && !global) {
        if (o->canChangeLook) {
          /* the active box, with its "Change Variant" button inside */
          win_activatable(w, true, -1, o->title, o->text, NULL, NULL);
          w->y -= 2;
          int save_cw = w->cw;
          win_row_begin(w);
          w->rx = 3;
          win_text_button(w, L_change_variant, base + 32 + i, BX_BUTTON, false, false);
          win_tip(w, NULL, L_change_appearance_upgrade);
          win_row_end(w);
          w->cw = save_cw;
          w->y += 4;
        } else {
          win_activatable(w, true, -1, o->title, o->text, NULL, NULL);
        }
        continue;
      }
      /* its price: for this building, or (the "all" button) for every building of its class */
      float price[M_COUNT];
      memcpy(price, o->price, sizeof price);
      char before[48];
      before[0] = 0;
      if (all && kind == UO_UPGRADE) {
        float total[M_COUNT];
        bool can_all;
        int num = upgrade_all_count(p, o->upgrade, total, &can_all);
        if (num <= 1) {
          str_cat(before, sizeof before, num == 0 ? L_per_building : global ? L_for_one_building : L_for_this_building);
        } else {
          if (can_all) str_cat(before, sizeof before, L_for_all_buildings);
          else lo_1(before, sizeof before, L_for_n_buildings, LN(num));
          for (int m = 0; m < M_COUNT; m++)
            if (m != M_KNOWLEDGE) price[m] = total[m];
        }
      }
      win_activatable(w, false, base + i, o->title, o->text, price, before);
    }
    if (kind == UO_UPGRADE && global && p->cls == CLS_Laboratory) {
      /* GlobalUpgradeWindow.addSmartUpgradeForLabs */
      float price[M_COUNT];
      bool any = upgrade_smart_labs_price(price);
      win_activatable(w, !any, BW_SMART_LABS, L_auto_upgrade, L_upgrade_all_labs, any ? price : NULL, NULL);
    }
    win_space(w, 4);
  }
}

static void upgrades_action(Perm *p, int a, bool global, int base) {
  UpgradeOption opts[MAX_OPTS];
  int n = upgrade_options(p, opts, MAX_OPTS);
  Win *wn = win_top();
  if (a == BW_ONE || a == BW_MODE_ONE) {
    wn->flags &= (uint8_t)~WF2_ALL;
  } else if (a == BW_ALL) {
    wn->flags |= WF2_ALL;
  } else if (a == BW_MODE_ALL) {
    wn->flags |= WF2_ALL;
    upgrade_mode_to_all(p);
  } else if (a == BW_SMART_LABS) {
    upgrade_smart_labs_buy();
  } else if (a >= base + 32 && a < base + 32 + n) {
    upgrade_change_look(p, opts[a - base - 32].upgrade);
  } else if (a >= base && a < base + n) {
    UpgradeOption *o = &opts[a - base];
    upgrade_press(p, o->upgrade, global || (wn->flags & WF2_ALL));
  }
}

/* the global upgrade window: a type's upgrades for all its buildings (createBuildingUpgradesForType) */
void upgrades_for_type(WinOut *w, Perm *p, int unused) {
  (void)unused;
  upgrades_part(w, p, true, 10000 + p->cls * 64);
}
void upgrades_for_type_action(int a) {
  if (a == BW_SMART_LABS) {
    upgrade_smart_labs_buy();
    return;
  }
  if (a < 10000) return;
  int c = (a - 10000) / 64;
  for (int k = 1; k < MAX_PERMS; k++)
    if ((perms[k].flags & PF_USED) && perms[k].cls == c) {
      upgrades_action(&perms[k], a, true, 10000 + c * 64);
      return;
    }
}

/* ---------------------------------------------------------------- the building's window */
void bwin_content(WinOut *w) {
  Win *wn = w->win;
  Perm *p = P(wn->arg);
  if (!p || !(p->flags & PF_USED)) {
    win_title(w, "");
    win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
    return;
  }
  win_title(w, perm_title(p));
  char t[64];
  if (is_building(p)) {
    /* House, WorkWithHome, Work and School: their lines (addWindowInfoLines) */
    bool house = is_a(p, B_HOUSE), work = is_a(p, B_WORK);
    if (house || work) {
      win_row_begin(w);
      if (house) win_simple_button(w, SPR_HOUSING, lo_2(t, sizeof t, L_residents, LN(p->nresidents), LN(bld_residents(p))), BW_RESIDENTS);
      if (work) win_simple_button(w, SPR_WORK, lo_2(t, sizeof t, L_workers, LN(p->nworkers), LN(bld_jobs(p))), BW_WORKERS);
      win_row_end(w);
      win_space(w, 4);
      if (is_a(p, B_SCHOOL)) win_para(w, FONT_STANDARD, lo_2(t, sizeof t, L_students, LN(p->nstudents), LN(bld_student_capacity(p))), 250, C_BLACK);
      if (house) win_para(w, FONT_STANDARD, lo_1(t, sizeof t, L_quality, LN((int)bld_attractiveness(p))), 250, C_BLACK);
    }
  }
  /* the class's own lines and buttons; its upgrades; then what the class shows after them (win_later) */
  bool own = PC(p)->ops && PC(p)->ops->window;
  if (own) PC(p)->ops->window(p, w);
  w->muted = false;
  if (is_building(p)) upgrades_part(w, p, false, BW_OPTION);
  if (own) {
    w->later = w->muted = true;
    PC(p)->ops->window(p, w);
    w->muted = false;
  }
  /* the bottom row: the class's buttons (win_bottom_button), a building's Destroy, then Close */
  const char *labels[3], *heads[3], *tips[3];
  int acts[3], n = 0;
  for (int i = 0; i < w->nbottom; i++)
    labels[n] = w->bottom[i].label, heads[n] = w->bottom[i].tip_header, tips[n] = w->bottom[i].tip, acts[n++] = w->bottom[i].action;
  if (is_building(p)) {
    if (!story.disableDestroy) {
      labels[n] = (wn->flags & WF2_CONFIRM) ? L_really_destroy : L_destroy;
      heads[n] = NULL, tips[n] = (wn->flags & WF2_CONFIRM) ? NULL : L_destroy_this_building;
      acts[n++] = BW_DESTROY;
    }
  }
  win_bottom_ex(w, labels, acts, heads, tips, n, L_close, ACT_CLOSE);
}

void bwin_action(Win *wn, int a) {
  Perm *p = P(wn->arg);
  if (!p || !(p->flags & PF_USED)) return;
  if (a == BW_DESTROY) {
    if (!(wn->flags & WF2_CONFIRM)) {
      wn->flags |= WF2_CONFIRM;
    } else {
      win_close_all();
      try_destroy(wn->arg);
    }
    return;
  }
  if (a == BW_RESIDENTS || a == BW_WORKERS) {
    win_open(WK_FOLLOW_LIST, wn->arg, (uint16_t)(a == BW_WORKERS));
    return;
  }
  if (a >= BW_ONE) {
    upgrades_action(p, a, false, BW_OPTION);
    return;
  }
  /* the user interface's own windows that a building's button asks for */
  switch (a) {
  case WA_UI_BIRTH_CONTROL: win_open(WK_BIRTH, 0, 0); return;
  case WA_UI_MOVE_RUINS:
    /* Builder with a world resource to move: the cursor picks an empty column */
    {
      uint16_t id = wn->arg;
      win_close_all();
      moving_resource = id;
      cu.selected = id;   /* spr_selectedbuilding on the ruins */
      builder_start(BK_MOVE, 0, 0);
    }
    return;
  case WA_UI_DUPLICATE: {
    /* createOrRemoveBuilderForThis: a builder of its class, look, mirroring and colour (kept if it is one) */
    bool mirrored;
    int look = bld_decoration_look(p, &mirrored);
    int32_t col = bld_decoration_color(p);
    win_close_all();
    if (!(cu.build_kind == BK_BUILDING && cu.build_cls == p->cls && cu.build_decor == look && cu.build_color == col))
      builder_start_ex(BK_BUILDING, p->cls, look, mirrored, col);
    return;
  }
  case WA_UI_CITY_NAME: text_window_open(TX_CITY_NAME); return;
  case WA_UI_REPLACE_HOUSES: win_open(WK_REPLACE, 0, 0); return;
  case WA_UI_COLOR_PICKER: color_picker_open(COLOR_FOR_BUILDING, wn->arg); return;
  }
  if (PC(p)->ops && PC(p)->ops->window_action) PC(p)->ops->window_action(p, a);
}

uint16_t moving_resource;

/* ---------------------------------------------------------------- residents and workers (MultiFollowWindow) */
/* a citizen's name (Resources.citizenNames, front-coded in lang_data.c) */
const char *cit_name_of(const Cit *c, char *buf, int size) {
  if (c->nameIndex < ncitizen_names) return citizen_name(c->nameIndex, buf, size);
  buf[0] = 0;
  str_cat(buf, size, L_this_citizen);
  if (buf[0] >= 'a' && buf[0] <= 'z') buf[0] = (char)(buf[0] - 32);
  return buf;
}

void flist_content(WinOut *w) {
  Win *wn = w->win;
  Perm *p = P(wn->arg);
  bool workers = wn->arg2;
  char t[96], name[48];
  if (!p || !(p->flags & PF_USED)) {
    win_close();
    return;
  }
  cu.selected = wn->arg;
  win_title(w, lo_1(t, sizeof t, workers ? L_workers_of : L_residents_of, LS(perm_title(p))));
  /* the citizens, in the order they came (workers by their place in the list) */
  int n = 0;
  for (int pass = 0; pass < 256 && pass >= 0; pass++) {
    bool any_left = false;
    for (int i = 1; i < MAX_CITS; i++) {
      Cit *c = &cits[i];
      if (!(c->flags & CF_ALIVE)) continue;
      if (workers ? c->job != wn->arg : c->home != wn->arg) continue;
      if (workers && c->jobIndex != pass) {
        if (c->jobIndex > pass) any_left = true;
        continue;
      }
      if (!workers && pass > 0) continue;
      win_text_button(w, cit_name_of(c, name, sizeof name), 4000 + i, BX_BUTTON, true, false);
      win_space(w, 4);
      n++;
    }
    if (!workers || !any_left) break;
  }
  if (!n) win_para(w, FONT_STANDARD, workers ? L_no_workers : L_no_residents, 250, C_BLACK);
  win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
}

void flist_action(Win *wn, int a) {
  (void)wn;
  if (a > 4000 && a < 4000 + MAX_CITS) {
    cu.following = (uint16_t)(a - 4000);
    cu.selected = 0;
    win_open(WK_CITIZEN, (uint16_t)(a - 4000), 0);
  }
}

/* ---------------------------------------------------------------- a citizen (FollowingCitizen) */
enum { CW_CHANGE = 1, CW_HOME, CW_WORK };
static const char *education_text(float e) {
  return e < 0.01f ? L_education_level_0 : e < 0.1f ? L_education_level_1 : e < 0.2f ? L_education_level_2
       : e < 0.4f ? L_education_level_3 : e < 0.6f ? L_education_level_4 : e < 0.7f ? L_education_level_5
       : e < 0.8f ? L_education_level_6 : e < 1 ? L_education_level_7 : L_education_level_8;
}

/* an ImageButton in spr_button_smaller (15 x 15, its icon 2 in) at x on the current row */
static void image_button(WinOut *w, int x, uint16_t icon, int action) {
  bool f = win_small_button(w, x, 15, 15, action);
  int bx, by;
  if (win_area(w, 0, 0, &bx, &by)) {
    ui_frame(SPR_BUTTON_SMALLER, 15, f ? (key_held(K_OK) ? 2 : 1) : 0, bx + x, by, 0, 255);
    ui_spr(icon, bx + x + 2, by + 2);
  }
  if (w->maxw < x + 15) w->maxw = x + 15;
}

void cwin_content(WinOut *w) {
  Win *wn = w->win;
  Cit *c = &cits[wn->arg];
  char t[96], name[48];
  cit_name_of(c, name, sizeof name);
  if (!(c->flags & CF_ALIVE)) {
    /* FollowingCitizen.onCitizenDie */
    win_title(w, lo_1(t, sizeof t, L_following, LS(name)));
    win_para(w, FONT_STANDARD, lo_1(t, sizeof t, L_citizen_passed_away, LS(name)), 250, C_BLACK);
    win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
    cu.following = 0;
    return;
  }
  bool can_work = cit_age(c) >= 16 && !c->school;
  if (wn->flags & WF2_CONFIRM) {
    /* createChangeHomeAndWorkOfCitizenWindow: OK on a building makes it the citizen's (the arrows move the
     * cursor over the city; var gives them to the window's buttons and back) */
    char t2[96];
    lo_1(t2, sizeof t2, L_for_citizen, LS(name));
    t[0] = 0;
    str_cat(str_cat(str_cat(t, sizeof t, can_work ? L_change_home_job : L_change_home_school), sizeof t, " "), sizeof t, t2);
    win_title(w, t);
    /* the bottom row: the buttons that centre the view on the home and on the job (or school), then Close */
    bool school = c->school || cit_age(c) < 16;
    int y0 = w->y;
    image_button(w, 0, SPR_HOUSING, CW_HOME);
    win_tip(w, NULL, L_center_on_house);
    image_button(w, 15 + 4, school ? SPR_ICON_SCHOOL : SPR_WORK, CW_WORK);
    win_tip(w, NULL, school ? L_center_on_school : L_center_on_work);
    w->y = y0;
    int close_w = ui_text_w(FONT_STANDARD, L_close) + 6;
    if (w->maxw < 15 + 4 + 15 + 2 + close_w) w->maxw = 15 + 4 + 15 + 2 + close_w;
    win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
    return;
  }
  win_title(w, lo_1(t, sizeof t, L_following, LS(name)));
  win_para(w, FONT_STANDARD, lo_2(t, sizeof t, L_age_and_education, LN((int)cit_age(c)), LS(education_text(c->educationLevel))), 250, C_BLACK);
  const char *label = can_work ? L_change_home_job : L_change_home_school;
  int a = CW_CHANGE;
  if (!story_is("cityofthekey") || !c->job || P(c->job)->cls != CLS_SecretSocietyHouse)
    win_bottom(w, &label, &a, 1, L_close, ACT_CLOSE);
  else
    win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
  /* the view follows the citizen */
  float x, y;
  cit_city_position(c, &x, &y);
  camera_follow(x, y);
}

/* the view centred on a building (and the cursor on it) */
static void center_on(uint16_t id) {
  Perm *p = P(id);
  if (!p) return;
  camera_follow((float)perm_city_x(p) + 10, (float)perm_city_y(p) + 10);
  cursor_to(perm_city_x(p) + 10, perm_city_y(p) + 10);
}

void cwin_action(Win *wn, int a) {
  Cit *c = &cits[wn->arg];
  if (a == CW_CHANGE) {
    uint16_t arg = wn->arg;
    win_open(WK_CITIZEN, arg, 0);
    win_top()->flags |= WF2_CONFIRM;
    /* the cursor starts on the citizen */
    float x, y;
    cit_city_position(&cits[arg], &x, &y);
    cursor_to((int)x, (int)y - 10);
  } else if (a == CW_HOME) {
    center_on(c->home);
  } else if (a == CW_WORK) {
    center_on(c->job ? c->job : c->school);
  }
}

/* ChangeCitizenVitalBuildings.performPermanentAction: the building under the cursor becomes the citizen's */
bool change_vital_building(uint16_t citizen, uint16_t id) {
  Cit *c = &cits[citizen];
  Perm *b = P(id);
  if (!b || !(c->flags & CF_ALIVE)) return false;
  if (story_is("cityofthekey") && b->cls == CLS_SecretSocietyHouse) {
    win_open_text(NULL, L_secret_society_forbidden);
    return true;
  }
  if (is_a(b, B_WORK) && !c->school && cit_age(c) >= 16) {
    Cit *lost = NULL;
    if (c->job != id) {
      for (int i = 1; i < MAX_CITS && !lost; i++)
        if ((cits[i].flags & CF_ALIVE) && cits[i].job == id && cits[i].jobIndex == 0) lost = &cits[i];
      if (lost && b->nworkers >= bld_jobs(b)) cit_lose_job(lost, false);
      cit_lose_job(c, true);
      c->flags &= (uint16_t)~CF_WITHOUT_JOB;
      c->job = id;
      b->nworkers++;
      bld_after_give_job(b, c);
      if (c->home && c->home != id && is_a(P(c->home), B_WORK) && bld_residents(P(c->home)) <= bld_jobs(P(c->home)))
        cit_evict_from_home(c);
    }
    if (is_a(b, B_HOUSE) && c->home != id) {
      bool full = b->nresidents >= bld_residents(b), can = false;
      if (lost && lost->home == id && full) cit_evict_from_home(lost), can = true;
      else if (full) {
        for (int i = 1; i < MAX_CITS; i++)
          if ((cits[i].flags & CF_ALIVE) && cits[i].home == id) { cit_evict_from_home(&cits[i]); break; }
        can = true;
      }
      if (!full || can) {
        cit_evict_from_home(c);
        c->home = id;
        b->nresidents++;
        c->flags |= CF_FORCED_HOME;
      }
    }
  } else if (is_a(b, B_HOUSE)) {
    if (c->home != id) {
      if (b->nresidents >= bld_residents(b))
        for (int i = 1; i < MAX_CITS; i++)
          if ((cits[i].flags & CF_ALIVE) && cits[i].home == id) { cit_evict_from_home(&cits[i]); break; }
      cit_evict_from_home(c);
      c->home = id;
      b->nresidents++;
      c->flags |= CF_FORCED_HOME;
      if (c->job && c->home != c->job && is_a(P(c->job), B_HOUSE) && bld_residents(P(c->job)) <= bld_jobs(P(c->job)))
        cit_lose_job(c, false);
    }
  } else if (is_a(b, B_SCHOOL) && cit_age(c) < 16 && c->school != id) {
    if (b->nstudents >= bld_student_capacity(b))
      for (int i = 1; i < MAX_CITS; i++)
        if ((cits[i].flags & CF_ALIVE) && cits[i].school == id) { cit_leave_school(&cits[i]); break; }
    cit_leave_school(c);
    c->school = id;
    b->nstudents++;
  } else {
    return false;
  }
  return true;
}

/* ---------------------------------------------------------------- the destroy step from the city (backspace) */
void destroy_content(WinOut *w) { bwin_content(w); }
void destroy_action(Win *wn, int a) { bwin_action(wn, a); }

/* ---------------------------------------------------------------- upgrading basic houses (ReplaceAllHousesWindow) */
/* the house types basic houses can become: unlocked (without a hint) or researched, not on rooftops */
static bool replace_choice(int c) {
  if (c == CLS_NormalHouse || pcls[c].category != pcls[CLS_NormalHouse].category || !pcls[c].residents) return false;
  if (pcls[c].base & B_ROOFTOP) return false;
  int st = unlock_state(c);
  return (st == UNLOCK_UNLOCKED && !cls_text[c].hint) || st == UNLOCK_RESEARCHED || st == UNLOCK_CREATEABLE;
}

void replace_content(WinOut *w) {
  win_title(w, L_upgrade_basic_houses);
  win_para(w, FONT_STANDARD, L_upgrade_basic_houses_hint, 250, C_BLACK);
  win_space(w, 2);
  /* the types as ImageButtons, 9 in the first row and 10 in each after (as written) */
  int in_row = 0;
  win_row_begin(w);
  for (int i = 0; i < nbuild_order; i++) {
    int c = build_order[i];
    if (!replace_choice(c)) continue;
    if (++in_row > 9) {
      win_row_end(w);
      win_row_begin(w);
      in_row = 0;
    }
    int x = w->rx, bx, by;
    bool f = win_small_button(w, x, 22, 22, 10 + c);
    win_tip(w, NULL, cls_text[c].name);
    w->rx = x + 22;
    if (w->rh < 22) w->rh = 22;
    if (w->maxw < w->rx) w->maxw = w->rx;
    int save = w->y;
    if (win_area(w, 0, 0, &bx, &by)) {
      ui_frame(SPR_BUILDINGBUTTON, 22, f ? (key_held(K_OK) ? 2 : 1) : 0, bx + x, by, 0, 255);
      type_image(bx + x + 1, by + 1, c);
    }
    w->y = save;
  }
  win_row_end(w);
  win_space(w, 6);
  win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
}

void replace_action(Win *wn, int a) {
  (void)wn;
  if (a >= 10 && a < 10 + CLS_COUNT) win_open(WK_REPLACE_TO, (uint16_t)(a - 10), 0);
}

/* ReplaceAllHousesWindowStep2: how many basic houses (NormalHouse itself, not its kinds), how many of them the city
 * can pay for (one at least), and the price: the new type's, less a basic house's recycled wood and stone,
 * rounded, times that number (knowledge once). shown: the price as displayed (no negative wood or stone) */
typedef struct { int houses, n; float total[M_COUNT], shown[M_COUNT]; } ReplacePlan;
static void replace_plan(int cls, ReplacePlan *rp) {
  memset(rp, 0, sizeof *rp);
  for (int i = 1; i < MAX_PERMS; i++)
    if ((perms[i].flags & PF_USED) && perms[i].cls == CLS_NormalHouse) rp->houses++;
  if (!rp->houses) return;
  float one[M_COUNT];
  for (int m = 0; m < M_COUNT; m++) one[m] = pcls[cls].cost[m];
  if (unlock_state(cls) == UNLOCK_RESEARCHED) one[M_KNOWLEDGE] = 0;
  one[M_WOOD] -= 4 * upgrades.recyclingAmount, one[M_STONE] -= 4 * upgrades.recyclingAmount;
  for (int m = 0; m < M_COUNT; m++) one[m] = floorf(one[m] + 0.5f);   /* roundAll (Math.round) */
  /* Materials.canAffordNumber without knowledge */
  long can = 1000000000;
  for (int m = 0; m < M_COUNT && can; m++) {
    if (m == M_KNOWLEDGE) continue;
    if (one[m] > city.materials.m[m]) can = 0;
    else if (one[m] > 0 && floor(city.materials.m[m] / one[m]) < can) can = (long)floor(city.materials.m[m] / one[m]);
  }
  rp->n = can < 1 ? 1 : can > rp->houses ? rp->houses : (int)can;
  for (int m = 0; m < M_COUNT; m++) rp->total[m] = m == M_KNOWLEDGE ? one[m] : one[m] * rp->n;
  memcpy(rp->shown, rp->total, sizeof rp->shown);
  if (rp->shown[M_WOOD] < 0) rp->shown[M_WOOD] = 0;
  if (rp->shown[M_STONE] < 0) rp->shown[M_STONE] = 0;
}

void replace_to_content(WinOut *w) {
  int cls = w->win->arg;
  char t[200], t2[96];
  win_title(w, lo_1(t, sizeof t, L_upgrade_to_x, LS(cls_text[cls].name)));
  ReplacePlan rp;
  replace_plan(cls, &rp);
  if (!rp.houses) {
    win_para(w, FONT_STANDARD, L_no_basic_houses, 250, C_BLACK);
  } else {
    t[0] = 0;
    str_cat(str_cat(str_cat(t, sizeof t, L_no_undo), sizeof t, "\n\n"), sizeof t,
            lo_2(t2, sizeof t2, L_upgrade_basic_for, LN(rp.n), LN(rp.houses)));
    win_para(w, FONT_STANDARD, t, 250, C_BLACK);
  }
  win_space(w, 4);
  /* the MaterialsCostDisplay (2 under it) */
  int x, y;
  float none[M_COUNT] = {0};
  const float *cost = rp.houses ? rp.shown : none;
  char after[24] = " ";
  str_cat(after, sizeof after, L_to_research);
  if (win_area(w, ui_cost(0, INT16_MIN, cost, "", after, true), 12 + 2, &x, &y)) ui_cost(x, y, cost, "", after, true);
  win_space(w, 4);
  win_text_button(w, rp.houses ? L_confirm : "-", 1, BX_BUTTON, false, false);
  win_space(w, 6);
  win_bottom(w, NULL, NULL, 0, L_close, ACT_CLOSE);
}

/* Confirm: each basic house (as many as paid for) destroyed for replacement and the new type built there */
void replace_to_action(Win *wn, int a) {
  if (a != 1) return;
  int cls = wn->arg;
  ReplacePlan rp;
  replace_plan(cls, &rp);
  if (!rp.houses || rp.n <= 0 || !mat_can_afford(rp.total)) return;
  unlocks_research(cls);
  int done = 0;
  for (int i = 1; i < MAX_PERMS && done < rp.n; i++) {
    Perm *p = &perms[i];
    if (!(p->flags & PF_USED) || p->cls != CLS_NormalHouse) continue;
    int wo = p->world, col = p->col, row = p->row;
    perm_destroy((uint16_t)i, true);
    uint16_t id = perm_new(cls, wo, col, row);
    if (id) bld_on_build(P(id));
    done++;
  }
  city_update_connections();
  sim_update_pathfinder(true, NONE);
  /* (the original then calls postCreate on the old houses, which does nothing for the new ones) */
  unlocks_check_build_related();
  mat_pay(rp.total);
  view_edges(&city_edges);
  win_close_all();
  lo_1(ui_text_buf, sizeof ui_text_buf, L_upgraded_houses, LN(done));
  win_open_text(NULL, ui_text_buf);
}
