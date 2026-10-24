#!/usr/bin/env python3
# SPDX-License-Identifier: 0BSD
#
# Draw benchwork output files as an SVG: one cluster of horizontal bars per
# benchmark (and one for the geomean), one bar per file, with the speed
# relative to the first file, whose bars all sit at 1.0; --help lists the
# options.
#
# --sizes draws one bar per build for its code size instead. --lines draws
# one line per benchmark across any number of files, for a series of
# releases: the time in ms on a log scale, colored by workload family.
#
# The files are read with results-table.py's parser, so the same arguments
# serve both. Up to five files: the colors are a fixed, validated set, and a
# missing row (a benchmark that failed) is marked instead of drawn.

import argparse
import importlib.util
import math
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

# The workload families that color the --lines lines, each a list of
# benchmark name prefixes; a benchmark in none of them gets the first color.
FAMILIES = [("compression", ["lha-", "zlib-", "png-"]),
            ("memory", ["memcpy-", "memmove-"]),
            ("graphics", ["scene-", "quantize-", "ftgrays", "wipeout-",
                          "nfs3-"]),
            ("crypto", ["sha512", "aesgcm", "chacha20poly1305", "x25519"]),
            ("dhrystone", ["dhrystone"])]

BAR = 12          # bar thickness
GAP = 2           # surface gap between bars of a cluster
PAD = 16          # air between clusters
LEFT = 180        # room for the benchmark names
RIGHT = 130       # room for the labels past the longest bar
WIDTH = 940
TOP = 36          # the legend
AXIS = 26         # the axis below the plot
LABEL_MIN = 0.05  # the smallest change that gets a label on the speed charts
LOG_TICKS = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000]


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def start(title, height, ncolors):
    """The SVG element, its light and dark styles, and the background."""
    out = [(f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" '
            f'height="{height}" viewBox="0 0 {WIDTH} {height}" role="img" '
            f'aria-label="{esc(title)}">')]
    css = [
        (".t{font-family:system-ui,-apple-system,'Segoe UI',Helvetica,Arial,sans-serif;"
         "font-size:12px;fill:#0b0b0b}"),
        ".t.muted{fill:#52514e}",
        ".t.big{font-size:16px}", ".t.name{font-weight:600}",
        ".t.axis{fill:#52514e;font-size:12px}",
        ".grid{stroke:#e6e5e1;stroke-width:1}", ".ref{stroke:#0b0b0b;stroke-width:2}",
        ".bg{fill:#fcfcfb}", ".ring{stroke:#fcfcfb;stroke-width:2}",
        ".dot{fill:#0b0b0b}",
    ]
    for i, (light, _) in enumerate(COLORS[:ncolors]):
        css.append(f".s{i}{{fill:{light}}}.k{i}{{stroke:{light}}}")
    dark = [
        ".t{fill:#ffffff}", ".t.muted{fill:#c3c2b7}", ".t.axis{fill:#c3c2b7}",
        ".grid{stroke:#333331}", ".ref{stroke:#ffffff}", ".bg{fill:#1a1a19}",
        ".ring{stroke:#1a1a19}", ".dot{fill:#ffffff}",
    ]
    for i, (_, d) in enumerate(COLORS[:ncolors]):
        dark.append(f".s{i}{{fill:{d}}}.k{i}{{stroke:{d}}}")
    out.append("<style>" + "".join(css) +
               "@media(prefers-color-scheme:dark){" + "".join(dark) + "}</style>")
    out.append(f'<rect class="bg" width="{WIDTH}" height="{height}"/>')
    return out


def scale(ratios, least=1.0, log=False):
    """The axis end, the next half above the longest bar (the next 1, 2 or 5
    times a power of ten on a log axis from 1.0), and the x mapping."""
    plot_w = WIDTH - LEFT - RIGHT
    if log:
        xmax = next(t for t in LOG_TICKS if t >= max(ratios))
        return xmax, lambda ratio: LEFT + math.log(ratio) / math.log(xmax) * plot_w
    xmax = max(least, (int(max(ratios) * 2) + 1) / 2)
    return xmax, lambda ratio: LEFT + ratio / xmax * plot_w


