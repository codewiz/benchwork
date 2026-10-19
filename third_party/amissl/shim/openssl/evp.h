/* Shim for the Benchwork build of curve25519.c: the SHA-512 digest calls of
   OpenSSL's openssl/evp.h, which only its Ed25519 code makes. The benchmark
   runs X25519 alone, so these fail instead of hashing: fetching SHA-512
   returns NULL, and Ed25519 gives up right there. */
#ifndef BENCHWORK_OPENSSL_EVP_H
#define BENCHWORK_OPENSSL_EVP_H

#include <stddef.h>
#include <openssl/types.h>

#define SN_sha512 "SHA512"

static inline EVP_MD *EVP_MD_fetch(OSSL_LIB_CTX *ctx, const char *algorithm,
                                   const char *properties)
{
    (void)ctx;
    (void)algorithm;
    (void)properties;
    return NULL;
}

static inline void EVP_MD_free(EVP_MD *md)
{
    (void)md;
}

static inline EVP_MD_CTX *EVP_MD_CTX_new(void)
{
    return NULL;
}

static inline void EVP_MD_CTX_free(EVP_MD_CTX *ctx)
{
    (void)ctx;
}

static inline int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type,
                                    ENGINE *impl)
{
    (void)ctx;
    (void)type;
    (void)impl;
    return 0;
}

static inline int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *d, size_t cnt)
{
    (void)ctx;
    (void)d;
    (void)cnt;
    return 0;
}

static inline int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md,
                                     unsigned int *s)
{
    (void)ctx;
    (void)md;
    (void)s;
    return 0;
}

static inline int EVP_Digest(const void *data, size_t count, unsigned char *md,
                             unsigned int *size, const EVP_MD *type,
                             ENGINE *impl)
{
    (void)data;
    (void)count;
    (void)md;
    (void)size;
    (void)type;
    (void)impl;
    return 0;
}

#endif
