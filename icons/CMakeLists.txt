project(plasma-birthdays-namedays-icons)

find_package(KDE4 REQUIRED)
add_definitions (${KDE4_DEFINITIONS})

kde4_install_icons( ${ICON_INSTALL_DIR} )