def grid(out, xmax, x, top, plot_h, log=False):
    """Gridlines at every half (at 1, 2 and 5 times the powers of ten on a
    log axis), the reference line at 1.0 on top."""
    ticks = [t for t in LOG_TICKS if t <= xmax] if log else \
        [i / 2 for i in range(int(xmax * 2) + 1)]
    for t in ticks:
        cls = "ref" if t == 1.0 else "grid"
        out.append(f'<line class="{cls}" x1="{x(t):.1f}" y1="{top}" '
                   f'x2="{x(t):.1f}" y2="{top + plot_h}"/>')
        text = f"{t:g}x" if log else f"{t:.1f}x"
        out.append(f'<text class="t axis" x="{x(t):.1f}" y="{top + plot_h + 18}" '
                   f'text-anchor="middle">{text}</text>')


def bar(out, i, y, w):
    """A bar of color i, square at the baseline and rounded at the data end."""
    rx = min(4, w / 2)
    out.append(f'<path class="s{i}" d="M{LEFT},{y} h{w - rx:.1f} '
               f'a{rx},{rx} 0 0 1 {rx},{rx} v{BAR - 2 * rx} '
               f'a{rx},{rx} 0 0 1 -{rx},{rx} h-{w - rx:.1f} z"/>')


def label_x(x, ratio, text):
    """Where a label goes: just past its bar, or past the 1.0 line when the
    label would cross it. 12px digits are about 6.5px wide."""
    end = x(ratio) + 5
    if end + 6.5 * len(text) > x(1.0) - 4 and ratio < 1.0:
        end = x(1.0) + 5
    return end


def better(out, text):
    """The which-way-is-better note, top right."""
    out.append(f'<text class="t muted" x="{WIDTH - 12}" y="21" '
               f'text-anchor="end">{text}</text>')


def num(v):
    """Three significant digits, without an exponent."""
    return f"{v:.{max(0, 2 - math.floor(math.log10(v)))}f}"


def sizes(opts):
    """One bar per build, each labeled with its own name, so no legend."""
    columns = []
    for arg in opts.columns:
        label, _, size = arg.rpartition("=")
        columns.append((label, int(size)))
    base = columns[0][1]
    ratios = [size / base for _, size in columns]
    xmax, x = scale(ratios)
    pitch = BAR + PAD
    top = TOP
    plot_h = len(columns) * pitch
    height = top + plot_h + AXIS
    out = start(opts.title, height, len(columns))
    better(out, "&#9664; Size: less is better")
    grid(out, xmax, x, top, plot_h)
    for i, ((label, size), ratio) in enumerate(zip(columns, ratios)):
        y = top + i * pitch + PAD / 2
        out.append(f'<text class="t big" x="{LEFT - 12}" y="{y + BAR - 1}" '
                   f'text-anchor="end">{esc(label)}</text>')
        bar(out, i, y, x(ratio) - LEFT)
        text = f"{size}"
        if i > 0:
            text += f" ({(ratio - 1) * 100:+.2f}%)"
        out.append(f'<text class="t muted" x="{label_x(x, ratio, text):.1f}" '
                   f'y="{y + BAR - 2}">{text}</text>')
    out.append("</svg>")
    return out


