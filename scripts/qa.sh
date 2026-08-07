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
metadata = json.loads((root / "src/metadata.json").read_text(encoding="utf-8"))
plugin = metadata["KPlugin"]
assert plugin["Id"] == "org.chathuranga.globalmenu"
assert metadata["X-Plasma-API-Minimum-Version"] == "6.0"

required = [
    "CMakeLists.txt",
    "README.md",
    "TODO.md",
    "docs/global-menu-preview.svg",
    "docs/INSTALL.md",
    "docs/QA.md",
    "docs/ARCHITECTURE.md",
    "src/CMakeLists.txt",
    "src/globalmenuapplet.cpp",
    "src/globalmenumodel.cpp",
    "src/globalmenuproperty.cpp",
    "src/displaymenumodel.h",
    "src/displaymenumodel.cpp",
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

readme = (root / "README.md").read_text(encoding="utf-8")
preview = (root / "docs/global-menu-preview.svg").read_text(encoding="utf-8")
install_doc = (root / "docs/INSTALL.md").read_text(encoding="utf-8")
qa_doc = (root / "docs/QA.md").read_text(encoding="utf-8")
architecture_doc = (root / "docs/ARCHITECTURE.md").read_text(encoding="utf-8")
main_qml = (root / "src/qml/main.qml").read_text(encoding="utf-8")
delegate_qml = (root / "src/qml/MenuDelegate.qml").read_text(encoding="utf-8")
applet_cpp = (root / "src/globalmenuapplet.cpp").read_text(encoding="utf-8")
display_cpp = (root / "src/displaymenumodel.cpp").read_text(encoding="utf-8")
display_header = (root / "src/displaymenumodel.h").read_text(encoding="utf-8")
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

# Product contract: existing-panel applet, persistent desktop fallback, app takeover.
assert "desktop fallback" in readme.lower()
assert "File   Edit   View   Go   Tools   Settings   Help" in readme
assert "existing Plasma panel" in readme
assert "real exported menu" in readme
assert "docs/global-menu-preview.svg" in readme
assert "docs/INSTALL.md" in readme
assert "docs/QA.md" in readme
assert "docs/ARCHITECTURE.md" in readme
assert "global-menu-kde-plasma6" in readme

assert "Desktop fallback" in preview
for heading in ("File", "Edit", "View", "Go", "Tools", "Settings", "Help"):
    assert f">{heading}<" in preview, f"preview missing heading: {heading}"
assert "Apple" not in preview
assert "second panel" not in preview.lower()

assert "desktop fallback" in install_doc.lower()
assert "existing Plasma panel" in install_doc
assert "RUN_INTEGRATION_TESTS" in install_doc
assert "bash ./scripts/install-user.sh" in install_doc
assert "bash ./scripts/uninstall-user.sh" in install_doc
assert "QT_PLUGIN_PATH" in install_doc
assert "desktop fallback" in qa_doc.lower()
assert "application takeover" in qa_doc.lower()
assert "Wayland" in qa_doc and "X11" in qa_doc
assert "DisplayMenuModel" in architecture_doc
assert "GlobalMenuModel" in architecture_doc
assert "com.canonical.AppMenu.Registrar" in architecture_doc

# QML must be a compact surface inside the existing containment, not its own panel.
assert "Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground" in main_qml
assert "Plasmoid.CanFillArea" not in main_qml
assert "HiddenStatus" not in main_qml
assert "hasApplicationMenu" not in main_qml
assert "Layout.fillWidth: false" in main_qml
assert "implicitWidth: buttonGrid.implicitWidth" in main_qml
assert "LayoutMirroring.enabled: Application.layoutDirection === Qt.RightToLeft" in main_qml
assert "function onActivated(): void" in main_qml
assert "activeAction" in main_qml
assert "activeAction?.text" in main_qml
assert "KeyboardIndicator.KeyState" in main_qml
assert "MnemonicData.richTextLabel" in delegate_qml
assert "MnemonicData.controlType" in delegate_qml

# No unrelated desktop-bar widgets are part of this applet.
qml_surface = main_qml + "\n" + delegate_qml
for forbidden in (
    "AppleMenu",
    "SystemStatus",
    "MPRIS",
    "WorkspaceIndicator",
    "ClockArea",
    "Now Playing",
):
    assert forbidden not in qml_surface, f"out-of-scope UI found: {forbidden}"

# Display layer must expose a seven-heading desktop menu and switch to app menu.
for heading in ("&File", "&Edit", "&View", "&Go", "&Tools", "&Settings", "&Help"):
    assert f'i18n("{heading}")' in display_cpp, f"fallback missing {heading}"
assert "usingApplicationMenu" in display_header
assert "m_applicationModel->menuAvailable()" in display_cpp
assert "actionForIndex" in display_header
assert "Clipboard History" in display_cpp
assert "showDesktop" in display_cpp
assert "krunner" in display_cpp
assert "systemsettings" in display_cpp
assert "khelpcenter" in display_cpp
assert "DisplayMenuModel" in applet_cpp
assert "m_applicationModel" in applet_cpp
assert "m_model->aboutToShow(index)" in applet_cpp

# KDE registrar/dbusmenu behavior remains intact for actual applications.
assert "ViewServiceLease" in lease_header
assert "activeLeaseCount" in lease_cpp
assert "destroyedChanged" in lease_cpp
assert "unregisterService(viewService())" in lease_cpp
assert "org.kde.plasma.appmenu" in lease_cpp
assert 'QStringLiteral("opened")' in model_cpp
assert 'QStringLiteral("closed")' in model_cpp
assert 'QStringLiteral("clicked")' in model_cpp
assert "QDBusVariant(QString())" in model_cpp
assert "m_sourceGeneration" in model_header
assert "sourceGeneration != m_sourceGeneration" in model_cpp
assert "GlobalMenuModel::property" in property_cpp
assert "applyActionProperties" in model_cpp
assert "m_actionsById" in model_header
assert "m_itemProperties" in model_header
assert "DBusMenuShortcut::toKeySequence" in dbus_types_cpp

# Build/test contract.
assert "displaymenumodel.cpp" in src_cmake
assert "displaymenumodel.cpp" in test_cmake
assert "KF6::I18n" in test_cmake
assert "LABELS \"integration\"" in test_cmake
assert "TIMEOUT 30" in test_cmake
assert "DisplayMenuModel displayModel" in model_test
assert "desktopHeadings" in model_test
assert "displayModel.usingApplicationMenu()" in model_test
assert "QDBusVirtualObject" in model_test
assert "ItemsPropertiesUpdated" in model_test

assert "run: bash ./scripts/qa.sh --static" in workflow
assert "ctest --test-dir build --output-on-failure" in workflow
assert "scripts/smoke-plasmawindowed.sh" in workflow
assert "actions/upload-artifact@v4" in workflow
assert "cancel-in-progress: true" in workflow

# Live installation must not hang on the integration harness.
assert "RUN_INTEGRATION_TESTS" in installer
assert "-LE integration" in installer
assert "-L integration --timeout 30" in installer
assert "plasma-workspace/env" in installer
assert "QT_PLUGIN_PATH" in installer
assert "install_manifest.txt" in installer
assert "global-menu-kde.sh" in installer
assert "global-menu-kde.sh" in uninstaller
assert "org.chathuranga.globalmenu.so" in prebuilt_installer
assert "plasma-workspace/env" in prebuilt_installer

assert "--smoke-test" in smoke_script
assert "xvfb-run" in smoke_script
assert "dbus-run-session" in smoke_script
assert "status" in smoke_script and "124" in smoke_script

print("metadata, desktop fallback, app takeover, compact panel UI, protocol, tests, packaging, and docs checks passed")
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
