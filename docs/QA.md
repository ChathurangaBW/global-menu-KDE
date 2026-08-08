# QA validation

Global Menu KDE uses one rolling source-install path. There is no distro-package release gate: the important contract is that the exact repository state builds, tests, installs, is discovered by Plasma without plugin-path hacks, and can be removed cleanly.

## Functional contract

A build is not considered correct unless all of these states work:

1. **Desktop / no application menu** → `File Edit View Go Tools Settings Help` is shown inside the existing Plasma panel.
2. **Application exports a global menu** → KDE's native application menu replaces the desktop fallback.
3. **Application menu disappears** → the desktop fallback returns.
4. No second panel, standalone bar, Apple menu, clock, search field, workspace controls, or unrelated status controls are created by this applet.
5. Desktop fallback submenu actions actually execute; rendering a submenu without action dispatch is a failure.

## Automated CI contract

The repository CI covers:

- construction of the desktop fallback model;
- fallback menu headings and action structure;
- fallback → application menu → fallback transition over D-Bus;
- KDE dbusmenu importer interaction;
- direct actions and submenus from a fake exporter;
- repeated application-menu lifecycle runs;
- Release build and full CTest execution;
- staged `/usr` installation layout;
- unresolved ELF dependency rejection;
- native Plasma plugin discovery with `QT_PLUGIN_PATH` unset;
- headless `plasmawindowed` loading;
- Bash syntax and ShellCheck validation for the installer scripts;
- `install.sh --help` CLI validation;
- end-to-end `install.sh --no-deps` execution as a non-root desktop user;
- persistent uninstall-manifest creation;
- `global-menu-kde-uninstall` removal verification.

The staged smoke test may temporarily set `QT_PLUGIN_PATH` only to point `plasmawindowed` at an uninstalled CI staging tree. The system-install smoke explicitly unsets it and verifies native discovery. The user-facing installer never creates a plugin-path override.

## Real desktop matrix

Automated headless loading does not replace real Plasma interaction. Representative manual checks include:

- KDE neon / current Plasma 6 Wayland session;
- at least one X11 Plasma session where the target distribution still supports it;
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

A manual test should record the exact commit, distribution, Plasma version, display protocol, and installation command used.

## Distribution portability

The installer is a source portability layer, not a universal prebuilt binary. `install.sh` contains dependency-name mappings for `apt`, `dnf`, `pacman`, and `zypper`; other Plasma 6 distributions can use `--no-deps` after supplying the required development packages.

A distro/architecture is not claimed as validated merely because another distro's binary package would have worked there. The host build is the compatibility boundary.

## Reporting a failure

For a useful bug report, include:

- distribution and exact Plasma version;
- Wayland or X11;
- CPU architecture;
- whether dependencies were installed automatically or with `--no-deps`;
- whether the applet can be dragged directly from Widget Explorer to the panel;
- whether it survives logout/login or reboot;
- the active application involved;
- terminal output from `install.sh` if relevant;
- a screenshot or short screen recording for layout/interaction problems.
