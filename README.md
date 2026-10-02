# BenchWork

CPU benchmarks for comparing m68k compilers, built from code that Amiga
programs actually run.

Everything works on memory buffers, so the timed region contains only
compiler-generated code: no disk, display or OS calls.

| benchmark | source | what it measures |
|---|---|---|
| dhrystone | Dhrystone 2.1, as used by xSysInfo | the classic integer mix |
| backdrop | p96cts's dithered landscape scene | per-pixel integer arithmetic, divides, byte stores |
| lha-pack, lha-unpack | LHa for UNIX 1.14i, -lh5- | hash-chain match search, Huffman coding, K&R-era C |
| zlib-deflate, zlib-inflate | zlib 1.3.2 | deflate's longest_match(), inflate's bit-level state machine |
| png-encode, png-decode | libpng 1.6.58 + zlib | libpng's filter selection and reconstruction byte loops |
| ftgrays | FreeType 2.12.1 smooth rasterizer | fixed-point curve subdivision, cell sweep, switch-heavy code |
| memcpy-small, memcpy-large | constant-size block copies, split at 128 bytes | by-pieces expansion below the limit, an expander taking over from the library above it |
| memcpy-var-small, memcpy-var-large | copies sized at run time | the library memcpy, which on AmigaOS reaches exec CopyMem: call overhead and throughput |
| memmove-small, memmove-large | overlapping moves both ways | the backwards path a block-move expander gets wrong |

## Building

The compiler is taken from `PATH`; override `CC` to compare toolchains, and
`BUILD` to keep their objects apart:

```
make CC=gcc-6.5/bin/m68k-amigaos-gcc BUILD=build-gcc6 TARGET=benchwork-gcc6
```

`CPUFLAGS` (default `-m68020-60`) and `OPT` (default `-O2 -fomit-frame-pointer`)
are the knobs a comparison usually turns.

## Running

```
benchwork [-n iterations] [-l] [name ...]
```

The FreeType rasterizer keeps its cell pool on the stack, so the program
refuses to start on the shell's default 4 KiB stack: `stack 65536` first.

Each benchmark reports its fastest and mean iteration in milliseconds, and a
checksum of its output that must be identical across compilers. The timer is
`ReadEClock()`.

## License

The harness is 0BSD. The vendored code keeps its own license, in each
`third_party/` directory:
* zlib (zlib)
* libpng (PNG Reference Library License)
* FreeType (FTL)
* LHa for UNIX (its redistribution terms, in Japanese)
* Dhrystone (Reinhold Weicker's original terms) via xSysInfo (BSD-2-Clause)
* The glyph outlines in `src/glyphs.c` are derived from DejaVu Sans (Bitstream Vera license).

Portions of this software are copyright (C) 1996-2022 The FreeType Project
(www.freetype.org). All rights reserved.

Because of the LHa terms, binaries must be distributed together with this
source, and the program must not be used as the main product of a
commercial offering.
