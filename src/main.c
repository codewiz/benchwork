// SPDX-License-Identifier: 0BSD
//
// Benchwork -- CPU benchmarks for comparing Amiga compilers.

#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <exec/tasks.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "bench.h"
#include "compiler.h"
#include "timer.h"

#ifndef BENCH_CFLAGS
#define BENCH_CFLAGS "?"
#endif

#ifndef BENCH_VERSION
#define BENCH_VERSION "?"
#endif

#ifndef BENCH_DATE
#define BENCH_DATE "?"
#endif

// vbcc has no version macro; its build passes the name from the Makefile.
#ifndef BENCH_COMPILER
    #if defined(__GNUC__)
        #define BENCH_COMPILER "gcc " __VERSION__
    #elif defined(__SASC)
        #define STR_(x) #x
        #define STR(x) STR_(x)
        #define BENCH_COMPILER "SAS/C " STR(__VERSION__) "." STR(__REVISION__)
    #else
        #define BENCH_COMPILER "unknown compiler"
    #endif
#endif

// For the AmigaOS "version" command.
static const char version_tag[] USED =
    "$VER: benchwork " BENCH_VERSION " (" BENCH_DATE ")";

// ftgrays keeps its 16 KB cell pool on the stack and the Amiga shell's
// default stack is 4 KB. Swapping to a bigger one from C is not portable
// (StackSwap() cannot be called from the middle of a C function, since the
// function's locals stay on the other stack), so the program checks the
// stack it was given and asks for a bigger one instead.
#define STACK_NEEDED (32 * 1024)

#ifdef __SASC
// SAS/C's startup code swaps to a stack of this size when the shell's is
// smaller, which is the stack check below done for us. It looks for the
// variable in the near data section, whatever the data model.
__near long __stack = STACK_NEEDED * 2;
#endif

// The ReadArgs() template, and where each argument lands in its array.
static const char TEMPLATE[] =
    "BENCH/M,REPEAT/K/N,MINTIME/K/N,FASTMEM/S,LIST/S,HELP=--help=-h/S";
enum { ARG_BENCH, ARG_REPEAT, ARG_MINTIME, ARG_FASTMEM, ARG_LIST, ARG_HELP,
       NARGS };

// Half an hour: a repetition stops short of twice MINTIME, and its
// microseconds have to fit in a ULONG, which lasts 71 minutes.
#define MAX_MINTIME 1800000L
#define MAX_REPEAT 100000L

// AmigaDOS answers "?" with the template alone, which gives the argument
// names but not what they mean. This is the rest of it.
static void usage(void) {
    printf("benchwork %s\n\n", BENCH_VERSION);
    printf("  BENCH/M        the benchmarks to run, in order (default all)\n"
           "  REPEAT/K/N     timed repetitions of each benchmark (default 3)\n"
           "  MINTIME/K/N    loop over the work until each repetition lasts\n"
           "                 this many milliseconds\n"
           "  FASTMEM/S      require fast memory for the benchmark buffers\n"
           "  LIST/S         list the benchmarks\n"
           "  HELP/S         this text; -h and --help work too\n");
}

// Whether A and B are the same name, ignoring case as AmigaDOS does.
static bool same_name(const char *a, const char *b) {
    for (; tolower((unsigned char)*a) == tolower((unsigned char)*b); a++, b++)
        if (!*a)
            return true;
    return false;
}

// The benchmark called NAME, or NULL.
static const struct Bench *find_bench(const char *name) {
    size_t j;

    for (j = 0; j < NBENCHES; j++)
        if (same_name(name, BENCHES[j]->name))
            return BENCHES[j];
    return NULL;
}

// Set OPTS from what ReadArgs() returned. The benchmarks are the ones named,
// in that order, repeats included; all of them, in table order, when none
// is. Returns -1 to go on, or the exit code.
static int apply_args(const LONG *args, struct BenchOptions *opts) {
    char **names = (char **)args[ARG_BENCH];
    size_t j, n = 0;

    if (args[ARG_HELP]) {
        usage();
        return RETURN_WARN;
    }
    if (args[ARG_LIST]) {
        for (j = 0; j < NBENCHES; j++)
            printf("%-16s %s\n", BENCHES[j]->name, BENCHES[j]->desc);
        return RETURN_OK;
    }
    if (args[ARG_REPEAT]) {
        LONG repeat = *(LONG *)args[ARG_REPEAT];

        if (repeat < 1 || repeat > MAX_REPEAT) {
            printf("REPEAT must be from 1 to %ld\n", MAX_REPEAT);
            return RETURN_FAIL;
        }
        opts->repeat = (int)repeat;
    }
    if (args[ARG_MINTIME]) {
        LONG ms = *(LONG *)args[ARG_MINTIME];

        if (ms < 0 || ms > MAX_MINTIME) {
            printf("MINTIME must be from 0 to %ld\n", MAX_MINTIME);
            return RETURN_FAIL;
        }
        opts->min_time_us = (ULONG)ms * 1000;
    }
    opts->fastmem = args[ARG_FASTMEM] != 0;

    if (names)
        while (names[n])
            n++;
    opts->nbenches = n ? n : NBENCHES;
    opts->benches = malloc(opts->nbenches * sizeof *opts->benches);
    if (!opts->benches) {
        printf("out of memory\n");
        return RETURN_FAIL;
    }
    for (j = 0; j < opts->nbenches; j++) {
        opts->benches[j] = n ? find_bench(names[j]) : BENCHES[j];
        if (!opts->benches[j]) {
            printf("unknown benchmark %s: LIST names them\n", names[j]);
            return RETURN_FAIL;
        }
    }
    return -1;
}

// Set OPTS from the command line. Returns -1 to go on, or the exit code.
static int parse_args(struct BenchOptions *opts) {
    LONG args[NARGS] = { 0 };
    struct RDArgs *rda;
    int status;

    rda = ReadArgs((STRPTR)TEMPLATE, args, NULL);
    if (!rda) {
        PrintFault(IoErr(), (STRPTR)"benchwork");
        usage();
        return RETURN_FAIL;
    }
    // The names are only looked up, so the arguments can go right away.
    status = apply_args(args, opts);
    FreeArgs(rda);
    return status;
}

// Print the banner and run the benchmarks. Returns the exit code.
static int run(const struct BenchOptions *opts) {
    int failures;

    if (!timer_open()) {
        printf("cannot open timer.device\n");
        return RETURN_FAIL;
    }

    printf("benchwork %s built with %s\n", BENCH_VERSION, BENCH_COMPILER);
    printf("CFLAGS: %s\n\n", BENCH_CFLAGS);

    failures = bench_run_all(opts);

    timer_close();
    if (failures < 0)
        return RETURN_WARN;
    return failures ? RETURN_ERROR : RETURN_OK;
}

int main(void) {
    struct Task *task = FindTask(NULL);
    ULONG stack = (ULONG)task->tc_SPUpper - (ULONG)task->tc_SPLower;
    struct BenchOptions opts = {
        /* .benches= */ NULL,
        /* .nbenches= */ 0,
        /* .repeat= */ 3,
        /* .min_time_us= */ 0,
        /* .fastmem= */ false,
    };
    int status;

    if (stack < STACK_NEEDED) {
        printf("benchwork needs a %lu byte stack, got %lu: run \"stack %lu\" first\n",
               (unsigned long)STACK_NEEDED, (unsigned long)stack,
               (unsigned long)STACK_NEEDED * 2);
        return RETURN_FAIL;
    }

    status = parse_args(&opts);
    if (status < 0)
        status = run(&opts);
    free((void *)opts.benches);  // SAS/C sees a const dropped without the cast

    return status;
}
