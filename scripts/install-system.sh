#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ChathurangaBW
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build-system"}
LEGACY_ENV_FILE="${XDG_CONFIG_HOME:-$HOME/.config}/plasma-workspace/env/global-menu-kde.sh"
SYSTEM_MANIFEST_DIR=/var/lib/global-menu-kde
SYSTEM_MANIFEST="$SYSTEM_MANIFEST_DIR/install_manifest.txt"
SYSTEM_UNINSTALLER=/usr/local/bin/global-menu-kde-uninstall

fail() {
    printf 'global-menu-kde: %s\n' "$*" >&2
    exit 1
}

if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
    fail "run this installer as your regular desktop user, without sudo"
fi

if command -v sudo >/dev/null 2>&1; then
    ROOT=(sudo)
elif command -v doas >/dev/null 2>&1; then
    ROOT=(doas)
else
    fail "sudo or doas is required for system-wide installation"
fi

as_root() {
    "${ROOT[@]}" "$@"
}

for command_name in cmake ninja ctest ldd; do
    command -v "$command_name" >/dev/null 2>&1 || fail "required command not found: $command_name"
done

printf 'Configuring KDE Global Menu for native system installation...\n'
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=ON

cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure --timeout 60

printf 'Installing into the system Plasma plugin path...\n'
as_root cmake --install "$BUILD_DIR"

MANIFEST="$BUILD_DIR/install_manifest.txt"
[[ -s "$MANIFEST" ]] || fail "CMake did not create an install manifest"

PLUGIN_FILE=$(grep -E '/plasma/applets/org\.chathuranga\.globalmenu\.so$' "$MANIFEST" | head -n 1 || true)
if [[ -z "$PLUGIN_FILE" || ! -f "$PLUGIN_FILE" ]]; then
    fail "installation completed, but the native Plasma plugin could not be verified"
fi

printf 'Auditing installed plugin dependencies...\n'
if ldd "$PLUGIN_FILE" | tee /tmp/global-menu-kde-ldd.txt | grep -q 'not found'; then
    cat /tmp/global-menu-kde-ldd.txt >&2
    fail "installed plugin has unresolved shared-library dependencies"
fi
rm -f /tmp/global-menu-kde-ldd.txt

# Persist the exact CMake manifest because curl|bash installations use a
# temporary source checkout. This keeps uninstall deterministic afterwards.
as_root install -d -m 0755 "$SYSTEM_MANIFEST_DIR"
as_root install -m 0644 "$MANIFEST" "$SYSTEM_MANIFEST"
as_root install -m 0755 "$ROOT_DIR/scripts/uninstall-system.sh" "$SYSTEM_UNINSTALLER"

# Remove only leftovers created by the discarded user-local implementation.
# The native implementation uses Qt/KDE's normal system plugin discovery.
rm -f -- "$LEGACY_ENV_FILE"
if [[ -d "$HOME/.local" ]]; then
    while IFS= read -r legacy_plugin; do
        rm -f -- "$legacy_plugin"
        printf 'Removed legacy user plugin: %s\n' "$legacy_plugin"
    done < <(find "$HOME/.local" -type f -path '*/plasma/applets/org.chathuranga.globalmenu.so' -print 2>/dev/null)
fi

if command -v kbuildsycoca6 >/dev/null 2>&1; then
    if ! kbuildsycoca6 --noincremental; then
        printf 'Warning: kbuildsycoca6 cache refresh failed; log out/in before adding the widget.\n' >&2
    fi
fi

cat <<MSG

KDE Global Menu installed successfully.

Native plugin:
  $PLUGIN_FILE

No QT_PLUGIN_PATH override is used.

Log out and log back in once. Then open Edit Mode -> Add Widgets and drag
"KDE Global Menu" directly into the existing panel, like KDE's native Global
Menu widget.

Uninstall later with:
  global-menu-kde-uninstall
MSG
