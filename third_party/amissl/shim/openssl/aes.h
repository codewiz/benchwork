/* Shim for the Benchwork build of aes_core.c: the key schedule and block
   functions of OpenSSL's include/openssl/aes.h, copied from OpenSSL 4.0.3
   as AmiSSL ships it.  Apache License 2.0, see ../../LICENSE.  */
#ifndef BENCHWORK_OPENSSL_AES_H
#define BENCHWORK_OPENSSL_AES_H

#include <stddef.h>

#define AES_BLOCK_SIZE 16

#define AES_ENCRYPT 1
#define AES_DECRYPT 0

#define AES_MAXNR 14

struct aes_key_st {
    unsigned int rd_key[4 * (AES_MAXNR + 1)];
    int rounds;
};
typedef struct aes_key_st AES_KEY;

int AES_set_encrypt_key(const unsigned char *userKey, int bits,
    AES_KEY *key);
int AES_set_decrypt_key(const unsigned char *userKey, int bits,
    AES_KEY *key);
void AES_encrypt(const unsigned char *in, unsigned char *out,
    const AES_KEY *key);
void AES_decrypt(const unsigned char *in, unsigned char *out,
    const AES_KEY *key);

#endif
