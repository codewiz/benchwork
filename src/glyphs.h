// SPDX-License-Identifier: 0BSD
//
// Glyph outlines for the rasterizer benchmark, generated into glyphs.c by
// tools/dumpglyphs.c. Coordinates are 26.6 fixed point at GLYPH_PIXELS.

#ifndef BENCHWORK_GLYPHS_H
#define BENCHWORK_GLYPHS_H

struct glyph {
    int first_point;   // index into GLYPH_POINTS and GLYPH_TAGS
    int n_points;
    int first_contour; // index into GLYPH_CONTOURS
    int n_contours;
    long advance;      // horizontal advance, 26.6
};

extern const short GLYPH_POINTS[][2];
extern const char GLYPH_TAGS[];
extern const short GLYPH_CONTOURS[];
extern const struct glyph GLYPHS[];
extern const int NGLYPHS;
extern const int GLYPH_PIXELS;
extern const int GLYPH_ASCENT;      // 26.6
extern const int GLYPH_LINE_HEIGHT; // 26.6

#endif
