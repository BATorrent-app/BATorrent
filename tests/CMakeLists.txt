# =============================================================================
#  BATorrent Test Suite
#  Framework: Catch2 v3
#  Tools:     ASan, MSVC /analyze, CppCheck, CRT Debug Heap, Dr. Memory
# =============================================================================
enable_testing()

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg Test Multimedia)
# In fork mode the tests must exercise the SAME vendored engine the app ships
# (BAT_TORRENT_LIB set by the root CMakeLists), not the system libtorrent.
if(NOT BAT_LIBTORRENT_SOURCE)
    find_package(LibtorrentRasterbar REQUIRED)
endif()

# --- Fetch Catch2 v3 ---
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

# --- Common sources (everything except main.cpp) ---
set(COMMON_SOURCES
    ${CMAKE_SOURCE_DIR}/src/torrent/iengine.h        # header-only QObject — listed so AUTOMOC builds its metaobject
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_streaming.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_config.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_persistence.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_categories.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_pertorrent.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_alerts.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_info.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_network.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/sessionmanager_security.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/ipblocklist.cpp
    ${CMAKE_SOURCE_DIR}/src/torrent/proxycontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/services/security/crashhandler.cpp     # CrashHandler::lastReport (used by qmlposterbridge)
    ${CMAKE_SOURCE_DIR}/src/services/platform/translator.cpp
    ${CMAKE_SOURCE_DIR}/src/services/platform/statshistory.cpp
    ${CMAKE_SOURCE_DIR}/src/services/platform/soundplayer.cpp
    ${CMAKE_SOURCE_DIR}/src/services/subtitles/subtitleparser.cpp
    ${CMAKE_SOURCE_DIR}/src/services/subtitles/subtitlesearch.cpp
    ${CMAKE_SOURCE_DIR}/src/services/security/suspiciousscan.cpp
    ${CMAKE_SOURCE_DIR}/src/services/security/archivescan.cpp
    ${CMAKE_SOURCE_DIR}/src/services/security/archiveextractor.cpp
    ${CMAKE_SOURCE_DIR}/src/services/metadata/releasepick.cpp
    ${CMAKE_SOURCE_DIR}/src/services/metadata/searchranker.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/debridpick.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/installerprofile.cpp
    ${CMAKE_SOURCE_DIR}/src/services/security/defender.cpp
    ${CMAKE_SOURCE_DIR}/src/webui/webserver.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/rssmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/discovery/addonmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/updater.cpp
    ${CMAKE_SOURCE_DIR}/src/services/platform/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/services/metadata/audiomode.cpp
    ${CMAKE_SOURCE_DIR}/src/services/metadata/episodegroup.cpp
    ${CMAKE_SOURCE_DIR}/src/services/metadata/nameparser.cpp
    ${CMAKE_SOURCE_DIR}/src/services/discovery/gamesourcemanager.cpp
    # C++<->QML bridge layer (split per domain) + its backend deps (for test_bridge)
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlposterbridge.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlpostermodel.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsessionbridge.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsessionbridge_games.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsessionbridge_library.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsessionbridge_playback.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsessionbridge_selection.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsessionbridge_disk.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlthemebridge.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsearchbridge.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsettingsbridge.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlrssbridge.h          # AUTOMOC: classes implemented in qmlposterbridge.cpp
    ${CMAKE_SOURCE_DIR}/src/bridges/qmladdonbridge.h
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlpairingbridge.h
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlsubtitlebridge.h
    ${CMAKE_SOURCE_DIR}/src/bridges/qmllogbridge.h
    ${CMAKE_SOURCE_DIR}/src/bridges/qmli18nbridge.h
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlnotificationbridge.h
    ${CMAKE_SOURCE_DIR}/src/bridges/discordrpcbridge.h
    ${CMAKE_SOURCE_DIR}/src/bridges/qmlupdaterbridge.h
    ${CMAKE_SOURCE_DIR}/src/services/metadata/metadataresolver.cpp
    ${CMAKE_SOURCE_DIR}/src/services/discovery/discoveryservice.cpp
    ${CMAKE_SOURCE_DIR}/src/services/platform/qrcodegen.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/geoip.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/geoipdb.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/geoipprovider.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/discordrpc.cpp
    ${CMAKE_SOURCE_DIR}/src/services/integrations/notifier.cpp
    ${CMAKE_SOURCE_DIR}/src/services/security/passwordhash.cpp
    ${CMAKE_SOURCE_DIR}/src/services/security/secretstore.cpp
)

