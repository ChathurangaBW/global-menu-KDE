# Development and QA checklist

## Implemented

- [x] Plasma 6 C++/QML applet scaffold.
- [x] `org.kde.kappmenuview` registration so KDE's registrar activates.
- [x] Reference-counted, undo-aware registrar-presence lifecycle compatible with KDE's stock widget.
- [x] Active-window tracking through `LibTaskManager`.
- [x] Canonical dbusmenu `GetLayout`, `AboutToShow`, and `Event` integration.
- [x] dbusmenu `opened`, `closed`, and `clicked` lifecycle events.
- [x] Native `QMenu` popup rendering and direct top-level action activation.
- [x] Disabled actions, check items, exclusive radio groups, icons, exported shortcuts, and mnemonics.
- [x] Incremental `ItemsPropertiesUpdated` handling with full-layout fallback for structural changes.
- [x] `DisplayMenuModel` layer for desktop fallback vs real application menu takeover.
- [x] Persistent desktop fallback headings: **File, Edit, View, Go, Tools, Settings, Help**.
- [x] Fallback desktop actions for filesystem locations, Clipboard History, Show Desktop/Restore, KRunner, Konsole, System Monitor, System Settings, and Help Center.
- [x] Automatic replacement of the fallback by an active application's real exported menu.
- [x] Automatic return to the fallback when the application export disappears.
- [x] Existing-panel presentation: `NoBackground`, no `CanFillArea`, compact content width.
- [x] No Apple/system menu, launcher, clock, workspace switcher, media/status area, or second panel.
- [x] Compact KDE-native menubar hover/pressed states.
- [x] RTL layout mirroring.
- [x] Plasmoid keyboard activation opens the first visible menu.
- [x] Static QA guards for fallback/app takeover and compact existing-panel presentation.
- [x] User install/uninstall scripts with Plasma session plugin-path setup.
- [x] Live installer excludes the private-D-Bus integration harness by default on KDE neon/unstable; optional run has a 30-second timeout.
- [x] Qt shortcut translation tests.
- [x] Fake dbusmenu exporter integration test.
- [x] Integration assertions for fallback → app takeover → fallback.
- [x] Integration assertions for layout import, submenu/direct actions, disabled state, incremental properties, `AboutToShow`, `opened`, `closed`, and `clicked`.
- [x] Plasma 6 CI configure/release compilation.
- [x] CI `ctest` execution.
- [x] Staged CMake installation.
- [x] Headless staged-plugin load through `plasmawindowed`, Xvfb, and private D-Bus.
- [x] Installable CI artifact with documentation and install/uninstall helpers.
- [x] Rich README and repository-native desktop-fallback preview.
- [x] Dedicated installation, QA, and architecture documentation.

## Real desktop QA still required

- [ ] Install current `main` on KDE neon unstable and confirm normal installer completes without waiting at `dbusmenumodel_test`.
- [ ] Log out and back in and add **Global Menu KDE** to an existing Plasma panel.
- [ ] Confirm no standalone dark applet background/second-panel appearance.
- [ ] Confirm desktop fallback shows `File Edit View Go Tools Settings Help` with no exporting application active.
- [ ] Confirm each desktop fallback menu opens and its available actions work.
- [ ] Confirm Dolphin replaces the fallback with Dolphin's real exported menu.
- [ ] Confirm Kate/KWrite replaces it with the correct menu and supports submenu/disabled/check/radio/icon/shortcut/mnemonic behavior.
- [ ] Confirm closing the exporting application returns to the desktop fallback.
- [ ] Confirm a non-exporting application leaves the desktop fallback available.
- [ ] Confirm hover switching between headings while a popup is open.
- [ ] Confirm Left/Right keyboard navigation.
- [ ] Confirm popup placement on top, bottom, left, and right panels.
- [ ] Confirm stock/custom Global Menu add/remove/undo coexistence.
- [ ] Confirm Wayland and X11 behavior.
- [ ] Confirm high-DPI and RTL interaction.
- [ ] Tune spacing at 24, 32, 40, and 48 px panel heights.

## Follow-up hardening

- [ ] Replace the SVG preview with captured real-session screenshots after desktop QA.
- [ ] Add screenshot-based visual regression tests after baselines are captured.
- [ ] Publish a signed release after desktop QA.
