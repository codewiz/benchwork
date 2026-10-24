#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Regenerate the charts in the results pages and README.md.
#
# Redraws results/*.svg from results/raw.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -ne 0 ]; then
    echo "Usage: $0" >&2
    exit 1
fi

cd "$(dirname "$0")/.."

R=results/raw
G=$R/gcc-16.2
size() { cut -d' ' -f2 "$R/sizes/gcc-16.2-$1"; }

tools/results-chart.py \
    --geomean-skip=nfs3-project,nfs3-clip,sha512,aesgcm,chacha20poly1305,x25519 \
    -o results/compilers-040.svg \
    -t "m68k compilers on benchwork-040, speed relative to gcc 6.5.0b" \
    "gcc 6.5.0b=$R/gcc-6.5.0b-O2-040.txt" "gcc 16.2-rc18=$G-rc18-O2-040.txt" \
    "vbcc 2026-01-12=$R/vbcc-20260112-O3-040.txt" \
    "SAS/C 6.58=$R/sasc-6.58-040-040.txt"

tools/results-chart.py --lines -o results/gcc-releases-040.svg \
    -t "GCC 16.2 releases on benchwork-040, speed relative to rc5" \
    rc5=$G-rc5-O2-040.txt rc11=$G-rc11-O2-040.txt rc12=$G-rc12-O2-040.txt \
    rc14=$G-rc14-O2-040.txt rc17=$G-rc17-O2-040.txt rc18=$G-rc18-O2-040.txt

tools/results-chart.py -o results/opt-levels-030.svg \
    -t "GCC 16.2-rc18 optimization levels on a 68030 with cache, speed relative to -O2" \
    -- -O2=$G-rc18-030-O2-030-cache.txt -Os=$G-rc18-030-Os-030-cache.txt \
    -O3=$G-rc18-030-O3-030-cache.txt \
    "-O2 -flto=$G-rc18-030-O2-lto-030-cache.txt"

tools/results-chart.py --sizes -o results/opt-levels-size-030.svg \
    -t "Code size of each optimization level relative to -O2" \
    -- "-O2=$(size rc18-030-O2-030-cache)" "-Os=$(size rc18-030-Os-030-cache)" \
    "-O3=$(size rc18-030-O3-030-cache)" \
    "-O2 -flto=$(size rc18-030-O2-lto-030-cache)"
