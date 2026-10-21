# Comparing GCC 16.2 releases

Part of the [Benchwork results](README.md); see there for how they are
measured.

benchwork-040 built with `-O2 -fomit-frame-pointer -m68040 -m68881` by each
AmigaPorts release[^releases] since rc5, the oldest one still published:

- rc11: LRA by default, small constant-size copies inlined.
- rc14: Bebbo's code generation work (costs, dbra, inline block copies).
- rc17: GCC 16.2.1, 64-bit integer operations split into 32-bit ones.
- rc18: newlib upstream merge (memmove), 68040 cost fix (sha512),
  `-m68020-60` multilibs.

The chart skips the releases in which no benchmark changes by more than 1%
from the release before.
Its vertical axis is logarithmic, so equal slopes are equal percentages.

![Time of each benchmark in the 16.2 releases from rc5 to rc18](gcc-releases-040.svg)

| benchmark | rc5 | rc11 | rc14 | rc17 | rc18 |
|---|---:|---:|---:|---:|---:|
| dhrystone | 509 | 509 | 415 (-18.54%) | 415 (-18.54%) | **414 (-18.70%)** |
| backdrop | 3545 | 3575 (+0.85%) | **2927 (-17.44%)** | **2927 (-17.44%)** | **2927 (-17.44%)** |
| lha-pack | **4042** | 4314 (+6.73%) | 4377 (+8.28%) | 4377 (+8.28%) | 4377 (+8.28%) |
| lha-unpack | **1115** | 1153 (+3.40%) | 1180 (+5.86%) | 1180 (+5.86%) | 1181 (+5.92%) |
| zlib-deflate | 3758 | 3757 (-0.05%) | **3637 (-3.23%)** | **3637 (-3.23%)** | **3637 (-3.23%)** |
| zlib-inflate | **839** | **839 (-0.03%)** | 926 (+10.35%) | 926 (+10.35%) | 920 (+9.65%) |
| png-encode | 2314 | 2312 (-0.07%) | **2231 (-3.60%)** | **2231 (-3.60%)** | **2231 (-3.60%)** |
| png-decode | 1000 | 985 (-1.52%) | **933 (-6.72%)** | **933 (-6.72%)** | 934 (-6.60%) |
| ftgrays | **651** | 653 (+0.26%) | 662 (+1.68%) | 662 (+1.68%) | 662 (+1.69%) |
| memcpy-small | 2815 | 2589 (-8.02%) | **908 (-67.75%)** | **908 (-67.75%)** | **908 (-67.75%)** |
| memcpy-large | 461 | 461 | **285 (-38.23%)** | **285 (-38.23%)** | **285 (-38.23%)** |
| memcpy-var-small | **515** | **515** | 523 (+1.54%) | 523 (+1.54%) | 523 (+1.54%) |
| memcpy-var-large | **592** | **592** | 593 (+0.19%) | 593 (+0.19%) | 593 (+0.19%) |
| memmove-small | 2440 | 2670 (+9.44%) | 2309 (-5.35%) | 2309 (-5.35%) | **1878 (-23.03%)** |
| memmove-large | 5008 | 5024 (+0.32%) | 4085 (-18.44%) | 4085 (-18.44%) | **1439 (-71.27%)** |
| wipeout-tris | 1329 | 895 (-32.62%) | 894 (-32.70%) | 894 (-32.70%) | **893 (-32.77%)** |
| nfs3-project | **1002** | 1066 (+6.39%) | 1064 (+6.19%) | 1064 (+6.19%) | 1064 (+6.19%) |
| nfs3-clip | 962 | 963 (+0.03%) | **951 (-1.17%)** | **951 (-1.17%)** | 952 (-1.10%) |
| sha512 | **957** | 1234 (+29.03%) | 1226 (+28.14%) | 1038 (+8.54%) | 960 (+0.32%) |
| aesgcm | 1103 | 1187 (+7.68%) | 1154 (+4.70%) | **852 (-22.69%)** | 853 (-22.66%) |
| chacha20poly1305 | **379** | 392 (+3.26%) | 399 (+5.34%) | 399 (+5.34%) | 399 (+5.34%) |
| x25519 | **494** | 515 (+4.25%) | 546 (+10.64%) | 547 (+10.81%) | 547 (+10.76%) |
| **GEOMEAN** | **1191** | **1201 (+0.79%)** | **1086 (-8.85%)** | **1063 (-10.77%)** | **1001 (-16.03%)** |
| **CODE SIZE** | 447512 | 435228 (-2.74%) | 412024 (-7.93%) | 402552 (-10.05%) | 402684 (-10.02%) |

[^releases]: The released x86_64 Linux tarballs, with their default newlib
    runtime.
