project(plasma-kbirthday)

include_directories(${KDE4_INCLUDE_DIR} ../../)

set(KBirthdayApplet_SRC
    kbirthdayapplet.cpp
    blistentry.cpp
    kbirthdaydialog.cpp
    kbirthdayview.cpp
)

kde4_add_ui_files(KBirthdayApplet_SRC kbirthdayapplet_config.ui)
kde4_add_plugin(plasma_applet_kbirthdayapplet ${KBirthdayApplet_SRC})
target_link_libraries(plasma_applet_kbirthdayapplet ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS plasma_applet_kbirthdayapplet
    DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-kbirthday.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})

install(FILES birthdaycake.svg
    DESTINATION ${DATA_INSTALL_DIR}/desktoptheme/default/widgets/)