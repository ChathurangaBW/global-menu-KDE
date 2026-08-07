# QA and release validation

The project deliberately separates normal development CI from the final production-release gate.

## Functional contract

A build is not considered correct unless all of these states work:

1. **Desktop / no application menu** → `File Edit View Go Tools Settings Help` is shown inside the existing Plasma panel.
2. **Application exports a global menu** → KDE's native application menu replaces the desktop fallback.
3. **Application menu disappears** → the desktop fallback returns.
4. No second panel, standalone bar, Apple menu, clock, search field, workspace controls, or unrelated status controls are created by this applet.
5. Desktop fallback submenu actions actually execute; rendering a submenu without action dispatch is a failure.

## Automated tests

The repository test suite covers:

- construction of the desktop fallback model;
- fallback menu headings and action structure;
- fallback → application menu → fallback transition over a private session D-Bus;
- KDE dbusmenu importer interaction;
- direct actions and submenus from a fake exporter;
- Release build and CTest execution;
- staged `/usr` installation layout;
- native Plasma plugin discovery with `QT_PLUGIN_PATH` unset;
- headless `plasmawindowed` loading;
- installer/uninstaller shell validation;
- sanitizer and repeated integration runs in the production gate.

## Manual desktop matrix before a stable release

Automated headless loading does not replace real Plasma interaction. A final stable release should be checked on representative systems for:

- KDE neon / current Plasma 6 Wayland session;
- at least one X11 Plasma session when the target distribution still supports it;
- horizontal top/bottom panels;
- vertical left/right panels where Global Menu is supported;
- direct drag from Widget Explorer into an existing panel;
- persistence after logout/login and full reboot;
- Dolphin, Kate/KWrite, and another dbusmenu-exporting application;
- desktop fallback when no application is exporting a menu;
- File/Go/Tools fallback actions, including Home Folder and Konsole;
- submenu hover switching and keyboard navigation;
- RTL layout;
- common panel heights and HiDPI scaling.

## Package release gate

Release assets should be published only when their own package jobs pass. A package job is expected to:

1. build the exact release source;
2. run tests;
3. create the native package;
4. inspect package metadata;
5. install it into the target environment;
6. verify ELF dependencies do not contain unresolved libraries;
7. verify Plasma discovers the applet with `QT_PLUGIN_PATH` unset;
8. smoke-load the installed applet;
9. uninstall the package and verify the plugin is removed.

A GitHub Release with missing or failed package assets is not considered a completed production release.

## Reporting a failure

For a useful bug report, include:

- distribution and exact Plasma version;
- Wayland or X11;
- installation method (`.deb`, `.rpm`, Arch package, source);
- whether the applet can be dragged directly from Widget Explorer to the panel;
- whether it survives logout/login or reboot;
- the active application involved;
- terminal output from the installer/build if relevant;
- a screenshot or short screen recording for layout/interaction problems.
