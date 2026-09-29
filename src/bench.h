// SPDX-License-Identifier: 0BSD
//
// The benchmark registry: what a benchmark provides and what the harness
// runs. Each benchmark lives in its own file and exports one struct bench
// through the table in main.c.

#ifndef BENCHWORK_BENCH_H
#define BENCHWORK_BENCH_H

#include <exec/types.h>
#include <stdbool.h>

struct bench {
    const char *name;
    const char *desc;

    // Allocate and prepare the input once per benchmark. Whatever runs in
    // here is not timed. Returns false if the benchmark cannot run.
    bool (*setup)(void);

    // One timed iteration. The checksum summarizes the output so that a
    // compiler that miscompiles the code, or a port that changed its
    // behavior, shows up as a different value rather than a faster time.
    // Returns false on failure.
    bool (*run)(ULONG *check);

    // Release what setup() allocated. May be NULL.
    void (*teardown)(void);
};

extern const struct bench bench_dhry;
extern const struct bench bench_backdrop;
extern const struct bench bench_lha_pack;
extern const struct bench bench_lha_unpack;
extern const struct bench bench_zlib_deflate;
extern const struct bench bench_zlib_inflate;
extern const struct bench bench_png_encode;
extern const struct bench bench_png_decode;
extern const struct bench bench_ftgrays;
extern const struct bench bench_memcpy_small;
extern const struct bench bench_memcpy_large;
extern const struct bench bench_memcpy_var_small;
extern const struct bench bench_memcpy_var_large;
extern const struct bench bench_memmove_small;
extern const struct bench bench_memmove_large;

// Fowler-Noll-Vo hash of a buffer, folded into a running checksum. Used by
// the benchmarks for their check value; cheap enough not to matter next to
// the work it summarizes.
ULONG checksum(ULONG seed, const void *data, ULONG len);

void *bench_alloc(ULONG size, const char *what);
void bench_free(void *p);

// Shaped for zlib's z_stream.zalloc/zfree and libpng's malloc_fn/free_fn.
void *bench_zalloc(void *opaque, unsigned items, unsigned size);
void bench_zfree(void *opaque, void *address);

// The backdrop scene as 8-bit RGB, for benchmarks that need a picture.
UBYTE *backdrop_rgb(SHORT *w, SHORT *h);

#endif
