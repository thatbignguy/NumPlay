/* The Final Earth 2: what the parts of the simulation call in each other. */
#ifndef FE_SIM_H
#define FE_SIM_H
#include "fe.h"
#include "pcls.h"

#define PF_USED 128   /* a permanent's slot is in use */

/* city.c */
extern uint8_t col_deco[MAX_COLS];
extern uint8_t landing_group_of[MAX_PERMS];
extern int nlanding_groups;
extern int direct_world_connections;
void rnd_seed(uint32_t s);
int world_add(int x, int y, int w, int h, uint32_t seed, uint8_t appearance);
uint16_t perm_new(int cls, int w, int col, int row);
void perm_destroy(uint16_t id, bool for_replacement);
int perm_count(void);
int col_stack_height(int w, int col);
void city_update_connections(void);
void city_reset(void);

/* sim.c: CitySimulation */
enum { SIM_JOBS_CHANGED = 1, SIM_HOUSES_CHANGED = 2, SIM_SCHOOLS_CHANGED = 4 };
extern uint8_t sim_flags;
void sim_update_pathfinder(bool invalidate, uint16_t related_to);
void sim_tick(float timeMod);
uint16_t cit_create(int world, float age, uint16_t start_in, int start_x);   /* createCitizen; start_x < 0: random */

/* systems.c: the city-wide systems */
void stats_update(float timeMod);
void stats_midnight_update(void);
void flying_update(float timeMod);
void spawners_update(float timeMod);
void animals_update(float timeMod);
void baby_update(float timeMod);
void building_upgrades_update(float timeMod);
void boosts_update(float timeMod);
void operating_cost_update(float timeMod);
void worlds_update(float timeMod);
void progress_update(float timeMod);
void happiness_update(float timeMod);
void eating_update(float timeMod);

/* citizen.c */
void cit_update_path(Cit *c, float timeMod);
void cit_update_daily_life(Cit *c, float timeMod);
void cit_infrequent_update(Cit *c);
void cit_mid_day_update(Cit *c);
void cit_reset_path(Cit *c, bool reset_can_view_self);
void cit_try_remove(Cit *c, bool for_send_away);
void cit_lose_job(Cit *c, bool instant_replace);
void cit_evict_from_home(Cit *c);
void cit_leave_school(Cit *c);
void cit_add_to_workers(Cit *c);
float cit_age(const Cit *c);
float cit_education_speed_modifier(const Cit *c);
float cit_steps_in_building(const Cit *c);
float cit_work_time_preference(const Cit *c);
bool cit_should_work_now(const Cit *c);
bool cit_should_school_now(const Cit *c);
void cit_go_to_permanent(Cit *c, uint16_t perm);
void cit_instant_leave_permanent(Cit *c);
bool cit_leave_permanent(Cit *c, float timeMod);   /* leavePermanent: true once outside */
bool cit_move_towards_x(Cit *c, float x, float timeMod);
void cit_city_position(const Cit *c, float *x, float *y);
void cits_leave_destroyed(uint16_t perm);
void cits_lose_job_at(uint16_t perm);
void cits_evict_from(uint16_t perm);
void cits_leave_school(uint16_t perm);

/* scripted steps (paths of their own): what to do at their end */
enum { THEN_NONE, THEN_COUNT_ = 160 };
/* a function for the end of a scripted path (the original's closures), registered once at start: its number */
uint8_t then_register(void (*fn)(Cit *c, uint16_t arg));
typedef void (*ThenFn)(Cit *c, uint16_t arg);
extern ThenFn then_fns[THEN_COUNT_];
#define CIT_SCRIPTED_MAX 128   /* steps of a scripted path at most */
void cit_set_path(Cit *c, const int16_t *steps, int n);           /* setPath with a scripted path (copied) */
void cit_set_path_then(Cit *c, const int16_t *steps, int n, uint8_t then, uint16_t arg);
void cit_wait(Cit *c, int time, uint8_t then, uint16_t arg);
void cit_wait_random(Cit *c, int tmin, int tmax, uint8_t then, uint16_t arg);
void cit_move(Cit *c, int x, uint8_t then, uint16_t arg, bool slow);
void cit_move_random(Cit *c, int xmin, int xmax, uint8_t then, uint16_t arg, bool slow);
void cit_move_and_wait(Cit *c, int x, int time, uint8_t then, uint16_t arg, bool modify_with_happiness, bool slow);
void cit_move_and_wait_random(Cit *c, int xmin, int xmax, int tmin, int tmax, uint8_t then, uint16_t arg,
                              bool modify_with_happiness, bool slow);
void cit_wait_and_move_and_wait_random(Cit *c, int t1min, int t1max, int xmin, int xmax, int tmin, int tmax,
                                       uint8_t then, uint16_t arg, bool modify_with_happiness, bool slow);
void cit_move_towards_random_in_world_coords(Cit *c, int xmin, int xmax, uint8_t then, uint16_t arg);
void cit_change_floor(Cit *c, uint8_t then, uint16_t arg);
void cit_change_floor_and_wait(Cit *c, int wait, uint8_t then, uint16_t arg);
void cit_change_floor_and_wait_random(Cit *c, int tmin, int tmax, uint8_t then, uint16_t arg);
void cit_change_floor_and_move_random(Cit *c, int xmin, int xmax, uint8_t then, uint16_t arg);
bool cit_is_at_ground_level(const Cit *c);
void cit_wander(Cit *c, float timeMod);
void cit_go_down_towards_world_surface(Cit *c, float timeMod);

/* path.c: the pathfinder (pathfinder.js and pathfinder.PathfinderManager) */
enum { GOAL_PERM, GOAL_ENTERTAINMENT };
void path_find(Cit *c, uint16_t goal, uint8_t on_fail);                  /* findPath */
void path_find_combined(Cit *c, uint16_t goal, uint8_t on_fail);         /* findPathCombined */
void path_find_entertainment(Cit *c, int max_distance, int type_bit);    /* findPathEntertainment */
bool path_may_request_now(uint16_t in_perm);
void path_invalidate_all(void);
void path_update(float timeMod);          /* answers the requests of the tick */
void path_distribute(void);
uint16_t flying_find_any_other_landing_site(uint16_t ls);
/* the search memory (path.c), also used by the PermanentFinder */
extern int heap_n;
void heap_up(int i);
void heap_push(uint16_t id);
uint16_t heap_pop(void);
uint16_t heap_index(uint16_t id);
void cit_set_found_path(Cit *c, const int16_t *pairs, int n);
uint16_t *cit_steps(const Cit *c);
uint16_t cit_new(int world, float x, float age, uint16_t start_in);
/* the path pool: each citizen's path is a list of (step, value) pairs */
void game_reset(void);   /* an empty city, every system back to its start (before a new city or a load) */
#endif
