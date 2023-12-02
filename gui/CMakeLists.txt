set(KKC_CONFIG_SRCS
  main.cpp
  dictwidget.cpp
  adddictdialog.cpp
  dictmodel.cpp
  shortcutmodel.cpp
  shortcutwidget.cpp
  rulemodel.cpp
  addshortcutdialog.cpp
  )

if(NOT ENABLE_QT)
  return()
endif()

add_library(fcitx5-kkc-config
  MODULE ${KKC_CONFIG_SRCS})

add_definitions(-DQT_NO_KEYWORDS)

target_include_directories(fcitx5-kkc-config PRIVATE ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(fcitx5-kkc-config PROPERTIES
                      AUTOMOC TRUE
                      AUTOUIC TRUE
                      AUTOUIC_OPTIONS "-tr=fcitx::tr2fcitx;--include=fcitxqti18nhelper.h")
target_link_libraries(fcitx5-kkc-config
  Qt${QT_MAJOR_VERSION}::Core
  Qt${QT_MAJOR_VERSION}::Widgets
  Fcitx5Qt${QT_MAJOR_VERSION}::WidgetsAddons
  LibKKC::LibKKC PkgConfig::GObject2 PkgConfig::JSonGlib PkgConfig::Gee
  )

install(TARGETS fcitx5-kkc-config DESTINATION ${CMAKE_INSTALL_LIBDIR}/fcitx5/qt${QT_MAJOR_VERSION})
