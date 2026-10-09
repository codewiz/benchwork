// SPDX-License-Identifier: 0BSD
//
// Wipeout's render_push_tris(), from arczi84's AmigaOS port of the Wipeout
// rewrite: the function the game calls once per triangle it draws. It
// rescales the three vertices' UVs with a float divide and multiply each,
// doubles and clamps their colour bytes, and assigns the 96-byte triangle
// into a 16-entry staging buffer, flushing the buffer whenever it fills or
// the texture changes.
//
// The triangle is passed by value, so the compiler copies 96 bytes into the
// call and 96 bytes out of it into the buffer: this is the workload behind
// the AmigaOS small-copy expansion, where a struct assignment above the
// by-pieces limit either becomes a memcpy() call or a run of moves. Around
// it sit the float math, the byte arithmetic and the flush branches of real
// game code, so the copy is measured in the proportion the game gives it.
//
// kernel.c is the extracted function, unchanged, in third_party/wipeout. The
// flush hands the batch to bench_consume() here, in a separate object with
// no LTO, so the stores into the buffer stay observable and the compiler
// cannot specialize the kernel on what happens to the triangles.
//
// A run submits 1024 triangles per pass under the three texture patterns
// arczi84's harness uses: one texture for the whole pass, a switch every 8
// triangles, and one every triangle, which takes the flush from only when
// the buffer fills to every call. The check
// value is a hash of one extra pass per pattern whose triangles are also
// compared against a scalar reference computed here; the inputs are chosen
// so the float results are exact, and a kernel that copies, scales or clamps
// wrongly fails the run rather than merely changing the hash.

#include <string.h>

#include "bench.h"
#include "third_party/wipeout/bench.h"

// Triangles per pass, as in the original harness.
#define N 1024

// Triangles per texture run (1024 is the whole pass, so no switch) and the
// passes timed under each. Together about a second on a 25 MHz 68040 with
// soft float.
static const unsigned RUNS[3] = { 1024, 8, 1 };
static const unsigned PASSES[3] = { 8, 8, 4 };

static tris_t *input;
static UWORD *tex;      // 3 * N texture indices, one set per pattern
static const UWORD *tex_cur;
static ULONG consumed;
static bool checking, bad;
static ULONG hash;

// What render_push_tris() must produce for input triangle I under texture T:
// the same arithmetic as the game's, written as multiplications by exact
// reciprocals, so it is independent of how the kernel was compiled.
static void reference(tris_t *t, unsigned texture) {
    unsigned j;

    for (j = 0; j < 3; j++) {
        vertex_t *v = &t->vertices[j];

        v->uv.x *= 1.0f / (float)(32 << (texture % 3));
        v->uv.y *= 0.5f / (float)(32 << ((texture + 1) % 3));
        if (v->color.a) {
            v->color.r = v->color.r > 127 ? 255 : v->color.r * 2;
            v->color.g = v->color.g > 127 ? 255 : v->color.g * 2;
            v->color.b = v->color.b > 127 ? 255 : v->color.b * 2;
        }
    }
}

// The kernel's flush. While timing it only counts; in the check pass it
// hashes the batch and compares it with the reference.
void bench_consume(const tris_t *tris, uint32_t count) {
    uint32_t i;

    if (checking) {
        for (i = 0; i < count; i++) {
            ULONG k = (consumed + i) % N;
            tris_t want = input[k];

            reference(&want, tex_cur[k]);
            if (memcmp(&tris[i], &want, sizeof want))
                bad = true;
            hash = checksum(hash, &tris[i], sizeof tris[i]);
        }
    }
    consumed += count;
}

static bool wipeout_setup(void) {
    unsigned i, j, m;

    input = bench_alloc(N * sizeof *input, "triangles");
    tex = bench_alloc(3 * N * sizeof *tex, "texture indices");
    if (!input || !tex)
        return false;

    memset(input, 0, N * sizeof *input);
    for (i = 0; i < N; i++) {
        for (j = 0; j < 3; j++) {
            vertex_t *v = &input[i].vertices[j];

            v->pos.x = (float)i;
            v->pos.y = (float)j;
            v->pos.z = (float)(i + j);
            v->uv.x = (float)((i * 7 + j) % 256);
            v->uv.y = (float)((i + j * 13) % 256);
            v->color.r = i % 256;
            v->color.g = (i + j * 31) % 256;
            v->color.b = (i * 3 + j) % 256;
            v->color.a = i % 7 ? 255 : 0;
        }
        for (m = 0; m < 3; m++)
            tex[m * N + i] = (i / RUNS[m]) % 8;
    }
    return true;
}

static void submit(unsigned passes) {
    unsigned p, i;

    consumed = 0;
    bench_reset();
    for (p = 0; p < passes; p++)
        for (i = 0; i < N; i++)
            render_push_tris(input[i], tex_cur[i]);
    bench_finish();
}

static bool wipeout_run(ULONG *check) {
    unsigned m;

    hash = 0;
    bad = false;
    for (m = 0; m < 3; m++) {
        tex_cur = tex + m * N;

        checking = false;
        submit(PASSES[m]);
        if (consumed != PASSES[m] * N)
            return false;

        checking = true;
        submit(1);
        if (bad || consumed != N)
            return false;
    }
    *check = hash;
    return true;
}

static void wipeout_teardown(void) {
    bench_free(input);
    bench_free(tex);
    input = NULL;
    tex = NULL;
}

const struct bench bench_wipeout_tris = {
    "wipeout-tris",
    "Wipeout's triangle submission: float UVs, byte clamps, 96-byte copies",
    wipeout_setup,
    wipeout_run,
    wipeout_teardown,
};
