#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
PREFIX=${PREFIX:-"$HOME/.local"}
CONFIG_HOME=${XDG_CONFIG_HOME:-"$HOME/.config"}
ENV_DIR="$CONFIG_HOME/plasma-workspace/env"
ENV_FILE="$ENV_DIR/global-menu-kde.sh"

PLUGIN_SOURCE=$(find "$ROOT_DIR" -type f -path '*/plasma/applets/org.chathuranga.globalmenu.so' -print -quit)
if [[ -z "$PLUGIN_SOURCE" ]]; then
    echo "The archive does not contain org.chathuranga.globalmenu.so" >&2
    exit 1
fi

RELATIVE_PLUGIN_PATH=${PLUGIN_SOURCE#"$ROOT_DIR"/}
PLUGIN_DESTINATION="$PREFIX/$RELATIVE_PLUGIN_PATH"
install -Dm0755 "$PLUGIN_SOURCE" "$PLUGIN_DESTINATION"

PLUGIN_ROOT=$(dirname -- "$(dirname -- "$(dirname -- "$PLUGIN_DESTINATION")")")
mkdir -p "$ENV_DIR"
{
    echo '#!/usr/bin/env bash'
    printf 'plugin_root=%q\n' "$PLUGIN_ROOT"
    cat <<'EOF'
case ":${QT_PLUGIN_PATH:-}:" in
    *:"$plugin_root":*) ;;
    *) export QT_PLUGIN_PATH="$plugin_root${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}" ;;
esac
unset plugin_root
EOF
} > "$ENV_FILE"
chmod 0755 "$ENV_FILE"

if command -v kbuildsycoca6 >/dev/null 2>&1; then
    QT_PLUGIN_PATH="$PLUGIN_ROOT${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}" \
        kbuildsycoca6 --noincremental
fi

cat <<MSG
Installed Global Menu KDE.

Plugin: $PLUGIN_DESTINATION
Session environment: $ENV_FILE

Log out and log back in, then add "Global Menu KDE" to a Plasma panel.
MSG
