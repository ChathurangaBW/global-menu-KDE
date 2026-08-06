#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
set -euo pipefail

ROOT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
MODE=${1:-full}

python3 - "$ROOT_DIR" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
metadata_path = root / "src" / "metadata.json"
metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
plugin = metadata["KPlugin"]
assert plugin["Id"] == "org.chathuranga.globalmenu"
assert metadata["X-Plasma-API-Minimum-Version"] == "6.0"

required = [
    "CMakeLists.txt",
    "src/CMakeLists.txt",
    "src/globalmenuapplet.cpp",
    "src/globalmenumodel.cpp",
    "src/qml/main.qml",
    "src/qml/MenuDelegate.qml",
    "tests/CMakeLists.txt",
    "tests/shortcuttest.cpp",
]
for relative in required:
    path = root / relative
    assert path.is_file(), f"missing {relative}"
    assert path.read_text(encoding="utf-8").strip(), f"empty {relative}"

main_qml = (root / "src/qml/main.qml").read_text(encoding="utf-8")
delegate_qml = (root / "src/qml/MenuDelegate.qml").read_text(encoding="utf-8")
applet_cpp = (root / "src/globalmenuapplet.cpp").read_text(encoding="utf-8")
model_cpp = (root / "src/globalmenumodel.cpp").read_text(encoding="utf-8")
model_header = (root / "src/globalmenumodel.h").read_text(encoding="utf-8")
dbus_types_cpp = (root / "src/dbusmenutypes.cpp").read_text(encoding="utf-8")
workflow = (root / ".github/workflows/ci.yml").read_text(encoding="utf-8")
installer = (root / "scripts/install-user.sh").read_text(encoding="utf-8")
uninstaller = (root / "scripts/uninstall-user.sh").read_text(encoding="utf-8")

assert "HiddenStatus" in main_qml
assert "noMenuPlaceholder" not in main_qml
assert "AppleMenu" not in main_qml
assert "activeAction" in main_qml
assert "activeAction?.text" in main_qml
assert "KeyboardIndicator.KeyState" in main_qml
assert "required property PlasmaCore.Action action" not in main_qml
assert "MnemonicData.richTextLabel" in delegate_qml
assert "MnemonicData.controlType" in delegate_qml

assert "WatchForUnregistration" in applet_cpp
assert "registerViewService" in applet_cpp
assert "static bool requested" not in applet_cpp
assert "sourceActionForIndex" in applet_cpp
assert "m_model->menuClosed(m_currentIndex);\n        setCurrentIndex(-1);" in applet_cpp

assert 'QStringLiteral("opened")' in model_cpp
assert 'QStringLiteral("closed")' in model_cpp
assert 'QStringLiteral("clicked")' in model_cpp
assert "QDBusVariant(QString())" in model_cpp
assert "<< 0u;" in model_cpp
assert "clearActions();\n\n    m_serviceName = serviceName" in model_cpp
assert 'QStringLiteral("shortcut")' in model_cpp
assert 'QStringLiteral("icon-data")' in model_cpp
assert 'QLatin1String("submenu")' in model_cpp
assert "QActionGroup" in model_cpp
assert "m_sourceGeneration" in model_header
assert "sourceGeneration != m_sourceGeneration" in model_cpp

assert "DBusMenuShortcut::toKeySequence" in dbus_types_cpp
assert "Control" in dbus_types_cpp and "Super" in dbus_types_cpp
assert "ctest --test-dir build --output-on-failure" in workflow
assert "run: bash ./scripts/qa.sh --static" in workflow
assert "cancel-in-progress: true" in workflow

assert "plasma-workspace/env" in installer
assert "QT_PLUGIN_PATH" in installer
assert "ctest --test-dir" in installer
assert "install_manifest.txt" in installer
assert "global-menu-kde.sh" in installer
assert "global-menu-kde.sh" in uninstaller

print("metadata, scope, protocol, lifecycle, menu-fidelity, mnemonic, and installation checks passed")
PY

if command -v shellcheck >/dev/null 2>&1; then
    shellcheck "$ROOT_DIR"/scripts/*.sh
else
    echo "shellcheck not installed; skipped"
fi

if command -v qmllint >/dev/null 2>&1; then
    qmllint "$ROOT_DIR"/src/qml/*.qml
else
    echo "qmllint not installed; skipped"
fi

if [[ "$MODE" == "--static" || "$MODE" == "static" ]]; then
    exit 0
fi

BUILD_DIR=${BUILD_DIR:-"$ROOT_DIR/build"}
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure

if command -v plasmawindowed >/dev/null 2>&1 \
    && [[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]]; then
    cmake --install "$BUILD_DIR" --prefix "$HOME/.local"
    plasmawindowed --smoke-test org.chathuranga.globalmenu
else
    echo "No graphical Plasma session detected; runtime smoke test skipped"
fi
