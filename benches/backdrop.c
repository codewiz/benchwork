// SPDX-License-Identifier: 0BSD
//
// The p96cts backdrop scene: a landscape with an off-center sun over two
// ridges, rippled water and a boat, synthesized per pixel, then
// Floyd-Steinberg dithered to a truecolor RGB444 frame and an 8-bit 3-3-2 pen
// frame. The scene is taken from p96cts's backdrop.c with the RastPort blit
// removed; p96cts dithers with a 4x4 Bayer matrix instead.
//
// It is a good compiler workload because it is what a software renderer
// looks like: a dozen 32-bit multiplies and divides per pixel, a handful of
// data-dependent branches, and byte stores, then error diffusion that carries
// signed values from pixel to pixel and row to row. Nothing is vectorizable
// and nothing hoists, so what is left is how well the compiler schedules
// plain integer code and keeps the loops' invariants in registers.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "bench.h"

// A PAL low-res screen. The PNG benchmark encodes the truecolor frame.
#define WIDTH 320
#define HEIGHT 256

// Integer hash of one value, for the terrain outline. Knuth's multiplicative
// constant; the shift picks bits that vary quickly.
static int hash8(int i) {
    unsigned int u = (unsigned int)i * 2654435761U;
    return (int)((u >> 13) & 0xFF);
}

// A ridge line: hashed control points every `step` pixels, linearly
// interpolated. Irregular and non-repeating, unlike a sine.
static int ridge(int x, int seed, int amp, int base, int step) {
    int i = x / step, f = x % step;
    int a = hash8(i + seed) * amp / 255;
    int b = hash8(i + 1 + seed) * amp / 255;
    return base - (a + (b - a) * f / step);
}

static UBYTE clamp8(int v) {
    return (UBYTE)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

// Synthesize the scene into px, w*h*3 bytes of R8G8B8.
static void backdrop(UBYTE *px, SHORT w, SHORT h) {
    int horizon = h * 3 / 5;
    int sun_x = w * 7 / 10, sun_y = h * 7 / 25, sun_r = h / 7;
    int boat_x = w / 4, boat_y = horizon + h / 5;
    int boat_w = w / 6, boat_h = h / 12;
    SHORT x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            int dx = x - sun_x, dy = y - sun_y;
            int r, g, b;
            ULONG p;

            if (y < horizon) {
                // Sky: deep blue overhead warming to orange at the horizon.
                int t = y * 255 / horizon;
                r = 40 + t * 200 / 255;
                g = 60 + t * 120 / 255;
                b = 150 - t * 100 / 255;
                if (dx * dx + dy * dy < sun_r * sun_r) {
                    r = 255;
                    g = 230;
                    b = 120;
                }
            } else {
                // Water: the sky's horizon tone, darkening with depth, with
                // ripples that break up every row differently.
                int t = (y - horizon) * 255 / (h - horizon);
                // Ripples: horizontal bars whose phase shifts per row, so the
                // water varies along both axes without looking like noise.
                int rip = ((x + hash8(y) / 8) / 3 & 7) * 5;
                r = 120 - t * 80 / 255 + rip;
                g = 90 - t * 60 / 255 + rip;
                b = 110 + t * 40 / 255 + rip;
                // Sun's reflection, wobbling as it goes down the water.
                if (x > sun_x - sun_r / 2 + hash8(y * 3) / 16 - 8 &&
                    x < sun_x + sun_r / 2 + hash8(y * 3) / 16 - 8) {
                    r += 90;
                    g += 70;
                }
            }

            // Two ridges, the near one darker, both drawn over the sky.
            if (y > ridge(x, 11, h / 5, horizon, w / 10) && y < horizon) {
                r = 90;
                g = 70;
                b = 90;
            }
            if (y > ridge(x, 77, h / 8, horizon, w / 16) && y < horizon) {
                r = 45;
                g = 40;
                b = 55;
            }

            // A boat: hull, then mast and sail.
            if (y >= boat_y && y < boat_y + boat_h &&
                x >= boat_x + (y - boat_y) && x < boat_x + boat_w - (y - boat_y)) {
                r = 30;
                g = 20;
                b = 25;
            }
            if (x >= boat_x + boat_w / 2 && x < boat_x + boat_w / 2 + 2 &&
                y < boat_y && y > boat_y - boat_h * 3)
                r = g = b = 20;
            if (y < boat_y && y > boat_y - boat_h * 3 &&
                x > boat_x + boat_w / 2 + 2 &&
                x < boat_x + boat_w / 2 + 2 + (y - (boat_y - boat_h * 3)) / 2) {
                r = 230;
                g = 225;
                b = 210;
            }

            p = ((ULONG)y * w + x) * 3;
            px[p] = clamp8(r);
            px[p + 1] = clamp8(g);
            px[p + 2] = clamp8(b);
        }
    }
}

