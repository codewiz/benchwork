// SPDX-License-Identifier: 0BSD
//
// LHa -lh5- compression and decompression of the text corpus, the workload
// every Amiga archive went through. The compressor is LHa for UNIX's
// slide.c (hash-chained sliding dictionary) and huf.c (static Huffman
// blocks), unmodified; only the stream I/O is redirected to memory buffers,
// see third_party/lha/lha.h.
//
// -lh5- (8 KiB dictionary) rather than -lh6- or -lh7- because it is what the
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
#include "../third_party/lha/lha.h"

static UBYTE *plain;
static struct lha_stream input, packed, unpacked;
static unsigned int plain_crc;

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

static bool lha_setup(void) {
    plain = corpus_alloc();
    if (!plain)
        return false;
    // The corpus compresses about 4:1, so this never fills; the encoder
    // reports unpackable data rather than overflowing if it did.
    packed.cap = CORPUS_SIZE;
    packed.data = AllocVec(packed.cap, MEMF_ANY);
    unpacked.cap = CORPUS_SIZE;
    unpacked.data = AllocVec(unpacked.cap, MEMF_ANY);
    if (!packed.data || !unpacked.data)
        return false;
    make_crctable();
    return pack(&plain_crc);
}

static void lha_teardown(void) {
    FreeVec(plain);
    FreeVec(packed.data);
    FreeVec(unpacked.data);
    plain = packed.data = unpacked.data = NULL;
}

static bool pack_run(ULONG *check) {
    unsigned int crc;

    if (!pack(&crc))
        return false;
    *check = checksum(crc, packed.data, packed.len);
    return true;
}

// Decompress the packed corpus and verify it byte for byte: the check
// value covers the output, so a decoder that produced the wrong bytes would
// show up even without the compare, but the compare makes a broken build
// fail loudly instead of merely reporting a strange checksum.
static bool unpack_run(ULONG *check) {
    struct interfacing iface;
    unsigned int crc;

    packed.pos = 0;
    unpacked.len = 0;

    memset(&iface, 0, sizeof(iface));
    iface.infile = &packed;
    iface.outfile = &unpacked;
    iface.original = CORPUS_SIZE;
    iface.packed = packed.len;
    iface.method = LZHUFF5_METHOD_NUM;
    iface.dicbit = LZHUFF5_DICBIT;
    crc = decode(&iface);
    if (crc != plain_crc || unpacked.len != CORPUS_SIZE ||
        memcmp(unpacked.data, plain, CORPUS_SIZE))
        return false;
    *check = checksum(crc, unpacked.data, unpacked.len);
    return true;
}

const struct bench bench_lha_pack = {
    "lha-pack",
    "LHa -lh5- compression of 256 KiB of text",
    lha_setup,
    pack_run,
    lha_teardown,
};

const struct bench bench_lha_unpack = {
    "lha-unpack",
    "LHa -lh5- decompression back to 256 KiB",
    lha_setup,
    unpack_run,
    lha_teardown,
};
