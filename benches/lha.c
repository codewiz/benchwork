// SPDX-License-Identifier: 0BSD
//
// LHa -lh5- compression and decompression of the text corpus, the workload
// every Amiga archive went through. The compressor is LHa for UNIX's
// slide.c (hash-chained sliding dictionary) and huf.c (static Huffman
// blocks), unmodified; only the stream I/O is redirected to memory buffers,
// see third_party/lha/lha.h.
//
// -lh5- (8 KB dictionary) rather than -lh6- or -lh7- because it is what the
// Amiga LhA produced for most of its life, and because the encoder's
// search_dict() spends its time in the same hash-chain walk either way.
//
// The compressor is old-style C: K&R definitions, short and unsigned short
// arithmetic everywhere, globals rather than a context struct. That is
// exactly why it belongs here: a lot of surviving Amiga code looks like it,
// and a compiler that only shines on modern C misses it.

#include <proto/exec.h>
#include <exec/memory.h>
#include <string.h>

#include "bench.h"
#include "corpus.h"
#include "third_party/lha/lha.h"

// LHa's own state is in globals, so ours stays in globals too.
static UBYTE *plain;
static struct lha_stream input, packed, unpacked;
static unsigned int plain_crc;

// What a run produced: the stream it wrote and the CRC the format carries.
struct lha_result {
    const struct lha_stream *out;
    unsigned int crc;
};

static struct lha_result lha_out;

// Compress the corpus once so the unpack benchmark has its input. Also
// what the pack benchmark does per iteration.
static bool pack(unsigned int *crc) {
    struct interfacing iface;

    input.data = plain;
    input.len = CORPUS_SIZE;
    input.pos = 0;
    packed.len = 0;

    memset(&iface, 0, sizeof(iface));
    iface.infile = &input;
    iface.outfile = &packed;
    iface.original = CORPUS_SIZE;
    iface.method = LZHUFF5_METHOD_NUM;
    encode_alloc(iface.method);
    *crc = encode(&iface);
    return !unpackable;
}

static void lha_teardown(void *state) {
    (void)state;
    bench_free(plain);
    bench_free(packed.data);
    bench_free(unpacked.data);
    plain = packed.data = unpacked.data = NULL;
}

static void *lha_setup(void) {
    plain = corpus_alloc(CORPUS_SIZE);
    // The corpus compresses about 4:1, so this never fills; the encoder
    // reports unpackable data rather than overflowing if it did.
    packed.cap = CORPUS_SIZE;
    packed.data = bench_alloc(packed.cap, "lha pack buffer");
    unpacked.cap = CORPUS_SIZE;
    unpacked.data = bench_alloc(unpacked.cap, "lha unpack buffer");
    if (!plain || !packed.data || !unpacked.data) {
        lha_teardown(NULL);
        return NULL;
    }
    make_crctable();
    if (!pack(&plain_crc)) {
        lha_teardown(NULL);
        return NULL;
    }
    return &lha_out;
}

static const void *pack_run(void *state) {
    struct lha_result *r = state;

    if (!pack(&r->crc))
        return NULL;
    r->out = &packed;
    return r;
}

static const void *unpack_run(void *state) {
    struct lha_result *r = state;
    struct interfacing iface;

    packed.pos = 0;
    unpacked.len = 0;

    memset(&iface, 0, sizeof(iface));
    iface.infile = &packed;
    iface.outfile = &unpacked;
    iface.original = CORPUS_SIZE;
    iface.packed = packed.len;
    iface.method = LZHUFF5_METHOD_NUM;
    iface.dicbit = LZHUFF5_DICBIT;
    r->crc = decode(&iface);
    r->out = &unpacked;
    return r;
}

static bool lha_verify(void *state, const void *result, ULONG *check) {
    const struct lha_result *r = result;

    (void)state;
    *check = checksum(r->crc, r->out->data, r->out->len);
    return true;
}

// Check the decompressed corpus byte for byte: the check value covers the
// output, so a decoder that produced the wrong bytes would show up even
// without the compare, but the compare makes a broken build fail loudly
// instead of merely reporting a strange checksum.
static bool unpack_verify(void *state, const void *result, ULONG *check) {
    const struct lha_result *r = result;

    if (r->crc != plain_crc || unpacked.len != CORPUS_SIZE ||
        memcmp(unpacked.data, plain, CORPUS_SIZE))
        return false;
    return lha_verify(state, result, check);
}

const struct Bench bench_lha_pack = {
    /* .name= */ "lha-pack",
    /* .desc= */ "LHa -lh5- compression of 64 KB of text",
    /* .unit= */ "KB",
    /* .per_loop= */ CORPUS_SIZE / 1024.0,
    /* .setup= */ lha_setup,
    /* .run= */ pack_run,
    /* .verify= */ lha_verify,
    /* .teardown= */ lha_teardown,
};

const struct Bench bench_lha_unpack = {
    /* .name= */ "lha-unpack",
    /* .desc= */ "LHa -lh5- decompression back to 64 KB",
    /* .unit= */ "KB",
    /* .per_loop= */ CORPUS_SIZE / 1024.0,
    /* .setup= */ lha_setup,
    /* .run= */ unpack_run,
    /* .verify= */ unpack_verify,
    /* .teardown= */ lha_teardown,
};
