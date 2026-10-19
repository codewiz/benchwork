/* Shim for the Benchwork build of the AmiSSL code: OpenSSL's openssl/crypto.h,
   reduced to what gcm128.c, poly1305.c and curve25519.c use: gcm128.c's
   context allocator, the constant-time compare and OPENSSL_cleanse().  The
   compare follows OpenSSL's portable C CRYPTO_memcmp() (4.0.3, Apache
   License 2.0, see ../../LICENSE). */
#ifndef BENCHWORK_OPENSSL_CRYPTO_H
#define BENCHWORK_OPENSSL_CRYPTO_H

#include <stdlib.h>
#include <string.h>

static inline void *OPENSSL_malloc(size_t num)
{
    return malloc(num);
}

static inline void OPENSSL_clear_free(void *str, size_t num)
{
    if (str != NULL) {
        memset(str, 0, num);
        free(str);
    }
}

static inline int CRYPTO_memcmp(const void *in_a, const void *in_b, size_t len)
{
    size_t i;
    const volatile unsigned char *a = in_a;
    const volatile unsigned char *b = in_b;
    unsigned char x = 0;

    for (i = 0; i < len; i++)
        x |= a[i] ^ b[i];

    return x;
}

/* Clears a context the way the caller asked; the volatile stores keep the
   compiler from dropping them as dead. */
static inline void OPENSSL_cleanse(void *ptr, size_t len)
{
    volatile unsigned char *p = ptr;

    while (len--)
        *p++ = 0;
}

#endif
