# SPDX-License-Identifier: GPL-2.0-or-later
add_compile_definitions(TRANSLATION_DOMAIN="plasma_applet_org.chathuranga.globalmenu")
add_test(NAME installer_preflight_test COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/installertest.sh)

add_executable(desktopfallback_test
    desktopfallbacktest.cpp
    ${CMAKE_SOURCE_DIR}/src/desktopfallback.cpp
)

target_compile_features(desktopfallback_test PRIVATE cxx_std_20)
target_include_directories(desktopfallback_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(desktopfallback_test PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::Test
    Qt::Widgets
    KF6::I18n
)

add_test(NAME desktopfallback_test COMMAND desktopfallback_test)
set_tests_properties(desktopfallback_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(appmenumodel_test
    appmenumodeltest.cpp
    ${CMAKE_SOURCE_DIR}/src/appmenumodel.cpp
    ${CMAKE_SOURCE_DIR}/src/desktopfallback.cpp
)

target_compile_features(appmenumodel_test PRIVATE cxx_std_20)
target_include_directories(appmenumodel_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/libdbusmenuqt
)
target_link_libraries(appmenumodel_test PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::Test
    Qt::Widgets
    Plasma::Plasma
    KF6::I18n
    KF6::WindowSystem
    PW::LibTaskManager
    dbusmenuqt
)

add_test(NAME appmenumodel_test
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run-dbus-test.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/dbus-test.conf
            $<TARGET_FILE:appmenumodel_test>
)
set_tests_properties(appmenumodel_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 30
)

add_test(NAME translation_catalog_test
    COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} --check
            -o ${CMAKE_CURRENT_BINARY_DIR}/translation-check.mo
            ${CMAKE_SOURCE_DIR}/po/fr/plasma_applet_org.chathuranga.globalmenu.po
)

add_executable(appmenuapplet_test
    appmenuapplettest.cpp
    ${CMAKE_SOURCE_DIR}/src/appmenuapplet.cpp
)
target_compile_features(appmenuapplet_test PRIVATE cxx_std_20)
target_include_directories(appmenuapplet_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(appmenuapplet_test PRIVATE METADATA_FILE="${CMAKE_SOURCE_DIR}/src/metadata.json")
target_link_libraries(appmenuapplet_test PRIVATE
    Qt::Core Qt::DBus Qt::Gui Qt::Quick Qt::Widgets Qt::Test
    Plasma::Plasma KF6::WindowSystem PW::LibTaskManager)
add_test(NAME appmenuapplet_test
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run-dbus-test.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/dbus-test.conf $<TARGET_FILE:appmenuapplet_test>)
set_tests_properties(appmenuapplet_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;LANGUAGE=en_US;LC_ALL=C.UTF-8"
    TIMEOUT 30)
