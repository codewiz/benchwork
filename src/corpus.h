// SPDX-License-Identifier: 0BSD
//
// The shared compressible input: see corpus.c.

#ifndef BENCHWORK_CORPUS_H
#define BENCHWORK_CORPUS_H

#include <exec/types.h>

// Twice zlib's 32 KB window and eight -lh5- dictionaries: past the first
// window, deflate already runs at the rate it keeps on longer input.
#define CORPUS_SIZE (64UL * 1024)

// SIZE bytes of the corpus; a shorter one is the start of a longer one.
UBYTE *corpus_alloc(ULONG size);

#endif
