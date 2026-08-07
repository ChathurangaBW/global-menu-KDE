# Installation Guide

Global Menu KDE is a compiled Plasma 6 applet. It belongs **inside an existing Plasma panel** and provides two modes:

1. **Desktop fallback** when no application exports a menu:
   `File Edit View Go Tools Settings Help`
2. **Application takeover** when Dolphin, Kate, KWrite, or another compatible application exports its real menu.

It does not create a second panel.

## Option 1 — Build and install from source

This is the recommended path for KDE neon, Fedora, openSUSE, Debian/Ubuntu, and other systems whose Qt/KF6/Plasma ABI may differ from the CI artifact.

### Requirements

- KDE Plasma 6
- Qt 6.6+
- KDE Frameworks 6
- CMake
- Extra CMake Modules
- C++ toolchain
- Plasma Workspace development files exposing `LibTaskManager`

Clone and install:

```bash
git clone https://github.com/ChathurangaBW/global-menu-KDE.git
cd global-menu-KDE
bash ./scripts/install-user.sh
```

The installer:

1. Configures a Release build.
2. Compiles the applet.
3. Runs the deterministic unit/AppStream checks used for live installation.
4. Skips the private-D-Bus integration harness by default so KDE neon/unstable cannot hang during test teardown.
5. Installs under `~/.local` by default.
6. Creates `~/.config/plasma-workspace/env/global-menu-kde.sh` with the required `QT_PLUGIN_PATH`.
7. Refreshes KDE's service cache when `kbuildsycoca6` is available.

To also run the full integration test during installation:

```bash
RUN_INTEGRATION_TESTS=1 bash ./scripts/install-user.sh
```

That integration test has a 30-second hard timeout.

### KDE neon / unstable

If an older checkout stopped at:

```text
Start 3: dbusmenumodel_test
```

press **Ctrl+C**, then update and rerun:

```bash
git pull --ff-only
bash ./scripts/install-user.sh
```

The already-built objects can normally be reused.

### Custom paths

```bash
PREFIX="$HOME/.local" bash ./scripts/install-user.sh
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

For normal user installation, prefer `scripts/install-user.sh` because it also configures the Plasma plugin search path.

---

## Option 2 — Prebuilt CI artifact

Every successful GitHub Actions build publishes `global-menu-kde-plasma6`.

The artifact contains:

- compiled applet plugin;
- `install.sh`;
- `uninstall.sh`;
- README and docs.

The binary is built on current Arch Linux x86-64. Use it only when your Qt/KF6/Plasma versions are compatible.

1. Open **Actions → CI**.
2. Download `global-menu-kde-plasma6` from a successful run.
3. Extract the ZIP.
4. Extract the tarball:

```bash
tar -xzf global-menu-kde-plasma6.tar.gz
```

5. Install:

```bash
bash ./install.sh
```

---

## After installation

**Log out and log back in.** Plasma reads the plugin search path when the session starts.

Then:

1. Edit your **existing Plasma panel**.
2. Add **Global Menu KDE**.
3. Remove KDE's stock **Global Menu** widget if both are present.
4. Do not create a separate panel for this applet.

### Expected desktop fallback

With the desktop active or no compatible app menu available, the panel should show:

```text
File   Edit   View   Go   Tools   Settings   Help
```

The fallback menus provide Plasma desktop actions such as Home, Trash, Clipboard History, Show Desktop, KRunner, Konsole, System Monitor, System Settings, and Help Center.

### Expected application takeover

Focus Dolphin or Kate. The fallback should immediately be replaced by that application's real exported menu. Clicking its items must invoke the application actions.

When the application closes or its menu export disappears, the desktop fallback returns.

---

## Verify installation

Locate the plugin:

```bash
find "$HOME/.local" -path '*plasma/applets/org.chathuranga.globalmenu.so' -print
```

Check the Plasma session hook:

```bash
cat "$HOME/.config/plasma-workspace/env/global-menu-kde.sh"
```

After logging back in, **Global Menu KDE** should appear in the widget picker.

---

## Troubleshooting

### A large dark rectangle appears on the desktop

The applet is intended for an **existing Plasma panel**, not as a standalone desktop widget. Current QML also requests `NoBackground` and no fill-area constraint so it does not create a second panel surface.

Remove the desktop instance and add **Global Menu KDE** to your panel.

### Desktop fallback does not appear

Confirm you are running current `main`:

```bash
git pull --ff-only
bash ./scripts/install-user.sh
```

Log out and back in after reinstalling.

### Dolphin/Kate does not replace the fallback

The application must export a compatible global menu through KDE's application-menu registrar. Run the checks in [`QA.md`](QA.md).

### Widget is missing from the widget picker

Log out and back in first, then check:

```bash
cat "$HOME/.config/plasma-workspace/env/global-menu-kde.sh"
find "$HOME/.local" -path '*org.chathuranga.globalmenu.so' -print
```

### Two menu applets are visible

Remove KDE's stock **Global Menu** widget and keep **Global Menu KDE**.

### `LibTaskManager` cannot be found

Install the Plasma Workspace development package supplied by your distribution.

### Clean rebuild

```bash
rm -rf build
bash ./scripts/install-user.sh
```

---

## Update

```bash
git pull --ff-only
bash ./scripts/install-user.sh
```

Log out and back in when the plugin changes.

## Uninstall

Source installation:

```bash
bash ./scripts/uninstall-user.sh
```

Prebuilt installation:

```bash
bash ./uninstall.sh
```

Log out and back in afterward.
