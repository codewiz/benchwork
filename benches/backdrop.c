// SPDX-License-Identifier: 0BSD
//
// The p96cts backdrop scene: a dithered landscape with an off-center sun over
// two ridges, rippled water and a boat, synthesized per pixel. The scene is
// taken from p96cts's backdrop.c with the RastPort blit removed; the pixel
// loop is verbatim.
//
// It is a good compiler workload because it is what a software renderer
// looks like: a dozen 32-bit multiplies and divides per pixel, a table
// lookup for the dither, a handful of data-dependent branches, and byte
// stores. Nothing is vectorizable and nothing hoists, so what is left is
// how well the compiler schedules plain integer code and keeps the loop's
// invariants in registers.

#include <proto/exec.h>
#include <exec/memory.h>

#include "bench.h"

// The benchmark renders a quarter of a 640x480 screen: at full size the
// dozen divides per pixel put one frame near four seconds on a 25 MHz
// 68040. The PNG benchmark takes the full-size scene, once, at setup.
#define WIDTH 320
#define HEIGHT 240

#define PNG_WIDTH 640
#define PNG_HEIGHT 480

// Integer hash of one value, for the terrain outline. Knuth's multiplicative
// constant; the shift picks bits that vary quickly.
static int hash8(int i) {
    unsigned int u = (unsigned int)i * 2654435761U;
    return (int)((u >> 13) & 0xFF);
}

// Ordered (Bayer) dither, the classic 4x4 recursive matrix. Preferred over
// error diffusion here because it is a pure function of the coordinates: the
// scene stays identical whatever order a driver happens to render in, and it
// costs one table lookup per pixel.
static const UBYTE BAYER[4][4] = {
    { 0,  8,  2, 10},
    {12,  4, 14,  6},
    { 3, 11,  1,  9},
    {15,  7, 13,  5},
};

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

// Quantize to the 3-3-2 palette cube. Pens 0-5 are named colors in p96cts,
// so a scene color landing on one is pushed up into the blue half of the
// cube instead.
static UBYTE pen_of(int r, int g, int b) {
    int pen = (clamp8(r) >> 5) | ((clamp8(g) >> 5) << 3) | ((clamp8(b) >> 6) << 6);
    return (UBYTE)(pen < 6 ? pen | 0x40 : pen);
}

// Synthesize the scene into px, w*h*3 bytes of R8G8B8 when truecolor, else
// w*h pens.
static void backdrop(UBYTE *px, SHORT w, SHORT h, bool truecolor) {
    int horizon = h * 3 / 5;
    int sun_x = w * 7 / 10, sun_y = h * 7 / 25, sun_r = h / 7;
    int boat_x = w / 4, boat_y = horizon + h / 5;
    int boat_w = w / 6, boat_h = h / 12;
    SHORT x, y;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            // Scaled to a bit under one palette step, so the dither shapes
            // the boundary between two adjacent shades rather than adding
            // visible speckle.
            int d = (BAYER[y & 3][x & 3] - 8) * 3;
            int dx = x - sun_x, dy = y - sun_y;
            int r, g, b;

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

            if (truecolor) {
                ULONG p = ((ULONG)y * w + x) * 3;
                px[p] = clamp8(r + d);
                px[p + 1] = clamp8(g + d);
                px[p + 2] = clamp8(b + d);
            } else {
                px[y * w + x] = pen_of(r + d, g + d, b + d);
            }
        }
    }
}

static UBYTE *pixels;

static bool backdrop_setup(void) {
    pixels = bench_alloc(WIDTH * HEIGHT * 3, "backdrop frame");
    return pixels != NULL;
}

// One truecolor frame and one 8-bit frame, so both the byte-triple store
// path and the pen quantization are measured.
static bool backdrop_run(ULONG *check) {
    backdrop(pixels, WIDTH, HEIGHT, true);
    *check = checksum(0, pixels, WIDTH * HEIGHT * 3);
    backdrop(pixels, WIDTH, HEIGHT, false);
    *check = checksum(*check, pixels, WIDTH * HEIGHT);
    return true;
}

static void backdrop_teardown(void) {
    bench_free(pixels);
    pixels = NULL;
}

// For the PNG benchmark, which needs a picture to encode: the 640x480 R8G8B8
// scene in a buffer the caller frees with bench_free(), or NULL.
UBYTE *backdrop_rgb(SHORT *w, SHORT *h) {
    UBYTE *px = bench_alloc(PNG_WIDTH * PNG_HEIGHT * 3, "backdrop scene");

    if (!px)
        return NULL;
    backdrop(px, PNG_WIDTH, PNG_HEIGHT, true);
    *w = PNG_WIDTH;
    *h = PNG_HEIGHT;
    return px;
}

const struct bench bench_backdrop = {
    "backdrop",
    "p96cts landscape scene, 320x240, truecolor and 8-bit",
    backdrop_setup,
    backdrop_run,
    backdrop_teardown,
};
