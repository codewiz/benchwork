#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Turn the table tools/release-sizes.sh writes into a Markdown table: a row
# per program or object, a column per release named on the command line,
# each size with its change against the first release that has one.
#
#   results-sizes-table.py release-sizes.txt rc5 rc12 rc23 ...

import sys


def main(args):
    if len(args) < 2:
        sys.exit(f"Usage: {sys.argv[0]} <release-sizes.txt> RELEASE ...")
    path, releases = args[0], args[1:]
    with open(path) as f:
        rows = [line.split() for line in f if line.strip()]
    head, by_release = rows[0], {r[0]: r for r in rows[1:]}
    for r in releases:
        if r not in by_release:
            sys.exit(f"{path}: no row for {r}")

    print("| bytes | " + " | ".join(releases) + " |")
    print("|---|" + "---:|" * len(releases))
    for k, name in enumerate(head):
        if not k:
            continue
        cells, base = [], None
        for r in releases:
            v = by_release[r][k]
            if v == "-":
                cells.append("")
                continue
            v = int(v)
            text = str(v)
            if base:
                # A change that rounds to zero is left out.
                change = round((v - base) / base * 100, 2)
                if change:
                    text += f" ({change:+.2f}%)"
            else:
                base = v
            cells.append(text)
        print(f"| {name} | " + " | ".join(cells) + " |")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
