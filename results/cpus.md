# Comparing GCC code generation per CPU

Part of the [Benchwork results](README.md); see there for how they are
measured.

## 68000, 68020, 68020-60-FP, 68040-FP

The release binaries that run on a 68040, built with
m68k-amigaos-gcc 16.2-rc26[^rc26] by `make release`, all run on the same
emulated 68040. The columns therefore show what the compiler gains from
being allowed each CPU's instructions, not how fast those CPUs are.
68020-60-FP is benchwork-020-060fp, built with `-m68020-60 -m68881`, which
runs on any 68020 or later with an FPU.

`-m68020-60` cannot use the 32x32->64 `muls.l` and `mulu.l`, which the
68060 lacks, so chacha20poly1305 and x25519 build each such multiply from
16-bit `mulu.w`: 897 of them in curve25519.o, against 226 `muls.l` for the
68040. Up to rc18 they were calls to `__muldi3`; rc19 inlines them
(AmigaPorts/gcc#112).

| benchmark | 68000 | 68020 | 68020-60-FP | 68040-FP |
|---|---:|---:|---:|---:|
| dhrystone | 462 | 434 (-6.06%) | 432 (-6.58%) | 433 (-6.23%) |
| scene-2d | 6709 | 1041 (-84.48%) | 1121 (-83.29%) | 1017 (-84.85%) |
| quantize-floyd | 910 | 658 (-27.77%) | 678 (-25.58%) | 658 (-27.77%) |
| lha-pack | 1230 | 1129 (-8.17%) | 1089 (-11.47%) | 1103 (-10.28%) |
| lha-unpack | 309 | 300 (-2.84%) | 284 (-8.12%) | 297 (-3.77%) |
| zlib-deflate | 840 | 810 (-3.63%) | 797 (-5.20%) | 803 (-4.44%) |
| zlib-inflate | 212 | 211 (-0.21%) | 205 (-3.10%) | 205 (-3.39%) |
| png-encode | 1315 | 1263 (-3.93%) | 1279 (-2.69%) | 1291 (-1.84%) |
| png-decode | 1084 | 982 (-9.40%) | 979 (-9.66%) | 979 (-9.69%) |
| ftgrays | 1229 | 662 (-46.15%) | 659 (-46.37%) | 656 (-46.59%) |
| memcpy-small | 2436 | 852 (-65.01%) | 852 (-65.01%) | 852 (-65.01%) |
| memcpy-large | 688 | 285 (-58.64%) | 285 (-58.64%) | 285 (-58.64%) |
| memcpy-var-small | 556 | 523 (-5.96%) | 515 (-7.38%) | 515 (-7.38%) |
| memcpy-var-large | 598 | 593 (-0.84%) | 592 (-1.04%) | 592 (-1.04%) |
| memmove-small | 950 | 909 (-4.31%) | 909 (-4.31%) | 909 (-4.31%) |
| memmove-large | 968 | 957 (-1.11%) | 957 (-1.11%) | 957 (-1.11%) |
| json-parse | 426 | 423 (-0.90%) | 411 (-3.69%) | 412 (-3.36%) |
| json-print | 445 | 298 (-33.02%) | 261 (-41.29%) | 262 (-41.13%) |
| wipeout-tris[^softfloat] | 2315 | 2305 (-0.43%) | 912 (-60.63%) | 873 (-62.31%) |
| nfs3-project[^softfloat] | 3302 | 3246 (-1.70%) | 1068 (-67.66%) | 1058 (-67.96%) |
| nfs3-clip[^softfloat] | 2745 | 2654 (-3.33%) | 989 (-63.99%) | 965 (-64.83%) |
| sha512 | 1089 | 1042 (-4.27%) | 1042 (-4.29%) | 1042 (-4.29%) |
| aesgcm | 953 | 824 (-13.47%) | 813 (-14.66%) | 803 (-15.73%) |
| chacha20poly1305 | 1610 | 390 (-75.77%) | 652 (-59.51%) | 401 (-75.11%) |
| x25519 | 5140 | 546 (-89.39%) | 1746 (-66.03%) | 547 (-89.36%) |
| **GEOMEAN** | **1073** | **733 (-31.71%)** | **688 (-35.88%)** | **641 (-40.30%)** |
| **CODE SIZE** | 467832 | 456688 (-2.38%) | 457956 (-2.11%) | 443152 (-5.28%) |

## On a 68060

There is no point in building with `-m68060`: it generates practically
the same code as `-m68020-60`, which also runs on every other 68020+. Emulated 50 MHz 68060 with its
caches modeled[^cache], built by rc26[^rc26] with
`volamos --clock-mhz 50 --cpu 68060 --fpu --cache --fpsp-emulate`:

| benchmark | -m68020-60 -m68881 | -m68060 -m68881 |
|---|---:|---:|
| dhrystone | **169** | 172 (+1.65%) |
| scene-2d | **418** | **418** |
| quantize-floyd | **275** | **275** |
| lha-pack | 438 | **435 (-0.68%)** |
| lha-unpack | **118** | **118** |
| zlib-deflate | **297** | **297** |
| zlib-inflate | **77** | **77** |
| png-encode | 394 | **393 (-0.01%)** |
| png-decode | **292** | **292 (+0.05%)** |
| ftgrays | **270** | **270 (-0.08%)** |
| memcpy-small | **435** | **435** |
| memcpy-large | **123** | **123** |
| memcpy-var-small | **219** | **219** |
| memcpy-var-large | **176** | **176** |
| memmove-small | **239** | **239** |
| memmove-large | **183** | **183** |
| json-parse | **170** | 172 (+0.94%) |
| json-print | **112** | **112 (-0.25%)** |
| wipeout-tris | 393 | **384 (-2.28%)** |
| nfs3-project | **447** | **447** |
| nfs3-clip | **413** | **413** |
| sha512 | **503** | **503** |
| aesgcm | **302** | **302** |
| chacha20poly1305 | **186** | **186** |
| x25519 | **508** | **508** |
| **GEOMEAN** | **254** | **253 (-0.03%)** |
| **CODE SIZE** | 457956 | 457072 (-0.19%) |

[^rc26]: `m68k-amigaos-gcc (AmigaDev 16.2-rc26) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^cache]: volamos `--cache`: a direct-mapped instruction cache, so a miss
    costs a memory fetch. It does not model the data cache or bus wait
    states.
[^softfloat]: The 68000 and 68020 builds are soft float, and newlib's
    float arithmetic calls the mathieee libraries: every float operation of
    wipeout-tris and most of the work of the NFS3 kernels. volamos runs its
    own native versions of those at zero cycles, so these runs load the
    68k code of AmigaOS 3.2's instead (mathieeesingbas 47.1,
    mathieeedoubbas 47.3, mathieeesingtrans 45.11, mathieeedoubtrans 47.2,
    with `tools/regen-results.sh --libs-dir`). With the native ones, these
    three benchmarks measured 27% to 33% faster on the 68000.
