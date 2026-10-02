# Results

Fastest iteration in milliseconds under volamos[^volamos], one iteration
each; the cycle count makes a second one identical. Check values agree
across every column. Percentages are against the first column.

## Release binaries, m68k-amigaos-gcc 16.2-rc13[^gcc]

Benchwork 1.0[^benchwork], built with `make release`.

| benchmark | 000 | 020 | 040 |
|---|---:|---:|---:|
| dhrystone | 608 | 624 (+2.60%) | 639 (+5.10%) |
| backdrop[^rom] | 2981 | 2010 (-32.56%) | 2082 (-30.15%) |
| lha-pack[^rom] | 5036 | 4515 (-10.35%) | 4483 (-10.99%) |
| lha-unpack[^rom] | 1549 | 1483 (-4.28%) | 1449 (-6.51%) |
| zlib-deflate[^rom] | 3874 | 3678 (-5.07%) | 3628 (-6.37%) |
| zlib-inflate[^rom] | 617 | 563 (-8.75%) | 563 (-8.73%) |
| png-encode[^rom] | 8169 | 7812 (-4.37%) | 7829 (-4.16%) |
| png-decode[^rom] | 2335 | 2107 (-9.79%) | 2110 (-9.65%) |
| ftgrays | 994 | 865 (-12.93%) | 859 (-13.58%) |
| memcpy-small | 1169 | 1117 (-4.48%) | 1097 (-6.19%) |
| memcpy-large | 1220 | 877 (-28.08%) | 874 (-28.36%) |
| memcpy-var-small | 1079 | 979 (-9.29%) | 947 (-12.29%) |
| memcpy-var-large | 1377 | 915 (-33.53%) | 910 (-33.91%) |
| memmove-small | 1069 | 814 (-23.85%) | 814 (-23.85%) |
| memmove-large | 751 | 743 (-1.12%) | 743 (-1.12%) |
| wipeout-tris[^softfloat] | 1180 | 1156 (-2.09%) | 616 (-47.77%) |
| **GEOMEAN** | **1564** | **1369 (-12.50%)** | **1312 (-16.14%)** |

## Compilers, benchwork-040 flags

`-O2 -fomit-frame-pointer -m68040 -mhard-float`, Benchwork 1.0.

| benchmark | 6.5.0b | 16.2-rc13 |
|---|---:|---:|
| dhrystone | 512 | 639 (+24.78%) |
| backdrop | 2891 | 2082 (-27.99%) |
| lha-pack | 4431 | 4483 (+1.17%) |
| lha-unpack | 1397 | 1449 (+3.68%) |
| zlib-deflate | 3096 | 3628 (+17.18%) |
| zlib-inflate | 465 | 563 (+20.95%) |
| png-encode | 6541 | 7829 (+19.70%) |
| png-decode | 1842 | 2110 (+14.55%) |
| ftgrays | 834 | 859 (+3.05%) |
| memcpy-small | 186 | 1097 (+490.96%) |
| memcpy-large | 328 | 874 (+166.48%) |
| memcpy-var-small | 821 | 947 (+15.32%) |
| memcpy-var-large | 670 | 910 (+35.82%) |
| memmove-small | 839 | 814 (-3.04%) |
| memmove-large | 689 | 743 (+7.86%) |
| wipeout-tris | 556 | 616 (+10.80%) |
| **GEOMEAN** | **1023** | **1312 (+28.31%)** |

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
    instructions. The 000 column understates the cost of multiplies and
    divides in these rows, and is not comparable with real hardware.
[^softfloat]: The 000 and 020 builds are soft float, and libnix's float
    arithmetic is a stub into the mathieeesingbas ROM library, which volamos
    runs natively at zero cycles: 370,000 calls in wipeout-tris. Those two
    columns measure everything in the benchmark except its float math.
