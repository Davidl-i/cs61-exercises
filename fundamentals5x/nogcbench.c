#include "m61gc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <getopt.h>

static unsigned n = 1000000;
static unsigned k = 2048;

static void benchmark(void) {
    char** chunks = (char**) m61_malloc(sizeof(char*) * k);
    for (unsigned s = 0; s != k; ++s)
        chunks[s] = NULL;

    // `n` chunk allocations of 1-4K each
    for (unsigned i = 0; i != n; ++i) {
        if (i % 100000 == 0 && i != 0)
            printf("Chunk allocation %u/%u\n", i, n);
        unsigned s = random() % k;
        // modify chunk, so valgrind will complain if the memory is free
        if (chunks[s]) {
            assert(chunks[s][0] == 62);
            chunks[s][0] = 61;
        }
        m61_free(chunks[s]);
        size_t sz = 1024 + random() % 3072;
        chunks[s] = m61_malloc(sz);
        chunks[s][0] = 62;
    }

    // modify chunks, so valgrind will complain if the memory is free
    for (unsigned s = 0; s != k; ++s)
        if (chunks[s]) {
            assert(chunks[s][0] == 62);
            chunks[s][0] = 61;
        }

    // clean up
    for (unsigned s = 0; s != k; ++s)
        m61_free(chunks[s]);
    m61_free(chunks);
}

int main(int argc, char* argv[]) {
    // Find an approximation for the bottom of the stack: an address
    // greater than or equal to the address of any local.  We use the
    // address of the `argv` array, which the OS initializes on the
    // stack before `main` is called (i.e., below `main`'s stack
    // frame).
    m61_stack_bottom = (char*) &argv[0];

    int opt, limit = 1;
    while ((opt = getopt(argc, argv, "n:k:l")) != -1)
        switch (opt) {
        case 'n':
            n = strtoul(optarg, NULL, 0);
            break;
        case 'k':
            k = strtoul(optarg, NULL, 0);
            break;
        case 'l':
            limit = 0;
            break;
        default:
            fprintf(stderr, "Usage: ./nogcbench [-n NOPS] [-k NALLOCS] [-l]\n");
            exit(EXIT_FAILURE);
        }

    if (limit) {
        // Limit memory to at most 256MB (does not work on OS X).
        struct rlimit memlimit = { 256L << 20, 256L << 20 };
        int r = setrlimit(RLIMIT_AS, &memlimit);
        assert(r >= 0);
    }

    benchmark();

    // this should print "0 allocations"
    m61_print_allocations();
}
