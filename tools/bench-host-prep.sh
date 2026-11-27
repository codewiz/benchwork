#!/bin/bash
# SPDX-License-Identifier: 0BSD
#
# Hold the host CPU at its nominal clock for benchmarks timed in host time,
# or give it back its boost afterwards.
#
# The emulator page times Amiberry, Copperline, vamos and volamos's JIT in
# host time, as does any run without volamos --clock-mhz. Boost clocks
# depend on temperature and on the rest of the load, so a laptop throttles
# when it likes; at the nominal clock (2 GHz on a
# Ryzen AI 9 HX 370) every core runs at the same speed for as long as it
# takes. This switches boost off, checks that every core's maximum is its
# nominal frequency, and then that every core runs at it under load: after
# a resume from s2idle, amd-pstate-epp has been seen to boost the cores to
# 5 GHz with boost off and a nominal maximum.
#
# --restore switches boost back on and raises every core's maximum to its
# hardware one, which switching boost off lowered and switching it on
# leaves, then checks that every core can run past its nominal frequency
# again. A tuned profile that keeps boost off, as the power saver one does,
# leaves the cores at nominal.
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

if [ -n "$restore" ]; then
    if [ "$(cat "$cpufreq/boost")" -eq 0 ] || grep -qx 0 "$cpufreq"/policy*/boost; then
        echo "the tuned profile keeps boost off, so the cores stay at nominal"
        exit 0
    fi
    for policy in "$cpufreq"/policy*; do
        cat "$policy/cpuinfo_max_freq" >"$policy/scaling_max_freq"
    done
fi

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

# A busy loop on every core, and each core's average clock under it, with
# 5% for the counters' jitter.
if [ -z "$restore" ]; then
    for cpu in /sys/devices/system/cpu/cpu[0-9]*; do
        timeout 3 taskset -c "${cpu##*/cpu}" sh -c 'while :; do :; done' &
    done
    sleep 2
    for cpu in /sys/devices/system/cpu/cpu[0-9]*; do
        avg=$(($(cat "$cpu/cpufreq/cpuinfo_avg_freq") / 1000))
        nominal=$(cat "$cpu/acpi_cppc/nominal_freq")
        if [ "$avg" -gt $((nominal * 105 / 100)) ]; then
            echo "${cpu##*/}: $avg MHz under load, nominal $nominal MHz" >&2
            wrong=1
        fi
    done
    wait
    if [ "$wrong" -ne 0 ]; then
        exit 1
    fi
fi

if [ -n "$restore" ]; then
    echo "boost is back: every core can run past its nominal $nominal MHz"
else
    echo "every core is held at its nominal $nominal MHz, also under load"
fi
