#define RAND_MAX 0x7FFFFFFF

static int rand_seed_set;
static unsigned rand_seed;

void srand(unsigned seed) {
    rand_seed = seed;
    rand_seed_set = 1;
}

int rand(void) {
    if (!rand_seed_set) {
        srand(819234718U);
    }
    rand_seed = rand_seed * 1664525U + 1013904223U;
    return rand_seed & RAND_MAX;
}