# Results

Fastest iteration in milliseconds under [volamos](https://github.com/sidick/volamos) 0.10.0.
The cycle count makes all iterations identical.
Percentages are against the first column.

## Comparing m68k compilers

All runs done with benchwork-040 built with `-O2 -fomit-frame-pointer -m68040
-m68881`, or the equivalent for vbcc and SAS/C. Each compiler links its own
C library, so the memcpy and memmove rows compare the compiler together with
its runtime.

### Time relative to gcc 6.5.0b

Each bar is the compiler's time divided by gcc 6.5.0b's, so shorter is faster.

![Time of each benchmark relative to gcc 6.5.0b](results/compilers-040.svg)

Same data in tabular form (time in milliseconds):

| benchmark | 6.5.0b[^gcc6] | 16.2-rc17[^rc17] | vbcc 0.9i[^vbcc] | SAS/C 6.58[^sasc] |
|---|---:|---:|---:|---:|
| dhrystone | **420** | 421 (+0.25%) | 599 (+42.49%) | 581 (+38.20%) |
| backdrop | 2891 | **1941 (-32.87%)** | 4238 (+46.57%) | 4267 (+47.57%) |
| lha-pack | **4377** | 4439 (+1.42%) | 4870 (+11.26%) | 5022 (+14.73%) |
| lha-unpack | 1385 | **1382 (-0.18%)** | 1457 (+5.21%) | 1507 (+8.80%) |
| zlib-deflate | **3172** | 3698 (+16.59%) | 4274 (+34.75%) | 4096 (+29.13%) |
| zlib-inflate | **447** | 511 (+14.21%) | 551 (+23.32%) | 492 (+10.05%) |
| png-encode | **6562** | 7543 (+14.95%) | 10221 (+55.76%) | 9483 (+44.51%) |
| png-decode | **1529** | 1587 (+3.82%) | 2357 (+54.21%) | 1985 (+29.87%) |
| ftgrays | **845** | 858 (+1.56%) | failed | 942 (+11.48%) |
| memcpy-small | **186** | 193 (+4.14%) | wrong output | 348 (+87.63%) |
| memcpy-large | 328 | **326 (-0.44%)** | 1267 (+286.43%) | 501 (+52.81%) |
| memcpy-var-small | 588 | 565 (-3.92%) | 654 (+11.34%) | **423 (-27.94%)** |
| memcpy-var-large | 638 | **635 (-0.52%)** | 1616 (+153.27%) | 644 (+0.92%) |
| memmove-small | **2247** | 2351 (+4.61%) | 4320 (+92.22%) | 3110 (+38.40%) |
| memmove-large | **4011** | 4127 (+2.89%) | 12458 (+210.62%) | 8911 (+122.20%) |
| wipeout-tris | **507** | 519 (+2.38%) | 943 (+86.08%) | 672 (+32.73%) |
| nfs3-project[^cxx] | 1168 | **1064 (-8.90%)** | not built | not built |
| nfs3-clip[^cxx] | 1005 | **951 (-5.40%)** | not built | not built |
| sha512[^longlong] | **964** | 1045 (+8.38%) | 3824 (+296.79%) | not built |
| aesgcm[^longlong] | 1121 | **903 (-19.42%)** | 3290 (+193.62%) | not built |
| **GEOMEAN** | **1149** | **1162 (+1.15%)** | | **1491 (+29.74%)** |

## Comparing GCC 16.2 releases

benchwork-040 built with `-O2 -fomit-frame-pointer -m68040 -m68881` by each
AmigaPorts release[^releases] since rc5, the oldest one still published:

- rc11: LRA by default, small constant-size copies inlined.
- rc14: Bebbo's code generation work (costs, dbra, inline block copies).
- rc17: GCC 16.2.1, 64-bit integer operations split into 32-bit ones.

All tests ran on an emulated 25MHz 68040, except where noted[^volamos].

The chart skips the releases that time within 1% of the one before them.
Its vertical axis is logarithmic, so equal slopes are equal percentages.

![Time of each benchmark in the 16.2 releases from rc5 to rc17](results/gcc-releases-040.svg)

| benchmark | rc5 | rc11 | rc14 | rc17 |
|---|---:|---:|---:|---:|
| dhrystone | 516 | 516 | **421 (-18.37%)** | **421 (-18.37%)** |
| backdrop | 2083 | 2082 (-0.05%) | **1941 (-6.81%)** | **1941 (-6.81%)** |
| lha-pack | **4108** | 4380 (+6.62%) | 4439 (+8.05%) | 4439 (+8.05%) |
| lha-unpack | **1330** | 1371 (+3.05%) | 1382 (+3.93%) | 1382 (+3.93%) |
| zlib-deflate | 3824 | 3822 (-0.05%) | **3698 (-3.28%)** | **3698 (-3.28%)** |
| zlib-inflate | **495** | 497 (+0.51%) | 511 (+3.21%) | 511 (+3.21%) |
| png-encode | 7913 | 7912 (-0.01%) | **7543 (-4.67%)** | **7543 (-4.67%)** |
| png-decode | 1743 | 1733 (-0.53%) | **1587 (-8.94%)** | **1587 (-8.94%)** |
| ftgrays | 860 | 862 (+0.20%) | **858 (-0.17%)** | **858 (-0.17%)** |
| memcpy-small | 514 | 476 (-7.33%) | **193 (-62.38%)** | **193 (-62.38%)** |
| memcpy-large | 505 | 505 | **326 (-35.38%)** | **326 (-35.38%)** |
| memcpy-var-small | **559** | **559** | 565 (+0.95%) | 565 (+0.95%) |
| memcpy-var-large | 636 | 636 | **635 (-0.23%)** | **635 (-0.23%)** |
| memmove-small | 2484 | 2715 (+9.27%) | **2351 (-5.36%)** | **2351 (-5.36%)** |
| memmove-large | 5053 | 5069 (+0.32%) | **4127 (-18.33%)** | **4127 (-18.33%)** |
| wipeout-tris | 666 | 538 (-19.23%) | **519 (-22.20%)** | **519 (-22.20%)** |
| nfs3-project | **1002** | 1066 (+6.39%) | 1064 (+6.19%) | 1064 (+6.19%) |
| nfs3-clip | 962 | 963 (+0.03%) | **951 (-1.17%)** | **951 (-1.17%)** |
| sha512 | **965** | 1226 (+26.99%) | 1220 (+26.35%) | 1045 (+8.21%) |
| aesgcm | 1156 | 1242 (+7.38%) | 1205 (+4.20%) | **903 (-21.92%)** |
| **GEOMEAN** | **1265** | **1282 (+1.36%)** | **1150 (-9.04%)** | **1125 (-11.03%)** |
| **CODE SIZE** | 393436 | 380648 (-3.25%) | 357496 (-9.13%) | 348252 (-11.48%) |

## Comparing GCC optimization levels

benchwork built with m68k-amigaos-gcc 16.2-rc17[^rc17] and
`-fomit-frame-pointer -m68030 -m68881` at each level, run on a 68030 with
its 256-byte instruction cache modeled[^cache].

### GCC 16.2 optimization levels

Each bar is the build's time divided by `-O2`'s. The Dhrystone LTO bar
does not measure faster code[^dhry-lto].

![Time of each benchmark relative to -O2](results/opt-levels-030.svg)

| benchmark | -O2 | -Os | -O3 | -O2 -flto |
|---|---:|---:|---:|---:|
| dhrystone[^dhry-lto] | 1657 | 2120 (+27.90%) | 1643 (-0.87%) | **242 (-85.42%)** |
| backdrop | 8826 | 14320 (+62.26%) | **6482 (-26.56%)** | 8813 (-0.14%) |
| lha-pack | 15950 | 17567 (+10.14%) | 16068 (+0.74%) | **14879 (-6.71%)** |
| lha-unpack | 5119 | 5335 (+4.23%) | 5160 (+0.82%) | **4559 (-10.93%)** |
| zlib-deflate | 13445 | 14113 (+4.96%) | **13094 (-2.61%)** | 13613 (+1.25%) |
| zlib-inflate | 1765 | **1728 (-2.09%)** | 1731 (-1.95%) | 1762 (-0.19%) |
| png-encode | 24566 | 27419 (+11.62%) | **23550 (-4.13%)** | 24611 (+0.19%) |
| png-decode | 5844 | 6372 (+9.03%) | **5636 (-3.57%)** | 6412 (+9.72%) |
| ftgrays | 3364 | 3502 (+4.10%) | **3337 (-0.80%)** | 3396 (+0.96%) |
| memcpy-small | 1138 | 1313 (+15.39%) | 1138 | **1136 (-0.20%)** |
| memcpy-large | **1933** | 2262 (+17.04%) | **1933** | 1938 (+0.28%) |
| memcpy-var-small | 2330 | 2461 (+5.63%) | 2318 (-0.49%) | **2295 (-1.48%)** |
| memcpy-var-large | 3039 | 3077 (+1.26%) | 3040 (+0.04%) | **3037 (-0.08%)** |
| memmove-small | 8147 | 8265 (+1.45%) | 8177 (+0.36%) | **8137 (-0.13%)** |
| memmove-large | 16046 | 16047 (+0.01%) | **16041 (-0.03%)** | 16045 (-0.01%) |
| wipeout-tris | 2045 | 2115 (+3.43%) | 2090 (+2.18%) | **1971 (-3.61%)** |
| nfs3-project | 4310 | 9580 (+122.27%) | **3426 (-20.51%)** | 3430 (-20.42%) |
| nfs3-clip | 3293 | 8491 (+157.84%) | **3195 (-2.97%)** | 3228 (-1.98%) |
| sha512 | 4699 | 4718 (+0.42%) | **4541 (-3.35%)** | 4699 |
| aesgcm | 3626 | 3748 (+3.35%) | **3600 (-0.74%)** | 3649 (+0.62%) |
| **GEOMEAN** | **4539** | **5362 (+18.12%)** | **4380 (-3.51%)** | **4047 (-10.83%)** |
| **CODE SIZE** | 348320 | 308728 (-11.37%) | 490492 (+40.82%) | 281868 (-19.08%) |

`-Os` turns constant-size copies into loops and keeps the NFS3 kernels'
`GuardedFloatMath` operations out of line, which more than doubles their
time. `-O3`'s larger loops no longer fit the cache in LHa and wipeout,
which lose 1 to 2%, but it still wins overall. LTO helps where a hot loop
calls into another file (LHa, wipeout, NFS3), costs png-decode 9.7%, and
cuts the code by a fifth.

### Code size relative to -O2

![Code size of each build relative to -O2](results/opt-levels-size-030.svg)

## Comparing GCC code generation for 68000, 68020, 68040-FP

The three release binaries, built with
m68k-amigaos-gcc 16.2-rc17[^rc17] by `make release`, all run on the same
emulated 68040[^volamos]. The columns therefore show what the compiler gains from
being allowed each CPU's instructions, not how fast those CPUs are.

| benchmark | 68000 | 68020 | 68040-FP |
|---|---:|---:|---:|
| dhrystone | 452 | 420 (-7.07%) | 421 (-6.90%) |
| backdrop | 9805 | 1978 (-79.82%) | 1941 (-80.20%) |
| lha-pack | 4974 | 4515 (-9.23%) | 4439 (-10.76%) |
| lha-unpack | 1454 | 1393 (-4.21%) | 1382 (-4.91%) |
| zlib-deflate | 3899 | 3736 (-4.17%) | 3698 (-5.14%) |
| zlib-inflate | 526 | 481 (-8.50%) | 511 (-2.86%) |
| png-encode | 7905 | 7733 (-2.16%) | 7543 (-4.57%) |
| png-decode | 1893 | 1602 (-15.39%) | 1587 (-16.18%) |
| ftgrays | 1477 | 864 (-41.47%) | 858 (-41.87%) |
| memcpy-small | 452 | 202 (-55.19%) | 193 (-57.23%) |
| memcpy-large | 740 | 327 (-55.80%) | 326 (-55.90%) |
| memcpy-var-small | 587 | 565 (-3.75%) | 565 (-3.75%) |
| memcpy-var-large | 647 | 634 (-2.11%) | 635 (-1.93%) |
| memmove-small | 2362 | 2351 (-0.44%) | 2351 (-0.44%) |
| memmove-large | 4137 | 4127 (-0.25%) | 4127 (-0.25%) |
| wipeout-tris[^softfloat] | 879 | 810 (-7.89%) | 519 (-41.00%) |
| nfs3-project[^softfloat] | 2480 | 2436 (-1.77%) | 1064 (-57.10%) |
| nfs3-clip[^softfloat] | 2127 | 2027 (-4.70%) | 951 (-55.28%) |
| sha512 | 1012 | 960 (-5.18%) | 1045 (+3.19%) |
| aesgcm | 1028 | 887 (-13.69%) | 903 (-12.16%) |
| **GEOMEAN** | **1574** | **1244 (-20.96%)** | **1125 (-28.51%)** |
| **CODE SIZE** | 371384 | 360348 (-2.97%) | 348252 (-6.23%) |

[^volamos]: `volamos --clock-mhz 25 --cpu 68040 --fpu <binary> -n 1`.
    The emulated time is derived from the CPU's cycle count at 25 MHz;
    native library calls cost no cycles.
[^gcc6]: `m68k-amigaos-gcc (GCC) 6.5.0b 20260819091705`, with newlib.
[^rc17]: `m68k-amigaos-gcc (AmigaDev v16.2-rc17) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^releases]: The released x86_64 Linux tarballs, with their default newlib
    runtime. Code size is the text size `m68k-amigaos-size` reports.
[^vbcc]: `vbcc V0.9i pre` from the same toolchain, `make vbcc-release`:
    `vc +aos68k -O2 -cpu=68040 -fpu=68040` with vc.lib, m040.lib and
    amiga.lib. vbcc 0.9i miscompiles two of the drivers: ftgrays fails
    before it runs, and memcpy-small produces a wrong checksum and writes
    outside its buffers, which is why no geomean is given.
[^sasc]: SAS/C 6.58, `make sasc-release`: `CPU=68040 MATH=68881` with
    `OPTIMIZE OPTIMIZERTIME OPTIMIZERINLINELOCAL OPTIMIZERSCHEDULER` and the
    optimizer depths at 8, `PARAMETERS=REGISTERS CODE=FAR DATA=FAR`, linked
    with scm040.lib, sc.lib and amiga.lib.
[^dhry-lto]: Dhrystone splits its procedures between dhry_1.c and
    dhry_2.c so that the compiler cannot inline across them. LTO defeats
    that, and its LTO time does not measure a faster CPU or better code
    generation.
[^cache]: volamos `--cache`: a direct-mapped instruction cache, so a miss
    costs a memory fetch. It does not model the data cache or bus wait
    states, nor yet the 68040, so the other sections run without it.
[^softfloat]: The 68000 and 68020 builds are soft float, and newlib's float
    arithmetic is a stub into the mathieeesingbas ROM library, which volamos
    runs natively at zero cycles: 370,000 calls in wipeout-tris, and most of
    the work of the NFS3 kernels. Those two columns measure everything in
    these benchmarks except their float math.
[^cxx]: The NFS3 kernels are C++, which of these compilers only gcc builds.
    The geomean of this table leaves them out, so that it covers the same
    benchmarks for every compiler; the other tables include them.
[^longlong]: SHA-512 and GHASH are built on 64-bit integers, and SAS/C 6.58
    has no `long long`. The geomean of this table leaves them out as it does
    the NFS3 kernels.
