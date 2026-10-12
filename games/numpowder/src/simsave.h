/* NumPowder: the simulation as a compact byte stream (the original's Simulation::Save and Load with GameSave's
 * OPS format, for the slots and Home's autosave): the particles by place, the walls, fans and air, Newtonian
 * gravity, the stickmen's boots, the settings, the frame count and the random numbers. As in OPS, particles come
 * back in the middle of their cells with new ids and velocities to 1/16; temperatures are kept to their 1/4 K.
 * Saving changes nothing, and a save of a loaded save is the same bytes. */
#ifndef NP_SIMSAVE_H
#define NP_SIMSAVE_H
#include <stdbool.h>
#include <stdint.h>

uint32_t simsave_size(void);                             /* the bytes simsave_write writes now */
uint32_t simsave_write(uint8_t *out, uint32_t room);     /* 0 if room is too small */
bool simsave_read(const uint8_t *data, uint32_t len);    /* false (and the simulation unchanged) if not a save */
int simsave_count(const uint8_t *data, uint32_t len);    /* a save's particles, -1 if not a save */
#endif
