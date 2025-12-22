cmake_minimum_required(VERSION 3.6)

# The Project Name
project(fcitx5-kkc VERSION 5.1.9)

set(REQUIRED_FCITX_VERSION 5.1.13)
find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

option(ENABLE_QT "Enable Qt for GUI configuration" On)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core ${REQUIRED_FCITX_VERSION} REQUIRED)
find_package(LibKKC REQUIRED)
find_package(Gettext REQUIRED)
pkg_check_modules(Gee IMPORTED_TARGET "gee-0.8" REQUIRED)
pkg_check_modules(JSonGlib IMPORTED_TARGET "json-glib-1.0" REQUIRED)
pkg_check_modules(GObject2 IMPORTED_TARGET "gobject-2.0" REQUIRED)

if (ENABLE_QT)
    set(QT_MAJOR_VERSION 6)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
    find_package(Fcitx5Qt${QT_MAJOR_VERSION}WidgetsAddons REQUIRED)
endif ()

if (NOT SKK_DEFAULT_PATH)
    set(SKK_DEFAULT_PATH "/usr/share/skk/")
endif ()

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-kkc\" -D_GNU_SOURCE)
fcitx5_add_i18n_definition()

configure_file(config.h.in config.h
  IMMEDIATE @ONLY)

add_subdirectory(po)
add_subdirectory(src)
add_subdirectory(gui)
add_subdirectory(data)

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Kkc.metainfo.xml.in
                              org.fcitx.Fcitx5.Addon.Kkc.metainfo.xml XML)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Kkc.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
