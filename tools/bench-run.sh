#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Build and run one benchwork variant.
#
# Builds benchwork with a gcc toolchain and flags, runs it under volamos,
# and saves, in the results directory:
#   <name>.txt     the output of the run
#   sizes/<name>   the binary's text size
#
# --repeat sets the timed repetitions of each benchmark (default 1).
#
# The build starts from an empty directory every time: make does not notice
# a changed compiler, so a reused one would measure the old objects.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

usage() {
    echo "Usage: $0 --results-dir <dir> [--repeat <n>] <toolchain dir> <name> \"<CPUFLAGS>\" \"<OPT>\" <volamos arguments...>" >&2
    exit 1
}

results=
repeat=1
while [ $# -gt 0 ]; do
    case $1 in
    --results-dir)
        [ $# -ge 2 ] || usage
        results=$2
        shift 2
        ;;
    --repeat)
        [ $# -ge 2 ] || usage
        repeat=$2
        shift 2
        ;;
    -*) usage ;;
    *) break ;;
    esac
done

if [ -z "$results" ] || [ $# -lt 5 ]; then
    usage
fi

toolchain=$1 name=$2 cpuflags=$3 opt=$4
shift 4

results=$(realpath -m "$results")
cd "$(dirname "$0")/.."
work=tmp/regen
mkdir -p "$work" "$results/sizes"

rm -rf "$work/build-$name" "$work/benchwork-$name"
make -s -j"$(nproc)" CC="$toolchain/bin/m68k-amigaos-gcc" \
    BUILD="$work/build-$name" TARGET="$work/benchwork-$name" \
    CPUFLAGS="$cpuflags" OPT="$opt" >"$work/build-$name.log" 2>&1

"$toolchain/bin/m68k-amigaos-size" "$work/benchwork-$name" | tail -1 |
    awk -v n="$name" '{ print n, $1 }' >"$results/sizes/$name"

volamos "$@" --stack 65536 -- "$work/benchwork-$name" REPEAT "$repeat" \
    >"$results/$name.txt" 2>&1
grep -E 'GEOMEAN|fail|changed' "$results/$name.txt" | sed "s/^/$name: /"
