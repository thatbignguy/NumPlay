/* The Final Earth 2: services (buildings.md 8): schools, libraries, herb
 * gardens and medical clinics. */
#include "bldi.h"

static void plain_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, PC(p)->sprite, false);
  draw_upgrades(b, p, layer, x, y, false);
  draw_upgrades(b, p, layer, x, y, true);
}

/* the sum of its workers' education speed modifiers */
static float workers_education(const Perm *p) {
  float sum = 0;
  uint16_t id = pid(p);
  for (int i = 1; i < MAX_CITS; i++)
    if ((cits[i].flags & CF_ALIVE) && cits[i].job == id) sum += cit_education_speed_modifier(&cits[i]);
  return sum;
}

/* ---------------------------------------------------------------- School */
static int school_capacity(const Perm *p) { (void)p; return (int)floorf(policies.schoolClassSizeMod * 30 + 0.5f); }
static void school_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  step_to_x(c, 3, timeMod);
}
/* beAtSchool: a seat once a visit, and a day's education */
static void school_be_at(Perm *p, Cit *c, float timeMod) {
  (void)timeMod;
  if (c->flags & CF_BUILDING_INITED) return;
  switch (student_index(p, c) % 3) {
  case 0: move_x(c, rnd_between(6, 7)); break;
  case 1: cit_change_floor_and_move_random(c, 3, 7, THEN_NONE, 0); break;
  default: move_x(c, rnd_between(12, 16)); break;
  }
  float cap = policies.schoolMaxEdu, e = c->educationLevel + 0.07f * policies.schoolMaxEdu;
  if (e > cap) e = cap;
  if (e > c->educationLevel) c->educationLevel = e;
  c->flags |= CF_BUILDING_INITED;
}
static void school_window(Perm *p, WinOut *w) { win_line2(w, "[0]/[1] students", p->nstudents, school_capacity(p)); }
const PermOps ops_School = {.work = school_work, .be_at_school = school_be_at, .student_capacity = school_capacity,
                            .draw = plain_draw, .window = school_window};

/* ---------------------------------------------------------------- Library */
static uint8_t TH_LIBRARY_MOVE;
static void th_library_move(Cit *c, uint16_t arg) {
  (void)arg;
  int x = c->relativeY < 5 ? rnd_between(3, 7) : rnd_between(3, 16);
  cit_move_and_wait(c, x, rnd_between(30, 60), THEN_NONE, 0, false, false);
}
static bool library_open(const Perm *p) {
  return p->nworkers == 1 && first_worker_working(p) && hours_in(hour_now(), 14 - 0.5f, 22);
}
static float zero_capacity(const Perm *p) { (void)p; return 0; }
static void library_be_entertained(Perm *p, Cit *c, float timeMod) {
  (void)p, (void)timeMod;
  if (rnd_int(2) == 1) cit_change_floor(c, TH_LIBRARY_MOVE, 0);
  else cit_wait_random(c, 120, 180, TH_LIBRARY_MOVE, 0);
  educate(c, 0.025f, 1.5f);
}
static void library_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p;
  if (stop && hour_now() > 22) c->currentAction = 2;
  else step_to_x(c, 12, timeMod);
}
const PermOps ops_Library = {.work = library_work, .is_open = library_open, .entertainment_capacity = zero_capacity,
                             .be_entertained = library_be_entertained, .draw = draw_merging};

