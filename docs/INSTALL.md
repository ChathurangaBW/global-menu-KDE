# Installation

Global Menu KDE is a native Plasma 6 binary applet. It installs into the normal system Qt/Plasma plugin prefix so Widget Explorer and panel drag/drop work like KDE's built-in Global Menu applet.

## KDE neon / Ubuntu-family Plasma 6

Install build dependencies:

```bash
sudo apt update
sudo apt install \
  git build-essential cmake ninja-build extra-cmake-modules \
  qt6-base-dev qt6-declarative-dev \
  libkf6config-dev libkf6i18n-dev libkf6windowsystem-dev \
  libplasma-dev plasma-workspace-dev
```

Clone and install:

```bash
git clone https://github.com/ChathurangaBW/global-menu-KDE.git
cd global-menu-KDE
bash ./scripts/install-system.sh
```

The installer builds with tests enabled, installs under `/usr`, refreshes KDE's service cache, and does not create a `QT_PLUGIN_PATH` session workaround.

After the first binary-plugin installation, log out of Plasma and back in once. Then open **Edit Mode → Add Widgets**, search for **Global Menu KDE**, and drag it directly into the existing panel.

## Manual build

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

## Upgrade an existing source installation

```bash
cd global-menu-KDE
git pull --ff-only
bash ./scripts/install-system.sh
```

Log out and back in if the running Plasma shell still has the older binary plugin loaded.

## Remove legacy user-local builds

Older development versions of this project used a user-local plugin and a session environment hook. The current native rewrite does not need either. The system installer removes only this project's legacy files when detected.

If you previously installed an old build manually, remove the old applet instance from the desktop/panel before adding the current **Global Menu KDE** widget.

## Uninstall

```bash
bash ./scripts/uninstall-system.sh
```

Then log out and back in once so Plasma fully unloads the binary plugin.

## Release packages

When a GitHub Release contains validated native assets, install the package matching your distribution and architecture rather than rebuilding from source.

- Debian/Ubuntu family: `.deb`
- Fedora/RHEL/openSUSE-style RPM family: `.rpm`
- Arch Linux: `.pkg.tar.zst`
- Other Plasma 6 distributions: source archive / source build

Do not install a package built for a different distribution family merely because the CPU architecture matches; Plasma/Qt/KF6 binary compatibility matters.
