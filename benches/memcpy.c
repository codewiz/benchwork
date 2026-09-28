// SPDX-License-Identifier: 0BSD
//
// Block moves: the copies a compiler either expands inline or turns into a
// library call, and the ones it must not get wrong.
//
// None of the other workloads measures this directly, yet it is one of the
// places where the m68k backend makes the largest difference. A copy whose
// size is a compile-time constant can be expanded by pieces, as a run of
// moves; one whose size is only known at run time becomes a call, which on
// AmigaOS with libnix reaches exec CopyMem. Struct assignment takes the same
// path as a constant-size memcpy, and an overlapping move has to be done
// backwards, which is where a block-move expander is most likely to be wrong
// rather than merely slow.
//
// The three benchmarks below cover those cases separately:
//
//   memcpy-fixed  constant sizes the compiler expands: aligned, unaligned
//                 and whole-struct assignment
//   memcpy-var    sizes the compiler cannot see, so the call survives
//   memmove       overlapping moves, forwards and backwards
//
// They are separate benchmarks rather than phases of one, because they move
// very different amounts of memory for the work they represent: rolled into
// a single number the library path would dominate and hide the expansion
// changes, which are the ones a backend patch usually moves.
//
// The checksum covers the whole destination buffer at the end of a run, so a
// copy that moves the wrong bytes, or a backwards move that eats its own
// source, changes the check value rather than the time. Each run leaves the
// buffer in the same state it found it in, so the value is stable across
// iterations: the copies are a fixed function of the source, and the
// overlapping moves below are done in pairs that cancel.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "bench.h"

// Big enough that the working set leaves the 68040's 4K caches, small enough
// to stay comfortable on a 2 MB machine.
#define BUFSIZE (64L * 1024)

// Repeats per phase, chosen so each benchmark lands around a second on a
// 25 MHz 68040, in the same range as the other workloads.
#define CONST_REPS  4800
#define ODD_REPS    3200
#define VAR_REPS    4800
#define STRUCT_REPS 6400
#define MOVE_REPS   960

static UBYTE *src;
static UBYTE *dst;

// Sizes for the variable-size phase. Read through a volatile pointer so the
// compiler cannot fold them into constants and expand the copy inline: this
// phase exists to measure the library call.
static const ULONG VAR_SIZES[] = {
    3, 7, 12, 16, 21, 32, 48, 64, 96, 128, 200, 256, 384, 512, 1024, 2048,
};
#define NVAR_SIZES (sizeof(VAR_SIZES) / sizeof(VAR_SIZES[0]))

// The structs a program actually copies: a few registers' worth, a small
// record, and something large enough to be worth a loop.
struct small { LONG a, b; };
struct point { SHORT x, y, z, pad; };
struct rec { ULONG id; UBYTE name[24]; LONG flags; };
struct big { ULONG w[32]; };

// Copy one constant size at a fixed offset. A macro rather than a function so
// the size stays a literal at the call site, which is what lets the compiler
// expand it by pieces.
#define COPY_CONST(off, len) memcpy(dst + (off), src + (off), (len))

static bool memcpy_setup(void) {
    ULONG i;
    ULONG x = 2463534242UL;

    src = AllocVec(BUFSIZE, MEMF_ANY);
    dst = AllocVec(BUFSIZE, MEMF_ANY);
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
    // The destination is filled too, rather than zeroed: the overlapping
    // moves work on it in place, and moving zeros over zeros would look
    // correct however badly it went.
    for (i = 0; i < BUFSIZE; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        dst[i] = (UBYTE)(x >> 24);
    }
    return true;
}

// Constant sizes at aligned offsets: the by-pieces path.
static void phase_const(void) {
    ULONG r;

    for (r = 0; r < CONST_REPS; r++) {
        ULONG base = (r * 512) & (BUFSIZE - 4096);

        COPY_CONST(base +    0,   4);
        COPY_CONST(base +   16,   8);
        COPY_CONST(base +   32,  12);
        COPY_CONST(base +   64,  16);
        COPY_CONST(base +   96,  24);
        COPY_CONST(base +  128,  32);
        COPY_CONST(base +  192,  48);
        COPY_CONST(base +  256,  64);
        COPY_CONST(base +  512, 128);
        COPY_CONST(base + 1024, 256);
    }
}

