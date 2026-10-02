# newlib string functions

`memcpy.c`, `memset.c` and `strcmp.c` are newlib's portable C
implementations, unchanged, from `newlib/libc/string` of
AmigaPorts/newlib-cygwin at d8eff91f1 (2026-09-25). They are linked into
every BenchWork binary ahead of the C library, so the out-of-line string
calls the benchmarks and the compiler make are code the compiler under test
generated, not the C library's: with libnix, `memcpy` otherwise reaches exec
`CopyMem`, whose speed is the ROM's, and the rest of libnix.a was compiled
by whatever built the toolchain.

`_ansi.h` and `local.h` stand in for newlib's internal headers.
