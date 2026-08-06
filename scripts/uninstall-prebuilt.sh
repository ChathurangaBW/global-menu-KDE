#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

PREFIX=${PREFIX:-"$HOME/.local"}
CONFIG_HOME=${XDG_CONFIG_HOME:-"$HOME/.config"}
PLUGIN_ROOT="$PREFIX/lib/qt6/plugins"
PLUGIN_FILE="$PLUGIN_ROOT/plasma/applets/org.chathuranga.globalmenu.so"
ENV_FILE="$CONFIG_HOME/plasma-workspace/env/global-menu-kde.sh"

rm -f -- "$PLUGIN_FILE" "$ENV_FILE"

if command -v kbuildsycoca6 >/dev/null 2>&1; then
    QT_PLUGIN_PATH="$PLUGIN_ROOT${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}" \
        kbuildsycoca6 --noincremental
fi

cat <<MSG
Uninstalled Global Menu KDE.
Removed plugin: $PLUGIN_FILE
Removed session environment: $ENV_FILE

Log out and log back in to remove the plugin path from the Plasma session.
MSG
