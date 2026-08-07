#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build"}
CONFIG_HOME=${XDG_CONFIG_HOME:-"$HOME/.config"}
ENV_FILE="$CONFIG_HOME/plasma-workspace/env/global-menu-kde.sh"
MANIFEST="$BUILD_DIR/install_manifest.txt"

if [[ ! -f "$MANIFEST" ]]; then
    echo "No install manifest found at $MANIFEST" >&2
    echo "Rebuild the project or remove org.chathuranga.globalmenu manually from your user prefix." >&2
    exit 1
fi

PLUGIN_FILE=$(grep -E '/plasma/applets/org\.chathuranga\.globalmenu\.(so|dylib|dll)$' "$MANIFEST" | head -n 1 || true)
PLUGIN_ROOT=""
if [[ -n "$PLUGIN_FILE" ]]; then
    PLUGIN_ROOT=$(dirname -- "$(dirname -- "$(dirname -- "$PLUGIN_FILE")")")
fi

while IFS= read -r path; do
    [[ -n "$path" ]] && rm -f -- "$path"
done < "$MANIFEST"
rm -f -- "$ENV_FILE"

if command -v kbuildsycoca6 >/dev/null 2>&1; then
    if [[ -n "$PLUGIN_ROOT" ]]; then
        QT_PLUGIN_PATH="$PLUGIN_ROOT${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}" \
            kbuildsycoca6 --noincremental
    else
        kbuildsycoca6 --noincremental
    fi
fi

cat <<MSG
Uninstalled Global Menu KDE.
Removed session environment hook: $ENV_FILE
Log out and log back in to remove the plugin path from the Plasma session.
MSG
