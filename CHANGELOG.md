# Changelog

All notable changes to Global Menu KDE are documented here.

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
