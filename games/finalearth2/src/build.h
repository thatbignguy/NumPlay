/* The Final Earth 2: building and destroying (build.c). */
#ifndef FE_BUILD_H
#define FE_BUILD_H
#include "fe.h"

/* the original's BuildingMode: Drag and DragReplace build on every cell the cursor passes while OK is held */
typedef enum { BM_NORMAL, BM_INSERT, BM_REPLACE, BM_DRAG, BM_DRAG_REPLACE, BM_DESTROY, BM_DESTROY_LEAVING_HOLE } BuildingMode;
#define BM_COUNT 7
typedef enum { BP_OK, BP_WORLD_NOT_ALLOWED, BP_IMPOSSIBLE, BP_IN_WORLD } BuildPossibility;
typedef enum { DR_OK, DR_PROTECTED, DR_INVALID_STRUCTURE, DR_INVALID_STRUCTURE_HOLE, DR_REFUSED } DestroyResult;
enum { UNLOCK_LOCKED, UNLOCK_UNLOCKED, UNLOCK_RESEARCHED };

extern BuildingMode building_mode;
extern float recycling_amount;
/* the builder's look (decorationAppearance, mirrored, decorationAppearanceColor): DragReplace leaves alone a
 * building of the same class that already looks so */
typedef struct { int decor; bool mirrored; int32_t color; } BuilderLook;
extern BuilderLook builder_look;
static inline bool building_mode_can_replace(void) { return building_mode == BM_REPLACE || building_mode == BM_DRAG_REPLACE; }
static inline bool building_mode_can_drag(void) { return building_mode == BM_DRAG || building_mode == BM_DRAG_REPLACE; }
bool bld_same_customizations(const Perm *p, int decor, bool mirrored, int32_t color);   /* areSameCustomizations */

uint16_t permanent_at_pos(int x, int y);
BuildPossibility builder_can_build(int w, int x, int y, int cls);
void builder_cost(int cls, Materials *out);
bool can_afford(const Materials *m);
bool builder_prerequisites(int cls);
uint16_t builder_build(int w, int x, int y, int cls);
DestroyResult building_try_destroy(uint16_t id);
void give_recycle_reward(const Perm *p, bool full);

/* from other parts */
bool world_mask_at(int w, int cx, int cy);          /* the island's collision mask (bg.c) */
int unlock_state(int cls);
void unlocks_research(int cls);
void unlocks_check_build_related(void);
bool unique_not_unique(int cls);                     /* notUniqueIfGalacticLibrary */
void building_cost_scale(int cls, Materials *cost);  /* progress.BuildingCost */
void building_upgrades_cost_sum(const Perm *p, Materials *m);
void bld_on_build(Perm *p);
void bld_post_create(Perm *p);
void perm_move(uint16_t id, int row);
void perm_destroy_keep_hole(uint16_t id, bool keep_hole);
#endif
