#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -u

config_file=$1
shift

bus_info=$(dbus-daemon --config-file="$config_file" --fork --print-address=1 --print-pid=1)
bus_address=$(printf '%s\n' "$bus_info" | sed -n '1p')
bus_pid=$(printf '%s\n' "$bus_info" | sed -n '2p')

cleanup()
{
    kill "$bus_pid" 2>/dev/null || true
}
trap cleanup EXIT

export DBUS_SESSION_BUS_ADDRESS=$bus_address
"$@"
