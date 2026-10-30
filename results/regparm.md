# GCC register arguments

Part of the [Benchwork results](README.md); see there for how they are
measured.

benchwork-040 built with m68k-amigaos-gcc 16.2-rc21[^rc21] and
`-O2 -fomit-frame-pointer -m68040 -m68881`, with and without
`-mregparm`[^regparm], on the 68040[^volamos].

Passing arguments in registers makes the code 3% smaller and Benchwork 1%
faster: call-heavy code gains most (Dhrystone 10%, LHa 2 to 3%, ftgrays 1
to 2%), and no benchmark loses more than 0.4% (x25519). Allowing four register
arguments instead of two changes little, and so does `-msasregparm`, which
puts a third integer or pointer argument on the stack instead of in a
register of the other kind.

Each bar is the stack-argument build's time divided by the build's, so
longer is faster; stack arguments are the line at 1.0.

![Speed of each benchmark relative to stack arguments](regparm-040.svg)

| benchmark | stack args | -mregparm=2 | -mregparm=4 | -msasregparm |
|---|---:|---:|---:|---:|
| dhrystone | 414 | **371 (-10.43%)** | **371 (-10.43%)** | **371 (-10.43%)** |
| scene-2d | **1032** | **1032** | **1032** | **1032** |
| quantize-floyd | **655** | **655** | **655** | **655** |
| lha-pack | 1110 | 1077 (-2.99%) | 1083 (-2.46%) | **1075 (-3.13%)** |
| lha-unpack | 296 | **289 (-2.36%)** | **289 (-2.36%)** | **289 (-2.36%)** |
| zlib-deflate | 804 | **800 (-0.61%)** | **800 (-0.61%)** | **800 (-0.61%)** |
| zlib-inflate | **233** | **233 (-0.01%)** | **233 (-0.05%)** | **233 (-0.01%)** |
| png-encode | 1324 | **1323 (-0.08%)** | **1323 (-0.08%)** | **1323 (-0.08%)** |
| png-decode | 985 | 985 | **983 (-0.15%)** | 984 (-0.07%) |
| ftgrays | 661 | 652 (-1.35%) | **646 (-2.34%)** | 652 (-1.31%) |
| memcpy-small | **908** | **908** | **908** | **908** |
| memcpy-large | **285** | **285** | **285** | **285** |
| memcpy-var-small | **523** | **523** | **523** | **523** |
| memcpy-var-large | **593** | **593** | **593** | **593** |
| memmove-small | **924** | **924** | **924** | **924** |
| memmove-large | **958** | **958** | **958** | **958** |
| wipeout-tris | 893 | **892 (-0.10%)** | **892 (-0.10%)** | **892 (-0.10%)** |
| nfs3-project | 1064 | **1050 (-1.32%)** | 1054 (-0.94%) | 1052 (-1.13%) |
| nfs3-clip | 952 | **948 (-0.37%)** | **948 (-0.37%)** | **948 (-0.37%)** |
| sha512 | 959 | **954 (-0.55%)** | 960 (+0.06%) | 960 (+0.06%) |
| aesgcm | 826 | **824 (-0.21%)** | 825 (-0.12%) | 825 (-0.09%) |
| chacha20poly1305 | **399** | 400 (+0.27%) | 400 (+0.27%) | 400 (+0.27%) |
| x25519 | 546 | 548 (+0.35%) | **544 (-0.24%)** | 545 (-0.22%) |
| **GEOMEAN** | **685** | **679 (-0.89%)** | **679 (-0.89%)** | **679 (-0.88%)** |
| **CODE SIZE** | 408308 | 396596 (-2.87%) | 395176 (-3.22%) | 397280 (-2.70%) |

[^volamos]: `volamos --clock-mhz 25 --cpu 68040 --fpu <binary> REPEAT 2`,
    each repetition looping for at least a second (`MINTIME`'s default).
    The emulated time is derived from the CPU's cycle count at 25 MHz;
    native library calls cost no cycles.
[^rc21]: `m68k-amigaos-gcc (AmigaDev 16.2-rc21) 16.2.1b 20261010182711`,
    with its default newlib runtime.
[^regparm]: The whole program is built with `-mregparm`; newlib, which
    is not, keeps stack arguments because its headers declare its
    functions `__stdargs`.
