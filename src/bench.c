// SPDX-License-Identifier: 0BSD
//
// The benchmark runner: sets up, times and checks each benchmark, prints
// the report, and provides the helpers bench.h declares for the
// benchmarks' own use.

#include <proto/exec.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bench.h"
#include "timer.h"

// What bench_alloc() asks for, set by bench_run_all(): the benchmarks call it
// from setup(), which takes no options.
static ULONG alloc_flags = MEMF_ANY;

// So that a clock that never moves cannot keep the doubling going forever.
#define MAX_LOOPS 65536UL

// 32-bit FNV-1a, seeded so that several buffers can be chained.
ULONG checksum(ULONG seed, const void *data, ULONG len) {
    const UBYTE *p = (const UBYTE *)data;
    ULONG h = seed ? seed : 2166136261UL;

    while (len--)
        h = (h ^ *p++) * 16777619UL;
    return h;
}

// Allocate a benchmark buffer, released with bench_free(). WHAT names it if
// the allocation fails. Not cleared, like malloc().
//
// Where the memory lands changes the timings: MEMF_ANY falls back to chip
// memory when fast memory is full or fragmented, and the CPU reaches chip
// memory only through bus arbitration against DMA, so a large buffer landing
// there makes a workload look slower for reasons that have nothing to do with
// the code under test. FASTMEM asks for fast memory and fails when there is
// none, rather than producing a number that cannot be compared.
void *bench_alloc(ULONG size, const char *what) {
    void *p = AllocVec(size, alloc_flags);

    if (!p)
        printf("%s: cannot allocate %lu bytes%s\n", what, (unsigned long)size,
               alloc_flags & MEMF_FAST ? " of fast memory" : "");
    return p;
}

// Release a bench_alloc() buffer. Takes NULL, as FreeVec() does.
void bench_free(void *p) {
    FreeVec(p);
}

// bench_alloc() for zlib's z_stream.zalloc and libpng's malloc_fn. Neither
// library expects cleared memory: zlib's own zcalloc() is malloc() on any
// target with 32-bit ints, despite the name.
void *bench_zalloc(void *opaque, unsigned items, unsigned size) {
    (void)opaque;
    return bench_alloc((ULONG)items * size, "library buffer");
}

// The matching zfree/free_fn.
void bench_zfree(void *opaque, void *address) {
    (void)opaque;
    bench_free(address);
}

// Milliseconds with three decimals, right-aligned in 11 columns, so that
// times up to 999999 ms keep a space before them.
static void print_ms(ULONG us) {
    printf("%7lu.%03lu", (unsigned long)(us / 1000), (unsigned long)(us % 1000));
}

// PER_LOOP units in US microseconds, per second, in 13 columns: a JIT on a
// fast host gets past ten million px/s. Four decimals below 100, so that a
// 68000's 0.03 kex/s still reads; printed through integers, as not every C
// library here formats doubles.
static void print_rate(double per_loop, ULONG us) {
    double rate = us ? per_loop * 1000000.0 / us : 0;
    ULONG scale = rate < 100 ? 10000 : 100, whole, frac;

    if (rate > 999999999.0)
        rate = 999999999.0;
    whole = (ULONG)rate;
    frac = (ULONG)((rate - whole) * scale + 0.5);
    if (frac >= scale) {
        whole++;
        frac -= scale;
    }
    if (scale == 100)
        printf(" %9lu.%02lu", (unsigned long)whole, (unsigned long)frac);
    else
        printf(" %7lu.%04lu", (unsigned long)whole, (unsigned long)frac);
}

// Set up, time and tear down one benchmark. Returns false on failure, after
// saying which step failed; on success *BEST_US is the time per loop in the
// fastest repetition.
//
// A repetition times a number of loops, calls to run(): one, doubled while a
// repetition is shorter than MINTIME, which then do not count. That is for
// emulators whose E clock only moves between frames, where one loop can
// finish between two ticks.
static bool run_bench(const struct Bench *b, const struct BenchOptions *opts,
                      ULONG *best_us) {
    ULONG best = ~0UL, total = 0, check = 0, first = 0, loops = 1, n;
    bool ok = true, checked = false;
    void *state;
    int i = 0;

    state = b->setup();
    if (!state) {
        printf("%-16s setup failed\n", b->name);
        return false;
    }
    while (i < opts->repeat) {
        struct EClockVal t0, t1;
        const void *result;
        ULONG us;

        // No task switches during the timed region. Interrupts still run,
        // which is what keeps the E clock and the emulator's pacing honest;
        // nothing inside a benchmark waits on the OS. Only run() is timed:
        // checking its output is verify()'s job, afterwards.
        Forbid();
        timer_now(&t0);
        result = b->run(state);
        for (n = 1; result && n < loops; n++)
            result = b->run(state);
        timer_now(&t1);
        Permit();
        if (!result) {
            printf("%-16s run %d failed\n", b->name, i + 1);
            ok = false;
            break;
        }
        if (!b->verify(state, result, &check)) {
            printf("%-16s run %d produced wrong output\n", b->name, i + 1);
            ok = false;
            break;
        }
        if (!checked) {
            first = check;
            checked = true;
        } else if (check != first) {
            // The same code on the same input produced different bytes: a
            // benchmark with state leaking between repetitions, or a
            // miscompile that depends on stale memory. Either way the times
            // mean nothing.
            printf("%-16s checksum changed between repetitions\n", b->name);
            ok = false;
            break;
        }
        us = eclock_micros(&t0, &t1);
        if (us < opts->min_time_us && loops < MAX_LOOPS) {
            loops *= 2;
            continue;
        }
        us /= loops;
        if (us < best)
            best = us;
        total += us;
        i++;
    }
    if (ok) {
        printf("%-16s %5lu", b->name, (unsigned long)loops);
        print_ms(best);
        print_ms(total / (ULONG)opts->repeat);
        print_rate(b->per_loop, best);
        // Padded to the longest unit, "glyph/s".
        printf(" %s/s%*s  %08lx\n", b->unit, 5 - (int)strlen(b->unit), "",
               (unsigned long)check);
        *best_us = best;
    }
    if (b->teardown)
        b->teardown(state);
    return ok;
}

// The report ends with the geometric mean of the benchmarks' fastest times
// when more than one ran: the one number that weighs a 10% change on a
// short workload the same as on a long one.
int bench_run_all(const struct BenchOptions *opts) {
    int failures = 0, ran = 0;
    double logsum = 0;
    size_t j;

    alloc_flags = opts->fastmem ? MEMF_FAST : MEMF_ANY;
    printf("%-16s %5s%11s%11s%13s %-7s  %s\n", "BENCHMARK", "LOOPS", "MIN MS",
           "AVG MS", "RATE", "UNITS", "CHECK");
    for (j = 0; j < opts->nbenches; j++) {
        ULONG best;

        // Only some C libraries check for Ctrl-C in their stdio (libnix
        // does, newlib does not), so look for it between benchmarks.
        if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
            printf("***Break\n");
            return -1;
        }
        if (!run_bench(opts->benches[j], opts, &best)) {
            failures++;
            continue;
        }
        logsum += log((double)best);
        ran++;
    }
    if (ran > 1) {
        printf("\n%-16s %5s", "GEOMEAN", "");
        print_ms((ULONG)(exp(logsum / ran) + 0.5));
        printf("\n");
    }
    return failures;
}
