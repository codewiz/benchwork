#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Print the CODE SIZE row of a results table.
#
# The sizes are the ones tools/bench-run.sh saved in the results directory's
# sizes/, for the runs named on the command line. The first is the base of
# the percentages.

import os
import sys

if len(sys.argv) < 4 or sys.argv[1] != "--results-dir":
    sys.exit(f"Usage: {sys.argv[0]} --results-dir <dir> <name> [<name> ...]")

DIR = os.path.join(sys.argv[2], "sizes")
args = sys.argv[3:]


def size(name):
    with open(os.path.join(DIR, name)) as f:
        return int(f.read().split()[1])


base = size(args[0])
cells = [str(base)]
for name in args[1:]:
    value = size(name)
    change = round((value - base) / base * 100, 2)
    cells.append(f"{value} ({change:+.2f}%)" if change else str(value))
print("| **CODE SIZE** | " + " | ".join(cells) + " |")
