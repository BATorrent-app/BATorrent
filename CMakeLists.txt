cmake_minimum_required(VERSION 3.16)
project(BATorrent VERSION 2.3.4)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg Multimedia)
find_package(LibtorrentRasterbar REQUIRED)
# Optional: QtKeychain stores credentials in the platform keyring (DPAPI on
# Windows, Keychain on macOS, kwallet/libsecret on Linux). If missing at
# build time the app falls back to QSettings — secrets stay in plaintext
# but the app still builds and runs.
find_package(Qt6Keychain QUIET)
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
    src/gui/addtorrentdialog.cpp
    src/app/secretstore.cpp
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

if(Qt6Keychain_FOUND)
    target_compile_definitions(BATorrent PRIVATE HAVE_QTKEYCHAIN)
    target_link_libraries(BATorrent PRIVATE Qt6Keychain::Qt6Keychain)
    message(STATUS "QtKeychain found — secrets will use the OS keyring")
else()
    message(STATUS "QtKeychain not found — secrets will live in QSettings (plaintext)")
endif()

# Windows: target Windows 7+ to silence Boost.Asio warnings
if(WIN32)
    target_compile_definitions(BATorrent PRIVATE _WIN32_WINNT=0x0601)
endif()

# Linux: install rules for icon and .desktop file
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS BATorrent RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Install the icon at multiple hicolor sizes. Most Linux desktops
    # request small sizes (16, 22, 32) for taskbar / window list and only
    # the matching size is used; a single 256px install forces every panel
    # to downscale at runtime, which looks washed out on GTK and KDE alike.
    # CMake's install(FILES) doesn't resize; we render the variants at
    # configure time via ImageMagick if available, otherwise we still ship
    # the 256px copy at every size (browsers will downscale, less ideal
    # but no worse than the previous setup).
    find_program(IMAGEMAGICK_CONVERT NAMES magick convert)
    set(BAT_ICON_SIZES 16 22 32 48 64 128 256 512)
    foreach(size IN LISTS BAT_ICON_SIZES)
        set(generated_icon "${CMAKE_BINARY_DIR}/icons/${size}x${size}/batorrent.png")
        if(IMAGEMAGICK_CONVERT)
            add_custom_command(
                OUTPUT "${generated_icon}"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${CMAKE_BINARY_DIR}/icons/${size}x${size}"
                COMMAND ${IMAGEMAGICK_CONVERT}
                        "${CMAKE_SOURCE_DIR}/src/images/logo1.png"
                        -resize ${size}x${size}
                        "${generated_icon}"
                DEPENDS "${CMAKE_SOURCE_DIR}/src/images/logo1.png"
                COMMENT "Generating ${size}x${size} app icon")
            list(APPEND BAT_ICON_FILES "${generated_icon}")
        else()
            # No ImageMagick — fall back to copying the 1024px source. The
            # icon theme still installs at every size; the desktop will
            # downscale on demand.
            set(generated_icon "${CMAKE_SOURCE_DIR}/src/images/logo1.png")
        endif()
        install(FILES "${generated_icon}"
                DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps"
                RENAME batorrent.png)
    endforeach()
    if(BAT_ICON_FILES)
        add_custom_target(bat_icons ALL DEPENDS ${BAT_ICON_FILES})
        add_dependencies(BATorrent bat_icons)
    endif()

    install(FILES dist/batorrent.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

# --- Test Suite ---
option(BAT_BUILD_TESTS "Build the BATorrent test suite" OFF)
if(BAT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
