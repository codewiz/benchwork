// SPDX-License-Identifier: 0BSD
//
// The shared compressible input: see corpus.c.

#ifndef BENCHWORK_CORPUS_H
#define BENCHWORK_CORPUS_H

#include <exec/types.h>

#define CORPUS_SIZE (256UL * 1024)

UBYTE *corpus_alloc(void);

#endif
