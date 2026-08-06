# Development and QA checklist

## Implemented

- [x] Plasma 6 C++/QML applet scaffold.
- [x] `org.kde.kappmenuview` registration so KDE's registrar activates.
- [x] Reacquire the shared presence service if KDE's stock widget unregisters it.
- [x] Active-window tracking through `LibTaskManager`.
- [x] Canonical dbusmenu `GetLayout`, `AboutToShow`, and `Event` integration.
- [x] dbusmenu `opened`, `closed`, and `clicked` lifecycle events.
- [x] Native `QMenu` popup rendering and direct top-level action activation.
- [x] Disabled actions, check items, contiguous exclusive radio groups, icons, and exported keyboard shortcuts.
- [x] Top-level menu headings only.
- [x] Complete hiding when no active application menu is exported.
- [x] No persistent placeholder, Apple menu, search, clock, workspace controls, or synthetic fallback.
- [x] User install and uninstall scripts with Plasma session plugin-path setup.
- [x] Static source and packaging checks.
- [x] Qt shortcut-translation unit tests.
- [x] Plasma 6 CI configure and compilation against current Arch Linux packages.
- [x] Plasma 6 CI `ctest` execution.

## Desktop QA still required

- [ ] Build against the target distribution's Plasma 6 development packages.
- [ ] Run the Qt test suite on the target distribution.
- [ ] Install into `~/.local`, log out, and log back in.
- [ ] Confirm the widget has zero panel width on the desktop.
- [ ] Confirm the widget has zero panel width for Firefox/Electron apps without an export.
- [ ] Confirm Dolphin shows its exported headings and each command executes.
- [ ] Confirm Kate/KWrite submenu, disabled item, check item, radio item, icon, shortcut, and mnemonic behavior.
- [ ] Confirm hover switching between headings while a popup is open.
- [ ] Confirm Left/Right keyboard navigation between headings.
- [ ] Confirm correct popup placement on top, bottom, left, and right panels.
- [ ] Confirm the registrar remains active after removing KDE's stock Global Menu widget.
- [ ] Confirm Wayland and X11 behavior.
- [ ] Confirm high-DPI and RTL layouts.
- [ ] Tune spacing against the supplied reference image at 24, 32, 40, and 48 px panel heights.

## Follow-up hardening

- [ ] Process `ItemsPropertiesUpdated` incrementally instead of refreshing the complete layout.
- [ ] Add an automated fake dbusmenu exporter for end-to-end protocol tests.
- [ ] Package a signed release archive.
