# Installation Guide

This project installs a compiled Plasma 6 applet named **Global Menu KDE**. It is not a pure QML plasmoid, so the installation includes a Qt/Plasma plugin and a small Plasma-session environment hook.

## Option 1 — Install a prebuilt CI artifact

The GitHub Actions workflow publishes an artifact named `global-menu-kde-plasma6` after every successful build.

> The prebuilt artifact is compiled on current Arch Linux x86-64. Use it only on systems with compatible Qt 6, KDE Frameworks 6, Plasma 6, and `LibTaskManager` library versions. For other distributions, build from source.

1. Open the repository **Actions** tab.
2. Open the latest successful **CI** run for the branch or release you want.
3. Download the `global-menu-kde-plasma6` artifact.
4. Extract the downloaded ZIP.
5. Extract the packaged tarball:

   ```bash
   tar -xzf global-menu-kde-plasma6.tar.gz
   ```

6. Install for the current user:

   ```bash
   bash ./install.sh
   ```

7. **Log out and log back in.** Plasma reads the plugin search path when the session starts.
8. Edit your Plasma panel and add **Global Menu KDE**.
9. Remove KDE's stock **Global Menu** widget if you do not want two menu consumers in the panel.

### Uninstall a prebuilt artifact

From the extracted artifact directory:

```bash
bash ./uninstall.sh
```

Then log out and back in.

---

## Option 2 — Build and install from source

### Requirements

- KDE Plasma 6
- Qt 6.6 or newer
- KDE Frameworks 6
- CMake
- Extra CMake Modules (ECM)
- C++ build toolchain
- Plasma Workspace development files exposing `LibTaskManager`

The CI build uses current Arch Linux packages:

```text
base-devel
cmake
ninja
extra-cmake-modules
qt6-base
qt6-declarative
kconfig
kcoreaddons
ki18n
kwindowsystem
libplasma
plasma-workspace
```

Package names differ on Fedora, openSUSE, Debian/Ubuntu, Neon, and other distributions. Install the equivalent development packages from your distribution.

### Clone

```bash
git clone https://github.com/ChathurangaBW/global-menu-KDE.git
cd global-menu-KDE
```

### One-command user installation

```bash
bash ./scripts/install-user.sh
```

The installer will:

1. Configure a Release build.
2. Compile the Plasma applet.
3. Run the deterministic package/unit tests used for local installation.
4. Skip the private-D-Bus `dbusmenumodel_test` by default because some live Plasma development/unstable sessions can take a long time to tear that isolated test environment down.
5. Install under `~/.local` by default.
6. Find the installed Qt plugin directory from CMake's install manifest.
7. Create `~/.config/plasma-workspace/env/global-menu-kde.sh` so Plasma can discover the plugin at session startup.
8. Refresh KDE's service cache when `kbuildsycoca6` is available.

The full D-Bus integration test remains mandatory in GitHub CI and `scripts/qa.sh`, where it has a 30-second hard timeout.

To explicitly run it during local installation:

```bash
RUN_INTEGRATION_TESTS=1 bash ./scripts/install-user.sh
```

After installation, **log out and log back in**, then add **Global Menu KDE** to the panel.

### Custom prefix

The source installer supports an alternate prefix:

```bash
PREFIX="$HOME/.local" bash ./scripts/install-user.sh
```

A custom build directory is also supported:

```bash
BUILD_DIR="$PWD/build-release" bash ./scripts/install-user.sh
```

### Manual build

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

For ordinary user installation, prefer `scripts/install-user.sh` because it also creates the Plasma-session Qt plugin-path hook.

---

## Updating

Pull the new source and run the installer again:

```bash
git pull
bash ./scripts/install-user.sh
```

Then log out and back in if the compiled plugin or environment hook changed.

## Uninstall a source installation

```bash
bash ./scripts/uninstall-user.sh
```

Log out and back in afterward.

---

## Verify the installation

The source installer prints the exact installed plugin path. You can also check manually:

```bash
find "$HOME/.local" -path '*plasma/applets/org.chathuranga.globalmenu.so' -print
```

Confirm the Plasma session hook exists:

```bash
cat "$HOME/.config/plasma-workspace/env/global-menu-kde.sh"
```

After logging back in, **Global Menu KDE** should appear in Plasma's widget picker.

## Expected behavior

- Focus Dolphin, Kate, KWrite, or another application that exports a dbusmenu: the menu headings appear in the panel.
- Open **File**, **Edit**, or another heading: the application's real menu is shown.
- Focus the desktop or an application without an exported application menu: the widget collapses to zero panel width.
- There is no Apple/system menu, launcher, clock, search, workspace control, or placeholder.

## Troubleshooting

### KDE neon/unstable appears to stop at `dbusmenumodel_test`

Older revisions of the installer ran the private-session dbusmenu integration test during every local install. On some KDE neon development/unstable combinations, the test process can take a long time to terminate because it creates a private D-Bus session while loading Plasma Workspace's task model.

Update the repository and rerun the installer:

```bash
git pull
bash ./scripts/install-user.sh
```

Current `main` does not run that integration test during ordinary installation. CI/full QA still runs it with a hard 30-second timeout.

If an older installer is already waiting at that test, press **Ctrl+C**, pull the current `main`, and rerun the command above. The completed compilation can normally be reused by CMake.

### The widget is installed but missing from the widget picker

Log out and back in first. The applet is a compiled Qt plugin and the installer adds its plugin root to `QT_PLUGIN_PATH` through Plasma's session environment directory.

Check:

```bash
cat "$HOME/.config/plasma-workspace/env/global-menu-kde.sh"
find "$HOME/.local" -path '*org.chathuranga.globalmenu.so' -print
```

### The widget is present but displays nothing

That is valid when the active application does not export a global menu. Test with Dolphin or Kate.

Firefox, many Electron applications, and some sandboxed applications may not provide a compatible dbusmenu export in a given desktop/session configuration.

### Dolphin/Kate still show an in-window menubar

Application-side menubar visibility is controlled by each application and desktop integration. This widget consumes the exported menu; it does not forcibly remove an application's local menubar.

### Two global menus appear

Remove KDE's stock **Global Menu** widget from the panel and keep **Global Menu KDE**.

### Build cannot find `LibTaskManager`

Install your distribution's Plasma Workspace development package. The project intentionally uses Plasma Workspace's active-task model rather than reimplementing window tracking.

### Clean rebuild

```bash
rm -rf build
bash ./scripts/install-user.sh
```

## Removing all user-installed state

Run:

```bash
bash ./scripts/uninstall-user.sh
```

Then confirm the environment hook was removed:

```bash
ls "$HOME/.config/plasma-workspace/env/global-menu-kde.sh"
```

A `No such file or directory` result is expected after successful removal.
