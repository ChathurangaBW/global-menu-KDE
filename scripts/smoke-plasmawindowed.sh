#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

PLUGIN_ID=${1:-org.chathuranga.globalmenu}
REMOVE_LOG=0
if [[ -n "${LOG_FILE:-}" ]]; then
    LOG_FILE=${LOG_FILE}
else
    LOG_FILE=$(mktemp)
    REMOVE_LOG=1
fi

cleanup() {
    if [[ "$REMOVE_LOG" -eq 1 ]]; then
        rm -f -- "$LOG_FILE"
    fi
}
trap cleanup EXIT

if ! command -v plasmawindowed >/dev/null 2>&1; then
    echo "plasmawindowed is required for the applet smoke test" >&2
    exit 1
fi

native_smoke=0
command=(plasmawindowed)
if plasmawindowed --help 2>&1 | grep -q -- '--smoke-test'; then
    native_smoke=1
    command+=(--smoke-test)
fi
command+=("$PLUGIN_ID")

if [[ -z "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]]; then
    if ! command -v xvfb-run >/dev/null 2>&1; then
        echo "No graphical display is available and xvfb-run is not installed" >&2
        exit 1
    fi
    command=(xvfb-run -a "${command[@]}")
fi

if [[ -z "${DBUS_SESSION_BUS_ADDRESS:-}" ]]; then
    if ! command -v dbus-run-session >/dev/null 2>&1; then
        echo "No D-Bus session is available and dbus-run-session is not installed" >&2
        exit 1
    fi
    command=(dbus-run-session -- "${command[@]}")
fi

set +e
if [[ "$native_smoke" -eq 1 ]]; then
    timeout --signal=TERM --kill-after=2s 20s "${command[@]}" >"$LOG_FILE" 2>&1
else
    timeout --signal=TERM --kill-after=2s 8s "${command[@]}" >"$LOG_FILE" 2>&1
fi
status=$?
set -e

cat "$LOG_FILE"

if grep -Eiq \
    'Could not find requested component|Could not load applet|Error loading QML|QQml[^:]*:.*error|module .* is not installed|failed to load|org\.chathuranga\.globalmenu.*not found' \
    "$LOG_FILE"; then
    exit 1
fi

if [[ "$native_smoke" -eq 1 ]]; then
    if [[ "$status" -ne 0 ]]; then
        exit "$status"
    fi
elif [[ "$status" -ne 0 && "$status" -ne 124 ]]; then
    exit "$status"
fi
