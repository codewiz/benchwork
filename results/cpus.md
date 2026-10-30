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
time. rc19 inlines that multiply (AmigaPorts/gcc#112).

| benchmark | 68000 | 68020 | 68020-60-FP | 68040-FP |
|---|---:|---:|---:|---:|
| dhrystone | 444 | 414 (-6.84%) | 414 (-6.84%) | 414 (-6.84%) |
| scene-2d | 6751 | 1037 (-84.63%) | 1034 (-84.68%) | 1032 (-84.71%) |
| quantize-floyd | 913 | 659 (-27.82%) | 659 (-27.87%) | 659 (-27.87%) |
| lha-pack | 1244 | 1129 (-9.18%) | 1110 (-10.77%) | 1110 (-10.77%) |
| lha-unpack | 309 | 304 (-1.59%) | 301 (-2.46%) | 301 (-2.46%) |
| zlib-deflate | 846 | 814 (-3.86%) | 804 (-4.94%) | 804 (-4.94%) |
| zlib-inflate | 218 | 215 (-1.01%) | 236 (+8.27%) | 237 (+8.79%) |
| png-encode | 1394 | 1357 (-2.60%) | 1324 (-4.96%) | 1324 (-4.96%) |
| png-decode | 1084 | 1015 (-6.41%) | 985 (-9.15%) | 985 (-9.14%) |
| ftgrays | 1231 | 668 (-45.75%) | 662 (-46.23%) | 662 (-46.23%) |
| memcpy-small | 2396 | 963 (-59.81%) | 908 (-62.12%) | 908 (-62.12%) |
| memcpy-large | 688 | 285 (-58.56%) | 285 (-58.66%) | 285 (-58.66%) |
| memcpy-var-small | 556 | 523 (-5.96%) | 523 (-5.96%) | 523 (-5.96%) |
| memcpy-var-large | 598 | 592 (-1.04%) | 593 (-0.84%) | 593 (-0.84%) |
| memmove-small | 959 | 939 (-2.07%) | 939 (-2.07%) | 939 (-2.07%) |
| memmove-large | 1251 | 959 (-23.34%) | 959 (-23.34%) | 959 (-23.34%) |
| wipeout-tris[^softfloat] | 2312 | 2323 (+0.46%) | 893 (-61.36%) | 893 (-61.36%) |
| nfs3-project[^softfloat] | 3282 | 3238 (-1.34%) | 1064 (-67.58%) | 1064 (-67.58%) |
| nfs3-clip[^softfloat] | 2762 | 2662 (-3.62%) | 955 (-65.44%) | 952 (-65.54%) |
| sha512 | 1023 | 960 (-6.19%) | 954 (-6.79%) | 954 (-6.79%) |
| aesgcm | 967 | 837 (-13.47%) | 851 (-11.94%) | 853 (-11.81%) |
| chacha20poly1305 | 1605 | 391 (-75.67%) | 985 (-38.65%) | 399 (-75.12%) |
| x25519 | 5252 | 556 (-89.41%) | 2967 (-43.52%) | 547 (-89.59%) |
| **GEOMEAN** | **1176** | **788 (-32.97%)** | **769 (-34.58%)** | **687 (-41.55%)** |
| **CODE SIZE** | 432772 | 418836 (-3.22%) | 410548 (-5.14%) | 405216 (-6.37%) |

## On a 68060

There is no point in building with `-m68060`: it generates practically
the same code as `-m68020-60`, which also runs on every other 68020+. Emulated 50 MHz 68060 with its
caches modeled[^cache], built by rc18[^rc18] with
`volamos --clock-mhz 50 --cpu 68060 --fpu --cache`:

| benchmark | -m68020-60 -m68881 | -m68060 -m68881 |
|---|---:|---:|
| dhrystone | **253** | **253** |
| scene-2d | **567** | **567** |
| quantize-floyd | **366** | **366** |
| lha-pack | **634** | **634** |
| lha-unpack | **169** | 170 (+0.35%) |
| zlib-deflate | **444** | **444** |
| zlib-inflate | **135** | **135** |
| png-encode | **711** | **711** |
| png-decode | **513** | **513** |
| ftgrays | **366** | **366** |
| memcpy-small | **461** | **461** |
| memcpy-large | **136** | **136** |
| memcpy-var-small | **309** | **309** |
| memcpy-var-large | **268** | **268** |
| memmove-small | **412** | **412** |
| memmove-large | **389** | **389** |
| wipeout-tris | **487** | **487** |
| nfs3-project | **616** | **616** |
| nfs3-clip | **532** | **532** |
| sha512 | 480 | **477 (-0.64%)** |
| aesgcm | **440** | **440** |
| chacha20poly1305 | **423** | **423** |
| x25519 | **1208** | **1208** |
| **GEOMEAN** | **399** | **398 (-0.01%)** |
| **CODE SIZE** | 410548 | 409768 (-0.19%) |

[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
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
