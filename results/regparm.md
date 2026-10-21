# GCC register arguments

Part of the [Benchwork results](README.md); see there for how they are
measured.

benchwork-040 built with m68k-amigaos-gcc 16.2-rc18[^rc18] and
`-O2 -fomit-frame-pointer -m68040 -m68881`, with and without
`-mregparm`[^regparm], on the 68040[^volamos].

Passing arguments in registers makes the code 3% smaller and Benchwork 1%
faster: call-heavy code gains most (Dhrystone 10%, LHa 3 to 4%, ftgrays 1
to 2%), and no benchmark loses more than 0.5%. Allowing four register
arguments instead of two changes little, and so does `-msasregparm`, which
puts a third integer or pointer argument on the stack instead of in a
register of the other kind.

| benchmark | stack args | -mregparm=2 | -mregparm=4 | -msasregparm |
|---|---:|---:|---:|---:|
| dhrystone | 414 | **371 (-10.43%)** | **371 (-10.43%)** | **371 (-10.43%)** |
| backdrop | **2927** | **2927** | **2927** | **2927** |
| lha-pack | 4377 | 4249 (-2.91%) | 4253 (-2.84%) | **4235 (-3.23%)** |
| lha-unpack | 1181 | **1133 (-4.05%)** | **1133 (-4.05%)** | **1133 (-4.05%)** |
| zlib-deflate | 3637 | **3628 (-0.23%)** | 3629 (-0.23%) | 3629 (-0.23%) |
| zlib-inflate | **920** | **920 (-0.01%)** | **920 (-0.02%)** | **920 (-0.01%)** |
| png-encode | **2231** | 2242 (+0.48%) | 2242 (+0.50%) | 2242 (+0.50%) |
| png-decode | 934 | 935 (+0.09%) | **926 (-0.87%)** | 932 (-0.23%) |
| ftgrays | 662 | 653 (-1.35%) | **646 (-2.33%)** | 653 (-1.31%) |
| memcpy-small | **908** | **908** | **908** | **908** |
| memcpy-large | **285** | **285** | **285** | **285** |
| memcpy-var-small | **523** | **523** | **523** | **523** |
| memcpy-var-large | **593** | **593** | **593** | **593** |
| memmove-small | **1878** | **1878** | **1878** | **1878** |
| memmove-large | **1439** | **1439** | **1439** | **1439** |
| wipeout-tris | 893 | **892 (-0.10%)** | **892 (-0.10%)** | **892 (-0.10%)** |
| nfs3-project | 1064 | **1050 (-1.32%)** | 1054 (-0.94%) | 1052 (-1.13%) |
| nfs3-clip | 952 | **948 (-0.37%)** | **948 (-0.37%)** | **948 (-0.37%)** |
| sha512 | **960** | **960 (+0.06%)** | **960 (+0.06%)** | **960 (+0.06%)** |
| aesgcm | 853 | **847 (-0.62%)** | 848 (-0.54%) | 848 (-0.52%) |
| chacha20poly1305 | **399** | 400 (+0.27%) | 400 (+0.27%) | 400 (+0.27%) |
| x25519 | 547 | 549 (+0.39%) | **546 (-0.18%)** | **546 (-0.14%)** |
| **GEOMEAN** | **1001** | **991 (-0.94%)** | **990 (-1.03%)** | **991 (-0.98%)** |
| **CODE SIZE** | 402696 | 390956 (-2.92%) | 389476 (-3.28%) | 391732 (-2.72%) |

[^volamos]: `volamos --clock-mhz 25 --cpu 68040 --fpu <binary> -n 2`.
    The emulated time is derived from the CPU's cycle count at 25 MHz;
    native library calls cost no cycles.
[^rc18]: `m68k-amigaos-gcc (AmigaDev v16.2-rc18) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^regparm]: The whole program is built with `-mregparm`; newlib, which
    is not, keeps stack arguments because its headers declare its
    functions `__stdargs`, and so does `main()`, which the startup code
    calls with stack arguments.
