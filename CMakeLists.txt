cmake_minimum_required(VERSION 3.16)
project(BATorrent VERSION 2.2.0)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg Multimedia)
find_package(LibtorrentRasterbar REQUIRED)
set(CMAKE_AUTOMOC ON)
qt_add_resources(RESOURCES src/resources.qrc src/webui/webuiresources.qrc)

# Windows: embed icon in exe via .rc file
if(WIN32)
    set(WIN_RC src/app.rc)
endif()

add_executable(BATorrent WIN32
    src/main.cpp
    src/torrent/sessionmanager.cpp
    src/app/translator.cpp
    src/gui/mainwindow.cpp
    src/gui/torrentmodel.cpp
    src/gui/torrentfilter.cpp
    src/gui/progressdelegate.cpp
    src/gui/detailspanel.cpp
    src/gui/piecemapwidget.cpp
    src/gui/settingsdialog.cpp
    src/gui/welcomedialog.cpp
    src/gui/createtorrentdialog.cpp
    src/gui/speedgraph.cpp
    src/gui/batwidget.cpp
    src/gui/splashwidget.cpp
    src/app/updater.cpp
    src/app/addonmanager.cpp
    src/gui/addondialog.cpp
    src/gui/searchdialog.cpp
    src/gui/rssdialog.cpp
    src/app/rssmanager.cpp
    src/app/geoip.cpp
    src/gui/thememanager.cpp
    src/gui/releasenotesdialog.cpp
    src/gui/speedtestdialog.cpp
    src/gui/statisticsdialog.cpp
    src/gui/shortcutsdialog.cpp
    src/webui/webserver.cpp
    ${RESOURCES}
    ${WIN_RC}
)
target_compile_definitions(BATorrent PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(BATorrent PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg Qt6::Multimedia LibtorrentRasterbar::torrent-rasterbar)

# Windows: target Windows 7+ to silence Boost.Asio warnings
if(WIN32)
    target_compile_definitions(BATorrent PRIVATE _WIN32_WINNT=0x0601)
endif()

# Linux: install rules for icon and .desktop file
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS BATorrent RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES src/images/logo1.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME batorrent.png)
    install(FILES dist/batorrent.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

# --- Test Suite ---
option(BAT_BUILD_TESTS "Build the BATorrent test suite" OFF)
if(BAT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
