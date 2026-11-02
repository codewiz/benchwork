# Comparing C runtimes

Part of the [Benchwork results](README.md); see there for how they are
measured.

benchwork-040 built with `-O2 -fomit-frame-pointer -m68020-60 -m68881` and
each of m68k-amigaos-gcc's C runtimes: newlib, its default, libnix
(`-mcrt=nix20`) and clib2 (`-mcrt=clib2`), by a toolchain built after
16.2-rc22[^toolchain]. Unlike the other pages, this one is measured on
Amiberry, cycle-exact, running AmigaOS 3.2.3[^amiberry]: libnix's `memcpy()`
calls exec's `CopyMem()`, which volamos runs natively in no time at all.

libnix takes 5% less time than newlib and makes the program 8% smaller;
clib2 takes 3% more and is 7% smaller. The copies decide it:

- libnix's `memcpy()` is a call to `CopyMem()`: the fastest for large
  copies, but the library call makes small ones take 37% longer.
- libnix's `memmove()` is an unrolled loop of long moves, which takes 40%
  less time than newlib's.
- clib2's copies are the slowest but for large moves.

Dhrystone, which calls `strcpy()` and `strcmp()` in every loop, takes 9%
longer with libnix and 3% longer with clib2. Every other benchmark is
within 3% of newlib.

Each bar is newlib's time divided by the runtime's, so longer is faster;
newlib is the line at 1.0.

![Speed of each benchmark relative to newlib](runtimes-040.svg)

| benchmark | newlib | libnix | clib2 |
|---|---:|---:|---:|
| dhrystone | **445** | 485 (+8.84%) | 461 (+3.47%) |
| scene-2d | **736** | **736** | **736** |
| quantize-floyd | 870 | **869 (-0.14%)** | 870 (-0.06%) |
| lha-pack | **1115** | 1134 (+1.65%) | 1133 (+1.56%) |
| lha-unpack | 314 | **313 (-0.32%)** | 315 (+0.02%) |
| zlib-deflate | 925 | **923 (-0.24%)** | 924 (-0.11%) |
| zlib-inflate | **317** | **317 (-0.19%)** | **317 (-0.09%)** |
| png-encode | **1732** | 1753 (+1.21%) | **1732 (-0.03%)** |
| png-decode | 1278 | **1269 (-0.68%)** | 1282 (+0.32%) |
| ftgrays | 666 | **648 (-2.72%)** | 653 (-1.97%) |
| memcpy-small | **1350** | **1350** | **1350** |
| memcpy-large | **449** | **449 (-0.01%)** | **449 (-0.01%)** |
| memcpy-var-small | **579** | 794 (+37.01%) | 925 (+59.70%) |
| memcpy-var-large | 739 | **412 (-44.32%)** | 853 (+15.40%) |
| memmove-small | 804 | **462 (-42.60%)** | 1081 (+34.42%) |
| memmove-large | 1011 | **616 (-39.09%)** | 761 (-24.79%) |
| wipeout-tris | **1146** | **1146** | **1146** |
| nfs3-project | **1191** | **1191** | **1191** |
| nfs3-clip | **999** | 1000 (+0.06%) | **999** |
| sha512 | **1670** | **1670** | **1670** |
| aesgcm | 1161 | 1161 (-0.01%) | **1160 (-0.05%)** |
| chacha20poly1305 | **1108** | **1108 (+0.01%)** | **1108 (+0.01%)** |
| x25519 | **3018** | 3019 (+0.03%) | **3018** |
| **GEOMEAN** | **896** | **849 (-5.29%)** | **922 (+2.88%)** |
| **CODE SIZE** | 411304 | 378836 (-7.89%) | 383908 (-6.66%) |

[^toolchain]: `m68k-amigaos-gcc 16.2.1b 20261010182711` from AmigaPorts
    m68k-amigaos-gcc 0eba518, after 16.2-rc22, with newlib 328c043ca,
    libnix built at `-O2`, and clib2 d934a64.
[^amiberry]: Amiberry be174e7de, after v8.5.0: an A4000 with a 25 MHz
    68040, `cpu_cycle_exact`, 2 MB of chip and 128 MB of motherboard RAM,
    Kickstart 47.115 and Workbench 3.2.3. Each benchmark is the fastest of
    three repetitions, of at least a second each.
