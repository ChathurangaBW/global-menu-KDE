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
- Native `QMenu` submenus, disabled actions, check items, exclusive radio groups, icons, and exported shortcuts.
- `AboutToShow`, `opened`, `closed`, and `clicked` dbusmenu lifecycle handling.
- Automatic reacquisition of `org.kde.kappmenuview` if KDE's stock Global Menu widget is removed.
- Complete hidden-state behavior when no exported menu is available.
- Static QA, Qt unit tests, and Plasma 6 compile/test workflow configuration.

## Target environment

- KDE Plasma 6
- Qt 6.6 or newer
- KDE Frameworks 6
- Wayland or X11

The build requires development packages for Qt Core/DBus/Gui/Quick/Widgets/Test, ECM, KF6 CoreAddons/I18n/WindowSystem, LibPlasma, and Plasma Workspace's `LibTaskManager`.

## Build and test

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Install for the current user

```bash
bash ./scripts/install-user.sh
```

The installer:

1. Configures and builds the applet.
2. Runs the Qt test suite.
3. Installs under `~/.local` by default.
4. Finds the installed Qt plugin root from CMake's install manifest.
5. Creates `~/.config/plasma-workspace/env/global-menu-kde.sh` so Plasma can discover the compiled applet plugin.

After installation, **log out and log back in**. Then add **Global Menu KDE** to a panel and remove KDE's stock Global Menu widget if it is present.

## Uninstall

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
