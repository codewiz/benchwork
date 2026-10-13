/* Shim for the Benchwork build of sha512.c: OPENSSL_store_u64_be() from
   OpenSSL's include/openssl/byteorder.h (4.0.3), in its portable form.
   Apache License 2.0, see ../../LICENSE.  */
#ifndef BENCHWORK_OPENSSL_BYTEORDER_H
#define BENCHWORK_OPENSSL_BYTEORDER_H

/* The real header gets uint64_t, which sha512.c casts to, from e_os2.h. */
#include <stdint.h>

static inline unsigned char *
OPENSSL_store_u64_be(unsigned char *out, unsigned long long val)
{
    *out++ = (val >> 56) & 0xff;
    *out++ = (val >> 48) & 0xff;
    *out++ = (val >> 40) & 0xff;
    *out++ = (val >> 32) & 0xff;
    *out++ = (val >> 24) & 0xff;
    *out++ = (val >> 16) & 0xff;
    *out++ = (val >> 8) & 0xff;
    *out++ = val & 0xff;
    return out;
}

#endif
