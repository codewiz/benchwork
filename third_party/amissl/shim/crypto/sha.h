/* Shim for the Benchwork build of sha512.c: OpenSSL's internal
   crypto/sha.h, reduced to the declarations sha512.c defines without a
   prior prototype.  From OpenSSL 4.0.3, Apache License 2.0 (../../LICENSE). */
#include <openssl/sha.h>

int sha512_224_init(SHA512_CTX *);
int sha512_256_init(SHA512_CTX *);
