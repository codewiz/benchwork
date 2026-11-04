#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Regenerate the charts in the results pages and README.md.
#
# Redraws results/*.svg from the raw results in --results-dir, usually
# results/raw.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -ne 2 ] || [ "$1" != --results-dir ]; then
    echo "Usage: $0 --results-dir <dir>" >&2
    exit 1
fi

R=$(realpath -m "$2")
cd "$(dirname "$0")/.."

G=$R/gcc-16.2
size() { cut -d' ' -f2 "$R/sizes/gcc-16.2-$1"; }

tools/results-chart.py \
    --geomean-skip=nfs3-project,nfs3-clip,sha512,aesgcm,chacha20poly1305,x25519 \
    -o results/compilers-040.svg \
    -t "m68k compilers on benchwork-040, speed relative to gcc 6.5.0b" \
    "gcc 6.5.0b=$R/gcc-6.5.0b-O2-040.txt" "gcc 16.2-rc18=$G-rc18-O2-040.txt" \
    "vbcc 2026-01-12=$R/vbcc-20260112-O3-040.txt" \
    "SAS/C 6.58=$R/sasc-6.58-040-040.txt"

# The releases, the application workloads and the memory group apart: each
# chart has the releases in which one of its benchmarks moves by more than
# 1%, and the latest.
L=$R/releases/gcc-16.2
tools/results-chart.py --lines --skip-groups=memory -o results/gcc-releases-040.svg \
    -t "GCC 16.2 releases on benchwork-040, time in ms" \
    "rc5=$L-rc5-O2-040.txt" "rc11=$L-rc11-O2-040.txt" "rc12=$L-rc12-O2-040.txt" \
    "rc14=$L-rc14-O2-040.txt" "rc15=$L-rc15-O2-040.txt" "rc17=$L-rc17-O2-040.txt" \
    "rc18=$L-rc18-O2-040.txt" "rc20=$L-rc20-O2-040.txt" "rc22=$L-rc22-O2-040.txt" \
    "rc23=$L-rc23-O2-040.txt"
tools/results-chart.py --lines --groups=memory -o results/gcc-releases-memory-040.svg \
    -t "GCC 16.2 releases on benchwork-040, memory benchmarks, time in ms" \
    "rc5=$L-rc5-O2-040.txt" "rc11=$L-rc11-O2-040.txt" "rc12=$L-rc12-O2-040.txt" \
    "rc14=$L-rc14-O2-040.txt" "rc18=$L-rc18-O2-040.txt" "rc20=$L-rc20-O2-040.txt" \
    "rc23=$L-rc23-O2-040.txt"
tools/results-chart.py --size-lines "$R/releases/release-sizes.txt" \
    -o results/gcc-releases-size-040.svg \
    -t "Code size in the GCC 16.2 releases, in bytes"

tools/results-chart.py --hide-base -o results/opt-levels-030.svg \
    -t "GCC 16.2-rc20 optimization levels on a 68030 with cache, speed relative to -O2" \
    -- "-O2=$G-rc20-030-O2-030-cache.txt" "-Os=$G-rc20-030-Os-030-cache.txt" \
    "-O3=$G-rc20-030-O3-030-cache.txt" \
    "-O2 -flto=$G-rc20-030-O2-lto-030-cache.txt"

tools/results-chart.py --sizes -o results/opt-levels-size-030.svg \
    -t "Code size of each optimization level relative to -O2" \
    -- "-O2=$(size rc20-030-O2-030-cache)" "-Os=$(size rc20-030-Os-030-cache)" \
    "-O3=$(size rc20-030-O3-030-cache)" \
    "-O2 -flto=$(size rc20-030-O2-lto-030-cache)"

tools/results-chart.py --hide-base -o results/regparm-040.svg \
    -t "GCC 16.2-rc21 register arguments on a 68040, speed relative to stack arguments" \
    -- "stack args=$G-rc21-base-040.txt" "-mregparm=2=$G-rc21-regparm2-040.txt" \
    "-mregparm=4=$G-rc21-regparm4-040.txt" \
    "-msasregparm=$G-rc21-sasregparm-040.txt"

C=$R/runtimes
tools/results-chart.py --hide-base -o results/runtimes-040.svg \
    -t "C runtimes on a 68040 under AmigaOS 3.2.3, speed relative to newlib" \
    "newlib=$C/newlib.txt" "libnix=$C/nix20.txt" "clib2=$C/clib2.txt"

# The emulators, against the cycle-counted 68040: up to two orders of
# magnitude apart, hence the log axis.
E=$R/emulators
tools/results-chart.py --log --hide-base -o results/emulators-040.svg \
    -t "m68k emulators on benchwork-040, speed relative to a 25 MHz 68040" \
    "25 MHz 68040=$G-rc18-O2-040.txt" "Copperline=$E/copperline-jit.txt" \
    "vamos=$E/vamos.txt" "volamos=$E/volamos-jit.txt" \
    "Amiberry=$E/amiberry-jit.txt" "Emu68=$E/emu68-1800.txt"
