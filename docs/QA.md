# QA and Validation Guide

Global Menu KDE has two visible states inside the **same existing Plasma panel**:

1. **Desktop fallback** — `File Edit View Go Tools Settings Help` when no application exports a usable global menu.
2. **Application takeover** — the active application's real exported menu when available.

QA must verify both states and the transition between them.

## Automated QA

### Static repository checks

```bash
bash ./scripts/qa.sh --static
```

This validates:

- Plasma package metadata;
- the seven desktop fallback headings;
- application takeover wiring through `DisplayMenuModel`;
- `NoBackground` and no `CanFillArea`, preventing a second-panel presentation;
- dbusmenu protocol/lifecycle paths;
- incremental `ItemsPropertiesUpdated` support;
- stale-reply protection;
- RTL and keyboard activation paths;
- live-installer integration-test timeout/skip behavior;
- install/uninstall packaging assumptions;
- ShellCheck and QML linting when available.

### Full local QA

```bash
bash ./scripts/qa.sh
```

This configures/builds the project, runs all tests, and performs a `plasmawindowed` load smoke test when suitable display tooling is available.

### Direct CMake QA

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The integration suite verifies:

- desktop fallback contains exactly **File, Edit, View, Go, Tools, Settings, Help**;
- each fallback heading owns a usable menu;
- a fake `com.canonical.dbusmenu` exporter replaces the fallback;
- removing/hiding the exported application menu restores the fallback;
- layout import, submenus, direct actions, disabled state;
- `AboutToShow`, `opened`, `closed`, and `clicked` forwarding;
- incremental property changes without unnecessary complete layout reloads;
- structural changes falling back to `GetLayout`.

## Live installer behavior

Normal installation intentionally excludes the private-D-Bus integration test:

```bash
bash ./scripts/install-user.sh
```

This avoids the KDE neon/unstable teardown hang observed with older revisions.

To explicitly run integration QA during installation:

```bash
RUN_INTEGRATION_TESTS=1 bash ./scripts/install-user.sh
```

The test has a 30-second hard timeout.

## GitHub Actions

`.github/workflows/ci.yml` runs:

### `static-qa`

- repository checkout;
- ShellCheck installation;
- `bash ./scripts/qa.sh --static`.

### `plasma6-build`

Current Arch Linux container:

- installs Qt 6/KF6/Plasma 6 build dependencies;
- configures a Release build;
- compiles and links the applet;
- executes the full `ctest` suite, including the fallback/application integration test;
- staged installation;
- headless `plasmawindowed` load under Xvfb/private D-Bus;
- installable artifact generation.

---

## Manual desktop QA

### Installation

- [ ] Install with `bash ./scripts/install-user.sh`.
- [ ] Log out and log back in.
- [ ] Confirm **Global Menu KDE** appears in the widget picker.
- [ ] Add it to an **existing Plasma panel**.
- [ ] Confirm it does not require or create another panel.
- [ ] Confirm uninstall removes the plugin and environment hook.

### Desktop fallback

With no menu-exporting application active:

- [ ] Existing panel shows `File Edit View Go Tools Settings Help`.
- [ ] The applet has no separate wide dark background.
- [ ] Headings occupy only their content width.
- [ ] **File** opens Home/Documents/Downloads/Trash actions.
- [ ] **Edit** opens Clipboard History.
- [ ] **View** can show/restore desktop windows.
- [ ] **Go** opens common filesystem locations.
- [ ] **Tools** exposes Run Command, Konsole, and System Monitor when installed.
- [ ] **Settings** opens System Settings.
- [ ] **Help** opens KDE Help Center.

### Application takeover

- [ ] Focus Dolphin: fallback is replaced by Dolphin's exported headings.
- [ ] Focus Kate/KWrite: menu changes to that application's exported headings.
- [ ] Close the exporting application: fallback returns.
- [ ] Focus an application without a compatible export: fallback remains available.
- [ ] Rapidly switch between applications: no stale menu is shown.

### Application menu functionality

Using Dolphin/Kate/KWrite:

- [ ] Open each top-level heading.
- [ ] Trigger normal commands.
- [ ] Trigger submenu commands.
- [ ] Disabled actions remain disabled.
- [ ] Checkable actions update state.
- [ ] Radio groups remain exclusive.
- [ ] Exported icons render.
- [ ] Exported shortcuts render.
- [ ] Alt mnemonic behavior works.
- [ ] Left/Right navigation between headings works.
- [ ] Hover switching between headings works while a popup is open.

### Panel geometry

Test in an existing:

- [ ] top panel;
- [ ] bottom panel;
- [ ] left panel;
- [ ] right panel.

For each placement:

- [ ] the applet does not stretch to fill the entire panel;
- [ ] no independent applet background/panel surface is drawn;
- [ ] popup opens on the usable side;
- [ ] popup is constrained to the screen work area;
- [ ] desktop fallback and application menu have compact content width.

### Display/session coverage

- [ ] Wayland.
- [ ] X11, where available.
- [ ] 100% scale.
- [ ] HiDPI/fractional scaling.
- [ ] RTL layout.
- [ ] multi-monitor panel placement.

### KDE stock Global Menu coexistence

- [ ] Add KDE's stock Global Menu next to Global Menu KDE.
- [ ] Remove the stock widget and confirm this applet keeps application export active.
- [ ] Remove this applet and confirm the stock widget still works.
- [ ] Test Plasma undo/restore after widget removal.
- [ ] Confirm `org.kde.kappmenuview` ownership is not left stale.

---

## Visual acceptance

Desktop fallback target:

```text
File   Edit   View   Go   Tools   Settings   Help
```

It must look like menu text integrated into the current Plasma panel—not a standalone floating panel or full-width dark rectangle.

Compare with [`global-menu-preview.svg`](global-menu-preview.svg).

Check typical panel heights:

- [ ] 24 px
- [ ] 32 px
- [ ] 40 px
- [ ] 48 px

## Bug report information

Include:

- Plasma version;
- Qt version;
- distribution;
- Wayland/X11;
- affected application;
- whether KDE's stock Global Menu works with that application;
- output of `bash ./scripts/qa.sh --static`;
- relevant plasmashell/journal errors;
- screenshot or short recording for visual/layout issues.
