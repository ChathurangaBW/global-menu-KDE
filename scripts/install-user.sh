#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build"}
PREFIX=${PREFIX:-"$HOME/.local"}
CONFIG_HOME=${XDG_CONFIG_HOME:-"$HOME/.config"}
ENV_DIR="$CONFIG_HOME/plasma-workspace/env"
ENV_FILE="$ENV_DIR/global-menu-kde.sh"
RUN_INTEGRATION_TESTS=${RUN_INTEGRATION_TESTS:-0}

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DBUILD_TESTING=ON
cmake --build "$BUILD_DIR" --parallel

# Keep installation validation deterministic on a live Plasma session. The
# dbusmenumodel integration test creates a private session bus and LibTaskManager
# model; on some development/unstable Plasma builds this can take a long time to
# tear down even after the assertions have completed. CI and scripts/qa.sh still
# run the integration test as a required test with its own hard timeout.
ctest --test-dir "$BUILD_DIR" --output-on-failure -LE integration

if [[ "$RUN_INTEGRATION_TESTS" == "1" ]]; then
    echo "Running optional dbusmenu integration test (30 second hard timeout)..."
    ctest --test-dir "$BUILD_DIR" --output-on-failure -L integration --timeout 30
else
    echo "Skipping live-install dbusmenu integration test; CI/full QA covers it."
    echo "Set RUN_INTEGRATION_TESTS=1 to run it during installation."
fi

cmake --install "$BUILD_DIR"

MANIFEST="$BUILD_DIR/install_manifest.txt"
if [[ ! -f "$MANIFEST" ]]; then
    echo "CMake did not produce an install manifest at $MANIFEST" >&2
    exit 1
fi

PLUGIN_FILE=$(grep -E '/plasma/applets/org\.chathuranga\.globalmenu\.(so|dylib|dll)$' "$MANIFEST" | head -n 1 || true)
if [[ -z "$PLUGIN_FILE" ]]; then
    echo "Could not locate the installed Plasma applet plugin in $MANIFEST" >&2
    exit 1
fi

# The plugin is installed as <plugin-root>/plasma/applets/<plugin>. Plasma's
# plugin loader must see <plugin-root> in Qt's library path at session startup.
PLUGIN_ROOT=$(dirname -- "$(dirname -- "$(dirname -- "$PLUGIN_FILE")")")
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
Installed Global Menu KDE for the current user.

Plugin: $PLUGIN_FILE
Session environment: $ENV_FILE

Required next steps:
  1. Log out and log back in so Plasma loads the updated QT_PLUGIN_PATH.
  2. Add "Global Menu KDE" to a panel.
  3. Remove KDE's stock Global Menu widget if it is present.

The applet stays hidden until the active application exports a menu.
MSG
