// SPDX-License-Identifier: 0BSD
//
// Block moves: the copies a compiler either expands inline or turns into a
// library call, and the ones it must not get wrong.
//
// None of the other workloads measures this directly, yet it is one of the
// places where the m68k backend makes the largest differences. A copy whose
// size is a compile-time constant can be expanded by pieces, as a run of
// moves; one whose size is only known at run time becomes a call, which on
// AmigaOS with libnix reaches exec CopyMem. Struct assignment takes the same
// path as a constant-size memcpy, and an overlapping move has to be done
// backwards, which is where a block-move expander is most likely to be wrong
// rather than merely slow.
//
// Each case is split at 128 bytes, the size up to which the AmigaOS by-pieces
// hook expands a copy inline: below it the two policies mostly agree, above it
// only an expander that takes over from the library does anything. The small
// buckets spread their sizes across 64 bytes as well, since that is where an
// expander that unrolls sixteen longwords per iteration turns into a loop.
//
//   memcpy-small      constant sizes up to 128, aligned and odd, plus struct
//                     assignment
//   memcpy-large      constant sizes from 256 to 4096
//   memcpy-var-small  small sizes the compiler cannot see: per-call overhead
//                     of the runtime's memcpy
//   memcpy-var-large  large ones: its throughput
//   memmove-small     overlapping moves up to 128 bytes, both directions
//   memmove-large     the same at 2048 and 4096
//
// Neither expander accepts a run-time size, so the two var benchmarks measure
// the runtime rather than code generation; they are here to tell a change in
// the library apart from a change in what the compiler inlines.
//
// The checksum covers the whole destination buffer at the end of a run, so a
// copy that moves the wrong bytes, or a backwards move that eats its own
// source, changes the check value rather than the time. Each run leaves the
// buffer as it found it, so the value is stable across iterations: the copies
// are a fixed function of the source, and the overlapping moves are done in
// pairs that cancel.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "bench.h"

// Big enough that the working set leaves the 68040's 4K caches, small enough
// to stay comfortable on a 2 MB machine.
#define BUFSIZE (64L * 1024)

// Repeats per phase, chosen so each benchmark lands around a second on a
// 25 MHz 68040, in the same range as the other workloads.
#define SMALL_REPS      4800
#define SMALL_ODD_REPS  3200
#define STRUCT_REPS     6400
#define LARGE_REPS      1200
#define VAR_SMALL_REPS  6000
#define VAR_LARGE_REPS   900
#define MOVE_SMALL_REPS 8000
#define MOVE_LARGE_REPS  600

static UBYTE *src;
static UBYTE *dst;

// Sizes for the variable-size phases. Read through a volatile pointer so the
// compiler cannot fold them into constants and expand the copy inline: these
// phases exist to measure the call.
static const ULONG VAR_SMALL_SIZES[] = {
    3, 7, 12, 16, 21, 32, 48, 64, 96, 128,
};
#define NVAR_SMALL (sizeof(VAR_SMALL_SIZES) / sizeof(VAR_SMALL_SIZES[0]))

static const ULONG VAR_LARGE_SIZES[] = {
    200, 256, 384, 512, 1024, 2048, 4096,
};
#define NVAR_LARGE (sizeof(VAR_LARGE_SIZES) / sizeof(VAR_LARGE_SIZES[0]))

// The structs a program actually copies: a few registers' worth, a small
// record, and one at the by-pieces limit.
struct small { LONG a, b; };
struct point { SHORT x, y, z, pad; };
struct rec { ULONG id; UBYTE name[24]; LONG flags; };
struct big { ULONG w[32]; };

// Copy one constant size at a fixed offset. A macro rather than a function so
// the size stays a literal at the call site, which is what lets the compiler
// expand it by pieces.
#define COPY_CONST(off, len) memcpy(dst + (off), src + (off), (len))

// Keeps every phase inside the buffer: the small phases touch at most 1152
// bytes past their base, the large ones at most 12288.
#define SMALL_BASE(r) (((r) * 512) & (BUFSIZE - 4096))
#define LARGE_BASE(r) (((r) * 4096) & (BUFSIZE / 2 - 1) & ~4095L)

static bool memcpy_setup(void) {
    ULONG i;
    ULONG x = 2463534242UL;

    src = bench_alloc(BUFSIZE, "block move source");
    dst = bench_alloc(BUFSIZE, "block move destination");
    if (!src || !dst)
        return false;

    // Xorshift, so the bytes are not compressible and not all equal: a copy
    // that drops or repeats a run then shows up in the checksum.
    for (i = 0; i < BUFSIZE; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        src[i] = (UBYTE)(x >> 24);
    }
    // The destination is filled too, rather than zeroed: the overlapping moves
    // work on it in place, and moving zeros over zeros would look correct
    // however badly it went.
    for (i = 0; i < BUFSIZE; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        dst[i] = (UBYTE)(x >> 24);
    }
    return true;
}

// Constant sizes up to the by-pieces limit, at aligned offsets.
static void phase_small(void) {
    ULONG r;

    for (r = 0; r < SMALL_REPS; r++) {
        ULONG base = SMALL_BASE(r);

        COPY_CONST(base +    0,   4);
        COPY_CONST(base +   16,   8);
        COPY_CONST(base +   32,  12);
        COPY_CONST(base +   64,  16);
        COPY_CONST(base +   96,  24);
        COPY_CONST(base +  128,  32);
        COPY_CONST(base +  192,  48);
        COPY_CONST(base +  256,  64);
        COPY_CONST(base +  512,  96);
        COPY_CONST(base + 1024, 128);
    }
}

