#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Rerun every measurement behind the results pages.
#
# Runs every build under volamos, then regenerates the tables and charts.
# Takes about an hour. The prose of the pages quotes numbers too: review it
# before committing. Given a directory name, the previous raw results are
# moved to results/raw/<dir>/ first.
#
# Environment:
#   TOOLCHAINS  where the AmigaPorts release tarballs are unpacked, as
#               m68k-amigaos-gcc-16.2-rc<N> and m68k-amigaos-gcc-6.5
#               (default ~/amiga)
#   VBCC_TOOLCHAIN  the toolchain whose vbccm68k is measured, one with
#               vbcc's 2026-01-12 sources (default the local build,
#               $TOOLCHAINS/m68k-amigaos-gcc-16.2)
#   SC          a wrapper that runs SAS/C's sc under volamos
#               (default sc-volamos)

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -gt 1 ]; then
    echo "Usage: $0 [<dir to keep the previous raw results in>]" >&2
    exit 1
fi

cd "$(dirname "$0")/.."

TOOLCHAINS=${TOOLCHAINS:-$HOME/amiga}
SC=${SC:-sc-volamos}
tc=$TOOLCHAINS/m68k-amigaos-gcc
rc18=$tc-16.2-rc18
rc19=$tc-16.2-rc19
vbcc_tc=${VBCC_TOOLCHAIN:-$tc-16.2}
run=tools/bench-run.sh

o2="-O2 -fomit-frame-pointer"
v030=(--clock-mhz 25 --cpu 68030 --fpu --cache)
v040=(--clock-mhz 25 --cpu 68040 --fpu)
v060=(--clock-mhz 50 --cpu 68060 --fpu --cache)

work=tmp/regen
mkdir -p "$work"
exec > >(tee "$work/regen-results.log") 2>&1

if [ -n "${1:-}" ]; then
    mkdir -p "results/raw/$1"
    mv results/raw/*.txt "results/raw/$1/"
    if [ -d results/raw/sizes ]; then
        mv results/raw/sizes "results/raw/$1/"
    fi
fi

# gcc 6.5 and every 16.2 release, for the compilers and releases pages.
"$run" "$tc-6.5" gcc-6.5.0b-O2-040 "-m68040 -m68881" "$o2" "${v040[@]}"
for rc in 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19; do
    "$run" "$tc-16.2-rc$rc" "gcc-16.2-rc$rc-O2-040" "-m68040 -m68881" "$o2" \
        "${v040[@]}"
done

# rc18's release binaries on the 68040.
"$run" "$rc18" gcc-16.2-rc18-000-040 -m68000 "$o2" "${v040[@]}"
"$run" "$rc18" gcc-16.2-rc18-020-040 -m68020 "$o2" "${v040[@]}"
"$run" "$rc18" gcc-16.2-rc18-020fp-040 "-m68020-60 -m68881" "$o2" "${v040[@]}"

# Optimization levels, on a 68030 with its instruction cache.
for opt in O2 Os O3; do
    "$run" "$rc19" "gcc-16.2-rc19-030-$opt-030-cache" "-m68030 -m68881" \
        "-$opt -fomit-frame-pointer" "${v030[@]}"
done
"$run" "$rc19" gcc-16.2-rc19-030-O2-lto-030-cache "-m68030 -m68881" \
    "$o2 -flto" "${v030[@]}"

# The 68060.
"$run" "$rc18" gcc-16.2-rc18-020fp-060-cache "-m68020-60 -m68881" "$o2" \
    "${v060[@]}"
"$run" "$rc18" gcc-16.2-rc18-060fp-060-cache "-m68060 -m68881" "$o2" \
    "${v060[@]}"

# Register arguments, two repetitions.
for v in base: regparm2:-mregparm=2 regparm4:-mregparm=4 \
    sasregparm:-msasregparm; do
    REPEAT=2 "$run" "$rc18" "gcc-16.2-rc18-${v%%:*}-040" "-m68040 -m68881" \
        "$o2 ${v#*:}" "${v040[@]}"
done

# vbcc: the compiler from VBCC_TOOLCHAIN, with rc18's vc, config, headers
# and libraries. rc18's config names the paths the tarball was built at, so
# a copy points them at the installs.
#
# - -c11: rc18's NDK timer.h gives struct timeval anonymous unions, which
#   this vbcc takes only in C11 mode. Drop it once a release has the NDK
#   patch that leaves them out for vbcc.
sed -e "s|/home/runner/m68k-amigaos-gcc-16.2|$rc18|g" \
    -e "s|=vbccm68k -quiet|=$vbcc_tc/bin/vbccm68k -c11 -quiet|" \
    -e "s|=vbccm68k %s|=$vbcc_tc/bin/vbccm68k -c11 %s|" \
    -e "s|=vasmm68k_mot|=$rc18/bin/vasmm68k_mot|" \
    -e "s|=vlink|=$rc18/bin/vlink|" \
    "$rc18/bin/aos68k" >"$work/aos68k"

rm -rf "$work/build-vbcc-040" "$work/benchwork-vbcc-040"
make -s -j"$(nproc)" CC="$rc18/bin/vc +$PWD/$work/aos68k" \
    BUILD="$work/build-vbcc" TARGET="$work/benchwork-vbcc" release-040 \
    >"$work/build-vbcc.log" 2>&1

# volamos 0.13 lets vbcc's startup code clip the last word of the command
# line (sidick/volamos#205), so the benchmarks are named, and a clipped name
# still matches. vbcc's run always has failed benchmarks, so benchwork's exit
# status is no news here; the check below looks at what failed.
vbcc=$work/benchwork-vbcc-040
names=$(volamos "${v040[@]}" --stack 65536 -- "$vbcc" LIST - | cut -d' ' -f1)
# shellcheck disable=SC2086 # one word per benchmark
{ volamos "${v040[@]}" --stack 65536 -- "$vbcc" REPEAT 1 $names 2>&1 ||
    true; } >results/raw/vbcc-20260112-O3-040.txt
echo "vbcc: $(grep GEOMEAN results/raw/vbcc-20260112-O3-040.txt)"

# SAS/C, compiled under volamos: slow, and two jobs at most.
rm -rf "$work/build-sasc-040" "$work/benchwork-sasc-040"
make -s -j2 CC="$SC" BUILD="$work/build-sasc" TARGET="$work/benchwork-sasc" \
    release-040 >"$work/build-sasc.log" 2>&1

volamos "${v040[@]}" --stack 65536 -- "$work/benchwork-sasc-040" REPEAT 1 \
    >results/raw/sasc-6.58-040-040.txt 2>&1
echo "sasc: $(grep GEOMEAN results/raw/sasc-6.58-040-040.txt)"

# Any failure but vbcc's known ones would make the tables lie.
if grep -lE 'fail|changed|unknown benchmark' results/raw/*.txt | grep -v vbcc
then
    echo "unexpected failures above"
    exit 1
fi

tools/results-tables.sh
tools/results-charts.sh

rm -rf "$work"/build-* "$work"/benchwork-*
git status --short results/
