# Benchwork

CPU benchmarks for comparing m68k compilers, built from code that Amiga
programs actually run.

Everything works on memory buffers, so the timed region contains only
compiler-generated code: no disk, display or OS calls.[^libc]
Numbers for the release binaries and a compiler comparison are in
[RESULTS.md](RESULTS.md).

| benchmark | source | what it measures |
|---|---|---|
| dhrystone | Dhrystone 2.1, as used by xSysInfo | the classic integer mix |
| backdrop | p96cts's dithered landscape scene | per-pixel integer arithmetic, divides, byte stores |
| lha-pack | LHa for UNIX 1.14i, -lh5- | hash-chain match search, Huffman coding, K&R-era C |
| lha-unpack | LHa for UNIX 1.14i, -lh5- | Huffman decoding, sliding-window copies |
| zlib-deflate | zlib 1.3.2 | deflate's longest_match() |
| zlib-inflate | zlib 1.3.2 | inflate's bit-level state machine |
| png-encode | libpng 1.6.58 + zlib | filter selection byte loops |
| png-decode | libpng 1.6.58 + zlib | filter reconstruction byte loops |
| ftgrays | FreeType 2.12.1 smooth rasterizer | fixed-point curve subdivision, cell sweep, switch-heavy code |
| memcpy-small | constant-size copies up to 128 bytes | by-pieces expansion[^pieces] |
| memcpy-large | constant-size copies from 256 to 4096 bytes | an expander taking over from the memcpy call |
| memcpy-var-small | small copies sized at run time | the memcpy call[^libc] |
| memcpy-var-large | large copies sized at run time | memcpy's copy loop |
| memmove-small | overlapping moves up to 128 bytes, both ways | the backwards path a block-move expander gets wrong |
| memmove-large | overlapping moves of 2048 and 4096 bytes, both ways | the same past the by-pieces limit |
| wipeout-tris | render_push_tris() from arczi84's Wipeout port | a game's per-triangle path: float UV scaling, byte clamps, 96-byte struct copies |

[^libc]: `memcpy`, `memset` and `strcmp` are newlib's C versions linked ahead
    of the C library, so they too are compiled by the compiler under test;
    libnix's memcpy would hand the work to exec `CopyMem`.
[^pieces]: 128 bytes is the size up to which the AmigaOS by-pieces hook
    expands a copy inline.

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
benchwork [-n iterations] [--fastmem] [-l] [name ...]
```

The FreeType rasterizer keeps its cell pool on the stack, so the program
refuses to start on the shell's default 4 KiB stack: `stack 65536` first.

Each benchmark reports its fastest and mean iteration in milliseconds, and a
checksum of its output that must be identical across compilers.

## License

The harness is 0BSD. The vendored code keeps its own license, in each
`third_party/` directory:
* newlib string functions (BSD, Red Hat)
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
