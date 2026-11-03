#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Turn benchwork output files into a Markdown table of how one benchmark
# changes across releases: a column per release, and a row per build, so
# several runtimes or settings can share one table.
#
#   results-releases-table.py --releases rc21,rc22,rc23 --bench NAME \
#       "newlib=FILE,FILE,FILE" "libnix=FILE,FILE,FILE" ...
#
# Each cell is the fastest repetition in ms, with its change against the
# release before it that has a time. A file whose benchmark failed shows
# "fails"; a cell that is not a file, such as "hangs", is printed as it is.
#
# Unlike results-table.py, it does not compare check values: the builds may
# use different C runtimes, which may print and parse floats with more or
# less precision.

import os
import re
import sys

# Name, loops, min and average ms, rate, units and check value.
ROW = re.compile(r"^([a-z][a-z0-9-]*) +(\d+) +(\d+\.\d+) +(\d+\.\d+) +(\d+\.\d+) "
                 r"+([A-Za-z]+/s) +([0-9a-f]{8})$")
FAILED = re.compile(r"^([a-z][a-z0-9-]*) +(setup failed|FAILED|failed)")


def parse(path):
    """The fastest time in ms of each benchmark, or None for one that
    failed, by name."""
    rows = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            m = ROW.match(line)
            if m:
                rows[m[1]] = float(m[3])
                continue
            m = FAILED.match(line)
            if m:
                rows[m[1]] = None
    return rows


def main(args):
    releases, bench, builds = None, None, []
    while args and args[0].startswith("--"):
        if args[0] == "--releases" and len(args) > 1:
            releases = args[1].split(",")
        elif args[0] == "--bench" and len(args) > 1:
            bench = args[1]
        else:
            break
        args = args[2:]
    if not releases or not bench or not args:
        sys.exit(f"Usage: {sys.argv[0]} --releases R1,R2,... --bench NAME "
                 "LABEL=CELL,CELL,... ...")
    for arg in args:
        label, _, cells = arg.partition("=")
        cells = cells.split(",")
        if len(cells) != len(releases):
            sys.exit(f"{label}: {len(cells)} cells for {len(releases)} releases")
        builds.append((label, [parse(c) if os.path.isfile(c) else c
                               for c in cells]))

    print(f"| {bench} | " + " | ".join(releases) + " |")
    print("|---|" + "---:|" * len(releases))
    for label, cells in builds:
        out, before = [], None
        for c in cells:
            if isinstance(c, str):
                out.append(c)
                continue
            if c.get(bench) is None:
                out.append("fails" if bench in c else "")
                continue
            ms = c[bench]
            text = f"{ms:.0f}"
            if before:
                # A change that rounds to zero is left out.
                change = round((ms - before) / before * 100, 2)
                if change:
                    text += f" ({change:+.2f}%)"
            out.append(text)
            before = ms
        print(f"| {label} | " + " | ".join(out) + " |")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
