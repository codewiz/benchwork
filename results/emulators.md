# Comparing m68k emulators

This page is part of the [Benchwork results](README.md). Unlike the
others, it measures the emulators rather than the code they run.

benchwork-040, built with m68k-amigaos-gcc after 16.2-rc26[^gcc], runs on
the JIT or the fastest interpreter of each emulator. All of them but Emu68
run on one host, an AMD Ryzen AI 9 HX 370 under Linux held at its nominal
2 GHz; Emu68 ran an older binary on the Raspberry Pi of a PiStorm[^emu68].
The first column is the same binary on the cycle-counted 25 MHz 68040 of
the other pages, and each cell gives the time and how many times faster
than that 68040 the emulator ran. The geomeans leave out the two JSON
benchmarks, which Emu68's older binary lacks.

Each bar is the 68040's time divided by the emulator's, so longer bars are
faster. The axis is logarithmic: the emulators are up to two orders of
magnitude apart.

![Speed of each benchmark relative to a 25 MHz 68040](emulators-040.svg)

The same data as a table, in milliseconds:

| benchmark | 25 MHz 68040 | Copperline | vamos | volamos | Amiberry | Emu68[^emu68] |
|---|---:|---:|---:|---:|---:|---:|
| dhrystone | 433 | 217 (2.0x) | 125 (3.5x) | 89.9 (4.8x) | 6.86 (63.1x) | 11.9 (36.4x) |
| scene-2d | 1017 | 405 (2.5x) | 205 (5.0x) | 159 (6.4x) | 17.9 (56.8x) | 10.9 (93.3x) |
| quantize-floyd | 658 | 414 (1.6x) | 223 (2.9x) | 169 (3.9x) | 6.86 (95.8x) | 14.3 (45.9x) |
| lha-pack | 1103 | 562 (2.0x) | 320 (3.4x) | 190 (5.8x) | 16.3 (67.8x) | 33.8 (32.6x) |
| lha-unpack | 297 | 163 (1.8x) | 91.7 (3.2x) | 58.3 (5.1x) | 5.79 (51.3x) | 8.59 (34.6x) |
| zlib-deflate | 803 | 453 (1.8x) | 255 (3.1x) | 121 (6.6x) | 12.7 (63.3x) | 21.4 (37.5x) |
| zlib-inflate | 205 | 113 (1.8x) | 78.6 (2.6x) | 48.0 (4.3x) | 3.48 (58.8x) | 7.06 (29.0x) |
| png-encode | 1290 | 413 (3.1x) | 412 (3.1x) | 150 (8.6x) | 18.2 (70.9x) | 40.2 (32.1x) |
| png-decode | 978 | 310 (3.2x) | 308 (3.2x) | 126 (7.8x) | 17.2 (56.8x) | 27.3 (35.9x) |
| ftgrays | 656 | 342 (1.9x) | 211 (3.1x) | 122 (5.4x) | 11.2 (58.6x) | 20.0 (32.9x) |
| memcpy-small | 852 | 668 (1.3x) | 467 (1.8x) | 343 (2.5x) | 10.1 (84.5x) | 19.0 (44.9x) |
| memcpy-large | 285 | 126 (2.3x) | 161 (1.8x) | 58.3 (4.9x) | 4.71 (60.4x) | 5.35 (53.2x) |
| memcpy-var-small | 515 | 198 (2.6x) | 164 (3.1x) | 81.5 (6.3x) | 12.4 (41.6x) | 26.9 (19.1x) |
| memcpy-var-large | 592 | 420 (1.4x) | 239 (2.5x) | 72.8 (8.1x) | 16.0 (36.9x) | 11.4 (51.8x) |
| memmove-small | 904 | 266 (3.4x) | 271 (3.3x) | 123 (7.3x) | 31.2 (29.0x) | 19.8 (45.7x) |
| memmove-large | 956 | 384 (2.5x) | 350 (2.7x) | 208 (4.6x) | 35.9 (26.6x) | 12.8 (74.4x) |
| json-parse | 403 | 230 (1.8x) | 140 (2.9x) | 113 (3.6x) | 6.47 (62.3x) |  |
| json-print | 262 | 134 (2.0x) | 104 (2.5x) | 72.2 (3.6x) | 3.91 (67.0x) |  |
| wipeout-tris | 873 | 639 (1.4x) | 494 (1.8x) | 491 (1.8x) | 13.8 (63.1x) | 21.8 (40.0x) |
| nfs3-project[^emu68fpu] | 1058 | 673 (1.6x) | 536 (2.0x) | 672 (1.6x) | 17.7 (59.7x) | 43.8 (24.1x) |
| nfs3-clip[^emu68fpu] | 965 | 590 (1.6x) | 438 (2.2x) | 461 (2.1x) | 17.1 (56.3x) | 27.7 (34.8x) |
| sha512 | 1042 | 843 (1.2x) | 658 (1.6x) | 405 (2.6x) | 28.2 (37.0x) | 21.3 (48.8x) |
| aesgcm | 803 | 331 (2.4x) | 343 (2.3x) | 141 (5.7x) | 26.4 (30.4x) | 9.81 (81.8x) |
| chacha20poly1305 | 401 | 219 (1.8x) | 161 (2.5x) | 101 (4.0x) | 3.81 (105.3x) | 4.38 (91.4x) |
| x25519 | 547 | 325 (1.7x) | 204 (2.7x) | 134 (4.1x) | 3.63 (150.5x) | 7.13 (76.7x) |
| **GEOMEAN** | **679** | **347 (2.0x)** | **255 (2.7x)** | **151 (4.5x)** | **11.9 (57.0x)** | **15.5 (43.7x)** |

