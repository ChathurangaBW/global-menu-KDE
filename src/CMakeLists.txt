# SPDX-License-Identifier: GPL-2.0-or-later
add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.chathuranga.globalmenu\")

plasma_add_applet(org.chathuranga.globalmenu
    QML_SOURCES
        qml/main.qml
        qml/MenuDelegate.qml
    CPP_SOURCES
        globalmenuapplet.cpp
        globalmenumodel.cpp
        globalmenuproperty.cpp
        viewservicelease.cpp
        dbusmenutypes.cpp
    QML_ARGS DEPENDENCIES QtCore org.kde.plasma.core
)

target_compile_features(org.chathuranga.globalmenu PRIVATE cxx_std_20)

target_link_libraries(org.chathuranga.globalmenu PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Gui
    Qt::Quick
    Qt::Widgets
    Plasma::Plasma
    KF6::CoreAddons
    KF6::I18n
    KF6::WindowSystem
    PW::LibTaskManager
)
