# QA and Validation Guide

The project uses layered QA so protocol handling, build/install behavior, and Plasma loading are checked independently.

## Automated QA

### Static repository checks

Run:

```bash
bash ./scripts/qa.sh --static
```

This validates:

- Plasma package metadata;
- application-menu-only scope;
- zero-size hidden-state implementation;
- no Apple/system/search/workspace/status UI in the QML surface;
- dbusmenu protocol/lifecycle code paths;
- incremental `ItemsPropertiesUpdated` support;
- RTL and keyboard-activation paths;
- install/uninstall packaging assumptions;
- ShellCheck when available;
- QML linting when `qmllint` is available.

### Full local QA

On a development machine with Qt/KF6/Plasma development packages:

```bash
bash ./scripts/qa.sh
```

The full QA path configures and builds the project, runs the test suite, and performs a `plasmawindowed` load smoke test when the required Plasma/display tooling is available.

### Direct build and test

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The test suite includes:

- shortcut-token translation tests;
- a fake `com.canonical.dbusmenu` exporter running inside a private D-Bus session;
- layout import and submenu/direct action assertions;
- disabled action state;
- `AboutToShow`, `opened`, `closed`, and `clicked` forwarding;
- incremental property changes without unnecessary complete layout refreshes;
- structural property changes that correctly fall back to `GetLayout`.

## GitHub Actions

`.github/workflows/ci.yml` runs two jobs:

### `static-qa`

Ubuntu runner:

- repository checkout;
- ShellCheck installation;
- `bash ./scripts/qa.sh --static`.

### `plasma6-build`

Current Arch Linux container:

- installs Qt 6/KF6/Plasma 6 build dependencies;
- configures a Release build;
- compiles and links the applet;
- executes `ctest`;
- installs into a staging prefix;
- loads the staged applet using `plasmawindowed` under Xvfb/private D-Bus;
- packages an installable `global-menu-kde-plasma6` artifact.

## Manual desktop QA

Automated tests cannot replace real panel interaction. Before a release, complete the following on a normal Plasma 6 desktop.

### Installation

- [ ] Install with `bash ./scripts/install-user.sh`.
- [ ] Log out and log back in.
- [ ] Confirm **Global Menu KDE** is present in the widget picker.
- [ ] Add it to a panel.
- [ ] Confirm uninstall removes the applet and environment hook.

### Menu visibility

- [ ] Focus the desktop: the widget occupies zero panel width.
- [ ] Focus an application without a compatible menu export: the widget occupies zero panel width.
- [ ] Focus Dolphin: application menu headings appear.
- [ ] Focus Kate/KWrite: application menu headings switch to that application.
- [ ] Rapidly switch between exporting applications: no stale menu is displayed.

### Menu functionality

Using Dolphin/Kate/KWrite or another known exporter:

- [ ] Open each top-level heading.
- [ ] Trigger a normal command.
- [ ] Trigger a submenu command.
- [ ] Verify disabled actions remain disabled.
- [ ] Verify checkable actions update state.
- [ ] Verify radio groups are exclusive.
- [ ] Verify icons render where exported.
- [ ] Verify keyboard shortcuts render correctly.
- [ ] Verify Alt mnemonic underlines/activation.
- [ ] Verify Left/Right navigation between headings while a popup is open.
- [ ] Verify hover switching between headings while a popup is open.

### Panel geometry

Test on:

- [ ] top panel;
- [ ] bottom panel;
- [ ] left panel;
- [ ] right panel.

For each placement:

- [ ] popup opens on the usable side of the panel;
- [ ] popup is constrained to the screen work area;
- [ ] headings do not create unnecessary panel expansion;
- [ ] hidden state does not retain an empty placeholder.

### Display/session coverage

- [ ] Wayland session.
- [ ] X11 session, where available.
- [ ] 100% scale.
- [ ] HiDPI/fractional scale.
- [ ] RTL locale/layout.
- [ ] multi-monitor panel placement.

### KDE stock widget coexistence

- [ ] Add KDE's stock Global Menu widget next to this widget.
- [ ] Remove the stock widget and confirm this widget continues working.
- [ ] Remove this widget and confirm the stock widget continues working.
- [ ] Test Plasma's undo/restore flow after widget removal.
- [ ] Confirm `org.kde.kappmenuview` ownership is not left stale after the final menu widget is removed.

## Visual acceptance

The target is a compact application-menu strip similar to a desktop menubar:

```text
File   Edit   View   Go   Tools   Settings   Help
```

The project deliberately does **not** include an Apple/system menu, launcher, clock, workspace switcher, media controls, or other panel widgets.

Check spacing at typical panel heights:

- [ ] 24 px
- [ ] 32 px
- [ ] 40 px
- [ ] 48 px

Compare the result with `docs/global-menu-preview.svg` and tune only the application-menu surface.

## Bug-report information

For a useful issue report, include:

- Plasma version;
- Qt version;
- distribution;
- Wayland or X11;
- affected application/version;
- whether KDE's stock Global Menu works with the same application;
- output of `bash ./scripts/qa.sh --static`;
- relevant journal/plasmashell errors;
- screenshot or short recording when the problem is visual.
