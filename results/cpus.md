# Comparing GCC code generation per CPU

Part of the [Benchwork results](README.md); see there for how they are
measured.

## 68000, 68020, 68020-60-FP, 68040-FP

The release binaries that run on a 68040, built with
m68k-amigaos-gcc 16.2-rc18[^rc18] by `make release`, all run on the same
emulated 68040. The columns therefore show what the compiler gains from
being allowed each CPU's instructions, not how fast those CPUs are.
68020-60-FP is benchwork-020-060fp, built with `-m68020-60 -m68881`, which
runs on any 68020 or later with an FPU.

`-m68020-60` cannot use the 64-bit `mulu.l`, which the 68060 lacks, so
chacha20poly1305 and x25519 call `__muldi3`, which multiplies 16 bits at a
time. AmigaPorts/gcc#112, not in rc18, inlines that multiply.

| benchmark | 68000 | 68020 | 68020-60-FP | 68040-FP |
|---|---:|---:|---:|---:|
| dhrystone | 444 | 414 (-6.84%) | 414 (-6.84%) | 414 (-6.84%) |
| backdrop | 11238 | 3020 (-73.12%) | 2877 (-74.40%) | 2927 (-73.96%) |
| lha-pack | 4908 | 4453 (-9.27%) | 4377 (-10.82%) | 4377 (-10.82%) |
| lha-unpack | 1211 | 1191 (-1.59%) | 1181 (-2.44%) | 1181 (-2.44%) |
| zlib-deflate | 3822 | 3675 (-3.85%) | 3637 (-4.85%) | 3637 (-4.85%) |
| zlib-inflate | 846 | 837 (-0.96%) | 916 (+8.33%) | 920 (+8.84%) |
| png-encode | 2354 | 2294 (-2.57%) | 2231 (-5.25%) | 2231 (-5.25%) |
| png-decode | 1053 | 920 (-12.71%) | 934 (-11.39%) | 934 (-11.33%) |
| ftgrays | 1231 | 668 (-45.75%) | 662 (-46.23%) | 662 (-46.23%) |
| memcpy-small | 2396 | 963 (-59.81%) | 908 (-62.12%) | 908 (-62.12%) |
| memcpy-large | 688 | 285 (-58.56%) | 285 (-58.66%) | 285 (-58.66%) |
| memcpy-var-small | 556 | 523 (-5.96%) | 523 (-5.96%) | 523 (-5.96%) |
| memcpy-var-large | 598 | 592 (-1.04%) | 593 (-0.84%) | 593 (-0.84%) |
| memmove-small | 1917 | 1878 (-2.07%) | 1878 (-2.07%) | 1878 (-2.07%) |
| memmove-large | 1877 | 1439 (-23.34%) | 1439 (-23.34%) | 1439 (-23.34%) |
| wipeout-tris[^softfloat] | 1814 | 1824 (+0.58%) | 893 (-50.75%) | 893 (-50.75%) |
| nfs3-project[^softfloat] | 2474 | 2430 (-1.78%) | 1064 (-56.99%) | 1064 (-56.99%) |
| nfs3-clip[^softfloat] | 2121 | 2021 (-4.71%) | 955 (-54.98%) | 952 (-55.11%) |
| sha512 | 1023 | 960 (-6.19%) | 960 (-6.19%) | 960 (-6.19%) |
| aesgcm | 967 | 837 (-13.47%) | 851 (-11.94%) | 853 (-11.81%) |
| chacha20poly1305 | 1605 | 391 (-75.67%) | 985 (-38.65%) | 399 (-75.12%) |
| x25519 | 5252 | 556 (-89.41%) | 2967 (-43.52%) | 547 (-89.59%) |
| **GEOMEAN** | **1629** | **1112 (-31.74%)** | **1125 (-30.96%)** | **1001 (-38.57%)** |
| **CODE SIZE** | 430132 | 416372 (-3.20%) | 407920 (-5.16%) | 402684 (-6.38%) |

## On a 68060

There is no point in building with `-m68060` yet: until GCC gets 68060
cost tables, it generates practically the same code as `-m68020-60`,
which also runs on every other 68020+. Emulated 50 MHz 68060 with its
caches modeled[^cache], built by rc18[^rc18] with
`volamos --clock-mhz 50 --cpu 68060 --fpu --cache`:

| benchmark | -m68020-60 -m68881 | -m68060 -m68881 |
|---|---:|---:|
| dhrystone | **253** | **253** |
| backdrop | **1692** | **1692** |
| lha-pack | **2510** | **2510** |
| lha-unpack | **664** | 666 (+0.34%) |
| zlib-deflate | **2005** | **2005** |
| zlib-inflate | **525** | **525** |
| png-encode | **1199** | **1199** |
| png-decode | **466** | **466** |
| ftgrays | **366** | **366** |
| memcpy-small | **461** | **461** |
| memcpy-large | **136** | **136** |
| memcpy-var-small | **309** | **309** |
| memcpy-var-large | **268** | **268** |
| memmove-small | **824** | **824** |
| memmove-large | **584** | **584** |
| wipeout-tris | **487** | **487** |
| nfs3-project | **616** | **616** |
| nfs3-clip | **532** | **532** |
| sha512 | **477** | **477** |
| aesgcm | **440** | **440** |
| chacha20poly1305 | **423** | **423** |
| x25519 | **1208** | **1208** |
| **GEOMEAN** | **581** | **581 (+0.02%)** |
| **CODE SIZE** | 407920 | 407264 (-0.16%) |

[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^cache]: volamos `--cache`: a direct-mapped instruction cache, so a miss
    costs a memory fetch. It does not model the data cache or bus wait
    states.
[^softfloat]: The 68000 and 68020 builds are soft float, and newlib's
    float arithmetic is a stub into the mathieeesingbas ROM library, which
    volamos runs natively at zero cycles: every float operation of
    wipeout-tris and most of the work of the NFS3 kernels. Those two
    columns measure everything in these benchmarks except their float math.
