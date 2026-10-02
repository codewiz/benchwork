#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Turn benchwork output files into one Markdown table: a row per benchmark,
# a column per file holding its fastest iteration in ms, then the geomean
# row. Usage:
#
#   tools/results-table.py LABEL=FILE [LABEL=FILE ...]
#
# The check values are compared across the files and a mismatch is reported
# on stderr, since differing output means the times cannot be compared.

import re
import sys

ROW = re.compile(r"^([a-z][a-z0-9-]*) +(\d+) +(\d+\.\d+) +(\d+\.\d+) +([0-9a-f]{8})$")


def parse(path):
    times, checks, geomean = {}, {}, None
    with open(path) as f:
        for line in f:
            line = line.rstrip()
            m = ROW.match(line)
            if m:
                times[m[1]] = float(m[3])
                checks[m[1]] = m[5]
            elif line.startswith("geomean"):
                geomean = float(line.split()[1])
    return times, checks, geomean


def main(args):
    columns = []
    for arg in args:
        label, _, path = arg.partition("=")
        columns.append((label, *parse(path)))

    names = []
    for _, times, _, _ in columns:
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
    for name in names:
        cells = [f"{c[1][name]:.0f}" if name in c[1] else "" for c in columns]
        print(f"| {name} | " + " | ".join(cells) + " |")
    cells = [f"{c[3]:.0f}" if c[3] is not None else "" for c in columns]
    print("| **geomean** | " + " | ".join(f"**{v}**" for v in cells) + " |")
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
