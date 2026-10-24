#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Hold the host CPU at its nominal clock for timing the JIT emulators, or
# give it back its boost afterwards.
#
# The emulator page times Amiberry, Copperline, vamos and volamos's JIT in
# host time. Boost clocks depend on temperature and on the rest of the load,
# so a laptop throttles when it likes; at the nominal clock (2 GHz on a
# Ryzen AI 9 HX 370) every core runs at the same speed for as long as it
# takes. This switches boost off, then checks that every core's maximum is
# its nominal frequency.
#
# --restore switches boost back on, as the power profiles leave it, and
# checks that every core can run past its nominal frequency again: after a
# resume from s2idle, amd-pstate-epp has been seen to keep the cores at
# nominal with boost on, which a boost toggle clears.
#
# Both restart tuned, which applies the profile KDE's power applet selects
# and can stop applying it after a suspend. Runs itself under sudo.

set -Eeuo pipefail
trap 'echo "$0: line $LINENO: $BASH_COMMAND failed" >&2' ERR

if [ $# -gt 1 ] || { [ $# -eq 1 ] && [ "$1" != --restore ]; }; then
    echo "Usage: $0 [--restore]" >&2
    exit 1
fi

if [ "$(id -u)" -ne 0 ]; then
    exec sudo "$0" "$@"
fi

cpufreq=/sys/devices/system/cpu/cpufreq
restore=${1:+1}

echo 0 >"$cpufreq/boost"
if [ -n "$restore" ]; then
    echo 1 >"$cpufreq/boost"
fi
systemctl restart tuned

echo "tuned profile $(tuned-adm active | sed 's/.*: //')," \
    "governor $(cat "$cpufreq/policy0/scaling_governor")," \
    "energy preference $(cat "$cpufreq/policy0/energy_performance_preference")"

wrong=0
for cpu in /sys/devices/system/cpu/cpu[0-9]*; do
    max=$(($(cat "$cpu/cpufreq/scaling_max_freq") / 1000))
    nominal=$(cat "$cpu/acpi_cppc/nominal_freq")
    if { [ -n "$restore" ] && [ "$max" -le "$nominal" ]; } ||
        { [ -z "$restore" ] && [ "$max" -ne "$nominal" ]; }; then
        echo "${cpu##*/}: maximum $max MHz, nominal $nominal MHz" >&2
        wrong=1
    fi
done
if [ "$wrong" -ne 0 ]; then
    exit 1
fi
if [ -n "$restore" ]; then
    echo "boost is back: every core can run past its nominal $nominal MHz"
else
    echo "every core is held at its nominal $nominal MHz"
fi
