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

| benchmark | 6.5.0b[^gcc6] | 16.2-rc26[^rc26] | vbcc 2026-01-12[^vbcc] | SAS/C 6.58[^sasc] |
|---|---:|---:|---:|---:|
| dhrystone | **414** | 433 (+4.71%) | 472 (+14.00%) | 573 (+38.51%) |
| scene-2d | 1437 | **1017 (-29.23%)** | 2213 (+54.07%) | 3154 (+119.52%) |
| quantize-floyd | 877 | **658 (-24.98%)** | 1239 (+41.34%) | 1095 (+24.88%) |
| lha-pack | 1094 | 1103 (+0.88%) | **1064 (-2.66%)** | 1233 (+12.75%) |
| lha-unpack | 305 | 297 (-2.37%) | **283 (-7.06%)** | 329 (+8.10%) |
| zlib-deflate | **698** | 803 (+15.10%) | 940 (+34.69%) | 894 (+28.16%) |
| zlib-inflate | 196 | 205 (+4.37%) | **186 (-5.09%)** | 213 (+8.95%) |
| png-encode | **1166** | 1291 (+10.67%) | 1814 (+55.59%) | 1682 (+44.20%) |
| png-decode | **919** | 979 (+6.45%) | 1281 (+39.27%) | 1243 (+35.23%) |
| ftgrays | **649** | 656 (+1.15%) | 785 (+21.08%) | 709 (+9.28%) |
| memcpy-small | 862 | **852 (-1.07%)** | 3525 (+309.02%) | 1790 (+107.75%) |
| memcpy-large | 286 | **285 (-0.45%)** | 1214 (+324.74%) | 451 (+57.88%) |
| memcpy-var-small | 546 | 515 (-5.67%) | 554 (+1.45%) | **374 (-31.53%)** |
| memcpy-var-large | 596 | **592 (-0.75%)** | 1557 (+161.11%) | 594 (-0.34%) |
| memmove-small | **906** | 909 (+0.42%) | 2132 (+135.37%) | 1530 (+68.98%) |
| memmove-large | 1028 | **957 (-6.92%)** | 8270 (+704.40%) | 5908 (+474.65%) |
| json-parse | 406 | 412 (+1.47%) | **223 (-45.13%)** | 765 (+88.44%) |
| json-print | 275 | 262 (-4.71%) | **201 (-27.00%)** | 327 (+18.99%) |
| wipeout-tris | 883 | **873 (-1.16%)** | 1808 (+104.79%) | 1342 (+51.95%) |
| nfs3-project[^cxx] | 1168 | **1058 (-9.42%)** | not built | not built |
| nfs3-clip[^cxx] | 1005 | **965 (-3.99%)** | not built | not built |
| sha512[^longlong] | **964** | 1042 (+8.12%) | 3822 (+296.54%) | not built |
| aesgcm[^longlong] | 1073 | **803 (-25.16%)** | 3102 (+189.21%) | not built |
| chacha20poly1305[^longlong] | **380** | 401 (+5.37%) | 982 (+158.09%) | not built |
| x25519[^longlong] | **539** | 547 (+1.55%) | failed | not built |
| **GEOMEAN** | **622** | **608 (-2.28%)** | **978 (+57.17%)** | **896 (+43.99%)** |

[^gcc6]: gcc 6.5.0b 20260819091705, with newlib, built from AmigaPorts'
    m68k-amigaos-gcc at f34b4e1, whose gcc 6 fixes include one for a
    miscompile of zlib's deflate. (Its `--version` says `AmigaDev
    v16.2-rc18-21-gf34b4e1`, the toolchain tree's own version.)
[^rc26]: `m68k-amigaos-gcc (AmigaDev 16.2-rc26) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^vbcc]: vbcc from Volker Barthelmann's source release of 2026-01-12, as
    16.2-rc20 ships it (it reports `vbcc V0.9i pre`), with rc18's vc, vc.lib,
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
