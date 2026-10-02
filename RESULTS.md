# Results

Fastest iteration in milliseconds under volamos[^volamos], one iteration
each; the cycle count makes a second one identical. Check values agree
across every column.

## Release binaries, m68k-amigaos-gcc 16.2-rc13[^gcc]

Benchwork 1.0[^benchwork], built with `make release`.

| benchmark | 000 | 020 | 040 |
|---|---:|---:|---:|
| dhrystone | 608 | 624 | 639 |
| backdrop | 2981[^rom] | 2010 | 2082 |
| lha-pack | 5036[^rom] | 4515 | 4483 |
| lha-unpack | 1549[^rom] | 1483 | 1449 |
| zlib-deflate | 3874[^rom] | 3678 | 3628 |
| zlib-inflate | 617[^rom] | 563 | 563 |
| png-encode | 8169[^rom] | 7812 | 7829 |
| png-decode | 2335[^rom] | 2107 | 2110 |
| ftgrays | 994 | 865 | 859 |
| memcpy-small | 1169 | 1117 | 1097 |
| memcpy-large | 1220 | 877 | 874 |
| memcpy-var-small | 1079 | 979 | 947 |
| memcpy-var-large | 1377 | 915 | 910 |
| memmove-small | 1069 | 814 | 814 |
| memmove-large | 751 | 743 | 743 |
| wipeout-tris | 1180[^softfloat] | 1156[^softfloat] | 616 |
| **geomean** | **1564** | **1369** | **1312** |

## Compilers, benchwork-040 flags

`-O2 -fomit-frame-pointer -m68040 -mhard-float`, Benchwork 1.0.

| benchmark | 6.5.0b | 16.2-rc13 |
|---|---:|---:|
| dhrystone | 512 | 639 |
| backdrop | 2891 | 2082 |
| lha-pack | 4431 | 4483 |
| lha-unpack | 1397 | 1449 |
| zlib-deflate | 3096 | 3628 |
| zlib-inflate | 465 | 563 |
| png-encode | 6541 | 7829 |
| png-decode | 1842 | 2110 |
| ftgrays | 834 | 859 |
| memcpy-small | 186 | 1097 |
| memcpy-large | 328 | 874 |
| memcpy-var-small | 821 | 947 |
| memcpy-var-large | 670 | 910 |
| memmove-small | 839 | 814 |
| memmove-large | 689 | 743 |
| wipeout-tris | 556 | 616 |
| **geomean** | **1023** | **1312** |

[^volamos]: volamos 0.8.0, `volamos --clock-mhz 25 --cpu 68040 --fpu <binary> -n 1`
    for every column. The emulated time is derived from the CPU's cycle count
    at 25 MHz; native library calls cost no cycles.
[^gcc]: `m68k-amigaos-gcc (AmigaDev v16.2-rc13) 16.2.0b 20260825082934`,
    the AmigaPorts 16.2-rc13 release, with its libnix. 6.5.0b is
    `m68k-amigaos-gcc (GCC) 6.5.0b 20260819091705`.
[^benchwork]: commit 2b33b37, which the 1.0 tag follows with only this file
    and `tools/results-table.py` added; the binaries report
    `benchwork 2b33b37`.
[^rom]: On the 68000 build, gcc calls `__divsi3`, `__mulsi3` and friends for
    32-bit multiply and divide, and libnix's versions jump into
    utility.library, which volamos runs natively at zero cycles: 13 million
    such calls in this run. The 020 and 040 builds use the CPU's
    instructions. These rows understate the 68000 cost of multiplies and
    divides, and are not comparable with real hardware.
[^softfloat]: The 000 and 020 builds are soft float, and libnix's float
    arithmetic is a stub into the mathieeesingbas ROM library, which volamos
    runs natively at zero cycles: 370,000 calls in wipeout-tris. Those two
    numbers measure everything in the benchmark except its float math.
