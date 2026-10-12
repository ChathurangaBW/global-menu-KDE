#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# SYNTHETIC regression tests. No real package manager or privilege tool is run.
# Variables in generated mock scripts must expand when those scripts execute.
# shellcheck disable=SC2016
set -euo pipefail

SOURCE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BASH_BIN="$(command -v bash)"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/global-menu-installer-test.XXXXXXXX")"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/bin" "$WORK/source/scripts"
cp "$SOURCE_ROOT/install.sh" "$WORK/source/install.sh"
: > "$WORK/source/CMakeLists.txt"
export MOCK_LOG="$WORK/calls" MOCK_VERSION='plasmashell 6.5.0' MOCK_STATUS=0 MOCK_SUDO_STATUS=0

# Only harmless file/path utilities are real. Everything that could mutate the
# host or fetch/build/install software is a mock in an isolated PATH.
for tool in cat dirname; do
    ln -s "$(command -v "$tool")" "$WORK/bin/$tool"
done
ln -s "$BASH_BIN" "$WORK/bin/bash"
printf '#!%s\nprintf "%%s\\n" "$MOCK_VERSION"\nexit "$MOCK_STATUS"\n' "$BASH_BIN" > "$WORK/bin/plasmashell"
printf '#!%s\nprintf "UNEXPECTED %%s %%s\\n" "${0##*/}" "$*" >> "$MOCK_LOG"\nexit 99\n' "$BASH_BIN" > "$WORK/guard"
printf '#!%s\nprintf "sudo %%s\\n" "$*" >> "$MOCK_LOG"\nexit "$MOCK_SUDO_STATUS"\n' "$BASH_BIN" > "$WORK/sudo"
printf '#!%s\nprintf "build-stub\\n" >> "$MOCK_LOG"\n' "$BASH_BIN" > "$WORK/source/scripts/install-system.sh"
chmod +x "$WORK/bin/plasmashell" "$WORK/guard" "$WORK/sudo"
for tool in git cmake ninja c++ ctest msgfmt ldd doas; do
    ln -s "$WORK/guard" "$WORK/bin/$tool"
done
ln -s "$WORK/sudo" "$WORK/bin/sudo"

fail() { printf 'FAIL: %s\n' "$*" >&2; cat "$WORK/output" >&2; exit 1; }
run_case() {
    local expected="$1" actual=0
    shift
    : > "$MOCK_LOG"
    PATH="$WORK/bin" "$BASH_BIN" "$WORK/source/install.sh" "$@" > "$WORK/output" 2>&1 || actual=$?
    [[ "$actual" -eq "$expected" ]] || fail "expected exit $expected, got $actual ($*)"
}
contains() { grep -Fq -- "$1" "$WORK/output" || fail "missing diagnostic: $1"; }
no_calls() { [[ ! -s "$MOCK_LOG" ]] || fail "unexpected command invocation: $(cat "$MOCK_LOG")"; }

printf 'Running SYNTHETIC installer regression tests (mocked commands)\n'
for version in 6.5.0 6.5.0-1.fc43 6.10.2 7.0.0; do
    export MOCK_VERSION="plasmashell $version"
    run_case 0 --check
    contains 'Preflight passed'
    no_calls
done
for version in 5.27.12 6.0.0 6.4.99; do
    export MOCK_VERSION="plasmashell $version"
    run_case 1 --check
    contains 'KDE Plasma >= 6.5 is required'
    no_calls
    if (( EUID != 0 )); then
        run_case 1
        contains 'no dependencies were changed'
        no_calls
        run_case 1 --no-deps
        contains 'no dependencies were changed'
        no_calls
    fi
done
for output in 'unknown' 'Qt 6.8.0' 'plasmashell 6.5' 'plasmashell 6.5.0.1'; do
    export MOCK_VERSION="$output"
    run_case 1 --check
    contains 'cannot parse'
    no_calls
done
export MOCK_VERSION='plasmashell 6.5.0' MOCK_STATUS=1
run_case 1 --check
contains 'could not query'
no_calls
export MOCK_STATUS=0
mv "$WORK/bin/plasmashell" "$WORK/plasmashell"
run_case 1 --check
contains 'plasmashell was not found'
no_calls
mv "$WORK/plasmashell" "$WORK/bin/plasmashell"
rm "$WORK/bin/msgfmt"
run_case 1 --check
contains 'missing: msgfmt'
no_calls
ln -s "$WORK/guard" "$WORK/bin/msgfmt"
rm "$WORK/bin/sudo" "$WORK/bin/doas"
run_case 0 --check --no-deps
no_calls
ln -s "$WORK/sudo" "$WORK/bin/sudo"
ln -s "$WORK/guard" "$WORK/bin/doas"
run_case 1 --check --uninstall
contains 'cannot be combined'
no_calls

for manager in apt-get dnf pacman zypper; do
    ln -s "$WORK/guard" "$WORK/bin/$manager"
    run_case 0 --check
    no_calls
    if [[ "$manager" == dnf ]]; then
        contains 'EPEL 10 and CRB'
        contains 'never enables repositories'
    fi
    if (( EUID != 0 )); then
        run_case 0
        grep -Eq '^sudo .* gettext( |$)' "$MOCK_LOG" || fail "$manager missing gettext"
        grep -Fq 'build-stub' "$MOCK_LOG" || fail "$manager did not reach build stub"
        if grep -Eq 'UNEXPECTED|config-manager|epel-release|--enablerepo' "$MOCK_LOG"; then
            fail 'unexpected invocation or repository mutation'
        fi
        if [[ "$manager" == dnf ]]; then
            export MOCK_SUDO_STATUS=1
            run_case 1
            contains 'dnf dependency installation failed'
            contains 'EPEL 10 and CRB'
            if grep -Fq 'build-stub' "$MOCK_LOG"; then fail 'build ran after dnf failure'; fi
            export MOCK_SUDO_STATUS=0
        fi
    fi
    rm "$WORK/bin/$manager"
done
if (( EUID == 0 )); then
    run_case 1
    contains 'regular desktop user'
    no_calls
    printf 'SKIP: install-path mock tests require non-root (root refusal verified instead)\n'
else
    printf 'NOTE: root refusal not exercised by this non-root process\n'
fi
printf 'PASS: synthetic installer regression tests\n'