// Floyd-Steinberg error diffusion of the R8G8B8 scene to levels[c] levels
// per channel. With rgb set, the quantized colors go to out as R8G8B8 scaled
// back to 0-255; otherwise the level indices go to out as 3-3-2 pens, the
// way p96cts packs them: pens 0-5 are named colors there, so a color landing
// on one is pushed up into the blue half of the cube instead. `err` holds two
// rows of carried error, (w + 2) * 3 ints each, as ints so the diffusion
// does not clamp.
static void dither(const UBYTE *scene, UBYTE *out, SHORT w, SHORT h,
                   const int levels[3], bool rgb, int *err) {
    int *cur = err + 3, *next = err + (w + 2) * 3 + 3;
    SHORT x, y;
    int c;

    memset(err, 0, (ULONG)(w + 2) * 3 * 2 * sizeof(int));
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            const UBYTE *s = scene + ((ULONG)y * w + x) * 3;
            int pen = 0;

            for (c = 0; c < 3; c++) {
                int top = levels[c] - 1;
                int v = s[c] + cur[x * 3 + c], i, q, e;

                v = v < 0 ? 0 : (v > 255 ? 255 : v);
                i = (v * top + 127) / 255;
                q = i * 255 / top;
                e = v - q;
                if (rgb)
                    out[((ULONG)y * w + x) * 3 + c] = (UBYTE)q;
                else
                    pen |= i << (c * 3);
                cur[(x + 1) * 3 + c] += e * 7 / 16;
                next[(x - 1) * 3 + c] += e * 3 / 16;
                next[x * 3 + c] += e * 5 / 16;
                next[(x + 1) * 3 + c] += e / 16;
            }
            if (!rgb)
                out[(ULONG)y * w + x] = (UBYTE)(pen < 6 ? pen | 0x40 : pen);
        }
        // The next row becomes the current one, and the old one is cleared
        // to collect the row after.
        {
            int *t = cur;
            cur = next;
            next = t;
            memset(next - 3, 0, (ULONG)(w + 2) * 3 * sizeof(int));
        }
    }
}

static const int RGB444[3] = {16, 16, 16};
static const int PEN332[3] = {8, 8, 4};

static UBYTE *scene, *frame;
static int *err;

static bool backdrop_setup(void) {
    scene = bench_alloc(WIDTH * HEIGHT * 3, "backdrop scene");
    frame = bench_alloc(WIDTH * HEIGHT * 3, "backdrop frame");
    err = bench_alloc((WIDTH + 2) * 3 * 2 * sizeof(int), "dither rows");
    return scene && frame && err;
}

// One truecolor frame and one 8-bit frame from the same scene, so both the
// byte-triple store path and the pen packing are measured.
static bool backdrop_run(ULONG *check) {
    backdrop(scene, WIDTH, HEIGHT);
    dither(scene, frame, WIDTH, HEIGHT, RGB444, true, err);
    *check = checksum(0, frame, WIDTH * HEIGHT * 3);
    dither(scene, frame, WIDTH, HEIGHT, PEN332, false, err);
    *check = checksum(*check, frame, WIDTH * HEIGHT);
    return true;
}

static void backdrop_teardown(void) {
    bench_free(scene);
    bench_free(frame);
    bench_free(err);
    scene = frame = NULL;
    err = NULL;
}

// For the PNG benchmark, which needs a picture to encode: the truecolor
// frame, in a buffer the caller frees with bench_free(), or NULL.
UBYTE *backdrop_rgb(SHORT *w, SHORT *h) {
    UBYTE *px = NULL;

    if (backdrop_setup()) {
        backdrop(scene, WIDTH, HEIGHT);
        dither(scene, frame, WIDTH, HEIGHT, RGB444, true, err);
        px = frame;
        frame = NULL;
        *w = WIDTH;
        *h = HEIGHT;
    }
    backdrop_teardown();
    return px;
}

const struct bench bench_backdrop = {
    "backdrop",
    "p96cts landscape scene, 320x256, truecolor and 8-bit",
    backdrop_setup,
    backdrop_run,
    backdrop_teardown,
};
