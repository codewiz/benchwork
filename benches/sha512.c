// SPDX-License-Identifier: 0BSD
//
// SHA-512 and SHA-384 from AmiSSL, OpenSSL's sha512.c as AmiSSL builds it
// for AmigaOS 3: plain C, since OpenSSL has no m68k assembly for SHA-2, with
// B_ENDIAN and the fully unrolled block function. In TLS it hashes the
// handshake of SHA-384 cipher suites and checks SHA-384 certificate
// signatures; outside TLS it is a common file and package checksum.
//
// It is the one real-world benchmark here built on 64-bit integers: each
// round is 64-bit rotates, shifts, adds and logic, which an m68k compiler
// has to split into 32-bit halves and keep in registers. The input is part
// of the shared corpus; its content does not change the work.

#include <string.h>

#include "bench.h"
#include "corpus.h"
#include <openssl/sha.h>

// Each timed run hashes the first 64 KiB of the corpus twice, once with
// SHA-512 and once with SHA-384: about a second on a 25 MHz 68040.
#define PASSES 2
#define HASH_SIZE (64UL * 1024)

static UBYTE *corpus;

// FIPS 180-2's first SHA-512 example, checked once so that a compiler that
// breaks the digest fails setup instead of only changing the check value.
static const UBYTE ABC_SHA512[SHA512_DIGEST_LENGTH] = {
    0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba,
    0xcc, 0x41, 0x73, 0x49, 0xae, 0x20, 0x41, 0x31,
    0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9, 0x7e, 0xa2,
    0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a,
    0x21, 0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8,
    0x36, 0xba, 0x3c, 0x23, 0xa3, 0xfe, 0xeb, 0xbd,
    0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8, 0x0e,
    0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f,
};

// A run's digests, one per pass: SHA-512 on even passes, SHA-384 (the first
// 48 bytes) on odd ones.
static UBYTE digests[PASSES][SHA512_DIGEST_LENGTH];

static void *sha512_setup(void) {
    SHA512_CTX ctx;
    UBYTE md[SHA512_DIGEST_LENGTH];

    SHA512_Init(&ctx);
    SHA512_Update(&ctx, "abc", 3);
    SHA512_Final(md, &ctx);
    if (memcmp(md, ABC_SHA512, sizeof(md)) != 0)
        return NULL;

    corpus = corpus_alloc();
    return corpus;
}

static const void *sha512_run(void *state) {
    SHA512_CTX ctx;
    int i;

    for (i = 0; i < PASSES; i++) {
        if (i & 1) {
            SHA384_Init(&ctx);
            SHA384_Update(&ctx, state, HASH_SIZE);
            SHA384_Final(digests[i], &ctx);
        } else {
            SHA512_Init(&ctx);
            SHA512_Update(&ctx, state, HASH_SIZE);
            SHA512_Final(digests[i], &ctx);
        }
    }
    return digests;
}

static bool sha512_verify(void *state, const void *result, ULONG *check) {
    const UBYTE (*md)[SHA512_DIGEST_LENGTH] = result;
    ULONG sum = 0;
    int i;

    (void)state;
    for (i = 0; i < PASSES; i++)
        sum = checksum(sum, md[i],
                       i & 1 ? SHA384_DIGEST_LENGTH : SHA512_DIGEST_LENGTH);
    *check = sum;
    return true;
}

static void sha512_teardown(void *state) {
    (void)state;
    bench_free(corpus);
    corpus = NULL;
}

const struct Bench bench_sha512 = {
    "sha512",
    "AmiSSL SHA-512 and SHA-384 over the corpus",
    sha512_setup,
    sha512_run,
    sha512_verify,
    sha512_teardown,
};
