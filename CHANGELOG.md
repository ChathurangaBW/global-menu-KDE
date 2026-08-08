# Changelog

Notable changes to the rolling `main` branch are documented here. Global Menu KDE is distributed through its source installer rather than versioned distro packages or GitHub Release assets.

## Current

### Added

- Native KDE Plasma 6 Global Menu applet architecture based on Plasma Workspace's appmenu implementation.
- Desktop fallback menu with `File`, `Edit`, `View`, `Go`, `Tools`, `Settings`, and `Help` when no application exports a global menu.
- Automatic takeover by a compatible application's exported global menu and restoration of the desktop fallback when that menu disappears.
- Native system plugin installation under `/usr` with no persistent `QT_PLUGIN_PATH` workaround.
- Desktop fallback and application-menu transition tests.
- Headless Plasma plugin discovery/load smoke tests.
- Portable `install.sh` source installer with dependency mapping for apt, dnf, pacman, and zypper.
- `--no-deps` installation path for other Plasma 6 distributions.
- Persistent install manifest and `global-menu-kde-uninstall` helper.
- Rich installation, architecture, QA, and visual documentation.

### Fixed

- Desktop fallback submenu actions execute their real commands instead of being display-only.
- Native Widget Explorer/panel integration and login/reboot persistence by using KDE's normal binary applet install path.

### Distribution changes

- Removed CPack DEB/RPM generation.
- Removed Arch package metadata and Debian package metadata.
- Removed the native-package/GitHub Release production workflow.
- The supported distribution mechanism is now a host-compiled Plasma 6 source install from `main`.
