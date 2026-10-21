// SPDX-License-Identifier: 0BSD
//
// The benchmark registry: what a benchmark provides and what the harness
// runs. Each benchmark lives in its own file and exports one struct Bench
// through the table in main.c.

#ifndef BENCHWORK_BENCH_H
#define BENCHWORK_BENCH_H

#include <exec/types.h>

#include "compiler.h"

struct Bench {
    const char *name;
    const char *desc;

    // Allocate and prepare the input once per benchmark. Whatever runs in
    // here is not timed. Returns the benchmark's state, which the other
    // hooks receive, or NULL if the benchmark cannot run.
    void *(*setup)(void);

    // One timed iteration. Returns where its output is, for verify(), or
    // NULL on failure. The output must be in memory: the clock reads
    // around run() are compiler barriers for memory, so no store can move
    // out of the timed region, but a result left in registers could.
    const void *(*run)(void *state);

    // Not timed, after every iteration: checks the output of run() and
    // reduces it to a checksum, so that a compiler that miscompiles the
    // code, or a port that changed its behavior, shows up as a different
    // value rather than a faster time. Returns false if the output is
    // wrong.
    bool (*verify)(void *state, const void *result, ULONG *check);

    // Release what setup() allocated, once. May be NULL.
    void (*teardown)(void *state);
};

extern const struct Bench bench_dhry;
extern const struct Bench bench_backdrop;
extern const struct Bench bench_lha_pack;
extern const struct Bench bench_lha_unpack;
extern const struct Bench bench_zlib_deflate;
extern const struct Bench bench_zlib_inflate;
extern const struct Bench bench_png_encode;
extern const struct Bench bench_png_decode;
extern const struct Bench bench_ftgrays;
extern const struct Bench bench_memcpy_small;
extern const struct Bench bench_memcpy_large;
extern const struct Bench bench_memcpy_var_small;
extern const struct Bench bench_memcpy_var_large;
extern const struct Bench bench_memmove_small;
extern const struct Bench bench_memmove_large;
extern const struct Bench bench_wipeout_tris;
#ifdef BENCH_HAVE_NFS3
extern const struct Bench bench_nfs3_project;
extern const struct Bench bench_nfs3_clip;
#endif
#ifdef BENCH_HAVE_AMISSL
extern const struct Bench bench_sha512;
extern const struct Bench bench_aesgcm;
extern const struct Bench bench_chacha20poly1305;
extern const struct Bench bench_x25519;
#endif

// Fowler-Noll-Vo hash of a buffer, folded into a running checksum. Used by
// the benchmarks' verify() for their check value, outside the timing.
ULONG checksum(ULONG seed, const void *data, ULONG len);

void *bench_alloc(ULONG size, const char *what);
void bench_free(void *p);

// Shaped for zlib's z_stream.zalloc/zfree and libpng's malloc_fn/free_fn.
void *bench_zalloc(void *opaque, unsigned items, unsigned size);
void bench_zfree(void *opaque, void *address);

// The backdrop scene as 8-bit RGB, for benchmarks that need a picture.
UBYTE *backdrop_rgb(SHORT *w, SHORT *h);

#endif
