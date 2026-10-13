// SPDX-License-Identifier: 0BSD
//
// AES-128-GCM from AmiSSL, OpenSSL's aes_core.c and gcm128.c as AmiSSL
// builds them for AmigaOS 3: plain C, since OpenSSL has no m68k assembly for
// either. TLS_AES_128_GCM_SHA256 is the cipher suite every TLS 1.3
// implementation must support, and the one most connections use: this is
// the work behind every record of an https download.
//
// Each record goes the way TLS 1.3 sends it: a nonce from the record
// number, the 5-byte record header as additional data, a full 16 KiB of
// payload and the tag; then the receiving side decrypts and checks it. AES
// is 32-bit table lookups; the GHASH that authenticates it is 64-bit
// shifts and XORs, which an m68k compiler splits into 32-bit halves.

#include <string.h>

#include "bench.h"
#include "corpus.h"
#include <openssl/aes.h>
#include <crypto/modes.h>

// Each timed run sends and receives 64 KiB of the corpus as four records.
#define RECORD_SIZE (16UL * 1024)
#define RECORDS 4
#define TAG_SIZE 16

// All three buffers are allocated, so they are longword aligned and gcm128.c
// takes its word-at-a-time path, as it does on AmiSSL's record buffers.
static UBYTE *corpus;
static UBYTE *cipher;
static UBYTE *plain;
static AES_KEY key;
static GCM128_CONTEXT gcm;

// Test case 4 of McGrew and Viega's GCM specification: AES-128 with
// additional data and a plaintext that ends in a partial block. Checked
// once so that a compiler that breaks the cipher fails setup instead of
// only changing the check value.
static const UBYTE TC4_KEY[16] = {
    0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
    0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
};
static const UBYTE TC4_IV[12] = {
    0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
    0xde, 0xca, 0xf8, 0x88,
};
static const UBYTE TC4_AAD[20] = {
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
    0xab, 0xad, 0xda, 0xd2,
};
static const UBYTE TC4_PLAIN[60] = {
    0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
    0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
    0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
    0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
    0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
    0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
    0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
    0xba, 0x63, 0x7b, 0x39,
};
static const UBYTE TC4_CIPHER[60] = {
    0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
    0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
    0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
    0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
    0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
    0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
    0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
    0x3d, 0x58, 0xe0, 0x91,
};
static const UBYTE TC4_TAG[TAG_SIZE] = {
    0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb,
    0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47,
};

static bool self_test(void) {
    UBYTE out[sizeof(TC4_PLAIN)];
    UBYTE tag[TAG_SIZE];

    AES_set_encrypt_key(TC4_KEY, 128, &key);
    CRYPTO_gcm128_init(&gcm, &key, (block128_f)AES_encrypt);
    CRYPTO_gcm128_setiv(&gcm, TC4_IV, sizeof(TC4_IV));
    if (CRYPTO_gcm128_aad(&gcm, TC4_AAD, sizeof(TC4_AAD)) != 0
        || CRYPTO_gcm128_encrypt(&gcm, TC4_PLAIN, out, sizeof(out)) != 0)
        return false;
    CRYPTO_gcm128_tag(&gcm, tag, TAG_SIZE);
    return memcmp(out, TC4_CIPHER, sizeof(out)) == 0
        && memcmp(tag, TC4_TAG, TAG_SIZE) == 0;
}

static bool aesgcm_setup(void) {
    if (!self_test())
        return false;

    // The connection's key and IV, from bytes of the corpus as a TLS key
    // schedule would derive them; the key schedule is outside the timing.
    corpus = corpus_alloc();
    cipher = bench_alloc(RECORDS * (RECORD_SIZE + TAG_SIZE), "aesgcm records");
    plain = bench_alloc(RECORD_SIZE, "aesgcm plaintext");
    if (corpus == NULL || cipher == NULL || plain == NULL)
        return false;
    AES_set_encrypt_key(corpus + RECORDS * RECORD_SIZE, 128, &key);
    CRYPTO_gcm128_init(&gcm, &key, (block128_f)AES_encrypt);
    return true;
}

// The per-record nonce: the IV with the record number XORed into its end.
static void set_nonce(ULONG record) {
    UBYTE nonce[12];
    int i;

    memcpy(nonce, corpus + RECORDS * RECORD_SIZE + 16, sizeof(nonce));
    for (i = 0; i < 4; i++)
        nonce[11 - i] ^= (UBYTE)(record >> (8 * i));
    CRYPTO_gcm128_setiv(&gcm, nonce, sizeof(nonce));
}

static bool aesgcm_run(ULONG *check) {
    // TLS 1.3's record header: application data, version 3.3, length.
    static const UBYTE header[5] = {
        0x17, 0x03, 0x03,
        (RECORD_SIZE + TAG_SIZE) >> 8, (RECORD_SIZE + TAG_SIZE) & 0xff,
    };
    ULONG r;

    for (r = 0; r < RECORDS; r++) {
        UBYTE *out = cipher + r * (RECORD_SIZE + TAG_SIZE);

        set_nonce(r);
        if (CRYPTO_gcm128_aad(&gcm, header, sizeof(header)) != 0
            || CRYPTO_gcm128_encrypt(&gcm, corpus + r * RECORD_SIZE, out,
                                     RECORD_SIZE) != 0)
            return false;
        CRYPTO_gcm128_tag(&gcm, out + RECORD_SIZE, TAG_SIZE);
    }

    // The receiver decrypts each record whole, as the TLS record layer
    // does, checks the tag, and compares with what was sent.
    for (r = 0; r < RECORDS; r++) {
        const UBYTE *in = cipher + r * (RECORD_SIZE + TAG_SIZE);

        set_nonce(r);
        if (CRYPTO_gcm128_aad(&gcm, header, sizeof(header)) != 0
            || CRYPTO_gcm128_decrypt(&gcm, in, plain, RECORD_SIZE) != 0
            || CRYPTO_gcm128_finish(&gcm, in + RECORD_SIZE, TAG_SIZE) != 0
            || memcmp(plain, corpus + r * RECORD_SIZE, RECORD_SIZE) != 0)
            return false;
    }

    *check = checksum(0, cipher, RECORDS * (RECORD_SIZE + TAG_SIZE));
    return true;
}

static void aesgcm_teardown(void) {
    bench_free(plain);
    bench_free(cipher);
    bench_free(corpus);
    plain = NULL;
    cipher = NULL;
    corpus = NULL;
}

const struct bench bench_aesgcm = {
    "aesgcm",
    "AmiSSL AES-128-GCM, sending and receiving TLS records",
    aesgcm_setup,
    aesgcm_run,
    aesgcm_teardown,
};
