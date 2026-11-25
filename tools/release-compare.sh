#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Compare a release candidate with the previous release before tagging it.
#
# Builds benchwork with each of the two toolchains in every configuration
# below, runs the builds cycle-counted under volamos, and prints a Markdown
# table of each configuration's times and code sizes, the candidate against
# the previous release. The configurations cover the ways one release has
# regressed unseen before: the 68000, which has no 32-bit multiply or
# divide, at -O2 and -Os, the 68040 build, and the 68020-60 build on a
# 68060, whose pipeline gcc tunes it for.
#
# --libs-dir takes the libraries directory of an AmigaOS installation, as
# for tools/regen-results.sh: the 68000 builds are soft float, and without
# the mathieee libraries their float arithmetic costs no cycles.
#
# Usage: tools/release-compare.sh --results-dir <dir> [--libs-dir <dir>]
#            <previous toolchain> <candidate toolchain>

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

usage() {
    echo "Usage: $0 --results-dir <dir> [--libs-dir <dir>] <previous toolchain> <candidate toolchain>" >&2
    exit 1
}

results='' libs_dir='' libs=()
while [ $# -gt 0 ]; do
    case $1 in
    --results-dir)
        [ $# -ge 2 ] || usage
        results=$(realpath -m "$2")
        shift 2
        ;;
    --libs-dir)
        [ $# -ge 2 ] || usage
        libs_dir=$(realpath "$2")
        shift 2
        ;;
    -*) usage ;;
    *) break ;;
    esac
done
if [ -z "$results" ] || [ $# -ne 2 ]; then
    usage
fi
old=$(realpath "$1")
new=$(realpath "$2")
cd "$(dirname "$0")/.."

# Only the mathieee libraries, as tools/regen-results.sh does: the others
# call into parts of the OS that volamos lacks. Amiga file names are
# case-insensitive, and volamos wants the libraries' own names.
if [ -n "$libs_dir" ]; then
    rm -rf "$results/libs"
    mkdir -p "$results/libs"
    for lib in mathieee{sing,doub}{bas,trans}.library; do
        f=$(find "$libs_dir" -maxdepth 1 -iname "$lib" | head -1)
        if [ -n "$f" ]; then
            cp "$f" "$results/libs/$lib"
        else
            echo "warning: no $lib in $libs_dir, its calls stay free" >&2
        fi
    done
    libs=(--libs-dir "$results/libs")
fi

# shellcheck disable=SC2034 # read through the nameref in the loop below
v000=(--clock-mhz 7.09 --cpu 68000 "${libs[@]}")
# shellcheck disable=SC2034
v040=(--clock-mhz 25 --cpu 68040 --fpu --fpsp-emulate "${libs[@]}")
# shellcheck disable=SC2034
v060=(--clock-mhz 50 --cpu 68060 --fpu --cache --fpsp-emulate "${libs[@]}")

# name, CPUFLAGS, OPT, volamos arguments (an array's name)
configs=(
    "000-O2|-m68000|-O2 -fomit-frame-pointer|v000"
    "000-Os|-m68000|-Os -fomit-frame-pointer|v000"
    "040-O2|-m68040 -m68881|-O2 -fomit-frame-pointer|v040"
    "020fp-060-O2|-m68020-60 -m68881|-O2 -fomit-frame-pointer|v060"
)

for c in "${configs[@]}"; do
    IFS='|' read -r name cpuflags opt vargs <<<"$c"
    declare -n v=$vargs
    tools/bench-run.sh --results-dir "$results" "$old" "old-$name" "$cpuflags" "$opt" "${v[@]}"
    tools/bench-run.sh --results-dir "$results" "$new" "new-$name" "$cpuflags" "$opt" "${v[@]}"
    unset -n v
done

if grep -lE 'fail|changed|unknown benchmark' "$results"/old-*.txt "$results"/new-*.txt; then
    echo "failed benchmarks above"
    exit 1
fi

for c in "${configs[@]}"; do
    IFS='|' read -r name cpuflags opt _ <<<"$c"
    echo
    echo "## $name: \`$cpuflags $opt\`"
    echo
    tools/results-table.py --best "previous=$results/old-$name.txt" \
        "candidate=$results/new-$name.txt"
    tools/results-sizes.py --results-dir "$results" "old-$name" "new-$name"
done
