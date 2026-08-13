# Changelog

All notable changes to Global Menu KDE are documented here.

## [1.1.1]

### Fixed

- Restart Plasma Shell through the managed user service when available, avoiding a kill-first shell restart that can leave an empty desktop.
- Added Close Window for unsupported applications through LibTaskManager's supported active-task API.
- Added confirmed Force Quit Window using KWin's interactive kill-window action.
- Disable window actions when no active window or supported operation is available.

### Testing

- Added restart-action and active-window action regression coverage.
- Added live X11+D-Bus Plasma loader validation for the updated plugin.

## [1.1.0]

### Added

- Reworked the desktop fallback around the requested File, Edit, View, Go, Tools, Settings, and Help actions from issue #14.
- Added a native-style Create New submenu for folders, text files, HTML files, URL links, file or directory links, and application launchers.
- Added Plasma session actions for shell restart, screen locking, logout prompting, desktop peeking, Overview, and Activities.
- Added direct launchers for desktop settings, display settings, common Plasma tools, system settings pages, project help, and the KDE community.

### Changed

- Made Go the single owner of common locations, including Home, Documents, Downloads, Trash, Root Filesystem, Network, and Recent Locations.
- Disable optional program actions when their executable is not installed.
- Require confirmation before restarting Plasma Shell.

### Security

- Reject path separators and traversal names when creating desktop items.
- Refuse to overwrite an existing desktop item.

### Testing

- Added regression coverage for the complete menu hierarchy, duplicate prevention, availability handling, action dispatch, restart confirmation, and safe desktop item creation.
- Isolated the D-Bus integration test from host desktop service activation for deterministic headless execution.

## [1.0.2]

### Fixed

- Removed duplicate Documents, Downloads, and Trash entries from the fallback Go menu.
- Kept common user folders under File and made Go contain distinct navigation targets: Root Filesystem, Network, and Recent Locations.
- Added regression coverage proving File and Go do not share location entries.

## [1.0.1]

### Added

- Debian packages for amd64 and arm64.
- Fedora RPM packages for x86_64 and aarch64.
- Arch Linux package and PKGBUILD.
- Reproducible source archive and SHA256 checksums.

## [1.0.0]

### Added

- Native KDE Plasma 6 Global Menu applet with desktop fallback.
- Portable source installer with dependency mapping for apt, dnf, pacman, and zypper.
- Native system plugin installation under `/usr` without a persistent `QT_PLUGIN_PATH` workaround.
- Desktop fallback and application-menu transition tests.
