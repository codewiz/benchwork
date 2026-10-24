// SPDX-License-Identifier: 0BSD
//
// A landscape with an off-center sun over two ridges, rippled water and a
// boat, and its remap to a 32-color screen: two benchmarks.
//
// - scene-2d renders it per pixel, which is what a software renderer
//   looks like: a dozen 32-bit multiplies and divides per pixel, a handful
//   of data-dependent branches, and byte stores. The scene is p96cts's
//   backdrop, with the RastPort blit removed.
// - quantize-floyd remaps it to 32 colors, as a paint program turns a 24-bit
//   picture into 5 bitplanes: Floyd-Steinberg error diffusion, carrying
//   signed error from pixel to pixel and row to row, and a nearest-color
//   search for each new shade.
//
// Nothing is vectorizable and nothing hoists, so what is left is how well the
// compiler schedules plain integer code and keeps the loops' invariants in
// registers.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "bench.h"

// An NTSC lores screen, which quantize-floyd remaps and the PNG benchmarks
// encode; scene-2d renders a hires one, which takes it to about a second.
#define WIDTH SCENE_WIDTH
#define HEIGHT SCENE_HEIGHT
#define HIRES_WIDTH (2 * SCENE_WIDTH)

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

// Render the scene into px, w*h*3 bytes of R8G8B8.
static void render(UBYTE *px, SHORT w, SHORT h) {
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

// The scene's 32 colors: a median cut of the rendered scene, computed once
// on a host and kept here, as a paint program saves the palette with the
// picture. Darkest first.
#define NCOLORS 32
static const UBYTE PALETTE[NCOLORS][3] = {
    { 29,  20,  24}, { 45,  40,  55}, { 53,  50, 151}, { 56,  69, 142},
    { 91,  72,  93}, { 83,  68, 144}, { 82,  73, 131}, { 84,  70, 169},
    { 90,  89, 125}, {114,  94, 120}, {116,  92, 140}, {114,  93, 158},
    {124, 108, 108}, {128, 105, 129}, {134, 108, 152}, {140, 111, 137},
    {148, 124,  96}, {154, 121, 138}, {153, 123, 147}, {159, 127, 161},
    {172, 139,  84}, {187, 148,  78}, {191, 150,  75}, {195, 152,  73},
    {194, 151, 136}, {187, 150, 165}, {207, 160,  67}, {204, 156, 117},
    {209, 168, 159}, {223, 176, 138}, {230, 225, 210}, {255, 230, 120},
};

// The PALETTE entry nearest to R, G, B by squared RGB distance, giving up on
// an entry as soon as part of its distance is already too far.
static int nearest(int r, int g, int b) {
    int best_d = 3 * 255 * 255 + 1, best = 0, i;

    for (i = 0; i < NCOLORS; i++) {
        int dr = r - PALETTE[i][0], dg, db, d = dr * dr;

        if (d >= best_d)
            continue;
        dg = g - PALETTE[i][1];
        d += dg * dg;
        if (d >= best_d)
            continue;
        db = b - PALETTE[i][2];
        d += db * db;
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

// An inverse colormap, as remappers kept one: the pen for each color at 5 bits
// a channel, searched the first time a pixel needs it. The search is for the
// cell's center, so the picture does not depend on the order cells fill.
#define CELL_EMPTY 0xFF
#define CELLS (32L * 32 * 32)

static int cached_pen(UBYTE *cells, int r, int g, int b) {
    int cell = (r >> 3) << 10 | (g >> 3) << 5 | b >> 3;

    if (cells[cell] == CELL_EMPTY)
        cells[cell] = (UBYTE)nearest(r | 4, g | 4, b | 4);
    return cells[cell];
}

// Floyd-Steinberg remap of the R8G8B8 scene to PALETTE, one pen per pixel.
// `err` holds two rows of carried error, (w + 2) * 3 ints each, so that the
// diffusion does not clamp; `cells` is the inverse colormap, emptied here.
static void floyd(const UBYTE *scene, UBYTE *pens, SHORT w, SHORT h,
                  int *err, UBYTE *cells) {
    int *cur = err + 3, *next = err + (w + 2) * 3 + 3;
    SHORT x, y;
    int c;

    memset(err, 0, (ULONG)(w + 2) * 3 * 2 * sizeof(int));
    memset(cells, CELL_EMPTY, CELLS);
    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            const UBYTE *s = scene + ((ULONG)y * w + x) * 3;
            int v[3], pen;

            v[0] = clamp8(s[0] + cur[x * 3]);
            v[1] = clamp8(s[1] + cur[x * 3 + 1]);
            v[2] = clamp8(s[2] + cur[x * 3 + 2]);
            pen = cached_pen(cells, v[0], v[1], v[2]);
            pens[(ULONG)y * w + x] = (UBYTE)pen;
            for (c = 0; c < 3; c++) {
                int e = v[c] - PALETTE[pen][c];

                cur[(x + 1) * 3 + c] += e * 7 / 16;
                next[(x - 1) * 3 + c] += e * 3 / 16;
                next[x * 3 + c] += e * 5 / 16;
                next[(x + 1) * 3 + c] += e / 16;
            }
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

// One kind of state for both benchmarks; scene-2d uses only the scene.
struct scene_state {
    UBYTE *scene;   // the rendered scene, 8-bit RGB
    UBYTE *pens;    // remapped to PALETTE
    int *err;       // two rows of dither error
    UBYTE *cells;   // the inverse colormap
};

static void scene_teardown(void *state) {
    struct scene_state *s = state;

    if (s) {
        bench_free(s->scene);
        bench_free(s->pens);
        bench_free(s->err);
        bench_free(s->cells);
        bench_free(s);
    }
}

static void *scene_setup(void) {
    struct scene_state *s = bench_alloc(sizeof *s, "scene state");

    if (!s)
        return NULL;
    s->scene = bench_alloc(HIRES_WIDTH * HEIGHT * 3, "scene pixels");
    s->pens = bench_alloc(WIDTH * HEIGHT, "scene pens");
    s->err = bench_alloc((WIDTH + 2) * 3 * 2 * sizeof(int), "dither rows");
    s->cells = bench_alloc(CELLS, "inverse colormap");
    if (!s->scene || !s->pens || !s->err || !s->cells) {
        scene_teardown(s);
        return NULL;
    }
    return s;
}

static const void *scene_run(void *state) {
    struct scene_state *s = state;

    render(s->scene, HIRES_WIDTH, HEIGHT);
    return s;
}

static bool scene_verify(void *state, const void *result, ULONG *check) {
    const struct scene_state *s = result;

    (void)state;
    *check = checksum(0, s->scene, HIRES_WIDTH * HEIGHT * 3);
    return true;
}

// The scene to remap is rendered here, outside the timing.
static void *floyd_setup(void) {
    struct scene_state *s = scene_setup();

    if (s)
        render(s->scene, WIDTH, HEIGHT);
    return s;
}

static const void *floyd_run(void *state) {
    struct scene_state *s = state;

    floyd(s->scene, s->pens, WIDTH, HEIGHT, s->err, s->cells);
    return s;
}

static bool floyd_verify(void *state, const void *result, ULONG *check) {
    const struct scene_state *s = result;

    (void)state;
    *check = checksum(0, s->pens, WIDTH * HEIGHT);
    return true;
}

// For the PNG benchmark, which needs a picture to encode: the 24-bit scene,
// in a buffer the caller frees with bench_free(), or NULL.
UBYTE *scene_rgb(SHORT *w, SHORT *h) {
    struct scene_state *s = floyd_setup();
    UBYTE *px = NULL;

    if (s) {
        px = s->scene;
        s->scene = NULL;
        *w = WIDTH;
        *h = HEIGHT;
    }
    scene_teardown(s);
    return px;
}

const struct Bench bench_scene_2d = {
    /* .name= */ "scene-2d",
    /* .desc= */ "a 640x200 landscape rendered per pixel",
    /* .unit= */ "px",
    /* .per_loop= */ (double)HIRES_WIDTH * HEIGHT,
    /* .setup= */ scene_setup,
    /* .run= */ scene_run,
    /* .verify= */ scene_verify,
    /* .teardown= */ scene_teardown,
};

const struct Bench bench_quantize_floyd = {
    /* .name= */ "quantize-floyd",
    /* .desc= */ "Floyd-Steinberg remap of the scene to 32 colors",
    /* .unit= */ "px",
    /* .per_loop= */ (double)WIDTH * HEIGHT,
    /* .setup= */ floyd_setup,
    /* .run= */ floyd_run,
    /* .verify= */ floyd_verify,
    /* .teardown= */ scene_teardown,
};
