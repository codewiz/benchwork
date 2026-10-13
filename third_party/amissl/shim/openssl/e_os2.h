/* Shim for the Benchwork build of aes_core.c: OpenSSL's openssl/e_os2.h,
   of which aes_local.h needs only the fixed-width integer types. */
#include <stdint.h>
