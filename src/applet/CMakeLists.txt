project(plasma-birthdaylist)

include_directories(${KDE4_INCLUDE_DIR} ../../)

set(BirthdayListApplet_SRC
    birthdaylist_applet.cpp
    birthdaylistentry.cpp
)

kde4_add_ui_files(BirthdayListApplet_SRC birthdaylist_datasource_config.ui birthdaylist_contents_config.ui birthdaylist_appearance_config.ui)
kde4_add_plugin(plasma_applet_birthdaylist ${BirthdayListApplet_SRC})
target_link_libraries(plasma_applet_birthdaylist ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS plasma_applet_birthdaylist
    DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-birthdaylist.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
