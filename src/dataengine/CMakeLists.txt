project(plasma-engine-birthdays-namedays)

# Find the required Libaries
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES} )

add_subdirectory(namedaydefs)

# We add our source code here
set(bn_engine_SRC bn_dataengine.cpp)

# Now make sure all files get to the right place
kde4_add_plugin(plasma_engine_birthdays_namedays ${bn_engine_SRC})

target_link_libraries(plasma_engine_birthdays_namedays ${KDE4_PLASMA_LIBS} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS})

install(TARGETS plasma_engine_birthdays_namedays
    DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-dataengine-birthdays-namedays.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
