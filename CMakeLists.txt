cmake_minimum_required(VERSION 3.16)
project(BATorrent VERSION 3.0.2)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg Multimedia Quick QuickWidgets QuickControls2)
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

# macOS: bundle the app icon (black bg + white logo) into Contents/Resources
set(MACOSX_APP_ICON "")
if(APPLE)
    set(MACOSX_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/src/images/appicon.icns")
    set_source_files_properties("${MACOSX_APP_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_executable(BATorrent WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/torrent/sessionmanager.cpp
    src/app/translator.cpp
    src/app/logger.cpp
    src/app/notifier.cpp
    src/app/qrcodegen.cpp
    src/app/discordrpc.cpp
    src/app/secretstore.cpp
    src/app/updater.cpp
    src/app/addonmanager.cpp
    src/app/rssmanager.cpp
    src/app/geoip.cpp
    src/app/nameparser.cpp
    src/app/metadataresolver.cpp
    src/gui/qmlposterbridge.cpp
    src/webui/webserver.cpp
    ${RESOURCES}
    ${WIN_RC}
    ${MACOSX_APP_ICON}
)
target_compile_definitions(BATorrent PRIVATE APP_VERSION="${PROJECT_VERSION}")

if(DEFINED ENV{BAT_TMDB_KEY})
    target_compile_definitions(BATorrent PRIVATE BAT_TMDB_KEY="$ENV{BAT_TMDB_KEY}")
endif()
if(DEFINED ENV{BAT_IGDB_ID})
    target_compile_definitions(BATorrent PRIVATE BAT_IGDB_CLIENT_ID="$ENV{BAT_IGDB_ID}")
endif()
if(DEFINED ENV{BAT_IGDB_SECRET})
    target_compile_definitions(BATorrent PRIVATE BAT_IGDB_CLIENT_SECRET="$ENV{BAT_IGDB_SECRET}")
endif()

# Microsoft Store builds must not include their own auto-updater (Store
# handles updates). Pass -DBAT_STORE_BUILD=ON in the store.yml workflow.
option(BAT_STORE_BUILD "Disable auto-updater for Microsoft Store submission" OFF)
if(BAT_STORE_BUILD)
    target_compile_definitions(BATorrent PRIVATE BAT_STORE_BUILD)
endif()
target_link_libraries(BATorrent PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg Qt6::Multimedia Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2 LibtorrentRasterbar::torrent-rasterbar)

# macOS: bundle metadata. Without these keys, NSStatusItem (the tray icon)
# can fail to allocate a menu-bar slot — Qt reports the item as visible but
# height 0, off-screen.
if(APPLE)
    set_target_properties(BATorrent PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "BATorrent"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.batorrent.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_INFO_STRING "BATorrent ${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "© 2024-2026 Mateus Cruz"
        MACOSX_BUNDLE_ICON_FILE "appicon"
    )
endif()

if(Qt6Keychain_FOUND)
    target_compile_definitions(BATorrent PRIVATE HAVE_QTKEYCHAIN)
    target_link_libraries(BATorrent PRIVATE Qt6Keychain::Qt6Keychain)
    message(STATUS "QtKeychain found — secrets will use the OS keyring")
else()
    message(STATUS "QtKeychain not found — secrets will live in QSettings (plaintext)")
endif()

# Windows: target Windows 7+ to silence Boost.Asio warnings
if(WIN32)
    # NOMINMAX: <shlobj.h>/<windows.h> otherwise define max()/min() macros that
    # break libtorrent headers (aligned_union.hpp uses std::max).
    target_compile_definitions(BATorrent PRIVATE _WIN32_WINNT=0x0601 NOMINMAX)
    # shell32/ole32: SHOpenFolderAndSelectItems for "reveal in Explorer"
    target_link_libraries(BATorrent PRIVATE shell32 ole32)
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
