# Global Menu KDE

[![CI](https://github.com/ChathurangaBW/global-menu-KDE/actions/workflows/ci.yml/badge.svg)](https://github.com/ChathurangaBW/global-menu-KDE/actions/workflows/ci.yml)
![Plasma 6](https://img.shields.io/badge/KDE%20Plasma-6-1d99f3)
![Qt 6](https://img.shields.io/badge/Qt-6-41cd52)
![License](https://img.shields.io/badge/license-GPL--2.0--or--later-blue)

A focused **KDE Plasma 6 global application menu** that displays only the active application's exported menu headings in a panel.

```text
File   Edit   View   Go   Tools   Settings   Help
```

It is intentionally **not** a complete macOS-style top bar replacement. There is no Apple/system menu, launcher, clock, workspace switcher, search, media/status area, or synthetic fallback menu.

<p align="center">
  <img src="docs/global-menu-preview.svg" alt="Global Menu KDE preview showing File, Edit, View, Go, Tools, Settings and Help with the File menu open" width="100%">
</p>

## What it does

| Situation | Result |
| --- | --- |
| Dolphin/Kate/KWrite or another compatible menu-exporting app is active | The application's real exported headings appear in the panel |
| The user opens **File**, **Edit**, etc. | A native Qt/KDE `QMenu` popup is shown and actions are sent back to the application |
| The active application changes | The displayed menu switches to the new application |
| The desktop or an app without a compatible export is active | The applet enters `HiddenStatus` and collapses to **zero panel size** |
| KDE's stock Global Menu widget is removed | This applet keeps KDE's menu infrastructure active through an undo-aware view-service lease |

## Scope

### Included

- Active-application tracking through Plasma Workspace `LibTaskManager`.
- Canonical `com.canonical.dbusmenu` layout import over QtDBus.
- Top-level headings only: **File**, **Edit**, **View**, **Go**, **Tools**, **Settings**, **Help**, and whatever else the application exports.
- Native `QMenu` popups and submenus.
- Disabled actions.
- Checkable actions and exclusive radio groups.
- Theme icons and exported raw icon data.
- Exported keyboard shortcuts.
- Alt mnemonics.
- Hover switching while a menu is open.
- Left/Right keyboard navigation between headings.
- Direct top-level actions.
- Incremental `ItemsPropertiesUpdated` handling without unnecessary complete menu reloads.
- RTL layout mirroring.
- Explicit zero-size hidden state.

### Deliberately not included

- Apple/system menu.
- Application launcher.
- KRunner/search box.
- Workspace indicator/switcher.
- Clock or calendar.
- Media controls.
- System tray or status controls.
- Synthetic fallback application actions.
- A placeholder when no menu is available.

The repository QA contains guards that reject these out-of-scope UI elements from the applet surface.

---

## Quick installation

### Prebuilt GitHub Actions artifact

Every successful CI run publishes an artifact named **`global-menu-kde-plasma6`**.

1. Open **Actions → CI** in this repository.
2. Open the latest successful run.
3. Download `global-menu-kde-plasma6`.
4. Extract the ZIP and then the contained tarball:

   ```bash
   tar -xzf global-menu-kde-plasma6.tar.gz
   ```

5. Install it:

   ```bash
   bash ./install.sh
   ```

6. **Log out and log back in.**
7. Add **Global Menu KDE** to a Plasma panel.
8. Remove KDE's stock **Global Menu** widget if you do not want two menu widgets.

> The prebuilt artifact is compiled on current Arch Linux x86-64. On distributions with different Qt/KF6/Plasma library versions, build from source instead.

### Build and install from source

```bash
git clone https://github.com/ChathurangaBW/global-menu-KDE.git
cd global-menu-KDE
```

While PR #1 is still under development, switch to the implementation branch:

```bash
git switch agent/plasma6-global-menu
```

Then run:

```bash
bash ./scripts/install-user.sh
```

The installer configures a Release build, compiles the applet, runs the Qt test suite, installs under `~/.local`, and creates the Plasma-session `QT_PLUGIN_PATH` hook required for the compiled plugin.

After installation, **log out and back in** before adding the widget.

Full installation details and troubleshooting: **[docs/INSTALL.md](docs/INSTALL.md)**.

---

## Requirements

| Component | Requirement |
| --- | --- |
| Desktop | KDE Plasma 6 |
| Qt | Qt 6.6 or newer |
| KDE Frameworks | KF6 |
| Build system | CMake + Extra CMake Modules |
| Active-window integration | Plasma Workspace `LibTaskManager` |
| Display session | Wayland or X11 |

The CI build currently uses these Arch Linux packages:

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
xorg-server-xvfb
```

Package names vary by distribution.

---

## How it works

```text
Active application
      │
      │ exports Canonical dbusmenu
      ▼
KDE's com.canonical.AppMenu.Registrar
      │
      │ menu service + object path
      ▼
GlobalMenuModel
      │
      ├─ LibTaskManager active-window tracking
      ├─ QtDBus GetLayout / AboutToShow / Event
      ├─ QAction / QMenu model
      └─ incremental property updates
      │
      ▼
GlobalMenuApplet
      │
      ├─ popup positioning
      ├─ menu switching
      └─ keyboard navigation
      │
      ▼
QML panel surface

File   Edit   View   Go   Tools   Settings   Help
```

The project does **not** take ownership of `com.canonical.AppMenu.Registrar`; KDE already provides that service. Competing for it would break normal Plasma/Qt global-menu integration.

Technical details: **[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)**.

---

## dbusmenu behavior

The importer supports the menu state expected from Qt/KDE application exporters:

- `GetLayout` for the exported menu tree;
- `AboutToShow` before opening lazily populated menus;
- `opened` and `closed` lifecycle events;
- `clicked` action events;
- label/visibility/enabled changes;
- toggle/check/radio state;
- theme icons and raw `icon-data`;
- exported keyboard shortcuts;
- structural refresh fallback when menu hierarchy changes.

Asynchronous layout replies are guarded with a source generation counter so a delayed reply from the previously active application cannot overwrite the current application's menu.

---

## Build and test

### Standard CMake build

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Repository QA

Static checks only:

```bash
bash ./scripts/qa.sh --static
```

Full local QA:

```bash
bash ./scripts/qa.sh
```

The full path also attempts a `plasmawindowed` applet-load smoke test when suitable Plasma/display tooling is available.

Detailed validation matrix and real-desktop checklist: **[docs/QA.md](docs/QA.md)**.

---

## Automated QA status

GitHub Actions validates the implementation at several layers:

| Layer | Coverage |
| --- | --- |
| Static QA | metadata, menu-only scope, hidden-state sizing, protocol guards, packaging assumptions, ShellCheck |
| Qt unit tests | dbusmenu shortcut token translation |
| Fake-exporter integration | private D-Bus session, layout import, direct/submenu actions, disabled state, lifecycle events, incremental updates |
| Plasma 6 build | current Arch Linux Qt/KF6/Plasma release build and link |
| Installation | staged CMake install |
| QML/plugin loading | `plasmawindowed` under Xvfb/private D-Bus |
| Packaging | installable `global-menu-kde-plasma6` artifact |

Real desktop interaction remains a separate release checklist because CI cannot reproduce a user's complete Plasma panel/session environment. See **[docs/QA.md](docs/QA.md)** and **[TODO.md](TODO.md)**.

---

## Expected desktop behavior

### Compatible application active

Dolphin, Kate, KWrite, and other applications that export a compatible application menu should show their headings in the panel.

Example:

```text
File   Edit   View   Go   Tools   Settings   Help
```

Clicking a heading opens the application's real menu; selecting an item executes it in that application.

### No compatible menu export

The applet should occupy no visible panel width. It does not display `Global Menu KDE`, an icon, an ellipsis, or a fallback menu.

Firefox, Electron applications, sandboxed applications, or other software may not expose a compatible dbusmenu depending on their toolkit/build/session integration. In that case, hidden state is expected.

---

## Troubleshooting

### Widget does not appear in the widget picker

The project is a compiled Plasma plugin. After installation, **log out and log back in** so Plasma starts with the installer-generated plugin search path.

Check:

```bash
cat "$HOME/.config/plasma-workspace/env/global-menu-kde.sh"
find "$HOME/.local" -path '*org.chathuranga.globalmenu.so' -print
```

### Widget appears but shows nothing

Test with Dolphin or Kate. Empty/zero-width state is intentional when the active application does not export a compatible menu.

### Two global menus are visible

Remove KDE's stock **Global Menu** widget from the panel and keep **Global Menu KDE**.

### `LibTaskManager` cannot be found while building

Install the Plasma Workspace development package supplied by your distribution.

More troubleshooting: **[docs/INSTALL.md](docs/INSTALL.md#troubleshooting)**.

---

## Uninstall

Source installation:

```bash
bash ./scripts/uninstall-user.sh
```

Prebuilt artifact:

```bash
bash ./uninstall.sh
```

Log out and back in after uninstalling so the Plasma session no longer uses the plugin path hook.

---

## Repository layout

```text
.
├── src/                    Plasma applet, model, dbusmenu client and QML
├── tests/                  Qt unit + fake-exporter integration tests
├── scripts/                install, uninstall, QA and smoke-test tooling
├── docs/
│   ├── global-menu-preview.svg
│   ├── INSTALL.md
│   ├── QA.md
│   └── ARCHITECTURE.md
├── .github/workflows/ci.yml
├── TODO.md
└── README.md
```

---

## Documentation

- **[Installation and troubleshooting](docs/INSTALL.md)**
- **[QA and release validation](docs/QA.md)**
- **[Architecture](docs/ARCHITECTURE.md)**
- **[Development checklist](TODO.md)**

---

## License

GPL-2.0-or-later.

The small dbusmenu serialization/shortcut component is LGPL-2.0-or-later. Individual source files contain SPDX identifiers.
