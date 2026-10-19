/* Shim for the Benchwork build of curve25519.c: OpenSSL's internal
   include/crypto/ecx.h, reduced to the key lengths and the X25519 and
   Ed25519 prototypes. From OpenSSL 4.0.3, Apache License 2.0
   (../../LICENSE). */
#ifndef BENCHWORK_CRYPTO_ECX_H
#define BENCHWORK_CRYPTO_ECX_H

#include <stddef.h>
#include <stdint.h>
#include <openssl/crypto.h>
#include <openssl/types.h>

#define X25519_KEYLEN 32
#define ED25519_KEYLEN 32
#define ED25519_SIGSIZE 64

int ossl_x25519(uint8_t out_shared_key[32], const uint8_t private_key[32],
    const uint8_t peer_public_value[32]);
void ossl_x25519_public_from_private(uint8_t out_public_value[32],
    const uint8_t private_key[32]);

int ossl_ed25519_public_from_private(OSSL_LIB_CTX *ctx, uint8_t out_public_key[32],
    const uint8_t private_key[32],
    const char *propq);
int ossl_ed25519_sign(uint8_t *out_sig, const uint8_t *tbs, size_t tbs_len,
    const uint8_t public_key[32], const uint8_t private_key[32],
    const uint8_t dom2flag, const uint8_t phflag, const uint8_t csflag,
    const uint8_t *context, size_t context_len,
    OSSL_LIB_CTX *libctx, const char *propq);
int ossl_ed25519_verify(const uint8_t *tbs, size_t tbs_len,
    const uint8_t signature[64], const uint8_t public_key[32],
    const uint8_t dom2flag, const uint8_t phflag, const uint8_t csflag,
    const uint8_t *context, size_t context_len,
    OSSL_LIB_CTX *libctx, const char *propq);
int ossl_ed25519_pubkey_verify(const uint8_t *pub, size_t pub_len);

#endif
