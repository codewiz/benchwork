# Comparing GCC code generation per CPU

Part of the [Benchwork results](README.md); see there for how they are
measured.

## 68000, 68020, 68020-60-FP, 68040-FP

The release binaries that run on a 68040, built with
m68k-amigaos-gcc 16.2-rc25[^rc25] by `make release`, all run on the same
emulated 68040. The columns therefore show what the compiler gains from
being allowed each CPU's instructions, not how fast those CPUs are.
68020-60-FP is benchwork-020-060fp, built with `-m68020-60 -m68881`, which
runs on any 68020 or later with an FPU.

`-m68020-60` cannot use the 32x32->64 `muls.l` and `mulu.l`, which the
68060 lacks, so chacha20poly1305 and x25519 build each such multiply from
16-bit `mulu.w`: 911 of them in curve25519.o, against 226 `muls.l` for the
68040. Up to rc18 they were calls to `__muldi3`; rc19 inlines them
(AmigaPorts/gcc#112).

| benchmark | 68000 | 68020 | 68020-60-FP | 68040-FP |
|---|---:|---:|---:|---:|
| dhrystone | 461 | 431 (-6.59%) | 430 (-6.77%) | 430 (-6.77%) |
| scene-2d | 6722 | 1048 (-84.42%) | 1009 (-84.99%) | 1007 (-85.02%) |
| quantize-floyd | 913 | 657 (-28.06%) | 665 (-27.18%) | 665 (-27.18%) |
| lha-pack | 1235 | 1125 (-8.88%) | 1098 (-11.04%) | 1099 (-11.04%) |
| lha-unpack | 302 | 295 (-2.41%) | 292 (-3.23%) | 292 (-3.23%) |
| zlib-deflate | 845 | 814 (-3.70%) | 802 (-5.14%) | 802 (-5.14%) |
| zlib-inflate | 217 | 215 (-0.90%) | 229 (+5.48%) | 230 (+5.99%) |
| png-encode | 1308 | 1264 (-3.37%) | 1254 (-4.16%) | 1254 (-4.16%) |
| png-decode | 1084 | 989 (-8.75%) | 982 (-9.39%) | 982 (-9.37%) |
| ftgrays | 1229 | 666 (-45.77%) | 659 (-46.34%) | 659 (-46.34%) |
| memcpy-small | 2396 | 908 (-62.12%) | 852 (-64.42%) | 852 (-64.42%) |
| memcpy-large | 688 | 285 (-58.56%) | 285 (-58.66%) | 285 (-58.66%) |
| memcpy-var-small | 556 | 523 (-5.96%) | 523 (-5.96%) | 523 (-5.96%) |
| memcpy-var-large | 598 | 592 (-1.04%) | 593 (-0.84%) | 593 (-0.84%) |
| memmove-small | 905 | 920 (+1.63%) | 920 (+1.63%) | 920 (+1.63%) |
| memmove-large | 956 | 957 (+0.18%) | 957 (+0.18%) | 957 (+0.18%) |
| json-parse | 429 | 419 (-2.12%) | 409 (-4.66%) | 409 (-4.66%) |
| json-print | 446 | 292 (-34.52%) | 256 (-42.62%) | 256 (-42.62%) |
| wipeout-tris[^softfloat] | 2312 | 2323 (+0.49%) | 890 (-61.50%) | 890 (-61.50%) |
| nfs3-project[^softfloat] | 3282 | 3238 (-1.34%) | 1064 (-67.58%) | 1064 (-67.58%) |
| nfs3-clip[^softfloat] | 2756 | 2655 (-3.67%) | 946 (-65.67%) | 943 (-65.77%) |
| sha512 | 1011 | 953 (-5.79%) | 959 (-5.18%) | 953 (-5.79%) |
| aesgcm | 957 | 838 (-12.39%) | 825 (-13.83%) | 826 (-13.70%) |
| chacha20poly1305 | 1609 | 385 (-76.07%) | 652 (-59.49%) | 399 (-75.18%) |
| x25519 | 5251 | 555 (-89.43%) | 1725 (-67.15%) | 546 (-89.61%) |
| **GEOMEAN** | **1068** | **733 (-31.40%)** | **684 (-35.95%)** | **641 (-40.03%)** |
| **CODE SIZE** | 469544 | 458608 (-2.33%) | 457768 (-2.51%) | 443472 (-5.55%) |

## On a 68060

There is no point in building with `-m68060`: it generates practically
the same code as `-m68020-60`, which also runs on every other 68020+. Emulated 50 MHz 68060 with its
caches modeled[^cache], built by rc25[^rc25] with
`volamos --clock-mhz 50 --cpu 68060 --fpu --cache --fpsp-emulate`:

| benchmark | -m68020-60 -m68881 | -m68060 -m68881 |
|---|---:|---:|
| dhrystone | **241** | 242 (+0.33%) |
| scene-2d | **521** | **521** |
| quantize-floyd | **317** | **317** |
| lha-pack | **589** | **589 (+0.04%)** |
| lha-unpack | **144** | **144** |
| zlib-deflate | **414** | **414** |
| zlib-inflate | **114** | **114** |
| png-encode | **610** | **610** |
| png-decode | **443** | **443 (+0.01%)** |
| ftgrays | **344** | **344 (+0.03%)** |
| memcpy-small | **431** | **431** |
| memcpy-large | **135** | **135** |
| memcpy-var-small | **303** | **303** |
| memcpy-var-large | **267** | **267** |
| memmove-small | **392** | **392** |
| memmove-large | **369** | **369** |
| json-parse | **226** | **226 (+0.03%)** |
| json-print | **139** | 140 (+0.05%) |
| wipeout-tris | **455** | **455** |
| nfs3-project | **609** | **609** |
| nfs3-clip | **514** | **514** |
| sha512 | **454** | 457 (+0.67%) |
| aesgcm | **351** | **351** |
| chacha20poly1305 | **193** | **193** |
| x25519 | **509** | **509** |
| **GEOMEAN** | **327** | **327 (+0.05%)** |
| **CODE SIZE** | 457768 | 456852 (-0.20%) |

[^rc25]: `m68k-amigaos-gcc (AmigaDev 16.2-rc25) 16.2.1b 20261010182711`,
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
