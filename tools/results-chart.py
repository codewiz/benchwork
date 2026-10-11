#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Draw benchwork output files as an SVG: one cluster of horizontal bars per
# benchmark (and one for the geomean), one bar per file, with the times
# normalized to the first file so its bars all sit at 1.0. Usage:
#
#   tools/results-chart.py [-o chart.svg] [-t TITLE] [--hide-first] LABEL=FILE ...
#   tools/results-chart.py --sizes [-o chart.svg] [-t TITLE] LABEL=BYTES ...
#
# --hide-first leaves out the first file's bars, which all sit on the 1.0
# line. --sizes draws one bar per build for its code size instead.
#
# The files are read with results-table.py's parser, so the same arguments
# serve both. Up to five files: the colors are a fixed, validated set, and a
# missing row (a benchmark that failed) is marked instead of drawn.

import argparse
import importlib.util
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("results_table",
                                              os.path.join(HERE, "results-table.py"))
results_table = importlib.util.module_from_spec(spec)
spec.loader.exec_module(results_table)

# Light and dark steps of the same four hues, in a fixed order.
COLORS = [("#2a78d6", "#3987e5"), ("#eb6834", "#d95926"),
          ("#1baf7a", "#199e70"), ("#eda100", "#c98500"),
          ("#e87ba4", "#d55181")]

BAR = 12          # bar thickness
GAP = 2           # surface gap between bars of a cluster
PAD = 16          # air between clusters
LEFT = 180        # room for the benchmark names
RIGHT = 70        # room for the labels past the longest bar
WIDTH = 940
TOP = 36          # the legend
AXIS = 26         # the axis below the plot


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def start(title, height, ncolors):
    """The SVG element, its light and dark styles, and the background."""
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" '
           f'height="{height}" viewBox="0 0 {WIDTH} {height}" role="img" '
           f'aria-label="{esc(title)}">']
    css = [
        ".t{font-family:system-ui,-apple-system,'Segoe UI',Helvetica,Arial,sans-serif;"
        "font-size:12px;fill:#0b0b0b}",
        ".t.muted{fill:#52514e}",
        ".t.big{font-size:16px}", ".t.name{font-weight:600}",
        ".t.axis{fill:#52514e;font-size:12px}",
        ".grid{stroke:#e6e5e1;stroke-width:1}", ".ref{stroke:#0b0b0b;stroke-width:2}",
        ".bg{fill:#fcfcfb}",
    ]
    for i, (light, _) in enumerate(COLORS[:ncolors]):
        css.append(f".s{i}{{fill:{light}}}")
    dark = [
        ".t{fill:#ffffff}", ".t.muted{fill:#c3c2b7}", ".t.axis{fill:#c3c2b7}",
        ".grid{stroke:#333331}", ".ref{stroke:#ffffff}", ".bg{fill:#1a1a19}",
    ]
    for i, (_, d) in enumerate(COLORS[:ncolors]):
        dark.append(f".s{i}{{fill:{d}}}")
    out.append("<style>" + "".join(css) +
               "@media(prefers-color-scheme:dark){" + "".join(dark) + "}</style>")
    out.append(f'<rect class="bg" width="{WIDTH}" height="{height}"/>')
    return out


def scale(ratios, least=2.0):
    """The axis end, the next half above the longest bar, and the x mapping."""
    xmax = max(least, (int(max(ratios) * 2) + 1) / 2)
    plot_w = WIDTH - LEFT - RIGHT
    return xmax, lambda ratio: LEFT + ratio / xmax * plot_w


def grid(out, xmax, x, top, plot_h):
    """Gridlines at every half, the reference line at 1.0 on top."""
    for i in range(int(xmax * 2) + 1):
        t = i / 2
        cls = "ref" if t == 1.0 else "grid"
        out.append(f'<line class="{cls}" x1="{x(t):.1f}" y1="{top}" '
                   f'x2="{x(t):.1f}" y2="{top + plot_h}"/>')
        out.append(f'<text class="t axis" x="{x(t):.1f}" y="{top + plot_h + 18}" '
                   f'text-anchor="middle">{t:.1f}x</text>')


def bar(out, i, y, w):
    """A bar of color i, square at the baseline and rounded at the data end."""
    rx = min(4, w / 2)
    out.append(f'<path class="s{i}" d="M{LEFT},{y} h{w - rx:.1f} '
               f'a{rx},{rx} 0 0 1 {rx},{rx} v{BAR - 2 * rx} '
               f'a{rx},{rx} 0 0 1 -{rx},{rx} h-{w - rx:.1f} z"/>')


