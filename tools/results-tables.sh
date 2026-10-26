#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Regenerate every table in the results pages.
#
# Rebuilds each table in results/*.md from results/raw and replaces it in
# place. The prose around the tables quotes numbers too, and is not touched.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -ne 0 ]; then
    echo "Usage: $0" >&2
    exit 1
fi

cd "$(dirname "$0")/.."

R=results/raw
G=$R/gcc-16.2
T=tmp/regen/tables
mkdir -p "$T"

sizes() { tools/results-sizes.py "$@"; }

# The compilers. The 64-bit and C++ benchmarks are left out of the geomean,
# which SAS/C and vbcc cannot build them for. results-table.py fails when a
# check value differs, which vbcc's miscompiles make expected here: the
# fixup reads which ones from its stderr.
{ tools/results-table.py --best \
    --geomean-skip=nfs3-project,nfs3-clip,sha512,aesgcm,chacha20poly1305,x25519 \
    "6.5.0b[^gcc6]=$R/gcc-6.5.0b-O2-040.txt" \
    "16.2-rc18[^rc18]=$G-rc18-O2-040.txt" \
    "vbcc 2026-01-12[^vbcc]=$R/vbcc-20260112-O3-040.txt" \
    "SAS/C 6.58[^sasc]=$R/sasc-6.58-040-040.txt" 2>"$T/compilers.err" ||
    true; } |
    sed -E -e 's/^\| (nfs3-[a-z]+) /| \1[^cxx] /' \
        -e 's/^\| (sha512|aesgcm|chacha20poly1305|x25519) /| \1[^longlong] /' \
    >"$T/compilers.raw"
tools/results-compilers-fixup.py "$T/compilers.raw" "$T/compilers.err" \
    >"$T/compilers.md"
tools/results-splice.py results/compilers.md "$T/compilers.md"

# The releases: the ones the chart shows, minus rc12, which only moves the
# chart's lines.
tools/results-table.py --best \
    rc5=$G-rc5-O2-040.txt rc11=$G-rc11-O2-040.txt rc14=$G-rc14-O2-040.txt \
    rc17=$G-rc17-O2-040.txt rc18=$G-rc18-O2-040.txt \
    rc20=$G-rc20-O2-040.txt >"$T/releases.md"
sizes gcc-16.2-{rc5,rc11,rc14,rc17,rc18,rc20}-O2-040 >>"$T/releases.md"
tools/results-splice.py results/gcc-releases.md "$T/releases.md"

# Optimization levels, on the 68030 with its cache.
tools/results-table.py --best \
    -O2=$G-rc20-030-O2-030-cache.txt -Os=$G-rc20-030-Os-030-cache.txt \
    -O3=$G-rc20-030-O3-030-cache.txt \
    "-O2 -flto=$G-rc20-030-O2-lto-030-cache.txt" |
    sed 's/^| dhrystone /| dhrystone[^dhry-lto] /' >"$T/opt.md"
sizes gcc-16.2-rc20-030-{O2,Os,O3,O2-lto}-030-cache >>"$T/opt.md"
tools/results-splice.py results/opt-levels.md "$T/opt.md"

# The release binaries on a 68040, then -m68020-60 against -m68060 on a
# 68060.
tools/results-table.py \
    68000=$G-rc18-000-040.txt 68020=$G-rc18-020-040.txt \
    68020-60-FP=$G-rc18-020fp-040.txt 68040-FP=$G-rc18-O2-040.txt |
    sed -E 's/^\| (wipeout-tris|nfs3-[a-z]+) /| \1[^softfloat] /' \
    >"$T/cpus.md"
sizes gcc-16.2-rc18-{000,020,020fp,O2}-040 >>"$T/cpus.md"
tools/results-table.py --best \
    "-m68020-60 -m68881=$G-rc18-020fp-060-cache.txt" \
    "-m68060 -m68881=$G-rc18-060fp-060-cache.txt" >"$T/060.md"
sizes gcc-16.2-rc18-{020fp,060fp}-060-cache >>"$T/060.md"
tools/results-splice.py results/cpus.md "$T/cpus.md" "$T/060.md"

# Register arguments.
tools/results-table.py --best \
    "stack args=$G-rc18-base-040.txt" \
    "-mregparm=2=$G-rc18-regparm2-040.txt" \
    "-mregparm=4=$G-rc18-regparm4-040.txt" \
    "-msasregparm=$G-rc18-sasregparm-040.txt" >"$T/regparm.md"
sizes gcc-16.2-rc18-{base,regparm2,regparm4,sasregparm}-040 >>"$T/regparm.md"
tools/results-splice.py results/regparm.md "$T/regparm.md"

# The emulators, against the cycle-counted 68040. Their runs are not part
# of regen-results.sh: two of them boot AmigaOS (see results/emulators.md).
E=$R/emulators
tools/results-table.py --speedup "25 MHz 68040=$G-rc18-O2-040.txt" \
    Copperline=$E/copperline-jit.txt vamos=$E/vamos.txt \
    volamos=$E/volamos-jit.txt Amiberry=$E/amiberry-jit.txt \
    >"$T/emulators.md"
tools/results-splice.py results/emulators.md "$T/emulators.md"
