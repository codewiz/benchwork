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

The chart shows each benchmark's time in ms, so lower is faster, and
skips the releases in which no benchmark changes by more than 1% from the
release before. Its vertical axis is logarithmic, so equal slopes are
equal percentages.

![Time of each benchmark in the 16.2 releases from rc5 to rc18, in ms](gcc-releases-040.svg)

| benchmark | rc5 | rc11 | rc14 | rc17 | rc18 |
|---|---:|---:|---:|---:|---:|
| dhrystone | 509 | 509 | 415 (-18.54%) | 415 (-18.54%) | **414 (-18.70%)** |
| scene-2d | 1159 | 1147 (-1.08%) | **1032 (-10.96%)** | **1032 (-10.96%)** | **1032 (-10.96%)** |
| quantize-floyd | 863 | 865 (+0.19%) | **659 (-23.70%)** | **659 (-23.70%)** | **659 (-23.70%)** |
| lha-pack | **1026** | 1095 (+6.69%) | 1110 (+8.14%) | 1110 (+8.14%) | 1110 (+8.14%) |
| lha-unpack | **285** | 294 (+3.40%) | 301 (+5.77%) | 301 (+5.77%) | 301 (+5.83%) |
| zlib-deflate | 829 | 829 (-0.05%) | **804 (-3.00%)** | **804 (-3.00%)** | **804 (-3.00%)** |
| zlib-inflate | **216** | **216 (-0.04%)** | 238 (+10.27%) | 238 (+10.27%) | 237 (+9.56%) |
| png-encode | 1378 | 1378 (-0.01%) | **1324 (-3.87%)** | **1324 (-3.87%)** | **1324 (-3.87%)** |
| png-decode | 1048 | 1021 (-2.61%) | **985 (-6.00%)** | **985 (-6.00%)** | **985 (-6.00%)** |
| ftgrays | **651** | 653 (+0.26%) | 662 (+1.68%) | 662 (+1.68%) | 662 (+1.69%) |
| memcpy-small | 2815 | 2589 (-8.02%) | **908 (-67.75%)** | **908 (-67.75%)** | **908 (-67.75%)** |
| memcpy-large | 461 | 461 | **285 (-38.23%)** | **285 (-38.23%)** | **285 (-38.23%)** |
| memcpy-var-small | **515** | **515** | 523 (+1.54%) | 523 (+1.54%) | 523 (+1.54%) |
| memcpy-var-large | **592** | **592** | 593 (+0.19%) | 593 (+0.19%) | 593 (+0.19%) |
| memmove-small | 1220 | 1335 (+9.44%) | 1155 (-5.35%) | 1155 (-5.35%) | **939 (-23.03%)** |
| memmove-large | 3339 | 3349 (+0.32%) | 2723 (-18.44%) | 2723 (-18.44%) | **959 (-71.27%)** |
| wipeout-tris | 1329 | 895 (-32.62%) | 894 (-32.70%) | 894 (-32.70%) | **893 (-32.77%)** |
| nfs3-project | **1002** | 1066 (+6.39%) | 1064 (+6.19%) | 1064 (+6.19%) | 1064 (+6.19%) |
| nfs3-clip | 962 | 963 (+0.03%) | **951 (-1.17%)** | **951 (-1.17%)** | 952 (-1.10%) |
| sha512 | **957** | 1226 (+28.13%) | 1226 (+28.14%) | 1038 (+8.54%) | 960 (+0.32%) |
| aesgcm | 1103 | 1187 (+7.68%) | 1154 (+4.70%) | **852 (-22.69%)** | 853 (-22.66%) |
| chacha20poly1305 | **379** | 392 (+3.26%) | 399 (+5.34%) | 399 (+5.34%) | 399 (+5.34%) |
| x25519 | **494** | 515 (+4.25%) | 546 (+10.64%) | 547 (+10.81%) | 547 (+10.76%) |
| **GEOMEAN** | **819** | **824 (+0.60%)** | **744 (-9.24%)** | **729 (-11.07%)** | **687 (-16.09%)** |
| **CODE SIZE** | 450536 | 438216 (-2.73%) | 414528 (-7.99%) | 405056 (-10.09%) | 405196 (-10.06%) |

[^releases]: The released x86_64 Linux tarballs, with their default newlib
    runtime.
