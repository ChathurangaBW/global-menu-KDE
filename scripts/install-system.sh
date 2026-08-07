#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ChathurangaBW
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build"}
PLUGIN_ID=org.chathuranga.globalmenu
LEGACY_ENV_FILE="${XDG_CONFIG_HOME:-$HOME/.config}/plasma-workspace/env/global-menu-kde.sh"

for command in cmake sudo; do
    if ! command -v "$command" >/dev/null 2>&1; then
        printf 'Required command not found: %s\n' "$command" >&2
        exit 1
    fi
done

printf 'Configuring Global Menu KDE for native system installation...\n'
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_TESTING=ON

cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure --timeout 60

printf 'Installing into the system Plasma plugin path...\n'
sudo cmake --install "$BUILD_DIR"

MANIFEST="$BUILD_DIR/install_manifest.txt"
PLUGIN_FILE=$(grep -E '/plasma/applets/org\.chathuranga\.globalmenu\.so$' "$MANIFEST" | head -n 1 || true)
if [[ -z "$PLUGIN_FILE" || ! -f "$PLUGIN_FILE" ]]; then
    echo "Installation completed, but the native Plasma plugin could not be verified." >&2
    exit 1
fi

# Remove only leftovers created by the discarded user-local implementation.
# The native rewrite does not use QT_PLUGIN_PATH and must survive a fresh login
# through Qt/KDE's normal system plugin discovery.
rm -f -- "$LEGACY_ENV_FILE"
if [[ -d "$HOME/.local" ]]; then
    while IFS= read -r legacy_plugin; do
        rm -f -- "$legacy_plugin"
        printf 'Removed legacy user plugin: %s\n' "$legacy_plugin"
    done < <(find "$HOME/.local" -type f -path '*/plasma/applets/org.chathuranga.globalmenu.so' -print 2>/dev/null)
fi

if command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 --noincremental
fi

cat <<MSG

Global Menu KDE installed successfully.

Native plugin:
  $PLUGIN_FILE

The rewrite does NOT use a user QT_PLUGIN_PATH hook.

To make a running Plasma session rescan newly installed binary applets, log out
and log back in once. Then open Edit Mode -> Add Widgets and drag
"Global Menu KDE" directly into the existing panel, like KDE's native Global
Menu widget.
MSG