// The same, but at offsets that are not a multiple of four and at sizes that
// do not divide evenly: the head and tail the expander has to handle byte by
// byte.
static void phase_odd(void) {
    ULONG r;

    for (r = 0; r < ODD_REPS; r++) {
        ULONG base = (r * 512) & (BUFSIZE - 4096);

        COPY_CONST(base +    1,   5);
        COPY_CONST(base +   18,   9);
        COPY_CONST(base +   35,  13);
        COPY_CONST(base +   67,  27);
        COPY_CONST(base +  101,  33);
        COPY_CONST(base +  131,  45);
        COPY_CONST(base +  195,  67);
        COPY_CONST(base +  259,  99);
        COPY_CONST(base +  515, 129);
        COPY_CONST(base + 1027, 257);
    }
}

// Sizes the compiler cannot see, so the call survives: on AmigaOS this is
// libnix's memcpy and thus exec CopyMem.
static void phase_var(void) {
    const volatile ULONG *sizes = VAR_SIZES;
    ULONG r, i;

    for (r = 0; r < VAR_REPS; r++) {
        ULONG base = (r * 512) & (BUFSIZE - 8192);

        for (i = 0; i < NVAR_SIZES; i++) {
            ULONG n = sizes[i];

            memcpy(dst + base + i * 64, src + base + i * 64, n);
        }
    }
}

// Whole-struct assignment: the same expansion as a constant-size memcpy, but
// reached through the type system, and the case bebbo's block-move work calls
// a structural move.
static void phase_struct(void) {
    ULONG r;

    for (r = 0; r < STRUCT_REPS; r++) {
        ULONG base = (r * 256) & (BUFSIZE - 4096);
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

// Overlapping moves within one buffer, both directions. A backwards move has
// to run from the end; an expander that copies forwards, or that increments a
// register it still needs, smears one value over the range and the checksum
// changes.
//
// The moves come in pairs that shift a block up and then back down by the
// same distance, which restores the block and leaves only the gap it was
// shifted across altered. That keeps the check value stable across iterations
// while still exercising both directions.
static void phase_move(void) {
    ULONG r;

    for (r = 0; r < MOVE_REPS; r++) {
        // destination above the source, then back
        memmove(dst + 64, dst, 4096);
        memmove(dst, dst + 64, 4096);
        // a smaller distance, where head and tail dominate
        memmove(dst + 8192 + 4, dst + 8192, 2048);
        memmove(dst + 8192, dst + 8192 + 4, 2048);
        // a size the expander may treat specially
        memmove(dst + 16384 + 2, dst + 16384, 128);
        memmove(dst + 16384, dst + 16384 + 2, 128);
    }
}

static bool fixed_run(ULONG *check) {
    phase_const();
    phase_odd();
    phase_struct();
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static bool var_run(ULONG *check) {
    phase_var();
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static bool move_run(ULONG *check) {
    phase_move();
    *check = checksum(0, dst, BUFSIZE);
    return true;
}

static void memcpy_teardown(void) {
    FreeVec(src);
    FreeVec(dst);
    src = NULL;
    dst = NULL;
}

const struct bench bench_memcpy_fixed = {
    "memcpy-fixed",
    "constant-size copies the compiler expands, aligned, odd and struct",
    memcpy_setup,
    fixed_run,
    memcpy_teardown,
};

const struct bench bench_memcpy_var = {
    "memcpy-var",
    "copies whose size is unknown at compile time, so the call survives",
    memcpy_setup,
    var_run,
    memcpy_teardown,
};

const struct bench bench_memmove = {
    "memmove",
    "overlapping moves, forwards and backwards",
    memcpy_setup,
    move_run,
    memcpy_teardown,
};
