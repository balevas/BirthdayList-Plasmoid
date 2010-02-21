project(plasma-kabc)

include_directories(${KDE4_INCLUDE_DIR} ../../)

set(kabc_engine_SRC kabcengine.cpp)

kde4_add_plugin(plasma_engine_kabc ${kabc_engine_SRC})
target_link_libraries(plasma_engine_kabc ${KDE4_PLASMA_LIBS} ${KDE4_KDECORE_LIBS} ${KDE4_KABC_LIBS})

install(TARGETS plasma_engine_kabc
    DESTINATION ${PLUGIN_INSTALL_DIR})

install(FILES plasma-dataengine-kabc.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
