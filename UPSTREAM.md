# Upstream Provenance

Global Menu KDE keeps the Plasma applet architecture and uses KDE's normal global-menu handoff whenever an application exports a usable menu. The local desktop fallback is project-specific behavior for the no-menu case.

## KDE-Derived Applet Code

The native applet implementation is derived from KDE Plasma Workspace's Global Menu applet. KDE-derived files retain their original SPDX copyright and license headers:

- `src/appmenuapplet.cpp`
- `src/appmenuapplet.h`
- `src/appmenumodel.cpp`
- `src/appmenumodel.h`
- `src/qml/*.qml`

## Vendored D-Bus Menu Importer

`third_party/libdbusmenuqt/` contains the private `libdbusmenuqt` importer sources needed by this standalone applet because Plasma Workspace does not expose that importer as a public framework target. The vendored files retain their Canonical/KDE SPDX metadata.

## Project-Specific Code

The desktop fallback, installer, packaging scripts, tests, and documentation are project-specific and use the SPDX headers declared in each file.
