# Global Menu KDE

[![CI](https://github.com/ChathurangaBW/global-menu-KDE/actions/workflows/ci.yml/badge.svg)](https://github.com/ChathurangaBW/global-menu-KDE/actions/workflows/ci.yml)
![Plasma 6](https://img.shields.io/badge/KDE%20Plasma-6-1d99f3)
![Qt 6](https://img.shields.io/badge/Qt-6-41cd52)

**KDE Plasma's native Global Menu applet, with one missing desktop state added.**

When no application provides a global menu, the applet shows:

```text
File   Edit   View   Go   Tools   Settings   Help
```

When Dolphin, Kate, KWrite, or another compatible application becomes active, KDE's normal application menu takes over. When that menu disappears, the desktop fallback returns.

## Design rule

This repository intentionally does **not** implement a second panel or a custom global-menu shell.

The rewrite is based directly on Plasma Workspace's native `applets/appmenu` architecture:

- KDE's `AppMenuApplet` popup/controller behavior;
- KDE's `AppMenuModel` active-window behavior;
- KDE's native `org.kde.kappmenuview` lifecycle;
- KDE's native panel sizing, compact/full modes, hover switching, keyboard handling, RTL behavior, and configuration UI;
- KDE's `dbusmenuqt` importer, vendored because Plasma Workspace builds it as a private static target.

The project-specific change is isolated to the **no-application-menu state**: instead of hiding, the model supplies a local desktop `QMenu` with seven headings.

```text
                    no exported menu
                           │
                           ▼
 File  Edit  View  Go  Tools  Settings  Help
                           │
              application exports dbusmenu
                           │
                           ▼
                 KDE AppMenuModel/importer
                           │
                           ▼
              application's real menu bar
```

## Desktop fallback

| Menu | Desktop actions |
| --- | --- |
| **File** | Home Folder, Documents, Downloads, Trash |
| **Edit** | Clipboard History |
| **View** | Show Desktop, Restore Windows |
| **Go** | Home, Documents, Downloads, Trash |
| **Tools** | Run Command, Konsole, System Monitor |
| **Settings** | System Settings |
| **Help** | KDE Help Center |

These fallback actions are present only while no usable application menu is exported.

## Installation

### KDE neon / Ubuntu-family Plasma 6

Install the build dependencies first:

```bash
sudo apt update
sudo apt install \
  git build-essential cmake ninja-build extra-cmake-modules \
  qt6-base-dev qt6-declarative-dev \
  libkf6config-dev libkf6i18n-dev libkf6windowsystem-dev \
  libplasma-dev plasma-workspace-dev
```

Then build, test, and install:

```bash
git clone https://github.com/ChathurangaBW/global-menu-KDE.git
cd global-menu-KDE
bash ./scripts/install-system.sh
```

The installer deliberately uses `/usr`, the normal Qt/KDE plugin prefix used by native Plasma binary applets. It does **not** create a `QT_PLUGIN_PATH` session workaround.

It also removes only legacy files created by older development builds of this project:

- `~/.config/plasma-workspace/env/global-menu-kde.sh`
- user-local `org.chathuranga.globalmenu.so` copies below `~/.local`

After the first installation, **log out and log back in once** so the running Plasma session fully rescans binary applets. Then:

1. Right-click the existing Plasma panel and choose **Enter Edit Mode**.
2. Choose **Add Widgets**.
3. Find **Global Menu KDE**.
4. Drag it **directly into the panel**, just like KDE's native Global Menu widget.
5. Remove KDE's stock **Global Menu** widget if you do not want both applets present.

### Manual build

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
sudo cmake --install build
kbuildsycoca6 --noincremental
```

## Uninstall

```bash
bash ./scripts/uninstall-system.sh
```

Then log out and back in to refresh Plasma completely.

## QA contract

The rewrite is not considered releasable unless all of these pass:

- clean Plasma 6 Release build;
- fallback unit test: exactly `File Edit View Go Tools Settings Help`;
- private-session D-Bus integration test: **fallback → real application menu → fallback**;
- real dbusmenu direct-action and submenu activation in the fixture;
- staged plugin dependency audit;
- staged `plasmawindowed` load under Xvfb/private D-Bus;
- native `/usr` installation;
- `plasmawindowed` discovery/loading again with **`QT_PLUGIN_PATH` unset**.

The last two checks specifically protect against the older reboot/discovery problem.

## Repository layout

```text
.
├── src/
│   ├── appmenuapplet.*       KDE native Global Menu controller
│   ├── appmenumodel.*        KDE native model + fallback selection
│   ├── desktopfallback.*     the only product-specific menu feature
│   ├── main.xml              native Global Menu configuration
│   └── qml/                  KDE native Global Menu presentation
├── third_party/
│   └── libdbusmenuqt/        KDE/Canonical private dbusmenu importer
├── tests/
│   ├── desktopfallbacktest.cpp
│   └── appmenumodeltest.cpp
├── scripts/
│   ├── install-system.sh
│   └── uninstall-system.sh
└── README.md
```

## Upstream and licensing

The native applet portions are derived from KDE Plasma Workspace's Global Menu applet and retain KDE's original SPDX copyright/license declarations. The vendored `libdbusmenuqt` files retain their Canonical/KDE `LGPL-2.0-or-later` declarations. Project-specific fallback code uses `GPL-2.0-or-later`.

See `LICENSE` and the SPDX header in each source file for the authoritative license of that file.
