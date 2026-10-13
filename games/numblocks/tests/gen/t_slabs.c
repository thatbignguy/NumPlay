/* Consistency: gen_slab(0..128) against the same chunk built from four 32-high slabs
 * (the engine asks for 32-high slabs). Prints the differing blocks per kind of difference.
 * A slab's rows are 9 bits a state (288 bytes a row), so the buffers are sized for that. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/blocks.h"
#include "../../src/states.h"
#include "../../src/gen.h"

#define ROWS 128
#define RBYTES (ROWS * 288)

int main(int argc, char **argv) {
    uint8_t gs[GEN_SCRATCH];
    gen_scratch(gs);
    int64_t seed = argc > 1 ? strtoll(argv[1], 0, 10) : 12345;
    int n = argc > 2 ? atoi(argv[2]) : 10;
    static uint8_t full[RBYTES], part[RBYTES];
    gen_init(seed);
    long diff = 0, chunks = 0;
    for (int cz = -n / 2; cz < n - n / 2; cz++)
        for (int cx = -n / 2; cx < n - n / 2; cx++) {
            gen_slab(cx, cz, 0, ROWS, full);
            for (int s = 0; s < 4; s++) gen_slab(cx, cz, s * 32, 32, part + s * 32 * 288);
            int d = 0;
            for (int c = 0; c < ROWS * 256; c++) {
                int a = b9_get(full, c), b = b9_get(part, c);
                if (a != b) {
                    if (diff + d < 12)
                        printf("chunk %d %d: x %d y %d z %d full %d slabs %d\n", cx, cz, c & 15, c >> 8, (c >> 4) & 15,
                               a, b);
                    d++;
                }
            }
            diff += d;
            chunks += d > 0;
        }
    printf("seed %lld: %ld blocks differ in %ld of %d chunks\n", (long long)seed, diff, chunks, n * n);
    return 0;
}
