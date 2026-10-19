/* Shim for the Benchwork build of curve25519.c: the opaque types of
   OpenSSL's openssl/types.h that its Ed25519 code passes around. */
#ifndef BENCHWORK_OPENSSL_TYPES_H
#define BENCHWORK_OPENSSL_TYPES_H

typedef struct ossl_lib_ctx_st OSSL_LIB_CTX;
typedef struct engine_st ENGINE;
typedef struct evp_md_st EVP_MD;
typedef struct evp_md_ctx_st EVP_MD_CTX;

#endif
