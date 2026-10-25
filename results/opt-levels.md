# Comparing GCC optimization levels

Part of the [Benchwork results](README.md); see there for how they are
measured.

benchwork built with m68k-amigaos-gcc 16.2-rc19[^rc19] and
`-fomit-frame-pointer -m68030 -m68881` at each level, run on a 68030 with
its 256-byte instruction cache modeled[^cache].

## Speed relative to -O2

Each bar is `-O2`'s time divided by the build's, so longer is faster;
`-O2` itself is the line at 1.0.

![Speed of each benchmark relative to -O2](opt-levels-030.svg)

| benchmark | -O2 | -Os | -O3 | -O2 -flto |
|---|---:|---:|---:|---:|
| dhrystone[^dhry-lto] | 1636 | 2100 (+28.37%) | **1623 (-0.78%)** | 1642 (+0.39%) |
| scene-2d | 5152 | 8538 (+65.72%) | **3685 (-28.47%)** | 5135 (-0.32%) |
| quantize-floyd | 2404 | 2695 (+12.09%) | **2396 (-0.36%)** | 2419 (+0.60%) |
| lha-pack | 3970 | 4242 (+6.86%) | 3988 (+0.47%) | **3677 (-7.37%)** |
| lha-unpack | 1105 | 1150 (+4.09%) | 1100 (-0.46%) | **972 (-11.99%)** |
| zlib-deflate | 2981 | 3118 (+4.59%) | **2909 (-2.44%)** | 2985 (+0.12%) |
| zlib-inflate | 806 | **779 (-3.35%)** | 829 (+2.84%) | 813 (+0.85%) |
| png-encode | 4372 | 4817 (+10.16%) | 4200 (-3.95%) | **4161 (-4.83%)** |
| png-decode | 3114 | 3494 (+12.23%) | **2946 (-5.38%)** | 3091 (-0.73%) |
| ftgrays | 2625 | 2764 (+5.30%) | 2596 (-1.10%) | **2554 (-2.70%)** |
| memcpy-small | **5883** | 6921 (+17.65%) | **5883** | **5883** |
| memcpy-large | **1776** | 2104 (+18.52%) | **1776** | **1776** |
| memcpy-var-small | 2127 | 2106 (-0.98%) | **2031 (-4.50%)** | 2034 (-4.37%) |
| memcpy-var-large | **2882** | 2889 (+0.25%) | 2888 (+0.20%) | 2888 (+0.21%) |
| memmove-small | 3155 | 3202 (+1.50%) | 3174 (+0.63%) | **3152 (-0.08%)** |
| memmove-large | **3968** | 3971 (+0.07%) | **3968 (-0.01%)** | **3968** |
| wipeout-tris | 3857 | 3977 (+3.10%) | 3953 (+2.47%) | **3245 (-15.88%)** |
| nfs3-project | 4310 | 9580 (+122.27%) | 3430 (-20.42%) | **3410 (-20.88%)** |
| nfs3-clip | 3293 | 8378 (+154.38%) | **3193 (-3.05%)** | 3230 (-1.92%) |
| sha512 | 4696 | 4737 (+0.86%) | 4695 (-0.02%) | **4694 (-0.05%)** |
| aesgcm | 3422 | 3525 (+2.98%) | **3396 (-0.77%)** | 3425 (+0.08%) |
| chacha20poly1305 | 1797 | 1889 (+5.09%) | **1688 (-6.06%)** | 1804 (+0.35%) |
| x25519 | **2450** | 2746 (+12.10%) | 2487 (+1.51%) | 2456 (+0.24%) |
| **GEOMEAN** | **2825** | **3298 (+16.72%)** | **2731 (-3.33%)** | **2736 (-3.15%)** |
| **CODE SIZE** | 410376 | 369612 (-9.93%) | 540232 (+31.64%) | 342856 (-16.45%) |

`-Os` turns constant-size copies into loops and keeps the NFS3 kernels'
`GuardedFloatMath` operations out of line, which more than doubles their
time. `-O3` loses up to 2.8% where its larger code fits the cache worse
(zlib-inflate, wipeout), but wins overall. LTO helps where a hot loop calls
into another file (LHa, wipeout, NFS3, png-encode), nearly ties `-O3`
overall, and cuts the code by a sixth.

## Code size relative to -O2

![Code size of each build relative to -O2](opt-levels-size-030.svg)

[^rc19]: `m68k-amigaos-gcc (AmigaDev 16.2-rc19) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^cache]: volamos `--cache`: a direct-mapped instruction cache, so a miss
    costs a memory fetch. It does not model the data cache or bus wait
    states, nor yet the 68040, so the other pages run without it.
[^dhry-lto]: Dhrystone splits its procedures between dhry_1.c and
    dhry_2.c so that the compiler cannot inline across them, so the two
    files are built without LTO even in the LTO build.
