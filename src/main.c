// SPDX-License-Identifier: 0BSD
//
// BenchWork -- CPU benchmarks for comparing m68k compilers.
//
// Every benchmark is a real workload lifted from a program people run on
// the Amiga (see README.md), reduced to a function that reads and writes
// memory buffers, so what is measured is the code the compiler generated and
// nothing else: no disk, no display, no OS calls inside the timed region.
//
// Each benchmark is set up once, then run the requested number of times.
// The report gives the fastest and the mean iteration, and a checksum of the
// output that must agree across compilers.

#include <proto/exec.h>
#include <exec/tasks.h>
#include <devices/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "timer.h"

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "?"
#endif

// ftgrays keeps its 16 KiB cell pool on the stack and the Amiga shell's
// default stack is 4 KiB. Swapping to a bigger one from C is not portable
// (StackSwap() cannot be called from the middle of a C function, since the
// function's locals stay on the other stack), so the program checks the
// stack it was given and asks for a bigger one instead.
#define STACK_NEEDED (32 * 1024)

static const struct bench *const BENCHES[] = {
    &bench_dhry,
    &bench_backdrop,
    &bench_lha_pack,
    &bench_lha_unpack,
    &bench_zlib_deflate,
    &bench_zlib_inflate,
    &bench_png_encode,
    &bench_png_decode,
    &bench_ftgrays,
    &bench_memcpy_small,
    &bench_memcpy_large,
    &bench_memcpy_var_small,
    &bench_memcpy_var_large,
    &bench_memmove_small,
    &bench_memmove_large,
};

#define NBENCHES (sizeof(BENCHES) / sizeof(BENCHES[0]))

// 32-bit FNV-1a, seeded so that several buffers can be chained.
ULONG checksum(ULONG seed, const void *data, ULONG len) {
    const UBYTE *p = (const UBYTE *)data;
    ULONG h = seed ? seed : 2166136261UL;

    while (len--)
        h = (h ^ *p++) * 16777619UL;
    return h;
}

static void usage(void) {
    printf("usage: benchwork [-n iterations] [--fastmem] [-l] [name ...]\n");
    printf("  --fastmem  allocate from fast memory only, and fail if there is\n");
    printf("             none, so a chip-memory fallback cannot skew timings\n");
}

// Milliseconds with three decimals, right-aligned in 10 columns.
static void print_ms(ULONG us) {
    printf("%6lu.%03lu", (unsigned long)(us / 1000), (unsigned long)(us % 1000));
}

// Set up, time and tear down one benchmark. Returns false on failure, after
// saying which step failed.
static bool run_bench(const struct bench *b, int iters) {
    ULONG best = ~0UL, total = 0, check = 0, first = 0;
    bool ok = true;

    if (!b->setup()) {
        printf("%-12s setup failed\n", b->name);
        return false;
    }
    for (int i = 0; i < iters; i++) {
        struct EClockVal t0, t1;
        ULONG us;

        // No task switches during the timed region. Interrupts still run,
        // which is what keeps the E clock and the emulator's pacing honest;
        // nothing inside a benchmark waits on the OS.
        Forbid();
        timer_now(&t0);
        if (!b->run(&check)) {
            Permit();
            printf("%-12s run %d failed\n", b->name, i + 1);
            ok = false;
            break;
        }
        timer_now(&t1);
        Permit();
        us = eclock_micros(&t0, &t1);
        if (i == 0)
            first = check;
        else if (check != first) {
            // The same code on the same input produced different bytes: a
            // benchmark with state leaking between iterations, or a
            // miscompile that depends on stale memory. Either way the times
            // mean nothing.
            printf("%-12s checksum changed between iterations\n", b->name);
            ok = false;
            break;
        }
        if (us < best)
            best = us;
        total += us;
    }
    if (ok) {
        printf("%-12s %5d ", b->name, iters);
        print_ms(best);
        print_ms(total / (ULONG)iters);
        printf("   %08lx\n", (unsigned long)check);
    }
    if (b->teardown)
        b->teardown();
    return ok;
}

static int iters = 3;
static bool selected[NBENCHES], any;
static bool fastmem;

void *bench_alloc(ULONG size, const char *what) {
    void *p = AllocVec(size, fastmem ? MEMF_FAST : MEMF_ANY);

    if (!p)
        printf("%s: cannot allocate %lu bytes%s\n", what, (unsigned long)size,
               fastmem ? " of fast memory" : "");
    return p;
}

// Run the selected benchmarks. Returns the failure count.
static int run_all(void) {
    int failures = 0;

    for (size_t j = 0; j < NBENCHES; j++) {
        if (any && !selected[j])
            continue;
        if (!run_bench(BENCHES[j], iters))
            failures++;
    }
    return failures;
}

int main(int argc, char **argv) {
    struct Task *task = FindTask(NULL);
    ULONG stack = (ULONG)task->tc_SPUpper - (ULONG)task->tc_SPLower;
    int failures, i;

    if (stack < STACK_NEEDED) {
        printf("benchwork needs a %lu byte stack, got %lu: run \"stack %lu\" first\n",
               (unsigned long)STACK_NEEDED, (unsigned long)stack,
               (unsigned long)STACK_NEEDED * 2);
        return 20;
    }

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            iters = atoi(argv[++i]);
            if (iters < 1)
                iters = 1;
        } else if (!strcmp(argv[i], "--fastmem")) {
            fastmem = true;
        } else if (!strcmp(argv[i], "-l")) {
            for (size_t j = 0; j < NBENCHES; j++)
                printf("%-12s %s\n", BENCHES[j]->name, BENCHES[j]->desc);
            return 0;
        } else if (argv[i][0] == '-') {
            usage();
            return 20;
        } else {
            size_t j;

            for (j = 0; j < NBENCHES; j++)
                if (!strcmp(argv[i], BENCHES[j]->name))
                    break;
            if (j == NBENCHES) {
                printf("unknown benchmark %s\n", argv[i]);
                return 20;
            }
            selected[j] = true;
            any = true;
        }
    }

    if (!timer_open()) {
        printf("cannot open timer.device\n");
        return 20;
    }

    printf("benchwork: %s\n", __VERSION__);
    printf("CFLAGS: %s\n\n", BENCH_CFLAGS);
    printf("%-12s %5s %10s %10s   %s\n", "benchmark", "iters", "min ms",
           "mean ms", "check");

    failures = run_all();

    timer_close();
    return failures ? 10 : 0;
}
