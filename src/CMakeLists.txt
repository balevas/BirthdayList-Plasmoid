project(plasma-birthdaylist)

include_directories(${KDE4_INCLUDE_DIR})

set(BirthdayListApplet_SRC 
        birthdaylist_aboutdata.cpp
        birthdaylist_applet.cpp
        birthdaylist_confighelper.cpp 
        birthdaylist_model.cpp
        birthdaylist_modelentry.cpp
        birthdaylist_source_akonadi.cpp
        birthdaylist_source_collections.cpp
        birthdaylist_source_contacts.cpp
#        birthdaylist_source_kabc.cpp
        birthdaylist_view.cpp 
)

kde4_add_ui_files(BirthdayListApplet_SRC 
        birthdaylist_config_colors.ui
        birthdaylist_config_contacts.ui 
        birthdaylist_config_events.ui 
        birthdaylist_config_table.ui 
)

kde4_add_plugin(plasma_applet_birthdaylist ${BirthdayListApplet_SRC})

target_link_libraries(plasma_applet_birthdaylist ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KABC_LIBS} ${KDE4_AKONADI_LIBS})

install(TARGETS plasma_applet_birthdaylist
    DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-applet-birthdaylist.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
