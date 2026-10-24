// SPDX-License-Identifier: 0BSD
//
// What a benchmark provides and what the harness gives it. Each benchmark
// lives in its own file, listed in benchlist.c.

#ifndef BENCHWORK_BENCH_H
#define BENCHWORK_BENCH_H

#include <exec/types.h>
#include <stddef.h>

#include "compiler.h"

struct Bench {
    const char *name;
    const char *desc;

    // The unit of the reported rate, short ("KB" of 1024 bytes, "px",
    // "Dhry"), and how many of them one run() does.
    const char *unit;
    double per_loop;

    // Allocate and prepare the input once per benchmark. Whatever runs in
    // here is not timed. Returns the benchmark's state, which the other
    // hooks receive, or NULL if the benchmark cannot run.
    void *(*setup)(void);

    // One loop over the work, timed. Returns where its output is, for
    // verify(), or NULL on failure. The output must be in memory: the clock
    // reads around run() are compiler barriers for memory, so no store can
    // move out of the timed region, but a result left in registers could.
    const void *(*run)(void *state);

    // Not timed, after every repetition: checks the output of run() and
    // reduces it to a checksum, so that a compiler that miscompiles the
    // code, or a port that changed its behavior, shows up as a different
    // value rather than a faster time. Returns false if the output is
    // wrong.
    bool (*verify)(void *state, const void *result, ULONG *check);

    // Release what setup() allocated, once. May be NULL.
    void (*teardown)(void *state);
};

// Every benchmark, in benchlist.c.
extern const struct Bench *const BENCHES[];
extern const size_t NBENCHES;

// What bench_run_all() runs, and how.
struct BenchOptions {
    // The benchmarks, in order; one can appear several times.
    const struct Bench **benches;
    size_t nbenches;
    // Timed repetitions of each benchmark.
    int repeat;
    // Each repetition loops over the work until it lasts this long, in
    // microseconds; 0 runs it once.
    ULONG min_time_us;
    // bench_alloc() asks for fast memory.
    bool fastmem;
};

// Run the benchmarks, each in its own report line. Returns the failure
// count, or -1 on Ctrl-C.
int bench_run_all(const struct BenchOptions *opts);

// Fowler-Noll-Vo hash of a buffer, folded into a running checksum. Used by
// the benchmarks' verify() for their check value, outside the timing.
ULONG checksum(ULONG seed, const void *data, ULONG len);

void *bench_alloc(ULONG size, const char *what);
void bench_free(void *p);

// Shaped for zlib's z_stream.zalloc/zfree and libpng's malloc_fn/free_fn.
void *bench_zalloc(void *opaque, unsigned items, unsigned size);
void bench_zfree(void *opaque, void *address);

// The scene-2d landscape at SCENE_WIDTH x SCENE_HEIGHT as 8-bit RGB, for the
// benchmarks that need a picture.
#define SCENE_WIDTH 320
#define SCENE_HEIGHT 200
UBYTE *scene_rgb(SHORT *w, SHORT *h);

#endif
