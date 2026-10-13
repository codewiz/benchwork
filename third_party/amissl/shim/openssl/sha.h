/* Shim for the Benchwork build of sha512.c: the SHA-512 part of OpenSSL's
   include/openssl/sha.h, copied from OpenSSL 4.0.3 as AmiSSL ships it.
   Apache License 2.0, see ../../LICENSE.  */
#ifndef BENCHWORK_OPENSSL_SHA_H
#define BENCHWORK_OPENSSL_SHA_H

#include <stddef.h>

typedef unsigned int SHA_LONG;

#define SHA_LBLOCK 16

#define SHA256_192_DIGEST_LENGTH 24
#define SHA224_DIGEST_LENGTH 28
#define SHA256_DIGEST_LENGTH 32
#define SHA384_DIGEST_LENGTH 48
#define SHA512_DIGEST_LENGTH 64

#define SHA512_CBLOCK (SHA_LBLOCK * 8)

typedef unsigned long long SHA_LONG64;

typedef struct SHA512state_st {
    SHA_LONG64 h[8];
    SHA_LONG64 Nl, Nh;
    union {
        SHA_LONG64 d[SHA_LBLOCK];
        unsigned char p[SHA512_CBLOCK];
    } u;
    unsigned int num, md_len;
} SHA512_CTX;

int SHA384_Init(SHA512_CTX *c);
int SHA384_Update(SHA512_CTX *c, const void *data, size_t len);
int SHA384_Final(unsigned char *md, SHA512_CTX *c);
int SHA512_Init(SHA512_CTX *c);
int SHA512_Update(SHA512_CTX *c, const void *data, size_t len);
int SHA512_Final(unsigned char *md, SHA512_CTX *c);
void SHA512_Transform(SHA512_CTX *c, const unsigned char *data);

#endif
