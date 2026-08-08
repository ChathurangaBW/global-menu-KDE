# SPDX-License-Identifier: GPL-2.0-or-later
plasma_add_applet(org.chathuranga.globalmenu
    QML_SOURCES
        qml/configGeneral.qml
        qml/config.qml
        qml/main.qml
        qml/MenuDelegate.qml
    CPP_SOURCES
        appmenuapplet.cpp
        appmenumodel.cpp
        desktopfallback.cpp
    RESOURCES
        main.xml
    QML_ARGS DEPENDENCIES QtCore org.kde.plasma.core
)

target_compile_features(org.chathuranga.globalmenu PRIVATE cxx_std_20)
target_compile_definitions(org.chathuranga.globalmenu PRIVATE
    TRANSLATION_DOMAIN="plasma_applet_org.chathuranga.globalmenu"
)

target_link_libraries(org.chathuranga.globalmenu PRIVATE
    Qt::Core
    Qt::Widgets
    Qt::Quick
    Qt::DBus
    Plasma::Plasma
    KF6::WindowSystem
    KF6::I18n
    PW::LibTaskManager
    dbusmenuqt
)
