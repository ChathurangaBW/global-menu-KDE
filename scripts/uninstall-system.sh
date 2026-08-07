#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 ChathurangaBW
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

PLUGIN_ID=org.chathuranga.globalmenu
SYSTEM_MANIFEST=/var/lib/global-menu-kde/install_manifest.txt
SYSTEM_MANIFEST_DIR=/var/lib/global-menu-kde
SYSTEM_UNINSTALLER=/usr/local/bin/global-menu-kde-uninstall
LEGACY_ENV_FILE="${XDG_CONFIG_HOME:-$HOME/.config}/plasma-workspace/env/global-menu-kde.sh"

if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
    ROOT=()
elif command -v sudo >/dev/null 2>&1; then
    ROOT=(sudo)
elif command -v doas >/dev/null 2>&1; then
    ROOT=(doas)
else
    printf 'global-menu-kde: sudo or doas is required for system-wide uninstall\n' >&2
    exit 1
fi

as_root() {
    "${ROOT[@]}" "$@"
}

removed=0

if [[ -s "$SYSTEM_MANIFEST" ]]; then
    mapfile -t installed_paths < "$SYSTEM_MANIFEST"
    for path in "${installed_paths[@]}"; do
        [[ -n "$path" ]] || continue
        case "$path" in
            /usr/*)
                if [[ -f "$path" || -L "$path" ]]; then
                    as_root rm -f -- "$path"
                    printf 'Removed: %s\n' "$path"
                    removed=1
                fi
                ;;
            *)
                printf 'Skipping unexpected manifest path: %s\n' "$path" >&2
                ;;
        esac
    done
else
    # Fallback for installations made before the persistent manifest existed.
    while IFS= read -r plugin; do
        as_root rm -f -- "$plugin"
        printf 'Removed native plugin: %s\n' "$plugin"
        removed=1
    done < <(find /usr -type f -path "*/plasma/applets/$PLUGIN_ID.so" -print 2>/dev/null)
fi

rm -f -- "$LEGACY_ENV_FILE"
if [[ -d "$HOME/.local" ]]; then
    while IFS= read -r legacy_plugin; do
        rm -f -- "$legacy_plugin"
        printf 'Removed legacy user plugin: %s\n' "$legacy_plugin"
        removed=1
    done < <(find "$HOME/.local" -type f -path '*/plasma/applets/org.chathuranga.globalmenu.so' -print 2>/dev/null)
fi

as_root rm -f -- "$SYSTEM_MANIFEST"
as_root rmdir "$SYSTEM_MANIFEST_DIR" 2>/dev/null || true

if command -v kbuildsycoca6 >/dev/null 2>&1; then
    if ! kbuildsycoca6 --noincremental; then
        printf 'Warning: kbuildsycoca6 cache refresh failed; log out/in to refresh Plasma.\n' >&2
    fi
fi

# Remove the installed helper last; deleting the currently running script is
# safe on Linux because the shell has already opened it.
if [[ -f "$SYSTEM_UNINSTALLER" ]]; then
    as_root rm -f -- "$SYSTEM_UNINSTALLER"
fi

if [[ $removed -eq 0 ]]; then
    echo "Global Menu KDE plugin was not found; nothing needed removal."
else
    echo "Global Menu KDE removed. Log out and back in to refresh Plasma completely."
fi
