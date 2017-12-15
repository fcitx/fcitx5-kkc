set(KKC_SOURCES
    kkc.cpp
)
add_library(kkc MODULE ${KKC_SOURCES})
target_link_libraries(kkc Fcitx5::Core Fcitx5::Config LibKKC::LibKKC)
set_target_properties(kkc PROPERTIES PREFIX "")
install(TARGETS kkc DESTINATION "${CMAKE_INSTALL_LIBDIR}/fcitx5")
fcitx5_translate_desktop_file(kkc.conf.in kkc.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kkc.conf" DESTINATION "${CMAKE_INSTALL_DATADIR}/fcitx5/inputmethod")
fcitx5_translate_desktop_file(kkc-addon.conf.in kkc-addon.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kkc-addon.conf" RENAME kkc.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

