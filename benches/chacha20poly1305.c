// SPDX-License-Identifier: 0BSD
//
// ChaCha20-Poly1305 from AmiSSL, OpenSSL's chacha_enc.c and poly1305.c as
// AmiSSL builds them for AmigaOS 3: plain C, since OpenSSL has no m68k
// assembly for either. TLS_CHACHA20_POLY1305_SHA256 is the TLS 1.3 cipher
// suite a client without AES hardware prefers, which on the Amiga means
// every client.
//
// Each record goes the way TLS 1.3 sends it, as in the aesgcm benchmark: a
// nonce from the record number, the 5-byte record header as additional
// data, a full 16 KiB of payload and the tag; then the receiving side checks
// the tag and decrypts. ChaCha20 is 32-bit adds, XORs and rotates; Poly1305
// is 32x32->64 multiplies, sixteen per 16-byte block, with their carries.
//
// OpenSSL keeps the AEAD construction in its provider code, so the RFC 8439
// composition of the two primitives is done here.

#include <string.h>

#include "bench.h"
#include "corpus.h"
#include <crypto/chacha.h>
#include <crypto/poly1305.h>

// Each timed run sends and receives 64 KiB of the corpus as four records.
#define RECORD_SIZE (16UL * 1024)
#define RECORDS 4
#define TAG_SIZE 16

static UBYTE *corpus;
static UBYTE *cipher;
static UBYTE *plain;
static unsigned int key[8];
static UBYTE iv[12];

// Feed `len` bytes to Poly1305, then zeros up to a 16-byte boundary.
static void mac_padded(POLY1305 *mac, const UBYTE *data, ULONG len) {
    static const UBYTE zeros[15];

    Poly1305_Update(mac, data, len);
    if (len % 16)
        Poly1305_Update(mac, zeros, 16 - len % 16);
}

// RFC 8439 section 2.8: the tag over the additional data and the
// ciphertext, keyed with the first 32 bytes of the nonce's block 0.
static void aead_tag(const unsigned int k[8], const UBYTE nonce[12],
                     const UBYTE *aad, ULONG aad_len,
                     const UBYTE *text, ULONG len, UBYTE tag[TAG_SIZE]) {
    static const UBYTE zeros[POLY1305_KEY_SIZE];
    unsigned int counter[4];
    UBYTE mac_key[POLY1305_KEY_SIZE];
    UBYTE lengths[16];
    POLY1305 mac;
    int i;

    counter[0] = 0;
    for (i = 0; i < 3; i++)
        counter[i + 1] = CHACHA_U8TOU32(nonce + 4 * i);
    ChaCha20_ctr32(mac_key, zeros, sizeof(mac_key), k, counter);

    // Both lengths as 64-bit little-endian; neither reaches 2^32 here.
    memset(lengths, 0, sizeof(lengths));
    for (i = 0; i < 4; i++) {
        lengths[i] = (UBYTE)(aad_len >> (8 * i));
        lengths[8 + i] = (UBYTE)(len >> (8 * i));
    }

    Poly1305_Init(&mac, mac_key);
    mac_padded(&mac, aad, aad_len);
    mac_padded(&mac, text, len);
    Poly1305_Update(&mac, lengths, sizeof(lengths));
    Poly1305_Final(&mac, tag);
}

// The payload, from block 1 of the nonce on.
static void aead_crypt(const unsigned int k[8], const UBYTE nonce[12],
                       const UBYTE *in, UBYTE *out, ULONG len) {
    unsigned int counter[4];
    int i;

    counter[0] = 1;
    for (i = 0; i < 3; i++)
        counter[i + 1] = CHACHA_U8TOU32(nonce + 4 * i);
    ChaCha20_ctr32(out, in, len, k, counter);
}

static void load_key(unsigned int k[8], const UBYTE *bytes) {
    int i;

    for (i = 0; i < 8; i++)
        k[i] = CHACHA_U8TOU32(bytes + 4 * i);
}

// RFC 8439 section 2.8.2: the AEAD test vector, with additional data and a
// plaintext that ends in a partial block. Checked once so that a compiler
// that breaks the cipher fails setup instead of only changing the check
// value.
static const UBYTE TV_NONCE[12] = {
    0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43,
    0x44, 0x45, 0x46, 0x47,
};
static const UBYTE TV_AAD[12] = {
    0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7,
};
static const char TV_PLAIN[] =
    "Ladies and Gentlemen of the class of '99: If I could offer you only one "
    "tip for the future, sunscreen would be it.";
