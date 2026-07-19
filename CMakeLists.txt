cmake_minimum_required(VERSION 3.16)
project(BATorrent VERSION 4.7.0)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg Multimedia Quick QuickWidgets QuickControls2)

# libtorrent: link the system package by default, or build our vendored fork from
# source (third_party/libtorrent) when BAT_LIBTORRENT_SOURCE=ON. The source path
# is what lets us patch the engine; both expose the same `torrent-rasterbar` target.
option(BAT_LIBTORRENT_SOURCE "Build libtorrent from the vendored source (fork)" OFF)
if(BAT_LIBTORRENT_SOURCE)
    set(build_tests OFF CACHE BOOL "" FORCE)
    set(build_examples OFF CACHE BOOL "" FORCE)
    set(build_tools OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/libtorrent EXCLUDE_FROM_ALL)
    set(BAT_TORRENT_LIB torrent-rasterbar)
    add_compile_definitions(BAT_LIBTORRENT_FORK)
    message(STATUS "[BAT] libtorrent: building vendored fork from source")
else()
    find_package(LibtorrentRasterbar REQUIRED)
    set(BAT_TORRENT_LIB LibtorrentRasterbar::torrent-rasterbar)
endif()
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
    src/torrent/iengine.h        # header-only QObject — listed so AUTOMOC builds its metaobject
    src/torrent/sessionmanager.cpp
    src/torrent/sessionmanager_streaming.cpp
    src/torrent/sessionmanager_config.cpp
    src/torrent/sessionmanager_persistence.cpp
    src/torrent/sessionmanager_categories.cpp
    src/torrent/sessionmanager_pertorrent.cpp
    src/torrent/sessionmanager_alerts.cpp
    src/torrent/sessionmanager_info.cpp
    src/torrent/sessionmanager_network.cpp
    src/torrent/sessionmanager_security.cpp
    src/torrent/ipblocklist.cpp
    src/torrent/proxycontroller.cpp
    src/ipc/enginehost.cpp
    src/ipc/ipcengine.cpp
    src/services/platform/translator.cpp
    src/services/platform/statshistory.cpp
    src/services/platform/soundplayer.cpp
    src/services/subtitles/subtitleparser.cpp
    src/services/subtitles/subtitlesearch.cpp
    src/services/integrations/idebridprovider.cpp
    src/services/integrations/realdebrid.cpp
    src/services/integrations/torbox.cpp
    src/services/integrations/debridmanager.cpp
    src/services/security/suspiciousscan.cpp
    src/services/security/archivescan.cpp
    src/services/security/archiveextractor.cpp
    src/services/metadata/releasepick.cpp
    src/services/metadata/searchranker.cpp
    src/services/integrations/debridpick.cpp
    src/services/integrations/installerprofile.cpp
    src/services/security/defender.cpp
    src/services/platform/logger.cpp
    src/services/security/crashhandler.cpp
    src/services/integrations/notifier.cpp
    src/services/platform/qrcodegen.cpp
    src/services/integrations/discordrpc.cpp
    src/services/security/passwordhash.cpp
    src/services/security/secretstore.cpp
    src/services/integrations/updater.cpp
    src/services/discovery/addonmanager.cpp
    src/services/integrations/rssmanager.cpp
    src/services/integrations/geoip.cpp
    src/services/integrations/geoipdb.cpp
    src/services/integrations/geoipprovider.cpp
    src/services/metadata/audiomode.cpp
    src/services/metadata/episodegroup.cpp
    src/services/metadata/nameparser.cpp
    src/services/metadata/metadataresolver.cpp
    src/services/metadata/mkvchapters.cpp
    src/services/downloads/httpdownload.cpp
    src/services/downloads/httpdownloadmanager.cpp
    src/services/downloads/httpmergeengine.cpp
    src/services/discovery/discoveryservice.cpp
    src/services/discovery/gamesourcemanager.cpp
    src/bridges/qmlposterbridge.cpp
    src/bridges/qmlpostermodel.cpp
    src/bridges/qmlsessionbridge.cpp
    src/bridges/qmlsessionbridge_games.cpp
    src/bridges/qmlsessionbridge_library.cpp
    src/bridges/qmlsessionbridge_playback.cpp
    src/bridges/qmlsessionbridge_selection.cpp
    src/bridges/qmlsessionbridge_disk.cpp
    src/bridges/qmlthemebridge.cpp
    src/bridges/qmlsearchbridge.cpp
    src/bridges/qmlsettingsbridge.cpp
    # bridge classes implemented in qmlposterbridge.cpp — listed so AUTOMOC builds their metaobjects
    src/bridges/qmlrssbridge.h
    src/bridges/qmladdonbridge.h
    src/bridges/qmlpairingbridge.h
    src/bridges/qmlsubtitlebridge.h
    src/bridges/qmllogbridge.h
    src/bridges/qmli18nbridge.h
    src/bridges/qmlnotificationbridge.h
    src/bridges/discordrpcbridge.h
    src/bridges/qmlupdaterbridge.h
    src/webui/webserver.cpp
    src/webui/streamserver.cpp
    ${RESOURCES}
    ${WIN_RC}
    ${MACOSX_APP_ICON}
)
target_compile_definitions(BATorrent PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_include_directories(BATorrent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(DEFINED ENV{BAT_TMDB_KEY})
    target_compile_definitions(BATorrent PRIVATE BAT_TMDB_KEY="$ENV{BAT_TMDB_KEY}")
endif()
if(DEFINED ENV{BAT_IGDB_ID})
    target_compile_definitions(BATorrent PRIVATE BAT_IGDB_CLIENT_ID="$ENV{BAT_IGDB_ID}")
endif()
if(DEFINED ENV{BAT_IGDB_SECRET})
    target_compile_definitions(BATorrent PRIVATE BAT_IGDB_CLIENT_SECRET="$ENV{BAT_IGDB_SECRET}")
endif()
if(DEFINED ENV{BAT_OS_KEY})
    target_compile_definitions(BATorrent PRIVATE BAT_OS_KEY="$ENV{BAT_OS_KEY}")
endif()
if(DEFINED ENV{BAT_SUBDL_KEY})
    target_compile_definitions(BATorrent PRIVATE BAT_SUBDL_KEY="$ENV{BAT_SUBDL_KEY}")
endif()

# Sentry crash reporting (opt-in). Links sentry-native (Crashpad); the DSN comes
# from .env (BAT_SENTRY_DSN) like the metadata keys, the Crashpad handler path
# from BAT_SENTRY_HANDLER. Default OFF so the normal build has no extra dep; the
# release build passes -DBAT_SENTRY=ON. See internal/DEBUGGING.md.
option(BAT_SENTRY "Enable Sentry crash reporting (needs sentry-native)" OFF)
if(BAT_SENTRY)
    find_package(sentry CONFIG REQUIRED)
    target_link_libraries(BATorrent PRIVATE sentry::sentry)
    target_compile_definitions(BATorrent PRIVATE BAT_HAVE_SENTRY)
    if(DEFINED ENV{BAT_SENTRY_DSN})
        target_compile_definitions(BATorrent PRIVATE BAT_SENTRY_DSN="$ENV{BAT_SENTRY_DSN}")
    endif()
    if(DEFINED ENV{BAT_SENTRY_HANDLER})
        target_compile_definitions(BATorrent PRIVATE BAT_SENTRY_HANDLER="$ENV{BAT_SENTRY_HANDLER}")
        # Ship the Crashpad handler next to the executable so the packaged app
        # finds it at runtime (initSentry prefers applicationDirPath/crashpad_handler).
        if(EXISTS "$ENV{BAT_SENTRY_HANDLER}")
            add_custom_command(TARGET BATorrent POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$ENV{BAT_SENTRY_HANDLER}" "$<TARGET_FILE_DIR:BATorrent>/"
                VERBATIM)
        endif()
    endif()
endif()

# Microsoft Store builds must not include their own auto-updater (Store
# handles updates). Pass -DBAT_STORE_BUILD=ON in the store.yml workflow.
option(BAT_STORE_BUILD "Disable auto-updater for Microsoft Store submission" OFF)
if(BAT_STORE_BUILD)
    target_compile_definitions(BATorrent PRIVATE BAT_STORE_BUILD)
endif()
find_package(ZLIB REQUIRED)   # subtitle .zip extraction (SubDL)
target_link_libraries(BATorrent PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg Qt6::Multimedia Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2 ZLIB::ZLIB ${BAT_TORRENT_LIB})

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
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
    )
    # Dev-only: homebrew Qt ships multimedia without the ffmpeg backend, so MKV/
    # AC3 won't play in dev builds. If the locally built plugin exists (recipe:
    # internal/build-qt-ffmpeg-plugin.sh), bundle it — Qt searches the bundle's
    # Contents/PlugIns on macOS, so any launch method picks it up. Absent on CI.
    if(EXISTS ${CMAKE_SOURCE_DIR}/dev-qt-plugins/multimedia/libffmpegmediaplugin.dylib)
        add_custom_command(TARGET BATorrent POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/dev-qt-plugins/multimedia/libffmpegmediaplugin.dylib
                $<TARGET_BUNDLE_DIR:BATorrent>/Contents/PlugIns/multimedia/libffmpegmediaplugin.dylib)
    endif()
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
    # psapi: GetProcessMemoryInfo for the memory guard
    target_link_libraries(BATorrent PRIVATE shell32 ole32 dwmapi psapi dbghelp)

    # Emit a PDB even in Release so Store / Partner Center crash dumps symbolicate.
    # Without symbols every Store crash is reported as "unknown" (we had 314/322
    # like that). /OPT:REF,ICF restore the size optimisations that /DEBUG disables,
    # so the shipped binary is unchanged in behaviour/size — only a .pdb is added.
    if(MSVC)
        target_compile_options(BATorrent PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Zi>)
        target_link_options(BATorrent PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:/DEBUG>
            $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
            $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>)
    endif()
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

# --- Controlled A/B download benchmark (engine/config delta) ---
option(BAT_BENCH "Build the libtorrent A/B benchmark harness" OFF)
if(BAT_BENCH)
    add_executable(bat_bench benchmark/bench.cpp)
    target_compile_features(bat_bench PRIVATE cxx_std_17)
    target_link_libraries(bat_bench PRIVATE ${BAT_TORRENT_LIB})
endif()
