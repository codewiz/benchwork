// SPDX-License-Identifier: 0BSD
//
// FreeType's anti-aliased rasterizer (ftgrays.c, the "smooth" renderer)
// drawing a page of text: the printable ASCII glyphs of DejaVu Sans at three
// sizes, from outlines baked into glyphs.c so no font loader is needed.
//
// ftgrays.c is fixed-point integer code end to end: Bezier subdivision,
// per-cell area and coverage accumulation in a sparse cell table, then a
// sweep that turns cells into gray spans. It is full of small switch
// statements and short loops with several exits, which is the shape that
// GCC's jump threading and block duplication reshape most, and it has no
// division in the hot path, so it measures something the compressors do
// not.
//
// The file is built in FreeType's own STANDALONE_ mode, unmodified.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

// What ftgrays.c defines for itself in stand-alone mode before including
// ftimage.h; the header is shared with it here.
#define STANDALONE_
#define FT_BEGIN_HEADER
#define FT_END_HEADER
#define FT_STATIC_BYTE_CAST(type, var) (type)(unsigned char)(var)
#include "third_party/freetype/ftgrays.h"

#include "bench.h"
#include "glyphs.h"

#define PAGE_W 640
#define PAGE_H 480

// Pixel sizes to render at, as multiples of GLYPH_PIXELS / 4. Three passes
// over the alphabet at 12, 24 and 48 pixels: small glyphs are dominated by
// the per-glyph setup and the cell sweep, large ones by curve subdivision.
static const int SIZES[] = {12, 24, 48};

#define NSIZES (sizeof(SIZES) / sizeof(SIZES[0]))

// Passes over the page per timed iteration.
#define PASSES 2

static UBYTE *page;
static FT_Raster raster;
static FT_Vector *points;
static char *tags;
static short *contours;
static int max_points, max_contours;

// Render glyph g at `size` pixels with its origin at (pen_x, baseline) on
// the page. Returns false when the rasterizer reports an error.
static bool draw_glyph(const struct glyph *g, int size, int pen_x, int baseline) {
    FT_Outline outline;
    FT_Bitmap bitmap;
    FT_Raster_Params params;
    long xmin = 0, xmax = 0, ymin = 0, ymax = 0;
    int left, top, w, h, i;

    if (g->n_points == 0)
        return true;

    // Scale the master outline to this size. 26.6 in, 26.6 out.
    for (i = 0; i < g->n_points; i++) {
        long x = (long)GLYPH_POINTS[g->first_point + i][0] * size / GLYPH_PIXELS;
        long y = (long)GLYPH_POINTS[g->first_point + i][1] * size / GLYPH_PIXELS;

        if (i == 0 || x < xmin)
            xmin = x;
        if (i == 0 || x > xmax)
            xmax = x;
        if (i == 0 || y < ymin)
            ymin = y;
        if (i == 0 || y > ymax)
            ymax = y;
        points[i].x = x;
        points[i].y = y;
        tags[i] = GLYPH_TAGS[g->first_point + i];
    }
    for (i = 0; i < g->n_contours; i++)
        contours[i] = GLYPH_CONTOURS[g->first_contour + i];

    // The glyph's pixel box, and the outline moved so that it sits at the
    // box's origin: ftgrays renders into a bitmap whose bottom-left is
    // (0, 0), exactly as FT_Render_Glyph() does with a glyph-sized bitmap.
    left = (int)(xmin >> 6);
    top = (int)((ymax + 63) >> 6);
    w = (int)((xmax + 63) >> 6) - left;
    h = top - (int)(ymin >> 6);
    if (w <= 0 || h <= 0)
        return true;
    for (i = 0; i < g->n_points; i++) {
        points[i].x -= (long)left << 6;
        points[i].y -= (long)(top - h) << 6;
    }
    left += pen_x;
    top = baseline - top;
    if (left < 0 || top < 0 || left + w > PAGE_W || top + h > PAGE_H)
        return true;

    outline.n_contours = (short)g->n_contours;
    outline.n_points = (short)g->n_points;
    outline.points = points;
    outline.tags = tags;
    outline.contours = contours;
    outline.flags = FT_OUTLINE_NONE;

    // A window onto the page: the rasterizer sees only the glyph's box, but
    // writes straight into the page's rows.
    bitmap.rows = (unsigned int)h;
    bitmap.width = (unsigned int)w;
    bitmap.pitch = PAGE_W;
    bitmap.buffer = page + (long)top * PAGE_W + left;
    bitmap.num_grays = 256;
    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    bitmap.palette_mode = 0;
    bitmap.palette = NULL;

    memset(&params, 0, sizeof(params));
    params.target = &bitmap;
    params.source = &outline;
    params.flags = FT_RASTER_FLAG_AA;
    return ft_grays_raster.raster_render(raster, &params) == 0;
}

// Lay the alphabet out left to right at every size, wrapping lines,
// starting at the top of the page.
static bool draw_page(void) {
    int baseline = 0;
    size_t s;

    memset(page, 0, PAGE_W * PAGE_H);
    for (s = 0; s < NSIZES; s++) {
        int size = SIZES[s];
        int line = (GLYPH_LINE_HEIGHT * size / GLYPH_PIXELS + 63) >> 6;
        int pen_x = 0;
        int i;

        baseline += (GLYPH_ASCENT * size / GLYPH_PIXELS + 63) >> 6;
        for (i = 0; i < NGLYPHS; i++) {
            const struct glyph *g = &GLYPHS[i];
            int advance = (int)((g->advance * size / GLYPH_PIXELS + 63) >> 6);

            if (pen_x + advance > PAGE_W) {
                pen_x = 0;
                baseline += line;
            }
            if (!draw_glyph(g, size, pen_x, baseline))
                return false;
            pen_x += advance;
        }
        baseline += line;
    }
    return true;
}

static bool ftgrays_setup(void) {
    int i;

    for (i = 0; i < NGLYPHS; i++) {
        if (GLYPHS[i].n_points > max_points)
            max_points = GLYPHS[i].n_points;
        if (GLYPHS[i].n_contours > max_contours)
            max_contours = GLYPHS[i].n_contours;
    }
    page = bench_alloc(PAGE_W * PAGE_H, "ftgrays page");
    points = bench_alloc((ULONG)max_points * sizeof(*points), "ftgrays points");
    tags = bench_alloc((ULONG)max_points, "ftgrays tags");
    contours = bench_alloc((ULONG)max_contours * sizeof(*contours),
                           "ftgrays contours");
    if (!page || !points || !tags || !contours)
        return false;
    return ft_grays_raster.raster_new(NULL, &raster) == 0;
}

static void ftgrays_teardown(void) {
    if (raster)
        ft_grays_raster.raster_done(raster);
    bench_free(page);
    bench_free(points);
    bench_free(tags);
    bench_free(contours);
    page = NULL;
    points = NULL;
    tags = NULL;
    contours = NULL;
    raster = NULL;
}

static bool ftgrays_run(ULONG *check) {
    int pass;

    for (pass = 0; pass < PASSES; pass++)
        if (!draw_page())
            return false;
    *check = checksum(0, page, PAGE_W * PAGE_H);
    return true;
}

const struct bench bench_ftgrays = {
    "ftgrays",
    "FreeType smooth rasterizer, 94 glyphs at 12/24/48 px, 2 passes",
    ftgrays_setup,
    ftgrays_run,
    ftgrays_teardown
};
