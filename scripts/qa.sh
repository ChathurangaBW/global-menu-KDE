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
    "src/globalmenuproperty.cpp",
    "src/viewservicelease.h",
    "src/viewservicelease.cpp",
    "src/qml/main.qml",
    "src/qml/MenuDelegate.qml",
    "tests/CMakeLists.txt",
    "tests/shortcuttest.cpp",
    "tests/modeltest.cpp",
    "scripts/install-user.sh",
    "scripts/uninstall-user.sh",
    "scripts/install-prebuilt.sh",
    "scripts/uninstall-prebuilt.sh",
    "scripts/smoke-plasmawindowed.sh",
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
property_cpp = (root / "src/globalmenuproperty.cpp").read_text(encoding="utf-8")
lease_header = (root / "src/viewservicelease.h").read_text(encoding="utf-8")
lease_cpp = (root / "src/viewservicelease.cpp").read_text(encoding="utf-8")
dbus_types_cpp = (root / "src/dbusmenutypes.cpp").read_text(encoding="utf-8")
src_cmake = (root / "src/CMakeLists.txt").read_text(encoding="utf-8")
test_cmake = (root / "tests/CMakeLists.txt").read_text(encoding="utf-8")
model_test = (root / "tests/modeltest.cpp").read_text(encoding="utf-8")
workflow = (root / ".github/workflows/ci.yml").read_text(encoding="utf-8")
installer = (root / "scripts/install-user.sh").read_text(encoding="utf-8")
uninstaller = (root / "scripts/uninstall-user.sh").read_text(encoding="utf-8")
prebuilt_installer = (root / "scripts/install-prebuilt.sh").read_text(encoding="utf-8")
smoke_script = (root / "scripts/smoke-plasmawindowed.sh").read_text(encoding="utf-8")

assert "HiddenStatus" in main_qml
assert "hasApplicationMenu" in main_qml
assert "implicitWidth: root.hasApplicationMenu ? buttonGrid.implicitWidth : 0" in main_qml
assert "implicitHeight: root.hasApplicationMenu ? buttonGrid.implicitHeight : 0" in main_qml
assert "Layout.maximumWidth: root.implicitWidth" in main_qml
assert "Layout.maximumHeight: root.implicitHeight" in main_qml
assert "noMenuPlaceholder" not in main_qml
assert "activeAction" in main_qml
assert "activeAction?.text" in main_qml
assert "KeyboardIndicator.KeyState" in main_qml
assert "required property PlasmaCore.Action action" not in main_qml
assert "MnemonicData.richTextLabel" in delegate_qml
assert "MnemonicData.controlType" in delegate_qml
assert "property bool vertical" in delegate_qml
assert "Kirigami.Units.smallSpacing * 2" in delegate_qml

# This applet is intentionally application-menu-only. System controls shown in
# early visual concepts must never become part of the implementation.
qml_surface = main_qml + "\n" + delegate_qml
for forbidden in (
    "AppleMenu",
    "SystemStatus",
    "KRunner",
    "MPRIS",
    "WorkspaceIndicator",
    "ClockArea",
    "Recent Items",
    "System Settings",
    "Now Playing",
):
    assert forbidden not in qml_surface, f"out-of-scope UI found: {forbidden}"

assert "sourceActionForIndex" in applet_cpp
assert "m_model->menuClosed(m_currentIndex);\n        setCurrentIndex(-1);" in applet_cpp
assert "ViewServiceLease" in lease_header
assert "activeLeaseCount" in lease_cpp
assert "destroyedChanged" in lease_cpp
assert "unregisterService(viewService())" in lease_cpp
assert "org.kde.plasma.appmenu" in lease_cpp
assert "blockSignals" in lease_cpp
assert "viewservicelease.cpp" in src_cmake
assert "globalmenuproperty.cpp" in src_cmake

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
assert "GlobalMenuModel::property" in property_cpp
assert "applyActionProperties" in model_cpp
assert "m_actionsById" in model_header
assert "m_itemProperties" in model_header

assert "DBusMenuShortcut::toKeySequence" in dbus_types_cpp
assert "Control" in dbus_types_cpp and "Super" in dbus_types_cpp
assert "dbusmenumodel_test" in test_cmake
assert "dbus-run-session" in test_cmake
assert "QDBusVirtualObject" in model_test
assert "GetLayout" in model_test and "AboutToShow" in model_test
assert 'QStringLiteral("opened")' in model_test
assert 'QStringLiteral("closed")' in model_test
assert 'QStringLiteral("clicked")' in model_test
assert "ItemsPropertiesUpdated" in model_test

assert "ctest --test-dir build --output-on-failure" in workflow
assert "run: bash ./scripts/qa.sh --static" in workflow
assert "scripts/smoke-plasmawindowed.sh" in workflow
assert "actions/upload-artifact@v4" in workflow
assert "cancel-in-progress: true" in workflow

assert "plasma-workspace/env" in installer
assert "QT_PLUGIN_PATH" in installer
assert "ctest --test-dir" in installer
assert "install_manifest.txt" in installer
assert "global-menu-kde.sh" in installer
assert "global-menu-kde.sh" in uninstaller
assert "org.chathuranga.globalmenu.so" in prebuilt_installer
assert "plasma-workspace/env" in prebuilt_installer

assert "--smoke-test" in smoke_script
assert "xvfb-run" in smoke_script
assert "dbus-run-session" in smoke_script
assert "Could not find requested component" in smoke_script
assert "status" in smoke_script and "124" in smoke_script

print("metadata, application-menu-only scope, protocol, lifecycle, integration, packaging, and applet-load checks passed")
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
    && { [[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]] || command -v xvfb-run >/dev/null 2>&1; }; then
    QT_PLUGIN_PATH="$BUILD_DIR/bin${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}" \
        bash "$ROOT_DIR/scripts/smoke-plasmawindowed.sh" org.chathuranga.globalmenu
else
    echo "Plasma applet smoke test skipped: plasmawindowed/display support unavailable"
fi
