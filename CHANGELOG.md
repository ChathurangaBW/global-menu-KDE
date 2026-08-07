# Changelog

All notable changes to Global Menu KDE are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and releases use semantic versioning.

## [1.0.0] - 2026-08-07

### Added

- Native KDE Plasma 6 Global Menu applet architecture based on Plasma Workspace's appmenu implementation.
- Desktop fallback menu with `File`, `Edit`, `View`, `Go`, `Tools`, `Settings`, and `Help` when no application exports a global menu.
- Automatic takeover by a compatible application's exported global menu and restoration of the desktop fallback when that menu disappears.
- Native system plugin installation under `/usr` with no persistent `QT_PLUGIN_PATH` workaround.
- Desktop fallback and application-menu transition tests.
- Headless Plasma plugin discovery/load smoke tests.
- Production package QA for Debian-family, Fedora/RPM-family, Arch Linux, and source archives.
- Rich installation, architecture, QA, and visual documentation.

### Fixed

- Desktop fallback submenu actions execute their real commands instead of being display-only.
- Native Widget Explorer/panel integration and login/reboot persistence by using KDE's normal binary applet install path.
