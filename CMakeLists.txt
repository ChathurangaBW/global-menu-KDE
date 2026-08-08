# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.22)

project(global-menu-kde VERSION 1.0.1 LANGUAGES CXX)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")
set(PLASMA_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs6)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(CTest)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus Gui Quick Widgets)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config I18n WindowSystem)
find_package(Plasma ${PLASMA_MIN_VERSION} REQUIRED)
find_package(LibTaskManager REQUIRED CONFIG)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)
endif()

add_subdirectory(third_party/libdbusmenuqt)
add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# Native distribution packages are generated from the exact tested build tree.
set(CPACK_PACKAGE_NAME "global-menu-kde")
set(CPACK_PACKAGE_VENDOR "ChathurangaBW")
set(CPACK_PACKAGE_CONTACT "ChathurangaBW <ChathurangaBW@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Native KDE Plasma 6 Global Menu applet with desktop fallback")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ChathurangaBW/global-menu-KDE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "global-menu-kde")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_STRIP_FILES TRUE)

set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_BINARY_DIR}/cpack-package-description.txt")
file(WRITE "${CPACK_PACKAGE_DESCRIPTION_FILE}" [=[Native KDE Plasma 6 Global Menu applet.
Shows a desktop fallback when no application exports a menu.
Switches back to the real application menu when one becomes available.
]=])

set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION [=[This Plasma applet exports the active application's global menu into a
 configurable global menu bar. When no application exports a menu, a desktop
 fallback with the usual File/Edit/View/Go/Tools/Settings/Help headings is
 shown instead.]=])
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "User Interface/Desktops")
set(CPACK_RPM_PACKAGE_DESCRIPTION [=[Native KDE Plasma 6 Global Menu applet.
Shows a desktop fallback when no application exports a menu.
Switches back to the real application menu when one becomes available.]=])
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

install(FILES README.md CHANGELOG.md LICENSE
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}")
install(FILES packaging/debian/copyright
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}")

find_program(GZIP_EXECUTABLE gzip)
if(GZIP_EXECUTABLE)
    set(DEBIAN_CHANGELOG_GZ "${CMAKE_BINARY_DIR}/changelog.gz")
    execute_process(
        COMMAND "${GZIP_EXECUTABLE}" -9 -n -c "${CMAKE_SOURCE_DIR}/packaging/debian/changelog"
        OUTPUT_FILE "${DEBIAN_CHANGELOG_GZ}"
        RESULT_VARIABLE GZIP_RESULT)
    if(NOT GZIP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to compress packaging/debian/changelog")
    endif()
    install(FILES "${DEBIAN_CHANGELOG_GZ}"
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CPACK_PACKAGE_NAME}"
        RENAME changelog.gz)
endif()

include(CPack)
