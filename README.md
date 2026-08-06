# Global Menu KDE

A focused Plasma 6 panel applet that shows only the active application's exported menu bar.

## Scope

- Display exported application headings such as **File**, **Edit**, **View**, **Go**, **Tools**, and **Help**.
- Use KDE Plasma's existing application-menu registrar and active-window metadata.
- Hide the applet completely when the active application does not export a menu.
- Do not add an Apple/system menu, search, workspace controls, clock, persistent placeholder, or synthetic fallback menu.
- Match a compact native menubar treatment suitable for a Plasma panel.

## Implemented

- Plasma 6 C++/QML applet.
- Active-window tracking through `LibTaskManager`.
- Canonical dbusmenu layout import and action activation.
- Native `QMenu` submenus, disabled actions, check items, exclusive radio groups, icons, exported shortcuts, and mnemonics.
- `AboutToShow`, `opened`, `closed`, and `clicked` dbusmenu lifecycle handling.
- Automatic reacquisition of `org.kde.kappmenuview` if KDE's stock Global Menu widget is removed.
- Complete hidden-state behavior when no exported menu is available.
- Static QA, Qt unit tests, Plasma 6 release compilation, staged installation, and prebuilt artifact generation in CI.

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

## QA

```bash
bash ./scripts/qa.sh
```

The applet should occupy no panel space on the desktop or while focusing an application without an exported menu. It should appear when an exporting application such as Dolphin or Kate is active.

Interactive Plasma session checks are tracked in [`TODO.md`](TODO.md).

## License

GPL-2.0-or-later. The small dbusmenu serialization/shortcut component is LGPL-2.0-or-later; individual files carry SPDX identifiers.
