# Comparing GCC optimization levels

Part of the [Benchwork results](README.md); see there for how they are
measured.

benchwork built with m68k-amigaos-gcc 16.2-rc18[^rc18] and
`-fomit-frame-pointer -m68030 -m68881` at each level, run on a 68030 with
its 256-byte instruction cache modeled[^cache].

## Speed relative to -O2

Each bar is `-O2`'s time divided by the build's, so longer is faster.

![Speed of each benchmark relative to -O2](opt-levels-030.svg)

| benchmark | -O2 | -Os | -O3 | -O2 -flto |
|---|---:|---:|---:|---:|
| dhrystone[^dhry-lto] | 1632 | 2099 (+28.58%) | **1624 (-0.49%)** | 1643 (+0.64%) |
| scene-2d | 5135 | 8538 (+66.26%) | **3680 (-28.33%)** | 5135 |
| quantize-floyd | 2429 | 2695 (+10.96%) | **2406 (-0.93%)** | 2444 (+0.61%) |
| lha-pack | 3992 | 4392 (+10.03%) | 4022 (+0.75%) | **3760 (-5.82%)** |
| lha-unpack | 1125 | 1181 (+4.93%) | 1132 (+0.54%) | **989 (-12.10%)** |
| zlib-deflate | 2981 | 3126 (+4.87%) | **2909 (-2.41%)** | 2999 (+0.60%) |
| zlib-inflate | 805 | **774 (-3.81%)** | 779 (-3.28%) | 811 (+0.70%) |
| png-encode | 4377 | 4895 (+11.83%) | **4188 (-4.32%)** | 4305 (-1.64%) |
| png-decode | 3192 | 3507 (+9.88%) | **2941 (-7.86%)** | 3356 (+5.14%) |
| ftgrays | 2628 | 2766 (+5.25%) | 2599 (-1.09%) | **2556 (-2.73%)** |
| memcpy-small | **5883** | 6921 (+17.65%) | **5883** | 5885 (+0.04%) |
| memcpy-large | 1776 | 2105 (+18.53%) | 1776 | **1765 (-0.58%)** |
| memcpy-var-small | **2028** | 2111 (+4.08%) | 2101 (+3.59%) | 2190 (+7.99%) |
| memcpy-var-large | 2907 | **2890 (-0.61%)** | 2904 (-0.10%) | 2893 (-0.51%) |
| memmove-small | **3056** | 3113 (+1.86%) | 3072 (+0.52%) | 3075 (+0.61%) |
| memmove-large | 4006 | 4012 (+0.15%) | **4005 (-0.04%)** | 4009 (+0.06%) |
| wipeout-tris | 3791 | 3977 (+4.90%) | 3945 (+4.04%) | **3255 (-14.14%)** |
| nfs3-project | 4310 | 9580 (+122.27%) | 3426 (-20.51%) | **3406 (-20.97%)** |
| nfs3-clip | 3293 | 8512 (+158.47%) | **3196 (-2.94%)** | 3229 (-1.96%) |
| sha512 | 4698 | 4718 (+0.42%) | **4541 (-3.35%)** | 4726 (+0.59%) |
| aesgcm | 3446 | 3568 (+3.53%) | **3419 (-0.78%)** | 3447 (+0.04%) |
| chacha20poly1305 | 1744 | 1844 (+5.75%) | **1691 (-3.04%)** | 1756 (+0.67%) |
| x25519 | **2457** | 2746 (+11.80%) | 2488 (+1.28%) | 2459 (+0.10%) |
| **GEOMEAN** | **2820** | **3307 (+17.26%)** | **2727 (-3.29%)** | **2762 (-2.06%)** |
| **CODE SIZE** | 407124 | 366460 (-9.99%) | 567968 (+39.51%) | 339504 (-16.61%) |

`-Os` turns constant-size copies into loops and keeps the NFS3 kernels'
`GuardedFloatMath` operations out of line, which more than doubles their
time. `-O3`'s larger loops no longer fit the cache in LHa and wipeout,
which lose up to 4.0%, but it still wins overall. LTO helps where a hot loop
calls into another file (LHa, wipeout, NFS3), costs png-decode 5.1%, and
cuts the code by a sixth.

## Code size relative to -O2

![Code size of each build relative to -O2](opt-levels-size-030.svg)

[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^cache]: volamos `--cache`: a direct-mapped instruction cache, so a miss
    costs a memory fetch. It does not model the data cache or bus wait
    states, nor yet the 68040, so the other pages run without it.
[^dhry-lto]: Dhrystone splits its procedures between dhry_1.c and
    dhry_2.c so that the compiler cannot inline across them, so the two
    files are built without LTO even in the LTO build.
