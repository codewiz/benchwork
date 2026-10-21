#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Replace the tables of a Markdown page.
#
# The page's tables are replaced, in order, by the given files. Nothing is
# written if the page has a different number of tables.

import sys
from pathlib import Path

if len(sys.argv) < 3:
    sys.exit(f"Usage: {sys.argv[0]} <page.md> <table1.md> [<table2.md> ...]")
path, tables = Path(sys.argv[1]), sys.argv[2:]
lines = path.read_text().split("\n")
out, n, i = [], 0, 0
while i < len(lines):
    if lines[i].startswith("|"):
        while i < len(lines) and lines[i].startswith("|"):
            i += 1
        if n < len(tables):
            out.extend(Path(tables[n]).read_text().rstrip("\n").split("\n"))
        n += 1
    else:
        out.append(lines[i])
        i += 1
if n != len(tables):
    sys.exit(f"{path} has {n} tables, {len(tables)} given")
path.write_text("\n".join(out))
