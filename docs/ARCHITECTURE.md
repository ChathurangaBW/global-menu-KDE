# Architecture

KDE Global Menu is intentionally built as a small extension of KDE Plasma's native Global Menu applet architecture rather than as a custom panel/menu shell.

## Design objective

Preserve KDE's existing behavior for real application menus and change only the missing desktop state.

```text
                 ┌─────────────────────────────┐
                 │ KDE AppMenuApplet / QML UI │
                 └──────────────┬──────────────┘
                                │
                 ┌──────────────▼──────────────┐
                 │        AppMenuModel         │
                 └───────────┬─────────┬───────┘
                             │         │
              app menu exists│         │no app menu
                             │         │
              ┌──────────────▼───┐  ┌──▼────────────────┐
              │ KDE dbusmenu     │  │ Desktop fallback │
              │ importer         │  │ local QMenu      │
              └──────────────┬───┘  └──┬────────────────┘
                             │         │
                             └────┬────┘
                                  │
                       ┌──────────▼──────────┐
                       │ Existing Plasma    │
                       │ panel menu surface │
                       └─────────────────────┘
```

## Native application path

For an application such as Dolphin or Kate that exports a compatible global menu:

1. Plasma/LibTaskManager identifies the active task and its application-menu D-Bus service/object path.
2. `AppMenuModel` selects that source.
3. KDE's dbusmenu importer builds the corresponding `QMenu`/`QAction` hierarchy.
4. KDE's normal applet controller and QML present the headings and native popups.

The project does not add an extra toolbar or panel around this path.

## Desktop fallback path

When no usable application menu is exported, `AppMenuModel` selects a local desktop fallback menu instead of hiding the applet.

The fallback exposes these headings:

```text
File   Edit   View   Go   Tools   Settings   Help
```

The fallback implementation is intentionally isolated from KDE's real application-menu path. As soon as an application menu becomes available, the native application source wins and the fallback disappears.

## Why the plugin installs under `/usr`

This is a compiled Plasma applet. Installing into the normal system Qt/KDE plugin prefix allows Plasma's Widget Explorer and plugin loader to discover it in the same way as other native binary applets.

The production design therefore avoids the older development approach that depended on a per-session `QT_PLUGIN_PATH` hook under the user's home directory.

## Vendored dbusmenu importer

Plasma Workspace builds its dbusmenu importer as a private target, so an external standalone repository cannot link against it as a public KDE Frameworks dependency. The relevant private importer code is vendored under `third_party/libdbusmenuqt/` with its original SPDX licensing retained.

## Scope boundaries

This applet intentionally does **not** implement:

- a new Plasma panel;
- an Apple logo/system menu;
- a clock, system tray, workspace switcher, or search field;
- its own window manager/task switcher;
- a synthetic replacement for an application's exported menu.

The only project-specific product feature is the desktop fallback menu shown when KDE otherwise has no application menu to display.
