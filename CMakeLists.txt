cmake_minimum_required(VERSION 3.6)

# The Project Name
project(fcitx5-kkc)

find_package(ECM REQUIRED 1.0.0)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

# option(ENABLE_QT "Enable Qt for GUI configuration" On)

find_package(PkgConfig REQUIRED)
find_package(Fcitx5Core REQUIRED)
find_package(LibKKC REQUIRED)
find_package(Gettext REQUIRED)

# if (ENABLE_QT)
# find_package(Qt5 5.7 REQUIRED COMPONENTS Core Gui Widgets)
# find_package(FcitxQt5WidgetsAddons 1.1 REQUIRED)
# endif ()

if (NOT SKK_DEFAULT_PATH)
    set(SKK_DEFAULT_PATH "/usr/share/skk/")
endif ()

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-kkc\")

configure_file(config.h.in config.h
  IMMEDIATE @ONLY)

add_subdirectory(po)
add_subdirectory(src)
#add_subdirectory(gui)
add_subdirectory(data)
