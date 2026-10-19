// SPDX-License-Identifier: 0BSD
//
// X25519 from AmiSSL, OpenSSL's curve25519.c as AmiSSL builds it for
// AmigaOS 3: the reference base 2^25.5 field arithmetic, ten 26- and 25-bit
// limbs in 32-bit words, since the m68k has no 128-bit integers. X25519 is
// the key exchange a TLS 1.3 client offers first.
//
// Each run is one handshake's key exchange, both ends of it: each side makes
// a key pair, then combines its private key with the peer's public key.
// Making the public key multiplies the base point through Ed25519's
// precomputed table; the shared secret runs the Montgomery ladder, 255 steps
// of field multiplies and squarings. Either way the time goes into fe_mul()
// and fe_sq(), signed 32x32->64 multiplies and the carries between limbs.
// One exchange is enough: a 68000 build takes ten times as long, every 64-bit
// product being a call into the C runtime.

#include <string.h>

#include "bench.h"
#include <crypto/ecx.h>

// RFC 7748 section 6.1: Alice's and Bob's keys and their shared secret.
// Checked once at setup, so that a compiler that breaks the arithmetic fails
// there instead of only changing the check value; the timed runs then repeat
// the same exchange.
static const UBYTE TV_ALICE_PRIV[32] = {
    0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d,
    0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45,
    0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a,
    0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a,
};
static const UBYTE TV_ALICE_PUB[32] = {
    0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54,
    0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a,
    0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4,
    0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a,
};
static const UBYTE TV_BOB_PRIV[32] = {
    0x5d, 0xab, 0x08, 0x7e, 0x62, 0x4a, 0x8a, 0x4b,
    0x79, 0xe1, 0x7f, 0x8b, 0x83, 0x80, 0x0e, 0xe6,
    0x6f, 0x3b, 0xb1, 0x29, 0x26, 0x18, 0xb6, 0xfd,
    0x1c, 0x2f, 0x8b, 0x27, 0xff, 0x88, 0xe0, 0xeb,
};
static const UBYTE TV_BOB_PUB[32] = {
    0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4,
    0xd3, 0x5b, 0x61, 0xc2, 0xec, 0xe4, 0x35, 0x37,
    0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78, 0x67, 0x4d,
    0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f,
};
static const UBYTE TV_SHARED[32] = {
    0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1,
    0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25,
    0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33,
    0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42,
};

static bool x25519_setup(void) {
    UBYTE pub[32];
    UBYTE shared[32];

    ossl_x25519_public_from_private(pub, TV_ALICE_PRIV);
    if (memcmp(pub, TV_ALICE_PUB, 32) != 0)
        return false;
    ossl_x25519_public_from_private(pub, TV_BOB_PRIV);
    if (memcmp(pub, TV_BOB_PUB, 32) != 0)
        return false;
    if (!ossl_x25519(shared, TV_ALICE_PRIV, TV_BOB_PUB))
        return false;
    return memcmp(shared, TV_SHARED, 32) == 0;
}

// Alice is the client and Bob the server.
static bool x25519_run(ULONG *check) {
    UBYTE client_pub[32], server_pub[32];
    UBYTE client_shared[32], server_shared[32];

    ossl_x25519_public_from_private(client_pub, TV_ALICE_PRIV);
    ossl_x25519_public_from_private(server_pub, TV_BOB_PRIV);
    if (!ossl_x25519(client_shared, TV_ALICE_PRIV, server_pub)
        || !ossl_x25519(server_shared, TV_BOB_PRIV, client_pub)
        || memcmp(client_shared, server_shared, 32) != 0)
        return false;

    *check = checksum(checksum(checksum(0, client_pub, 32), server_pub, 32),
                      client_shared, 32);
    return true;
}

const struct bench bench_x25519 = {
    "x25519",
    "AmiSSL X25519, both ends of a TLS key exchange",
    x25519_setup,
    x25519_run,
    NULL,
};
