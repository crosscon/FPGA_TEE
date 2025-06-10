#include <wolfssl/wolfcrypt/types.h>

unsigned char my_rng_seed_gen(void) {
    static unsigned int seed = 12345;
    seed = (seed * 1103515245 + 12345) & 0xFFFFFFFF; // Simple LCG
    return (unsigned char)(seed & 0xFF);
}
