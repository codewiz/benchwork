/* Shim for the Benchwork build of curve25519.c: OpenSSL's internal/numbers.h,
   whose INT128_MAX would pick the base 2^51 code. The m68k has no 128-bit
   integers, so it stays undefined and the reference base 2^25.5 code
   builds, as it does for AmiSSL. */
#include <stdint.h>
