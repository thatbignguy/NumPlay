/* The Final Earth 2: saves (save.c). */
#ifndef FE_SAVE_H
#define FE_SAVE_H
#include "fe.h"

typedef struct SaveIO SaveIO;
/* writing (in the order they will be read) */
void save_bytes(SaveIO *io, const void *data, uint32_t n);
void save_u8(SaveIO *io, uint8_t v);
void save_u16(SaveIO *io, uint16_t v);
void save_u32(SaveIO *io, uint32_t v);
void save_f32(SaveIO *io, float v);
void save_f64(SaveIO *io, double v);
/* reading */
bool load_bytes(SaveIO *io, void *data, uint32_t n);
uint8_t load_u8(SaveIO *io);
uint16_t load_u16(SaveIO *io);
uint32_t load_u32(SaveIO *io);
float load_f32(SaveIO *io);
double load_f64(SaveIO *io);

bool save_game(void);
bool save_exists(void);
bool load_game(void);
void save_delete(void);
/* the copy of the saves in a Python script (savecopy.c): written after each save, read back at start when the
 * files are gone */
void save_copy(void);
void save_copy_drop(void);
void save_restore(void);

/* the buildings' own state outside their permanents (bld*.c, upgrades.c): saved after everything */
void bld_save(SaveIO *io);
bool bld_load(SaveIO *io);
/* the view kept with the city (City's viewPos and zoomScale, and the keypad's cursor): the user interface sets
 * the cursor before a save; a load sets it all and valid, until the city's view is set from it */
typedef struct { float x, y; int16_t cx, cy; uint8_t zoom; bool valid; } SavedView;
extern SavedView saved_view;
/* systems.c: the happiness boosts running */
void boosts_save(SaveIO *io);
bool boosts_load(SaveIO *io);
/* city.c, citizen.c */
bool perm_restore(uint16_t id, const Perm *p);
void cits_recount(void);
#endif