// The same sizes at offsets that are not a multiple of four, and at sizes that
// do not divide evenly: the head and tail an expander handles byte by byte.
static void phase_small_odd(void) {
    ULONG r;

    for (r = 0; r < SMALL_ODD_REPS; r++) {
        ULONG base = SMALL_BASE(r);

        COPY_CONST(base +    1,   5);
        COPY_CONST(base +   18,   9);
        COPY_CONST(base +   35,  13);
        COPY_CONST(base +   67,  27);
        COPY_CONST(base +  101,  33);
        COPY_CONST(base +  131,  45);
        COPY_CONST(base +  195,  67);
        COPY_CONST(base +  259,  99);
        COPY_CONST(base +  515, 125);
    }
}

// Whole-struct assignment: the same expansion as a constant-size memcpy, but
// reached through the type system, and the case bebbo's block-move work calls
// a structural move.
static void phase_struct(void) {
    ULONG r;

    for (r = 0; r < STRUCT_REPS; r++) {
        ULONG base = SMALL_BASE(r);
        struct small *s_in  = (struct small *)(void *)(src + base);
        struct small *s_out = (struct small *)(void *)(dst + base);
        struct point *p_in  = (struct point *)(void *)(src + base + 64);
        struct point *p_out = (struct point *)(void *)(dst + base + 64);
        struct rec *r_in    = (struct rec *)(void *)(src + base + 128);
        struct rec *r_out   = (struct rec *)(void *)(dst + base + 128);
        struct big *b_in    = (struct big *)(void *)(src + base + 256);
        struct big *b_out   = (struct big *)(void *)(dst + base + 256);

        *s_out = *s_in;
        *p_out = *p_in;
        *r_out = *r_in;
        *b_out = *b_in;
    }
}

// Constant sizes past the by-pieces limit, where the library call is the
// default and only an expander that takes over changes anything.
static void phase_large(void) {
    ULONG r;

    for (r = 0; r < LARGE_REPS; r++) {
        ULONG base = LARGE_BASE(r);

        COPY_CONST(base +    0,  256);
        COPY_CONST(base +  257,  257);
        COPY_CONST(base + 1024,  384);
        COPY_CONST(base + 2048,  512);
        COPY_CONST(base + 4096, 1024);
        COPY_CONST(base + 8192, 4096);
    }
}

static void phase_var(const ULONG *sizes, ULONG count, ULONG reps,
                      ULONG stride) {
    const volatile ULONG *vsizes = sizes;
    ULONG r, i;

    for (r = 0; r < reps; r++) {
        ULONG base = LARGE_BASE(r);

        for (i = 0; i < count; i++) {
            ULONG n = vsizes[i];
            ULONG off = base + i * stride;

            memcpy(dst + off, src + off, n);
        }
    }
}

// Overlapping moves, both directions, in pairs that shift a block up and then
// back down by the same distance. That restores the block and leaves only the
// gap it was shifted across altered, which keeps the check value stable across
// iterations while still exercising both directions. A backwards move that
// runs forwards smears one value over the range and the checksum changes.
static void phase_move_small(void) {
    ULONG r;

    for (r = 0; r < MOVE_SMALL_REPS; r++) {
        memmove(dst + 4, dst, 128);
        memmove(dst, dst + 4, 128);
        memmove(dst + 1024 + 2, dst + 1024, 64);
        memmove(dst + 1024, dst + 1024 + 2, 64);
        memmove(dst + 2048 + 1, dst + 2048, 32);
        memmove(dst + 2048, dst + 2048 + 1, 32);
    }
}

static void phase_move_large(void) {
    ULONG r;

    for (r = 0; r < MOVE_LARGE_REPS; r++) {
        memmove(dst + 64, dst, 4096);
        memmove(dst, dst + 64, 4096);
        memmove(dst + 8192 + 4, dst + 8192, 2048);
        memmove(dst + 8192, dst + 8192 + 4, 2048);
    }
}

static bool small_run(ULONG *check) {
    phase_small();
    phase_small_odd();
    phase_struct();
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static bool large_run(ULONG *check) {
    phase_large();
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static bool var_small_run(ULONG *check) {
    phase_var(VAR_SMALL_SIZES, NVAR_SMALL, VAR_SMALL_REPS, 256);
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static bool var_large_run(ULONG *check) {
    phase_var(VAR_LARGE_SIZES, NVAR_LARGE, VAR_LARGE_REPS, 4096);
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static bool move_small_run(ULONG *check) {
    phase_move_small();
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static bool move_large_run(ULONG *check) {
    phase_move_large();
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static void memcpy_teardown(void) {
    bench_free(src);
    bench_free(dst);
    src = NULL;
    dst = NULL;
}

const struct bench bench_memcpy_small = {
    "memcpy-small",
    "constant copies up to 128 bytes, aligned, odd and struct",
    memcpy_setup,
    small_run,
    memcpy_teardown,
};

const struct bench bench_memcpy_large = {
    "memcpy-large",
    "constant copies from 256 to 4096 bytes",
    memcpy_setup,
    large_run,
    memcpy_teardown,
};

const struct bench bench_memcpy_var_small = {
    "memcpy-var-small",
    "small copies sized at run time: the library call overhead",
    memcpy_setup,
    var_small_run,
    memcpy_teardown,
};

const struct bench bench_memcpy_var_large = {
    "memcpy-var-large",
    "large copies sized at run time: the library throughput",
    memcpy_setup,
    var_large_run,
    memcpy_teardown,
};

const struct bench bench_memmove_small = {
    "memmove-small",
    "overlapping moves up to 128 bytes, both directions",
    memcpy_setup,
    move_small_run,
    memcpy_teardown,
};

const struct bench bench_memmove_large = {
    "memmove-large",
    "overlapping moves of 2048 and 4096 bytes, both directions",
    memcpy_setup,
    move_large_run,
    memcpy_teardown,
};