static const UBYTE TV_CIPHER[114] = {
    0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb,
    0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e, 0xc2,
    0xa4, 0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe,
    0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6,
    0x3d, 0xbe, 0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12,
    0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b,
    0x1a, 0x71, 0xde, 0x0a, 0x9e, 0x06, 0x0b, 0x29,
    0x05, 0xd6, 0xa5, 0xb6, 0x7e, 0xcd, 0x3b, 0x36,
    0x92, 0xdd, 0xbd, 0x7f, 0x2d, 0x77, 0x8b, 0x8c,
    0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58,
    0xfa, 0xb3, 0x24, 0xe4, 0xfa, 0xd6, 0x75, 0x94,
    0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc,
    0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b, 0x7a, 0x9d,
    0xe5, 0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b,
    0x61, 0x16,
};
static const UBYTE TV_TAG[TAG_SIZE] = {
    0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a,
    0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91,
};

static bool self_test(void) {
    unsigned int k[8];
    UBYTE key_bytes[32];
    UBYTE out[sizeof(TV_CIPHER)];
    UBYTE tag[TAG_SIZE];
    int i;

    // The test key is the bytes 0x80 to 0x9f.
    for (i = 0; i < 32; i++)
        key_bytes[i] = (UBYTE)(0x80 + i);
    load_key(k, key_bytes);
    aead_crypt(k, TV_NONCE, (const UBYTE *)TV_PLAIN, out, sizeof(out));
    aead_tag(k, TV_NONCE, TV_AAD, sizeof(TV_AAD), out, sizeof(out), tag);
    return memcmp(out, TV_CIPHER, sizeof(out)) == 0
        && memcmp(tag, TV_TAG, TAG_SIZE) == 0;
}

static bool chacha20poly1305_setup(void) {
    if (!self_test())
        return false;

    // The connection's key and IV, from bytes of the corpus as a TLS key
    // schedule would derive them; the key schedule is outside the timing.
    corpus = corpus_alloc();
    cipher = bench_alloc(RECORDS * (RECORD_SIZE + TAG_SIZE),
                         "chacha20poly1305 records");
    plain = bench_alloc(RECORD_SIZE, "chacha20poly1305 plaintext");
    if (corpus == NULL || cipher == NULL || plain == NULL)
        return false;
    load_key(key, corpus + RECORDS * RECORD_SIZE);
    memcpy(iv, corpus + RECORDS * RECORD_SIZE + 32, sizeof(iv));
    return true;
}

// The per-record nonce: the IV with the record number XORed into its end.
static void record_nonce(UBYTE nonce[12], ULONG record) {
    int i;

    memcpy(nonce, iv, 12);
    for (i = 0; i < 4; i++)
        nonce[11 - i] ^= (UBYTE)(record >> (8 * i));
}

static bool chacha20poly1305_run(ULONG *check) {
    // TLS 1.3's record header: application data, version 3.3, length.
    static const UBYTE header[5] = {
        0x17, 0x03, 0x03,
        (RECORD_SIZE + TAG_SIZE) >> 8, (RECORD_SIZE + TAG_SIZE) & 0xff,
    };
    UBYTE nonce[12];
    UBYTE tag[TAG_SIZE];
    ULONG r;

    for (r = 0; r < RECORDS; r++) {
        UBYTE *out = cipher + r * (RECORD_SIZE + TAG_SIZE);

        record_nonce(nonce, r);
        aead_crypt(key, nonce, corpus + r * RECORD_SIZE, out, RECORD_SIZE);
        aead_tag(key, nonce, header, sizeof(header), out, RECORD_SIZE,
                 out + RECORD_SIZE);
    }

    // The receiver checks each record's tag, decrypts it whole, as the TLS
    // record layer does, and compares with what was sent.
    for (r = 0; r < RECORDS; r++) {
        const UBYTE *in = cipher + r * (RECORD_SIZE + TAG_SIZE);

        record_nonce(nonce, r);
        aead_tag(key, nonce, header, sizeof(header), in, RECORD_SIZE, tag);
        if (memcmp(tag, in + RECORD_SIZE, TAG_SIZE) != 0)
            return false;
        aead_crypt(key, nonce, in, plain, RECORD_SIZE);
        if (memcmp(plain, corpus + r * RECORD_SIZE, RECORD_SIZE) != 0)
            return false;
    }

    *check = checksum(0, cipher, RECORDS * (RECORD_SIZE + TAG_SIZE));
    return true;
}

static void chacha20poly1305_teardown(void) {
    bench_free(plain);
    bench_free(cipher);
    bench_free(corpus);
    plain = NULL;
    cipher = NULL;
    corpus = NULL;
}

const struct bench bench_chacha20poly1305 = {
    "chacha20poly1305",
    "AmiSSL ChaCha20-Poly1305, sending and receiving TLS records",
    chacha20poly1305_setup,
    chacha20poly1305_run,
    chacha20poly1305_teardown,
};
