# SPDX-License-Identifier: GPL-2.0-or-later

qt_add_executable(dbusmenushortcut_test
    shortcuttest.cpp
    ../src/dbusmenutypes.cpp
)

target_include_directories(dbusmenushortcut_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(dbusmenushortcut_test PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::Test
)

add_test(NAME dbusmenushortcut_test COMMAND dbusmenushortcut_test)
