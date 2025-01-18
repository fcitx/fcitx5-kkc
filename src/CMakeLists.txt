set(KKC_SOURCES
    kkc.cpp
)
add_fcitx5_addon(kkc ${KKC_SOURCES})
target_link_libraries(kkc Fcitx5::Core Fcitx5::Config LibKKC::LibKKC PkgConfig::GObject2 PkgConfig::JSonGlib PkgConfig::Gee)
target_include_directories(kkc PRIVATE ${PROJECT_BINARY_DIR})
set_target_properties(kkc PROPERTIES PREFIX "")
install(TARGETS kkc DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
fcitx5_translate_desktop_file(kkc.conf.in kkc.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kkc.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod")
configure_file(kkc-addon.conf.in.in kkc-addon.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/kkc-addon.conf.in" kkc-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kkc-addon.conf" RENAME kkc.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dictionary_list.in ${CMAKE_CURRENT_BINARY_DIR}/dictionary_list @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dictionary_list rule DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/kkc")
