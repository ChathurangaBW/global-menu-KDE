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

This project does **not** implement a second panel or a custom global-menu shell. It is based directly on Plasma Workspace's native `applets/appmenu` architecture:

- KDE's `AppMenuApplet` popup/controller behavior;
- KDE's `AppMenuModel` active-window behavior;
- KDE's native `org.kde.kappmenuview` lifecycle;
- KDE's native panel sizing, compact/full modes, hover switching, keyboard handling, RTL behavior, and configuration UI;
- KDE's private `dbusmenuqt` importer, vendored with its original licensing because Plasma Workspace builds it as a private static target.

The project-specific change is isolated to the **no-application-menu state**: instead of hiding, the model supplies a local desktop `QMenu` with seven headings.

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

## Production packages

Production releases publish native packages built and smoke-tested on the target packaging family:

| Package | Target | CPU architectures |
| --- | --- | --- |
| `.deb` | Debian testing / compatible Plasma 6 Debian-family systems | `amd64`, `arm64` |
| `.rpm` | Fedora 44 / compatible Plasma 6 RPM-family systems | `x86_64`, `aarch64` |
| `.pkg.tar.zst` | Official Arch Linux | `x86_64` |
| `.tar.gz` | Source archive | architecture-independent source |

Every release also contains `SHA256SUMS`. Official Arch Linux itself targets x86-64, so the Arch package is intentionally x86-64; ARM64 is covered by the Debian and Fedora package builds.

### Install a release package

Debian/Ubuntu-family Plasma 6:

```bash
sudo apt install ./global-menu-kde_1.0.0_amd64.deb
```

Fedora/RPM-family Plasma 6:

```bash
sudo dnf install ./global-menu-kde-1.0.0-1.x86_64.rpm
```

Arch Linux:

```bash
sudo pacman -U ./global-menu-kde-1.0.0-1-x86_64.pkg.tar.zst
```

After the first installation, log out and back in once so the running Plasma session fully rescans binary applets. Then add **Global Menu KDE** directly to your existing Plasma panel. Remove KDE's stock **Global Menu** widget if you do not want both applets present.

## Build from source

For KDE neon / Ubuntu-family Plasma 6, install the build dependencies first:

```bash
sudo apt update
sudo apt install \
  git build-essential cmake ninja-build extra-cmake-modules \
  qt6-base-dev qt6-declarative-dev \
  libkf6config-dev libkf6i18n-dev libkf6windowsystem-dev \
  libplasma-dev plasma-workspace-dev
```

Then:

```bash
git clone https://github.com/ChathurangaBW/global-menu-KDE.git
cd global-menu-KDE
bash ./scripts/install-system.sh
```

The installer uses `/usr`, the normal Qt/KDE plugin prefix used by native Plasma binary applets. It does **not** create a `QT_PLUGIN_PATH` session workaround.

Manual build:

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

## Final production QA gate

A production release is created **only after** the dedicated release workflow passes every gate below:

- repository integrity and `git diff --check`;
- JSON metadata validation and strict project/metadata version synchronization;
- ShellCheck and Bash syntax validation for system install/uninstall scripts;
- SPDX license-header audit across source, tests, QML, vendored code, and scripts;
- `qmllint` against the Plasma 6 QML environment;
- complete Release build and CTest suite;
- AddressSanitizer + UndefinedBehaviorSanitizer build and tests;
- repeated private-session D-Bus stress testing of **fallback → application menu → fallback**;
- real dbusmenu direct-action and submenu activation through the test fixture;
- `.deb` builds on native `amd64` and `arm64` runners;
- `.rpm` builds on native `x86_64` and `aarch64` runners;
- native Arch package creation with `makepkg`;
- package metadata/lint inspection (`lintian`, `rpmlint`, `namcap`);
- installation of every generated package into its target distribution environment;
- ELF dependency audit with `ldd` and rejection of unresolved libraries;
- Plasma `plasmawindowed` smoke load after package installation;
- native plugin discovery with **`QT_PLUGIN_PATH` unset**;
- uninstall smoke test confirming the package and plugin are actually removed;
- source archive generation and final SHA-256 checksums.

The GitHub Release publish job depends on all of those jobs, so a failed architecture, package, sanitizer, test, install, discovery, or uninstall check blocks the release.

## Uninstall source installation

```bash
bash ./scripts/uninstall-system.sh
```

Then log out and back in to refresh Plasma completely.

## Repository layout

```text
.
├── src/
│   ├── appmenuapplet.*       KDE native Global Menu controller
│   ├── appmenumodel.*        KDE native model + fallback selection
│   ├── desktopfallback.*     project-specific desktop fallback
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
