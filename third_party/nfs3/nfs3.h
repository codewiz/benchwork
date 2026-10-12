// SPDX-License-Identifier: 0BSD
//
// The two NFS3 kernels as benches/nfs3.c calls them: each runs ROUNDS
// rounds of COUNT items and returns arczi84's checksum of the results.

#ifndef BENCHWORK_NFS3_H
#define BENCHWORK_NFS3_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t nfs3_project(unsigned rounds, unsigned count);
uint32_t nfs3_clip(unsigned rounds, unsigned count);

#ifdef __cplusplus
}
#endif

#endif
