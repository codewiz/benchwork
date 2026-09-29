// SPDX-License-Identifier: 0BSD
//
// libpng encoding and decoding of the 640x480 backdrop scene, entirely in
// memory. This is the pngbench that the m68k-amigaos-gcc issue tracker
// measures the toolchain's libpng with, made self-contained: the picture is
// synthesized rather than loaded, and libpng and zlib are built here rather
// than taken from the installed toolchain, so two compilers can be compared
// on the same source.
//
// The encode side is dominated by libpng's per-row filter selection
// (png_write_find_filter() tries all five filters over every row) and then
// zlib deflate; the decode side by inflate and the filter reconstruction in
// pngrutil.c. Both are byte loops over pointers, which is where m68k
// addressing modes and auto-increment either happen or do not.

#include <proto/exec.h>
#include <exec/memory.h>
#include <png.h>
#include <string.h>

#include "bench.h"

struct buf {
    UBYTE *data;
    ULONG len;      // bytes in use
    ULONG capacity; // bytes allocated
    ULONG pos;      // read cursor
};

static UBYTE *pixels;
static SHORT width, height;
static struct buf encoded;
static UBYTE *decoded;
static png_bytep *rows;

static void mem_read(png_structp png, png_bytep out, size_t length) {
    struct buf *b = (struct buf *)png_get_io_ptr(png);

    if (b->pos + length > b->len)
        png_error(png, "short read");
    memcpy(out, b->data + b->pos, length);
    b->pos += length;
}

static void mem_write(png_structp png, png_bytep data, size_t length) {
    struct buf *b = (struct buf *)png_get_io_ptr(png);

    if (b->len + length > b->capacity)
        png_error(png, "encode buffer too small");
    memcpy(b->data + b->len, data, length);
    b->len += length;
}

static void mem_flush(png_structp png) {
    (void)png;
}

// Encode the scene into `encoded` with libpng's defaults: 8-bit RGB,
// adaptive filtering, zlib level 6. Returns false on a libpng error.
// libpng allocates its row buffers and zlib's state through these, so
// --fastmem covers them too. Like libpng's own default, they do not clear.
static png_voidp png_bench_malloc(png_structp png, png_alloc_size_t size) {
    (void)png;
    return bench_alloc((ULONG)size, "libpng buffer");
}

static void png_bench_free(png_structp png, png_voidp ptr) {
    (void)png;
    bench_free(ptr);
}

static bool encode(void) {
    png_structp png;
    png_infop info;

    encoded.len = 0;
    png = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL,
                                    NULL, png_bench_malloc, png_bench_free);
    if (!png)
        return false;
    info = png_create_info_struct(png);
    if (!info || setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, info ? &info : NULL);
        return false;
    }
    png_set_write_fn(png, &encoded, mem_write, mem_flush);
    png_set_IHDR(png, info, (png_uint_32)width, (png_uint_32)height, 8,
                 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    for (SHORT y = 0; y < height; y++)
        rows[y] = pixels + (ULONG)y * width * 3;
    png_write_info(png, info);
    png_write_image(png, rows);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    return true;
}

// Decode `encoded` into `decoded`. Returns false on a libpng error or if the
// image is not the 8-bit RGB it was encoded as.
static bool decode(void) {
    png_structp png;
    png_infop info;
    png_uint_32 w, h;
    int depth, color;

    encoded.pos = 0;
    png = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL,
                                   NULL, png_bench_malloc, png_bench_free);
    if (!png)
        return false;
    info = png_create_info_struct(png);
    if (!info || setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, info ? &info : NULL, NULL);
        return false;
    }
    png_set_read_fn(png, &encoded, mem_read);
    png_read_info(png, info);
    png_get_IHDR(png, info, &w, &h, &depth, &color, NULL, NULL, NULL);
    if (w != (png_uint_32)width || h != (png_uint_32)height || depth != 8 ||
        color != PNG_COLOR_TYPE_RGB)
        png_error(png, "unexpected image format");
    for (SHORT y = 0; y < height; y++)
        rows[y] = decoded + (ULONG)y * width * 3;
    png_read_image(png, rows);
    png_read_end(png, NULL);
    png_destroy_read_struct(&png, &info, NULL);
    return true;
}

static bool png_setup(void) {
    pixels = backdrop_rgb(&width, &height);
    if (!pixels)
        return false;
    // An encode is never bigger than the raw pixels plus libpng's per-row
    // filter byte and a zlib worst case, so this is generous but bounded.
    encoded.capacity = (ULONG)width * height * 3 + height + 65536;
    encoded.data = bench_alloc(encoded.capacity, "png encode buffer");
    decoded = bench_alloc((ULONG)width * height * 3, "png pixel buffer");
    rows = bench_alloc((ULONG)height * sizeof(png_bytep), "png row table");
    if (!encoded.data || !decoded || !rows)
        return false;
    return encode();
}

static void png_teardown(void) {
    bench_free(pixels);
    bench_free(encoded.data);
    bench_free(decoded);
    bench_free(rows);
    pixels = encoded.data = decoded = NULL;
    rows = NULL;
}

static bool encode_run(ULONG *check) {
    if (!encode())
        return false;
    *check = checksum(0, encoded.data, encoded.len);
    return true;
}

static bool decode_run(ULONG *check) {
    if (!decode())
        return false;
    if (memcmp(decoded, pixels, (ULONG)width * height * 3))
        return false;
    *check = checksum(0, decoded, (ULONG)width * height * 3);
    return true;
}

const struct bench bench_png_encode = {
    "png-encode",
    "libpng encode of the 640x480 scene",
    png_setup,
    encode_run,
    png_teardown,
};

const struct bench bench_png_decode = {
    "png-decode",
    "libpng decode of the same PNG",
    png_setup,
    decode_run,
    png_teardown,
};