vamos and volamos are not Amiga emulators: they run one program on an
emulated CPU and implement the OS libraries in host code. That code runs
natively, which favors them in memcpy-var-small and memcpy-var-large, whose
copies go through exec's CopyMem(). Copperline and Amiberry boot AmigaOS
3.x on an emulated A4000 and emulate its chipset in real time alongside the
CPU. Emu68 replaces only the Amiga's CPU, and boots AmigaOS on its real
chipset.

Copperline and volamos share their CPU emulation, m68k-rs, yet volamos is
2.3 times as fast: Copperline runs the CPU alongside the chipset, at the
clock its governor sets, and volamos lets it run free.

Amiberry's JIT is 21 times as fast as vamos. Amiberry #2708 gave the JIT a
fast path for FPU instructions on x86-64 hosts, so wipeout-tris and the two
nfs3 benchmarks now gain as much over the 68040 as the integer ones.

## How the emulators were run

All times are host time. vamos and volamos derive the guest's E clock from
the host clock. Amiberry and Copperline keep the chipset, and with it the
E clock, in real time, and give the CPU the rest of the host's time, which
they only do in their fastest modes. Amiberry's JIT runs in bursts during
which the E clock stands still, so a short loop can finish before it moves.
Every emulator therefore runs `benchwork-040 REPEAT 3 MINTIME 1000`, which
raises each benchmark's loop count until one repetition lasts at least a
second, and then times three repetitions.

Amiberry and Copperline boot from a volume of their own rather than from
Workbench. Its startup sequence runs TinySetPatch[^tinysetpatch], which
loads MMULib's 68040.library[^mmulib]: the library turns the CPU caches on
and emulates the instructions the 68040 lacks, as SetPatch does on any
booted machine. The sequence then runs benchwork-040 and leaves the output
in a host directory. Nothing is printed until the end, so no screen opens
and the chipset fetches no bitplanes while the benchmarks run.

Each emulator runs the 68040 on one host core. With boost, the host's clock
would depend on its temperature and on which core that is, so
`tools/bench-host-prep.sh` turns boost off first, which holds every core at
its nominal 2 GHz; on this host, boost reaches 5.2 GHz on four of its
twelve cores. The previous version of this page ran vamos, volamos and
Copperline on cores that boosted to 5 GHz despite that setting, so they
appeared about twice as fast as they do here.

- vamos[^vamos], which emulates the CPU with Musashi: `vamos --cpu 68040
  -m 8192 -s 64`.
- volamos 0.23.0[^volamos]: `volamos --jit --cpu 68040 --stack 65536`.
- Amiberry[^amiberry]: an A4000 with a 68040 and its FPU, 32 MB of fast
  memory, no MMU and `cpu_speed=max`, the JIT with its FPU and a 16 MB
  translation cache, and no sound.
- Copperline[^copperline]: an A4000 with a 68040 and 16 MB of fast memory,
  `jit = true` and `speed = "fastest"` from a 25 MHz floor. Its governor
  set the CPU clock between 25 and 51 MHz, 32 MHz in the median. In 15% of
  its samples the host fell behind real time, by 43% on average, so the
  guest measured times about 3% shorter than the host time they took.

[^gcc]: `m68k-amigaos-gcc (AmigaDev 16.2-rc26-22-gdaee1aa7072) 16.2.1b
    20261010182711`, with its default newlib runtime: AmigaPorts/gcc
    amiga16.2 plus the 68060 scheduling of AmigaPorts/gcc#218. Its 68040
    code is the same as amiga16.2's.
[^vamos]: vamos from cnvogelg/amitools b435a703, with machine68k 0.4.1.
[^volamos]: volamos bf4e3c8, 0.23.0 with m68k-rs 0.18.1.
[^amiberry]: Amiberry d4b95e9a3, after v8.5.0.
[^copperline]: Copperline 34f727e2, after v1.2.0.
[^tinysetpatch]: TinySetPatch from reinauer/xSysInfo, a minimal
    SetPatch under the BSD 2-clause licence.
[^mmulib]: MMULib 47.11.2 by Thomas Richter, from Aminet.
[^emu68]: Emu68 1.1.beta.2 on a classic PiStorm (PocketStorm) with a
    Raspberry Pi 4 8 GB at 1800 MHz, measured by mschulz. Unlike the other
    columns, it ran a different binary on a different host: the
    benchwork-040 of the Benchwork 2.0 release, built with 16.2-rc19, which
    has no JSON benchmarks. At 2100 MHz its geomean is 13.3 ms (51.2x).
[^emu68fpu]: Emu68's check values for nfs3-project and nfs3-clip are
    8599502a and 50ad9de3, where the 68040 and the other emulators give
    a12e9d08 and b256492e. Both kernels are floating point, so Emu68's FPU
    probably rounds some results differently.
