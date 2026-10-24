// SPDX-License-Identifier: 0BSD
//
// Every benchmark, in the order a full run takes them.

#include "bench.h"

extern const struct Bench bench_dhry;
extern const struct Bench bench_scene_2d;
extern const struct Bench bench_quantize_floyd;
extern const struct Bench bench_lha_pack;
extern const struct Bench bench_lha_unpack;
extern const struct Bench bench_zlib_deflate;
extern const struct Bench bench_zlib_inflate;
extern const struct Bench bench_png_encode;
extern const struct Bench bench_png_decode;
extern const struct Bench bench_ftgrays;
extern const struct Bench bench_memcpy_small;
extern const struct Bench bench_memcpy_large;
extern const struct Bench bench_memcpy_var_small;
extern const struct Bench bench_memcpy_var_large;
extern const struct Bench bench_memmove_small;
extern const struct Bench bench_memmove_large;
extern const struct Bench bench_wipeout_tris;
#ifdef BENCH_HAVE_NFS3
extern const struct Bench bench_nfs3_project;
extern const struct Bench bench_nfs3_clip;
#endif
#ifdef BENCH_HAVE_AMISSL
extern const struct Bench bench_sha512;
extern const struct Bench bench_aesgcm;
extern const struct Bench bench_chacha20poly1305;
extern const struct Bench bench_x25519;
#endif

const struct Bench *const BENCHES[] = {
    &bench_dhry,
    &bench_scene_2d,
    &bench_quantize_floyd,
    &bench_lha_pack,
    &bench_lha_unpack,
    &bench_zlib_deflate,
    &bench_zlib_inflate,
    &bench_png_encode,
    &bench_png_decode,
    &bench_ftgrays,
    &bench_memcpy_small,
    &bench_memcpy_large,
    &bench_memcpy_var_small,
    &bench_memcpy_var_large,
    &bench_memmove_small,
    &bench_memmove_large,
    &bench_wipeout_tris,
#ifdef BENCH_HAVE_NFS3
    &bench_nfs3_project,
    &bench_nfs3_clip,
#endif
#ifdef BENCH_HAVE_AMISSL
    &bench_sha512,
    &bench_aesgcm,
    &bench_chacha20poly1305,
    &bench_x25519,
#endif
};

const size_t NBENCHES = sizeof(BENCHES) / sizeof(BENCHES[0]);