def lines(opts, columns):
    """Every benchmark as one line across the files in order, in ms on a log
    scale so that equal changes have equal slopes, named at the right end.
    Time rather than speed: relative speeds would all start at 1.0 and
    bunch up."""
    series = [(n, [c[1].get(n) for c in columns]) for n in columns[0][1]]
    series.append(("geomean", [c[4] for c in columns]))
    flat = [v for _, vs in series for v in vs if v]
    lo, hi = math.log(min(flat) * 0.9), math.log(max(flat) * 1.1)
    left, right, top, bottom, height = 64, 160, 44, 30, 680
    w, h = WIDTH - left - right, height - top - bottom
    x = lambda i: left + i / (len(columns) - 1) * w
    y = lambda v: top + (hi - math.log(v)) / (hi - lo) * h
    out = start(opts.title, height, len(FAMILIES))
    better(out, "&#9660; Time: less is better")
    lx = left
    for i, (family, _) in enumerate(FAMILIES):
        out.append(f'<rect class="s{i}" x="{lx}" y="8" width="14" height="14" rx="2"/>')
        out.append(f'<text class="t big" x="{lx + 20}" y="21">{family}</text>')
        lx += 20 + 9 * len(family) + 24
    for i in range(len(columns)):
        out.append(f'<line class="grid" x1="{x(i):.1f}" y1="{top}" x2="{x(i):.1f}" '
                   f'y2="{top + h}" stroke-dasharray="3 4"/>')
    for t in (50, 100, 200, 300, 500, 1000, 2000, 3000, 5000):
        if lo < math.log(t) < hi:
            out.append(f'<line class="grid" x1="{left}" y1="{y(t):.1f}" '
                       f'x2="{left + w}" y2="{y(t):.1f}"/>')
            out.append(f'<text class="t axis" x="{left - 6}" y="{y(t) + 4:.1f}" '
                       f'text-anchor="end">{t} ms</text>')
    for i, c in enumerate(columns):
        out.append(f'<text class="t axis" x="{x(i):.1f}" y="{height - 10}" '
                   f'text-anchor="middle">{esc(c[0])}</text>')
    # The names at the right end, pushed apart where they would overlap.
    ends = sorted((y(vs[-1]), n) for n, vs in series if vs[-1])
    placed = []
    for ey, n in ends:
        placed.append([max(ey, placed[-1][0] + 13) if placed else ey, n])
    over = placed[-1][0] - (top + h)
    for p in placed if over > 0 else ():
        p[0] -= over
    for k in range(len(placed) - 2, -1, -1):
        placed[k][0] = min(placed[k][0], placed[k + 1][0] - 13)
    names = {n: py for py, n in placed}
    for n, vs in series:
        geo = n == "geomean"
        f = next((i for i, (_, names_) in enumerate(FAMILIES)
                  if any(n.startswith(p) for p in names_)), 0)
        d = "".join(f'{"L" if i and vs[i - 1] else "M"}{x(i):.1f},{y(v):.1f}'
                    for i, v in enumerate(vs) if v)
        out.append(f'<path class="{"ref" if geo else f"k{f}"}" d="{d}" fill="none" '
                   f'stroke-width="{3 if geo else 2}" stroke-linejoin="round">'
                   f'<title>{esc(n)}</title></path>')
        for i, v in enumerate(vs):
            if v:
                # A dot ringed with the surface color, and a wider hit target
                # for the tooltip.
                out.append(f'<circle class="{"dot" if geo else f"s{f}"} ring" '
                           f'cx="{x(i):.1f}" cy="{y(v):.1f}" r="4"/>')
                rate = columns[i][3].get(n)
                what = f"{num(rate[0])} {rate[1]}, " if rate else ""
                out.append(f'<circle cx="{x(i):.1f}" cy="{y(v):.1f}" r="7" '
                           f'fill="transparent"><title>{esc(n)} {esc(columns[i][0])}: '
                           f'{num(v)} ms, {what}{(vs[0] / v - 1) * 100:+.2f}% '
                           f'speed</title></circle>')
        if vs[-1]:
            # A leader from the line's end to its name, which may have moved.
            out.append(f'<path class="grid" d="M{left + w + 2},{y(vs[-1]):.1f} '
                       f'L{left + w + 10},{names[n]:.1f} h6" fill="none"/>')
            out.append(f'<text class="t{" name" if geo else ""}" x="{left + w + 20}" '
                       f'y="{names[n] + 4:.1f}">{esc(n)}</text>')
    out.append("</svg>")
    return out