find_package(ZLIB REQUIRED)
set(COMMON_LIBS
    Qt6::Widgets Qt6::Network Qt6::Svg Qt6::Test Qt6::Multimedia
    ${BAT_TORRENT_LIB}
    Catch2::Catch2WithMain
    ZLIB::ZLIB
)
if(WIN32)
    list(APPEND COMMON_LIBS shell32 ole32)   # shell ops pulled in via the bridge deps
endif()

set(COMMON_DEFS APP_VERSION="test")
if(WIN32)
    list(APPEND COMMON_DEFS _WIN32_WINNT=0x0601)
endif()

# =============================================================================
#  OPTIONS
# =============================================================================
option(BAT_ASAN       "Enable AddressSanitizer"              OFF)
option(BAT_CPPCHECK   "Enable CppCheck static analysis"      OFF)
option(BAT_MSVC_ANALYZE "Enable MSVC /analyze"               OFF)
option(BAT_CRT_DEBUG  "Enable Windows CRT Debug Heap"        OFF)
option(BAT_DRMEMORY   "Add Dr. Memory wrapper targets"       OFF)

# =============================================================================
#  TOOL: AddressSanitizer (ASan)
# =============================================================================
if(BAT_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        # MSVC ASan doesn't need a link flag, but incompatible with /RTC
        string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG}")
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address)
    endif()
    message(STATUS "[BAT] AddressSanitizer ENABLED")
endif()

# =============================================================================
#  TOOL: MSVC Code Analysis (/analyze)
# =============================================================================
if(BAT_MSVC_ANALYZE AND MSVC)
    add_compile_options(/analyze /analyze:external- /W4)
    message(STATUS "[BAT] MSVC /analyze ENABLED")
endif()

# =============================================================================
#  TOOL: CppCheck
# =============================================================================
if(BAT_CPPCHECK)
    find_program(CPPCHECK_EXE cppcheck)
    if(CPPCHECK_EXE)
        set(CMAKE_CXX_CPPCHECK
            ${CPPCHECK_EXE}
            --enable=warning,performance,portability
            --suppress=missingIncludeSystem
            --inline-suppr
            --quiet
        )
        message(STATUS "[BAT] CppCheck ENABLED: ${CPPCHECK_EXE}")
    else()
        message(WARNING "[BAT] CppCheck requested but not found in PATH")
    endif()
endif()

# =============================================================================
#  TOOL: Windows CRT Debug Heap
# =============================================================================
if(BAT_CRT_DEBUG AND WIN32)
    list(APPEND COMMON_DEFS BAT_CRT_DEBUG_HEAP)
    message(STATUS "[BAT] CRT Debug Heap ENABLED")
endif()

# =============================================================================
#  Helper: create a test target
# =============================================================================
function(bat_add_test NAME SOURCE)
    # Qt MOC needs AUTOMOC
    set(CMAKE_AUTOMOC ON)
    qt_add_executable(${NAME} ${SOURCE} ${COMMON_SOURCES})
    target_compile_definitions(${NAME} PRIVATE ${COMMON_DEFS})
    target_link_libraries(${NAME} PRIVATE ${COMMON_LIBS})
    target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    catch_discover_tests(${NAME})

    # --- Dr. Memory wrapper target ---
    if(BAT_DRMEMORY AND WIN32)
        find_program(DRMEMORY_EXE drmemory HINTS "C:/Program Files (x86)/Dr. Memory/bin")
        if(DRMEMORY_EXE)
            add_custom_target(${NAME}_drmemory
                COMMAND ${DRMEMORY_EXE}
                    -batch
                    -brief
                    -no_follow_children
                    -suppress ${CMAKE_CURRENT_SOURCE_DIR}/drmemory_suppress.txt
                    -- $<TARGET_FILE:${NAME}>
                DEPENDS ${NAME}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running Dr. Memory on ${NAME}..."
            )
        endif()
    endif()
