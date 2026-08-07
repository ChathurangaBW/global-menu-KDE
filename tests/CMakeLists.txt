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
set_tests_properties(dbusmenushortcut_test PROPERTIES
    LABELS "unit"
    TIMEOUT 15
)

find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session REQUIRED)

qt_add_executable(dbusmenumodel_test
    modeltest.cpp
    ../src/globalmenumodel.cpp
    ../src/globalmenumodel.h
    ../src/globalmenuproperty.cpp
    ../src/displaymenumodel.cpp
    ../src/displaymenumodel.h
    ../src/dbusmenutypes.cpp
    ../src/dbusmenutypes.h
)

target_include_directories(dbusmenumodel_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(dbusmenumodel_test PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::Widgets
    Qt::Test
    KF6::I18n
    PW::LibTaskManager
)

add_test(
    NAME dbusmenumodel_test
    COMMAND "${DBUS_RUN_SESSION_EXECUTABLE}" -- $<TARGET_FILE:dbusmenumodel_test>
)
set_tests_properties(dbusmenumodel_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    LABELS "integration"
    TIMEOUT 30
)
