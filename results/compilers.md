# Comparing m68k compilers

Part of the [Benchwork results](README.md); see there for how they are
measured.

All runs done with benchwork-040: gcc with `-O2 -fomit-frame-pointer -m68040
-m68881`, vbcc and SAS/C with their full optimizers for the 68040 (see the
notes under the table). Each compiler links its own
C library, so the memcpy and memmove rows compare the compiler together with
its runtime.

## Speed relative to gcc 6.5.0b

Each bar is gcc 6.5.0b's time divided by the compiler's, so longer is faster.

![Speed of each benchmark relative to gcc 6.5.0b](compilers-040.svg)

Same data in tabular form (time in milliseconds):

| benchmark | 6.5.0b[^gcc6] | 16.2-rc18[^rc18] | vbcc 2026-01-12[^vbcc] | SAS/C 6.58[^sasc] |
|---|---:|---:|---:|---:|
| dhrystone | **414** | **414 (+0.07%)** | 472 (+14.00%) | 573 (+38.51%) |
| scene-2d | 1437 | **1032 (-28.15%)** | 2213 (+54.07%) | 3154 (+119.52%) |
| quantize-floyd | 877 | **659 (-24.84%)** | 1239 (+41.34%) | 1095 (+24.88%) |
| lha-pack | 1094 | 1110 (+1.46%) | **1064 (-2.66%)** | 1233 (+12.75%) |
| lha-unpack | 305 | 301 (-1.08%) | **283 (-7.06%)** | 329 (+8.10%) |
| zlib-deflate | **698** | 804 (+15.30%) | 941 (+34.88%) | 894 (+28.16%) |
| zlib-inflate | 196 | 237 (+20.79%) | **186 (-5.09%)** | 213 (+8.94%) |
| png-encode | **1166** | 1324 (+13.58%) | 1814 (+55.60%) | 1682 (+44.21%) |
| png-decode | **920** | 985 (+7.16%) | 1280 (+39.25%) | 1243 (+35.21%) |
| ftgrays | **649** | 662 (+2.05%) | 785 (+21.08%) | 709 (+9.28%) |
| memcpy-small | **862** | 908 (+5.35%) | 3672 (+326.13%) | 1790 (+107.75%) |
| memcpy-large | 286 | **285 (-0.45%)** | 1215 (+324.93%) | 451 (+57.88%) |
| memcpy-var-small | 546 | 523 (-4.22%) | 554 (+1.45%) | **374 (-31.53%)** |
| memcpy-var-large | 596 | **593 (-0.56%)** | 1557 (+161.11%) | 594 (-0.34%) |
| memmove-small | **906** | 939 (+3.68%) | 2132 (+135.37%) | 1530 (+68.98%) |
| memmove-large | 1028 | **959 (-6.70%)** | 8270 (+704.40%) | 5908 (+474.65%) |
| wipeout-tris | **883** | 893 (+1.16%) | 1808 (+104.79%) | 1342 (+51.95%) |
| nfs3-project[^cxx] | 1168 | **1064 (-8.90%)** | not built | not built |
| nfs3-clip[^cxx] | 1005 | **952 (-5.33%)** | not built | not built |
| sha512[^longlong] | 964 | **960 (-0.41%)** | 3808 (+295.18%) | not built |
| aesgcm[^longlong] | 1073 | **853 (-20.50%)** | 3102 (+189.21%) | not built |
| chacha20poly1305[^longlong] | **380** | 399 (+5.02%) | 982 (+158.09%) | not built |
| x25519[^longlong] | **539** | 547 (+1.53%) | failed | not built |
| **GEOMEAN** | **669** | **666 (-0.51%)** | **1174 (+75.36%)** | **959 (+43.33%)** |

[^gcc6]: gcc 6.5.0b 20260819091705, with newlib, built from AmigaPorts'
    m68k-amigaos-gcc at f34b4e1, whose gcc 6 fixes include one for a
    miscompile of zlib's deflate. (Its `--version` says `AmigaDev
    v16.2-rc18-21-gf34b4e1`, the toolchain tree's own version.)
[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^vbcc]: vbcc built from Volker Barthelmann's source release of
    2026-01-12, which no AmigaDev toolchain ships yet (it reports `vbcc
    V0.9i pre`), with rc18's vc, vc.lib,
    m040.lib and amiga.lib: `make vbcc-release`, that is `vc +aos68k -O3
    -cpu=68040 -fpu=68040`. Only `-O3` inlines, unrolls and optimizes
    across functions. Wipeout's kernel.c is built at `-O2`, as `-O3` crashes
    vbcc on it. x25519 fails its self-test (miscompiled from `-O1` up).
[^sasc]: SAS/C 6.58, `make sasc-release`: `CPU=68040 MATH=68881` with
    `OPTIMIZE OPTIMIZERTIME OPTIMIZERINLINELOCAL OPTIMIZERSCHEDULER` and the
    optimizer depths at 8, `PARAMETERS=REGISTERS CODE=FAR DATA=FAR`, linked
    with scm040.lib, sc.lib and amiga.lib.
[^cxx]: The NFS3 kernels are C++, which of these compilers only gcc builds.
    The geomean of this table leaves them out, so that it covers the same
    benchmarks for every compiler; the other tables include them.
[^longlong]: SHA-512, GHASH, Poly1305 and X25519 are built on 64-bit
    integers, and SAS/C 6.58 has no `long long`. The geomean of this table
    leaves them out as it does the NFS3 kernels.
