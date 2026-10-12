# Installation

KDE Global Menu is a native Plasma 6 binary applet. Validated releases provide native packages for Debian/Ubuntu (`.deb`), Fedora/RPM (`.rpm`), and Arch (`.pkg.tar.zst`/`PKGBUILD`), plus a source archive and portable source installer. Native packages must match the target distribution family and architecture.

## Recommended install

Plasma **6.5 or newer** is required at runtime and for development. The applet uses `plasma_add_applet` with `QML_ARGS`, so Plasma 6.0 through 6.4 are not supported.

Run the installer as your normal desktop user, not with `sudo`:

```bash
curl -fsSL https://raw.githubusercontent.com/ChathurangaBW/global-menu-KDE/main/install.sh | bash
```

Or clone first so you can inspect the code before executing it:

```bash
git clone https://github.com/ChathurangaBW/global-menu-KDE.git
cd global-menu-KDE
bash ./install.sh
```

The installer:

1. verifies `plasmashell --version` reports Plasma 6.5 or newer before changing dependencies (also with `--no-deps`);
2. detects `apt`, `dnf`, `pacman`, or `zypper` and installs the required development dependencies;
3. configures a Release build with tests enabled;
4. builds and runs CTest as the desktop user;
5. installs the plugin system-wide under `/usr` using `sudo` or `doas`;
6. checks the installed plugin for unresolved shared-library dependencies;
7. removes this project's obsolete user-local plugin / environment-hook leftovers;
8. stores the exact CMake install manifest under `/var/lib/global-menu-kde`;
9. installs `global-menu-kde-uninstall` for deterministic removal.

The installer never creates or exports a `QT_PLUGIN_PATH` session override.

### Read-only preflight

```bash
bash ./install.sh --check
```

This checks the runtime version and the presence of `git`, `cmake`, `ninja`, `c++`, `ctest`, `msgfmt`, and `ldd`. Missing, unreadable, or unrecognized runtime versions fail safely. It does not invoke privilege tools, install packages, enable repositories, fetch sources, configure, or build. Unlike installation, this check can run as root, but it cannot be combined with `--uninstall`. It checks tool presence only, not tool versions or development-package compatibility. A successful check is not a guarantee that CMake configuration or the build will succeed.

### Enterprise Linux 10 repositories

On EL10, the DNF path prints a diagnostic about EPEL 10 and CRB (CodeReady Builder), which may be needed for Plasma/KF6 development packages. An administrator must verify repository availability and suitability for the specific distribution. The installer never automatically enables repositories or installs repository-release packages. Missing packages can still cause dependency installation to fail, and enabling repositories does not make a Plasma runtime older than 6.5 supported.

After the first binary-plugin installation, log out of Plasma and back in once. Then open **Edit Mode → Add Widgets**, search for **KDE Global Menu**, and drag it directly into the existing panel.

## Other distributions

If your distribution is not handled by the automatic dependency mapper, install these requirements using the distribution's normal development packages:

- C++20 compiler and standard build tools;
- CMake 3.22+ and Ninja;
- GNU gettext (`msgfmt`) for translation catalogs, including the translation test;
- Extra CMake Modules / ECM;
- Qt 6.6+ Core, DBus, Gui, Quick, Widgets, and Test development files;
- KF6 Config, I18n, and WindowSystem development files;
- Plasma 6.5+ development files;
- Plasma Workspace development files providing `LibTaskManager`.

Then run:

```bash
bash ./install.sh --no-deps
```

The source installer is architecture-neutral; the resulting binary plugin is native to the host architecture and local Plasma/Qt ABI.

All four automatic dependency mappings explicitly install `gettext`. Installer preflight regressions can be run with `bash tests/installertest.sh`. These are **synthetic tests with mocked commands**, not validation of package availability or installations on those distributions.

## Upgrade

From an existing clone:

```bash
cd global-menu-KDE
git pull --ff-only
bash ./install.sh
```

The new installation replaces the previous plugin and refreshes the persistent uninstall manifest.

## Uninstall

After any installation made by the CLI installer:

```bash
global-menu-kde-uninstall
```

From a source checkout you can also use:

```bash
bash ./install.sh --uninstall
```

Log out and back in once so Plasma fully unloads the binary plugin.

## Manual build

For development only:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For system installation, use `bash ./install.sh` rather than a raw `sudo cmake --install` so dependency auditing and deterministic uninstall metadata are also installed.

## Legacy user-local builds

Older development versions of this project used a user-local plugin and a Plasma session environment hook. The source installer removes only this project's matching legacy files when detected. The current native implementation relies on KDE's normal system plugin discovery under `/usr`.
