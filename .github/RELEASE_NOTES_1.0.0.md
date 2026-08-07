# Global Menu KDE 1.0.0

First production release of the Plasma 6 Global Menu KDE applet.

## Behavior

- Lives inside an existing Plasma panel; it does not create a second panel.
- Shows `File Edit View Go Tools Settings Help` as a desktop fallback when no application exports a menu.
- Automatically switches to the active application's real exported dbusmenu when available.
- Restores the desktop fallback when the application menu disappears.
- Uses KDE/Qt native menu actions, submenu behavior, disabled/check/radio states, icons, shortcuts, mnemonics, hover switching, and RTL-aware layout.

## Production QA gate

The release is published only after all of the following pass on the exact release commit:

- static product-contract and packaging validation;
- GCC Release builds and complete Qt test suite;
- Clang Debug build under AddressSanitizer + UndefinedBehaviorSanitizer;
- repeated unit/integration tests to detect intermittent failures;
- staged-install dependency audit with `ldd`;
- headless `plasmawindowed` applet load smoke test;
- native package build, metadata inspection, installation, dependency audit, and installed-plugin smoke test.

## Native packages

Release assets are built on and tested against their target runtime family:

- Debian 13: `amd64` and `arm64` `.deb` packages;
- Fedora 44: `x86_64` and `aarch64` `.rpm` packages;
- Arch Linux: `x86_64` `.pkg.tar.zst` package;
- source tarball for other Plasma 6 distributions/architectures.

Plasma plugins are ABI-sensitive. A package built for one distro family is not claimed to be a universal package for every Plasma/Qt/KF6 combination. Use the native package matching the listed target, or build the source tarball on the destination system.

## Upgrade

Remove older development/prebuilt installations before installing a system package if they were installed under `~/.local`, then log out and back in after installation so Plasma reloads the plugin path and applet metadata.
