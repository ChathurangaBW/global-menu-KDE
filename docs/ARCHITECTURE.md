# Architecture

Global Menu KDE is a Plasma 6 **panel applet** with two display sources:

1. a persistent desktop fallback menu;
2. the active application's real exported menu when one is available.

The applet itself does not create a panel. It renders compact menu items inside the existing Plasma containment.

## High-level data flow

```text
                         ┌─────────────────────────────┐
                         │      DisplayMenuModel       │
                         │                             │
                         │ application menu usable?    │
                         │     yes          no         │
                         └──────┬───────────┬──────────┘
                                │           │
                                │           └── desktop fallback
                                │               File / Edit / View /
                                │               Go / Tools / Settings / Help
                                ▼
Active application ──► GlobalMenuModel
        │                  │
        │                  ├─ LibTaskManager active-window tracking
        │                  ├─ QtDBus dbusmenu import
        │                  ├─ QAction/QMenu state
        │                  └─ remote application events
        │
        └─ KDE com.canonical.AppMenu.Registrar
                                │
                                ▼
                         GlobalMenuApplet
                                │
                                ├─ popup positioning
                                ├─ heading switching
                                └─ keyboard navigation
                                │
                                ▼
                         main.qml / MenuDelegate.qml
                                │
                                ▼
                         existing Plasma panel
```

## `DisplayMenuModel`

`DisplayMenuModel` is the presentation switch.

When `GlobalMenuModel::menuAvailable()` is true and the application model has rows, it mirrors the real application menu roles.

When no usable application menu exists, it supplies seven local top-level actions:

```text
File   Edit   View   Go   Tools   Settings   Help
```

Fallback actions are desktop-level Plasma operations:

- filesystem locations via `QDesktopServices`;
- Klipper clipboard history through session D-Bus;
- KWin Show Desktop/Restore Windows through session D-Bus;
- KRunner, Konsole, Plasma System Monitor, System Settings, and KDE Help Center through their installed executables.

The display model resets when application menu availability changes, so switching from the desktop to Dolphin/Kate is automatic.

## `GlobalMenuModel`

`GlobalMenuModel` remains responsible only for real application menus.

It uses Plasma Workspace `LibTaskManager` to determine the active task and obtain the application-menu service/object path exported through KDE's normal global-menu stack.

## Why the applet does not own the registrar

KDE Plasma already provides `com.canonical.AppMenu.Registrar`. The applet must not compete for that well-known name.

Instead it consumes KDE's window → menu service/object-path mapping and keeps KDE's menu infrastructure active through a reference-counted `org.kde.kappmenuview` presence lease.

## dbusmenu import

Real application menus use Canonical `com.canonical.dbusmenu` over QtDBus.

Supported state includes:

- labels;
- visibility/enabled state;
- submenu hierarchy;
- check/radio state;
- theme icons and raw icon data;
- exported keyboard shortcuts.

The imported hierarchy is represented by native `QAction` and `QMenu` objects.

## Incremental updates

Non-structural `ItemsPropertiesUpdated` changes mutate cached actions in place, including:

- label;
- visible/enabled state;
- icon;
- shortcut;
- toggle state.

Unknown IDs and structural changes fall back to a fresh `GetLayout`.

## Remote events

For real application menus the applet forwards:

- `AboutToShow`;
- `opened`;
- `closed`;
- `clicked`.

Desktop fallback menus are local and therefore do not emit dbusmenu lifecycle events.

## Stale-reply protection

D-Bus menu requests are asynchronous. `GlobalMenuModel` tracks a source generation and ignores replies that belong to the previously active application.

## Existing-panel UI contract

The QML layer must not resemble a second panel.

It therefore:

- sets `Plasmoid.backgroundHints` to `NoBackground`;
- does **not** request `Plasmoid.CanFillArea`;
- sizes the root to the menu content;
- disables horizontal fill on the menu strip;
- uses KDE's `widgets/menubaritem` only for individual item hover/pressed states.

The actual panel background comes from the user's existing Plasma panel containment.

## Desktop fallback lifecycle

```text
No exported menu
      │
      ▼
Desktop fallback visible
      │
      │ Dolphin/Kate exports dbusmenu
      ▼
Real app menu visible
      │
      │ app closes / export disappears
      ▼
Desktop fallback visible again
```

Unlike the earlier incorrect implementation, there is no `HiddenStatus`/zero-width idle state.

## Source map

```text
src/
├── globalmenuapplet.cpp/.h      Plasma applet controller and popup positioning
├── globalmenumodel.cpp/.h       Real application menu importer
├── displaymenumodel.cpp/.h      Desktop fallback + application takeover switch
├── globalmenuproperty.cpp       dbusmenu property helper
├── dbusmenutypes.cpp/.h         D-Bus serialization and shortcuts
├── viewservicelease.cpp/.h      org.kde.kappmenuview lifecycle
├── metadata.json                Plasma metadata
└── qml/
    ├── main.qml                 compact existing-panel menu surface
    └── MenuDelegate.qml         menu heading appearance/interaction

tests/
├── shortcuttest.cpp
└── modeltest.cpp                fallback + fake dbusmenu integration
```

## Testing strategy

1. Static product-contract checks.
2. Shortcut unit tests.
3. Private-session fake dbusmenu integration.
4. Desktop fallback → application takeover → fallback assertions.
5. Plasma 6 release compilation/linking.
6. Staged installation.
7. Headless `plasmawindowed` load.
8. Real panel QA on Wayland/X11 before release.

See [`QA.md`](QA.md).
