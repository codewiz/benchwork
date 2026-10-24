## Results

- [m68k compilers](compilers.md): gcc 6.5, gcc 16.2, vbcc and
  SAS/C on the same benchmarks.
- [GCC 16.2 releases](gcc-releases.md): every AmigaPorts release
  since rc5.
- [GCC optimization levels](opt-levels.md): `-O2`, `-Os`, `-O3` and
  LTO on a 68030 with its instruction cache.
- [GCC code generation per CPU](cpus.md): the release binaries for
  the 68000, 68020, 68020 to 68060 with FPU and 68040, and `-m68060`
  against `-m68020-60` on a 68060.
- [GCC register arguments](regparm.md): `-mregparm` and
  `-msasregparm` against stack arguments.

## Methodology

### Timing

Every number is the fastest repetition of a benchmark, in milliseconds,
under [volamos](https://github.com/sidick/volamos) 0.13.0 with `--clock-mhz`:
the emulated time is derived from the CPU's cycle count, so all repetitions
of a run time the same and host load changes nothing. Unless a page says
otherwise, the CPU is a 25 MHz 68040:

    volamos --clock-mhz 25 --cpu 68040 --fpu <binary> REPEAT 1

volamos has no bus wait states and runs native library calls in zero
cycles, so absolute times are lower than on hardware; the comparisons
between builds are what these pages are for.

### What is timed

Only each benchmark's work: checking its output and computing the check
value happen after every repetition, outside the timing.

The check value must be the same for every compiler and setting.

### Reading the tables

Percentages are against the first column. The fastest result in a row is
in bold, every one of them when several tie at whole milliseconds.
GEOMEAN is the geometric mean of a column, which weighs a 10% change in a
short benchmark the same as in a long one. CODE SIZE is the text size
`m68k-amigaos-size` reports for the whole binary.

### Compilers

The gcc 16.2 releases are AmigaPorts' x86_64 Linux tarballs,
with their default newlib runtime; each page names the exact version.

### Raw data

The output of every run is in [raw/](raw/), named
`<compiler>-<version>-<build>-<emulated CPU>[-cache].txt`.
`tools/regen-results.sh` reruns all of them and regenerates the tables and
charts.
