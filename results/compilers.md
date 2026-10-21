# Comparing m68k compilers

Part of the [Benchwork results](README.md); see there for how they are
measured.

All runs done with benchwork-040 built with `-O2 -fomit-frame-pointer -m68040
-m68881`, or the equivalent for vbcc and SAS/C. Each compiler links its own
C library, so the memcpy and memmove rows compare the compiler together with
its runtime.

## Time relative to gcc 6.5.0b

Each bar is the compiler's time divided by gcc 6.5.0b's, so shorter is faster.

![Time of each benchmark relative to gcc 6.5.0b](compilers-040.svg)

Same data in tabular form (time in milliseconds):

| benchmark | 6.5.0b[^gcc6] | 16.2-rc18[^rc18] | vbcc 0.9i[^vbcc] | SAS/C 6.58[^sasc] |
|---|---:|---:|---:|---:|
| dhrystone | **414** | **414 (+0.07%)** | 591 (+42.76%) | 573 (+38.51%) |
| backdrop | 3767 | **2927 (-22.30%)** | 5587 (+48.32%) | 5362 (+42.34%) |
| lha-pack | **4315** | 4377 (+1.44%) | 4792 (+11.06%) | 4878 (+13.04%) |
| lha-unpack | 1193 | 1181 (-1.04%) | **1142 (-4.28%)** | 1291 (+8.21%) |
| zlib-deflate | **3110** | 3637 (+16.92%) | 4197 (+34.94%) | 4023 (+29.34%) |
| zlib-inflate | 767 | 920 (+19.97%) | **710 (-7.40%)** | 815 (+6.30%) |
| png-encode | **1954** | 2231 (+14.14%) | 2989 (+52.93%) | 2752 (+40.80%) |
| png-decode | **863** | 934 (+8.29%) | 1234 (+43.07%) | 1156 (+33.99%) |
| ftgrays | **649** | 662 (+2.05%) | failed | 709 (+9.28%) |
| memcpy-small | **862** | 908 (+5.35%) | wrong output | 1790 (+107.75%) |
| memcpy-large | 286 | **285 (-0.45%)** | 1215 (+324.99%) | 451 (+57.88%) |
| memcpy-var-small | 546 | 523 (-4.22%) | 602 (+10.29%) | **374 (-31.53%)** |
| memcpy-var-large | 596 | **593 (-0.56%)** | 1564 (+162.29%) | 594 (-0.34%) |
| memmove-small | 2205 | **1878 (-14.86%)** | 4268 (+93.50%) | 3060 (+38.77%) |
| memmove-large | 3969 | **1439 (-63.75%)** | 12405 (+212.53%) | 8862 (+123.25%) |
| wipeout-tris | **883** | 893 (+1.16%) | 1839 (+108.30%) | 1342 (+51.95%) |
| nfs3-project[^cxx] | 1168 | **1064 (-8.90%)** | not built | not built |
| nfs3-clip[^cxx] | 1005 | **952 (-5.33%)** | not built | not built |
| sha512[^longlong] | 964 | **960 (-0.41%)** | 3824 (+296.81%) | not built |
| aesgcm[^longlong] | 1073 | **853 (-20.50%)** | 3211 (+199.39%) | not built |
| chacha20poly1305[^longlong] | **380** | 399 (+5.02%) | 1014 (+166.52%) | not built |
| x25519[^longlong] | **539** | 547 (+1.53%) | failed | not built |
| **GEOMEAN** | **1174** | **1114 (-5.13%)** | | **1533 (+30.65%)** |

[^gcc6]: `m68k-amigaos-gcc (GCC) 6.5.0b 20260819091705`, with newlib.
[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^vbcc]: `vbcc V0.9i pre` from the same toolchain, `make vbcc-release`:
    `vc +aos68k -O2 -cpu=68040 -fpu=68040` with vc.lib, m040.lib and
    amiga.lib. vbcc 0.9i miscompiles three of the benchmarks: ftgrays
    fails before it runs, x25519 fails its self-test (from `-O1` up), and
    memcpy-small produces a wrong checksum and writes outside its buffers,
    which is why no geomean is given.
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
