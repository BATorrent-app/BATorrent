cmake_minimum_required(VERSION 3.16)
project(BATorrent VERSION 4.9.0)
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
    if(MSVC)
        # MSVC Release ships no debug info → no torrent-rasterbar.pdb → every
        # Sentry crash inside the engine DLL (the top crasher lives there) shows
        # <unknown> frames. /Z7 embeds the debug info in each .obj (unlike /Zi's
        # shared PDB, /Z7 is compatible with compiler caching / sccache); /DEBUG
        # still emits the .pdb at link time. Costs nothing at runtime.
        target_compile_options(torrent-rasterbar PRIVATE $<$<CONFIG:Release>:/Z7>)
        target_link_options(torrent-rasterbar PRIVATE
            $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)
    endif()
    message(STATUS "[BAT] libtorrent: building vendored fork from source")
else()
    find_package(LibtorrentRasterbar REQUIRED)
    set(BAT_TORRENT_LIB LibtorrentRasterbar::torrent-rasterbar)
endif()
# Optional: QtKeychain stores credentials in the platform keyring (DPAPI on
# Windows, Keychain on macOS, kwallet/libsecret on Linux). If missing at
# build time the app falls back to QSettings: secrets stay in plaintext
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
    src/app/enginechild.cpp
    src/app/mediachild.cpp
    src/app/singleinstance.cpp
    src/app/macfileopen.cpp
    src/app/boothealth.cpp
    src/app/appruntime.cpp
    src/app/steamartprovider.cpp
    src/app/appservices.cpp
    src/app/vpnwiring.cpp
    src/app/mediarefresh.cpp
    src/app/gamecatalogseed.cpp
    src/app/qmlcontextwiring.cpp
    src/app/qmlboot.cpp
    src/torrent/iengine.h        # header-only QObject — listed so AUTOMOC builds its metaobject
    src/torrent/sessionmanager.cpp
    src/torrent/addkind.cpp
    src/torrent/filetree.cpp
    src/torrent/sessionmanager_query.cpp
    src/torrent/sessionmanager_streaming.cpp
    src/torrent/sessionmanager_config.cpp
    src/torrent/sessionmanager_limits.cpp
    src/torrent/sessionmanager_apply.cpp
    src/torrent/sessionconfig.cpp
    src/torrent/sessionresume.cpp
    src/torrent/sessionmanager_persistence.cpp
    src/torrent/sessionmanager_categories.cpp
    src/torrent/sessionmanager_pertorrent.cpp
    src/torrent/sessionmanager_alerts.cpp
    src/torrent/sessionmanager_alerts_finish.cpp
    src/torrent/sessionmanager_info.cpp
    src/torrent/sessionmanager_network.cpp
    src/torrent/sessionmanager_security.cpp
    src/torrent/sessionmanager_lifecycle.cpp
    src/torrent/sessionmanager_staging.cpp
    src/torrent/sessionmanager_tick.cpp
    src/torrent/sessionmanager_scheduler.cpp
    src/torrent/sessionmanager_stats.cpp
    src/torrent/sessionmanager_import.cpp
    src/torrent/sessionmanager_extract.cpp
    src/torrent/contentprobe.cpp
    src/torrent/ipblocklist.cpp
    src/torrent/proxycontroller.cpp
    src/ipc/enginehost.cpp
    src/ipc/ipcengine.cpp
    src/ipc/mediaframe.cpp
    src/ipc/mediahost.cpp
    src/ipc/sharedsegment.cpp
    src/ipc/mediasandbox.cpp
    src/ipc/remotesource.cpp
    src/ipc/mediafeed.cpp
    src/ipc/mediachannel.cpp
    src/services/platform/translator.cpp
    src/services/platform/statshistory.cpp
    src/services/platform/soundplayer.cpp
    src/services/platform/soundsynth.cpp
    src/services/subtitles/subtitleparser.cpp
    src/services/subtitles/asstext.cpp
    src/services/subtitles/subtitlesearch.cpp
    src/services/integrations/idebridprovider.cpp
    src/services/integrations/realdebrid.cpp
    src/services/integrations/torbox.cpp
    src/services/integrations/debridmanager.cpp
    src/services/security/suspiciousscan.cpp
    src/services/security/mediasniff.cpp
    src/services/security/mediaguard.cpp
    src/services/security/archivescan.cpp
    src/services/security/archiveextractor.cpp
    src/services/metadata/releasepick.cpp
    src/services/metadata/gamereleasepick.cpp
    src/services/metadata/searchranker.cpp
    src/services/metadata/releasetrust.cpp
    src/services/integrations/debridpick.cpp
    src/services/integrations/installerprofile.cpp
    src/services/discovery/trackerscrape.cpp
    src/services/integrations/gameinstall.cpp
    src/services/games/playlog.cpp
    src/services/games/achievements.cpp
    src/services/games/achievementformats.cpp
    src/services/games/achievementfiles.cpp
    src/services/games/achievementledger.cpp
    src/services/games/achievementservice.cpp
    src/services/games/achievementwatch.cpp
    src/services/games/steamstore.cpp
    src/services/integrations/gameexedetect.cpp
    src/services/security/defender.cpp
    src/services/platform/logger.cpp
    src/services/security/crashhandler.cpp
    src/services/integrations/notifier.cpp
    src/services/platform/qrcodegen.cpp
    src/services/integrations/discordrpc.cpp
    src/services/security/passwordhash.cpp
    src/services/security/secretstore.cpp
    src/services/security/blocklistupdater.cpp
    src/services/integrations/updater.cpp
    src/services/discovery/addonmanager.cpp
    src/services/discovery/addonparse.cpp
    src/services/discovery/cinemeta.cpp
    src/services/discovery/addonfinish.cpp
    src/services/discovery/addoncatalog.cpp
    src/services/integrations/rssmanager.cpp
    src/services/integrations/geoip.cpp
    src/services/integrations/geoipdb.cpp
    src/services/integrations/geoipprovider.cpp
    src/services/metadata/audiomode.cpp
    src/services/metadata/episodegroup.cpp
    src/services/metadata/filegroups.cpp
    src/services/metadata/nameparser.cpp
    src/services/metadata/releasegroup.cpp
    src/services/platform/contentlanguage.cpp
    src/services/platform/settingsbackup.cpp
    src/services/platform/settingspolicy.cpp
    src/services/platform/fileassociation.cpp
    src/services/platform/autostart.cpp
    src/services/metadata/metadataresolver.cpp
    src/services/metadata/metadatamatch.cpp
    src/services/metadata/mkvchapters.cpp
    src/services/metadata/aniskip.cpp
    src/services/downloads/httpdownload.cpp
    src/services/downloads/httpdownloadmanager.cpp
    src/services/downloads/httpmergeengine.cpp
    src/services/vpn/wgtunnel.h            # header-only QObjects — listed so AUTOMOC builds their metaobjects
    src/services/vpn/wgtunnel_userspace.h  # WireGuard in a child process (batwg), over local SOCKS5
    src/services/vpn/wgtunnel_userspace.cpp
    src/services/vpn/vpnmanager.h
    src/services/vpn/vpnmanager.cpp
    src/services/discovery/discoveryservice.cpp
    src/services/discovery/discoveryservice_title.cpp
    src/services/discovery/discoveryservice_game.cpp
    src/services/discovery/discoveryservice_art.cpp
    src/services/discovery/logotone.cpp
    src/services/discovery/discoveryassemble.cpp
    src/services/discovery/discoverysearch.cpp
    src/services/discovery/discoveryfinish.cpp
    src/services/discovery/hublogic.cpp
    src/services/discovery/tmdbparse.cpp
    src/services/discovery/igdbparse.cpp
    src/services/discovery/gamesourcemanager.cpp
    src/bridges/qmlpostermodel.cpp
    src/bridges/librarygroups.cpp
    src/bridges/qmllibrarymodel.cpp
    src/bridges/session/qmlsessionbridge.cpp
    src/bridges/session/qmlsessionbridge_add.cpp
    src/bridges/session/qmlsessionbridge_addwindow.cpp
    src/bridges/session/qmlsessionbridge_freeup.cpp
    src/bridges/session/qmlsessionbridge_gamepage.cpp
    src/bridges/session/qmlsessionbridge_details.cpp
    src/bridges/session/qmlsessionbridge_create.cpp
    src/bridges/session/qmlsessionbridge_games.cpp
    src/bridges/session/qmlsessionbridge_gameinstall.cpp
    src/bridges/session/qmlsessionbridge_achievements.cpp
    src/bridges/session/qmlsessionbridge_library.cpp
    src/bridges/session/qmlsessionbridge_playback.cpp
    src/bridges/session/qmlsessionbridge_overview.cpp
    src/bridges/session/qmlsessionbridge_guard.cpp
    src/bridges/session/qmlsessionbridge_subtitles.cpp
    src/bridges/session/qmlsessionbridge_watch.cpp
    src/bridges/session/qmlsessionbridge_selection.cpp
    src/bridges/session/qmlsessionbridge_disk.cpp
    src/bridges/session/qmlsessionbridge_transfers.cpp
    src/bridges/session/qmlsessionbridge_actions.cpp
    src/bridges/session/qmlsessionbridge_diagnose.cpp
    src/bridges/session/qmlsessionbridge_queue.cpp
    src/bridges/session/qmlsessionbridge_clipboard.cpp
    src/bridges/session/qmlsessionbridge_stats.cpp
    src/bridges/qmlthemebridge.cpp
    src/bridges/search/qmlsearchbridge.cpp
    src/bridges/search/qmlsearchbridge_util.cpp
    src/bridges/search/qmlsearchbridge_attrs.cpp
    src/bridges/search/qmlsearchbridge_work.cpp
    src/bridges/search/qmlsearchbridge_next.cpp
    src/bridges/search/qmlsearchbridge_covers.cpp
    src/bridges/search/qmlsearchbridge_activate.cpp
    src/bridges/search/qmlsearchbridge_ranking.cpp
    src/bridges/search/qmlsearchbridge_getwatch.cpp
    src/bridges/search/qmlsearchbridge_aggregate.cpp
    src/bridges/search/qmlsearchbridge_games.cpp
    src/bridges/qmlsettingsbridge.cpp
    src/bridges/qmlrssbridge.cpp
    src/bridges/qmladdonbridge.cpp
    src/bridges/qmlpairingbridge.cpp
    src/bridges/qmlsubtitlebridge.cpp
    src/bridges/qmllogbridge.cpp
    src/bridges/qmlnotificationbridge.cpp
    src/bridges/isolatedmediaplayer.cpp
    src/bridges/discordrpcbridge.cpp
    src/bridges/qmlupdaterbridge.cpp
    # header-only QObject: listed so AUTOMOC builds its metaobject
    src/bridges/qmli18nbridge.h
    src/webui/webserver.cpp
    src/webui/streamserver.cpp
    ${RESOURCES}
    ${WIN_RC}
    ${MACOSX_APP_ICON}
)
target_compile_definitions(BATorrent PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_include_directories(BATorrent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# The in-app VPN (tools/batwg, WireGuard in its own process over a local
# SOCKS5 proxy): built with Go when Go is on the PATH, and put beside the app
# binary, where UserspaceWgTunnel looks for it. No cgo, so no C toolchain.
find_program(GO_EXECUTABLE go)
if(GO_EXECUTABLE)
    if(WIN32)
        set(BATWG_NAME batwg.exe)
        set(BATWG_LDFLAGS "-s -w -H=windowsgui")   # no console window
    else()
        set(BATWG_NAME batwg)
        set(BATWG_LDFLAGS "-s -w")
    endif()
    add_custom_target(batwg ALL
        COMMAND ${CMAKE_COMMAND} -E env CGO_ENABLED=0
                ${GO_EXECUTABLE} build -trimpath -mod=readonly -ldflags=${BATWG_LDFLAGS}
                -o "$<TARGET_FILE_DIR:BATorrent>/${BATWG_NAME}" .
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/batwg
        COMMENT "Building batwg (the in-app VPN)"
        VERBATIM)
    add_dependencies(batwg BATorrent)
else()
    message(STATUS "[BAT] Go not found: the in-app VPN (tools/batwg) is not built")
endif()

if(DEFINED ENV{BAT_TMDB_KEY})
    target_compile_definitions(BATorrent PRIVATE BAT_TMDB_KEY="$ENV{BAT_TMDB_KEY}")
endif()
if(DEFINED ENV{BAT_IGDB_ID})
    target_compile_definitions(BATorrent PRIVATE BAT_IGDB_CLIENT_ID="$ENV{BAT_IGDB_ID}")
endif()
if(DEFINED ENV{BAT_IGDB_SECRET})
    target_compile_definitions(BATorrent PRIVATE BAT_IGDB_CLIENT_SECRET="$ENV{BAT_IGDB_SECRET}")
endif()
if(DEFINED ENV{BAT_STEAM_KEY})
    target_compile_definitions(BATorrent PRIVATE BAT_STEAM_KEY="$ENV{BAT_STEAM_KEY}")
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
# can fail to allocate a menu-bar slot: Qt reports the item as visible but
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
    # internal/build-qt-ffmpeg-plugin.sh), bundle it: Qt searches the bundle's
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
    # userenv: AppContainer profiles for the sandboxed media child
    target_sources(BATorrent PRIVATE src/ipc/appcontainer_win.cpp)
    target_link_libraries(BATorrent PRIVATE userenv)

    # Emit a PDB even in Release so Store / Partner Center crash dumps symbolicate.
    # Without symbols every Store crash is reported as "unknown" (we had 314/322
    # like that). /OPT:REF,ICF restore the size optimisations that /DEBUG disables,
    # so the shipped binary is unchanged in behaviour/size: only a .pdb is added.
    if(MSVC)
        target_compile_options(BATorrent PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Zi>)
        target_link_options(BATorrent PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:/DEBUG>
            $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
            $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>)
    endif()
endif()

# Windows: the main window draws its own title bar (the navigation bar doubles
# as one) through QWindowKit, which keeps snap layouts, the system resize
# borders and Aero shake. macOS and Linux keep the native title bar.
if(WIN32)
    option(BAT_FRAMELESS "Merge the title bar into the navigation bar (QWindowKit)" ON)
else()
    set(BAT_FRAMELESS OFF)
endif()
if(BAT_FRAMELESS)
    include(FetchContent)
    set(QWINDOWKIT_BUILD_QUICK ON CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_WIDGETS OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(QWINDOWKIT_INSTALL OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_ENABLE_STYLE_AGENT OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(qwindowkit
        GIT_REPOSITORY https://github.com/stdware/qwindowkit.git
        GIT_TAG 1.5.0
        GIT_SHALLOW TRUE
        GIT_SUBMODULES qmsetup)
    FetchContent_MakeAvailable(qwindowkit)
    target_link_libraries(BATorrent PRIVATE QWindowKit::Quick)
    target_compile_definitions(BATorrent PRIVATE BAT_FRAMELESS)
endif()

# Linux: install rules for icon and .desktop file
if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS BATorrent RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    if(GO_EXECUTABLE)
        install(PROGRAMS "$<TARGET_FILE_DIR:BATorrent>/${BATWG_NAME}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

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
            # No ImageMagick: fall back to copying the 1024px source. The
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
