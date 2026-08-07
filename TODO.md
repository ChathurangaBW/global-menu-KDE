# Development and QA checklist

## Implemented

- [x] Plasma 6 C++/QML applet scaffold.
- [x] `org.kde.kappmenuview` registration so KDE's registrar activates.
- [x] Reference-counted, undo-aware registrar-presence lifecycle compatible with KDE's stock widget.
- [x] Reacquire the shared presence service if KDE's stock widget unregisters it.
- [x] Active-window tracking through `LibTaskManager`.
- [x] Canonical dbusmenu `GetLayout`, `AboutToShow`, and `Event` integration.
- [x] dbusmenu `opened`, `closed`, and `clicked` lifecycle events.
- [x] Native `QMenu` popup rendering and direct top-level action activation.
- [x] Disabled actions, check items, contiguous exclusive radio groups, icons, exported keyboard shortcuts, and mnemonics.
- [x] Incremental `ItemsPropertiesUpdated` handling for non-structural action changes, with full-layout fallback for structural changes.
- [x] Top-level menu headings only.
- [x] Complete hiding when no active application menu is exported.
- [x] No persistent placeholder, Apple menu, search, clock, workspace controls, or synthetic fallback.
- [x] User install and uninstall scripts with Plasma session plugin-path setup.
- [x] Static source and packaging checks.
- [x] Qt shortcut-translation unit tests.
- [x] Fake dbusmenu exporter integration test in a private D-Bus session.
- [x] Integration assertions for layout import, submenu/direct actions, disabled state, incremental property updates, `AboutToShow`, `opened`, `closed`, and `clicked`.
- [x] Plasma 6 CI configure and release compilation against current Arch Linux packages.
- [x] Plasma 6 CI `ctest` execution.
- [x] Staged CMake installation in CI.
- [x] Headless staged-plugin load through `plasmawindowed`, Xvfb, and a private D-Bus session.
- [x] Reusable version-adaptive Plasma applet smoke launcher.
- [x] Installable CI artifact with prebuilt install/uninstall helpers.
- [x] Artifact structure, ELF metadata, dependency list, install, environment-hook, and uninstall smoke tests.

## Desktop QA still required

- [ ] Build against the target distribution's Plasma 6 development packages, unless using the compatible prebuilt artifact.
- [ ] Run the Qt test suite on the target distribution when building from source.
- [ ] Install into `~/.local`, log out, and log back in on a real Plasma session.
- [ ] Confirm the widget has zero panel width on the desktop.
- [ ] Confirm the widget has zero panel width for Firefox/Electron apps without an export.
- [ ] Confirm Dolphin shows its exported headings and each command executes.
- [ ] Confirm Kate/KWrite submenu, disabled item, check item, radio item, icon, shortcut, and mnemonic behavior.
- [ ] Confirm hover switching between headings while a popup is open.
- [ ] Confirm Left/Right keyboard navigation between headings.
- [ ] Confirm correct popup placement on top, bottom, left, and right panels.
- [ ] Confirm registrar behavior while adding, removing, and undo-restoring stock and custom Global Menu widgets.
- [ ] Confirm Wayland and X11 behavior.
- [ ] Confirm high-DPI and RTL layouts.
- [ ] Tune spacing against the supplied reference image at 24, 32, 40, and 48 px panel heights.

## Follow-up hardening

- [ ] Add screenshot-based visual regression tests after desktop layout baselines are captured.
- [ ] Publish a signed release after desktop QA.
