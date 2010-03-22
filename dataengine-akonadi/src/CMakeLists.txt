project(plasma-engine-birthdaylist_akonadi)

# Find the required Libaries
find_package(KDE4 REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")
include(KDE4Defaults)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES} )

# We add our source code here
set(birthdaylist_akonadi_engine_SRC birthdaylist_akonadi_dataengine.cpp)

# Now make sure all files get to the right place
kde4_add_plugin(plasma_engine_birthdaylist_akonadi ${birthdaylist_akonadi_engine_SRC})

target_link_libraries(plasma_engine_birthdaylist_akonadi ${KDE4_PLASMA_LIBS} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS} ${KDE4_AKONADI_LIBS})

install(TARGETS plasma_engine_birthdaylist_akonadi
    DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-dataengine-birthdaylist_akonadi.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
