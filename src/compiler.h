// SPDX-License-Identifier: 0BSD
//
// Wrappers for the compiler-specific constructs the sources use:
//
//   USED            keep a static variable nothing references (the $VER tag)
//   ALIGNED(n)      align a variable to n bytes
//   PACKED          lay a struct out without padding
//   HAVE_LONG_LONG  the compiler has a 64-bit integer type
//   bool            C99's, or an enum for compilers without <stdbool.h>
//
// Add a branch for a compiler rather than defining the attributes away.

#ifndef BENCHWORK_COMPILER_H
#define BENCHWORK_COMPILER_H

#if defined(__GNUC__)
    #define USED        __attribute__((used))
    #define ALIGNED(n)  __attribute__((aligned(n)))
    #define PACKED      __attribute__((packed))
    #define HAVE_LONG_LONG
    #include <stdbool.h>
#elif defined(__VBCC__)
    // vbcc keeps unreferenced statics, has no alignment attribute, and pads
    // structs only to two bytes, which the packed structs here never need.
    #define USED        /* unsupported */
    #define ALIGNED(n)  /* unsupported */
    #define PACKED      /* unsupported */
    #define HAVE_LONG_LONG
    #include <stdbool.h>
#elif defined(__SASC)
    // SAS/C 6.58 is C89: no long long, no <stdbool.h>, its own spelling of
    // inline, and no restrict. Its __aligned qualifies a variable, not a
    // struct type, and only to a longword, so it does not fit ALIGNED(n).
    #define USED        /* unsupported */
    #define ALIGNED(n)  /* unsupported */
    #define PACKED      /* unsupported */
    #define inline      __inline
    #define restrict    /* unsupported */
    typedef enum { false, true } bool;
#elif defined(_DCC)
    // DICE-nx: C89 without inline or restrict, and no <stdbool.h>. It
    // accepts long long but makes it 32 bits wide, so HAVE_LONG_LONG stays
    // undefined. Its __aligned, like SAS/C's, only longword-aligns a
    // variable.
    #define USED        /* unsupported */
    #define ALIGNED(n)  /* unsupported */
    #define PACKED      /* unsupported */
    #define inline      /* unsupported */
    #define restrict    /* unsupported */
    typedef enum { false, true } bool;
#else
    #error Add this compiler to src/compiler.h
#endif

#endif
