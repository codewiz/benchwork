# newlib string functions

`memcpy.c`, `memset.c` and `strcmp.c` are newlib's portable C
implementations from `newlib/libc/string` of
AmigaPorts/newlib-cygwin at d8eff91f1 (2026-09-25). They are linked into
every Benchwork binary ahead of the C library, so the out-of-line string
calls the benchmarks and the compiler make are code the compiler under test
generated, not the C library's: with libnix, `memcpy` otherwise reaches exec
`CopyMem`, whose speed is the ROM's, and the rest of libnix.a was compiled
by whatever built the toolchain.

`local.h` stands in for newlib's internal header; `memcpy.c` drops the
`_ansi.h` include and spells `restrict` the C99 way.
