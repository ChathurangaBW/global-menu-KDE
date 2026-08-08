# KDE plasma-workspace libdbusmenuqt, vendored because the upstream target is private/static.
set(libdbusmenu_SRCS
    dbusmenuimporter.cpp
    dbusmenushortcut_p.cpp
    dbusmenutypes_p.cpp
    utils.cpp
    dbusmenuimporter.h
    dbusmenushortcut_p.h
    dbusmenutypes_p.h
    utils_p.h
)

ecm_qt_declare_logging_category(libdbusmenu_SRCS
    HEADER debug.h
    IDENTIFIER DBUSMENUQT
    CATEGORY_NAME org.kde.libdbusmenuqt
    DEFAULT_SEVERITY Info
)

set_source_files_properties(com.canonical.dbusmenu.xml PROPERTIES
    NO_NAMESPACE true
    INCLUDE "dbusmenutypes_p.h"
    CLASSNAME DBusMenuInterface
)
qt_add_dbus_interface(libdbusmenu_SRCS com.canonical.dbusmenu.xml dbusmenu_interface)

add_library(dbusmenuqt STATIC ${libdbusmenu_SRCS})
set_property(TARGET dbusmenuqt PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(dbusmenuqt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(dbusmenuqt PUBLIC
    Qt::DBus
    Qt::Widgets
)
