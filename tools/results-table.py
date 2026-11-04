#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Turn benchwork output files into one Markdown table: a row per benchmark,
# a column per file holding its fastest repetition in ms, then the GEOMEAN
# row. Columns after the first also show their change against the first
# column in parentheses.
#
# --best puts the fastest cell of each row in bold, every one of them when
# several tie at whole milliseconds.
#
# --speedup prints three significant digits, and how many times faster than
# the first column each cell is instead of the percentage.
#
# --geomean-skip leaves the named benchmarks out of every column's geomean,
# so that columns where some compilers cannot build them stay comparable.
#
# --groups=G,... keeps only the benchmarks of those groups, and
# --skip-groups=G,... leaves those groups out, rows and geomean alike: the
# groups are the ones benches/*.c give each benchmark.
#
# The check values are compared across the files and a mismatch is reported
# on stderr, since differing output means the times cannot be compared.

import glob
import math
import os
import re
import sys

# Name, loops, min and average ms, rate, units and check value.
ROW = re.compile(r"^([a-z][a-z0-9-]*) +(\d+) +(\d+\.\d+) +(\d+\.\d+) +(\d+\.\d+) "
                 r"+([A-Za-z]+/s) +([0-9a-f]{8})$")

# A benchmark's name and group in its struct Bench initializer.
BENCH = re.compile(r'/\* \.name= \*/ "([^"]+)",.*?/\* \.group= \*/ GROUP_([A-Z]+)',
                   re.DOTALL)


def groups():
    """The group of each benchmark, by name, as benches/*.c define them."""
    found = {}
    top = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
    for path in glob.glob(os.path.join(top, "benches", "*.c")):
        with open(path) as f:
            for name, group in BENCH.findall(f.read()):
                found[name] = group.lower()
    return found


def selector(only, skip):
    """Which benchmarks to keep, given the groups to keep (all when empty)
    and the groups to leave out, or None to keep every benchmark."""
    if not only and not skip:
        return None
    known = groups()
    for g in set(only) | set(skip):
        if g not in known.values():
            sys.exit(f"unknown group {g}")
    return lambda name: ((not only or known.get(name) in only)
                         and known.get(name) not in skip)


def parse(path, skip=(), keep=None):
    """The fastest times in ms, the check values and the (rate, units) of
    each benchmark, by name, and the geomean of the times. KEEP, when given,
    says which benchmarks to read at all."""
    times, checks, rates, printed = {}, {}, {}, None
    dropped = False
    with open(path) as f:
        for line in f:
            line = line.rstrip()
            m = ROW.match(line)
            if m and keep and not keep(m[1]):
                dropped = True
                continue
            if m:
                times[m[1]] = float(m[3])
                rates[m[1]] = (float(m[5]), m[6])
                checks[m[1]] = m[7]
            elif line.lower().startswith("geomean"):
                printed = float(line.split()[1])
    # The geomean is recomputed from the rows rather than taken from the
    # binary, whose own log() and exp() are part of what is under test.
    geomean = None
    kept = [t for name, t in times.items() if name not in skip]
    if kept:
        geomean = math.exp(sum(map(math.log, kept)) / len(kept))
        # The binary's own geomean covers every row, so it is only
        # comparable when none was skipped or left out.
        if (printed is not None and len(kept) == len(times) and not dropped
                and abs(printed - geomean) > geomean / 1000):
            print(f"{path}: the binary printed GEOMEAN {printed:.3f}, the rows "
                  f"give {geomean:.3f}", file=sys.stderr)
    return times, checks, rates, geomean


def cell(value, base, first, speedup=False):
    if value is None:
        return ""
    if speedup:
        # Three significant digits: the fastest emulators take a few ms.
        text = f"{value:.{max(0, 2 - math.floor(math.log10(value)))}f}"
        if not first and base:
            text += f" ({base / value:.1f}x)"
        return text
    text = f"{value:.0f}"
    if not first and base:
        # A change that rounds to zero is left out.
        change = round((value - base) / base * 100, 2)
        if change:
            text += f" ({change:+.2f}%)"
    return text


def main(args):
    best = "--best" in args
    speedup = "--speedup" in args
    skip, only, skip_groups = set(), set(), set()
    files = []
    for arg in args:
        if arg in ("--best", "--speedup"):
            continue
        if arg.startswith("--geomean-skip="):
            skip.update(arg.partition("=")[2].split(","))
            continue
        if arg.startswith("--groups="):
            only.update(arg.partition("=")[2].split(","))
            continue
        if arg.startswith("--skip-groups="):
            skip_groups.update(arg.partition("=")[2].split(","))
            continue
        files.append(arg)
    if not files:
        sys.exit(f"Usage: {sys.argv[0]} [--best] [--speedup] "
                 "[--geomean-skip=NAME,...] [--groups=G,...] "
                 "[--skip-groups=G,...] LABEL=FILE ...")
    keep = selector(only, skip_groups)
    columns = []
    for arg in files:
        # The last "=", so that a label can be a flag like -mregparm=2.
        label, _, path = arg.rpartition("=")
        columns.append((label, *parse(path, skip, keep)))

    names = []
    for _, times, *_ in columns:
        for name in times:
            if name not in names:
                names.append(name)

    status = 0
    for name in names:
        seen = {c[2].get(name) for c in columns} - {None}
        if len(seen) > 1:
            print(f"{name}: check values differ: {' '.join(sorted(seen))}",
                  file=sys.stderr)
            status = 1

    print("| benchmark | " + " | ".join(c[0] for c in columns) + " |")
    print("|---|" + "---:|" * len(columns))
    base_times = columns[0][1]
    for name in names:
        values = [c[1].get(name) for c in columns]
        # Ties are judged at the printed precision, whole milliseconds.
        fastest = min(round(v) for v in values if v is not None)
        cells = [cell(v, base_times.get(name), i == 0, speedup)
                 for i, v in enumerate(values)]
        if best:
            cells = [f"**{text}**" if v is not None and round(v) == fastest
                     else text
                     for v, text in zip(values, cells)]
        print(f"| {name} | " + " | ".join(cells) + " |")
    cells = [cell(c[4], columns[0][4], i == 0, speedup)
             for i, c in enumerate(columns)]
    print("| **GEOMEAN** | " + " | ".join(f"**{v}**" for v in cells) + " |")
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
