# Comparing GCC optimization levels

Part of the [Benchwork results](README.md); see there for how they are
measured.

benchwork built with m68k-amigaos-gcc 16.2-rc18[^rc18] and
`-fomit-frame-pointer -m68030 -m68881` at each level, run on a 68030 with
its 256-byte instruction cache modeled[^cache].

## Time relative to -O2

Each bar is the build's time divided by `-O2`'s. The Dhrystone LTO bar
does not measure faster code[^dhry-lto].

![Time of each benchmark relative to -O2](opt-levels-030.svg)

| benchmark | -O2 | -Os | -O3 | -O2 -flto |
|---|---:|---:|---:|---:|
| dhrystone[^dhry-lto] | 1642 | 2096 (+27.63%) | 1621 (-1.27%) | **208 (-87.36%)** |
| backdrop | 12006 | 16071 (+33.86%) | **8415 (-29.90%)** | 12006 |
| lha-pack | 15740 | 17365 (+10.32%) | 15847 (+0.68%) | **14853 (-5.64%)** |
| lha-unpack | 4413 | 4626 (+4.82%) | 4438 (+0.57%) | **3807 (-13.74%)** |
| zlib-deflate | 13215 | 13882 (+5.05%) | **12864 (-2.66%)** | 13352 (+1.04%) |
| zlib-inflate | 3133 | **3023 (-3.52%)** | 3030 (-3.29%) | 3155 (+0.68%) |
| png-encode | 7623 | 8327 (+9.24%) | **7348 (-3.60%)** | 7606 (-0.22%) |
| png-decode | 3485 | 3789 (+8.72%) | **3271 (-6.16%)** | 3677 (+5.51%) |
| ftgrays | 2628 | 2765 (+5.24%) | 2600 (-1.07%) | **2556 (-2.71%)** |
| memcpy-small | **5883** | 6921 (+17.65%) | **5883** | 5885 (+0.04%) |
| memcpy-large | 1776 | 2100 (+18.25%) | 1776 | **1765 (-0.58%)** |
| memcpy-var-small | 2119 | 2270 (+7.14%) | 2156 (+1.77%) | **2069 (-2.38%)** |
| memcpy-var-large | **2882** | 2913 (+1.08%) | **2882** | 2901 (+0.67%) |
| memmove-small | 6170 | 6250 (+1.31%) | **6115 (-0.89%)** | 6159 (-0.17%) |
| memmove-large | **6008** | 6015 (+0.12%) | 6009 (+0.02%) | 6010 (+0.04%) |
| wipeout-tris | 3797 | 3990 (+5.09%) | 3945 (+3.92%) | **3239 (-14.70%)** |
| nfs3-project | 4310 | 9580 (+122.27%) | 3426 (-20.51%) | **3406 (-20.97%)** |
| nfs3-clip | 3293 | 8454 (+156.71%) | **3198 (-2.89%)** | 3229 (-1.96%) |
| sha512 | 4698 | 4718 (+0.42%) | **4541 (-3.35%)** | 4726 (+0.59%) |
| aesgcm | 3446 | 3568 (+3.53%) | **3419 (-0.78%)** | 3447 (+0.04%) |
| chacha20poly1305 | 1744 | 1844 (+5.75%) | **1691 (-3.04%)** | 1748 (+0.23%) |
| x25519 | **2457** | 2746 (+11.80%) | 2488 (+1.28%) | 2459 (+0.10%) |
| **GEOMEAN** | **4127** | **4803 (+16.39%)** | **3980 (-3.57%)** | **3657 (-11.39%)** |
| **CODE SIZE** | 404620 | 364436 (-9.93%) | 563824 (+39.35%) | 336340 (-16.88%) |

`-Os` turns constant-size copies into loops and keeps the NFS3 kernels'
`GuardedFloatMath` operations out of line, which more than doubles their
time. `-O3`'s larger loops no longer fit the cache in LHa and wipeout,
which lose up to 3.9%, but it still wins overall. LTO helps where a hot loop
calls into another file (LHa, wipeout, NFS3), costs png-decode 5.5%, and
cuts the code by a sixth.

## Code size relative to -O2

![Code size of each build relative to -O2](opt-levels-size-030.svg)

[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^cache]: volamos `--cache`: a direct-mapped instruction cache, so a miss
    costs a memory fetch. It does not model the data cache or bus wait
    states, nor yet the 68040, so the other pages run without it.
[^dhry-lto]: Dhrystone splits its procedures between dhry_1.c and
    dhry_2.c so that the compiler cannot inline across them. LTO defeats
    that, and its LTO time does not measure a faster CPU or better code
    generation.