endfunction()

# =============================================================================
#  TEST TARGETS
# =============================================================================
# libFuzzer harness for the untrusted-input parsers (clang only).
# Configure with -DBAT_FUZZ=ON, then: ./fuzz_nameparser -max_total_time=30
option(BAT_FUZZ "Build libFuzzer harnesses" OFF)
if(BAT_FUZZ)
    add_executable(fuzz_nameparser
        ${CMAKE_CURRENT_SOURCE_DIR}/fuzz_nameparser.cpp
        ${CMAKE_SOURCE_DIR}/src/services/metadata/nameparser.cpp
        ${CMAKE_SOURCE_DIR}/src/services/security/suspiciousscan.cpp
        ${CMAKE_SOURCE_DIR}/src/services/security/archivescan.cpp)
    target_compile_options(fuzz_nameparser PRIVATE -fsanitize=fuzzer,address,undefined -g)
    target_link_options(fuzz_nameparser PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_libraries(fuzz_nameparser PRIVATE Qt6::Core)
    target_include_directories(fuzz_nameparser PRIVATE ${CMAKE_SOURCE_DIR}/src)
    message(STATUS "[BAT] Fuzz harnesses ENABLED")
endif()

bat_add_test(test_security   test_security.cpp)
bat_add_test(test_memory     test_memory.cpp)
bat_add_test(test_unit       test_unit.cpp)
bat_add_test(test_nameparser test_nameparser.cpp)
bat_add_test(test_episodegroup test_episodegroup.cpp)
bat_add_test(test_bridge     test_bridge.cpp)
bat_add_test(test_gamesource test_gamesource.cpp)
bat_add_test(test_memguard   test_memguard.cpp)
bat_add_test(test_geoipdb    test_geoipdb.cpp)
bat_add_test(test_searchranker test_searchranker.cpp)
bat_add_test(test_ipblocklist  test_ipblocklist.cpp)
bat_add_test(test_bandwidthschedule test_bandwidthschedule.cpp)
bat_add_test(test_proxycontroller test_proxycontroller.cpp)
bat_add_test(test_ipcprotocol test_ipcprotocol.cpp)
bat_add_test(test_debrid     test_debrid.cpp)

# =============================================================================
#  QML layer (Qt Quick Test) — drives real .qml components headless (offscreen).
#  Separate from the Catch2 suites: links the app resources qrc (so
#  qrc:/src/qml/* + the Theme singleton resolve) instead of COMMON_SOURCES.
# =============================================================================
find_package(Qt6 QUIET COMPONENTS Quick QuickTest Qml)
if(Qt6Quick_FOUND AND Qt6QuickTest_FOUND)
    set(CMAKE_AUTOMOC ON)
    qt_add_resources(QML_TEST_RES ${CMAKE_SOURCE_DIR}/src/resources.qrc)
    qt_add_executable(test_qml ${CMAKE_CURRENT_SOURCE_DIR}/qml/main.cpp ${QML_TEST_RES})
    target_compile_definitions(test_qml PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml")
    target_link_libraries(test_qml PRIVATE Qt6::Quick Qt6::QuickTest Qt6::Qml Qt6::Gui)
    add_test(NAME test_qml COMMAND test_qml)
    # Offscreen QPA so it runs in CI / over SSH with no display.
    set_tests_properties(test_qml PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    list(APPEND BAT_QML_TEST_DEP test_qml)
else()
    message(STATUS "[BAT] Qt Quick Test not found — skipping test_qml")
endif()

# =============================================================================
#  Convenience: run all analysis tools
# =============================================================================
add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    DEPENDS test_security test_memory test_unit test_nameparser test_bridge test_gamesource
    COMMENT "Running all BATorrent tests..."
)
