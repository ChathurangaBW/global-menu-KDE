# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.22)

project(global-menu-kde VERSION 1.0.0 LANGUAGES CXX)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")
set(PLASMA_MIN_VERSION "6.0.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs6)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)
include(CTest)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus Gui Quick Widgets)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Config CoreAddons I18n WindowSystem)
find_package(Plasma ${PLASMA_MIN_VERSION} REQUIRED)
find_package(LibTaskManager REQUIRED CONFIG)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)
endif()

add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# Native distro packaging. Packages are built and smoke-tested on the distro
# they target; they are not advertised as ABI-portable across Plasma releases.
set(CPACK_PACKAGE_NAME "global-menu-kde")
set(CPACK_PACKAGE_VENDOR "ChathurangaBW")
set(CPACK_PACKAGE_CONTACT "ChathurangaBW")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KDE Plasma 6 global menu applet with desktop fallback")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ChathurangaBW/global-menu-KDE")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SECTION "kde")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "plasma-workspace (>= 4:6.0), libplasma6 (>= 6.0)")

set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "User Interface/Desktops")
set(CPACK_RPM_PACKAGE_REQUIRES "plasma-workspace >= 6.0, libplasma >= 6.0")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    "/usr/lib64/qt6"
    "/usr/lib64/qt6/plugins"
    "/usr/lib64/qt6/plugins/plasma"
    "/usr/lib64/qt6/plugins/plasma/applets"
    "/usr/lib/qt6"
    "/usr/lib/qt6/plugins"
    "/usr/lib/qt6/plugins/plasma"
    "/usr/lib/qt6/plugins/plasma/applets"
)

include(CPack)
