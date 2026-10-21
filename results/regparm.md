# GCC register arguments

Part of the [Benchwork results](../RESULTS.md).

benchwork-040 built with m68k-amigaos-gcc 16.2-rc18[^rc18] and
`-O2 -fomit-frame-pointer -m68040 -m68881`, with and without
`-mregparm`[^regparm], on the 68040[^volamos].

Passing arguments in registers makes the code 3% smaller and speeds up
call-heavy code (Dhrystone 10%, LHa 2%). The losses of up to 4% are
Benchwork's own fault: the checksum each run takes of its output loses
its `dbra` loop, about 2 cycles per byte, which is nearly all that
zlib-inflate, png-decode, backdrop and wipeout-tris lose; their own code
changes by about 1 ms at most. Allowing four register arguments instead of two
changes little, and so does `-msasregparm`, which puts a third integer or
pointer argument on the stack instead of in a register of the other kind.

| benchmark | stack args | -mregparm=2 | -mregparm=4 | -msasregparm |
|---|---:|---:|---:|---:|
| dhrystone | 420 | **378 (-10.09%)** | **378 (-10.09%)** | **378 (-10.09%)** |
| backdrop | **3137** | 3163 (+0.84%) | 3163 (+0.84%) | 3163 (+0.84%) |
| lha-pack | 4439 | 4319 (-2.70%) | 4323 (-2.62%) | **4305 (-3.01%)** |
| lha-unpack | 1383 | **1356 (-1.94%)** | **1356 (-1.94%)** | **1356 (-1.94%)** |
| zlib-deflate | **3698** | **3698 (-0.02%)** | **3698 (-0.02%)** | **3698 (-0.02%)** |
| zlib-inflate | **509** | 530 (+4.12%) | 530 (+4.11%) | 530 (+4.12%) |
| png-encode | **2247** | 2260 (+0.57%) | 2260 (+0.58%) | 2260 (+0.58%) |
| png-decode | **501** | 521 (+3.98%) | 518 (+3.39%) | 520 (+3.78%) |
| ftgrays | **858** | 874 (+1.82%) | 868 (+1.06%) | 874 (+1.85%) |
| memcpy-small | **193** | 198 (+2.71%) | 198 (+2.71%) | 198 (+2.71%) |
| memcpy-large | **326** | 332 (+1.61%) | 332 (+1.61%) | 332 (+1.61%) |
| memcpy-var-small | **565** | 570 (+0.93%) | 570 (+0.93%) | 570 (+0.93%) |
| memcpy-var-large | **635** | 640 (+0.83%) | 640 (+0.83%) | 640 (+0.83%) |
| memmove-small | **1920** | 1925 (+0.27%) | 1925 (+0.27%) | 1925 (+0.27%) |
| memmove-large | **1481** | 1486 (+0.35%) | 1486 (+0.35%) | 1486 (+0.35%) |
| wipeout-tris | **518** | 540 (+4.11%) | 540 (+4.11%) | 540 (+4.11%) |
| nfs3-project | 1064 | **1050 (-1.32%)** | 1054 (-0.94%) | 1052 (-1.13%) |
| nfs3-clip | 952 | **948 (-0.37%)** | **948 (-0.37%)** | **948 (-0.37%)** |
| sha512 | 960 | **954 (-0.55%)** | **954 (-0.55%)** | 960 (+0.07%) |
| aesgcm | **903** | **903 (-0.01%)** | 904 (+0.07%) | 904 (+0.09%) |
| chacha20poly1305 | **450** | 456 (+1.40%) | 456 (+1.40%) | 456 (+1.40%) |
| x25519 | 547 | 549 (+0.39%) | **546 (-0.18%)** | **546 (-0.14%)** |
| **GEOMEAN** | **903** | **905 (+0.27%)** | **905 (+0.21%)** | **905 (+0.27%)** |
| **CODE SIZE** | 401960 | 390248 (-2.91%) | 388808 (-3.27%) | 391020 (-2.72%) |

[^volamos]: `volamos --clock-mhz 25 --cpu 68040 --fpu <binary> -n 2`.
    The emulated time is derived from the CPU's cycle count at 25 MHz;
    native library calls cost no cycles.
[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^regparm]: The whole program is built with `-mregparm`; newlib, which
    is not, keeps stack arguments because its headers declare its
    functions `__stdargs`, and so does `main()`, which the startup code
    calls with stack arguments.
