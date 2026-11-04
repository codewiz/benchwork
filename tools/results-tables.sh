#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Regenerate every table in the results pages.
#
# Rebuilds each table in results/*.md from the raw results in --results-dir,
# usually results/raw, and replaces it in place. The prose around the tables
# quotes numbers too, and is not touched.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -ne 2 ] || [ "$1" != --results-dir ]; then
    echo "Usage: $0 --results-dir <dir>" >&2
    exit 1
fi

R=$(realpath -m "$2")
cd "$(dirname "$0")/.."

G=$R/gcc-16.2
T=tmp/regen/tables
mkdir -p "$T"

sizes() { tools/results-sizes.py --results-dir "$R" "$@"; }

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

# The releases, run apart with the JSON benchmarks: the application
# workloads, then the memory group, each with a geomean of its own. The
# releases are the ones each chart shows, minus rc12 and rc15, which only
# move the chart's lines.
L=$R/releases/gcc-16.2
tools/results-table.py --best --skip-groups=memory \
    "rc5=$L-rc5-O2-040.txt" "rc11=$L-rc11-O2-040.txt" "rc14=$L-rc14-O2-040.txt" \
    "rc17=$L-rc17-O2-040.txt" "rc18=$L-rc18-O2-040.txt" \
    "rc20=$L-rc20-O2-040.txt" "rc22=$L-rc22-O2-040.txt" \
    "rc23=$L-rc23-O2-040.txt" >"$T/releases.md"
tools/results-sizes.py --results-dir "$R/releases" \
    gcc-16.2-{rc5,rc11,rc14,rc17,rc18,rc20,rc22,rc23}-O2-040 >>"$T/releases.md"
tools/results-table.py --best --groups=memory \
    "rc5=$L-rc5-O2-040.txt" "rc11=$L-rc11-O2-040.txt" "rc14=$L-rc14-O2-040.txt" \
    "rc18=$L-rc18-O2-040.txt" "rc20=$L-rc20-O2-040.txt" \
    "rc23=$L-rc23-O2-040.txt" >"$T/releases-memory.md"
tools/results-splice.py results/gcc-releases.md "$T/releases.md" \
    "$T/releases-memory.md"

# Optimization levels, on the 68030 with its cache.
tools/results-table.py --best \
    "-O2=$G-rc20-030-O2-030-cache.txt" "-Os=$G-rc20-030-Os-030-cache.txt" \
    "-O3=$G-rc20-030-O3-030-cache.txt" \
    "-O2 -flto=$G-rc20-030-O2-lto-030-cache.txt" |
    sed 's/^| dhrystone /| dhrystone[^dhry-lto] /' >"$T/opt.md"
sizes gcc-16.2-rc20-030-{O2,Os,O3,O2-lto}-030-cache >>"$T/opt.md"
tools/results-splice.py results/opt-levels.md "$T/opt.md"

# The release binaries on a 68040, then -m68020-60 against -m68060 on a
# 68060.
tools/results-table.py \
    "68000=$G-rc18-000-040.txt" "68020=$G-rc18-020-040.txt" \
    "68020-60-FP=$G-rc18-020fp-040.txt" "68040-FP=$G-rc18-O2-040.txt" |
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
    "stack args=$G-rc21-base-040.txt" \
    "-mregparm=2=$G-rc21-regparm2-040.txt" \
    "-mregparm=4=$G-rc21-regparm4-040.txt" \
    "-msasregparm=$G-rc21-sasregparm-040.txt" >"$T/regparm.md"
sizes gcc-16.2-rc21-{base,regparm2,regparm4,sasregparm}-040 >>"$T/regparm.md"
tools/results-splice.py results/regparm.md "$T/regparm.md"

# The C runtimes. Their runs are not part of regen-results.sh: they boot
# AmigaOS 3.2.3 on Amiberry (see results/runtimes.md).
C=$R/runtimes
tools/results-table.py --best "newlib=$C/newlib.txt" "libnix=$C/nix20.txt" \
    "clib2=$C/clib2.txt" >"$T/runtimes.md"
sizes runtimes-{newlib,nix20,clib2} >>"$T/runtimes.md"
# The JSON benchmarks across releases, a table each. The clib2 of rc21 and
# rc22 hangs before printing anything, so there is no file for it.
for b in json-parse json-print; do
    tools/results-releases-table.py --releases rc21,rc22,rc23 --bench "$b" \
        "newlib=$C/json-rc21-newlib.txt,$C/json-rc22-newlib.txt,$C/json-rc23-newlib.txt" \
        "libnix=$C/json-rc21-nix20.txt,$C/json-rc22-nix20.txt,$C/json-rc23-nix20.txt" \
        "clib2=hangs,hangs,$C/json-rc23-clib2.txt" >"$T/runtimes-$b.md"
done
tools/results-splice.py results/runtimes.md "$T/runtimes.md" \
    "$T/runtimes-json-parse.md" "$T/runtimes-json-print.md"

# The emulators, against the cycle-counted 68040. Their runs are not part
# of regen-results.sh: three of them boot AmigaOS, one on a PiStorm (see
# results/emulators.md). Emu68's nfs3 check values differ from the others',
# which the footnote explains.
E=$R/emulators
{ tools/results-table.py --speedup "25 MHz 68040=$G-rc18-O2-040.txt" \
    "Copperline=$E/copperline-jit.txt" "vamos=$E/vamos.txt" \
    "volamos=$E/volamos-jit.txt" "Amiberry=$E/amiberry-jit.txt" \
    "Emu68[^emu68]=$E/emu68-1800.txt" 2>"$T/emulators.err" || true; } |
    sed -E 's/^\| (nfs3-[a-z]+) /| \1[^emu68fpu] /' >"$T/emulators.md"
if grep -v '^nfs3-[a-z]*: check values differ' "$T/emulators.err"; then
    exit 1
fi
tools/results-splice.py results/emulators.md "$T/emulators.md"
