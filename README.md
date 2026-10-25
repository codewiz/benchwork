# Benchwork

CPU benchmarks for comparing m68k compilers, built from code that Amiga
programs actually run.

Everything works on memory buffers, so the timed region contains only
compiler-generated code: no disk, display or OS calls.[^libc]

Results for compilers, GCC releases and options, CPUs and emulators are in
[results/](results/README.md).

[![Time of each benchmark across the GCC 16.2 releases](results/gcc-releases-040.svg)](results/gcc-releases.md)

## Benchmarks

| benchmark | source | what it measures |
|---|---|---|
| dhrystone | Dhrystone 2.1, as used by xSysInfo | the classic integer mix |
| scene-2d | p96cts's landscape scene, rendered at 640x200 | per-pixel integer arithmetic, divides, byte stores |
| quantize-floyd | the scene remapped to a fixed 32-color palette | Floyd-Steinberg error diffusion, a nearest-color search with an inverse colormap |
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
| nfs3-project | projectObjectVertex() from arczi84's NFS3 port (C++, gcc only) | float multiply-add chains, a perspective divide, bit tests on float results |
| nfs3-clip | PolygonClipWork::run() from arczi84's NFS3 port (C++, gcc only) | triangle clipping: float compares, divides, interpolation |
| sha512 | AmiSSL's OpenSSL 4.0.3 SHA-512 and SHA-384 (gcc and vbcc) | 64-bit rotates, shifts and adds split into 32-bit halves |
| aesgcm | AmiSSL's OpenSSL 4.0.3 AES-128-GCM on TLS 1.3 records (gcc and vbcc) | AES table lookups, then GHASH's 64-bit shifts and XORs (about 60% of the time) |
| chacha20poly1305 | AmiSSL's OpenSSL 4.0.3 ChaCha20-Poly1305 on TLS 1.3 records (gcc and vbcc) | ChaCha20's 32-bit adds, XORs and rotates, then Poly1305's 32x32->64 multiplies (about 47% of the time) |
| x25519 | AmiSSL's OpenSSL 4.0.3 X25519, both ends of a TLS key exchange (gcc and vbcc) | signed 32x32->64 multiplies and carries over ten 26-bit limbs, constant-time table lookups |

[^libc]: Out-of-line `memcpy`, `memset` and `memmove` are each compiler's
    own C library's, so these rows compare the compiler together with its
    runtime. gcc links the toolchain's default newlib, whose memcpy is
    assembler; libnix's hands the work to exec `CopyMem`, which an
    emulator may run for free, so the gcc builds use newlib.
[^pieces]: 128 bytes is the size up to which the AmigaOS by-pieces hook
    expands a copy inline.


## Building

### m68k-amigaos-gcc

The compiler defaults to `m68k-amigaos-gcc` from the PATH; override `CC`
to compare toolchains, and `BUILD` to keep their objects apart:

```
make CC=gcc-6.5/bin/m68k-amigaos-gcc BUILD=build-gcc6 TARGET=benchwork-gcc6
```

`CPUFLAGS` (default `-m68020-60`) and `OPT` (default `-O2 -fomit-frame-pointer`)
are the knobs a comparison usually turns.


### VBCC

vbcc from the same toolchain builds `benchwork-vbcc` with `make vbcc`, and
the release binaries `benchwork-vbcc-000` and so on with `make vbcc-release`;
the flags are spelled its way:

```
make vbcc CPUFLAGS=-cpu=68020 OPT=-O3
```

NOTE: vbcc needs its source snapshot of 2026-01-12 or later; older ones
miscompile ftgrays. Until the compiler is fixed, `x25519` reports `setup
failed` from `-O1` up, and `memcpy-small` gives a wrong checksum and
scribbles over memory with `-O2 -cpu=68040` (but not with `-O3`).
`-O3` crashes vbcc on Wipeout's kernel.c, so the Makefile builds that file
at `-O2`.


### SAS/C

SAS/C 6.58 builds `benchwork-sasc` with `make sasc`, and the release
binaries with `make sasc-release`; `SC=sc-volamos` names a wrapper that
runs sc under volamos, with slink reached the same way.


## Running

The release archive has one binary per CPU: benchwork-000, -020 (68020
and 68030), -020-060fp (any 68020 to 68060 with an FPU), -040 and -060.

```
benchwork BENCH/M,REPEAT/K/N,MINTIME/K/N,FASTMEM/S,LIST/S,HELP/S
```

- `BENCH`: the benchmarks to run, in this order; all of them when none is
  named.
- `REPEAT`: timed repetitions of each benchmark, 3 by default.
- `MINTIME`: loop over the work until each repetition lasts this many
  milliseconds; see below.
- `FASTMEM`: put the benchmark buffers in fast memory, and fail if there is
  not enough.
- `LIST`: list the benchmarks.
- `HELP`: describe the arguments; `-h` and `--help` work too.

For example, `benchwork REPEAT 5 dhrystone ftgrays`.

The FreeType rasterizer keeps its cell pool on the stack, so the program
refuses to start on the shell's default 4 KB stack: `stack 65536` first.

Each benchmark reports the time of one loop over its work in its fastest
repetition and on average, in milliseconds; the fastest as a RATE too, the
work done per second, where larger is better; and a checksum of its output
that must be identical across compilers. The rate's UNITS are KB/s (1024
bytes), px/s (pixels), Dhry/s (Dhrystones), tri/s (triangles), vert/s
(vertices), poly/s (polygons), glyph/s or kex/s (key exchanges). Only the
work itself is timed: checking the output and computing the checksum happen
after each repetition.

A repetition is one loop unless `MINTIME` asks for more: then the count
doubles until a repetition lasts at least that many milliseconds, and
LOOPS reports the count it settled on. That is for JIT emulators running
as fast as the host allows: some update the E clock only between frames,
so a single loop can measure as 0 ms.


## License

The harness is 0BSD ([LICENSE](LICENSE)); the vendored code keeps its own
licenses, listed in [third_party/LICENSE.md](third_party/LICENSE.md), and
the LHa terms mean binaries must be distributed together with this source.
