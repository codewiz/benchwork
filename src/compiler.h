// SPDX-License-Identifier: 0BSD
//
// Wrappers for the compiler-specific constructs the sources use:
//
//   USED        keep a static variable nothing references (the $VER tag)
//   ALIGNED(n)  align a variable to n bytes
//   PACKED      lay a struct out without padding
//
// Add a branch for a compiler rather than defining the attributes away.

#ifndef BENCHWORK_COMPILER_H
#define BENCHWORK_COMPILER_H

#if defined(__GNUC__)
#define USED        __attribute__((used))
#define ALIGNED(n)  __attribute__((aligned(n)))
#define PACKED      __attribute__((packed))
#elif defined(__VBCC__)
// vbcc keeps unreferenced statics, has no alignment attribute, and pads
// structs only to two bytes, which the packed structs here never need.
#define USED
#define ALIGNED(n)
#define PACKED
#else
#error Add this compiler to src/compiler.h
#endif

#endif