/* ---------------------------------------------------------------- HerbGarden */
static int herb_frames(void) { return frames_of(SPR_HERBGARDEN_PLANTS, 20); }
/* the modes: culinary herbs (restaurants around take more people) or medicinal ones */
void herb_garden_mode(Perm *p) {
  if (p->mode == MODE_HerbGarden_MedicalHerbs) {
    p->flags |= PF_IS_MEDICAL;
    p->s[0] = (uint8_t)(rnd_int(herb_frames() / 2) * 2 + 1);
  } else {
    p->flags &= (uint8_t)~PF_IS_MEDICAL;
    p->s[0] = (uint8_t)(rnd_int(herb_frames() / 2) * 2);
  }
}
static void herb_init(Perm *p) {
  p->s[0] = (uint8_t)(rnd_int(herb_frames() / 2) * 2);
  upgrade_first_mode(p);
}
static float herb_quality(const Perm *p) { (void)p; return 100; }
static float herb_medical_capacity(const Perm *p) { return p->mode == MODE_HerbGarden_MedicalHerbs ? 100 : 0; }
static float herb_capacity(const Perm *p) { return 50.0f * p->nworkers; }
static void herb_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p, (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  if (c->relativeY < 5 && worker_index(c) == 1) {
    cit_change_floor(c, THEN_NONE, 0);
    return;
  }
  int x = rnd_between(3, 16);
  cit_move_and_wait(c, x, rnd_between(30, 60), THEN_NONE, 0, false, true);
}
static void herb_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_merging(b, p, layer, x, y);
  if (layer == LAYER_BG) dr_frame(b, SPR_HERBGARDEN_PLANTS, 20, p->s[0] % herb_frames(), x, y, 0, 0, 0, 255, GFX_NO_TINT);
}
static void herb_window(Perm *p, WinOut *w) {
  (void)p;
  char a[96], b[96], t[200];
  lo2(a, sizeof a, "Herbs reduce your citizens' food consumption by [0]/[1]%", floorf(eating.currentFoodSaveByHerbsPct * 0.2f * 100 + 0.5f), 20);
  lo1(b, sizeof b, "Total medical happiness from herb gardens: [0]/20%",
      ncits ? floorf(100 * (happiness.healthCapacityByType[1] / ncits) + 0.5f) : 0);
  int k = 0;
  for (const char *s = a; *s && k < 190; s++) t[k++] = *s;
  t[k++] = '\n';
  for (const char *s = b; *s && k < 198; s++) t[k++] = *s;
  t[k] = 0;
  win_line(w, t);
  win_button(w, 0, "Change Herb Plants", WA_CYCLE_0, true);
}
static void herb_action(Perm *p, int a) {
  if (a == WA_CYCLE_0) p->s[0] = (uint8_t)((p->s[0] + 2) % herb_frames());
}
const PermOps ops_HerbGarden = {.init = herb_init, .work = herb_work, .medical_quality = herb_quality,
                                .medical_capacity = herb_medical_capacity, .herb_capacity = herb_capacity,
                                .draw = herb_draw, .window = herb_window, .window_action = herb_action};

/* ---------------------------------------------------------------- MedicalClinic and SpecializedMedicalClinic */
static float clinic_quality(const Perm *p) {
  float part = has_upgrade_bit(p, UB_MedicalClinic_MedicalEducation) ? 1 : 0;
  float extra = has_upgrade_bit(p, UB_MedicalClinic_AIAssistedDiagnosis) ? 25 : 0;
  return 50 + 0.25f * happiness.schoolHappiness * part + extra;
}
static float clinic_capacity(const Perm *p) {
  float extra = has_upgrade_bit(p, UB_MedicalClinic_AIAssistedDiagnosis) ? 50 : 0;
  return (50 + extra) * (p->nworkers == 0 ? 0 : workers_education(p));
}
static void clinic_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  if (c->relativeY < 5 && worker_index(c) == 1) cit_change_floor(c, THEN_NONE, 0);
  else step_to_x(c, c->relativeY < 5 ? 5 : 3, timeMod);
}
static void clinic_window(Perm *p, WinOut *w) {
  win_line1(w, "[0] capacity", floorf(bld_medical_capacity(p)));
  win_line1(w, "Quality: [0]", floorf(bld_medical_quality(p)));
  if (upgrades.hasBirthControl) win_button(w, 0, "Manage Birth Control", WA_UI_BIRTH_CONTROL, true);
}
const PermOps ops_MedicalClinic = {.work = clinic_work, .medical_quality = clinic_quality, .medical_capacity = clinic_capacity,
                                   .draw = plain_draw, .window = clinic_window};

static float special_quality(const Perm *p) { (void)p; return 100; }
static float special_capacity(const Perm *p) { return 50 * (p->nworkers == 0 ? 0 : workers_education(p)); }
static void special_work(Perm *p, Cit *c, float timeMod, bool stop) {
  (void)p, (void)timeMod;
  if (stop) {
    c->currentAction = 2;
    return;
  }
  if (c->relativeY < 5 && worker_index(c) == 1) {
    cit_change_floor(c, THEN_NONE, 0);
  } else {
    int x = rnd_between(3, c->relativeY < 5 ? 17 : 8);
    cit_move_and_wait(c, x, rnd_between(60, 90), THEN_NONE, 0, false, true);
  }
}
static void special_draw(const GfxBand *b, const Perm *p, int layer, int x, int y) {
  draw_normal(b, p, layer, x, y, SPR_ADVANCEDMEDICALCLINIC, false);   /* (its class has no sprite of its own) */
}
const PermOps ops_SpecializedMedicalClinic = {.work = special_work, .medical_quality = special_quality,
                                              .medical_capacity = special_capacity, .draw = special_draw,
                                              .window = clinic_window};

void bld_services_init(void) { TH_LIBRARY_MOVE = then_register(th_library_move); }
