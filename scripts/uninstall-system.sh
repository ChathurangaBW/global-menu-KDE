#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ChathurangaBW
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

PLUGIN_ID=org.chathuranga.globalmenu
LEGACY_ENV_FILE="${XDG_CONFIG_HOME:-$HOME/.config}/plasma-workspace/env/global-menu-kde.sh"

plugin_roots=()
if command -v qtpaths6 >/dev/null 2>&1; then
    qt_plugin_dir=$(qtpaths6 --plugin-dir 2>/dev/null || true)
    if [[ -n "$qt_plugin_dir" ]]; then
        plugin_roots+=("$qt_plugin_dir")
    fi
fi
plugin_roots+=(/usr/lib/qt6/plugins /usr/lib64/qt6/plugins)

removed=0
for root in "${plugin_roots[@]}"; do
    plugin="$root/plasma/applets/$PLUGIN_ID.so"
    if [[ -f "$plugin" ]]; then
        sudo rm -f -- "$plugin"
        printf 'Removed native plugin: %s\n' "$plugin"
        removed=1
    fi
done

rm -f -- "$LEGACY_ENV_FILE"
if [[ -d "$HOME/.local" ]]; then
    while IFS= read -r legacy_plugin; do
        rm -f -- "$legacy_plugin"
        printf 'Removed legacy user plugin: %s\n' "$legacy_plugin"
        removed=1
    done < <(find "$HOME/.local" -type f -path '*/plasma/applets/org.chathuranga.globalmenu.so' -print 2>/dev/null)
fi

if command -v kbuildsycoca6 >/dev/null 2>&1; then
    kbuildsycoca6 --noincremental
fi

if [[ $removed -eq 0 ]]; then
    echo "Global Menu KDE plugin was not found; nothing needed removal."
else
    echo "Global Menu KDE removed. Log out and back in to refresh Plasma completely."
fi
