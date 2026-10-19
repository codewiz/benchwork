/* Shim for the Benchwork build of chacha_enc.c: OpenSSL's internal
   crypto/ctype.h, reduced to ossl_toascii(), which chacha_enc.c uses to spell
   the "expand 32-byte k" constant. The identity on an ASCII machine.
   From OpenSSL 4.0.3, Apache License 2.0 (../../LICENSE). */
#ifndef BENCHWORK_CRYPTO_CTYPE_H
#define BENCHWORK_CRYPTO_CTYPE_H

#define ossl_toascii(c) (c)

#endif
