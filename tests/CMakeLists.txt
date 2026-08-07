# SPDX-License-Identifier: GPL-2.0-or-later

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
