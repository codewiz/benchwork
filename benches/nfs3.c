// SPDX-License-Identifier: 0BSD
//
// Two floating-point kernels from arczi84's 68k port of Need for Speed III,
// both among the hottest functions of its race on a PiStorm:
//
//   nfs3-project  a model vertex through a 3x3 matrix and a translation,
//                 then the perspective divide, every result checked for
//                 overflow and underflow by testing its bits;
//   nfs3-clip     a triangle clipped against the near plane and the four
//                 screen edges, nine floats interpolated for each new
//                 vertex: short loops of compares and divides.
//
// The kernels are C++, in third_party/nfs3, so only the gcc build has them.
// They generate their own inputs from a fixed LCG and return a hash of the
// results; the FPU runs them in single rounding precision, as the game does.

#include "bench.h"
#include "third_party/nfs3/nfs3.h"

// Vertices and triangles per iteration: about a second each on a 25 MHz
// 68040. arczi84's harness runs rounds of 100000 and 20000; 5 of those give
// 449f5330 and 4352ff86 here, the sums of his x86-64 build, as the FPU in
// single precision rounds every result as SSE does.
#define VERTICES 50000
#define TRIANGLES 6000

static bool nfs3_setup(void) {
    return true;
}

static bool project_run(ULONG *check) {
    *check = nfs3_project(1, VERTICES);
    return true;
}

static bool clip_run(ULONG *check) {
    *check = nfs3_clip(1, TRIANGLES);
    return true;
}

const struct bench bench_nfs3_project = {
    "nfs3-project",
    "NFS3's vertex projection: float multiply-adds, a divide, bit tests",
    nfs3_setup,
    project_run,
    NULL,
};

const struct bench bench_nfs3_clip = {
    "nfs3-clip",
    "NFS3's triangle clipper: float compares, divides, interpolation",
    nfs3_setup,
    clip_run,
    NULL,
};
