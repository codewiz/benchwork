#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Build and run one benchwork variant.
#
# Builds benchwork with a gcc toolchain and flags, runs it under volamos,
# and saves:
#   results/raw/<name>.txt     the output of the run
#   results/raw/sizes/<name>   the binary's text size
#
# The build starts from an empty directory every time: make does not notice
# a changed compiler, so a reused one would measure the old objects.
#
# Environment:
#   REPEAT  timed repetitions of each benchmark (default 1)

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -lt 5 ]; then
    echo "Usage: [REPEAT=n] $0 <toolchain dir> <name> \"<CPUFLAGS>\" \"<OPT>\" <volamos arguments...>" >&2
    exit 1
fi

toolchain=$1 name=$2 cpuflags=$3 opt=$4
shift 4

cd "$(dirname "$0")/.."
work=tmp/regen
mkdir -p "$work" results/raw/sizes

rm -rf "$work/build-$name" "$work/benchwork-$name"
make -s -j"$(nproc)" CC="$toolchain/bin/m68k-amigaos-gcc" \
    BUILD="$work/build-$name" TARGET="$work/benchwork-$name" \
    CPUFLAGS="$cpuflags" OPT="$opt" >"$work/build-$name.log" 2>&1

"$toolchain/bin/m68k-amigaos-size" "$work/benchwork-$name" | tail -1 |
    awk -v n="$name" '{ print n, $1 }' >"results/raw/sizes/$name"

volamos "$@" --stack 65536 -- "$work/benchwork-$name" REPEAT "${REPEAT:-1}" \
    >"results/raw/$name.txt" 2>&1
grep -E 'GEOMEAN|fail|changed' "results/raw/$name.txt" | sed "s/^/$name: /"
