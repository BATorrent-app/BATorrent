cmake_minimum_required(VERSION 3.16)
project(BATorrent VERSION 1.0)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(LibtorrentRasterbar REQUIRED)
set(CMAKE_AUTOMOC ON)
qt_add_resources(RESOURCES src/resources.qrc)

# Windows: embed icon in exe via .rc file
if(WIN32)
    set(WIN_RC src/app.rc)
endif()

add_executable(BATorrent WIN32
    src/main.cpp
    src/core/sessionmanager.cpp
    src/core/translator.cpp
    src/gui/mainwindow.cpp
    src/gui/torrentmodel.cpp
    src/gui/torrentfilter.cpp
    src/gui/progressdelegate.cpp
    src/gui/detailspanel.cpp
    src/gui/settingsdialog.cpp
    src/gui/welcomedialog.cpp
    src/gui/createtorrentdialog.cpp
    src/gui/speedgraph.cpp
    src/gui/batwidget.cpp
    src/gui/splashwidget.cpp
    src/gui/thememanager.cpp
    ${RESOURCES}
    ${WIN_RC}
)
target_link_libraries(BATorrent PRIVATE Qt6::Widgets LibtorrentRasterbar::torrent-rasterbar)

# Linux: install rules for icon and .desktop file
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS BATorrent RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES src/images/logo1.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps RENAME batorrent.png)
    install(FILES dist/batorrent.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()
