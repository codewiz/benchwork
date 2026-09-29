// SPDX-License-Identifier: 0BSD
//
// zlib deflate and inflate of the text corpus, at the default level 6 that
// libpng, gzip and most callers use. zlib is the compressor under every PNG
// and gzip on the Amiga, and its two halves stress different things:
// deflate is longest_match()'s byte compares and hash-chain walks plus the
// Huffman tree builder in trees.c, inflate is a bit-at-a-time state machine
// in inflate() with inflate_fast() as the hot decoding loop.
//
// Separate from the PNG benchmark so a change in zlib is measured on its
// own: a 256 KiB stream in one deflate() call, and inflated back in one
// inflate() call, the way compress2() and uncompress() do it.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>
#include <zlib.h>

#include "bench.h"
#include "corpus.h"

static UBYTE *plain, *packed, *unpacked;
static ULONG packed_cap, packed_len;

// Compress the whole corpus in one call. Returns false on a zlib error.
static bool deflate_corpus(void) {
    z_stream strm;
    int rc;

    memset(&strm, 0, sizeof(strm));
    if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK)
        return false;
    strm.next_in = plain;
    strm.avail_in = CORPUS_SIZE;
    strm.next_out = packed;
    strm.avail_out = packed_cap;
    rc = deflate(&strm, Z_FINISH);
    packed_len = strm.total_out;
    deflateEnd(&strm);
    return rc == Z_STREAM_END;
}

static bool zlib_setup(void) {
    plain = corpus_alloc();
    packed_cap = (ULONG)deflateBound(NULL, CORPUS_SIZE);
    packed = bench_alloc(packed_cap, "zlib deflate buffer");
    unpacked = bench_alloc(CORPUS_SIZE, "zlib inflate buffer");
    if (!plain || !packed || !unpacked)
        return false;
    return deflate_corpus();
}

static void zlib_teardown(void) {
    FreeVec(plain);
    FreeVec(packed);
    FreeVec(unpacked);
    plain = packed = unpacked = NULL;
}

static bool deflate_run(ULONG *check) {
    if (!deflate_corpus())
        return false;
    *check = checksum(0, packed, packed_len);
    return true;
}

static bool inflate_run(ULONG *check) {
    z_stream strm;
    int rc;

    memset(&strm, 0, sizeof(strm));
    if (inflateInit(&strm) != Z_OK)
        return false;
    strm.next_in = packed;
    strm.avail_in = packed_len;
    strm.next_out = unpacked;
    strm.avail_out = CORPUS_SIZE;
    rc = inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
    if (rc != Z_STREAM_END || strm.total_out != CORPUS_SIZE ||
        memcmp(unpacked, plain, CORPUS_SIZE))
        return false;
    *check = checksum(0, unpacked, CORPUS_SIZE);
    return true;
}

const struct bench bench_zlib_deflate = {
    "zlib-deflate",
    "zlib level 6 compression of 256 KiB of text",
    zlib_setup,
    deflate_run,
    zlib_teardown,
};

const struct bench bench_zlib_inflate = {
    "zlib-inflate",
    "zlib decompression back to 256 KiB",
    zlib_setup,
    inflate_run,
    zlib_teardown,
};