def sizes(opts):
    """One bar per build, each labeled with its own name, so no legend."""
    columns = []
    for arg in opts.columns:
        label, _, size = arg.partition("=")
        columns.append((label, int(size)))
    base = columns[0][1]
    ratios = [size / base for _, size in columns]
    xmax, x = scale(ratios, least=1.0)
    pitch = BAR + PAD
    top = 8
    plot_h = len(columns) * pitch
    height = top + plot_h + AXIS
    out = start(opts.title, height, len(columns))
    grid(out, xmax, x, top, plot_h)
    for i, ((label, size), ratio) in enumerate(zip(columns, ratios)):
        y = top + i * pitch + PAD / 2
        out.append(f'<text class="t big" x="{LEFT - 12}" y="{y + BAR - 1}" '
                   f'text-anchor="end">{esc(label)}</text>')
        bar(out, i, y, x(ratio) - LEFT)
        text = f"{size}"
        if i > 0:
            text += f" ({(ratio - 1) * 100:+.2f}%)"
        # Past the 1.0 line too, which would cross a shorter bar's label.
        out.append(f'<text class="t muted" x="{max(x(ratio), x(1.0)) + 5:.1f}" '
                   f'y="{y + BAR - 2}">{text}</text>')
    out.append("</svg>")
    return out


def main(args):
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", default="results.svg")
    # Not drawn: the title and the caption belong to the document that shows
    # the chart. The title is the SVG's accessible name.
    ap.add_argument("-t", "--title", default="Time relative to the first compiler")
    ap.add_argument("--sizes", action="store_true",
                    help="the arguments are LABEL=BYTES: chart the code sizes")
    ap.add_argument("--hide-first", action="store_true",
                    help="draw no bars for the first file, which the 1.0 line stands for")
    ap.add_argument("columns", nargs="+", metavar="LABEL=FILE")
    opts = ap.parse_args(args)
    if len(opts.columns) > len(COLORS):
        sys.exit(f"at most {len(COLORS)} files")
    if opts.sizes:
        write(opts.output, sizes(opts))
        return

    columns = []
    for arg in opts.columns:
        label, _, path = arg.partition("=")
        times, checks, geomean = results_table.parse(path)
        columns.append((label, times, checks, geomean))

    names = []
    for _, times, _, _ in columns:
        for name in times:
            if name not in names:
                names.append(name)
    base_times, base_checks = columns[0][1], columns[0][2]

    # A cell is a ratio to the first column, "failed" when the benchmark did
    # not run, or "wrong" when its output differs from the first column's,
    # which makes its time meaningless. A column with any such cell gets no
    # geomean.
    def cell(c, n):
        if n not in c[1] or n not in base_times:
            return "failed"
        if c[2].get(n) != base_checks.get(n):
            return "wrong"
        return c[1][n] / base_times[n]

    rows = [(n, [cell(c, n) for c in columns]) for n in names]
    complete = [all(isinstance(r[1][i], float) for r in rows)
                for i in range(len(columns))]
    base_geomean = columns[0][3]
    rows.append(("geomean", [c[3] / base_geomean if complete[i] else "n/a"
                             for i, c in enumerate(columns)]))

    ratios = [r for _, rs in rows for r in rs if isinstance(r, float)]
    xmax, x = scale(ratios)
    # The files drawn; the others keep their colors when the first is hidden.
    first = 1 if opts.hide_first else 0
    drawn = len(columns) - first
    cluster_h = drawn * BAR + (drawn - 1) * GAP
    pitch = cluster_h + PAD
    plot_h = len(rows) * pitch
    height = TOP + plot_h + AXIS
    out = start(opts.title, height, len(columns))

    # Legend.
    lx = LEFT
    for i, (label, _, _, _) in enumerate(columns):
        if i < first:
            # The hidden file is the 1.0 line: its swatch is a piece of it.
            out.append(f'<line class="ref" x1="{lx + 7}" y1="7" x2="{lx + 7}" y2="23"/>')
        else:
            out.append(f'<rect class="s{i}" x="{lx}" y="8" width="14" height="14" rx="2"/>')
        out.append(f'<text class="t big" x="{lx + 20}" y="21">{esc(label)}</text>')
        # A rough width of the label at 16px: dashes, spaces and thin
        # letters take about half the room of the others.
        lx += 20 + sum(5 if c in " -.,:;!|()ijlft1" else 9 for c in label) + 28

    grid(out, xmax, x, TOP, plot_h)

    for r, (name, rs) in enumerate(rows):
        y0 = TOP + r * pitch
        cls = "t big name" if name == "geomean" else "t big"
        out.append(f'<text class="{cls}" x="{LEFT - 12}" y="{y0 + cluster_h / 2 + 5:.1f}" '
                   f'text-anchor="end">{esc(name)}</text>')
        for i, ratio in enumerate(rs):
            if i < first:
                continue
            y = y0 + (i - first) * (BAR + GAP)
            if not isinstance(ratio, float):
                text = {"wrong": "wrong output"}.get(ratio, ratio)
                out.append(f'<text class="t muted" x="{LEFT + 6}" y="{y + BAR - 2}">'
                           f'{text}</text>')
                continue
            bar(out, i, y, x(ratio) - LEFT)
            if i > 0 and abs(ratio - 1) >= 0.25:
                out.append(f'<text class="t muted" x="{x(ratio) + 5:.1f}" '
                           f'y="{y + BAR - 2}">{ratio:.2f}x</text>')
    out.append("</svg>")
    write(opts.output, out)


def write(path, out):
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main(sys.argv[1:])
