project(plasma-birthdays-namedays)

include_directories(${KDE4_INCLUDE_DIR} ../../)

set(BirthdaysAndNamedaysApplet_SRC
    bn_applet.cpp
    bn_listentry.cpp
)

kde4_add_ui_files(BirthdaysAndNamedaysApplet_SRC bn_applet_config.ui)
kde4_add_plugin(plasma_applet_birthdays_namedays ${BirthdaysAndNamedaysApplet_SRC})
target_link_libraries(plasma_applet_birthdays_namedays ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS plasma_applet_birthdays_namedays
    DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-birthdays-namedays.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
