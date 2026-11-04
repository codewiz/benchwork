#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Code sizes across the GCC 16.2 releases, for the releases page's size
# chart: one row per release, one column per program or object, the text
# size in bytes, or "-" where that release cannot build it. Saved as
# release-sizes.txt in the results directory.
#
# - benchwork-040: the benchwork-040 binary, newlib included
# - benchwork-000: the same for the 68000, built here, with soft float
# - objects: benchwork-040's own objects, summed: the compiler alone
# - curve25519.o, pngread.o, cJSON.o, deflate.o: four large objects
# - ODFileSystem, p96cts: two other programs, at a pinned release tag
#
# Each keeps its own optimization level, which holds across releases:
# Benchwork -O2, p96cts -O3, and ODFileSystem's normal handler at -Os, the
# level of its ROM build, so that at least one line is optimized for size.
#
# The Benchwork sizes come from the builds tools/regen-results.sh leaves in
# tmp/regen, so it runs after that. ODFileSystem and p96cts are built from
# worktrees of the given checkouts, at their tags, so the checkouts
# themselves are left alone.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

ODFS_TAG=v1.0.0-beta3
P96CTS_TAG=v1.0.1
RELEASES="5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23"
OBJECTS="amissl/curve25519 libpng/pngread cjson/cJSON zlib/deflate"

usage() {
    echo "Usage: $0 --results-dir <dir> --odfs <checkout> --p96cts <checkout>" >&2
    exit 1
}

results='' odfs='' p96cts=''
while [ $# -gt 0 ]; do
    [ $# -ge 2 ] || usage
    case $1 in
    --results-dir) results=$2 ;;
    --odfs) odfs=$2 ;;
    --p96cts) p96cts=$2 ;;
    *) usage ;;
    esac
    shift 2
done
if [ -z "$results" ] || [ -z "$odfs" ] || [ -z "$p96cts" ]; then
    usage
fi

results=$(realpath -m "$results")
cd "$(dirname "$0")/.."
TOOLCHAINS=${TOOLCHAINS:-$HOME/amiga}
work=$PWD/tmp/sizes
mkdir -p "$work" "$results"

# A worktree of CHECKOUT at TAG in DIR, made afresh.
worktree() {
    local checkout=$1 tag=$2 dir=$3

    if [ -e "$dir" ]; then
        git -C "$checkout" worktree remove --force "$dir"
    fi
    git -C "$checkout" worktree add -q --detach "$dir" "$tag"
    git -C "$dir" submodule -q update --init --recursive
}
worktree "$odfs" "$ODFS_TAG" "$work/odfs"
worktree "$p96cts" "$P96CTS_TAG" "$work/p96cts"

# The text size of each file named, summed, or "-" if one is missing.
text() {
    local size=$1
    shift
    for f in "$@"; do
        [ -f "$f" ] || { echo -; return; }
    done
    "$size" "$@" | awk 'NR > 1 { t += $1 } END { print t }'
}

out=$results/release-sizes.txt
{
    printf '%-8s %13s %13s %9s' release benchwork-040 benchwork-000 objects
    for o in $OBJECTS; do
        printf ' %12s' "${o#*/}.o"
    done
    printf ' %12s %9s\n' ODFileSystem p96cts
    for rc in $RELEASES; do
        bin=$TOOLCHAINS/m68k-amigaos-gcc-16.2-rc$rc/bin/m68k-amigaos
        build=tmp/regen/build-gcc-16.2-rc$rc-O2-040
        rm -rf "$work/odfs-rc$rc" "$work/p96-rc$rc" "$work/p96cts-rc$rc" \
            "$work/build-000-rc$rc" "$work/benchwork-000-rc$rc"
        make -s -j"$(nproc)" CC="$bin-gcc" CPUFLAGS=-m68000 \
            BUILD="$work/build-000-rc$rc" TARGET="$work/benchwork-000-rc$rc" \
            >"$work/benchwork-000-rc$rc.log" 2>&1 || true
        make -s -C "$work/odfs" amiga CC="$bin-gcc" AMIGA_TOOL_PREFIX="$bin" \
            AMIGA_OPT=-Os AMIGA_BUILD="$work/odfs-rc$rc" \
            >"$work/odfs-rc$rc.log" 2>&1 || true
        make -s -C "$work/p96cts" CC="$bin-gcc" BUILD="$work/p96-rc$rc" \
            TARGET="$work/p96cts-rc$rc" >"$work/p96cts-rc$rc.log" 2>&1 || true
        printf '%-8s %13s %13s %9s' "rc$rc" \
            "$(text "$bin-size" "tmp/regen/benchwork-gcc-16.2-rc$rc-O2-040")" \
            "$(text "$bin-size" "$work/benchwork-000-rc$rc")" \
            "$(find "$build" -name '*.o' -print0 | xargs -0 "$bin-size" |
                awk 'NR > 1 { t += $1 } END { print t }')"
        for o in $OBJECTS; do
            printf ' %12s' "$(text "$bin-size" "$build/third_party/$o.o")"
        done
        printf ' %12s %9s\n' "$(text "$bin-size" "$work/odfs-rc$rc/ODFileSystem")" \
            "$(text "$bin-size" "$work/p96cts-rc$rc")"
    done
} >"$out"
cat "$out"
