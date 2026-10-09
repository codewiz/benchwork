# Results

Fastest iteration in milliseconds under volamos[^volamos].
The cycle count makes all iterations identical.
Percentages are against the first column.

## Release binaries, m68k-amigaos-gcc 16.2-rc14[^gcc]

Benchwork 1.1[^benchwork], built with `make release`.

| benchmark | 000 | 020 | 040 |
|---|---:|---:|---:|
| dhrystone | 448 | 416 (-7.14%) | 417 (-6.96%) |
| backdrop[^rom] | 2857 | 1978 (-30.74%) | 1941 (-32.04%) |
| lha-pack[^rom] | 4942 | 4645 (-6.01%) | 4569 (-7.55%) |
| lha-unpack[^rom] | 1534 | 1452 (-5.35%) | 1441 (-6.03%) |
| zlib-deflate[^rom] | 3824 | 3663 (-4.20%) | 3625 (-5.20%) |
| zlib-inflate[^rom] | 585 | 541 (-7.57%) | 571 (-2.51%) |
| png-encode[^rom] | 7747 | 7717 (-0.38%) | 7527 (-2.85%) |
| png-decode[^rom] | 2107 | 1957 (-7.14%) | 1941 (-7.87%) |
| ftgrays | 988 | 855 (-13.49%) | 849 (-14.10%) |
| memcpy-small | 452 | 202 (-55.19%) | 193 (-57.23%) |
| memcpy-large | 820 | 327 (-60.11%) | 326 (-60.20%) |
| memcpy-var-small | 957 | 948 (-0.95%) | 951 (-0.57%) |
| memcpy-var-large | 847 | 835 (-1.34%) | 837 (-1.12%) |
| memmove-small | 1071 | 811 (-24.28%) | 811 (-24.28%) |
| memmove-large | 754 | 740 (-1.81%) | 740 (-1.81%) |
| wipeout-tris[^softfloat] | 1183 | 1135 (-4.09%) | 592 (-50.00%) |
| **GEOMEAN** | **1334** | **1107 (-17.02%)** | **1057 (-20.73%)** |

## m68k compiler comparison

All runs done with benchwork-040 built with `-O2 -fomit-frame-pointer -m68040
-mhard-float`, or the equivalent for vbcc and SAS/C. The fastest cell of each
row is in bold.

| benchmark | 6.5.0b | 16.2-rc14 | vbcc 0.9i[^vbcc] | SAS/C 6.58[^sasc] |
|---|---:|---:|---:|---:|
| dhrystone | 512 | **417 (-18.57%)** | 599 (+16.89%) | 581 (+13.38%) |
| backdrop | 2891 | **1941 (-32.87%)** | 4238 (+46.57%) | 4267 (+47.57%) |
| lha-pack | **4431** | 4569 (+3.13%) | 4870 (+9.91%) | 5022 (+13.34%) |
| lha-unpack | **1397** | 1441 (+3.15%) | 1457 (+4.28%) | 1507 (+7.84%) |
| zlib-deflate | **3096** | 3625 (+17.09%) | 4274 (+38.06%) | 4096 (+32.30%) |
| zlib-inflate | **465** | 571 (+22.68%) | 551 (+18.51%) | 492 (+5.76%) |
| png-encode | **6541** | 7527 (+15.07%) | 10221 (+56.27%) | 9483 (+44.99%) |
| png-decode | **1842** | 1941 (+5.40%) | 2357 (+27.98%) | 1985 (+7.78%) |
| ftgrays | **834** | 849 (+1.83%) | failed | 942 (+13.04%) |
| memcpy-small | **186** | 193 (+4.14%) | wrong output | 348 (+87.63%) |
| memcpy-large | 328 | **326 (-0.44%)** | 1267 (+286.43%) | 501 (+52.81%) |
| memcpy-var-small | 821 | 951 (+15.87%) | 654 (-20.30%) | **423 (-48.42%)** |
| memcpy-var-large | 670 | 837 (+24.92%) | 1616 (+141.13%) | **644 (-3.92%)** |
| memmove-small | 839 | **811 (-3.35%)** | 4320 (+414.66%) | 3110 (+270.54%) |
| memmove-large | **689** | 740 (+7.48%) | 12458 (+1708.63%) | 8911 (+1193.77%) |
| wipeout-tris | **556** | 592 (+6.32%) | 943 (+69.40%) | 672 (+20.83%) |
| **GEOMEAN** | **1023** | **1057 (+3.41%)** | | **1491 (+45.78%)** |

[^volamos]: volamos 0.8.0, `volamos --clock-mhz 25 --cpu 68040 --fpu <binary> -n 1`
    for every column. The emulated time is derived from the CPU's cycle count
    at 25 MHz; native library calls cost no cycles.
[^gcc]: `m68k-amigaos-gcc (AmigaDev v16.2-rc14) 16.2.0b 20260825082934`,
    the AmigaPorts 16.2-rc14 release, with its libnix. 6.5.0b is
    `m68k-amigaos-gcc (GCC) 6.5.0b 20260819091705`.
[^vbcc]: `vbcc V0.9i pre` from the same toolchain, `make vbcc-release`:
    `vc +aos68k -O2 -cpu=68040 -fpu=68040` with vc.lib, m040.lib and
    amiga.lib. vbcc 0.9i miscompiles two of the drivers: ftgrays fails
    before it runs, and memcpy-small produces a wrong checksum and writes
    outside its buffers, which is why no geomean is given.
[^sasc]: SAS/C 6.58, `make sasc-release`: `CPU=68040 MATH=68881` with
    `OPTIMIZE OPTIMIZERTIME OPTIMIZERINLINELOCAL OPTIMIZERSCHEDULER` and the
    optimizer depths at 8, `PARAMETERS=REGISTERS CODE=FAR DATA=FAR`, linked
    with scm040.lib, sc.lib and amiga.lib. Its memcpy and memmove are the
    C library's own when the harness does not supply them.
[^benchwork]: Benchwork 1.1.
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
