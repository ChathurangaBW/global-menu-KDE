# Architecture

Global Menu KDE is intentionally small: it consumes the active application's exported menu and renders only the application-menu headings in a Plasma panel.

## Data flow

```text
Active KDE application
        │
        │ exports Canonical dbusmenu
        ▼
com.canonical.AppMenu.Registrar
        │
        │ window → service/object-path mapping
        ▼
GlobalMenuModel
        │
        ├─ tracks active task with LibTaskManager
        ├─ requests dbusmenu layouts with QtDBus
        ├─ updates QAction/QMenu state
        └─ forwards menu events to the application
        │
        ▼
GlobalMenuApplet
        │
        ├─ exposes the model to QML
        ├─ positions native QMenu popups
        └─ handles heading switching/navigation
        │
        ▼
main.qml + MenuDelegate.qml
        │
        └─ File  Edit  View  Go  Tools  Settings  Help
```

## Why the applet does not own the registrar

KDE Plasma already provides `com.canonical.AppMenu.Registrar`. Competing for that well-known D-Bus name would break integration with KDE's normal global-menu stack and Qt applications.

The applet instead behaves as a consumer and uses a small `org.kde.kappmenuview` presence lease so KDE's menu infrastructure remains active while a menu view exists.

## Active application tracking

`GlobalMenuModel` uses Plasma Workspace's `LibTaskManager` to determine the active task/window. This avoids duplicating KWin/X11/Wayland-specific active-window tracking inside the applet.

When the active window changes, the model resolves the associated menu service/object path and imports the application's menu.

## dbusmenu import

The importer handles Canonical `com.canonical.dbusmenu` data with QtDBus.

Supported properties include:

- label;
- visibility;
- enabled state;
- submenu structure;
- toggle/check/radio state;
- theme icon names;
- raw icon data;
- exported keyboard shortcuts.

The model translates the exported tree into `QAction` and `QMenu` objects so popup behavior remains native to Qt/KDE.

## Incremental updates

Non-structural `ItemsPropertiesUpdated` changes update cached actions in place. This covers normal runtime changes such as:

- label updates;
- visible/enabled changes;
- icon changes;
- shortcut changes;
- toggle-state changes.

A complete `GetLayout` refresh is reserved for structural changes, unknown item IDs, or properties that alter menu hierarchy/type.

## Remote events

The applet forwards the menu lifecycle expected by dbusmenu exporters:

- `AboutToShow` before opening a menu;
- `opened` when a heading/menu opens;
- `closed` when it closes;
- `clicked` when an action is activated.

This is required for applications that lazily update menu contents or depend on lifecycle notifications.

## Stale-reply protection

D-Bus requests are asynchronous. If the active application changes while a request is in flight, an old reply must not replace the new application's menu.

The model uses a source-generation counter and ignores replies from a previous source generation.

## Hidden-state behavior

The product requirement is strict: when no compatible application menu is exported, the applet must not leave a placeholder in the panel.

The QML surface therefore:

- sets `Plasmoid.status` to `HiddenStatus`;
- hides the representation;
- explicitly drives root/representation implicit and layout dimensions to zero.

There is no fallback label or synthetic menu.

## UI layer

The QML UI is deliberately limited to the exported headings. `MenuDelegate.qml` uses KDE's `widgets/menubaritem` SVG theme element for normal/hover/pressed states and supports Alt mnemonics.

RTL layout mirroring follows KDE's current AppMenu behavior.

Out of scope by design:

- Apple/system menu;
- application launcher;
- KRunner/search;
- workspace switcher;
- clock/calendar;
- media controls;
- system tray/status controls;
- synthetic application actions.

## Source map

```text
src/
├── globalmenuapplet.cpp/.h      Plasma applet controller and popup positioning
├── globalmenumodel.cpp/.h       Active app tracking and menu model
├── globalmenuproperty.cpp       dbusmenu property helper
├── dbusmenutypes.cpp/.h         D-Bus serialization and shortcut translation
├── viewservicelease.cpp/.h      org.kde.kappmenuview lifecycle
├── metadata.json                Plasma plugin metadata
└── qml/
    ├── main.qml                 Menu strip and hidden-state layout
    └── MenuDelegate.qml         Individual menu heading appearance/interaction

tests/
├── shortcuttest.cpp             Shortcut translation unit tests
└── modeltest.cpp                Private-session fake dbusmenu integration test

scripts/
├── install-user.sh              Build/test/install for current user
├── uninstall-user.sh            Source-install cleanup
├── install-prebuilt.sh          CI artifact installer
├── uninstall-prebuilt.sh        CI artifact cleanup
├── qa.sh                        Static/full repository QA
└── smoke-plasmawindowed.sh      Headless applet load test
```

## Testing strategy

The repository intentionally tests at multiple layers:

1. Static scope and packaging assertions.
2. C++ unit tests.
3. Fake dbusmenu integration under a private D-Bus session.
4. Full Plasma 6 release compilation.
5. Staged CMake installation.
6. `plasmawindowed` plugin/QML load under Xvfb.
7. Real Plasma desktop QA before release.

See `docs/QA.md` for the full checklist.
