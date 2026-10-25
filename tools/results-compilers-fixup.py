#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Fill in the cells of the compilers table that have no time.
#
# The table has the columns benchmark | gcc 6.5 | gcc 16.2 | vbcc | SAS/C.
# results-table.py's stderr says which check values differ. The empty cells
# become:
#   vbcc, benchmark missing          "failed" ("not built" for the C++
#                                    NFS3 kernels)
#   vbcc, check value differs        "wrong output"
#   SAS/C, benchmark missing         "not built"
#   vbcc's geomean                   left empty if a benchmark it covers
#                                    failed or gave the wrong output

import re
import sys
from pathlib import Path

if len(sys.argv) != 3:
    sys.exit(f"Usage: {sys.argv[0]} <table.md> <results-table.py stderr>")

table, mismatches = Path(sys.argv[1]), Path(sys.argv[2])
wrong = {
    line.split(":")[0]
    for line in mismatches.read_text().splitlines()
    if "check values differ" in line
}

# The benchmarks results-tables.sh leaves out of the geomean.
GEOMEAN_SKIP = {
    "nfs3-project",
    "nfs3-clip",
    "sha512",
    "aesgcm",
    "chacha20poly1305",
    "x25519",
}

vbcc_broken = False
for line in table.read_text().splitlines():
    cells = [c.strip() for c in line.strip("|").split("|")]
    if len(cells) != 5 or cells[0] in ("benchmark", "---"):
        print(line)
        continue
    name = re.sub(r"\[\^[^]]*\]", "", cells[0])
    if name == "**GEOMEAN**":
        if vbcc_broken:
            cells[3] = ""
    else:
        if name in wrong:
            cells[3] = "wrong output"
        elif not cells[3]:
            cells[3] = "not built" if name.startswith("nfs3-") else "failed"
        if cells[3] in ("wrong output", "failed") and name not in GEOMEAN_SKIP:
            vbcc_broken = True
        if not cells[4]:
            cells[4] = "not built"
    print(("| " + " | ".join(cells) + " |").replace("|  |", "| |"))
