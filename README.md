# Global Menu KDE

A focused Plasma 6 panel applet that shows only the active application's exported menu bar.

## Scope

This is **not** a full macOS-style desktop bar replacement. It implements only the application-menu group that KDE is missing from the requested panel setup.

- Display exported application headings such as **File**, **Edit**, **View**, **Go**, **Tools**, and **Help**.
- Use KDE Plasma's existing application-menu registrar and active-window metadata.
- Appear only while the active application exports a menu, for example Dolphin or Kate.
- Collapse to zero layout size when the desktop or an application without an exported menu is active.
- Do not add an Apple/system menu, application launcher, search, workspace controls, media/status controls, clock, persistent placeholder, or synthetic fallback menu.
- Keep a compact desktop-menubar treatment with KDE-native hover/pressed states and denser horizontal spacing.

## Implemented

- Plasma 6 C++/QML applet.
- Active-window tracking through `LibTaskManager`.
- Canonical dbusmenu layout import and action activation.
- Native `QMenu` submenus, disabled actions, check items, exclusive radio groups, icons, exported shortcuts, and mnemonics.
- Incremental `ItemsPropertiesUpdated` propagation for labels, visibility, enabled state, icons, shortcuts, and toggle state; structural changes fall back to `GetLayout`.
- `AboutToShow`, `opened`, `closed`, and `clicked` dbusmenu lifecycle handling.
- Reference-counted, undo-aware `org.kde.kappmenuview` lifecycle compatible with KDE's stock Global Menu widget.
- Explicit zero-size root and representation layout hints when no exported menu is available.
- Static QA guards that reject out-of-scope Apple/system/search/workspace/status UI from the QML surface.
- Static QA, Qt unit tests, fake-exporter integration tests, Plasma 6 release compilation, staged installation, headless applet loading, and prebuilt artifact generation in CI.

## Automated validation

The CI pipeline validates:

- source structure, application-menu-only scope, protocol behavior, lifecycle safeguards, and packaging assertions;
- ShellCheck for all repository scripts;
- Qt shortcut-token translation tests;
- a fake `com.canonical.dbusmenu` exporter in a private D-Bus session;
- layout import, submenu and direct actions, disabled state, incremental property mutation without unnecessary `GetLayout` calls, top-level hidden-state changes, and structural-refresh fallback;
- `AboutToShow`, `opened`, `closed`, and `clicked` forwarding;
- Plasma 6 release compilation and linking;
- staged CMake installation;
- staged-plugin loading through `plasmawindowed` under Xvfb and a private D-Bus session;
- installable artifact creation and upload.

Real-panel interaction and visual checks remain listed in [`TODO.md`](TODO.md).

## Target environment

- KDE Plasma 6
- Qt 6.6 or newer
- KDE Frameworks 6
- Wayland or X11

The source build requires development packages for Qt Core/DBus/Gui/Quick/Widgets/Test, ECM, KF6 Config/CoreAddons/I18n/WindowSystem, LibPlasma, and Plasma Workspace's `LibTaskManager`.

## Prebuilt CI artifact

Each successful CI run publishes `global-menu-kde-plasma6`, containing an Arch Linux x86-64 build of the applet plus installation helpers.

1. Download and extract the `global-menu-kde-plasma6` artifact from the latest successful GitHub Actions run.
2. Extract `global-menu-kde-plasma6.tar.gz`.
3. Run:

```bash
bash ./install.sh
```

To remove that installation, run `bash ./uninstall.sh` from the extracted directory. The prebuilt archive is intended for current Plasma 6 systems with compatible Qt, KDE Frameworks, Plasma, and `LibTaskManager` shared-library versions.

## Build and test from source

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Run the repository QA, including a local applet-load smoke test when Plasma tooling is available:

```bash
bash ./scripts/qa.sh
```

## Install from source for the current user

```bash
bash ./scripts/install-user.sh
```

The source installer:

1. Configures and builds the applet.
2. Runs the Qt test suite.
3. Installs under `~/.local` by default.
4. Finds the installed Qt plugin root from CMake's install manifest.
5. Creates `~/.config/plasma-workspace/env/global-menu-kde.sh` so Plasma can discover the compiled applet plugin.

After either installation method, **log out and log back in**. Then add **Global Menu KDE** to a panel and remove KDE's stock Global Menu widget if it is present.

## Uninstall a source build

```bash
bash ./scripts/uninstall-user.sh
```

Log out and back in after uninstalling so the session plugin path is removed.

## Expected behavior

When Dolphin is active, the applet should show only Dolphin's exported headings such as **File**, **Edit**, **View**, and the rest of its menu bar. When the desktop or an application without a dbusmenu export is active, the applet should occupy no panel space.

## License

GPL-2.0-or-later. The small dbusmenu serialization/shortcut component is LGPL-2.0-or-later; individual files carry SPDX identifiers.