def main(args):
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", default="results.svg")
    # Not drawn: the title and the caption belong to the document that shows
    # the chart. The title is the SVG's accessible name.
    ap.add_argument("-t", "--title", default="Speed relative to the first compiler")
    ap.add_argument("--sizes", action="store_true",
                    help="the arguments are LABEL=BYTES: chart the code sizes")
    ap.add_argument("--geomean-skip", default="", metavar="NAME,...",
                    help="benchmarks left out of the geomean, which some "
                    "compilers cannot build")
    ap.add_argument("--hide-base", action="store_true",
                    help="do not draw the first file's bars, which are all 1.0")
    ap.add_argument("--log", action="store_true",
                    help="a log axis from 1.0, labeled with the speedup, for "
                    "speeds far apart")
    ap.add_argument("--lines", action="store_true",
                    help="one line per benchmark across the files in order, "
                    "for a series of releases")
    ap.add_argument("columns", nargs="+", metavar="LABEL=FILE")
    opts = ap.parse_args(args)
    if len(opts.columns) - opts.hide_base > len(COLORS) and not opts.lines:
        sys.exit(f"at most {len(COLORS)} files drawn")
    if opts.sizes:
        write(opts.output, sizes(opts))
        return

    skip = set(filter(None, opts.geomean_skip.split(",")))
    columns = []
    # The benchmarks each column's run reported as failed, by label.
    failed = {}
    for arg in opts.columns:
        # The last "=", so that a label can be a flag like -mregparm=2.
        label, _, path = arg.rpartition("=")
        columns.append((label, *results_table.parse(path, skip)))
        with open(path) as f:
            failed[label] = {line.split()[0] for line in f
                             if line.rstrip().endswith(" failed")}
    if opts.lines:
        write(opts.output, lines(opts, columns))
        return

    names = []
    for _, times, *_ in columns:
        for name in times:
            if name not in names:
                names.append(name)
    base_times, base_checks = columns[0][1], columns[0][2]

    # A cell is the speed relative to the first column, "failed" when the
    # benchmark did not run, or "wrong" when its output differs from the first
    # column's, which makes its time meaningless. A column with any such cell gets no
    # geomean. A benchmark left out of the geomean costs no column its
    # geomean, and one that a column lacks was not built for it unless the
    # run said it failed.
    def cell(c, n):
        if n not in c[1] or n not in base_times:
            return "not built" if n in skip and n not in failed[c[0]] else "failed"
        if c[2].get(n) != base_checks.get(n):
            return "wrong"
        return base_times[n] / c[1][n]

    rows = [(n, [cell(c, n) for c in columns]) for n in names]
    complete = [all(isinstance(r[1][i], float) or r[0] in skip for r in rows)
                for i in range(len(columns))]
    base_geomean = columns[0][4]
    rows.append(("geomean", [base_geomean / c[4] if complete[i] else "n/a"
                             for i, c in enumerate(columns)]))

    ratios = [r for _, rs in rows for r in rs if isinstance(r, float)]
    xmax, x = scale(ratios, log=opts.log)
    # With --hide-base the first file is only the reference: its bars would
    # all sit at 1.0, which the reference line already shows.
    first = 1 if opts.hide_base else 0
    shown = len(columns) - first
    cluster_h = shown * BAR + (shown - 1) * GAP
    pitch = cluster_h + PAD
    plot_h = len(rows) * pitch
    height = TOP + plot_h + AXIS
    out = start(opts.title, height, shown)
    better(out, "Speed: more is better &#9654;")

    # Legend.
    lx = LEFT
    for i, (label, *_) in enumerate(columns[first:]):
        out.append(f'<rect class="s{i}" x="{lx}" y="8" width="14" height="14" rx="2"/>')
        out.append(f'<text class="t big" x="{lx + 20}" y="21">{esc(label)}</text>')
        # A rough width of the label at 16px: dashes, spaces and thin
        # letters take about half the room of the others.
        lx += 20 + sum(5 if c in " -.,:;!|()ijlft1" else 9 for c in label) + 28

    grid(out, xmax, x, TOP, plot_h, opts.log)

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
            bar(out, i - first, y, x(ratio) - LEFT)
            # The rate, and the change in speed: on a log axis, the speedup.
            if opts.log:
                text = f"{num(ratio)}x"
                out.append(f'<text class="t muted" x="{x(ratio) + 5:.1f}" '
                           f'y="{y + BAR - 2}">{text}</text>')
            elif i > 0 and abs(ratio - 1) >= LABEL_MIN:
                rate = columns[i][3].get(name)
                text = f"{(ratio - 1) * 100:+.2f}%"
                if rate:
                    text = f"{num(rate[0])} {rate[1]} ({text})"
                out.append(f'<text class="t muted" x="{label_x(x, ratio, text):.1f}" '
                           f'y="{y + BAR - 2}">{text}</text>')
    out.append("</svg>")
    write(opts.output, out)


def write(path, out):
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main(sys.argv[1:])
